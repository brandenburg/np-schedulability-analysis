#ifndef CLOCK_HPP
#define CLOCK_HPP

#include <time.h>

class Processor_clock {

	private:

	clock_t accum = 0, start_time = 0;
	bool running = false;

	static constexpr double scale_to_seconds = 1.0 / (double) CLOCKS_PER_SEC;

	public:

	void start()
	{
		running = true;
		start_time = clock();
	}


	double stop()
	{
		auto delta = clock() - start_time;
		if (running) {
			accum += delta;
			running = false;
			return delta * scale_to_seconds;
		}
		else
			return 0;
	}

	operator double() const {
		clock_t extra = 0;
		if (running)
			extra = clock() - start_time;
		return (accum + extra) * scale_to_seconds;
	}

};

#endif
