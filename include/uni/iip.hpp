#ifndef IIP_HPP
#define IIP_HPP

namespace NP {

	namespace Uniproc {

		template<class Time> class Null_IIP
		{
			public:

			typedef Schedule_state<Time> State;
			typedef State_space<Time, Null_IIP> Space;
			typedef typename State_space<Time, Null_IIP>::Workload Jobs;

			typedef Job_set Scheduled;

			static const bool can_block = false;

			Null_IIP(const Space &space, const Jobs &jobs) {}

			Time latest_start(const Job<Time>& j, Time t, const State& s)
			{
				return Time_model::constants<Time>::infinity();
			}
		};

		template<class Time> class Precatious_RM_IIP
		{
			public:

			typedef Schedule_state<Time> State;
			typedef State_space<Time, Precatious_RM_IIP> Space;
			typedef typename State_space<Time, Precatious_RM_IIP>::Workload Jobs;

			typedef Job_set Scheduled;

			static const bool can_block = true;

			Precatious_RM_IIP(const Space &space, const Jobs &jobs)
			: space(space), max_priority(highest_prio(jobs))
			{
				for (const Job<Time>& j : jobs)
					if (j.get_priority() == max_priority)
						hp_jobs.insert({j.latest_arrival(), &j});
				DM("IIP max priority = " << max_priority);
			}

			Time latest_start(const Job<Time>& j, Time t, const State& s)
			{
				DM("IIP P-RM for " << j << ": ");

				// Never block maximum-priority jobs
				if (j.get_priority() == max_priority) {
					DM("Self." << std::endl);
					return Time_model::constants<Time>::infinity();
				}

				for (auto it = hp_jobs.upper_bound(t); it != hp_jobs.end(); it++) {
					const Job<Time>& h = *it->second;
					if (space.incomplete(s, h)) {
						Time latest = h.get_deadline()
						              - h.maximal_cost()
						              - j.maximal_cost();

						DM("latest=" << latest << " " << h << std::endl);
						return latest;
					}
				}

				DM("None." << std::endl);

				// If we didn't find anything relevant, then there is no reason
				// to block this job.
				return Time_model::constants<Time>::infinity();
			}

			private:

			const Space &space;
			const Time max_priority;
			std::multimap<Time, const Job<Time>*> hp_jobs;

			static Time highest_prio(const Jobs &jobs)
			{
				Time prio = Time_model::constants<Time>::infinity();
				for (auto j : jobs)
					prio = std::min(prio, j.get_priority());
				return prio;
			}

		};

		template<class Time> class Critical_window_IIP
		{
			public:

			typedef Schedule_state<Time> State;
			typedef State_space<Time, Critical_window_IIP> Space;
			typedef typename State_space<Time, Critical_window_IIP>::Workload Jobs;

			typedef Job_set Scheduled;

			static const bool can_block = true;

			Critical_window_IIP(const Space &space, const Jobs &jobs)
			: space(space)
			, max_cost(maximal_cost(jobs))
			, n_tasks(count_tasks(jobs))
			{
			}

			Time latest_start(const Job<Time>& j, Time t, const State& s)
			{
				DM("IIP CW for " << j << ": ");
				auto ijs = influencing_jobs(j, t, s);
				Time latest = Time_model::constants<Time>::infinity();
				// travers from job with latest to job with earliest deadline
				for (auto it  = ijs.rbegin(); it != ijs.rend(); it++)
					latest = std::min(latest, (*it)->get_deadline())
					         - (*it)->maximal_cost();
				DM("latest=" << latest << " " << std::endl);
				return latest - j.maximal_cost();
			}

			private:

			const Space &space;
			const Time max_cost;
			const std::size_t n_tasks;

			static Time maximal_cost(const Jobs &jobs)
			{
				Time cmax = 0;
				for (auto j : jobs)
					cmax = std::max(cmax, j.maximal_cost());
				return cmax;
			}

			static std::size_t count_tasks(const Jobs &jobs)
			{
				std::unordered_set<unsigned long> all_tasks;
				for (const Job<Time>& j : jobs)
						all_tasks.insert(j.get_task_id());
				return all_tasks.size();
			}

			std::vector<const Job<Time>*> influencing_jobs(
				const Job<Time>& j_i,
				Time at,
				const State& s)
			{
				// influencing jobs
				std::unordered_map<unsigned long, const Job<Time>*> ijs;

				// first, check everything that's already pending at time t
				// is accounted for
				for (auto it = space.jobs_by_earliest_arrival
				                    .lower_bound(s.earliest_job_release());
				     it != space.jobs_by_earliest_arrival.end()
				     && it->second->earliest_arrival() <= at;
				     it++) {
					const Job<Time>& j = *(it->second);
					auto tid = j.get_task_id();
					if (j_i.get_task_id() != tid
					    && space.incomplete(s, j)
					    && (ijs.find(tid) == ijs.end()
					        || ijs[tid]->earliest_arrival()
					           > j.earliest_arrival())) {
						ijs[tid] = &j;
					}
				}

				// How far do we need to look into future releases?
				Time latest_deadline = 0;
				for (auto ij : ijs)
					latest_deadline = std::max(latest_deadline,
					                           ij.second->get_deadline());

				// second, go looking for later releases, if we are still
				// missing tasks

				for (auto it = space.jobs_by_earliest_arrival.upper_bound(at);
				     ijs.size() < n_tasks - 1
					   && it != space.jobs_by_earliest_arrival.end();
					 it++) {
					const Job<Time>& j = *it->second;
					auto tid = j.get_task_id();

					// future jobs should still be pending...
					assert(space.incomplete(s, j));

					if (ijs.find(tid) == ijs.end()) {
						ijs[tid] = &j;
					 	latest_deadline = std::max(latest_deadline,
					 	                           j.get_deadline());
					}

					// can we stop searching already?
					if (latest_deadline + max_cost < it->first) {
						// we have reached the horizon --- whatever comes now
						// cannot influence the latest start time anymore
					 	break;
					}
				}

				std::vector<const Job<Time>*> i_jobs_by_release_time;

				for (auto ij : ijs)
					i_jobs_by_release_time.push_back(ij.second);

				auto by_deadline = [](const Job<Time>* a, const Job<Time>* b)
				{
					return a->get_deadline() < b->get_deadline();
				};

				std::sort(i_jobs_by_release_time.begin(),
				          i_jobs_by_release_time.end(),
				          by_deadline);

				return i_jobs_by_release_time;
			}

		};

	}
}

#endif
