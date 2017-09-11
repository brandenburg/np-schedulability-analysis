#include "doctest.h"

#include <iostream>


#include "schedule_space.hpp"

using namespace NP;

static const auto inf = Time_model::constants<dense_t>::infinity();

inline Interval<dense_t> D(dense_t a, dense_t b)
{
	return Interval<dense_t>{a, b};
}

TEST_CASE("[dense time] Example in Figure 1(a,b)") {
	Uniproc::State_space<dense_t>::Workload jobs{
		// high-frequency task
		Job<dense_t>{1, D( 0,  0), D(1, 2), 10, 10},
		Job<dense_t>{2, D(10, 10), D(1, 2), 20, 20},
		Job<dense_t>{3, D(20, 20), D(1, 2), 30, 30},
		Job<dense_t>{4, D(30, 30), D(1, 2), 40, 40},
		Job<dense_t>{5, D(40, 40), D(1, 2), 50, 50},
		Job<dense_t>{6, D(50, 50), D(1, 2), 60, 60},

		// middle task
		Job<dense_t>{7, D( 0,  0), D(7, 8), 30, 30},
		Job<dense_t>{8, D(30, 30), D(7, 7), 60, 60},

		// the long task
		Job<dense_t>{9, D( 0,  0), D(3, 13), 60, 60}
	};

// 	Uniproc::State_space space{jobs};

	SUBCASE("[dense time] naive state evolution (figure 2)") {
		Uniproc::Schedule_state<dense_t> v1;

		CHECK(v1.earliest_finish_time() == doctest::Approx(0));
		CHECK(v1.latest_finish_time() == doctest::Approx(0));

		Uniproc::Schedule_state<dense_t> v2{v1, jobs[0], 0, inf, inf};

		DM("v1: " << v1 << std::endl);
		DM("     ---[ " << jobs[0] << " --->" << std::endl);
		DM("v2: " << v2 << std::endl);

		CHECK(v2.earliest_finish_time() == doctest::Approx(1));
		CHECK(v2.latest_finish_time() == doctest::Approx(2));

		Uniproc::Schedule_state<dense_t> v3{v2, jobs[6], 6, 10, inf};

		DM("     ---[ " << jobs[6] << " --->" << std::endl);
		DM("v3: " << v3 << std::endl);

		CHECK(v3.earliest_finish_time() == doctest::Approx(8));
		CHECK(v3.latest_finish_time() == doctest::Approx(10));

		Uniproc::Schedule_state<dense_t> v4{v3, jobs[8], 8, 10, inf};

		DM("     ---[ " << jobs[8] << " --->" << std::endl);
		DM("v4: " << v4 << std::endl);

		CHECK(v4.earliest_finish_time() == doctest::Approx(11));
		CHECK(v4.latest_finish_time() == doctest::Approx(23));

		Uniproc::Schedule_state<dense_t> v6{v4, jobs[1], 1, inf, inf};

		DM("     ---[ " << jobs[1] << " --->" << std::endl);
		DM("v6: " << v6 << std::endl);

		CHECK(v6.earliest_finish_time() == doctest::Approx(12));
		CHECK(v6.latest_finish_time() == doctest::Approx(25));

		Uniproc::Schedule_state<dense_t> v8{v6, jobs[2], 2, inf, inf};

		DM("     ---[ " << jobs[2] << " --->" << std::endl);
		DM("v8: " << v8 << std::endl);

		CHECK(v8.earliest_finish_time() == doctest::Approx(21));
		CHECK(v8.latest_finish_time() == doctest::Approx(27));


		DM("==========[ lower branch ]=========" << std::endl);

		Uniproc::Schedule_state<dense_t> v5{v3, jobs[1], 1, inf, inf};

		DM("v3: " << v3 << std::endl);
		DM("    ---[ " << jobs[1] << " --->" << std::endl);
		DM("v5: " << v5 << std::endl);

		CHECK(v5.earliest_finish_time() == doctest::Approx(11));
		CHECK(v5.latest_finish_time() == doctest::Approx(12));

		Uniproc::Schedule_state<dense_t> v7{v5, jobs[8], 8, inf, inf};

		DM("    ---[ " << jobs[8] << " --->" << std::endl);
		DM("v7: " << v7 << std::endl);

		CHECK(v7.earliest_finish_time() == doctest::Approx(14));
		CHECK(v7.latest_finish_time() == doctest::Approx(25));

		Uniproc::Schedule_state<dense_t> v9{v7, jobs[2], 2, inf, inf};

		DM("    ---[ " << jobs[2] << " --->" << std::endl);
		DM("v9: " << v9 << std::endl);

		CHECK(v9.earliest_finish_time() == doctest::Approx(21));
		CHECK(v9.latest_finish_time() == doctest::Approx(27));


		CHECK(v9.get_key() == v8.get_key());
	}

	SUBCASE("Naive exploration") {
		auto space = Uniproc::State_space<dense_t>::explore_naively(jobs);
		CHECK(!space.is_schedulable());
	}

	SUBCASE("Exploration with state-merging") {
		auto space = Uniproc::State_space<dense_t>::explore(jobs);
		CHECK(!space.is_schedulable());
	}
}


TEST_CASE("[dense time] Example in Figure 1(c)") {
	Uniproc::State_space<dense_t>::Workload jobs{
		// high-frequency task
		Job<dense_t>{1, D( 0,  0), D(1, 2), 10, 1},
		Job<dense_t>{2, D(10, 10), D(1, 2), 20, 2},
		Job<dense_t>{3, D(20, 20), D(1, 2), 30, 3},
		Job<dense_t>{4, D(30, 30), D(1, 2), 40, 4},
		Job<dense_t>{5, D(40, 40), D(1, 2), 50, 5},
		Job<dense_t>{6, D(50, 50), D(1, 2), 60, 6},

		// the long task
		Job<dense_t>{9, D( 0,  0), D(3, 13), 60, 7},

		// middle task
		Job<dense_t>{7, D( 0,  0), D(7, 8), 30, 8},
		Job<dense_t>{8, D(30, 30), D(7, 7), 60, 9},
	};

	auto nspace = Uniproc::State_space<dense_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	auto space = Uniproc::State_space<dense_t>::explore(jobs);
	CHECK(space.is_schedulable());

	for (const Job<dense_t>& j : jobs) {
		CHECK(nspace.get_finish_times(j) == space.get_finish_times(j));
		CHECK(nspace.get_finish_times(j).from() != 0);
	}
}
