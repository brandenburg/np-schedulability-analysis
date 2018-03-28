#include "doctest.h"

#include <iostream>
#include <sstream>

#include "io.hpp"
#include "uni/space.hpp"

using namespace NP;


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


const std::string prec_dag_file =
"Predecessor TID,	Predecessor JID,	Successor TID, Successor JID\n"
"1, 1,    1, 2\n"
"1, 2,    1, 3\n"
"1, 3,    1, 4\n"
"1, 4,    1, 5\n"
"1, 5,    1, 6\n"
"2, 7,    2, 8\n"
"1, 2,    3, 9\n";

TEST_CASE("[prec] RTSS17-Fig1a") {
	auto dag_in = std::istringstream(prec_dag_file);
	auto dag = NP::parse_dag_file(dag_in);

	auto in = std::istringstream(fig1a_jobs_file);
	auto jobs = NP::parse_file<dtime_t>(in);

	NP::validate_prec_refs<dtime_t>(dag, jobs);

	auto nspace = Uniproc::State_space<dtime_t>::explore_naively(jobs, 0, 20, dag);
	CHECK(nspace.is_schedulable());

	auto space = Uniproc::State_space<dtime_t>::explore(jobs, 0, 20, dag);
	CHECK(space.is_schedulable());

	for (const Job<dtime_t>& j : jobs) {
		CHECK(nspace.get_finish_times(j) == space.get_finish_times(j));
		CHECK(nspace.get_finish_times(j).from() != 0);
	}
}
