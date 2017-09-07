#include "doctest.h"

#include <iostream>


#include "schedule_space.hpp"

using namespace NP;

static const auto inf = Time_model::constants<dtime_t>::infinity();


// The example in Fig 1 of Nasri & Fohler (ECRTS 2016):
//    "Non-Work-Conserving Non-Preemptive Scheduling:
//     Motivations, Challenges, and Potential Solutions"
TEST_CASE("[IIP] P-RM example")
{
	Uniproc::State_space<dtime_t>::Workload jobs{
		// high-frequency task tau_1
		Job<dtime_t>{1, I( 0,  0), I(1, 1), 10, 1, 1},
		Job<dtime_t>{2, I(10, 10), I(1, 1), 20, 1, 1},
		Job<dtime_t>{3, I(20, 20), I(1, 1), 30, 1, 1},
		Job<dtime_t>{4, I(30, 30), I(1, 1), 40, 1, 1},
		Job<dtime_t>{5, I(40, 40), I(1, 1), 50, 1, 1},
		Job<dtime_t>{6, I(50, 50), I(1, 1), 60, 1, 1},

		// middle task
		Job<dtime_t>{7, I( 0,  0), I(8, 8), 30, 2, 2},
		Job<dtime_t>{8, I(30, 30), I(8, 8), 60, 2, 2},

		// the long task
		Job<dtime_t>{9, I( 0,  0), I(17, 17), 60, 3, 3}
	};

	SUBCASE("RM, naive exploration") {
		auto space = Uniproc::State_space<dtime_t>::explore_naively(jobs);
		CHECK(!space.is_schedulable());
	}

	SUBCASE("RM, exploration with state-merging") {
		auto space = Uniproc::State_space<dtime_t>::explore(jobs);
		CHECK(!space.is_schedulable());
	}

	SUBCASE("P-RM, naive exploration") {
		auto space = Uniproc::State_space<dtime_t, Uniproc::Precatious_RM_IIP<dtime_t>>::explore_naively(jobs);
		CHECK(space.is_schedulable());
	}

	SUBCASE("P-RM, exploration with state-merging") {
		auto space = Uniproc::State_space<dtime_t, Uniproc::Precatious_RM_IIP<dtime_t>>::explore(jobs);
		CHECK(space.is_schedulable());
	}

}
