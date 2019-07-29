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

	class InvalidAbortParameter : public std::exception
	{
		public:

		InvalidAbortParameter(const JobID& bad_id)
		: ref(bad_id)
		{}

		const JobID ref;

		virtual const char* what() const noexcept override
		{
			return "invalid abort parameter";
		}

	};

	template<class Time>
	void validate_abort_refs(const std::vector<Abort_action<Time>>& aborts,
	                         const typename Job<Time>::Job_set jobs)
	{
		for (auto action : aborts) {
			auto job = lookup<Time>(jobs, action.get_id());
			if (action.earliest_trigger_time() < job.earliest_arrival() ||
			    action.latest_trigger_time() < job.latest_arrival())
				throw InvalidAbortParameter(action.get_id());
		}
	}


}

#endif
