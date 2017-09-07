#include <iostream>
#include <ostream>
#include <cassert>

#include "jobs.hpp"

#define CONFIG_COLLECT_SCHEDULE_GRAPH

namespace NP {

	// use pointers as a primitive form of unique ID

	namespace Uniproc {

		template<class Time> class Schedule_state
		{
			public:

			typedef std::unordered_set<const Job<Time>*> Job_uid_set;

			private:

			Interval<Time> finish_time;

			Job_uid_set scheduled_jobs;
			hash_value_t lookup_key;

			Schedule_state(Time eft, Time lft, const Job_uid_set &jobs,
						   hash_value_t k)
			: finish_time{eft, lft}, scheduled_jobs{jobs}, lookup_key{k}
			{
			}

			public:

			static Schedule_state initial_state()
			{
				return Schedule_state{0, 0, Job_uid_set(), 0};
			}

			Time earliest_finish_time() const
			{
				return finish_time.from();
			}

			Time latest_finish_time() const
			{
				return finish_time.until();
			}

			const Interval<Time>& finish_range() const
			{
				return finish_time;
			}

			void update_finish_range(Interval<Time> &update)
			{
				assert(update.intersects(finish_time));
				finish_time.widen(update);
			}

			hash_value_t get_key() const
			{
				return lookup_key;
			}

			const Job_uid_set& get_scheduled_jobs() const
			{
				return scheduled_jobs;
			}

			bool matches(const Schedule_state& other) const
			{
				return lookup_key == other.lookup_key &&
					   scheduled_jobs == other.scheduled_jobs;
			}

			// t_S in paper, see definition 6.
			Time next_earliest_start_time(const Job<Time>& j) const
			{
				return std::max(earliest_finish_time(), j.earliest_arrival());
			}

			// e_k, equation 5
			Time next_earliest_finish_time(const Job<Time>& j) const
			{
				return next_earliest_start_time(j) + j.least_cost();
			}

			// l_k, equation 6
			Time next_latest_finish_time(
				const Job<Time>& j,
				const Time other_certain_start,
				const Time iip_latest_start) const
			{
				// t_s'
				auto own_latest_start = std::max(latest_finish_time(),
												 j.latest_arrival());

				// t_R, t_I
				auto last_start_before_other = std::min(
					other_certain_start - Time_model::constants<Time>::epsilon(),
					iip_latest_start);

				return std::min(own_latest_start, last_start_before_other)
					   + j.maximal_cost();
			}

			hash_value_t next_key(const Job<Time>& j) const
			{
				return get_key() ^ j.get_key();
			}

			Schedule_state<Time> schedule(
				const Job<Time>& j,
				const Time other_certain_start,
				const Time iip_latest_start) const
			{
				DM("cost: " << j.least_cost() << "--" << j.maximal_cost() <<  std::endl);
				DM("Other: " << other_certain_start <<  std::endl);
				Time eft = next_earliest_finish_time(j);
				Time lft = next_latest_finish_time(j, other_certain_start,
				                                   iip_latest_start);
				DM("eft:" << eft << " lft:" << lft << std::endl);
				Schedule_state next(eft, lft, scheduled_jobs, next_key(j));
				next.scheduled_jobs.insert(&j);
				return next;
			}

			friend std::ostream& operator<< (std::ostream& stream,
			                                 const Schedule_state<Time>& s)
			{
				stream << "State(" << s.finish_range() << ", {";
				bool first = true;
				for (auto j : s.get_scheduled_jobs()) {
					if (!first)
						stream << ", ";
					first = false;
					stream << j->get_id();
				}
				stream << "})";
				return stream;
			}
		};

	}
}
