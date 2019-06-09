#include <algorithm>
using std::swap;

#include <limits>
using std::numeric_limits;

#include <iostream>
using std::cout;

#define CGAL_HEADER_ONLY 1
#include "CGAL/intersections.h"
#include "CGAL/Exact_predicates_inexact_constructions_kernel.h"
using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
using Point = Kernel::Point_2;
using Segment = Kernel::Segment_2;

#include "Logging.h"

#include "BinnedGeometry.h"

using std::vector;

static const int W = 512;
std::vector<Edge*> buffer[W * W];

bool BinnedGeometry::check_intersections(const vector<Vertex*>& vertices, const vector<Edge*>& edges) {
	minX = numeric_limits<double>::max();
	minY = numeric_limits<double>::max();
	maxX = numeric_limits<double>::lowest();
	maxY = numeric_limits<double>::lowest();
	for (Vertex* v : vertices) {
		minX = std::min(minX, v->current.x);
		maxX = std::max(maxX, v->current.x);
		minY = std::min(minY, v->current.y);
		maxY = std::max(maxY, v->current.y);
	}
	// clear buffer
	for (int i = 0; i < W * W; ++i) {
		buffer[i].clear();
	}
	// draw segments into buffer
	for (Edge* e : edges) {
		draw_edge(e);
	}

	// handle bins
	for (int i = 0; i < W * W; ++i) {
		if (!check_bin(buffer[i])) return false;
	}
	return true;
}

bool BinnedGeometry::check_bin(const std::vector<Edge*>& edges) {
	// quadratic-time brute force
	int n = edges.size();
	for (int i = 0; i < n - 1; ++i) {
		Edge* e = edges[i];
		Segment seg = Segment(Point(e->a->current.x, e->a->current.y), Point(e->b->current.x, e->b->current.y));
		for (int j = i + 1; j < n; ++j) {
			Edge* e2 = edges[j];
			if (e->a == e2->a || e->a == e2->b || e->b == e2->a || e->b == e2->b) continue;
			Segment seg2 = Segment(Point(e2->a->current.x, e2->a->current.y), Point(e2->b->current.x, e2->b->current.y));
			if (intersection(seg, seg2)) return false;
		}
	}
	return true;
}

double scale_to_unit(double x, double min, double max) {
	return (x - min) / (max - min);
}

void BinnedGeometry::draw_edge(Edge* e) {

	// scale coordinates to buffer index space
	double x0 = (W - 2.0) * scale_to_unit(e->a->current.x, minX, maxX) + 1;
	double y0 = (W - 2.0) * scale_to_unit(e->a->current.y, minY, maxY) + 1;
	double x1 = (W - 2.0) * scale_to_unit(e->b->current.x, minX, maxX) + 1;
	double y1 = (W - 2.0) * scale_to_unit(e->b->current.y, minY, maxY) + 1;

	if (x0 == x1) {
		// vertical lines
		int ix = static_cast<int>(std::floor(x0));
		if (y0 < y1) {
			int start_i = static_cast<int>(std::floor(y0));
			for (int i = start_i; i <= y1; i++)	draw_pixel(ix, i, e);
		}
		else {
			int start_i = static_cast<int>(std::floor(y1));
			for (int i = start_i; i <= y0; i++) draw_pixel(ix, i, e);
		}
	}
	else {
		// not vertical
		if (x0 > x1) {
			// Switch points so we go left-to-right
			swap(x0, x1);
			swap(y0, y1);
		}
		// Get parameters of line equation: y = m*x + b
		double m = (y0 - y1) / (x0 - x1);
		double b = (x0 * y1 - x1 * y0) / (x0 - x1);
		int ix0 = static_cast<int>(std::floor(x0));
		int iy0 = static_cast<int>(std::floor(y0));
		if (std::abs(m) < 1.0e-14) {
			// Almost-horizontal lines by special case
			for (int i = ix0; i <= x1; i++) draw_pixel(i, iy0, e);
		}
		else if (y0 < y1) {
			// Ascending (left to right)
			double y = m * (ix0 + 1) + b;
			while (ix0 <= x1 - 1) { // was: y < y1
				for (int i = iy0; i < y; i++) draw_pixel(ix0, i, e);
				iy0 = static_cast<int>(std::floor(y));
				ix0++;
				y += m; // In effect: y = m*(ix0+1) + b
			}
			for (int i = iy0; i <= y1; i++) draw_pixel(ix0, i, e);
		}
		else if (y0 > y1) {
			// Descending (left to right)
			double y = m * (ix0 + 1) + b;
			while (ix0 <= x1 - 1) { // was: y > y1
				for (int i = iy0; i > y - 1; i--) draw_pixel(ix0, i, e);
				iy0 = static_cast<int>(std::floor(y));
				ix0++;
				y += m;
			}
			for (int i = iy0; i > y1 - 1; i--) draw_pixel(ix0, i, e);
		}
	}
}

void BinnedGeometry::draw_pixel(int x, int y, Edge* e) {
	// put segment into buffer bin
	buffer[y * W + x].push_back(e);
}

bool BinnedGeometry::check_vertex_overlap(const std::vector<Vertex*>& vertices, Vertex* v) {
	for (Vertex* u : vertices) {
		if (u->is_rounded && u != v) {
			if (u->current.x == v->current.x && u->current.y == v->current.y) return false;
		}
	}
	return true;
}