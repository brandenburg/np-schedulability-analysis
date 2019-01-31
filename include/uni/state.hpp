#include <iostream>
#include <ostream>
#include <cassert>
#include <algorithm>

#include <set>

#include "index_set.hpp"
#include "jobs.hpp"
#include "cache.hpp"

namespace NP {

	// use pointers as a primitive form of unique ID

	namespace Uniproc {

		typedef Index_set Job_set;

		template<class Time> class Schedule_state
		{
			private:

			Interval<Time> finish_time;
			Time earliest_pending_release;

			Job_set scheduled_jobs;
			hash_value_t lookup_key;

			// no accidental copies
			Schedule_state(const Schedule_state& origin)  = delete;

			public:

			// initial state
			Schedule_state()
			: finish_time{0, 0}
			, lookup_key{0}
			, earliest_pending_release{0}
			{
			}

			// transition: new state by scheduling a job in an existing state
			Schedule_state(
				const Schedule_state& from,
				const Job<Time>& j,
				std::size_t idx,
				Interval<Time> ftimes,
				const Time next_earliest_release)
			: finish_time{ftimes}
			, scheduled_jobs{from.scheduled_jobs, idx}
			, lookup_key{from.next_key(j)}
			, earliest_pending_release{next_earliest_release}
			{
			}

			Time earliest_finish_time() const
			{
				return finish_time.from();
			}

			Time latest_finish_time() const
			{
				return finish_time.until();
			}

			Time earliest_job_release() const
			{
				return earliest_pending_release;
			}

			const Interval<Time>& finish_range() const
			{
				return finish_time;
			}

			void update_finish_range(const Interval<Time> &update)
			{
				assert(update.intersects(finish_time));
				finish_time.widen(update);
			}

			hash_value_t get_key() const
			{
				return lookup_key;
			}

			const Job_set& get_scheduled_jobs() const
			{
				return scheduled_jobs;
			}

			bool matches(const Schedule_state& other) const
			{
				return lookup_key == other.lookup_key &&
					   scheduled_jobs == other.scheduled_jobs;
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
