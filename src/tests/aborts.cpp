#include "doctest.h"

#include <iostream>
#include <sstream>

#include "problem.hpp"
#include "io.hpp"
#include "uni/space.hpp"


using namespace NP;


// - basic semantics

const std::string basic_jobs_file =
"TID, JID, Rmin, Rmax, Cmin, Cmax,  DL, Prio\n"
"   1,  1,    0,    0,   30,  100, 150,    2\n"
"   2,  1,    0,    0,    2,    4,  60,    4\n"
"   3,  1,    0,    0,  100,  100, 100,    1\n"
"   4,  1,    0,    0,   10,   10,  10,    3\n";

const std::string basic_aborts_file =
"TID, JID, Tmin, Tmax, Cmin, Cmax\n"
"  1,   1,   50,   54,    1,    2\n"
"  3,   1,    5,    5,    0,    0\n"
"  4,   1,   10,   10,    0,    0\n";

TEST_CASE("[uni] basic aborts") {
	auto jobs_in = std::istringstream(basic_jobs_file);
	auto dag_in  = std::istringstream("\n");
	auto aborts_in = std::istringstream(basic_aborts_file);

	Scheduling_problem<dtime_t> prob{
		parse_file<dtime_t>(jobs_in),
		parse_dag_file(dag_in),
		parse_abort_file<dtime_t>(aborts_in),
		1
	};

	Analysis_options opts;
	opts.early_exit = false;

	auto space = Uniproc::State_space<dtime_t>::explore(prob, opts);
	CHECK_FALSE(space.is_schedulable());

	CHECK(space.get_finish_times(prob.jobs[1]).min() == 37);
	CHECK(space.get_finish_times(prob.jobs[1]).max() == 60);
};

const std::string cascade_jobs_file =
"TID, JID, Rmin, Rmax, Cmin, Cmax,  DL, Prio\n"
"   1,  1,    0,    0,    6,    6,   9,    1\n"
"   2,  1,   10,   10,    2,    6,  15,    2\n"
"   3,  1,   16,   16,    3,    6,  23,    3\n"
"   4,  1,    5,    5,    6,    7,  15,    4\n";

const std::string cascade_aborts_file =
"TID, JID, Tmin, Tmax, Cmin, Cmax\n"
"  2,   1,   15,   15,    0,    0\n";

TEST_CASE("[uni] abort stops DL miss cascade") {
	auto jobs_in = std::istringstream(cascade_jobs_file);
	auto dag_in  = std::istringstream("\n");
	auto aborts_in = std::istringstream(cascade_aborts_file);

	Analysis_options opts;
	opts.early_exit = false;

	SUBCASE("without aborts") {
		Scheduling_problem<dtime_t> prob{parse_file<dtime_t>(jobs_in)};

		auto space = Uniproc::State_space<dtime_t>::explore(prob, opts);
		CHECK_FALSE(space.is_schedulable());

		CHECK(space.get_finish_times(prob.jobs[0]).min() == 6);
		CHECK(space.get_finish_times(prob.jobs[0]).max() == 6);
		CHECK(space.get_finish_times(prob.jobs[1]).min() == 14);
		CHECK(space.get_finish_times(prob.jobs[1]).max() == 19);
		CHECK(space.get_finish_times(prob.jobs[2]).min() == 19);
		CHECK(space.get_finish_times(prob.jobs[2]).max() == 25);
		CHECK(space.get_finish_times(prob.jobs[3]).min() == 12);
		CHECK(space.get_finish_times(prob.jobs[3]).max() == 13);
	}

	SUBCASE("with aborts") {
		Scheduling_problem<dtime_t> prob{
			parse_file<dtime_t>(jobs_in),
			parse_dag_file(dag_in),
			parse_abort_file<dtime_t>(aborts_in),
			1
		};

		auto space = Uniproc::State_space<dtime_t>::explore(prob, opts);
		CHECK(space.is_schedulable());

		CHECK(space.get_finish_times(prob.jobs[0]).min() == 6);
		CHECK(space.get_finish_times(prob.jobs[0]).max() == 6);
		CHECK(space.get_finish_times(prob.jobs[1]).min() == 14);
		CHECK(space.get_finish_times(prob.jobs[1]).max() == 15);
		CHECK(space.get_finish_times(prob.jobs[2]).min() == 19);
		CHECK(space.get_finish_times(prob.jobs[2]).max() == 22);
		CHECK(space.get_finish_times(prob.jobs[3]).min() == 12);
		CHECK(space.get_finish_times(prob.jobs[3]).max() == 13);
	}

};
