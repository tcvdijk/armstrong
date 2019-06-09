#include "Vertex.h"
#include "Edge.h"

double Edge::get_angle(Vertex* p) {
	Vertex* q = other(p);
	double dx = q->current.x - p->current.x;
	double dy = q->current.y - p->current.y;
	return atan2(dy, dx);
}