#ifndef SCHEDULE_SPACE_H
#define SCHEDULE_SPACE_H

#include <unordered_set>
#include <unordered_map>
#include <map>
#include <vector>
#include <deque>
#include <list>
#include <algorithm>

#include <iostream>
#include <ostream>
#include <cassert>

#include "jobs.hpp"

// DM : debug message -- disable for now
// #define DM(x) std::cerr << x
#define DM(x)

#include "schedule_state.hpp"


#define CONFIG_COLLECT_SCHEDULE_GRAPH

namespace NP {

	namespace Uniproc {

		template<class Time> class Null_IIP;

		template<class Time, class IIP = Null_IIP<Time>> class State_space
		{
			public:

			typedef std::vector<Job<Time>> Workload;
			typedef Schedule_state<Time> State;

			static State_space explore_naively(
					const Workload& jobs,
					std::size_t num_buckets = 1000)
			{
				auto s = State_space(jobs, num_buckets);
				s.explore_naively();
				return s;
			}

			static State_space explore(
					const Workload& jobs,
					std::size_t num_buckets = 1000)
			{
				auto s = State_space(jobs, num_buckets);
				s.explore();
				return s;
			}

			Interval<Time> get_finish_times(const Job<Time>& j) const
			{
				auto rbounds = rta.find(&j);
				if (rbounds == rta.end()) {
					return Interval<Time>{0, Time_model::constants<Time>::infinity()};
				} else {
					return rbounds->second;
				}
			}

			bool is_schedulable() const
			{
				return !aborted;
			}

			unsigned long number_of_states() const
			{
				return num_states;
			}

#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH

			struct Edge {
				const Job<Time>* scheduled;
				const State* source;
				const State* target;
				const Time latest_finish_time;

				Edge(const Job<Time>* s, const State* src, const State* tgt,
				     Time lft)
				: scheduled(s)
				, source(src)
				, target(tgt)
				, latest_finish_time(lft)
				{
				}

				bool deadline_miss_possible() const
				{
					return scheduled->exceeds_deadline(latest_finish_time);
				}
			};

			const std::deque<Edge>& get_edges() const
			{
				return edges;
			}

			const std::list<State>& get_states() const
			{
				return states;
			}

#endif
			private:

			typedef Job_set<Time> Scheduled;

			typedef std::list<State> States;
			typedef typename std::list<State>::iterator State_ref;
			typedef std::unordered_multimap<hash_value_t, State_ref> States_map;

			typedef const Job<Time>* Job_ref;
			typedef std::multimap<Time, Job_ref> By_time_map;

			typedef std::deque<State_ref> Todo_queue;

			typedef Interval_lookup_table<Time, Job<Time>, Job<Time>::scheduling_window> Jobs_lut;

			typedef std::unordered_map<const Job<Time>*, Interval<Time> > Response_times;

#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
			std::deque<Edge> edges;
#endif

			IIP iip;
			friend IIP;

			Response_times rta;
			bool aborted;

			const Workload& jobs;

			Jobs_lut jobs_by_win;

			By_time_map jobs_by_latest_arrival;
			By_time_map jobs_by_earliest_arrival;

			States states;
			unsigned long num_states;
			States_map states_by_key;
			Todo_queue todo;

			State_space(const Workload& jobs,
			            std::size_t num_buckets = 1000)
			: jobs_by_win(Interval<Time>{0, max_deadline(jobs)},
			              max_deadline(jobs) / num_buckets)
			, jobs(jobs)
			, aborted(false)
			, iip(*this, jobs)
			, num_states(0)
			{
				for (const Job<Time>& j : jobs) {
					jobs_by_latest_arrival.insert({j.latest_arrival(), &j});
					jobs_by_earliest_arrival.insert({j.earliest_arrival(), &j});
					jobs_by_win.insert(j);
				}
			}


			private:

			static Time max_deadline(const Workload &jobs)
			{
				Time dl = 0;
				for (auto j : jobs)
					dl = std::max(dl, j.get_deadline());
				return dl;
			}

			void update_finish_times(const Job<Time>& j, Interval<Time> range)
			{
				auto rbounds = rta.find(&j);
				if (rbounds == rta.end()) {
					rta.emplace(&j, range);
					if (j.exceeds_deadline(range.upto()))
						aborted = true;
				} else {
					rbounds->second.widen(range);
					if (j.exceeds_deadline(rbounds->second.upto()))
						aborted = true;
				}
				DM("RTA " << j.get_id() << ": " << rta.find(&j)->second << std::endl);
			}

			std::size_t index_of(const Job<Time>& j) const
			{
				return (std::size_t) (&j - &(jobs[0]));
			}

			bool incomplete(const Scheduled &scheduled, const Job<Time>& j) const
			{
				return !scheduled.contains(index_of(j));
			}

			bool incomplete(const State& s, const Job<Time>& j) const
			{
				return incomplete(s.get_scheduled_jobs(), j);
			}

			// find next time by which a job is certainly released on
			// or after a given point in time
			Time next_certain_job_release(const State& s, Time on_or_after)
			{
				const Scheduled &already_scheduled = s.get_scheduled_jobs();

				for (auto it = jobs_by_latest_arrival.lower_bound(on_or_after);
				     it != jobs_by_latest_arrival.end(); it++) {
					const Job<Time>& j = *(it->second);

					// not relevant if already scheduled
					if (!incomplete(already_scheduled, j))
						continue;

					// If the job is not IIP-eligible when it is certainly
					// released, then there exists a schedule where it doesn't
					// count, so skip it.
					if (!iip_eligible(s, j, j.latest_arrival()))
						continue;

					// It must be priority-eligible when released, too.
					// Relevant only if we have an IIP, otherwise the job is
					// trivially priority-eligible.
					if (iip.can_block &&
					    !priority_eligible(j.latest_arrival(), already_scheduled, j))
						continue;

					// great, this job fits the bill
					return j.latest_arrival();
				}

				return Time_model::constants<Time>::infinity();
			}

			// find next time by which a job of higher priority
			// is certainly released on or after a given point in time
			Time next_certain_higher_priority_job_release(
				Time on_or_after,
				const Scheduled& already_scheduled,
				const Job<Time>& reference_job)
			{
				for (auto it = jobs_by_latest_arrival.lower_bound(on_or_after);
				     it != jobs_by_latest_arrival.end(); it++) {
					const Job<Time>& j = *(it->second);

					// not relevant if already scheduled
					if (!incomplete(already_scheduled, j))
						continue;

					// irrelevant if not of sufficient priority
					if (!j.higher_priority_than(reference_job))
						continue;

					// great, this job fits the bill
					return j.latest_arrival();
				}

				return Time_model::constants<Time>::infinity();
			}


			// returns true if there is certainly some pending job at the given
			// time ready to be scheduled
			bool exists_certainly_released_job(const State &s, Time at)
			{
				for (const Job<Time>& j : jobs_by_win.lookup(at))
					if (j.latest_arrival() <= at
					    && incomplete(s, j)
				        && (!iip.can_block || priority_eligible(at, s.get_scheduled_jobs(), j))
					    && iip_eligible(s, j, at)) {
					    DM("\t\t\t\tcertainly released at " << at << ":" << j << std::endl);
						return true;
					}
				return false;
			}

			// returns true if there is certainly some pending job of higher
			// priority at the given time ready to be scheduled
			bool exists_certainly_released_higher_prio_job(
				Time at,
				const Scheduled& already_scheduled,
				const Job<Time>& reference_job)
			{
				for (const Job<Time>& j : jobs_by_win.lookup(at))
					if (j.latest_arrival() <= at &&
					    &j != &reference_job &&
					    incomplete(already_scheduled, j) &&
					    j.higher_priority_than(reference_job)) {
					    DM("\t\t\t\t " << j << " <<HP<< " << reference_job);
						return true;
					}
				return false;
			}

			bool iip_eligible(const State &s, const Job<Time> &j, Time t)
			{
				return !iip.can_block
				       || t <= iip.latest_start(j, t, s.get_scheduled_jobs());
			}

			bool priority_eligible(Time t_s, const Scheduled& already_scheduled,
			                       const Job<Time> &j)
			{
				return !exists_certainly_released_higher_prio_job(
				            t_s, already_scheduled, j);
			}

			bool priority_eligible(Time t_s, const State &s, const Job<Time> &j)
			{
				return priority_eligible(t_s, s.get_scheduled_jobs(), j);
			}

			bool potentially_next(const State &s, const Job<Time> &j)
			{
				auto t_latest = s.latest_finish_time();
				if (t_latest < j.earliest_arrival()) {
					// any certainly pending at t_latest
					if (exists_certainly_released_job(s, t_latest))
						// if something is certainly pending at t_latest and
						// IIP-eligible, then j can't be next
						return false;

					// any certainly pending since then
					Time r = next_certain_job_release(s, t_latest);
					// if something else is certainly released before j and IIP-
					// eligible at the time of certain release, then j can't
					// possibly be next
					if (r < j.earliest_arrival())
						return false;
				}
				return true;
			}

			bool is_eligible_successor(const State &s, const Job<Time> &j)
			{
				if (!incomplete(s, j)) {
					DM("        * not incomplete" <<  std::endl);
					return false;
				}
				auto t_s = s.next_earliest_start_time(j);
				if (!priority_eligible(t_s, s, j)) {
					DM("        * not priority eligible" <<  std::endl);
					return false;
				}
				if (!potentially_next(s, j)) {
					DM("        * not potentially next" <<  std::endl);
					return false;
				}
				if (!iip_eligible(s, j, t_s)) {
					DM("        * not IIP eligible" << std::endl);
					return false;
				}
				return true;
			}

			void make_initial_state()
			{
				// construct initial state
				new_state();
			}

			template <typename... Args>
			State& new_state(Args&&... args)
			{
				states.emplace_back(std::forward<Args>(args)...);
				State_ref s_ref = --states.end();
				todo.push_back(s_ref);
				states_by_key.insert(std::make_pair(s_ref->get_key(), s_ref));
				num_states++;
				return *s_ref;
			}

			const State& next_state()
			{
				auto s = todo.front();
				return *s;
			}

			bool in_todo(State_ref s)
			{
				for (auto it : todo)
					if (it == s)
						return true;
				return false;
			}

			void done_with_current_state()
			{
				State_ref s = todo.front();
				// remove from TODO list
				todo.pop_front();

#ifndef CONFIG_COLLECT_SCHEDULE_GRAPH
				// If we don't need to collect all states, we can remove
				// all those that we are done with, which saves a lot of
				// memory.

				// remove from lookup map
				auto matches = states_by_key.equal_range(s->get_key());
				bool deleted = false;
				for (auto it = matches.first; it != matches.second; it++)
					if (it->second == s) {
						states_by_key.erase(it);
						deleted = true;
						break;
					}
				assert(deleted);

				// delete from master list to free up memory
				states.erase(s);
#endif
			}

			bool not_done()
			{
				return !todo.empty();
			}

			// naive: no state merging
			void schedule_naively(const State &s, const Job<Time> &j)
			{
				auto t_s = s.next_earliest_start_time(j);

				Time other_certain_start =
					next_certain_higher_priority_job_release(
						t_s, s.get_scheduled_jobs(), j);
				Time iip_latest_start =
					iip.latest_start(j, t_s, s.get_scheduled_jobs());

				DM("nest=" << s.next_earliest_start_time(j) << std::endl);
				DM("other_certain_start=" << other_certain_start << std::endl);
				// construct new state from s by scheduling j
				const State& next = new_state(s, j, index_of(j), other_certain_start, iip_latest_start);
				// update response times
				update_finish_times(j, next.finish_range());
#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
				edges.emplace_back(&j, &s, &next, next.latest_finish_time());
#endif
			}

			void explore_naively()
			{
				make_initial_state();

				while (not_done() && !aborted) {
					const State& s = next_state();

					DM("\nLooking at: " << s << std::endl);

					// Identify relevant interval for next job
					// relevant job buckets
					auto ts_min = s.earliest_finish_time();
					auto latest_idle = next_certain_job_release(s, ts_min);

					Interval<Time> next_range{ts_min,
					    std::max(latest_idle, s.latest_finish_time())};

					DM(" => " << next_range << std::endl);

					bool found_at_least_one = false;

					// (1) first check jobs that can be already pending
					for (const Job<Time>& j : jobs_by_win.lookup(ts_min)) {
						DM("    -?? " << j << std::endl);
						// if it is potentially active in the range of
						// interest...
						if (j.scheduling_window().intersects(next_range))
							// if it can be scheduled next...
							if (is_eligible_successor(s, j)) {
								DM("        --> ok!"  << std::endl);
								// create the relevant state and continue
								schedule_naively(s, j);
								found_at_least_one = true;
							}
					}
					// (2) check jobs that are released later in the interval
					for (auto it = jobs_by_earliest_arrival.upper_bound(ts_min);
					     it != jobs_by_earliest_arrival.end(); it++) {
						const Job<Time>& j = *it->second;
						// stop looking once we've left the range of interest
						DM("    -?? " << j << std::endl);
						if (!j.scheduling_window().intersects(next_range))
							break;
						// if it can be scheduled next...
						if (is_eligible_successor(s, j)) {
							DM("        --> OK!"  << std::endl);
							// create the relevant state and continue
							schedule_naively(s, j);
							found_at_least_one = true;
						}
					}

					// check for a dead end
					if (!found_at_least_one &&
					    s.get_scheduled_jobs().number_of_jobs() != jobs.size()) {
						// out of options and we didn't schedule all jobs
						aborted = true;
					}

					done_with_current_state();
				}
			}


			void schedule(const State &s, const Job<Time> &j)
			{
				auto k = s.next_key(j);

				auto r = states_by_key.equal_range(k);

				if (r.first != r.second) {
					auto t_s = s.next_earliest_start_time(j);

					Time other_certain_start =
						next_certain_higher_priority_job_release(
							t_s, s.get_scheduled_jobs(), j);
					Time iip_latest_start =
						iip.latest_start(j, t_s, s.get_scheduled_jobs());

					State goal{s, j, index_of(j), other_certain_start,
					           iip_latest_start};

					for (auto it = r.first; it != r.second; it++) {
						State &found = *it->second;

						// key collision if the job sets don't match exactly
						if (found.get_scheduled_jobs()
						    != goal.get_scheduled_jobs())
							continue;

						// cannot merge if without loss of accuracy if the
						// intervals do not overlap
						if (!goal.finish_range()
						     .intersects(found.finish_range()))
							continue;

						// great, we found a match and can merge the states
						update_finish_times(j, goal.finish_range());
						found.update_finish_range(goal.finish_range());
#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
						edges.emplace_back(&j, &s, &(found),
						                   goal.finish_range().upto());
#endif
						return;
					}
				}

				// If we reach here, we didn't find a match and need to create
				// a new state.
				schedule_naively(s, j);
			}

			void explore()
			{
				make_initial_state();

				while (not_done() && !aborted) {
					const State& s = next_state();

					DM("\nLooking at: " << s << std::endl);

					// Identify relevant interval for next job
					// relevant job buckets
					auto ts_min = s.earliest_finish_time();
					auto latest_idle = next_certain_job_release(s, ts_min);

					Interval<Time> next_range{ts_min,
					    std::max(latest_idle, s.latest_finish_time())};

					DM(" => " << next_range << std::endl);

					bool found_at_least_one = false;

					// (1) first check jobs that can be already pending
					for (const Job<Time>& j : jobs_by_win.lookup(ts_min)) {
						DM("    -?? " << j << std::endl);
						// if it is potentially active in the range of
						// interest...
						if (j.scheduling_window().intersects(next_range)) {
							// if it can be scheduled next...
							if (is_eligible_successor(s, j)) {
								DM("        --> ok!"  << std::endl);
								// create the relevant state and continue
								schedule(s, j);
								//new_state(schedule_naively(s, j));
								found_at_least_one = true;
							}
						} else {
							DM("        * not pending" << std::endl);
						}
					}
					// (2) check jobs that are released later in the interval
					for (auto it = jobs_by_earliest_arrival.upper_bound(ts_min);
					     it != jobs_by_earliest_arrival.end(); it++) {
						const Job<Time>& j = *it->second;
						// stop looking once we've left the range of interest
						DM("    -?? " << j << std::endl);
						if (!j.scheduling_window().intersects(next_range)) {
							DM("        * not pending" << std::endl);
							break;
						}
						// if it can be scheduled next...
						if (is_eligible_successor(s, j)) {
							DM("        --> OK!"  << std::endl);
							// connect the relevant state and continue
							schedule(s, j);
							//new_state(schedule_naively(s, j));
							found_at_least_one = true;
						}
					}

					DM("Done." << std::endl);

					// check for a dead end
					if (!found_at_least_one &&
					    s.get_scheduled_jobs().number_of_jobs() != jobs.size()) {
						// out of options and we didn't schedule all jobs
						aborted = true;
						DM(":: Didn't find any possible successors. Aborting." << std::endl);
					}

					done_with_current_state();
				}
			}

#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
			friend std::ostream& operator<< (std::ostream& out,
			                                 const State_space<Time, IIP>& space)
			{
					std::map<const Schedule_state<Time>*, unsigned int> state_id;
					unsigned int i = 0;
					out << "digraph {" << std::endl;
					for (const Schedule_state<Time>& s : space.get_states()) {
						state_id[&s] = i++;
						out << "\tS" << state_id[&s]
							<< "[label=\"S" << state_id[&s] << ": ["
							<< s.earliest_finish_time()
							<< ", "
							<< s.latest_finish_time()
							<< "]\"];"
							<< std::endl;
					}
					for (auto e : space.get_edges()) {
						out << "\tS" << state_id[e.source]
						    << " -> "
						    << "S" << state_id[e.target]
						    << "[label=\""
						    << "T" << e.scheduled->get_task_id()
						    << " J" << e.scheduled->get_id()
						    << "\\nDL=" << e.scheduled->get_deadline()
						    << "\\nLFT=" << e.latest_finish_time
						    << "\"";
						if (e.deadline_miss_possible()) {
							out << ",color=Red,fontcolor=Red";
						}
						out << "]"
						    << ";"
						    << std::endl;
						if (e.deadline_miss_possible()) {
							out << "S" << state_id[e.target]
								<< "[color=Red];"
								<< std::endl;
						}
					}
					out << "}" << std::endl;
				return out;
			}
#endif
		};

	}
}

#include "iip.hpp"

namespace std
{
	template<class Time> struct hash<NP::Uniproc::Schedule_state<Time>>
    {
		std::size_t operator()(NP::Uniproc::Schedule_state<Time> const& s) const
        {
            return s.get_key();
        }
    };
}


#endif
