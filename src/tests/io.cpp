#include "doctest.h"

#include <iostream>
#include <sstream>

#include "io.hpp"

const std::string one_line = "       920,          6,              50000.0,              50010.0,   23.227497252002234,    838.6724123730141,              60000.0,                    1";

const std::string bad_line = "       920,          6,              foo, bar";

const std::string four_lines =
"   Task ID,     Job ID,          Arrival min,          Arrival max,             Cost min,             Cost max,             Deadline,             Priority\n"
"       920,          1,                  0.0,                 10.0,   23.227497252002234,    838.6724123730141,              10000.0,                    1\n"
"       920,          2,              10000.0,              10010.0,   23.227497252002234,    838.6724123730141,              20000.0,                    1\n"
"       920,          3,              20000.0,              20010.0,   23.227497252002234,    838.6724123730141,              30000.0,                    1\n";


TEST_CASE("[dense time] job parser") {
	auto in = std::istringstream(one_line);

	NP::Job<dense_t> j = NP::parse_job<dense_t>(in);

	CHECK(j.get_job_id() == 6);
	CHECK(j.get_priority() == 1);
	CHECK(j.get_deadline() == 60000);
}

TEST_CASE("[dense time] job parser exception") {
	auto in = std::istringstream(bad_line);

	REQUIRE_THROWS_AS(NP::parse_job<dense_t>(in), std::ios_base::failure);
}

TEST_CASE("[dense time] file parser") {
	auto in = std::istringstream(four_lines);

	auto jobs = NP::parse_file<dense_t>(in);

	CHECK(jobs.size() == 3);

	for (auto j : jobs) {
		CHECK(j.get_priority() == 1);
		CHECK(j.get_task_id() == 920);
	}

	CHECK(jobs[0].get_job_id() == 1);
	CHECK(jobs[1].get_job_id() == 2);
	CHECK(jobs[2].get_job_id() == 3);

	CHECK(jobs[0].earliest_arrival() ==     0);
	CHECK(jobs[1].earliest_arrival() == 10000);
	CHECK(jobs[2].earliest_arrival() == 20000);

	CHECK(jobs[0].get_deadline() == 10000);
	CHECK(jobs[1].get_deadline() == 20000);
	CHECK(jobs[2].get_deadline() == 30000);
}

TEST_CASE("[disc time] don't parse dense files") {
	auto in = std::istringstream(four_lines);

	REQUIRE_THROWS_AS(NP::parse_job<dtime_t>(in), std::ios_base::failure);
}

const std::string precedence_line = "1, 2, 3, 5";

const std::string bad_precedence_line = "1, 2, 3,";
const std::string bad_precedence_line2 = "1, 2, 3x, 5";

TEST_CASE("[parser] JobID") {
	auto in = std::istringstream(precedence_line);

	auto id = NP::parse_job_id(in);

	CHECK(id.job  == 2);
	CHECK(id.task == 1);
}

TEST_CASE("[parser] precedence constraint") {
	auto in = std::istringstream(precedence_line);

	auto c = NP::parse_precedence_constraint(in);

	CHECK(c.first.job  == 2);
	CHECK(c.first.task == 1);
	CHECK(c.second.job  == 5);
	CHECK(c.second.task == 3);
}

TEST_CASE("[parser] too-short precedence constraint") {
	auto in = std::istringstream(bad_precedence_line);

	REQUIRE_THROWS_AS(NP::parse_precedence_constraint(in), std::ios_base::failure);
}

TEST_CASE("[parser] invalid precedence constraint") {
	auto in = std::istringstream(bad_precedence_line2);

	REQUIRE_THROWS_AS(NP::parse_precedence_constraint(in), std::ios_base::failure);
}

const std::string precedence_file =
"Predecessor TID,	Predecessor JID,	Successor TID,	Successor JID\n"
"              1,                 1,               1,             2\n"
"              1,                 1,               2,             1\n"
"              2,                 1,               3,            13\n";

TEST_CASE("[parser] precedence file") {
	auto in = std::istringstream(precedence_file);

	auto dag = NP::parse_dag_file(in);

	CHECK(dag.size() == 3);
	CHECK(dag[0].first.task  == 1);
	CHECK(dag[0].first.job   == 1);
	CHECK(dag[0].second.task == 1);
	CHECK(dag[0].second.job  == 2);

	CHECK(dag[1].first.task  == 1);
	CHECK(dag[1].first.job   == 1);
	CHECK(dag[1].second.task == 2);
	CHECK(dag[1].second.job  == 1);

	CHECK(dag[2].first.task  == 2);
	CHECK(dag[2].first.job   == 1);
	CHECK(dag[2].second.task == 3);
	CHECK(dag[2].second.job  == 13);
}

TEST_CASE("[parser] invalid precedence reference") {
	auto dag_in = std::istringstream(precedence_file);
	auto dag = NP::parse_dag_file(dag_in);

	auto in = std::istringstream(four_lines);
	auto jobs = NP::parse_file<dense_t>(in);

	REQUIRE_THROWS_AS(NP::validate_prec_refs<dense_t>(dag, jobs), NP::InvalidJobReference);
}

const std::string sequential_task_prec_file =
"Predecessor TID,	Predecessor JID,	Successor TID,	Successor JID\n"
"            920,                 1,             920,             2\n"
"            920,                 2,             920,             3\n";

TEST_CASE("[parser] valid precedence reference") {
	auto dag_in = std::istringstream(sequential_task_prec_file);
	auto dag = NP::parse_dag_file(dag_in);

	auto in = std::istringstream(four_lines);
	auto jobs = NP::parse_file<dense_t>(in);

	NP::validate_prec_refs<dense_t>(dag, jobs);
	// dummy check; real check is that previous line didn't throw an exception
	CHECK(true);
}
