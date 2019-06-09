#include "cartogram_preprocess.h"
#include "geometry_help.h"
#include "Vertex.h"
#include "Edge.h"
#include "logging.h"

#include <cmath>
#include <tuple>
using std::tuple;

#include <map>
using std::map;

using std::vector;

#define CGAL_HEADER_ONLY 1
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
using Kernel = CGAL::Exact_predicates_inexact_constructions_kernel;
using Itag = CGAL::No_intersection_tag;
using CDT = CGAL::Constrained_Delaunay_triangulation_2<Kernel, CGAL::Default, Itag>;
using Point = CDT::Point;

#include <Eigen/Sparse>
using namespace Eigen;

constexpr int x_id(int i) { return 2 * i; }
constexpr int y_id(int i) { return 2 * i + 1; }

constexpr double lerp(double from, double to, double t) {
	return t * to + (1.0 - t) * from;
}
void set_vertices_from_eigen(const vector<Vertex*>& vertices, const VectorXd& x, double t) {
	for (Vertex* v : vertices) {
		v->current.x = lerp(v->original.x, x[x_id(v->id)], t);
		v->current.y = lerp(v->original.y, x[y_id(v->id)], t);
	}
}

void apply_cartogram(const std::vector<Vertex*>& vertices, const std::vector<Edge*>& edges, bool enlarge_short_edges, bool space_nearby_vertices, bool add_cdt) {

	// === Settings
	const double position_weight = 0.1;

	const double edge_weight = 1.0;
	const double edge_min_length = std::sqrt(2.0);

	const double too_near_weight = 1.0;
	const double too_near_distance = std::sqrt(2.0);

	const double delaunay_weight = 1.0;
	const double delaunay_min_length = std::sqrt(2.0);

	// === Figure out the constraints
	// basic contraints
	const int vars_per_vertex = 2;
	const int num_vars = vertices.size() * vars_per_vertex;

	const int rows_per_vertex = 2;
	const int rows_per_edge = 2;
	int num_rows = vertices.size() * rows_per_vertex + edges.size() * rows_per_edge;

	const int nonzeroes_per_vertex = 2;
	const int nonzeroes_per_edge = 4;
	int num_nonzeroes = vertices.size() * nonzeroes_per_vertex + edges.size() * nonzeroes_per_edge;

	// too-near constraints
	vector<tuple<Vertex*, Vertex*>> too_near;
	if (space_nearby_vertices) {
		for (Vertex* a : vertices) {
			for (Vertex* b : vertices) {
				if (a != b) {
					double dx = b->original.x - a->original.x;
					double dy = b->original.y - a->original.y;
					double length = std::sqrt(dx * dx + dy * dy);
					const double min_length = std::sqrt(2.0);
					if (length < too_near_distance) {
						too_near.emplace_back(a, b);
					}
				}
			}
		}
	}
	const int rows_per_too_near = 2;
	num_rows += too_near.size() * rows_per_too_near;
	const int nonzeroes_per_too_near = 4;
	num_nonzeroes += too_near.size() * nonzeroes_per_too_near;

	// constrained delaunay triangulation
	vector<tuple<Vertex*, Vertex*>> delaunay_cons;
	if (add_cdt) {
		CDT cdt;
		vector<CDT::Vertex_handle> handles;
		handles.reserve(vertices.size());
		map<CDT::Vertex_handle, int> handle_map;
		for (Vertex* v : vertices) {
			CDT::Vertex_handle h = cdt.insert(Point(v->current.x, v->current.y));
			handles.push_back(h);
			handle_map[h] = v->id;
		}
		for (Edge* e : edges) {
			cdt.insert_constraint(handles[e->a->id], handles[e->b->id]);
		}
		console->info("Number of Delaunay points = {}", cdt.number_of_vertices());
		for (CDT::Finite_edges_iterator eit = cdt.finite_edges_begin(); eit != cdt.finite_edges_end(); ++eit) {
			Vertex* a = vertices[handle_map[eit->first->vertex(CDT::cw(eit->second))]];
			Vertex* b = vertices[handle_map[eit->first->vertex(CDT::ccw(eit->second))]];
			double dx = b->original.x - a->original.x;
			double dy = b->original.y - a->original.y;
			double length = std::sqrt(dx * dx + dy * dy);

			if (length < delaunay_min_length) {
				delaunay_cons.emplace_back(a, b);
			}
		}
		console->info("Number of Delaunay constraints to add = {}", delaunay_cons.size());
	}
	const int rows_per_delaunay = 2;
	num_rows += delaunay_cons.size() * rows_per_delaunay;
	const int nonzeroes_per_delaunay = 4;
	num_nonzeroes += delaunay_cons.size() * nonzeroes_per_delaunay;

	// === Build system
	VectorXd rhs(num_rows);
	typedef Triplet<double> Nonzero;
	vector<Nonzero> nz;
	nz.reserve(num_nonzeroes);
	int current_row = 0;
	// position
	for (Vertex* v : vertices) {
		nz.emplace_back(current_row, x_id(v->id), position_weight);
		rhs[current_row] = position_weight * v->original.x;
		++current_row;
		nz.emplace_back(current_row, y_id(v->id), position_weight);
		rhs[current_row] = position_weight * v->original.y;
		++current_row;
	}
	// edge
	for (Edge* e : edges) {
		double dx = e->b->original.x - e->a->original.x;
		double dy = e->b->original.y - e->a->original.y;
		if (enlarge_short_edges) {
			double length = std::sqrt(dx * dx + dy * dy);
			if (length < edge_min_length) {
				dx *= edge_min_length / length;
				dy *= edge_min_length / length;
			}
		}
		nz.emplace_back(current_row, x_id(e->a->id), -edge_weight);
		nz.emplace_back(current_row, x_id(e->b->id), edge_weight);
		rhs[current_row] = edge_weight * dx;
		++current_row;
		nz.emplace_back(current_row, y_id(e->a->id), -edge_weight);
		nz.emplace_back(current_row, y_id(e->b->id), edge_weight);
		rhs[current_row] = edge_weight * dy;
		++current_row;
	}
	// too near
	for (auto [a, b] : too_near) {
		double dx = b->original.x - a->original.x;
		double dy = b->original.y - a->original.y;
		double length = std::sqrt(dx * dx + dy * dy);
		dx *= too_near_distance / length;
		dy *= too_near_distance / length;
		nz.emplace_back(current_row, x_id(a->id), -too_near_weight);
		nz.emplace_back(current_row, x_id(b->id), too_near_weight);
		rhs[current_row] = too_near_weight * dx;
		++current_row;
		nz.emplace_back(current_row, y_id(a->id), -too_near_weight);
		nz.emplace_back(current_row, y_id(b->id), too_near_weight);
		rhs[current_row] = too_near_weight * dy;
		++current_row;
	}
	// delaunay
	for (auto [a, b] : delaunay_cons) {
		double dx = b->original.x - a->original.x;
		double dy = b->original.y - a->original.y;
		double length = std::sqrt(dx * dx + dy * dy);
		dx *= delaunay_min_length / length;
		dy *= delaunay_min_length / length;
		nz.emplace_back(current_row, x_id(a->id), -delaunay_weight);
		nz.emplace_back(current_row, x_id(b->id), delaunay_weight);
		rhs[current_row] = delaunay_weight * dx;
		++current_row;
		nz.emplace_back(current_row, y_id(a->id), -delaunay_weight);
		nz.emplace_back(current_row, y_id(b->id), delaunay_weight);
		rhs[current_row] = delaunay_weight * dy;
		++current_row;
	}

	// === Solve
	SparseMatrix<double> A(num_rows, num_vars);
	A.setFromTriplets(nz.begin(), nz.end());

	SparseMatrix<double> AtA = A.transpose() * A;
	VectorXd Atb = A.transpose() * rhs;
	SimplicialLDLT<SparseMatrix<double>> solver(AtA);
	VectorXd x = solver.solve(Atb);
	//VectorXd discrep = A * x - rhs;

	// === Read out solution and put it into the Vertices
	for (Vertex* v : vertices) {
		v->current.x = x[x_id(v->id)];
		v->current.y = x[y_id(v->id)];
	}
	// back off if result is not topogically valid
	double t = 1.0;
	while (!check_valid_full(vertices, edges)) {
		t -= 0.1;
		console->info("Checking cartogram at time {}", t);
		set_vertices_from_eigen(vertices, x, t);
	}
	console->info("Accepting cartogram at time {}", t);

}