#ifndef INCLUDED_BINNED_GEOMETRY
#define INCLUDED_BINNED_GEOMETRY

#include <vector>

#include "Vertex.h"
#include "Edge.h"

struct BinnedGeometry {
	double minX;
	double minY;
	double maxX;
	double maxY;
	bool check_intersections(const std::vector<Vertex*>& vertices, const std::vector<Edge*>& edges);
	bool check_bin(const std::vector<Edge*>& segs);
	void draw_edge(Edge* e);
	void draw_pixel(int x, int y, Edge* s);

	bool check_vertex_overlap(const std::vector<Vertex*>& vertices, Vertex* v);

};

#endif //ndef INCLUDED_BINNED_GEOMETRY