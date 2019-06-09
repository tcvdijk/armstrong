#ifndef INCLUDED_LOAD_SHAPEFILE
#define INCLUDED_LOAD_SHAPEFILE

#include <string>
#include <vector>

class Vertex;
class Edge;

bool load_shapefile(const std::string& filename, std::vector<Vertex*>& vertices, std::vector<Edge*>& edges);

#endif //ndef INCLUDED_LOAD_SHAPEFILE