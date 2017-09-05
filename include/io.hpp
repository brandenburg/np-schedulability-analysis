#ifndef IO_HPP
#define IO_HPP

#include <iostream>


#include "interval.hpp"
#include "time.hpp"
#include "jobs.hpp"
#include "schedule_space.hpp"


namespace NP {

	inline void skip_over(std::istream& in, char c)
	{
		while (in.good() && in.get() != (int) c)
			/* skip */;
	}

	inline bool more_data(std::istream& in)
	{
		in.peek();
		return !in.eof();
	}

	template<class Time> Job<Time> parse_job(std::istream& in)
	{
		unsigned long tid, jid;

		std::ios_base::iostate state_before = in.exceptions();

		Time arr_min, arr_max, cost_min, cost_max, dl, prio;

		in.exceptions(std::istream::failbit | std::istream::badbit);

		in >> tid;
		skip_over(in, ',');
		in >> jid;
		skip_over(in, ',');
		in >> arr_min;
		skip_over(in, ',');
		in >> arr_max;
		skip_over(in, ',');
		in >> cost_min;
		skip_over(in, ',');
		in >> cost_max;
		skip_over(in, ',');
		in >> dl;
		skip_over(in, ',');
		in >> prio;

		in.exceptions(state_before);

		return Job<Time>{jid, Interval<Time>{arr_min, arr_max},
						 Interval<Time>{cost_min, cost_max}, dl, prio, tid};
	}

	template<class Time>
	typename Uniproc::State_space<Time>::Workload parse_file(std::istream& in)
	{
		// first row contains a comment, just skip it
		skip_over(in, '\n');

		typename Uniproc::State_space<Time>::Workload jobs;

		while (more_data(in)) {
			jobs.push_back(parse_job<Time>(in));
			// munge any trailing whitespace or extra columns
			skip_over(in, '\n');
		}

		return jobs;
	}

}

#endif
