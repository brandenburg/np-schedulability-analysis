#ifndef ABORT_HANDLER_HPP
#define ABORT_HANDLER_HPP

#include "jobs.hpp"

namespace NP {

	template<class Time>
	class Abort_action {

	public:

		Abort_action(JobID id,
		             Interval<Time> trigger_time,
		             Interval<Time> cleanup_cost)
		: job_id(id)
		, trigger_time(trigger_time)
		, cleanup_cost(cleanup_cost)
		{
		}

		JobID get_id() const
		{
			return job_id;
		}

		Time earliest_trigger_time() const
		{
			return trigger_time.from();
		}

		Time latest_trigger_time() const
		{
			return trigger_time.until();
		}

		Time least_cleanup_cost() const
		{
			return cleanup_cost.min();
		}

		Time maximum_cleanup_cost() const
		{
			return cleanup_cost.max();
		}

	private:
		JobID job_id;
		Interval<Time> trigger_time;
		Interval<Time> cleanup_cost;
	};


	template<class Time>
	void validate_abort_refs(const std::vector<Abort_action<Time>>& aborts,
	                         const typename Job<Time>::Job_set jobs)
	{
		for (auto action : aborts) {
			lookup<Time>(jobs, action.get_id());
		}
	}


}

#endif
