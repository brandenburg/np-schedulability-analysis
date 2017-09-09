#include <iostream>
#include <ostream>
#include <cassert>

#include <set>

#include "jobs.hpp"

#define CONFIG_COLLECT_SCHEDULE_GRAPH

namespace NP {

	// use pointers as a primitive form of unique ID

	namespace Uniproc {

		template<class Time> class Job_set
		{
			public:

			typedef std::unordered_set<const Job<Time>*> Set_type;

			// new empty job set
			Job_set()
			: the_set()
			{
			}

			// derive a new job set by "cloning" an existing set and adding a job
			Job_set(const Job_set& from, const Job<Time>* njob)
			: the_set(from.the_set)
			{
				the_set.insert(njob);
			}

			bool contains(const Job<Time>& j) const
			{
				return the_set.find(&j) != the_set.end();
			}

			bool contains(const Job<Time>* j) const
			{
				return contains(*j);
			}

			std::size_t number_of_jobs() const
			{
				return the_set.size();
			}

			friend std::ostream& operator<< (std::ostream& stream,
			                                 const Job_set<Time>& s)
			{
				bool first = true;
				stream << "{";
				for (auto j : s.the_set) {
					if (!first)
						stream << ", ";
					first = false;
					stream << "T" << j->get_task_id() << " J" << j->get_id();
				}
				stream << "}";

				return stream;
			}

			private:

			Set_type the_set;

			// no accidental copies
			Job_set(const Job_set& origin) = delete;
		};



		template<class Time> class Schedule_state
		{
			public:

			private:

			Interval<Time> finish_time;

			Job_set<Time> scheduled_jobs;
			hash_value_t lookup_key;

			Schedule_state(Time eft, Time lft, const Job_set<Time> &jobs,
						   hash_value_t k)
			: finish_time{eft, lft}, scheduled_jobs{jobs}, lookup_key{k}
			{
			}

			// no accidental copies
			Schedule_state(const Schedule_state& origin)  = delete;

			public:

			// initial state
			Schedule_state()
			: finish_time{0, 0}
			, lookup_key{0}
			{
			}

			// transition: new state by scheduling a job in an existing state
			Schedule_state(
				const Schedule_state& from,
				const Job<Time>& j,
				const Time other_certain_start,
				const Time iip_latest_start)
			: finish_time{from.next_earliest_finish_time(j),
			              from.next_latest_finish_time(j, other_certain_start,
			                                          iip_latest_start)}
			, scheduled_jobs{from.scheduled_jobs, &j}
			, lookup_key{from.next_key(j)}
			{
				DM("cost: " << j.least_cost() << "--" << j.maximal_cost() <<  std::endl);
				DM("Other: " << other_certain_start <<  std::endl);
				DM("eft:" << finish_time.from() << " lft:" << finish_time.upto() << std::endl);
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

			const Job_set<Time>& get_scheduled_jobs() const
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

			friend std::ostream& operator<< (std::ostream& stream,
			                                 const Schedule_state<Time>& s)
			{
				stream << "State(" << s.finish_range() << ", "
				       << s.get_scheduled_jobs() << ")";
				return stream;
			}
		};

	}
}
