#ifndef INCLUDED_WRITE_SVG
#define INCLUDED_WRITE_SVG

#include <vector>
#include <string>
#include "Vertex.h"
class Edge;

void write_svg(
	std::vector<Vertex*>& vertices,
	std::vector<Edge*>& edges,
	std::vector<Vertex::Point>& positions_after_preprocessing,
	std::vector<Vertex::Point>& positions_first_feasible,
	std::vector<Vertex::Point>& positions_after_annealing,
	const std::string& filename
);

#endif //ndef INCLUDED_WRITE_SVG
