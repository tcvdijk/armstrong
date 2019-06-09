#ifndef INCLUDED_VERTEX
#define INCLUDED_VERTEX

#include <functional>
#include <random>
#include <vector>
#include <cmath>

class Edge;

class Vertex {
public:

	Vertex(double x, double y) :
		original{ x, y },
		current{ x, y },
		id(-1),
		density(0)
	{ set_rounded_state(); }

	struct Point { double x, y; };
	Point original;
	Point current;
	int id;
	std::vector<Edge*> N;

	bool is_rounded;
	void set_rounded_state();

	double density;

	double rounding_cost();

	void set_rotsys();
	bool rotsys_valid();
	bool neighborhood_rotsys_valid();

	//template<typename RNG>
	void mutate(std::mt19937& rng) {
		if (is_rounded) {
			std::uniform_int_distribution<std::mt19937::result_type> move(0, 2);
			int dx = 0;
			int dy = 0;
			// rejection sampling to force movement
			while (dx == 0 && dy == 0) {
				dx = static_cast<int>(move(rng)) - 1;
				dy = static_cast<int>(move(rng)) - 1;
			}
			current.x += dx;
			current.y += dy;
		}
		else {
			std::bernoulli_distribution coin(0.5);
			current.x = coin(rng) ? std::floor(current.x) : std::ceil(current.x);
			current.y = coin(rng) ? std::floor(current.y) : std::ceil(current.y);
		}
	}

	bool climb(std::vector<Vertex*>& vertices, std::vector<Edge*>& edges);

};

bool operator<(const Vertex& a, const Vertex& b);

double distance_sqr(const Vertex* u, const Vertex* v);

typedef std::vector<Vertex*> Vertices;

#endif //ndef INCLUDED_VERTEX