static const char USAGE[] =
R"(
Armstrong.
Usage:
  armstrong [options] <input>

Options:
   -v --verbose          Verbose output.
   -f --feasibility=<m>  Feasibility method. One of: round, greedy, anneal, grid, none
   --carto               Preprocess with linear cartogram
   -m --steps=<n>        Number of steps for quality annealing. [default: 10000]
   -t --temp=<x>         Initial temperature for quality annealing.
   --mintemp=<x>         Minimum temperature for quality annealing. [default: 0]
   -c --cooling=<x>      Cooling factor during quality annealing.
   --autocool            Automaticly pick cooling factor such that c^{steps} * temp = mintemp
   -g --grid=<g>         Scale input for this grid size.
   --hillclimb           Apply hill climbing after quality annealing [default: no]
   --nocenter            Do not center the input network.
   -o --output=<file>    Output filename, otherwise to stdout.
   -d --dump             Write intermediate results to file.
   -h --help             Show this screen.
)";

#include <functional>
#include <algorithm>
#include <fstream>
#include <vector>
#include <tuple>
#include <map>
using namespace std;

#include <random>
using RandomEngine = std::mt19937;

#define DOCOPT_HEADER_ONLY
#include <stdexcept> // may need to help docopt
#include "docopt.h"

#include "shapefil.h"

#include "logging.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "Vertex.h"
#include "Edge.h"
#include "BinnedGeometry.h"
#include "geometry_help.h"
#include "annealing_help.h"
#include "cartogram_preprocess.h"
#include "Greedy.h"
#include "Checkpoint.h"
#include "Timer.h"
#include "Logging.h"
#include "LinearProgress.h"

#include "load_shapefile.h"
#include "agf_file.h"
#include "write_svg.h"

#include "density_annealing.h"


double evaluate_rounding_cost(const vector<Vertex*>& vertices) {
	double score = 0;
	for (Vertex* v : vertices) score += v->rounding_cost();
	return score;
}

void handle_docopt_double(string_view message, double& result, const docopt::value& val) {
	if (val) {
		if (val.isLong()) {
			// does not seem to happen; it's a string.
			result = val.asLong();
		}
		else if (val.isString()) {
			string s = val.asString();
			size_t pos = s.length();
			double d = std::stod(s, &pos);
			if (pos == s.length()) {
				result = d;
			}
			else {
				console->warn("Expected number for '{}' but could not interpret \"{}\"; ignored.", message, s);
			}
		}
		else if (val.isBool()) {
			console->warn("Expected number for '{}' but got boolean {}; ignored.", message, val.asBool());
		}
	}
	// Do not do anything if value is missing; this is not a warning.
}

int main(int argc, char** argv) {
	// Parse command line arguments.
	auto args = docopt::docopt(USAGE, { argv + 1, argv + argc }, true, "Align");

	// Setup logging console and handle verbose flag.
	console = spdlog::stderr_color_mt("main");
	if (args["--verbose"].asBool()) console->set_level(spdlog::level::info);
	else console->set_level(spdlog::level::err);

	Vertices vertices;
	Edges edges;

	// Load and normalise input
	string shapefile_extension = ".shp";
	string agf_extension = ".agf";
	string graph_filename = args["<input>"].asString();
	if (0 == graph_filename.compare(graph_filename.length() - shapefile_extension.length(), shapefile_extension.length(), shapefile_extension)) {
		if (!load_shapefile(graph_filename, vertices, edges)) return -1;
	}
	if (0 == graph_filename.compare(graph_filename.length() - agf_extension.length(), agf_extension.length(), agf_extension)) {
		if (!load_agffile(graph_filename, vertices, edges)) return -1;
	}
	double min_x = numeric_limits<double>::max();
	double min_y = numeric_limits<double>::max();
	double max_x = numeric_limits<double>::lowest();
	double max_y = numeric_limits<double>::lowest();
	for (Vertex* v : vertices) {
		min_x = std::min(min_x, v->current.x);
		max_x = std::max(max_x, v->current.x);
		min_y = std::min(min_y, v->current.y);
		max_y = std::max(max_y, v->current.y);
	}
	double width = max_x - min_x;
	double height = max_y - min_y;
	double extent = std::max(width, height);

	if (args["--nocenter"].asBool()) {}
	else {
		console->info("Centering input graph (old center was: {} {})", (width / 2) + min_x, (height / 2) + min_y);
		for (Vertex* v : vertices) {
			v->current.x -= (width / 2) + min_x;
			v->current.y -= (height / 2) + min_y;
			v->original.x -= (width / 2) + min_x;
			v->original.y -= (height / 2) + min_y;
		}
	}

	// rescale
	int grid_size = static_cast<int>(extent); // integer unit grid by default 
	if (args["--grid"]) {
		grid_size = args["--grid"].asLong();
		for (Vertex* v : vertices) {
			v->current.x = (v->current.x / extent) * grid_size;
			v->current.y = (v->current.y / extent) * grid_size;
			v->original.x = (v->original.x / extent) * grid_size;
			v->original.y = (v->original.y / extent) * grid_size;
		}
	}

	// set up random source
	std::random_device random_device;
	RandomEngine rng(random_device());
	std::uniform_int_distribution<std::mt19937::result_type> random_vertex(0, vertices.size() - 1);

	// --feasibility
	function<void()> ensure_feasible;
	auto feasibility_arg = args["--feasibility"];
	if (feasibility_arg.isString()) {
		string arg = feasibility_arg.asString();
		if (arg == "round") {
			console->info("Feasibility method: rounding coordinates.");
			ensure_feasible = [&]() {
				scale_and_round(vertices, edges);
			};
		}
		else if (arg == "greedy") {
			console->info("Feasibility method: greedy heuristic.");
			ensure_feasible = [&]() {
				scale_and_greedy(vertices, edges);
			};
		}
		else if (arg == "anneal") {
			console->info("Feasibility method: annealing with continuous density.");
			ensure_feasible = [&]() {
				density_annealing(vertices, edges, [&]() { return evaluate_density(vertices); }, rng);
			};
		}
		else if (arg == "grid") {
			console->info("Feasibility method: annealing with grid density.");
			ensure_feasible = [&]() {
				density_annealing(vertices, edges, [&]() { return evaluate_grid_density(vertices); }, rng);
			};
		}
		else if (arg == "cost") {
			console->info("Feasibility method: cost.");
			ensure_feasible = [&]() {
				density_annealing(vertices, edges, [&]() { return evaluate_rounding_cost(vertices); }, rng);
			};
		}
		else if (arg == "none") {
			console->info("Feasibility method: none. Input drawing should be feasible.");
			ensure_feasible = []() { return; };
		}
		else {
			console->error("Did not recognise '{}' as feasibility method. Will skip feasibility phase.", arg);
			ensure_feasible = []() { return; };
		}
	}
	else {
		console->warn("No feasibility method indicated; things will be bad if input is not feasible.");
		ensure_feasible = []() { return; };
	}

	// --max-steps
	int max_iterations = args["--steps"].asLong(); // has docopt default

	// --temperature
	double temperature = 1.0; // default is set here
	handle_docopt_double("--temp", temperature, args["--temp"]);

	// --minimum temperature
	double min_temperature = 0.0; // default is set here
	handle_docopt_double("--mintemp", min_temperature, args["--mintemp"]);

	// --cooling
	double cooling = 0.99; // default is set here
	handle_docopt_double("--cooling", cooling, args["--cooling"]);

	if (args["--autocool"].asBool()) {
		cooling = exponential_schedule(temperature, min_temperature, max_iterations);
		console->info("Setting cooling schedule from {} to {} in {} steps (factor {})", temperature, min_temperature, max_iterations, cooling);
	}

	bool hillclimb = args["--hillclimb"].asBool();
	if (hillclimb) {
		console->info("Postprocess hillclimbing enabled.");
	}

	// --output
	ofstream output_file;
	bool output_to_file = false;
	if (args["--output"].isString()) {
		output_to_file = true;
		output_file.open(args["--output"].asString());
	}
	ostream& output = output_to_file ? output_file : std::cout;

	// === Check input. ===

	console->info("Setting up embeddings...");
	for (Vertex* v : vertices) {
		v->set_rotsys();
	}

	console->info("Testing every rotsys...");
	for (Vertex* v : vertices) {
		if (!v->rotsys_valid()) {
			console->error("Input does not pass rotsys check. Things are going to be bad.");
			break;
		}
	}

	console->info("Testing for intersections...");
	BinnedGeometry geom_checker;
	if (!geom_checker.check_intersections(vertices, edges)) {
		console->error("Input does not pass geometry check. Things are going to be bad.");
	}

	// === Work. ===

	// apply linear cartogram preprocessing?
	if (args["--carto"].asBool()) {
		console->info("Applying linear cartogram...");
		apply_cartogram(vertices, edges, true, true, true);
		if (!check_valid_full(vertices, edges)) {
			console->error("Drawing no longer valid after cartogram. Things are going to be bad.");
		}
	}
	auto positions_after_preprocessing = backup_vertices(vertices);

	// turn input graph into SOME grid drawing
	ensure_feasible();
	auto positions_first_feasible = backup_vertices(vertices);
	{
		double score = evaluate_rounding_cost(vertices);
		console->info("Average cost per vertex: {}", score / vertices.size());
	}
	if (args["--dump"].asBool()) {
		write_agffile("feasible.agf", vertices, edges);
	}

	// sanity check: is the "ensured feasible" drawing actually valid?
	for (Vertex* v : vertices) {
		if (!v->rotsys_valid()) {
			console->error("Supposedly feasible drawing does not pass rotsys check. Things are going to be bad.");
			break;
		}
	}
	if (!geom_checker.check_intersections(vertices, edges)) {
		console->error("Supposedly feasible drawing does not pass geometry check. Things are going to be bad.");
	}

	// anneal for quality
	double score = evaluate_rounding_cost(vertices);
	int annealing_iteration = 0;
	console->info("================== Annealing for quality.");
	LinearProgress progress_report("Annealing ", "iterations", max_iterations);
	progress_report.start();
	int recent_rejections = 0;
	vector<int> vertex_weights(vertices.size());
	while (annealing_iteration < max_iterations) {
		progress_report.tick(score);
		++annealing_iteration;
		temperature *= cooling;

		// if temperature drops below threshold, disable cooling.
		if (temperature < min_temperature) {
			console->info("Minimum Temperature reached; staying at {}", min_temperature);
			temperature = min_temperature;
			cooling = 1.0;
		}

		// pick random vertex uniform
		Vertex* v = vertices[random_vertex(rng)];

		// mutate current solution, but be able to undo it.
		Checkpoint checkpoint(v->current);
		v->mutate(rng);

		if (check_valid_after_move(v, vertices, edges)) {
			// "annealing" decision whether to accept move
			double new_score = evaluate_rounding_cost(vertices);
			if (accept_move(temperature, score, new_score, rng)) {
				score = new_score;
				checkpoint.commit();
				recent_rejections = 0;
			}
			else {
				// rejected annealing step
				// checkpoint will reset vertex
				++recent_rejections;
			}
		}

	}
	progress_report.done(score);
	console->info("================== Annealed for {} iterations.", annealing_iteration);
	auto positions_after_annealing = backup_vertices(vertices);

	console->info("Average cost per vertex: {}", score / vertices.size());

	// hillclimb to local optimum
	if (hillclimb) {
		console->info("================== Hillclimbing for quality.");
		LinearProgress climbing_progress("Hillclimbing ", "rounds", 0);
		int climb_iteration = 0;
		bool changed = false;
		do {
			climbing_progress.tick(0);
			++climb_iteration;
			changed = false;
			for (Vertex* v : vertices) {
				while (v->climb(vertices, edges)) { changed = true; }
			}
		} while (changed);
		console->info("================== Hillclimbed for {} rounds.", climb_iteration);
		score = evaluate_rounding_cost(vertices);
		console->info("Average cost per vertex: {}", score / vertices.size());
	}

	console->info("Final average cost per vertex: {}", score / vertices.size());

	const string svg_filename = "output.svg";
	console->info("Writing {} ...", svg_filename);
	write_svg(vertices, edges, positions_after_preprocessing, positions_first_feasible, positions_after_annealing, svg_filename);

}
