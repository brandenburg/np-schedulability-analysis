#ifndef IO_HPP
#define IO_HPP

#include <iostream>
#include <utility>


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

	inline bool skip_one(std::istream& in, char c)
	{
		if (in.good() && in.peek() == (int) c) {
			in.get(); /* skip */
			return true;
		} else
			return false;
	}

	inline void skip_all(std::istream& in, char c)
	{
		while (skip_one(in, c))
			/* skip */;
	}

	inline bool more_data(std::istream& in)
	{
		in.peek();
		return !in.eof();
	}

	inline void next_field(std::istream& in)
	{
		// eat up any trailing spaces
		skip_all(in, ' ');
		// eat up field separator
		skip_one(in, ',');
	}

	inline void next_line(std::istream& in)
	{
		skip_over(in, '\n');
	}

	inline JobID parse_job_id(std::istream& in)
	{
		unsigned long jid, tid;

		in >> tid;
		next_field(in);
		in >> jid;
		return JobID(jid, tid);
	}

	typedef std::pair<JobID, JobID> PrecedenceConstraint;

	inline PrecedenceConstraint parse_precedence_constraint(std::istream &in)
	{
		std::ios_base::iostate state_before = in.exceptions();
		in.exceptions(std::istream::failbit | std::istream::badbit);

		// first two columns
		auto from = parse_job_id(in);

		next_field(in);

		// last two columns
		auto to = parse_job_id(in);

		in.exceptions(state_before);

		return PrecedenceConstraint(from, to);
	}

	typedef std::vector<PrecedenceConstraint> PrecedenceConstraints;

	inline PrecedenceConstraints parse_dag_file(std::istream& in)
	{
		PrecedenceConstraints edges;

		// skip column headers
		next_line(in);

		// parse all rows
		while (more_data(in)) {
			// each row contains one precedence constraint
			edges.push_back(parse_precedence_constraint(in));
			next_line(in);
		}

		return edges;
	}

	template<class Time> Job<Time> parse_job(std::istream& in)
	{
		unsigned long tid, jid;

		std::ios_base::iostate state_before = in.exceptions();

		Time arr_min, arr_max, cost_min, cost_max, dl, prio;

		in.exceptions(std::istream::failbit | std::istream::badbit);

		in >> tid;
		next_field(in);
		in >> jid;
		next_field(in);
		in >> arr_min;
		next_field(in);
		in >> arr_max;
		next_field(in);
		in >> cost_min;
		next_field(in);
		in >> cost_max;
		next_field(in);
		in >> dl;
		next_field(in);
		in >> prio;

		in.exceptions(state_before);

		return Job<Time>{jid, Interval<Time>{arr_min, arr_max},
						 Interval<Time>{cost_min, cost_max}, dl, prio, tid};
	}

	template<class Time>
	typename Uniproc::State_space<Time>::Workload parse_file(std::istream& in)
	{
		// first row contains a comment, just skip it
		next_line(in);

		typename Uniproc::State_space<Time>::Workload jobs;

		while (more_data(in)) {
			jobs.push_back(parse_job<Time>(in));
			// munge any trailing whitespace or extra columns
			next_line(in);
		}

		return jobs;
	}

}

#endif
