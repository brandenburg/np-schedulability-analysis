#ifndef JOBS_HPP
#define JOBS_HPP

#include <ostream>
#include <functional> // for hash

#include "time.hpp"

namespace NP {

	typedef std::size_t hash_value_t;

	template<class Time> class Job {

	typedef Time Priority; // Make it a time value to support EDF

	private:
		Interval<Time> arrival;
		Interval<Time> cost;
		Time deadline;
		Priority priority;
		unsigned long id, task_id;
		hash_value_t key;

		void compute_hash() {
			auto h = std::hash<Time>{};
			key = h(arrival.from());
			key = (key << 1) ^ h(task_id);
			key = (key << 1) ^ h(arrival.until());
			key = (key << 1) ^ h(cost.from());
			key = (key << 1) ^ h(cost.upto());
			key = (key << 1) ^ h(deadline);
			key = (key << 1) ^ h(priority);
			key = (key << 1) ^ h(id);
		}

	public:

		Job(unsigned long id,
			Interval<Time> arr, Interval<Time> cost,
			Time dl, Priority prio,
			unsigned long tid = 0)
		: arrival(arr), cost(cost),
		  deadline(dl), priority(prio), id(id), task_id(tid)
		{
			compute_hash();
		}

		hash_value_t get_key() const
		{
			return key;
		}

		Time earliest_arrival() const
		{
			return arrival.from();
		}

		Time latest_arrival() const
		{
			return arrival.until();
		}

		Time least_cost() const
		{
			return cost.from();
		}

		Time maximal_cost() const
		{
			return cost.upto();
		}

		Priority get_priority() const
		{
			return priority;
		}

		Time get_deadline() const
		{
			return deadline;
		}

		unsigned long get_id() const
		{
			return id;
		}

		unsigned long get_task_id() const
		{
			return task_id;
		}

		bool higher_priority_than(const Job &other) const
		{
			return priority < other.priority ||
			       (priority == other.priority &&
			        latest_arrival() < other.earliest_arrival());
		}

		bool priority_at_least_that_of(const Job &other) const
		{
			return priority <= other.priority;
		}

		bool priority_exceeds(Priority prio_level) const
		{
			return priority < prio_level;
		}

		bool priority_at_least(Priority prio_level) const
		{
			return priority <= prio_level;
		}

		Interval<Time> scheduling_window() const
		{
			// inclusive interval, so take off one epsilon
			return Interval<Time>{
			                earliest_arrival(),
			                deadline - Time_model::constants<Time>::epsilon()};
		}

		static Interval<Time> scheduling_window(const Job& j)
		{
			return j.scheduling_window();
		}

		friend std::ostream& operator<< (std::ostream& stream, const Job& j)
		{
			stream << "Job{" << j.id << ", " << j.arrival << ", "
			       << j.cost << ", " << j.deadline << ", " << j.priority
			       << ", " << j.task_id << "}";
			return stream;
		}

	};

}

// inline std::ostream& operator<< (std::ostream& stream, const NP::Job& j)
// {
// }

namespace std {
	template<class T> struct hash<NP::Job<T>>
    {
		std::size_t operator()(NP::Job<T> const& j) const
        {
            return j.get_key();
        }
    };
}

#endif
