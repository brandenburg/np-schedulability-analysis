#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <algorithm>
#include <iostream>

#include "jobs.hpp"
#include "schedule_space.hpp"

using namespace NP;

static const auto inf = Time_model::constants<dtime_t>::infinity();

TEST_CASE("Intervals") {
	auto i1 = Interval<dtime_t>{10, 20};
	auto i2 = Interval<dtime_t>{15, 25};
	auto i3 = Interval<dtime_t>{21, 30};
	auto i4 = Interval<dtime_t>{5,  45};

	Interval<dtime_t> ivals[]{i1, i2, i3, i4};

	CHECK(i1.intersects(i2));
	CHECK(i2.intersects(i3));
	CHECK(i1.disjoint(i3));

	for (auto i : ivals)
		CHECK(i.intersects(i4));

	CHECK(i1.merge(i2).merge(i3) == I(10, 30));

	CHECK(I(10, 20).intersects(I(10, 20)));
}



TEST_CASE("Job hashes work") {
	Job<dtime_t> j1{9,  I(0, 0), I(3, 13), 60, 60};
	Job<dtime_t> j2{9,  I(0, 0), I(3, 13), 60, 60};
	Job<dtime_t> j3{10, I(0, 0), I(3, 13), 60, 60};

	auto h = std::hash<Job<dtime_t>>{};

	CHECK(h(j1) == h(j2));
	CHECK(h(j3) != h(j1));
}


TEST_CASE("Interval LUT") {

	Interval_lookup_table<dtime_t, Job<dtime_t>, &Job<dtime_t>::scheduling_window> lut(I(0, 60), 10);

	Job<dtime_t> j1{10, I(0, 0), I(3, 13), 60, 60};

	lut.insert(j1);

	int count = 0;
	for (auto j : lut.lookup(30))
		count++;

	CHECK(count == 1);
}


TEST_CASE("state space") {

	NP::Uniproc::Schedule_state<dtime_t> s0;

	CHECK(s0.earliest_finish_time() == 0);
	CHECK(s0.latest_finish_time() == 0);

	auto h = std::hash<Uniproc::Schedule_state<dtime_t>>{};

	CHECK(h(s0) == 0);

	Job<dtime_t> j1{10, I(0, 0), I(3, 13), 60, 60};

	CHECK(j1.least_cost() == 3);
	CHECK(j1.maximal_cost() == 13);
	CHECK(j1.earliest_arrival() == 0);
	CHECK(j1.latest_arrival() == 0);

	NP::Uniproc::Schedule_state<dtime_t> s1{s0, j1, 0, inf, inf};

	CHECK(s1.earliest_finish_time() == j1.least_cost());
	CHECK(s1.latest_finish_time() == j1.maximal_cost());

//	auto sp = NP::Uniproc::State_space
}


TEST_CASE("bool vector assumptions") {
	std::vector<bool> v1(100);

	CHECK(v1.size() == 100);
	CHECK(!v1[10]);
	v1[10] = true;

	std::vector<bool> v2(400);

	v2 = v1;
	CHECK(v2.size() == 100);
	CHECK(v2.capacity() > v1.capacity());

	std::vector<bool> v3(400);
	std::copy(v1.begin(), v1.end(), v3.begin());
	CHECK(v3.size() == 400);
	CHECK(v3.capacity() > v1.capacity());
	CHECK(v3[10]);
}

