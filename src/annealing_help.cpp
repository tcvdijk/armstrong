#include "annealing_help.h"

#include <cmath>

double exponential_schedule(double start_temp, double end_temp, int steps) {
	// solving alpha ^ steps * start = end for alpha
	double fraction = end_temp / start_temp;
	if (fraction == 0) fraction = 0.000001;
	return std::pow(fraction, 1.0 / steps);
}