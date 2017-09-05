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

	CHECK(j.get_id() == 6);
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

	CHECK(jobs[0].get_id() == 1);
	CHECK(jobs[1].get_id() == 2);
	CHECK(jobs[2].get_id() == 3);

	CHECK(jobs[0].earliest_arrival() ==     0);
	CHECK(jobs[1].earliest_arrival() == 10000);
	CHECK(jobs[2].earliest_arrival() == 20000);

	CHECK(jobs[0].get_deadline() == 10000);
	CHECK(jobs[1].get_deadline() == 20000);
	CHECK(jobs[2].get_deadline() == 30000);
}
