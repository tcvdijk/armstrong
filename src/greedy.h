#ifndef INCLUDED_GREEDY
#define INCLUDED_GREEDY

#include <vector>
class Vertex;
class Edge;

void scale_and_greedy(std::vector<Vertex*>& vertices, std::vector<Edge*>& edges);
void scale_and_round(std::vector<Vertex*>& vertices, std::vector<Edge*>& edges);

#endif //ndef INCLUDED_GREEDY