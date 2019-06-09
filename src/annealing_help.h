#ifndef INCLUDED_ANNEALING_HELP
#define INCLUDED_ANNEALING_HELP

#include <random>

template< typename RNG >
bool accept_move(double temperature, double value, double new_value, RNG& rng) {
	if (new_value < value) return true;
	double accept_prob = std::exp(-(new_value - value) / temperature);
	std::bernoulli_distribution coin(accept_prob);
	bool accept = coin(rng);
	return accept;
}

double exponential_schedule(
	double start_temp,
	double end_temp,
	int steps
);

#endif //ndef INCLUDED_ANNEALING_HELP
