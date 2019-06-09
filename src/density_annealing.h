#ifndef INCLUDED_DENSITY_ANNEALING
#define INCLUDED_DENSITY_ANNEALING

#include <vector>
#include <limits>
#include <functional>
#include "Vertex.h"
#include "Edge.h"
#include "Logging.h"
#include "annealing_help.h"
#include "geometry_help.h"
#include "LinearProgress.h"
#include "Checkpoint.h"

double evaluate_density(const std::vector<Vertex*>& vertices);
double evaluate_grid_density(const std::vector<Vertex*>& vertices);
double evaluate_rounding_cost(const std::vector<Vertex*>& vertices);

template<typename RNG>
void density_annealing(std::vector<Vertex*>& vertices, std::vector<Edge*>& edges, std::function<double()> evaluate_score, RNG& rng) {
	int max_iterations = std::numeric_limits<int>::max();
	double temperature = 1.0;
	double cooling = 1.0;
	int num_rounded = 0;
	for (Vertex* v : vertices) {
		if (v->is_rounded) ++num_rounded;
	}
	if (num_rounded == vertices.size()) {
		console->info("Input already was feasible drawing.");
		return;
	}

	double score = evaluate_score();
	int iteration = 0;
	console->info("================== Annealing for feasibility.");
	LinearProgress progress_report("Annealing ", "iterations", 0);
	progress_report.start();

	std::vector<double> vertex_weights(vertices.size());

	while (iteration < max_iterations) {
		progress_report.tick(num_rounded);
		++iteration;
		temperature *= cooling;
		// attempt greedy on each unrounded vertex
		bool greedy_changed_something = false;
		for (Vertex* v : vertices) {
			if (!v->is_rounded) {
				if (attempt_greedy(v, vertices, edges)) {
					++num_rounded;
					greedy_changed_something = true;
				}
			}
		}
		if (greedy_changed_something) {
			score = evaluate_score();
		}
		// we are done if all vertices are feasible
		if (num_rounded == vertices.size()) {
			progress_report.done(num_rounded);
			console->info("================== Found feasible drawing by greedy after {} iterations.", iteration);
			return;
		}
		// pick random vertex
		for (Vertex* v : vertices) {
			vertex_weights[v->id] = v->density;
			if (!v->is_rounded) {
				vertex_weights[v->id] *= 10;
			}
			else if (iteration % 2 == 1) vertex_weights[v->id] = 0;
		}
		std::discrete_distribution<> vertex_distribution(vertex_weights.begin(), vertex_weights.end());
		Vertex* v = vertices[vertex_distribution(rng)];

		// mutate current solution, but be able to undo it.
		Checkpoint checkpoint(v->current);
		v->mutate(rng);

		if (check_valid_after_move(v, vertices, edges)) {
			double new_score = evaluate_score();
			bool was_already_rounded = v->is_rounded;
			v->set_rounded_state();
			if (!was_already_rounded && v->is_rounded) {
				// always accept if we round a vertex
				++num_rounded;
				score = new_score;
				checkpoint.commit();
				if (num_rounded == vertices.size()) {
					progress_report.done(num_rounded);
					console->info("Found feasible drawing in {} iterations.", iteration);
					return;
				}
			}
			else {
				// "annealing" decision whether to accept move
				if (accept_move(temperature, score, new_score, rng)) {
					score = new_score;
					checkpoint.commit();
				}
				else {
					// rejected annealing step
					// checkpoint will reset vertex
				}
			}
		}
	}
	progress_report.done(num_rounded);
	console->error("Density annealing failed to find feasible solution after {} iterations; things are going to be bad.", max_iterations);
}

#endif //ndef INCLUDED_DENSITY_ANNEALING
