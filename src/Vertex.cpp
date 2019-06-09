#include <algorithm>
using std::sort;

#include <vector>
using std::vector;

#include <tuple>
using std::make_tuple;

#include "Vertex.h"
#include "Edge.h"

using std::function;

#include "Checkpoint.h"
#include "geometry_help.h"

void Vertex::set_rounded_state() {
	is_rounded = current.x == std::floor(current.x) && current.y == std::floor(current.y);
}

double Vertex::rounding_cost() {
	double dx = current.x - original.x;
	double dy = current.y - original.y;
	return std::sqrt(dx * dx + dy * dy);
}

void Vertex::set_rotsys() {
	for (Edge* e : N) { e->angle = e->get_angle(this); }
	sort(N.begin(), N.end(), [this](Edge* a, Edge* b) {
		return a->angle < b->angle;
		});
}

bool Vertex::rotsys_valid() {
	if (N.size() <= 2) return true;
	vector<double> current_angles;
	current_angles.reserve(N.size());
	for (Edge* e : N) {
		current_angles.push_back(e->get_angle(this));
	}
	bool have_jumped = false;
	for (int i = 1; i < current_angles.size(); ++i) {
		if (current_angles[i - 1] > current_angles[i]) {
			if (have_jumped) return false;
			else have_jumped = true;
		}
	}
	if (current_angles.back() > current_angles.front()) {
		if (have_jumped) return false;
	}
	// check for overlapping outgoing edges
	sort(current_angles.begin(), current_angles.end());
	double last = -42; // not a valid angle
	for (double a : current_angles) {
		if (a == last) return false;
		last = a;
	}
	return true;
}

bool Vertex::neighborhood_rotsys_valid() {
	if (!rotsys_valid()) return false;
	for (Edge* e : N) if (!e->other(this)->rotsys_valid()) return false;
	return true;
}

bool operator<(const Vertex& a, const Vertex& b) {
	return make_tuple(a.current.x, a.current.y) < make_tuple(b.current.x, b.current.y);
}

double distance_sqr(const Vertex* u, const Vertex* v) {
	double dx = u->current.x - v->current.x;
	double dy = u->current.y - v->current.y;
	return dx * dx + dy * dy;
}

bool Vertex::climb(vector<Vertex*>& vertices, vector<Edge*>& edges) {
	double score = 0;
	int best_dx = 0;
	int best_dy = 0;
	double score_best = rounding_cost();
	for (int dx = -1; dx < 2; ++dx) {
		for (int dy = -1; dy < 2; ++dy) {
			if (dx == 0 && dy == 0) continue;
			Checkpoint checkpoint(current);
			current.x += dx;
			current.y += dy;
			double score_here = rounding_cost();
			if (score_here < score_best) {
				if (check_valid_after_move(this, vertices, edges)) {
					best_dx = dx;
					best_dy = dy;
					score_best = score_here;
				}
			}
		}
	}
	if (best_dx != 0 || best_dy != 0) {
		current.x += best_dx;
		current.y += best_dy;
		return true;
	}
	else {
		return false;
	}
}