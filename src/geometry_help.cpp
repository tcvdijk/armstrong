#include "geometry_help.h"
#include "BinnedGeometry.h"

using std::vector;

#include "Vertex.h"
#include "Checkpoint.h"

vector<Vertex::Point> backup_vertices(const vector<Vertex*>& vertices) {
	vector<Vertex::Point> result;
	result.reserve(vertices.size());
	for (Vertex* v : vertices) {
		result.push_back({ v->current.x, v->current.y });
	}
	return result;
}

bool attempt_move(Vertex* v, double x, double y, const std::vector<Vertex*>& vertices, const std::vector<Edge*>& edges) {
	Checkpoint attempt(v->current);
	v->current.x = x;
	v->current.y = y;
	if (check_valid_after_move(v, vertices, edges)) {
		v->set_rounded_state();
		attempt.commit();
		return true;
	}
	return false;
}
bool attempt_greedy(Vertex* v, const std::vector<Vertex*>& vertices, const std::vector<Edge*>& edges) {
	// rounding is cheapest
	if (attempt_move(v, std::round(v->current.x), std::round(v->current.y), vertices, edges)) return true;
	// try grid-adjacent positions
	double dx = std::abs(v->current.x - std::round(v->current.x));
	double dy = std::abs(v->current.y - std::round(v->current.y));
	if (dx >= dy) {
		if (attempt_move(v, round_away(v->current.x), std::round(v->current.y), vertices, edges)) return true;
		if (attempt_move(v, std::round(v->current.x), round_away(v->current.y), vertices, edges)) return true;
	}
	else {
		if (attempt_move(v, std::round(v->current.x), round_away(v->current.y), vertices, edges)) return true;
		if (attempt_move(v, round_away(v->current.x), std::round(v->current.y), vertices, edges)) return true;
	}
	// try diagonal grid point
	return attempt_move(v, round_away(v->current.x), round_away(v->current.y), vertices, edges);
}

bool check_valid_full(const vector<Vertex*>& vertices, const vector<Edge*>& edges) {
	BinnedGeometry geom_checker;
	for (Vertex* v : vertices) {
		if (!geom_checker.check_vertex_overlap(vertices, v)) return false;
		if (!v->rotsys_valid()) return false;
	}
	if (!geom_checker.check_intersections(vertices, edges)) return false;
	return true;
}

bool check_valid_after_move(Vertex* v, const vector<Vertex*>& vertices, const vector<Edge*>& edges) {
	BinnedGeometry geom_checker;
	if (!geom_checker.check_vertex_overlap(vertices, v)) return false;
	if (!v->rotsys_valid()) return false;
	for (Edge* e : v->N) {
		if (!e->other(v)->rotsys_valid()) return false;
	}
	return geom_checker.check_intersections(vertices, edges);
}