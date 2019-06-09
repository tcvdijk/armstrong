#include "Greedy.h"

using std::vector;

#include "Vertex.h"
#include "Edge.h"
#include "geometry_help.h"

#include "Logging.h"

void scale_and_greedy(vector<Vertex*>& vertices, vector<Edge*>& edges) {
	console->info("Running scale-and-greedy...");
	double factor = 0.0;
	bool done = false;
	while (!done) {
		factor += 1.0;
		console->info("Scale-and-greedy trying factor {}...", factor);
		// scale everybody
		for (Vertex* v : vertices) {
			v->current.x = factor * v->original.x;
			v->current.y = factor * v->original.y;
		}
		// round vertices one by one
		done = true; // provisionally think we're done
		for (Vertex* v : vertices) {
			if (!attempt_greedy(v, vertices, edges)) {
				done = false;
				break;
			}
		}
	}
	for (Vertex* v : vertices) {
		v->is_rounded = true;
	}
	console->info("Scale-and-greedy successful.");
}
void scale_and_round(vector<Vertex*>& vertices, vector<Edge*>& edges) {
	console->info("Running scale-and-round...");
	double factor = 0.0;
	while (true) {
		factor += 1.0;
		console->info("Scale-and-round trying factor {}...", factor);
		// scale and round
		for (Vertex* v : vertices) {
			v->current.x = std::round(factor * v->original.x);
			v->current.y = std::round(factor * v->original.y);
		}
		if (check_valid_full(vertices, edges)) break;
	}
	for (Vertex* v : vertices) {
		v->is_rounded = true;
	}
	console->info("Scale-and-round successful.");
}