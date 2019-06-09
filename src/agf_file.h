#ifndef INCLUDED_AGF_FILE
#define INCLUDED_AGF_FILE

#include <string>
#include <vector>

class Vertex;
class Edge;

bool load_agffile(const std::string& filename, std::vector<Vertex*>& vertices, std::vector<Edge*>& edges);
void write_agffile(const std::string& filename, std::vector<Vertex*>& vertices, std::vector<Edge*>& edges);

#endif //ndef INCLUDED_AGF_FILE
