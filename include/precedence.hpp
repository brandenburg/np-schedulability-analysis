#ifndef PRECEDENCE_HPP
#define PRECEDENCE_HPP

#include "jobs.hpp"

namespace NP {

	typedef std::pair<JobID, JobID> PrecedenceConstraint;
	typedef std::vector<PrecedenceConstraint> PrecedenceConstraints;

	template<class Time>
	void validate_prec_refs(const PrecedenceConstraints& dag,
	                        const typename Job<Time>::JobSet jobs)
	{
		for (auto constraint : dag) {
			lookup<Time>(jobs, constraint.first);
			lookup<Time>(jobs, constraint.second);
		}
	}

}

#endif
