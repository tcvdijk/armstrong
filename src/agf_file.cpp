#include "agf_file.h"
using std::string;
using std::vector;

#include <map>
using std::map;
using std::make_pair;

#include <limits>
using std::numeric_limits;

#include <fstream>
using std::ifstream;
using std::ofstream;
using std::getline;

#include <string>
using std::stoi;
using std::stod;

#include <sstream>
using std::stringstream;

#include "Vertex.h"
#include "Edge.h"
#include "Logging.h"
#include "Timer.h"
#include "load_shapefile.h"

Edge* make_edge(Vertex* p, Vertex* q);

bool load_agffile(const string& filename, vector<Vertex*>& vertices, vector<Edge*>& edges) {
	console->info("Loading agf file...");
	Timer load_time;

	typedef std::map<int, Vertex*> VertexMap;
	VertexMap vertexMap;

	ifstream file(filename);
	if (!file.is_open()) {
		console->error("agf reader failed to open '{}'", filename);
		return false;
	}

	int numNodes = 0, numEdges = 0;

	int num_hits = 0;
	int num_entries = 0;
	int num_zero_segments_skipped = 0;

	string line;

	getline(file, line);
	numNodes = stoi(line); // read number of nodes for vertex-reading for-loop

	getline(file, line);
	numEdges = stoi(line);

	for (int i = 0; i < numNodes; ++i) {
		getline(file, line);
		stringstream linestream(line);

		string part;
		getline(linestream, part, ' ');
		double xCoord = stod(part);

		getline(linestream, part, ' ');
		double yCoord = stod(part);

		Vertex* c = new Vertex(xCoord, yCoord);
		vertices.push_back(c);
		c->id = i;

		if (getline(linestream, part, ' ')) {
			double xCoordRound = stod(part, nullptr);
			getline(linestream, part, ' ');
			double yCoordRound = stod(part, nullptr);
			c->current = { xCoordRound, yCoordRound };
			c->set_rounded_state();
		}
	}

	for (int i = 0; i < numEdges; ++i) {
		getline(file, line);
		stringstream linestream(line);
		string part;
		getline(linestream, part, ' ');
		int a_id = stoi(part);
		getline(linestream, part, ' ');
		int b_id = stoi(part);

		Edge* e = make_edge(vertices[a_id], vertices[b_id]);
		if (e != nullptr) edges.push_back(e);
	}

	console->info("Number of points: {}", vertices.size());
	console->info("Number of segments: {}", edges.size());

	console->info("Loading time = {} s.", load_time.elapsed().count());
	return true;
}

void write_agffile(const std::string& filename, std::vector<Vertex*>& vertices, std::vector<Edge*>& edges) {
	ofstream out(filename);
	out << vertices.size() << '\n';
	out << edges.size() << '\n';
	for (Vertex* v : vertices) {
		out << v->original.x << ' '
			<< v->original.y << ' '
			<< v->current.x << ' '
			<< v->current.y << '\n';
	}
	for (Edge* e : edges) {
		out << e->a->id << ' ' << e->b->id << '\n';
	}
}