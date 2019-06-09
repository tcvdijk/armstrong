#include "density_annealing.h"

#include <map>
#include <tuple>
#include <cmath>
using std::map;
using std::tuple;
using std::vector;

double evaluate_density(const vector<Vertex*>& vertices) {
	double score = 0;
	int n = vertices.size();
	for (int i = 0; i < n - 1; ++i) {
		double local_density = 0;
		for (int j = i + 1; j < n; ++j) {
			local_density += 1.0 / (distance_sqr(vertices[i], vertices[j]));
		}
		vertices[i]->density = local_density;
		score += local_density;
	}
	return score;
}

constexpr tuple<int, int> make_entry(double x, double y) {
	return { static_cast<int>(std::round(x)), static_cast<int>(std::round(y)) };
}

double evaluate_grid_density(const std::vector<Vertex*>& vertices) {
	map<tuple<int, int>, double> density;
	for (Vertex* v : vertices) {
		if (v->is_rounded) {
			const double weight = 1.0 / 9.0;
			for (int x = v->current.x - 1; x <= v->current.x + 1; ++x) {
				for (int y = v->current.y - 1; y <= v->current.y + 1; ++y) {
					density[{x, y}] += weight;
				}
			}
		}
		else {
			const double weight = 1.0 / 4.0;
			density[make_entry(std::floor(v->current.x), std::floor(v->current.y))] += weight;
			density[make_entry(std::ceil(v->current.x), std::floor(v->current.y))] += weight;
			density[make_entry(std::floor(v->current.x), std::ceil(v->current.y))] += weight;
			density[make_entry(std::ceil(v->current.x), std::ceil(v->current.y))] += weight;
		}
	}
	double score = 0;
	for (Vertex* v : vertices) {
		double local_density = 0;
		int here = density[make_entry(v->current.x, v->current.y)];
		local_density += here * here;
		v->density = local_density;
		score += local_density;
	}
	return score;
}
