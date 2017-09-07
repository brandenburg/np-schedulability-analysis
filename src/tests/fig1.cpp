#include "doctest.h"

#include <iostream>


#include "schedule_space.hpp"

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
		Job<dtime_t>{8, I(30, 30), I(7, 7), 60, 60},

		// the long task
		Job<dtime_t>{9, I( 0,  0), I(3, 13), 60, 60}
	};

// 	Uniproc::State_space space{jobs};

	SUBCASE("Naive state evolution (figure 2)") {
		auto v1 = Uniproc::Schedule_state<dtime_t>::initial_state();

		CHECK(v1.earliest_finish_time() == 0);
		CHECK(v1.latest_finish_time() == 0);

		auto v2 = v1.schedule(jobs[0], 0, inf);

		std::cout << "v1: " << v1 << std::endl;
		std::cout << "     ---[ " << jobs[0] << " --->" << std::endl;
		std::cout << "v2: " << v2 << std::endl;

		CHECK(v2.earliest_finish_time() == 1);
		CHECK(v2.latest_finish_time() == 2);

		auto v3 = v2.schedule(jobs[6], 10, inf);

		std::cout << "     ---[ " << jobs[6] << " --->" << std::endl;
		std::cout << "v3: " << v3 << std::endl;

		CHECK(v3.earliest_finish_time() == 8);
		CHECK(v3.latest_finish_time() == 10);

		auto v4 = v3.schedule(jobs[8], 10, inf);

		std::cout << "     ---[ " << jobs[8] << " --->" << std::endl;
		std::cout << "v4: " << v4 << std::endl;

		CHECK(v4.earliest_finish_time() == 11);
		CHECK(v4.latest_finish_time() == 22);

		auto v6 = v4.schedule(jobs[1], inf, inf);

		std::cout << "     ---[ " << jobs[1] << " --->" << std::endl;
		std::cout << "v6: " << v6 << std::endl;

		CHECK(v6.earliest_finish_time() == 12);
		CHECK(v6.latest_finish_time() == 24);

		auto v8 = v6.schedule(jobs[2], inf, inf);

		std::cout << "     ---[ " << jobs[2] << " --->" << std::endl;
		std::cout << "v8: " << v8 << std::endl;

		CHECK(v8.earliest_finish_time() == 21);
		CHECK(v8.latest_finish_time() == 26);


		std::cout << "==========[ lower branch ]=========" << std::endl;

		auto v5 = v3.schedule(jobs[1], inf, inf);

		std::cout << "v3: " << v3 << std::endl;
		std::cout << "    ---[ " << jobs[1] << " --->" << std::endl;
		std::cout << "v5: " << v5 << std::endl;

		CHECK(v5.earliest_finish_time() == 11);
		CHECK(v5.latest_finish_time() == 12);

		auto v7 = v5.schedule(jobs[8], inf, inf);

		std::cout << "    ---[ " << jobs[8] << " --->" << std::endl;
		std::cout << "v7: " << v7 << std::endl;

		CHECK(v7.earliest_finish_time() == 14);
		CHECK(v7.latest_finish_time() == 25);

		auto v9 = v7.schedule(jobs[2], inf, inf);

		std::cout << "    ---[ " << jobs[2] << " --->" << std::endl;
		std::cout << "v9: " << v9 << std::endl;

		CHECK(v9.earliest_finish_time() == 21);
		CHECK(v9.latest_finish_time() == 27);


		CHECK(v9.get_key() == v8.get_key());
	}

	SUBCASE("Naive exploration") {
		auto space = Uniproc::State_space<dtime_t>::explore_naively(jobs);
		CHECK(!space.is_schedulable());
	}

	SUBCASE("Exploration with state-merging") {
		auto space = Uniproc::State_space<dtime_t>::explore(jobs);
		CHECK(!space.is_schedulable());
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

	for (const Job<dtime_t>& j : jobs)
		std::cout << j << " -> " << space.get_finish_times(j) << std::endl;
}
