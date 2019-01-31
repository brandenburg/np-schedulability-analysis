#include "doctest.h"

#include <iostream>


#include "uni/space.hpp"

using namespace NP;

static const auto inf = Time_model::constants<dtime_t>::infinity();

TEST_CASE("Example in Figure 1(a,b)") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		// high-frequency task
		Job<dtime_t>{1, I( 0,  0), I(1, 2), 10, 10},
		Job<dtime_t>{2, I(10, 10), I(1, 2), 20, 20},
		Job<dtime_t>{3, I(20, 20), I(1, 2), 30, 30},
		Job<dtime_t>{4, I(30, 30), I(1, 2), 40, 40},
		Job<dtime_t>{5, I(40, 40), I(1, 2), 50, 50},
		Job<dtime_t>{6, I(50, 50), I(1, 2), 60, 60},

		// middle task
		Job<dtime_t>{7, I( 0,  0), I(7, 8), 30, 30},
		Job<dtime_t>{8, I(30, 30), I(7, 8), 60, 60},

		// the long task
		Job<dtime_t>{9, I( 0,  0), I(3, 13), 60, 60}
	};

	SUBCASE("Naive exploration") {
		auto space = Uniproc::State_space<dtime_t>::explore_naively(jobs);
		CHECK(!space.is_schedulable());

		// make sure we saw the right deadline miss
		auto ftimes = space.get_finish_times(jobs[1]);
		CHECK(ftimes.min() == 11);
		CHECK(ftimes.max() == 24);
	}

	SUBCASE("Exploration with state-merging") {
		auto space = Uniproc::State_space<dtime_t>::explore(jobs);
		CHECK(!space.is_schedulable());

		// make sure we saw the right deadline miss
		auto ftimes = space.get_finish_times(jobs[1]);
		CHECK(ftimes.min() == 11);
		CHECK(ftimes.max() == 24);
	}

	SUBCASE("Exploration after deadline miss") {
		// explore with early_exit = false
		Scheduling_problem<dtime_t> prob{jobs};
		Analysis_options opts;
		opts.early_exit = false;
		auto space = Uniproc::State_space<dtime_t>::explore(prob, opts);
		CHECK(!space.is_schedulable());

		// make sure the analysis continued after the deadline miss
		auto ftimes = space.get_finish_times(jobs[5]);
		CHECK(ftimes.min() == 51);
		CHECK(ftimes.max() == 52);

		ftimes = space.get_finish_times(jobs[4]);
		CHECK(ftimes.min() == 41);
		CHECK(ftimes.max() == 42);

		ftimes = space.get_finish_times(jobs[3]);
		CHECK(ftimes.min() == 31);
		CHECK(ftimes.max() == 32);

		ftimes = space.get_finish_times(jobs[7]);
		CHECK(ftimes.min() == 38);
		CHECK(ftimes.max() == 40);
	}
}


TEST_CASE("Example in Figure 1(c)") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		// high-frequency task
		Job<dtime_t>{1, I( 0,  0), I(1, 2), 10, 1},
		Job<dtime_t>{2, I(10, 10), I(1, 2), 20, 2},
		Job<dtime_t>{3, I(20, 20), I(1, 2), 30, 3},
		Job<dtime_t>{4, I(30, 30), I(1, 2), 40, 4},
		Job<dtime_t>{5, I(40, 40), I(1, 2), 50, 5},
		Job<dtime_t>{6, I(50, 50), I(1, 2), 60, 6},

		// the long task
		Job<dtime_t>{9, I( 0,  0), I(3, 13), 60, 7},

		// middle task
		Job<dtime_t>{7, I( 0,  0), I(7, 8), 30, 8},
		Job<dtime_t>{8, I(30, 30), I(7, 7), 60, 9},
	};

	auto nspace = Uniproc::State_space<dtime_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	auto space = Uniproc::State_space<dtime_t>::explore(jobs);
	CHECK(space.is_schedulable());

	for (const Job<dtime_t>& j : jobs) {
		CHECK(nspace.get_finish_times(j) == space.get_finish_times(j));
		CHECK(nspace.get_finish_times(j).from() != 0);
	}
}
