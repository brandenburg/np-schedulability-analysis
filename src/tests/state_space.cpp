#include "doctest.h"

#include <iostream>

#include "uni/space.hpp"

using namespace NP;

static const auto inf = Time_model::constants<dtime_t>::infinity();

TEST_CASE("[NP state space] Find all next jobs") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		Job<dtime_t>{1, I( 0,  0), I(3, 8), 100, 1},
		Job<dtime_t>{2, I( 7,  7), I(5, 5),  100, 2},
		Job<dtime_t>{3, I(10, 10), I(1, 11),  100, 3},
	};

	SUBCASE("Naive exploration") {
		auto space = Uniproc::State_space<dtime_t>::explore_naively(jobs);
		CHECK(space.is_schedulable());

		CHECK(space.get_finish_times(jobs[0]).from()  == 3);
		CHECK(space.get_finish_times(jobs[0]).until() == 8);

		CHECK(space.get_finish_times(jobs[1]).from()  == 12);
		CHECK(space.get_finish_times(jobs[1]).until() == 13);

		CHECK(space.get_finish_times(jobs[2]).from()  == 13);
		CHECK(space.get_finish_times(jobs[2]).until() == 24);
	}

	SUBCASE("Exploration with merging") {
		auto space = Uniproc::State_space<dtime_t>::explore(jobs);
		CHECK(space.is_schedulable());

		CHECK(space.get_finish_times(jobs[0]).from()  == 3);
		CHECK(space.get_finish_times(jobs[0]).until() == 8);

		CHECK(space.get_finish_times(jobs[1]).from()  == 12);
		CHECK(space.get_finish_times(jobs[1]).until() == 13);

		CHECK(space.get_finish_times(jobs[2]).from()  == 13);
		CHECK(space.get_finish_times(jobs[2]).until() == 24);
	}

}

TEST_CASE("[NP state space] Consider large enough interval") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		Job<dtime_t>{1, I( 0,  0), I(3, 10),  100, 3},
		Job<dtime_t>{2, I( 7,  7),  I(5, 5),  100, 2},
		Job<dtime_t>{3, I(10, 10),  I(5, 5),  100, 1},
	};

	auto nspace = Uniproc::State_space<dtime_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  3);
	CHECK(nspace.get_finish_times(jobs[0]).until() == 10);

	CHECK(nspace.get_finish_times(jobs[1]).from()  == 12);
	CHECK(nspace.get_finish_times(jobs[1]).until() == 20);

	CHECK(nspace.get_finish_times(jobs[2]).from()  == 15);
	CHECK(nspace.get_finish_times(jobs[2]).until() == 19);

	auto space = Uniproc::State_space<dtime_t>::explore_naively(jobs);
	CHECK(space.is_schedulable());

	CHECK(space.get_finish_times(jobs[0]).from()  ==  3);
	CHECK(space.get_finish_times(jobs[0]).until() == 10);

	CHECK(space.get_finish_times(jobs[1]).from()  == 12);
	CHECK(space.get_finish_times(jobs[1]).until() == 20);

	CHECK(space.get_finish_times(jobs[2]).from()  == 15);
	CHECK(space.get_finish_times(jobs[2]).until() == 19);
}



TEST_CASE("[NP state space] Respect priorities") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		Job<dtime_t>{1, I( 0,  0), I(3, 10),  100, 2},
		Job<dtime_t>{2, I( 0,  0),  I(5, 5),  100, 1},
	};

	auto nspace = Uniproc::State_space<dtime_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  8);
	CHECK(nspace.get_finish_times(jobs[0]).until() == 15);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  5);
	CHECK(nspace.get_finish_times(jobs[1]).until() ==  5);

	auto space = Uniproc::State_space<dtime_t>::explore(jobs);
	CHECK(space.is_schedulable());

	CHECK(space.get_finish_times(jobs[0]).from()  ==  8);
	CHECK(space.get_finish_times(jobs[0]).until() == 15);

	CHECK(space.get_finish_times(jobs[1]).from()  ==  5);
	CHECK(space.get_finish_times(jobs[1]).until() ==  5);
}

TEST_CASE("[NP state space] Respect jitter") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		Job<dtime_t>{1, I( 0,  1), I(3, 10),  100, 2},
		Job<dtime_t>{2, I( 0,  1),  I(5, 5),  100, 1},
	};

	auto nspace = Uniproc::State_space<dtime_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  3);
	CHECK(nspace.get_finish_times(jobs[0]).until() == 16);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  5);
	CHECK(nspace.get_finish_times(jobs[1]).until() == 15);

	auto space = Uniproc::State_space<dtime_t>::explore(jobs);
	CHECK(space.is_schedulable());

	CHECK(space.get_finish_times(jobs[0]).from()  ==  3);
	CHECK(space.get_finish_times(jobs[0]).until() == 16);

	CHECK(space.get_finish_times(jobs[1]).from()  ==  5);
	CHECK(space.get_finish_times(jobs[1]).until() == 15);
}

TEST_CASE("[NP state space] Be eager") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		Job<dtime_t>{1, I( 0,  0),  I(1,  5),  100, 2},
		Job<dtime_t>{2, I( 0,  0),  I(1, 20),  100, 3},
		Job<dtime_t>{3, I(10, 10),  I(5,  5),  100, 1},
	};

	auto nspace = Uniproc::State_space<dtime_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  1);
	CHECK(nspace.get_finish_times(jobs[0]).until() ==  5);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  2);
	CHECK(nspace.get_finish_times(jobs[1]).until() ==  25);

	CHECK(nspace.get_finish_times(jobs[2]).from()  ==  15);
	CHECK(nspace.get_finish_times(jobs[2]).until() ==  30);

	auto space = Uniproc::State_space<dtime_t>::explore(jobs);
	CHECK(space.is_schedulable());

	CHECK(space.get_finish_times(jobs[0]).from()  ==  1);
	CHECK(space.get_finish_times(jobs[0]).until() ==  5);

	CHECK(space.get_finish_times(jobs[1]).from()  ==  2);
	CHECK(space.get_finish_times(jobs[1]).until() ==  25);

	CHECK(space.get_finish_times(jobs[2]).from()  ==  15);
	CHECK(space.get_finish_times(jobs[2]).until() ==  30);
}


TEST_CASE("[NP state space] Be eager, with short deadline") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		Job<dtime_t>{1, I( 0,  0),  I(1,  5),  100, 2},
		Job<dtime_t>{2, I( 9,  9),  I(1, 15),   25, 3},
		Job<dtime_t>{3, I(30, 30),  I(5,  5),  100, 1},
	};

	auto nspace = Uniproc::State_space<dtime_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  1);
	CHECK(nspace.get_finish_times(jobs[0]).until() ==  5);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  10);
	CHECK(nspace.get_finish_times(jobs[1]).until() ==  24);

	CHECK(nspace.get_finish_times(jobs[2]).from()  ==  35);
	CHECK(nspace.get_finish_times(jobs[2]).until() ==  35);

	auto space = Uniproc::State_space<dtime_t>::explore(jobs);
	CHECK(space.is_schedulable());

	CHECK(space.get_finish_times(jobs[0]).from()  ==  1);
	CHECK(space.get_finish_times(jobs[0]).until() ==  5);

	CHECK(space.get_finish_times(jobs[1]).from()  ==  10);
	CHECK(space.get_finish_times(jobs[1]).until() ==  24);

	CHECK(space.get_finish_times(jobs[2]).from()  ==  35);
	CHECK(space.get_finish_times(jobs[2]).until() ==  35);
}


TEST_CASE("[NP state space] Treat equal-priority jobs correctly") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		Job<dtime_t>{1, I(    0,    10),  I( 2,    50),  2000, 1},
		Job<dtime_t>{2, I(    0,    10),  I(50,  1200),  5000, 2},
		Job<dtime_t>{3, I( 1000,  1010),  I( 2,    50),  3000, 1},
	};

	auto nspace = Uniproc::State_space<dtime_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  2);
	CHECK(nspace.get_finish_times(jobs[0]).until() ==  1259);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  50);
	CHECK(nspace.get_finish_times(jobs[1]).until() ==  1260);

	CHECK(nspace.get_finish_times(jobs[2]).from()  ==  1002);
	CHECK(nspace.get_finish_times(jobs[2]).until() ==  1310);

	auto space = Uniproc::State_space<dtime_t>::explore(jobs);
	CHECK(space.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  2);
	CHECK(nspace.get_finish_times(jobs[0]).until() ==  1259);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  50);
	CHECK(nspace.get_finish_times(jobs[1]).until() ==  1260);

	CHECK(nspace.get_finish_times(jobs[2]).from()  ==  1002);
	CHECK(nspace.get_finish_times(jobs[2]).until() ==  1310);
}

TEST_CASE("[NP state space] Equal-priority simultaneous arrivals") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		Job<dtime_t>{1, I(    0,    10),  I(  2,    50),  2000, 2000, 1},
		Job<dtime_t>{2, I(    0,    10),  I(100,   150),  2000, 2000, 2},
	};

	auto nspace = Uniproc::State_space<dtime_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  2);
	CHECK(nspace.get_finish_times(jobs[0]).until() ==   9 + 150 + 50);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  100);
	CHECK(nspace.get_finish_times(jobs[1]).until() ==  10 + 50 + 150);

	auto space = Uniproc::State_space<dtime_t>::explore(jobs);
	CHECK(space.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  2);
	CHECK(nspace.get_finish_times(jobs[0]).until() ==   9 + 150 + 50);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  100);
	CHECK(nspace.get_finish_times(jobs[1]).until() ==  10 + 50 + 150);
}

TEST_CASE("[NP state space] don't skip over deadline-missing jobs") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		Job<dtime_t>{1, I(  100,   100),  I(   2,    50),   200, 1},
		Job<dtime_t>{2, I(    0,     0),  I(1200,  1200),  5000, 2},
		Job<dtime_t>{3, I(  200,   250),  I( 2,    50),    6000, 3},
		Job<dtime_t>{4, I(  200,   250),  I( 2,    50),    6000, 4},
		Job<dtime_t>{5, I(  200,   250),  I( 2,    50),    6000, 5},
	};

	SUBCASE("Naive exploration") {
		auto nspace = Uniproc::State_space<dtime_t>::explore_naively(jobs);
		CHECK(!nspace.is_schedulable());

		CHECK(nspace.number_of_edges() == 2);
		CHECK(nspace.number_of_states() == 3);
	}

	SUBCASE("Exploration with state-merging") {
		auto space = Uniproc::State_space<dtime_t>::explore(jobs);
		CHECK(!space.is_schedulable());

		CHECK(space.number_of_edges() == 2);
		CHECK(space.number_of_states() == 3);
	}

	SUBCASE("Exploration after deadline miss") {
		Scheduling_problem<dtime_t> prob{jobs};
		Analysis_options opts;
		opts.early_exit = false;

		auto space = Uniproc::State_space<dtime_t>::explore(prob, opts);
		CHECK(!space.is_schedulable());

		CHECK(space.number_of_edges() == 5);
		CHECK(space.number_of_states() == 6);

		// make sure the analysis continued after the deadline miss
		auto ftimes = space.get_finish_times(prob.jobs[0]);
		CHECK(ftimes.min() == 1202);
		CHECK(ftimes.max() == 1250);

		ftimes = space.get_finish_times(prob.jobs[1]);
		CHECK(ftimes.min() == 1200);
		CHECK(ftimes.max() == 1200);

		ftimes = space.get_finish_times(prob.jobs[2]);
		CHECK(ftimes.min() == 1204);
		CHECK(ftimes.max() == 1300);

		ftimes = space.get_finish_times(prob.jobs[3]);
		CHECK(ftimes.min() == 1206);
		CHECK(ftimes.max() == 1350);

		ftimes = space.get_finish_times(prob.jobs[4]);
		CHECK(ftimes.min() == 1208);
		CHECK(ftimes.max() == 1400);
	}
}

TEST_CASE("[NP state space] explore all branches with deadline-missing jobs") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		Job<dtime_t>{1, I(  100,   100),  I(   2,    50),   200, 1},
		Job<dtime_t>{2, I(    0,   150),  I(1200,  1200),  5000, 2},
		Job<dtime_t>{3, I(  200,   250),  I( 2,    50),    6000, 3},
		Job<dtime_t>{4, I(  200,   250),  I( 2,    50),    6000, 4},
		Job<dtime_t>{5, I(  200,   250),  I( 2,    50),    6000, 5},
	};
	Scheduling_problem<dtime_t> prob{jobs};
	Analysis_options opts;
	opts.early_exit = false;

	auto space = Uniproc::State_space<dtime_t>::explore(prob, opts);
	CHECK(!space.is_schedulable());

	CHECK(space.number_of_edges() ==  7);
	CHECK(space.number_of_states() == 7);

	// make sure the analysis continued after the deadline miss
	auto ftimes = space.get_finish_times(prob.jobs[0]);
	CHECK(ftimes.min() ==  102);
	CHECK(ftimes.max() == 1349);

	ftimes = space.get_finish_times(prob.jobs[1]);
	CHECK(ftimes.min() == 1200);
	CHECK(ftimes.max() == 1350);

	ftimes = space.get_finish_times(prob.jobs[2]);
	CHECK(ftimes.min() == 1204);
	CHECK(ftimes.max() == 1400);

	ftimes = space.get_finish_times(prob.jobs[3]);
	CHECK(ftimes.min() == 1206);
	CHECK(ftimes.max() == 1450);

	ftimes = space.get_finish_times(prob.jobs[4]);
	CHECK(ftimes.min() == 1208);
	CHECK(ftimes.max() == 1500);
}

TEST_CASE("[NP state space] explore across bucket boundaries") {
	Uniproc::State_space<dtime_t>::Workload jobs{
		Job<dtime_t>{1, I(  100,   100),  I(  50,   50),  10000, 1},
		Job<dtime_t>{2, I( 3000,  3000),  I(4000, 4000),  10000, 2},
		Job<dtime_t>{3, I( 6000,  6000),  I(   2,    2),  10000, 3},
	};

	Scheduling_problem<dtime_t> prob{jobs};
	Analysis_options opts;
	opts.num_buckets = 2;


	opts.be_naive = true;
	auto nspace = Uniproc::State_space<dtime_t>::explore(prob, opts);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.number_of_edges() == 3);

	opts.be_naive = false;
	auto space = Uniproc::State_space<dtime_t>::explore(prob, opts);
	CHECK(space.is_schedulable());

	CHECK(space.number_of_edges() == 3);
}

TEST_CASE("[NP state space] start times satisfy work-conserving property")
{
    Job<dtime_t> j0{0, I( 0,  0), I(2, 2), 10, 2, 0};
    Job<dtime_t> j1{1, I(0, 8), I(2, 2), 10, 1, 1};

    Uniproc::State_space<dtime_t>::Workload jobs{j0, j1};

    SUBCASE("naive exploration") {
        auto space = Uniproc::State_space<dtime_t>::explore_naively(jobs);
        CHECK(space.is_schedulable());
        CHECK(space.get_finish_times(j0) == I(2, 4));
        CHECK(space.get_finish_times(j1) == I(2, 10));
    }

    SUBCASE("exploration with state-merging") {
        auto space = Uniproc::State_space<dtime_t>::explore(jobs);
        CHECK(space.is_schedulable());
        CHECK(space.get_finish_times(j0) == I(2, 4));
        CHECK(space.get_finish_times(j1) == I(2, 10));
    }
}
