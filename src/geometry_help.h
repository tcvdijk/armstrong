#ifndef INCLUDED_GEOMETRY_HELP
#define INCLUDED_GEOMETRY_HELP

#include <vector>
#include "Vertex.h"
class Edge;

std::vector<Vertex::Point> backup_vertices(const std::vector<Vertex*>& vertices);

template<typename T >
T round_away(T x) {
	if (x < std::round(x)) return std::floor(x); else return std::ceil(x);
}

bool attempt_greedy(Vertex* v, const std::vector<Vertex*>& vertices, const std::vector<Edge*>& edges);

bool check_valid_full(const std::vector<Vertex*>& vertices, const std::vector<Edge*>& edges);
bool check_valid_after_move(Vertex* v, const std::vector<Vertex*>& vertices, const std::vector<Edge*>& edges);

#endif //ndef INCLUDED_GEOMETRY_HELP
