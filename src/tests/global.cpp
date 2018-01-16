#include "doctest.h"

#include <iostream>
#include <sstream>

#include "io.hpp"
#include "global/space.hpp"

const std::string fig1a_jobs_file =
"   Task ID,     Job ID,          Arrival min,          Arrival max,             Cost min,             Cost max,             Deadline,             Priority\n"
"1, 1,  0,  0, 1,  2, 10, 10\n"
"1, 2, 10, 10, 1,  2, 20, 20\n"
"1, 3, 20, 20, 1,  2, 30, 30\n"
"1, 4, 30, 30, 1,  2, 40, 40\n"
"1, 5, 40, 40, 1,  2, 50, 50\n"
"1, 6, 50, 50, 1,  2, 60, 60\n"
"2, 7,  0,  0, 7,  8, 30, 30\n"
"2, 8, 30, 30, 7,  7, 60, 60\n"
"3, 9,  0,  0, 3, 13, 60, 60\n";


TEST_CASE("[global] running job") {
	auto rj = NP::Global::Running_job<dtime_t>();

	CHECK(rj.is_idle());

	auto in = std::istringstream(fig1a_jobs_file);
	auto jobs = NP::parse_file<dtime_t>(in);

	rj.next_job(jobs[0], 0);
	CHECK_FALSE(rj.is_idle());
	CHECK(rj.earliest_finish_time() == 1);
	CHECK(rj.latest_finish_time() == 2);

	CHECK(rj.earliest_start_time(jobs[6]) ==  1);
	CHECK(rj.earliest_start_time(jobs[1]) == 10);

	rj.next_job(jobs[6], 2);
	CHECK_FALSE(rj.is_idle());
	CHECK(rj.earliest_finish_time() == 8);
	CHECK(rj.latest_finish_time() == 10);

	CHECK(rj.earliest_start_time(jobs[8]) ==  8);

	rj.next_job(jobs[8], 15);
	CHECK(!rj.is_idle());
	CHECK(rj.earliest_finish_time() == 8  + 3);
	CHECK(rj.latest_finish_time() == 15 + 13);

	// make a copy
	auto rj2 = rj;

	rj.become_idle();
	CHECK(rj.is_idle());
	CHECK_FALSE(rj2.is_idle());

	auto rj3 = rj2;
	auto rj4 = rj2;

	rj2.fast_forward(30, 35);
	CHECK(rj2.is_idle());
	CHECK(rj2.earliest_finish_time() == 30);
	CHECK(rj2.latest_finish_time() == 30);

	rj3.fast_forward(25, 27);
	CHECK_FALSE(rj3.is_idle());
	CHECK(rj3.earliest_finish_time() == 25);
	CHECK(rj3.latest_finish_time() == 28);

	rj4.fast_forward(25, 28);
	CHECK(rj4.is_idle());
	CHECK(rj4.earliest_finish_time() == 25);
	CHECK(rj4.latest_finish_time() == 25);
}

TEST_CASE("[global] Running_jobs") {
	auto rjs = NP::Global::Running_jobs<dtime_t>(2);

	CHECK(rjs.size() == 2);
	CHECK(rjs.idle_core_exists());

	auto in = std::istringstream(fig1a_jobs_file);
	auto jobs = NP::parse_file<dtime_t>(in);

	rjs[0].next_job(jobs[0], 0);

	CHECK(rjs.idle_core_exists());

	rjs[1].next_job(jobs[6], 0);

	CHECK_FALSE(rjs.idle_core_exists());

	CHECK(rjs.possible_core_availability_time() == 1);
	CHECK(rjs.certain_core_availability_time() == 2);

	CHECK(rjs[0].earliest_finish_time() == 1);
	CHECK(rjs[0].latest_finish_time() == 2);
	CHECK(rjs[1].earliest_finish_time() == 7);
	CHECK(rjs[1].latest_finish_time() == 8);

	rjs.replace(0, jobs[8], 2);

	CHECK_FALSE(rjs.idle_core_exists());

	CHECK(rjs[0].earliest_finish_time() == 4);
	CHECK(rjs[0].latest_finish_time() == 15);
	CHECK(rjs[1].earliest_finish_time() == 7);
	CHECK(rjs[1].latest_finish_time() == 8);

	CHECK(rjs.possible_core_availability_time() == 1 + 3);
	CHECK(rjs.certain_core_availability_time()  == 8);

	auto copy = NP::Global::Running_jobs<dtime_t>(rjs);

	rjs.replace(1, jobs[1], 10);

	CHECK_FALSE(rjs.idle_core_exists());

	CHECK(rjs[0].earliest_finish_time() == 10);
	CHECK(rjs[0].latest_finish_time() == 15);
	CHECK(rjs[1].earliest_finish_time() == 11);
	CHECK(rjs[1].latest_finish_time() == 12);

	CHECK(rjs.possible_core_availability_time() == 10);
	CHECK(rjs.certain_core_availability_time()  == 12);


	copy.replace(0, jobs[1], 10);

	CHECK(copy.idle_core_exists());

	CHECK(copy[0].earliest_finish_time() == 11);
	CHECK(copy[0].latest_finish_time() == 12);
	CHECK(copy[1].earliest_finish_time() == 10);
	CHECK(copy[1].latest_finish_time() == 10);

	CHECK(copy.possible_core_availability_time() == 10);
	CHECK(copy.certain_core_availability_time()  == 10);

	copy.replace(0, jobs[2], 20);

	CHECK(copy.idle_core_exists());

	CHECK(copy[0].earliest_finish_time() == 21);
	CHECK(copy[0].latest_finish_time() == 22);
	CHECK(copy[1].earliest_finish_time() == 20);
	CHECK(copy[1].latest_finish_time() == 20);

	CHECK(copy.possible_core_availability_time() == 20);
	CHECK(copy.certain_core_availability_time()  == 20);

	rjs.replace(1, jobs[2], 20);

	CHECK(rjs.idle_core_exists());

	CHECK(rjs[0].earliest_finish_time() == 20);
	CHECK(rjs[0].latest_finish_time() == 20);
	CHECK(rjs[1].earliest_finish_time() == 21);
	CHECK(rjs[1].latest_finish_time() == 22);

	CHECK(rjs.possible_core_availability_time() == 20);
	CHECK(rjs.certain_core_availability_time()  == 20);

	rjs.fast_forward(50, 60);
	copy.fast_forward(50, 60);

	CHECK(rjs.idle_core_exists());
	CHECK(copy.idle_core_exists());

	CHECK(rjs[0].earliest_finish_time() == 50);
	CHECK(rjs[0].latest_finish_time() == 50);
	CHECK(rjs[1].earliest_finish_time() == 50);
	CHECK(rjs[1].latest_finish_time() == 50);

	CHECK(copy[0].earliest_finish_time() == 50);
	CHECK(copy[0].latest_finish_time() == 50);
	CHECK(copy[1].earliest_finish_time() == 50);
	CHECK(copy[1].latest_finish_time() == 50);

	std::vector<int> matching;
}

TEST_CASE("[global] RTSS17-Fig-1a") {
	auto in = std::istringstream(fig1a_jobs_file);
	auto jobs = NP::parse_file<dtime_t>(in);

	auto num_cpus = 2;
	auto dag = NP::Precedence_constraints();

	auto nspace = NP::Global::State_space<dtime_t>::explore_naively(jobs,
		0, 1000, dag, num_cpus);

	CHECK(nspace.is_schedulable());

	auto space = NP::Global::State_space<dtime_t>::explore(jobs,
		0, 1000, dag, num_cpus);

	CHECK(space.is_schedulable());


	num_cpus = 1;
	auto nspace2 = NP::Global::State_space<dtime_t>::explore_naively(jobs,
		0, 1000, dag, num_cpus);

	CHECK_FALSE(nspace2.is_schedulable());

	auto space2 = NP::Global::State_space<dtime_t>::explore(jobs,
		0, 1000, dag, num_cpus);

	CHECK_FALSE(space2.is_schedulable());
}

const std::string global_fig1_file =
"Task ID, Job ID, Arrival min, Arrival max, Cost min, Cost max, Deadline, Priority\n"
"1, 1, 0, 0, 2, 4, 7, 1\n"
"2, 1, 0, 0, 10, 15, 20, 2\n"
"3, 1, 5, 5, 1, 7, 15, 3\n"
"4, 1, 8, 8, 2, 3, 20, 4\n"
"5, 1, 8, 8, 1, 1, 14, 5\n";


TEST_CASE("[global] ECRTS18-Fig-1") {
	auto in = std::istringstream(global_fig1_file);
	auto jobs = NP::parse_file<dtime_t>(in);

	auto num_cpus = 2;
	auto dag = NP::Precedence_constraints();

	auto nspace = NP::Global::State_space<dtime_t>::explore_naively(jobs,
		0, 1000, dag, num_cpus);

	CHECK_FALSE(nspace.is_schedulable());

	auto space = NP::Global::State_space<dtime_t>::explore(jobs,
		0, 1000, dag, num_cpus);

	CHECK_FALSE(space.is_schedulable());
}

static const auto inf = Time_model::constants<dtime_t>::infinity();

TEST_CASE("[global] Find all next jobs") {
	NP::Job<dtime_t>::Job_set jobs{
		NP::Job<dtime_t>{1, I( 0,  0), I(3, 8), 100, 1},
		NP::Job<dtime_t>{2, I( 7,  7), I(5, 5),  100, 2},
		NP::Job<dtime_t>{3, I(10, 10), I(1, 11),  100, 3},
	};

	SUBCASE("Naive exploration") {
		auto space = NP::Global::State_space<dtime_t>::explore_naively(jobs);
		CHECK(space.is_schedulable());

		CHECK(space.get_finish_times(jobs[0]).from()  == 3);
		CHECK(space.get_finish_times(jobs[0]).until() == 8);

		CHECK(space.get_finish_times(jobs[1]).from()  == 12);
		CHECK(space.get_finish_times(jobs[1]).until() == 13);

		CHECK(space.get_finish_times(jobs[2]).from()  == 13);
		CHECK(space.get_finish_times(jobs[2]).until() == 24);
	}

	SUBCASE("Exploration with merging") {
		auto space = NP::Global::State_space<dtime_t>::explore(jobs);
		CHECK(space.is_schedulable());

		CHECK(space.get_finish_times(jobs[0]).from()  == 3);
		CHECK(space.get_finish_times(jobs[0]).until() == 8);

		CHECK(space.get_finish_times(jobs[1]).from()  == 12);
		CHECK(space.get_finish_times(jobs[1]).until() == 13);

		CHECK(space.get_finish_times(jobs[2]).from()  == 13);
		CHECK(space.get_finish_times(jobs[2]).until() == 24);
	}

}

TEST_CASE("[global] Consider large enough interval") {
	NP::Job<dtime_t>::Job_set jobs{
		NP::Job<dtime_t>{1, I( 0,  0), I(3, 10),  100, 3},
		NP::Job<dtime_t>{2, I( 7,  7),  I(5, 5),  100, 2},
		NP::Job<dtime_t>{3, I(10, 10),  I(5, 5),  100, 1},
	};

	auto nspace = NP::Global::State_space<dtime_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  3);
	CHECK(nspace.get_finish_times(jobs[0]).until() == 10);

	CHECK(nspace.get_finish_times(jobs[1]).from()  == 12);
	CHECK(nspace.get_finish_times(jobs[1]).until() == 20);

	CHECK(nspace.get_finish_times(jobs[2]).from()  == 15);
	CHECK(nspace.get_finish_times(jobs[2]).until() == 19);

	auto space = NP::Global::State_space<dtime_t>::explore_naively(jobs);
	CHECK(space.is_schedulable());

	CHECK(space.get_finish_times(jobs[0]).from()  ==  3);
	CHECK(space.get_finish_times(jobs[0]).until() == 10);

	CHECK(space.get_finish_times(jobs[1]).from()  == 12);
	CHECK(space.get_finish_times(jobs[1]).until() == 20);

	CHECK(space.get_finish_times(jobs[2]).from()  == 15);
	CHECK(space.get_finish_times(jobs[2]).until() == 19);
}

TEST_CASE("[global] Respect priorities") {
	NP::Job<dtime_t>::Job_set jobs{
		NP::Job<dtime_t>{1, I( 0,  0), I(3, 10),  100, 2},
		NP::Job<dtime_t>{2, I( 0,  0),  I(5, 5),  100, 1},
	};

	auto nspace = NP::Global::State_space<dtime_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  8);
	CHECK(nspace.get_finish_times(jobs[0]).until() == 15);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  5);
	CHECK(nspace.get_finish_times(jobs[1]).until() ==  5);

	auto space = NP::Global::State_space<dtime_t>::explore(jobs);
	CHECK(space.is_schedulable());

	CHECK(space.get_finish_times(jobs[0]).from()  ==  8);
	CHECK(space.get_finish_times(jobs[0]).until() == 15);

	CHECK(space.get_finish_times(jobs[1]).from()  ==  5);
	CHECK(space.get_finish_times(jobs[1]).until() ==  5);
}

TEST_CASE("[global] Respect jitter") {
	NP::Job<dtime_t>::Job_set jobs{
		NP::Job<dtime_t>{1, I( 0,  1), I(3, 10),  100, 2},
		NP::Job<dtime_t>{2, I( 0,  1),  I(5, 5),  100, 1},
	};

	auto nspace = NP::Global::State_space<dtime_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  3);
	CHECK(nspace.get_finish_times(jobs[0]).until() == 16);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  5);
	CHECK(nspace.get_finish_times(jobs[1]).until() == 15);

	auto space = NP::Global::State_space<dtime_t>::explore(jobs);
	CHECK(space.is_schedulable());

	CHECK(space.get_finish_times(jobs[0]).from()  ==  3);
	CHECK(space.get_finish_times(jobs[0]).until() == 16);

	CHECK(space.get_finish_times(jobs[1]).from()  ==  5);
	CHECK(space.get_finish_times(jobs[1]).until() == 15);
}

TEST_CASE("[global] Be eager") {
	NP::Job<dtime_t>::Job_set jobs{
		NP::Job<dtime_t>{1, I( 0,  0),  I(1,  5),  100, 2},
		NP::Job<dtime_t>{2, I( 0,  0),  I(1, 20),  100, 3},
		NP::Job<dtime_t>{3, I(10, 10),  I(5,  5),  100, 1},
	};

	auto nspace = NP::Global::State_space<dtime_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  1);
	CHECK(nspace.get_finish_times(jobs[0]).until() ==  5);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  2);
	CHECK(nspace.get_finish_times(jobs[1]).until() ==  25);

	CHECK(nspace.get_finish_times(jobs[2]).from()  ==  15);
	CHECK(nspace.get_finish_times(jobs[2]).until() ==  30);

	auto space = NP::Global::State_space<dtime_t>::explore(jobs);
	CHECK(space.is_schedulable());

	CHECK(space.get_finish_times(jobs[0]).from()  ==  1);
	CHECK(space.get_finish_times(jobs[0]).until() ==  5);

	CHECK(space.get_finish_times(jobs[1]).from()  ==  2);
	CHECK(space.get_finish_times(jobs[1]).until() ==  25);

	CHECK(space.get_finish_times(jobs[2]).from()  ==  15);
	CHECK(space.get_finish_times(jobs[2]).until() ==  30);
}


TEST_CASE("[global] Be eager, with short deadline") {
	NP::Job<dtime_t>::Job_set jobs{
		NP::Job<dtime_t>{1, I( 0,  0),  I(1,  5),  100, 2},
		NP::Job<dtime_t>{2, I( 9,  9),  I(1, 15),   25, 3},
		NP::Job<dtime_t>{3, I(30, 30),  I(5,  5),  100, 1},
	};

	auto nspace = NP::Global::State_space<dtime_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  1);
	CHECK(nspace.get_finish_times(jobs[0]).until() ==  5);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  10);
	CHECK(nspace.get_finish_times(jobs[1]).until() ==  24);

	CHECK(nspace.get_finish_times(jobs[2]).from()  ==  35);
	CHECK(nspace.get_finish_times(jobs[2]).until() ==  35);

	auto space = NP::Global::State_space<dtime_t>::explore(jobs);
	CHECK(space.is_schedulable());

	CHECK(space.get_finish_times(jobs[0]).from()  ==  1);
	CHECK(space.get_finish_times(jobs[0]).until() ==  5);

	CHECK(space.get_finish_times(jobs[1]).from()  ==  10);
	CHECK(space.get_finish_times(jobs[1]).until() ==  24);

	CHECK(space.get_finish_times(jobs[2]).from()  ==  35);
	CHECK(space.get_finish_times(jobs[2]).until() ==  35);
}


TEST_CASE("[global] Treat equal-priority jobs correctly") {
	NP::Job<dtime_t>::Job_set jobs{
		NP::Job<dtime_t>{1, I(    0,    10),  I( 2,    50),  2000, 1},
		NP::Job<dtime_t>{2, I(    0,    10),  I(50,  1200),  5000, 2},
		NP::Job<dtime_t>{3, I( 1000,  1010),  I( 2,    50),  3000, 1},
	};

	auto nspace = NP::Global::State_space<dtime_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  2);
	CHECK(nspace.get_finish_times(jobs[0]).until() ==  1259);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  50);
	CHECK(nspace.get_finish_times(jobs[1]).until() ==  1260);

	CHECK(nspace.get_finish_times(jobs[2]).from()  ==  1002);
	CHECK(nspace.get_finish_times(jobs[2]).until() ==  1310);

	auto space = NP::Global::State_space<dtime_t>::explore(jobs);
	CHECK(space.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  2);
	CHECK(nspace.get_finish_times(jobs[0]).until() ==  1259);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  50);
	CHECK(nspace.get_finish_times(jobs[1]).until() ==  1260);

	CHECK(nspace.get_finish_times(jobs[2]).from()  ==  1002);
	CHECK(nspace.get_finish_times(jobs[2]).until() ==  1310);
}

TEST_CASE("[global] Equal-priority simultaneous arrivals") {
	NP::Job<dtime_t>::Job_set jobs{
		NP::Job<dtime_t>{1, I(    0,    10),  I(  2,    50),  2000, 2000, 1},
		NP::Job<dtime_t>{2, I(    0,    10),  I(100,   150),  2000, 2000, 2},
	};

	auto nspace = NP::Global::State_space<dtime_t>::explore_naively(jobs);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  2);
	CHECK(nspace.get_finish_times(jobs[0]).until() ==   9 + 150 + 50);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  100);
	CHECK(nspace.get_finish_times(jobs[1]).until() ==  10 + 50 + 150);

	auto space = NP::Global::State_space<dtime_t>::explore(jobs);
	CHECK(space.is_schedulable());

	CHECK(nspace.get_finish_times(jobs[0]).from()  ==  2);
	CHECK(nspace.get_finish_times(jobs[0]).until() ==   9 + 150 + 50);

	CHECK(nspace.get_finish_times(jobs[1]).from()  ==  100);
	CHECK(nspace.get_finish_times(jobs[1]).until() ==  10 + 50 + 150);
}

TEST_CASE("[global] don't skip over deadline-missing jobs") {
	NP::Job<dtime_t>::Job_set jobs{
		NP::Job<dtime_t>{1, I(  100,   100),  I(   2,    50),   200, 1},
		NP::Job<dtime_t>{2, I(    0,     0),  I(1200,  1200),  5000, 2},
		NP::Job<dtime_t>{3, I(  200,   250),  I( 2,    50),    6000, 3},
		NP::Job<dtime_t>{4, I(  200,   250),  I( 2,    50),    6000, 4},
		NP::Job<dtime_t>{5, I(  200,   250),  I( 2,    50),    6000, 5},
	};

	auto nspace = NP::Global::State_space<dtime_t>::explore_naively(jobs);
	CHECK(!nspace.is_schedulable());

	CHECK(nspace.number_of_edges() == 2);
	CHECK(nspace.number_of_states() == 3);

	auto space = NP::Global::State_space<dtime_t>::explore(jobs);
	CHECK(!space.is_schedulable());

	CHECK(space.number_of_edges() == 2);
	CHECK(space.number_of_states() == 3);
}

TEST_CASE("[global] explore across bucket boundaries") {
	NP::Job<dtime_t>::Job_set jobs{
		NP::Job<dtime_t>{1, I(  100,   100),  I(  50,   50),  10000, 1},
		NP::Job<dtime_t>{2, I( 3000,  3000),  I(4000, 4000),  10000, 2},
		NP::Job<dtime_t>{3, I( 6000,  6000),  I(   2,    2),  10000, 3},
	};

	auto nspace = NP::Global::State_space<dtime_t>::explore_naively(jobs, 2);
	CHECK(nspace.is_schedulable());

	CHECK(nspace.number_of_edges() == 3);

	auto space = NP::Global::State_space<dtime_t>::explore(jobs, 2);
	CHECK(space.is_schedulable());

	CHECK(space.number_of_edges() == 3);
}

