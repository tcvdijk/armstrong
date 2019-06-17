#include "write_svg.h"
#include "Edge.h"
#include "Logging.h"

#include <fmt/format.h>

#include <fstream>
using std::ofstream;

#include <limits>
using std::numeric_limits;

using std::vector;
using std::string;

void write_svg(vector<Vertex*>& vertices, vector<Edge*>& edges,
	vector<Vertex::Point>& positions_after_preprocessing,
	vector<Vertex::Point>& positions_first_feasible,
	vector<Vertex::Point>& positions_after_annealing,
	const string& filename) {

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
	ofstream svg(filename);
	int width = std::ceil(max_x - min_x);
	int height = std::ceil(max_y - min_y);
	if (width > 1000 || height > 1000) console->warn("Grid is very big; SVG file will be unwieldy.");
	svg << fmt::format(R"(<svg viewbox="-1 -1 {} {}" xmlns="http://www.w3.org/2000/svg" xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape">)",
		std::ceil(-min_x + max_x) + 3,
		std::ceil(-min_y + max_y) + 3
	);
	svg << R"(
	<defs>
		<style type="text/css"><![CDATA[
		#grid {
			stroke: lightgray;
			stroke-width: 0.05;
		}
		#original {
			stroke: pink;
			fill: pink;
			stroke-width: 0.15;
			stroke-linecap: round;
		}
		#preprocessed{
			stroke: red;
			fill: red;
			stroke-width: 0.05;
			stroke-linecap: round;
			display:none
		}
		#feasible {
			stroke: green;
			fill: green;
			stroke-width: 0.05;
			stroke-linecap: round;
			display:none
		}
		#annealed {
			stroke: blue;
			fill: blue;
			stroke-width: 0.05;
			stroke-linecap: round;
			display:none
		}
		#solution {
			stroke: black;
			fill: black;
			stroke-width: 0.05;
			stroke-linecap: round;
		}
	]]></style>
	</defs>)" << '\n';
	svg << R"(<g inkscape:label="Grid" inkscape:groupmode="layer" id="grid">)";
	for (int x = 0; x <= -min_x + max_x + 1; ++x) {
		svg << fmt::format(R"(<line x1="{0}" x2="{0}" y1="{1}" y2="{2}"/>)", x, 0, std::ceil(-min_y + max_y + 1)) << '\n';
	}
	for (int y = 0; y <= -min_y + max_y + 1; ++y) {
		svg << fmt::format(R"(<line x1="{1}" x2="{2}" y1="{0}" y2="{0}"/>)", y, 0, std::ceil(-min_x + max_x + 1)) << '\n';
	}
	svg << "</g>\n";
	svg << R"(<g inkscape:label="Original" inkscape:groupmode="layer" id="original">)";
	for (Edge* e : edges) {
		svg << fmt::format(R"(<line x1="{}" y1="{}" x2="{}" y2="{}"/>)",
			-min_x + e->a->original.x,
			-min_y - e->a->original.y,
			-min_x + e->b->original.x,
			-min_y - e->b->original.y
		) << '\n';
	}
	for (Vertex* v : vertices) {
		svg << fmt::format(R"(<circle cx="{}" cy="{}" r="0.05" />)",
			-min_x + v->original.x,
			-min_y - v->original.y
		) << '\n';
	}
	svg << "</g>\n";
	svg << R"(<g inkscape:label="Preprocessed" inkscape:groupmode="layer" id="preprocessed">)";
	for (Edge* e : edges) {
		svg << fmt::format(R"(<line x1="{}" y1="{}" x2="{}" y2="{}"/>)",
			-min_x + positions_after_preprocessing[e->a->id].x,
			-min_y - positions_after_preprocessing[e->a->id].y,
			-min_x + positions_after_preprocessing[e->b->id].x,
			-min_y - positions_after_preprocessing[e->b->id].y
		) << '\n';
	}
	for (Vertex* v : vertices) {
		svg << fmt::format(R"(<circle cx="{}" cy="{}" r="0.05" />)",
			-min_x + positions_after_preprocessing[v->id].x,
			-min_y - positions_after_preprocessing[v->id].y
		) << '\n';
	}
	svg << "</g>\n";
	svg << R"(<g inkscape:label="Feasible" inkscape:groupmode="layer" id="feasible">)";
	for (Edge* e : edges) {
		svg << fmt::format(R"(<line x1="{}" y1="{}" x2="{}" y2="{}"/>)",
			-min_x + positions_first_feasible[e->a->id].x,
			-min_y - positions_first_feasible[e->a->id].y,
			-min_x + positions_first_feasible[e->b->id].x,
			-min_y - positions_first_feasible[e->b->id].y
		) << '\n';
	}
	for (Vertex* v : vertices) {
		svg << fmt::format(R"(<circle cx="{}" cy="{}" r="0.05" />)",
			-min_x + positions_first_feasible[v->id].x,
			-min_y - positions_first_feasible[v->id].y
		) << '\n';
	}
	svg << "</g>\n";
	svg << R"(<g inkscape:label="Annealed" inkscape:groupmode="layer" id="annealed">)";
	for (Edge* e : edges) {
		svg << fmt::format(R"(<line x1="{}" y1="{}" x2="{}" y2="{}"/>)",
			-min_x + positions_after_annealing[e->a->id].x,
			-min_y - positions_after_annealing[e->a->id].y,
			-min_x + positions_after_annealing[e->b->id].x,
			-min_y - positions_after_annealing[e->b->id].y
		) << '\n';
	}
	for (Vertex* v : vertices) {
		svg << fmt::format(R"(<circle cx="{}" cy="{}" r="0.05" />)",
			-min_x + positions_after_annealing[v->id].x,
			-min_y - positions_after_annealing[v->id].y
		) << '\n';
	}
	svg << "</g>\n";
	svg << R"(<g inkscape:label="Solution" inkscape:groupmode="layer" id="solution">)";
	for (Edge* e : edges) {
		svg << fmt::format(R"(<line x1="{}" y1="{}" x2="{}" y2="{}"/>)",
			-min_x + e->a->current.x,
			-min_y - e->a->current.y,
			-min_x + e->b->current.x,
			-min_y - e->b->current.y
		) << '\n';
	}
	for (Vertex* v : vertices) {
		svg << fmt::format(R"(<circle cx="{}" cy="{}" r="0.05" />)",
			-min_x + v->current.x,
			-min_y - v->current.y
		) << '\n';
	}
	svg << "</g>\n";
	svg << "</svg>\n";
}
