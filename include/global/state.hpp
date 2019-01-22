#include <iostream>
#include <ostream>
#include <cassert>
#include <algorithm>

#include <set>

#include "util.hpp"
#include "index_set.hpp"
#include "jobs.hpp"
#include "cache.hpp"

namespace NP {

	namespace Global {

		typedef std::size_t Job_index;
		typedef std::vector<Job_index> Job_precedence_set;

		template<class Time> class Schedule_state
		{
			public:

			// initial state -- nothing yet has finished, nothing is running
			Schedule_state(unsigned int num_processors)
			: scheduled_jobs()
			, num_jobs_scheduled(0)
			, core_avail{num_processors, Interval<Time>(Time(0), Time(0))}
			, lookup_key{0x9a9a9a9a9a9a9a9aUL}
			{
				assert(core_avail.size() > 0);
			}

			// transition: new state by scheduling a job in an existing state,
			//             by replacing a given running job.
			Schedule_state(
				const Schedule_state& from,
				Job_index j,
				const Job_precedence_set& predecessors,
				Interval<Time> start_times,
				Interval<Time> finish_times,
				hash_value_t key)
			: num_jobs_scheduled(from.num_jobs_scheduled + 1)
			, scheduled_jobs{from.scheduled_jobs, j}
			, lookup_key{from.lookup_key ^ key}
			{
				auto est = start_times.min();
				auto lst = start_times.max();
				auto eft = finish_times.min();
				auto lft = finish_times.max();

				DM("est: " << est << std::endl
				<< "lst: " << lst << std::endl
				<< "eft: " << eft << std::endl
				<< "lft: " << lft << std::endl);

				std::vector<Time> ca, pa;

				pa.push_back(eft);
				ca.push_back(lft);

				// skip first element in from.core_avail
				for (int i = 1; i < from.core_avail.size(); i++) {
					pa.push_back(std::max(est, from.core_avail[i].min()));
					ca.push_back(std::max(est, from.core_avail[i].max()));
				}

				// update scheduled jobs
				// keep it sorted to make it easier to merge
				bool added_j = false;
				for (const auto& rj : from.certain_jobs) {
					auto x = rj.first;
					auto x_eft = rj.second.min();
					auto x_lft = rj.second.max();
					if (contains(predecessors, x)) {
						if (lst < x_lft) {
							auto pos = std::find(ca.begin(), ca.end(), x_lft);
							if (pos != ca.end())
								*pos = lst;
						}
					} else if (lst <= x_eft) {
						if (!added_j && rj.first > j) {
							// right place to add j
							certain_jobs.emplace_back(j, finish_times);
							added_j = true;
						}
						certain_jobs.emplace_back(rj);
					}
				}
				// if we didn't add it yet, add it at the back
				if (!added_j)
					certain_jobs.emplace_back(j, finish_times);


				// sort in non-decreasing order
				std::sort(pa.begin(), pa.end());
				std::sort(ca.begin(), ca.end());

				for (int i = 0; i < from.core_avail.size(); i++) {
					DM(i << " -> " << pa[i] << ":" << ca[i] << std::endl);
					core_avail.emplace_back(pa[i], ca[i]);
				}

				assert(core_avail.size() > 0);
				DM("*** new state: constructed " << *this << std::endl);
			}

			hash_value_t get_key() const
			{
				return lookup_key;
			}

			bool same_jobs_scheduled(const Schedule_state &other) const
			{
				return scheduled_jobs == other.scheduled_jobs;
			}

			bool can_merge_with(const Schedule_state<Time>& other) const
			{
				assert(core_avail.size() == other.core_avail.size());

				if (get_key() != other.get_key())
					return false;
				if (!same_jobs_scheduled(other))
					return false;
				for (int i = 0; i < core_avail.size(); i++)
					if (!core_avail[i].intersects(other.core_avail[i]))
						return false;
				return true;
			}

			bool try_to_merge(const Schedule_state<Time>& other)
			{
				if (!can_merge_with(other))
					return false;

				for (int i = 0; i < core_avail.size(); i++)
					core_avail[i] |= other.core_avail[i];

				// vector to collect joint certain jobs
				std::vector<std::pair<Job_index, Interval<Time>>> new_cj;

				// walk both sorted job lists to see if we find matches
				auto it = certain_jobs.begin();
				auto jt = other.certain_jobs.begin();
				while (it != certain_jobs.end() &&
				       jt != other.certain_jobs.end()) {
					if (it->first == jt->first) {
						// same job
						new_cj.emplace_back(it->first, it->second | jt->second);
						it++;
						jt++;
					} else if (it->first < jt->first)
						it++;
					else
						jt++;
				}
				// move new certain jobs into the state
				certain_jobs.swap(new_cj);

				DM("+++ merged " << other << " into " << *this << std::endl);

				return true;
			}

			const unsigned int number_of_scheduled_jobs() const
			{
				return num_jobs_scheduled;
			}

			Interval<Time> core_availability() const
			{
				assert(core_avail.size() > 0);
				return core_avail[0];
			}

			bool get_finish_times(Job_index j, Interval<Time> &ftimes) const
			{
				for (const auto& rj : certain_jobs) {
					// check index
					if (j == rj.first) {
						ftimes = rj.second;
						return true;
					}
					// Certain_jobs is sorted in order of increasing job index.
					// If we see something larger than 'j' we are not going
					// to find it. For large processor counts, it might make
					// sense to do a binary search instead.
					if (j < rj.first)
						return false;
				}
				return false;
			}

			const bool job_incomplete(Job_index j) const
			{
				return !scheduled_jobs.contains(j);
			}

			const bool job_ready(const Job_precedence_set& predecessors) const
			{
				for (auto j : predecessors)
					if (!scheduled_jobs.contains(j))
						return false;
				return true;
			}

			friend std::ostream& operator<< (std::ostream& stream,
			                                 const Schedule_state<Time>& s)
			{
				stream << "Global::State(";
				for (const auto& a : s.core_avail)
					stream << "[" << a.from() << ", " << a.until() << "] ";
				stream << "(";
				for (const auto& rj : s.certain_jobs)
					stream << rj.first << "";
				stream << ") " << s.scheduled_jobs << ")";
				stream << " @ " << &s;
				return stream;
			}

			void print_vertex_label(std::ostream& out,
				const typename Job<Time>::Job_set& jobs) const
			{
				for (const auto& a : core_avail)
					out << "[" << a.from() << ", " << a.until() << "] ";
				out << "\\n";
				bool first = true;
				out << "{";
				for (const auto& rj : certain_jobs) {
					if (!first)
						out << ", ";
					out << "T" << jobs[rj.first].get_task_id()
					    << "J" << jobs[rj.first].get_job_id() << ":"
					    << rj.second.min() << "-" << rj.second.max();
					first = false;
				}
				out << "}";
			}

			private:

			const unsigned int num_jobs_scheduled;

			// set of jobs that have been dispatched (may still be running)
			const Index_set scheduled_jobs;

			// imprecise set of certainly running jobs
			std::vector<std::pair<Job_index, Interval<Time>>> certain_jobs;

			// system availability intervals
			std::vector<Interval<Time>> core_avail;

			const hash_value_t lookup_key;

			// no accidental copies
			Schedule_state(const Schedule_state& origin)  = delete;
		};

	}
}
