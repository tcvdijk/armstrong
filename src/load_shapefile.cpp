#include "load_shapefile.h"
using std::string;
using std::vector;

#include <map>
using std::map;
using std::make_pair;

#include <limits>
using std::numeric_limits;

#include "shapefil.h"

#include "Vertex.h"
#include "Edge.h"
#include "Logging.h"
#include "Timer.h"

Edge* make_edge(Vertex* p, Vertex* q) {
	// does this edge already exist?
	for (Edge* e : p->N) {
		if (e->other(p) == q) {
			return nullptr;
		}
	}
	Edge* e = new Edge(p, q);
	p->N.push_back(e);
	q->N.push_back(e);
	return e;
}

bool load_shapefile(const string& filename, vector<Vertex*>& vertices, vector<Edge*>& edges) {
	console->info("Loading shapefile...");
	Timer load_time;

	typedef std::map<Vertex, Vertex*> VertexMap;
	VertexMap vertex_map;

	int num_nodes = 0, num_edges = 0;
	SHPHandle hSHP = SHPOpen(filename.c_str(), "rb");
	if (hSHP == 0) {
		console->error("Shapelib failed to open '{}'", filename);
		return false;
	}

	int num_entities, shape_type;
	double 	min_bound[4], max_bound[4];
	SHPGetInfo(hSHP, &num_entities, &shape_type, min_bound, max_bound);

	int num_hits = 0;
	int num_entries = 0;
	int num_zero_segments_skipped = 0;

	console->info("numEntities: {}", num_entities);
	for (int i = 0; i < num_entities; ++i) {
		SHPObject* s = SHPReadObject(hSHP, i);

		Vertex* previous_point_on_this_stroke = 0;
		for (int j = 0; j < s->nVertices; ++j) {
			++num_entries;
			double x = s->padfX[j];
			double y = s->padfY[j];
			Vertex p(x, y);
			VertexMap::iterator existing_point = vertex_map.find(p);
			Vertex* current_point = 0;
			if (existing_point == vertex_map.end()) {
				// point didn't exist yet
				current_point = new Vertex(x, y);
				current_point->id = vertices.size();
				vertices.push_back(current_point);
				vertex_map.insert(make_pair(*current_point, current_point));
				++num_nodes;

			}
			else {
				// existing point
				current_point = existing_point->second;
				++num_hits;
			}

			if (previous_point_on_this_stroke == current_point) {
				++num_zero_segments_skipped;
				continue;
			}

			if (previous_point_on_this_stroke) {
				Edge* e = make_edge(current_point, previous_point_on_this_stroke);
				if (e != nullptr) edges.push_back(e);
				++num_edges;
			}
			previous_point_on_this_stroke = current_point;

		}

		SHPDestroyObject(s);

	}
	console->info("Number of points: {}", vertices.size());
	console->info("Number of edges: {}", edges.size());
	SHPClose(hSHP);

	console->info("Loading time = {} s.", load_time.elapsed().count());
	return true;
}