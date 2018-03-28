#ifndef GLOBAL_SPACE_H
#define GLOBAL_SPACE_H

#include <unordered_map>
#include <map>
#include <vector>
#include <deque>
#include <forward_list>
#include <algorithm>

#include <iostream>
#include <ostream>
#include <cassert>

#include "config.h"
#include "jobs.hpp"
#include "precedence.hpp"
#include "clock.hpp"

#include "global/state.hpp"

namespace NP {

	namespace Global {

		template<class Time> class State_space
		{
			public:

			typedef typename Job<Time>::Job_set Workload;
			typedef Schedule_state<Time> State;

			static State_space explore_naively(
					const Workload& jobs,
					double timeout = 0,
					std::size_t num_buckets = 1000,
					const Precedence_constraints& dag = Precedence_constraints(),
					unsigned int num_processors = 1,
					unsigned int max_depth = 0)
			{
				// does currently not support precedence constraints
				assert(dag.empty());

				auto s = State_space(jobs, num_processors, timeout, max_depth,
				                     num_buckets);
				s.cpu_time.start();
				s.explore_naively();
				s.cpu_time.stop();
				return s;
			}

			static State_space explore(
					const Workload& jobs,
					double timeout = 0,
					std::size_t num_buckets = 1000,
					const Precedence_constraints& dag = Precedence_constraints(),
					unsigned int num_processors = 1,
					unsigned int max_depth = 0)
			{
				// does currently not support precedence constraints
				assert(dag.empty());

				auto s = State_space(jobs, num_processors, timeout, max_depth,
				                     num_buckets);
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

			typedef std::deque<State> States;

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

			const std::deque<States>& get_states() const
			{
				return states_storage;
			}

#endif
			private:

			typedef Job_set Scheduled;

			typedef typename std::deque<State>::iterator State_ref;
			typedef typename std::forward_list<State_ref> State_refs;
			typedef std::unordered_map<hash_value_t, State_refs> States_map;

			typedef const Job<Time>* Job_ref;
			typedef std::multimap<Time, Job_ref> By_time_map;

			typedef std::deque<State_ref> Todo_queue;

			typedef Interval_lookup_table<Time, Job<Time>, Job<Time>::scheduling_window> Jobs_lut;

			typedef std::unordered_map<const Job<Time>*, Interval<Time> > Response_times;


#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
			std::deque<Edge> edges;
#endif

			Response_times rta;
			bool aborted;
			bool timed_out;

			unsigned int max_depth;

			bool be_naive;

			const Workload& jobs;

			Jobs_lut jobs_by_win;

			std::deque<States> states_storage;

			By_time_map jobs_by_latest_arrival;
			By_time_map jobs_by_earliest_arrival;
			By_time_map jobs_by_deadline;

			unsigned long num_states, num_edges, width;
			States_map states_by_key;

			unsigned long current_job_count;

			Processor_clock cpu_time;
			double timeout;

			unsigned int num_cpus;

			State_space(const Workload& jobs,
			            unsigned int num_cpus,
			            double max_cpu_time = 0,
			            unsigned int max_depth = 0,
			            std::size_t num_buckets = 1000)
			: jobs_by_win(Interval<Time>{0, max_deadline(jobs)},
			              max_deadline(jobs) / num_buckets)
			, jobs(jobs)
			, aborted(false)
			, timed_out(false)
			, be_naive(false)
			, timeout(max_cpu_time)
			, max_depth(max_depth)
			, num_states(0)
			, num_edges(0)
			, width(0)
			, current_job_count(0)
			, num_cpus(num_cpus)
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
				for (const auto& j : jobs)
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

			void check_for_deadline_misses(Time t_earliest, const State& new_s)
			{
				auto horizon = new_s.get_running_jobs()
				               .possible_core_availability_time();

				// check if we skipped any jobs that are now guaranteed
				// to miss their deadline
				for (auto it = jobs_by_deadline
				               .lower_bound(t_earliest);
				     it != jobs_by_deadline.end(); it++) {
					const Job<Time>& j = *(it->second);
					if (j.get_deadline() < horizon) {
						if (incomplete(new_s, j)) {
							// This job is still incomplete but has no chance
							// of being scheduled before its deadline anymore.
							// Abort.
							aborted = true;
							// create a dummy state for explanation purposes
							auto inf = Time_model::constants<Time>::infinity();
							const State& next = new_state(new_s, j, index_of(j),
							                              0, inf);
							// update response times
							auto frange = next.get_running_jobs()[0]
							              .finish_range();
							update_finish_times(j, frange);
#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
							edges.emplace_back(&j, &new_s, &next, frange);
#endif
							num_edges++;
							break;
						}
					} else
						// deadlines now after the next earliest finish time
						break;
				}
			}

			void cache_state(State_ref s)
			{
				// make it available for fast lookup

				// create a new list if needed, or lookup if already existing
				auto res = states_by_key.emplace(
					std::make_pair(s->get_key(), State_refs()));

				auto pair_it = res.first;
				State_refs& list = pair_it->second;

				list.push_front(s);
			}

			void make_initial_state()
			{
				// construct initial state
				states_storage.emplace_back();
				new_state(num_cpus);
			}

			States& states()
			{
				return states_storage.back();
			}

			template <typename... Args>
			State_ref alloc_state(Args&&... args)
			{
				states().emplace_back(std::forward<Args>(args)...);
				return --states().end();
			}

			void dealloc_state(State_ref s)
			{
				assert(--states().end() == s);
				states().pop_back();
			}

			template <typename... Args>
			State_ref make_state(Args&&... args)
			{
				State_ref s_ref = alloc_state(std::forward<Args>(args)...);

				// make sure we didn't screw up...
				auto njobs = s_ref->get_scheduled_jobs().size();
				assert (
					(!njobs && num_states == 0) // initial state
				    || (njobs == current_job_count + 1) // normal State
				    || (njobs == current_job_count + 2 && aborted) // deadline miss
				);

				// Fast-forward this state to aid future merges.
				State& s = *s_ref;
				auto& rjs = s.get_running_jobs();
				auto t_ref = rjs.possible_core_availability_time();
				auto t_min = next_possible_job_release(*s_ref, t_ref);
				auto t_max = next_certain_job_release(*s_ref, t_ref);
				rjs.fast_forward(t_min, t_max);
				rjs.ensure_sorted_by_eft();

				return s_ref;
			}

			template <typename... Args>
			State& new_state(Args&&... args)
			{
				auto s_ref = make_state(std::forward<Args>(args)...);
				num_states++;

				return *s_ref;
			}

			template <typename... Args>
			State& new_or_merged_state(Args&&... args)
			{
				State_ref s_ref = make_state(std::forward<Args>(args)...);
				State_ref tmp = s_ref;

				// try to merge the new state into an existing state
				bool merged = try_to_merge(s_ref);
				if (merged) {
					// great, we merged!
					// clean up the just-created state that we no longer need
					dealloc_state(tmp);
				} else {
					// nope, need to explore this state
					cache_state(s_ref);
					num_states++;
				}
				return *s_ref;
			}

			bool try_to_merge(State_ref& s_ref)
			{
				State& s = *s_ref;

				const auto pair_it = states_by_key.find(s.get_key());

				// cannot merge if key doesn't exist
				if (pair_it == states_by_key.end())
					return false;

				for (State_ref other : pair_it->second) {
					State &found = *other;
					DM("$$$$ Attempting to merge " << s << " into "
					   << found << std::endl);
					if (try_to_merge_into(s, found)) {
						DM("$$$$ Merge attempt succeeded: " << found << std::endl);
						s_ref = other;
						return true;
					}
					DM("$$$$ Merge attempt failed." << std::endl);
				}
				return false;
			}

			void check_cpu_timeout()
			{
				if (timeout && get_cpu_time() > timeout) {
					aborted = true;
					timed_out = true;
				}
			}

			void check_depth_abort()
			{
				if (max_depth && current_job_count > max_depth)
					aborted = true;
			}

			bool merge_condition_met_at(
				const Time t,
				const Running_jobs<Time>& rjs1,
				const Running_jobs<Time>& rjs2,
				const Running_jobs<Time>& merged)
			{
				auto ivals_1 = rjs1.count_intervals_including(t);
				auto ivals_2 = rjs2.count_intervals_including(t);
				auto ivals_m = merged.count_intervals_including(t);
				DM("[merge_condition_met_at] time " << t << ":"
				   << " ivals_1=" << ivals_1
				   << " ivals_2=" << ivals_2
				   << " ivals_m=" << ivals_m
				   << std::endl);
				return ivals_1 == ivals_m || ivals_2 == ivals_m;
			}

			bool try_to_merge_into(const State& s1, State& s2)
			{
				// We need to check that there's no accidental hash key collision
				// (however unlikely).
				if (s1.get_scheduled_jobs() != s2.get_scheduled_jobs())
					return false;

				// Check intersections
				const auto& rjs1 = s1.get_running_jobs();
				const auto& rjs2 = s2.get_running_jobs();

				if (!rjs1.intervals_compatible(rjs2))
					return false;

				// Construct the merged intervals
				Running_jobs<Time> merged(rjs1, rjs2);

				// check all relevant times
				for (const auto& rj : rjs1) {
					auto eft = rj.earliest_finish_time();
					auto lft = rj.latest_finish_time();
					if (!merge_condition_met_at(eft, rjs1, rjs2, merged))
						return false;
					if (!merge_condition_met_at(lft, rjs1, rjs2, merged))
						return false;
				}

				for (const auto& rj : rjs2) {
					auto eft = rj.earliest_finish_time();
					auto lft = rj.latest_finish_time();
					if (!merge_condition_met_at(eft, rjs1, rjs2, merged))
						return false;
					if (!merge_condition_met_at(lft, rjs1, rjs2, merged))
						return false;
				}

				// ok, merge condition met, let's actually merge
				merged.ensure_sorted_by_eft();
				s2.get_running_jobs() = merged;
				return true;
			}

			// Find next time by which any job is possibly released.
			// Note that this time may be in the past.
			Time next_possible_job_release(const State& s, const Time t_earliest)
			{
				Time when = Time_model::constants<Time>::infinity();

				// check everything that overlaps with t_earliest
				for (const Job<Time>& j : jobs_by_win.lookup(t_earliest))
					if (incomplete(s, j))
						when = std::min(when, j.earliest_arrival());

				// No point looking in the future when we've already
				// found one in the present.
				if (when <= t_earliest)
					return when;

				// Ok, let's look also in the future.
				for (auto it = jobs_by_earliest_arrival
				               .lower_bound(t_earliest);
				     it != jobs_by_earliest_arrival.end(); it++) {
					const Job<Time>& j = *(it->second);

					// j is not relevant if it is already scheduled
					if (incomplete(s, j))
						// does it beat what we've already seen?
						return std::min(when, j.earliest_arrival());
				}

				return when;
			}

			// Find next time by which any job is certainly released.
			// Note that this time may be in the past.
			Time next_certain_job_release(const State& s, const Time t_earliest)
			{
				Time when = Time_model::constants<Time>::infinity();

				// check everything that overlaps with t_earliest
				for (const Job<Time>& j : jobs_by_win.lookup(t_earliest))
					if (incomplete(s, j))
						when = std::min(when, j.latest_arrival());

				// No point looking in the future when we've already
				// found one in the present.
				if (when <= t_earliest)
					return when;

				// Ok, let's look also in the future.
				for (auto it = jobs_by_latest_arrival
				               .lower_bound(t_earliest);
				     it != jobs_by_latest_arrival.end(); it++) {
					const Job<Time>& j = *(it->second);

					// j is not relevant if it is already scheduled
					if (incomplete(s, j))
						// does it beat what we've already seen?
						return std::min(when, j.latest_arrival());
				}

				return when;
			}

			// Find next time by which any job is certainly released.
			// Note that this time may be in the past.
			Time next_certain_higher_priority_job_release(
				const State& s,
				const Job<Time> &reference_job,
				const Time t_earliest)
			{
				Time when = Time_model::constants<Time>::infinity();

				// check everything that overlaps with t_earliest
				for (const Job<Time>& j : jobs_by_win.lookup(t_earliest))
					if (incomplete(s, j)
					    && j.higher_priority_than(reference_job))
						when = std::min(when, j.latest_arrival());

				// No point looking in the future when we've already
				// found one in the present.
				if (when <= t_earliest)
					return when;

				// Ok, let's look also in the future.
				for (auto it = jobs_by_latest_arrival
				               .lower_bound(t_earliest);
				     it != jobs_by_latest_arrival.end(); it++) {
					const Job<Time>& j = *(it->second);

					// j is not relevant if it is already scheduled
					if (incomplete(s, j)
					    && j.higher_priority_than(reference_job))
						// does it beat what we've already seen?
						return std::min(when, j.latest_arrival());
				}

				return when;
			}

			// naive: no state merging
			void explore_naively()
			{
				be_naive = true;
				explore();
			}

			// with state merging
			bool try_to_schedule(const State &s, const Job<Time> &j,
			                     Time t_earliest, Time t_horizon)
			{
				// latest time before a higher-priority job is released
				auto t_higher_prio =
					next_certain_higher_priority_job_release(s, j, t_earliest)
					- Time_model::constants<Time>::epsilon();

				// latest start time
				auto lst = std::min(t_higher_prio, t_horizon);

				// check if this job has even a chance to go somewhere
				if (lst < t_earliest || lst < j.earliest_arrival())
					return false;

				bool found_one = false;
				bool found_idle = false;
				// now let's see if this job goes anywhere

				std::size_t rj_idx = 0;
				for (const auto& rj : s.get_running_jobs()) {
					// We only need to explore one idle core, so skip any
					// others that we encounter.
					if (found_idle && rj.is_idle()) {
						rj_idx++;
						continue;
					}

					// Check if this job's core becomes available by the time
					// we must be scheduled.
					auto est = rj.earliest_start_time(j);
					if (est <= lst) {
						// ok, this transition is actually feasible
						const State& next = be_naive ?
							new_state(s, j, index_of(j), rj_idx, lst) :
							new_or_merged_state(s, j, index_of(j), rj_idx, lst);

						// update response times
						auto ftimes = Interval<Time>(est + j.least_cost(),
						                             lst + j.maximal_cost());
						update_finish_times(j, ftimes);
						check_for_deadline_misses(t_earliest, next);
#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
						edges.emplace_back(&j, &s, &next, ftimes);
#endif
						num_edges++;

						found_one = true;

						if (rj.is_idle())
							found_idle = true;
					}
					rj_idx++;
				}

				return found_one;
			}

			void explore_state(const State& s)
			{
				DM("\nLooking at: " << s << std::endl);

				auto rjs = s.get_running_jobs();

				// earliest time the next job could get scheduled
				auto t_earliest = rjs.possible_core_availability_time();

				// earliest certain core availability
				auto t_core = rjs.certain_core_availability_time();

				// latest time some unfinished job is certainly released
				auto t_any_released = next_certain_job_release(s, t_earliest);

				// time by which some job is certainly scheduled next
				auto t_horizon = std::max(t_core, t_any_released);

				bool found_one = false;

				// (1) first check jobs that may be already pending
				for (const Job<Time>& j : jobs_by_win.lookup(t_earliest)) {
					DM("    -?? " << j << std::endl);
					if (incomplete(s, j)
						&& j.earliest_arrival() <= t_earliest) {
						found_one |= try_to_schedule(s, j, t_earliest, t_horizon);
					}
				}

				// (2) check jobs that are released only later in the interval
				for (auto it = jobs_by_earliest_arrival
							   .upper_bound(t_earliest);
					 it != jobs_by_earliest_arrival.end();
					 it++) {
					const Job<Time>& j = *it->second;
					// stop looking once we've left the range of interest
					DM("    -?? " << j << std::endl);
					if (j.earliest_arrival() > t_horizon)
						break;
					// Since this job is released in the future, it better
					// be incomplete...
					assert(incomplete(s, j));

					found_one |= try_to_schedule(s, j, t_earliest, t_horizon);
				}

				// check for a dead end
				if (!found_one
					&& s.get_scheduled_jobs().size() != jobs.size()) {
					// out of options and we didn't schedule all jobs
					aborted = true;
				}
			}

			void explore()
			{
				make_initial_state();
				if (!be_naive) {
					// add initial state to cache
					cache_state(states_storage.front().begin());
				}

				while (current_job_count < jobs.size() && !aborted) {
					States& exploration_front = states();
					// allocate states space for next depth
					states_storage.emplace_back();

					// keep track of exploration front width
					width = std::max(width, exploration_front.size());

					// explore all states at the current depth
					for (const State& s : exploration_front) {
						explore_state(s);
						check_cpu_timeout();
						if (aborted)
							break;
					}

					// clean up the state cache if necessary
					if (!be_naive)
						states_by_key.clear();

					current_job_count++;
					check_depth_abort();

#ifndef CONFIG_COLLECT_SCHEDULE_GRAPH
					// If we don't need to collect all states, we can remove
					// all those that we are done with, which saves a lot of
					// memory.
					states_storage.pop_front();
#endif
				}
			}


#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
			friend std::ostream& operator<< (std::ostream& out,
			                                 const State_space<Time>& space)
			{
					std::map<const Schedule_state<Time>*, unsigned int> state_id;
					unsigned int i = 0;
					out << "digraph {" << std::endl;
					for (const States& states : space.get_states()) {
						for (const Schedule_state<Time>& s : states) {
							state_id[&s] = i++;
							out << "\tS" << state_id[&s]
								<< "[label=\"S" << state_id[&s] << ": [";
							for (const auto& rj : s.get_running_jobs()) {
								out << (rj.is_idle() ? "(idle, " : "(busy, ");
								if (rj.earliest_finish_time() ==
									Time_model::constants<Time>::infinity()) {
									out << "infinity, infinity)";
								} else {
									out << rj.earliest_finish_time() << ", "
										<< rj.latest_finish_time() << ")";
								}
							}
							out << "]\"];" << std::endl;
						}
					}
					for (const auto& e : space.get_edges()) {
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

namespace std
{
	template<class Time> struct hash<NP::Global::Schedule_state<Time>>
    {
		std::size_t operator()(NP::Global::Schedule_state<Time> const& s) const
        {
            return s.get_key();
        }
    };
}


#endif
