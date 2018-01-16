#ifndef PRECEDENCE_HPP
#define PRECEDENCE_HPP

#include "jobs.hpp"

namespace NP {

	typedef std::pair<JobID, JobID> Precedence_constraint;
	typedef std::vector<Precedence_constraint> Precedence_constraints;

	template<class Time>
	void validate_prec_refs(const Precedence_constraints& dag,
	                        const typename Job<Time>::Job_set jobs)
	{
		for (auto constraint : dag) {
			lookup<Time>(jobs, constraint.first);
			lookup<Time>(jobs, constraint.second);
		}
	}

}

#endif
