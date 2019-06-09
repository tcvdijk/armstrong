#ifndef INCLUDED_EDGE
#define INCLUDED_EDGE

#include <vector>

class Vertex;

class Edge {
public:
	Edge(Vertex* a, Vertex* b) : a(a), b(b) {}
	Vertex* a, * b;
	double angle;
	double get_angle(Vertex* p);
	Vertex* other(Vertex* p) {
		return p == a ? b : a;
	}
};
typedef std::vector<Edge*> Edges;

#endif //ndef INCLUDED_EDGE