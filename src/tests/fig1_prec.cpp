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
	auto in = std::istringstream(fig1a_jobs_file);

	Scheduling_problem<dtime_t> prob{
		parse_file<dtime_t>(in),
		parse_dag_file(dag_in)};

	Analysis_options opts;

	opts.be_naive = true;
	auto nspace = Uniproc::State_space<dtime_t>::explore(prob, opts);
	CHECK(nspace.is_schedulable());

	opts.be_naive = false;
	auto space = Uniproc::State_space<dtime_t>::explore(prob, opts);
	CHECK(space.is_schedulable());

	for (const Job<dtime_t>& j : prob.jobs) {
		CHECK(nspace.get_finish_times(j) == space.get_finish_times(j));
		CHECK(nspace.get_finish_times(j).from() != 0);
	}
}

const std::string prec_dag_file_with_cycle =
"Predecessor TID,	Predecessor JID,	Successor TID, Successor JID\n"
"1, 1,    1, 2\n"
"1, 2,    1, 3\n"
"1, 3,    1, 4\n"
"1, 4,    1, 5\n"
"1, 5,    1, 6\n"
"1, 6,    1, 1\n"  // back edge
"2, 7,    2, 8\n"
"1, 2,    3, 9\n";

TEST_CASE("[prec] handle cycles gracefully") {
	auto dag_in = std::istringstream(prec_dag_file_with_cycle);
	auto in = std::istringstream(fig1a_jobs_file);

	Scheduling_problem<dtime_t> prob{
		parse_file<dtime_t>(in),
		parse_dag_file(dag_in)};

	Analysis_options opts;
	opts.early_exit = false;

	opts.be_naive = true;
	auto nspace = Uniproc::State_space<dtime_t>::explore(prob, opts);
	CHECK_FALSE(nspace.is_schedulable());

	opts.be_naive = false;
	auto space = Uniproc::State_space<dtime_t>::explore(prob, opts);
	CHECK_FALSE(space.is_schedulable());
}

const std::string deadend_jobs_file =
"   Task ID,     Job ID,          Arrival min,          Arrival max,             Cost min,             Cost max,             Deadline,             Priority\n"
"1, 1,  0,  0, 1,  2, 10, 10\n"
"1, 2, 10, 10, 1,  2, 20, 20\n"
"1, 3, 20, 20, 1,  2, 30, 30\n"
"1, 4, 30, 30, 1,  2, 40, 40\n"
"1, 5, 40, 40, 1,  2, 50, 50\n"
"1, 6, 50, 50, 1,  2, 60, 60\n";


const std::string deadend_dag_file =
"Predecessor TID,	Predecessor JID,	Successor TID, Successor JID\n"
"1, 1,    1, 2\n"
"1, 2,    1, 3\n"
"1, 3,    1, 4\n"
"1, 4,    1, 5\n"
"1, 5,    1, 6\n"
"1, 6,    1, 1\n";  // back edge


TEST_CASE("[prec] handle analysis deadend gracefully") {
	auto dag_in = std::istringstream(deadend_dag_file);
	auto in = std::istringstream(deadend_jobs_file);

	Scheduling_problem<dtime_t> prob{
		parse_file<dtime_t>(in),
		parse_dag_file(dag_in)};

	Analysis_options opts;
	opts.early_exit = false;

	opts.be_naive = true;
	auto nspace = Uniproc::State_space<dtime_t>::explore(prob, opts);
	CHECK_FALSE(nspace.is_schedulable());

	opts.be_naive = false;
	auto space = Uniproc::State_space<dtime_t>::explore(prob, opts);
	CHECK_FALSE(space.is_schedulable());
}

