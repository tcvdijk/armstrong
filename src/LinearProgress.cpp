#include "LinearProgress.h"

#include "Logging.h"

using namespace std;

LinearProgress::LinearProgress(string_view task_name, string_view unit, int n) : task_name(task_name), unit(unit), ticks(0), n(n) {}

void LinearProgress::start() {
	start_time = chrono::system_clock::now();
}
void LinearProgress::tick(double score) {
	++ticks;
	alive(score);
}
void LinearProgress::alive(double score) {
	auto now = chrono::system_clock::now();
	auto wait = now - last_message_time;
	if (wait > chrono::seconds(1)) {
		last_message_time = now;
		message(score);
	}
}
void LinearProgress::done(double score) {
	message(score);
}

void LinearProgress::message(double score) {
	double duration = chrono::duration<double>(last_message_time - start_time).count();
	double per_second = static_cast<double>(ticks) / duration;
	if (n > 0) {
		int progress = static_cast<int>(100.0 * static_cast<double>(ticks) / static_cast<double>(n));
		int remaining = static_cast<int>((n - ticks) / per_second);
		console->info("{0}{2:>3}%, {3:.4} {1}/sec, remaining: {4} sec: {5}", task_name, unit, progress, per_second, remaining, score);
	}
	else {
		console->info("{0} {2:.4} {1}/sec, total {3} sec: {4}", task_name, unit, per_second, std::ceil(duration), score);
	}
}
