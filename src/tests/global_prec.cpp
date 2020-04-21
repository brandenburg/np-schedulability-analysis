#include "doctest.h"

#include <iostream>
#include <sstream>

#include "io.hpp"
#include "global/space.hpp"

const std::string ts1_jobs =
"Task ID, Job ID, Arrival min, Arrival max, Cost min, Cost max, Deadline, Priority\n"
"      1,      1,           0,        6000,     5000,     9000,    30000,    30000\n"
"      1,      2,           0,        6000,     3000,     6000,    30000,    30000\n"
"      1,      3,           0,        6000,     2000,    15000,    30000,    30000\n"
"      2,      1,           0,        3000,     5000,    10000,    30000,    30000\n"
"      2,      2,           0,        3000,     3000,     5000,    30000,    30000\n";

const std::string ts1_edges =
"From TID, From JID,   To TID,   To JID\n"
"       1,        1,        1,        2\n"
"       1,        1,        1,        3\n"
"       2,        1,        2,        2\n";

const std::string ts2_jobs =
"Task ID, Job ID, Arrival min, Arrival max, Cost min, Cost max, Deadline, Priority\n"
"      1,      1,           0,           0,     2000,     5000,    40000,    40000\n"
"      1,      2,           0,           0,     3000,    10000,    40000,    40000\n"
"      1,      3,           0,           0,     3000,    10000,    40000,    40000\n"
"      1,      4,           0,           0,     3000,    10000,    40000,    40000\n"
"      1,      5,           0,           0,     5000,    15000,    40000,    40000\n"
"      2,      1,           0,       40000,        0,    10000,    80000,    80000\n"
"      1,     11,       40000,       40000,     2000,     5000,    80000,    80000\n"
"      1,     12,       40000,       40000,     3000,    10000,    80000,    80000\n"
"      1,     13,       40000,       40000,     3000,    10000,    80000,    80000\n"
"      1,     14,       40000,       40000,     3000,    10000,    80000,    80000\n"
"      1,     15,       40000,       40000,     5000,    15000,    80000,    80000\n";

const std::string ts2_edges =
"From TID, From JID,   To TID,   To JID\n"
"       1,        1,        1,        2\n"
"       1,        1,        1,        3\n"
"       1,        1,        1,        4\n"
"       1,        2,        1,        5\n"
"       1,        3,        1,        5\n"
"       1,        4,        1,        5\n"
"       1,       11,        1,       12\n"
"       1,       11,        1,       13\n"
"       1,       11,        1,       14\n"
"       1,       12,        1,       15\n"
"       1,       13,        1,       15\n"
"       1,       14,        1,       15\n";

const std::string ts3_jobs =
"Task ID, Job ID, Arrival min, Arrival max, Cost min, Cost max, Deadline, Priority\n"
"      0,      0,          10,          10,       80,       80,      110,        2\n"
"      1,      0,         200,         200,       20,       20,     8000,        4\n"
"      2,      0,         200,         200,       20,       20,     8000,        5\n"
"      3,      0,         200,         200,       40,       40,     8000,        3\n"
"      0,      1,         210,         210,       80,       80,     310,         2\n";

const std::string ts3_edges =
"From TID, From JID,   To TID,   To JID\n"
"       1,        0,        2,        0\n"
"       2,        0,        3,        0\n";

TEST_CASE("[global-prec] taskset-1") {
	auto dag_in = std::istringstream(ts1_edges);
	auto dag = NP::parse_dag_file(dag_in);

	auto in = std::istringstream(ts1_jobs);
	auto jobs = NP::parse_file<dtime_t>(in);

	NP::Scheduling_problem<dtime_t> prob{jobs, dag};
	NP::Analysis_options opts;

	prob.num_processors = 2;
	opts.be_naive = true;
	auto nspace2 = NP::Global::State_space<dtime_t>::explore(prob, opts);

	CHECK_FALSE(nspace2.is_schedulable());

	opts.be_naive = false;
	auto space2 = NP::Global::State_space<dtime_t>::explore(prob, opts);

	CHECK_FALSE(space2.is_schedulable());

	prob.num_processors = 3;
	opts.be_naive = true;
	auto nspace3 = NP::Global::State_space<dtime_t>::explore(prob, opts);

	CHECK(nspace3.is_schedulable());

	opts.be_naive = false;
	auto space3 = NP::Global::State_space<dtime_t>::explore(prob, opts);

	CHECK(space3.is_schedulable());

	for (const NP::Job<dtime_t>& j : jobs) {
		CHECK(nspace3.get_finish_times(j) == space3.get_finish_times(j));
		CHECK(nspace3.get_finish_times(j).from() != 0);
	}
}

TEST_CASE("[global-prec] taskset-2") {
	auto dag_in = std::istringstream(ts2_edges);
	auto dag = NP::parse_dag_file(dag_in);

	auto in = std::istringstream(ts2_jobs);
	auto jobs = NP::parse_file<dtime_t>(in);

	NP::Scheduling_problem<dtime_t> prob{jobs, dag};
	NP::Analysis_options opts;

	prob.num_processors = 2;
	opts.be_naive = true;
	auto nspace2 = NP::Global::State_space<dtime_t>::explore(prob, opts);

	CHECK(nspace2.is_schedulable());

	opts.be_naive = false;
	auto space2 = NP::Global::State_space<dtime_t>::explore(prob, opts);

	CHECK(space2.is_schedulable());

	for (const NP::Job<dtime_t>& j : jobs) {
		CHECK(nspace2.get_finish_times(j) == space2.get_finish_times(j));
		if (j.least_cost() != 0)
			CHECK(nspace2.get_finish_times(j).from() != 0);
	}

	prob.num_processors = 3;
	opts.be_naive = true;
	auto nspace3 = NP::Global::State_space<dtime_t>::explore(prob, opts);

	CHECK(nspace3.is_schedulable());

	opts.be_naive = false;
	auto space3 = NP::Global::State_space<dtime_t>::explore(prob, opts);

	CHECK(space3.is_schedulable());

	for (const NP::Job<dtime_t>& j : jobs) {
		CHECK(nspace3.get_finish_times(j) == space3.get_finish_times(j));
		if (j.least_cost() != 0)
			CHECK(nspace3.get_finish_times(j).from() != 0);
	}
}

TEST_CASE("[global-prec] taskset-3") {
	auto dag_in = std::istringstream(ts3_edges);
	auto dag = NP::parse_dag_file(dag_in);

	auto in = std::istringstream(ts3_jobs);
	auto jobs = NP::parse_file<dtime_t>(in);

	NP::Scheduling_problem<dtime_t> prob{jobs, dag};
	NP::Analysis_options opts;

	prob.num_processors = 1;
	opts.be_naive = false;
	auto space = NP::Global::State_space<dtime_t>::explore(prob, opts);

	CHECK(space.is_schedulable());
}
