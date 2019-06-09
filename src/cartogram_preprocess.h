#ifndef INCLUDED_CARTOGRAM_PREPROCESS
#define INCLUDED_CARTOGRAM_PREPROCESS

#include <vector>

class Vertex;
class Edge;

void apply_cartogram(const std::vector<Vertex*>& vertices, const std::vector<Edge*>& edges, bool enlarge_short_edges, bool space_nearby_vertices, bool add_cdt);

#endif //ndef INCLUDED_CARTOGRAM_PREPROCESS