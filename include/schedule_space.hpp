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
#include "clock.hpp"

// DM : debug message -- disable for now
// #define DM(x) std::cerr << x
#define DM(x)

#include "schedule_state.hpp"

// #define CONFIG_COLLECT_SCHEDULE_GRAPH

namespace NP {

	namespace Uniproc {

		template<class Time> class Null_IIP;

		template<class Time, class IIP = Null_IIP<Time>> class State_space
		{
			public:

			typedef typename Job<Time>::Job_set Workload;
			typedef Schedule_state<Time> State;

			static State_space explore_naively(
					const Workload& jobs,
					double timeout = 0,
					std::size_t num_buckets = 1000)
			{
				auto s = State_space(jobs, timeout, num_buckets);
				s.cpu_time.start();
				s.explore_naively();
				s.cpu_time.stop();
				return s;
			}

			static State_space explore(
					const Workload& jobs,
					double timeout = 0,
					std::size_t num_buckets = 1000)
			{
				auto s = State_space(jobs, timeout, num_buckets);
				s.cpu_time.start();
				s.explore();
				s.cpu_time.stop();
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

			bool was_timed_out() const
			{
				return timed_out;
			}

			unsigned long number_of_states() const
			{
				return num_states;
			}

			unsigned long number_of_edges() const
			{
				return num_edges;
			}

			unsigned long max_exploration_front_width() const
			{
				return width;
			}

			double get_cpu_time() const
			{
				return cpu_time;
			}

#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH

			struct Edge {
				const Job<Time>* scheduled;
				const State* source;
				const State* target;
				const Interval<Time> finish_range;

				Edge(const Job<Time>* s, const State* src, const State* tgt,
				     const Interval<Time>& fr)
				: scheduled(s)
				, source(src)
				, target(tgt)
				, finish_range(fr)
				{
				}

				bool deadline_miss_possible() const
				{
					return scheduled->exceeds_deadline(finish_range.upto());
				}

				Time earliest_finish_time() const
				{
					return finish_range.from();
				}

				Time latest_finish_time() const
				{
					return finish_range.upto();
				}

				Time earliest_start_time() const
				{
					return finish_range.from() - scheduled->least_cost();
				}

				Time latest_start_time() const
				{
					return finish_range.upto() - scheduled->maximal_cost();
				}

			};

			const std::deque<Edge>& get_edges() const
			{
				return edges;
			}

			const std::deque<State>& get_states() const
			{
				return states;
			}

#endif
			private:

			typedef Job_set Scheduled;

			typedef std::deque<State> States;
			typedef typename std::deque<State>::iterator State_ref;
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
			bool timed_out;

			const Workload& jobs;

			Jobs_lut jobs_by_win;

			By_time_map jobs_by_latest_arrival;
			By_time_map jobs_by_earliest_arrival;
			By_time_map jobs_by_deadline;

			States states;
			unsigned long num_states, num_edges, width;
			States_map states_by_key;

			static const std::size_t num_todo_queues = 3;

			Todo_queue todo[num_todo_queues];
			int todo_idx;
			unsigned long current_job_count;

			Processor_clock cpu_time;
			double timeout;

			State_space(const Workload& jobs,
						double max_cpu_time = 0,
			            std::size_t num_buckets = 1000)
			: jobs_by_win(Interval<Time>{0, max_deadline(jobs)},
			              max_deadline(jobs) / num_buckets)
			, jobs(jobs)
			, aborted(false)
			, timed_out(false)
			, timeout(max_cpu_time)
			, iip(*this, jobs)
			, num_states(0)
			, num_edges(0)
			, width(0)
			, todo_idx(0)
			, current_job_count(0)
			{
				for (const Job<Time>& j : jobs) {
					jobs_by_latest_arrival.insert({j.latest_arrival(), &j});
					jobs_by_earliest_arrival.insert({j.earliest_arrival(), &j});
					jobs_by_deadline.insert({j.get_deadline(), &j});
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

			void check_for_deadline_misses(const State& s, const State& new_s)
			{
				// check if we skipped any jobs that are now guaranteed
				// to miss their deadline
				for (auto it = jobs_by_deadline
				               .lower_bound(s.earliest_finish_time());
				     it != jobs_by_deadline.end(); it++) {
					const Job<Time>& j = *(it->second);
					if (j.get_deadline() <= new_s.earliest_finish_time()) {
						if (incomplete(new_s, j)) {
							// This job is still incomplete but has no chance
							// of being scheduled before its deadline anymore.
							// Abort.
							aborted = true;
							// create a dummy state for explanation purposes
							auto inf = Time_model::constants<Time>::infinity();
							const State& next = new_state(new_s, j, index_of(j),
							                              inf, inf);
							// update response times
							update_finish_times(j, next.finish_range());
#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
							edges.emplace_back(&j, &new_s, &next,
							                   next.finish_range());
#endif
							num_edges++;
							break;
						}
					} else
						// deadlines now after the next earliest finish time
						break;
				}
			}

			// find next time by which a job is certainly released
			Time next_certain_job_release(const State& s)
			{
				const Scheduled &already_scheduled = s.get_scheduled_jobs();

				for (auto it = jobs_by_latest_arrival
				               .lower_bound(s.earliest_finish_time());
				     it != jobs_by_latest_arrival.end(); it++) {
					const Job<Time>& j = *(it->second);

					DM("ncjr:: "  << j << std::endl);

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
					    !priority_eligible(s, j, j.latest_arrival()))
						continue;

					// great, this job fits the bill
					return j.latest_arrival();
				}

				return Time_model::constants<Time>::infinity();
			}

			// find next time by which a job of higher priority
			// is certainly released on or after a given point in time
			Time next_certain_higher_priority_job_release(
				const State& s,
				const Job<Time>& reference_job)
			{

				for (auto it = jobs_by_latest_arrival
				               .lower_bound(s.earliest_finish_time());
				     it != jobs_by_latest_arrival.end(); it++) {
					const Job<Time>& j = *(it->second);

					// not relevant if already scheduled
					if (!incomplete(s, j))
						continue;

					// irrelevant if not of sufficient priority
					if (!j.higher_priority_than(reference_job))
						continue;

					// great, this job fits the bill
					return j.latest_arrival();
				}

				return Time_model::constants<Time>::infinity();
			}

			// returns true if there is certainly some pending job of higher
			// priority in this state
			bool exists_certainly_released_higher_prio_job(
				const State& s,
				const Job<Time>& reference_job)
			{
				auto ts_min = s.earliest_finish_time();

				DM("~~@~~ " << ts_min << ": " << reference_job << std::endl);
				for (const Job<Time>& j : jobs_by_win.lookup(ts_min)) {
					DM(":?: " << j << std::endl);
					if (j.latest_arrival() <= ts_min
						&& &j != &reference_job
					    && incomplete(s, j)
					    && j.higher_priority_than(reference_job)) {
						DM("\t\t\t\t " << j << " <<HP<< " << reference_job << std::endl);
						return true;
					}
				}
				return false;
			}

			// returns true if there is certainly some pending job of higher
			// priority at the given time ready to be scheduled
			bool exists_certainly_released_higher_prio_job(
				const State& s,
				const Job<Time>& reference_job,
				Time at)
			{
				auto ts_min = s.earliest_finish_time();
				DM("~~~ " << at << ": " << reference_job << std::endl);
				assert(at >= ts_min);

				// first check anything already pending at ts_min
				if (exists_certainly_released_higher_prio_job(s, reference_job))
					return true;

				// if not, check later releases
				for (auto it = jobs_by_latest_arrival.lower_bound(ts_min);
				     it != jobs_by_latest_arrival.end(); it++) {
					const Job<Time>& j = *(it->second);

					DM(":?: " << j << std::endl);

					// skip reference job
					if (&j == &reference_job)
						continue;

					if (j.latest_arrival() > at) {
						// ran out of jobs certainly released prior to 'at'
						return false;
					} else if (incomplete(s, j)
					           && j.higher_priority_than(reference_job)) {
					    DM("\t\t\t\t " << j << " <<HP<< " << reference_job << std::endl);
						return true;
					}
				}
				return false;
			}

			bool _iip_eligible(const State &s, const Job<Time> &j, Time t)
			{
				return t <= iip.latest_start(j, t, s.get_scheduled_jobs());
			}

			bool iip_eligible(const State &s, const Job<Time> &j, Time t)
			{
				return !iip.can_block || _iip_eligible(s, j, t);
			}

			bool priority_eligible(const State &s, const Job<Time> &j, Time t)
			{
				return !exists_certainly_released_higher_prio_job(s, j, t);
			}

			// returns true if there is certainly some pending job in this state
			bool exists_certainly_pending_job(const State &s)
			{
				auto ts_min = s.earliest_finish_time();

				for (const Job<Time>& j : jobs_by_win.lookup(ts_min))
					if (j.latest_arrival() <= ts_min
					    && incomplete(s, j)
				        && (!iip.can_block || priority_eligible(s, j, ts_min))
					    && iip_eligible(s, j, ts_min)) {
					    DM("\t\t\t\tcertainly released by " << ts_min << ":" << j << std::endl);
						return true;
					}
				return false;
			}

			bool potentially_next(const State &s, const Job<Time> &j)
			{
				auto t_latest = s.latest_finish_time();

				// if t_latest >=  j.earliest_arrival(), then the
				// job is trivially potentially next, so check the other case.

				if (t_latest < j.earliest_arrival()) {
					if (exists_certainly_pending_job(s))
						return false;

					Time r = next_certain_job_release(s);
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
				if (!priority_eligible(s, j, t_s)) {
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

				auto njobs = s_ref->get_scheduled_jobs().size();
				assert (
					(!njobs && num_states == 0) // initial state
				    || (njobs == current_job_count + 1) // normal State
				    || (njobs == current_job_count + 2 && aborted) // deadline miss
				);
				auto idx = njobs % num_todo_queues;
				todo[idx].push_back(s_ref);
				states_by_key.insert(std::make_pair(s_ref->get_key(), s_ref));
				num_states++;
				width = std::max(width, (unsigned long) todo[idx].size() - 1);
				return *s_ref;
			}

			const State& next_state()
			{
				if (todo[todo_idx].empty()) {
					current_job_count++;
					todo_idx = current_job_count % num_todo_queues;
				}
				auto s = todo[todo_idx].front();
				return *s;
			}

			bool in_todo(State_ref s)
			{
				for (auto it : todo)
					if (it == s)
						return true;
				return false;
			}

			void check_cpu_timeout()
			{
				if (timeout && get_cpu_time() > timeout) {
					aborted = true;
					timed_out = true;
				}
			}

			void done_with_current_state()
			{
				State_ref s = todo[todo_idx].front();
				// remove from TODO list
				todo[todo_idx].pop_front();

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

				// delete from master sequence to free up memory
				assert(states.begin() == s);
				states.pop_front();
#endif
			}

			bool not_done()
			{
				return current_job_count < jobs.size() || !todo[todo_idx].empty();
			}

			// naive: no state merging
			void schedule_naively(const State &s, const Job<Time> &j)
			{
				auto t_s = s.next_earliest_start_time(j);

				Time other_certain_start =
					next_certain_higher_priority_job_release(s, j);
				Time iip_latest_start =
					iip.latest_start(j, t_s, s.get_scheduled_jobs());

				DM("nest=" << s.next_earliest_start_time(j) << std::endl);
				DM("other_certain_start=" << other_certain_start << std::endl);
				// construct new state from s by scheduling j
				const State& next = new_state(s, j, index_of(j), other_certain_start, iip_latest_start);
				// update response times
				update_finish_times(j, next.finish_range());
				check_for_deadline_misses(s, next);
#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
				edges.emplace_back(&j, &s, &next, next.finish_range());
#endif
				num_edges++;
			}

			void explore_naively()
			{
				make_initial_state();

				while (not_done() && !aborted) {
					const State& s = next_state();

					DM("\nLooking at: S" << (todo.front() - states.begin() + 1) << " " << s << std::endl);

					// Identify relevant interval for next job
					// relevant job buckets
					auto ts_min = s.earliest_finish_time();
					auto latest_idle = next_certain_job_release(s);

					Interval<Time> next_range{ts_min,
					    std::max(latest_idle, s.latest_finish_time())};

					DM(" => " << next_range << std::endl);

					bool found_at_least_one = false;

					// (1) first check jobs that can be already pending
					for (const Job<Time>& j : jobs_by_win.lookup(ts_min)) {
						DM("    -?? " << j << std::endl);
						// if it is potentially active at ts_min...
						if (j.earliest_arrival() <= ts_min) {
							// if it can be scheduled next...
							if (is_eligible_successor(s, j)) {
								DM("        --> ok!"  << std::endl);
								// create the relevant state and continue
								schedule_naively(s, j);
								found_at_least_one = true;
							}
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
					    s.get_scheduled_jobs().size() != jobs.size()) {
						// out of options and we didn't schedule all jobs
						aborted = true;
					}

					done_with_current_state();
					check_cpu_timeout();
				}
			}


			void schedule(const State &s, const Job<Time> &j)
			{
				auto k = s.next_key(j);

				auto r = states_by_key.equal_range(k);

				if (r.first != r.second) {
					auto t_s = s.next_earliest_start_time(j);

					Time other_certain_start =
						next_certain_higher_priority_job_release(s, j);
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

						// cannot merge without loss of accuracy if the
						// intervals do not overlap
						if (!goal.finish_range()
						     .intersects(found.finish_range()))
							continue;

						// great, we found a match and can merge the states
						update_finish_times(j, goal.finish_range());
						found.update_finish_range(goal.finish_range());
#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
						edges.emplace_back(&j, &s, &(found),
						                   goal.finish_range());
#endif
						num_edges++;
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

					DM("\nLooking at: S" << (todo.front() - states.begin()) << " " << s << std::endl);

					// Identify relevant interval for next job
					// relevant job buckets
					auto ts_min = s.earliest_finish_time();
					auto latest_idle = next_certain_job_release(s);

					Interval<Time> next_range{ts_min,
					    std::max(latest_idle, s.latest_finish_time())};

					DM(" => " << next_range << std::endl);

					bool found_at_least_one = false;

					// (1) first check jobs that can be already pending
					for (const Job<Time>& j : jobs_by_win.lookup(ts_min)) {
						DM("    -?? " << j << std::endl);
						// if it is potentially active at ts_min...
						if (j.earliest_arrival() <= ts_min) {
							// if it can be scheduled next...
							if (is_eligible_successor(s, j)) {
								DM("        --> ok!"  << std::endl);
								// create the relevant state and continue
								schedule(s, j);
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
					    s.get_scheduled_jobs().size() != jobs.size()) {
						// out of options and we didn't schedule all jobs
						aborted = true;
						DM(":: Didn't find any possible successors. Aborting." << std::endl);
					}

					done_with_current_state();
					check_cpu_timeout();
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
						    << "\\nES=" << e.earliest_start_time()
 						    << "\\nLS=" << e.latest_start_time()
						    << "\\nEF=" << e.earliest_finish_time()
						    << "\\nLF=" << e.latest_finish_time()
						    << "\"";
						if (e.deadline_miss_possible()) {
							out << ",color=Red,fontcolor=Red";
						}
						out << ",fontsize=8" << "]"
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
