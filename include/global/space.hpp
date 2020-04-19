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

#ifdef CONFIG_PARALLEL
#include "tbb/concurrent_hash_map.h"
#include "tbb/enumerable_thread_specific.h"
#include "tbb/parallel_for.h"
#endif

#include "problem.hpp"
#include "clock.hpp"

#include "global/state.hpp"

namespace NP {

	namespace Global {

		template<class Time> class State_space
		{
			public:

			typedef Scheduling_problem<Time> Problem;
			typedef typename Scheduling_problem<Time>::Workload Workload;
			typedef Schedule_state<Time> State;

			static State_space explore(
					const Problem& prob,
					const Analysis_options& opts)
			{
				// doesn't yet support exploration after deadline miss
				assert(opts.early_exit);

				auto s = State_space(prob.jobs, prob.dag, prob.num_processors, opts.timeout,
				                     opts.max_depth, opts.num_buckets);
				s.be_naive = opts.be_naive;
				s.cpu_time.start();
				s.explore();
				s.cpu_time.stop();
				return s;

			}

			// convenience interface for tests
			static State_space explore_naively(
				const Workload& jobs,
				unsigned int num_cpus)
			{
				Problem p{jobs, num_cpus};
				Analysis_options o;
				o.be_naive = true;
				return explore(p, o);
			}

			// convenience interface for tests
			static State_space explore(
				const Workload& jobs,
				unsigned int num_cpus)
			{
				Problem p{jobs, num_cpus};
				Analysis_options o;
				return explore(p, o);
			}

			Interval<Time> get_finish_times(const Job<Time>& j) const
			{
				auto rbounds = rta.find(j.get_id());
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

#ifdef CONFIG_PARALLEL
			typedef tbb::enumerable_thread_specific< States > Split_states;
			typedef std::deque<Split_states> States_storage;
#else
			typedef std::deque< States > States_storage;
#endif

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

			const States_storage& get_states() const
			{
				return states_storage;
			}

#endif
			private:

			typedef typename std::deque<State>::iterator State_ref;
			typedef typename std::forward_list<State_ref> State_refs;

#ifdef CONFIG_PARALLEL
			typedef tbb::concurrent_hash_map<hash_value_t, State_refs> States_map;
			typedef typename States_map::accessor States_map_accessor;
#else
			typedef std::unordered_map<hash_value_t, State_refs> States_map;
#endif

			typedef const Job<Time>* Job_ref;
			typedef std::multimap<Time, Job_ref> By_time_map;

			typedef std::deque<State_ref> Todo_queue;

			typedef Interval_lookup_table<Time, Job<Time>, Job<Time>::scheduling_window> Jobs_lut;

			typedef std::unordered_map<JobID, Interval<Time> > Response_times;

#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
			std::deque<Edge> edges;
#endif

			Response_times rta;

#ifdef CONFIG_PARALLEL
			tbb::enumerable_thread_specific<Response_times> partial_rta;
#endif

			bool aborted;
			bool timed_out;

			const unsigned int max_depth;

			bool be_naive;

			const Workload& jobs;

			// not touched after initialization
			Jobs_lut _jobs_by_win;
			By_time_map _jobs_by_latest_arrival;
			By_time_map _jobs_by_earliest_arrival;
			By_time_map _jobs_by_deadline;
			std::vector<Job_precedence_set> _predecessors;

			// use these const references to ensure read-only access
			const Jobs_lut& jobs_by_win;
			const By_time_map& jobs_by_latest_arrival;
			const By_time_map& jobs_by_earliest_arrival;
			const By_time_map& jobs_by_deadline;
			const std::vector<Job_precedence_set>& predecessors;

			States_storage states_storage;

			States_map states_by_key;
			// updated only by main thread
			unsigned long num_states, width;
			unsigned long current_job_count;
			unsigned long num_edges;

#ifdef CONFIG_PARALLEL
			tbb::enumerable_thread_specific<unsigned long> edge_counter;
#endif
			Processor_clock cpu_time;
			const double timeout;

			const unsigned int num_cpus;

			State_space(const Workload& jobs,
			            const Precedence_constraints &dag_edges,
			            unsigned int num_cpus,
			            double max_cpu_time = 0,
			            unsigned int max_depth = 0,
			            std::size_t num_buckets = 1000)
			: _jobs_by_win(Interval<Time>{0, max_deadline(jobs)},
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
			, jobs_by_latest_arrival(_jobs_by_latest_arrival)
			, jobs_by_earliest_arrival(_jobs_by_earliest_arrival)
			, jobs_by_deadline(_jobs_by_deadline)
			, jobs_by_win(_jobs_by_win)
			, _predecessors(jobs.size())
			, predecessors(_predecessors)
			{
				for (const Job<Time>& j : jobs) {
					_jobs_by_latest_arrival.insert({j.latest_arrival(), &j});
					_jobs_by_earliest_arrival.insert({j.earliest_arrival(), &j});
					_jobs_by_deadline.insert({j.get_deadline(), &j});
					_jobs_by_win.insert(j);
				}

				for (auto e : dag_edges) {
					const Job<Time>& from = lookup<Time>(jobs, e.first);
					const Job<Time>& to   = lookup<Time>(jobs, e.second);
					_predecessors[index_of(to)].push_back(index_of(from));
				}
			}

			private:

			void count_edge()
			{
#ifdef CONFIG_PARALLEL
				edge_counter.local()++;
#else
				num_edges++;
#endif
			}

			static Time max_deadline(const Workload &jobs)
			{
				Time dl = 0;
				for (const auto& j : jobs)
					dl = std::max(dl, j.get_deadline());
				return dl;
			}

			void update_finish_times(Response_times& r, const JobID& id,
			                         Interval<Time> range)
			{
				auto rbounds = r.find(id);
				if (rbounds == r.end()) {
					r.emplace(id, range);
				} else {
					rbounds->second |= range;
				}
				DM("RTA " << id << ": " << r.find(id)->second << std::endl);
			}

			void update_finish_times(
				Response_times& r, const Job<Time>& j, Interval<Time> range)
			{
				update_finish_times(r, j.get_id(), range);
				if (j.exceeds_deadline(range.upto()))
					aborted = true;
			}

			void update_finish_times(const Job<Time>& j, Interval<Time> range)
			{
				Response_times& r =
#ifdef CONFIG_PARALLEL
					partial_rta.local();
#else
					rta;
#endif
				update_finish_times(r, j, range);
			}


			std::size_t index_of(const Job<Time>& j) const
			{
				return (std::size_t) (&j - &(jobs[0]));
			}

			const Job_precedence_set& predecessors_of(const Job<Time>& j) const
			{
				return predecessors[index_of(j)];
			}

			void check_for_deadline_misses(const State& old_s, const State& new_s)
			{
				auto check_from = old_s.core_availability().min();
				auto earliest   = new_s.core_availability().min();

				// check if we skipped any jobs that are now guaranteed
				// to miss their deadline
				for (auto it = jobs_by_deadline.lower_bound(check_from);
				     it != jobs_by_deadline.end(); it++) {
					const Job<Time>& j = *(it->second);
					if (j.get_deadline() < earliest) {
						if (unfinished(new_s, j)) {
							DM("deadline miss: " << new_s << " -> " << j << std::endl);
							// This job is still incomplete but has no chance
							// of being scheduled before its deadline anymore.
							// Abort.
							aborted = true;
							// create a dummy state for explanation purposes
							auto frange = new_s.core_availability() + j.get_cost();
							const State& next =
								new_state(new_s, index_of(j), predecessors_of(j),
								          frange, frange, j.get_key());
							// update response times
							update_finish_times(j, frange);
#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
							edges.emplace_back(&j, &new_s, &next, frange);
#endif
							count_edge();
							break;
						}
					} else
						// deadlines now after the next earliest finish time
						break;
				}
			}

			void make_initial_state()
			{
				// construct initial state
				states_storage.emplace_back();
				new_state(num_cpus);
			}

			States& states()
			{
#ifdef CONFIG_PARALLEL
				return states_storage.back().local();
#else
				return states_storage.back();
#endif
			}

			template <typename... Args>
			State_ref alloc_state(Args&&... args)
			{
				states().emplace_back(std::forward<Args>(args)...);
				State_ref s = --states().end();

				// make sure we didn't screw up...
				auto njobs = s->number_of_scheduled_jobs();
				assert (
					(!njobs && num_states == 0) // initial state
				    || (njobs == current_job_count + 1) // normal State
				    || (njobs == current_job_count + 2 && aborted) // deadline miss
				);

				return s;
			}

			void dealloc_state(State_ref s)
			{
				assert(--states().end() == s);
				states().pop_back();
			}

			template <typename... Args>
			State& new_state(Args&&... args)
			{
				return *alloc_state(std::forward<Args>(args)...);
			}

			template <typename... Args>
			State& new_or_merged_state(Args&&... args)
			{
				State_ref s_ref = alloc_state(std::forward<Args>(args)...);

				// try to merge the new state into an existing state
				State_ref s = merge_or_cache(s_ref);
				if (s != s_ref) {
					// great, we merged!
					// clean up the just-created state that we no longer need
					dealloc_state(s_ref);
				}
				return *s;
			}

#ifdef CONFIG_PARALLEL

			// make state available for fast lookup
			void insert_cache_state(States_map_accessor &acc, State_ref s)
			{
				assert(!acc.empty());

				State_refs& list = acc->second;
				list.push_front(s);
			}

			// returns true if state was merged
			State_ref merge_or_cache(State_ref s)
			{
				States_map_accessor acc;

				while (true) {
					// check if key exists
					if (states_by_key.find(acc, s->get_key())) {
						for (State_ref other : acc->second)
							if (other->try_to_merge(*s))
								return other;
						// If we reach here, we failed to merge, so go ahead
						// and insert it.
						insert_cache_state(acc, s);
						return s;
					// otherwise, key doesn't exist yet, let's try to create it
					} else if (states_by_key.insert(acc, s->get_key())) {
						// We created the list, so go ahead and insert our state.
						insert_cache_state(acc, s);
						return s;
					}
					// if we raced with concurrent creation, try again
				}
			}

#else

			void cache_state(State_ref s)
			{
				// create a new list if needed, or lookup if already existing
				auto res = states_by_key.emplace(
					std::make_pair(s->get_key(), State_refs()));

				auto pair_it = res.first;
				State_refs& list = pair_it->second;

				list.push_front(s);
			}


			State_ref merge_or_cache(State_ref s_ref)
			{
				State& s = *s_ref;

				const auto pair_it = states_by_key.find(s.get_key());

				// cannot merge if key doesn't exist
				if (pair_it != states_by_key.end())
					for (State_ref other : pair_it->second)
						if (other->try_to_merge(*s_ref))
							return other;
				// if we reach here, we failed to merge
				cache_state(s_ref);
				return s_ref;
			}
#endif

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

			bool unfinished(const State& s, const Job<Time>& j) const
			{
				return s.job_incomplete(index_of(j));
			}

			bool ready(const State& s, const Job<Time>& j) const
			{
				return unfinished(s, j) && s.job_ready(predecessors_of(j));
			}

			bool all_jobs_scheduled(const State& s) const
			{
				return s.number_of_scheduled_jobs() == jobs.size();
			}

			// assumes j is ready
			Interval<Time> ready_times(const State& s, const Job<Time>& j) const
			{
				Interval<Time> r = j.arrival_window();
				for (auto pred : predecessors_of(j)) {
					Interval<Time> ft{0, 0};
					if (!s.get_finish_times(pred, ft))
						ft = get_finish_times(jobs[pred]);
					r.lower_bound(ft.min());
					r.extend_to(ft.max());
				}
				return r;
			}

			// assumes j is ready
			Interval<Time> ready_times(
				const State& s, const Job<Time>& j,
				const Job_precedence_set& disregard) const
			{
				Interval<Time> r = j.arrival_window();
				for (auto pred : predecessors_of(j)) {
					// skip if part of disregard
					if (contains(disregard, pred))
						continue;
					Interval<Time> ft{0, 0};
					if (!s.get_finish_times(pred, ft))
						ft = get_finish_times(jobs[pred]);
					r.lower_bound(ft.min());
					r.extend_to(ft.max());
				}
				return r;
			}

			Time latest_ready_time(const State& s, const Job<Time>& j) const
			{
				return ready_times(s, j).max();
			}

			Time earliest_ready_time(const State& s, const Job<Time>& j) const
			{
				return ready_times(s, j).min();
			}

			Time latest_ready_time(
				const State& s, Time earliest_ref_ready,
				const Job<Time>& j_hp, const Job<Time>& j_ref) const
			{
				auto rt = ready_times(s, j_hp, predecessors_of(j_ref));
				return std::max(rt.max(), earliest_ref_ready);
			}

			// Find next time by which any job is certainly released.
			// Note that this time may be in the past.
			Time next_higher_prio_job_ready(
				const State& s,
				const Job<Time> &reference_job,
				const Time t_earliest) const
			{
				auto ready_min = earliest_ready_time(s, reference_job);
				Time when = Time_model::constants<Time>::infinity();

				// check everything that overlaps with t_earliest
				for (const Job<Time>& j : jobs_by_win.lookup(t_earliest))
					if (ready(s, j)
					    && j.higher_priority_than(reference_job)) {
						when = std::min(when,
							latest_ready_time(s, ready_min, j, reference_job));
					}

				// No point looking in the future when we've already
				// found one in the present.
				if (when <= t_earliest)
					return when;

				// Ok, let's look also in the future.
				for (auto it = jobs_by_latest_arrival
				               .lower_bound(t_earliest);
				     it != jobs_by_latest_arrival.end(); it++) {
					const Job<Time>& j = *(it->second);

					// check if we can stop looking
					if (when < j.latest_arrival())
						break; // yep, nothing can lower 'when' at this point

					// j is not relevant if it is already scheduled or blocked
					if (ready(s, j)
					    && j.higher_priority_than(reference_job)) {
						// does it beat what we've already seen?
						when = std::min(when,
							latest_ready_time(s, ready_min, j, reference_job));
					}
				}

				return when;
			}

			// Find next time by which any job is certainly released.
			// Note that this time may be in the past.
			Time next_job_ready(const State& s, const Time t_earliest) const
			{
				Time when = Time_model::constants<Time>::infinity();

				// check everything that overlaps with t_earliest
				for (const Job<Time>& j : jobs_by_win.lookup(t_earliest))
					if (ready(s, j))
						when = std::min(when, latest_ready_time(s, j));

				// No point looking in the future when we've already
				// found one in the present.
				if (when <= t_earliest)
					return when;

				// Ok, let's look also in the future.
				for (auto it = jobs_by_latest_arrival
				               .lower_bound(t_earliest);
				     it != jobs_by_latest_arrival.end(); it++) {
					const Job<Time>& j = *(it->second);

					// check if we can stop looking
					if (when < j.latest_arrival())
						break; // yep, nothing can lower 'when' at this point

					// j is not relevant if it is already scheduled or blocked
					if (ready(s, j))
						// does it beat what we've already seen?
						when = std::min(when, latest_ready_time(s, j));
				}

				return when;
			}

			// assumes j is ready
			// NOTE: we don't use Interval<Time> here because the
			//       Interval c'tor sorts its arguments.
			std::pair<Time, Time> start_times(
				const State& s, const Job<Time>& j, Time t_wc) const
			{
				auto rt = ready_times(s, j);
				auto at = s.core_availability();
				Time est = std::max(rt.min(), at.min());

				DM("rt: " << rt << std::endl
				<< "at: " << at << std::endl);

				auto t_high = next_higher_prio_job_ready(s, j, at.min());
				Time lst    = std::min(t_wc,
					t_high - Time_model::constants<Time>::epsilon());

				DM("est: " << est << std::endl);
				DM("lst: " << lst << std::endl);

				return {est, lst};
			}

			bool dispatch(const State& s, const Job<Time>& j, Time t_wc)
			{
				// check if this job has a feasible start-time interval
				auto _st = start_times(s, j, t_wc);
				if (_st.first > _st.second)
					return false; // nope

				Interval<Time> st{_st};

				// yep, job j is a feasible successor in state s

				// compute range of possible finish times
				Interval<Time> ftimes = st + j.get_cost();

				// update finish-time estimates
				update_finish_times(j, ftimes);

				// expand the graph, merging if possible
				const State& next = be_naive ?
					new_state(s, index_of(j), predecessors_of(j),
					          st, ftimes, j.get_key()) :
					new_or_merged_state(s, index_of(j), predecessors_of(j),
					                    st, ftimes, j.get_key());

				// make sure we didn't skip any jobs
				check_for_deadline_misses(s, next);

#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
				edges.emplace_back(&j, &s, &next, ftimes);
#endif
				count_edge();

				return true;
			}

			void explore(const State& s)
			{
				bool found_one = false;

				DM("----" << std::endl);

				// (0) define the window of interest

				// earliest time a core is possibly available
				auto t_min  = s.core_availability().min();
				// latest time some unfinished job is certainly ready
				auto t_job  = next_job_ready(s, t_min);
				// latest time some core is certainly available
				auto t_core = s.core_availability().max();
				// latest time by which a work-conserving scheduler
				// certainly schedules some job
				auto t_wc   = std::max(t_core, t_job);

				DM(s << std::endl);
				DM("t_min: " << t_min << std::endl
				<< "t_job: " << t_job << std::endl
				<< "t_core: " << t_core << std::endl
				<< "t_wc: " << t_wc << std::endl);

				DM("==== [1] ====" << std::endl);
				// (1) first check jobs that may be already pending
				for (const Job<Time>& j : jobs_by_win.lookup(t_min))
					if (j.earliest_arrival() <= t_min && ready(s, j))
						found_one |= dispatch(s, j, t_wc);

				DM("==== [2] ====" << std::endl);
				// (2) check jobs that are released only later in the interval
				for (auto it = jobs_by_earliest_arrival.upper_bound(t_min);
					 it != jobs_by_earliest_arrival.end();
					 it++) {
					const Job<Time>& j = *it->second;
					DM(j << " (" << index_of(j) << ")" << std::endl);
					// stop looking once we've left the window of interest
					if (j.earliest_arrival() > t_wc)
						break;

					// Job could be not ready due to precedence constraints
					if (!ready(s, j))
						continue;

					// Since this job is released in the future, it better
					// be incomplete...
					assert(unfinished(s, j));

					found_one |= dispatch(s, j, t_wc);
				}

				// check for a dead end
				if (!found_one && !all_jobs_scheduled(s))
					// out of options and we didn't schedule all jobs
					aborted = true;
			}

			// naive: no state merging
			void explore_naively()
			{
				be_naive = true;
				explore();
			}

			void explore()
			{
				make_initial_state();

				while (current_job_count < jobs.size()) {
					unsigned long n;
#ifdef CONFIG_PARALLEL
					const auto& new_states_part = states_storage.back();
					n = 0;
					for (const States& new_states : new_states_part) {
						n += new_states.size();
					}
#else
					States& exploration_front = states();
					n = exploration_front.size();
#endif

					// allocate states space for next depth
					states_storage.emplace_back();

					// keep track of exploration front width
					width = std::max(width, n);

					num_states += n;

					check_depth_abort();
					check_cpu_timeout();
					if (aborted)
						break;

#ifdef CONFIG_PARALLEL

					parallel_for(new_states_part.range(),
						[&] (typename Split_states::const_range_type& r) {
							for (auto it = r.begin(); it != r.end(); it++) {
								const States& new_states = *it;
								auto s = new_states.size();
								tbb::parallel_for(tbb::blocked_range<size_t>(0, s),
									[&] (const tbb::blocked_range<size_t>& r) {
										for (size_t i = r.begin(); i != r.end(); i++)
											explore(new_states[i]);
								});
							}
						});

#else
					for (const State& s : exploration_front) {
						explore(s);
						check_cpu_timeout();
						if (aborted)
							break;
					}
#endif

					// clean up the state cache if necessary
					if (!be_naive)
						states_by_key.clear();

					current_job_count++;

#ifdef CONFIG_PARALLEL
					// propagate any updates to the response-time estimates
					for (auto& r : partial_rta)
						for (const auto& elem : r)
							update_finish_times(rta, elem.first, elem.second);
#endif

#ifndef CONFIG_COLLECT_SCHEDULE_GRAPH
					// If we don't need to collect all states, we can remove
					// all those that we are done with, which saves a lot of
					// memory.
#ifdef CONFIG_PARALLEL
					parallel_for(states_storage.front().range(),
						[] (typename Split_states::range_type& r) {
							for (auto it = r.begin(); it != r.end(); it++)
								it->clear();
						});
#endif
					states_storage.pop_front();
#endif
				}


#ifndef CONFIG_COLLECT_SCHEDULE_GRAPH
				// clean out any remaining states
				while (!states_storage.empty()) {
#ifdef CONFIG_PARALLEL
					parallel_for(states_storage.front().range(),
						[] (typename Split_states::range_type& r) {
							for (auto it = r.begin(); it != r.end(); it++)
								it->clear();
						});
#endif
					states_storage.pop_front();
				}
#endif


#ifdef CONFIG_PARALLEL
				for (auto &c : edge_counter)
					num_edges += c;
#endif
			}


#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
			friend std::ostream& operator<< (std::ostream& out,
			                                 const State_space<Time>& space)
			{
					std::map<const Schedule_state<Time>*, unsigned int> state_id;
					unsigned int i = 0;
					out << "digraph {" << std::endl;
#ifdef CONFIG_PARALLEL
					for (const Split_states& states : space.get_states()) {
						for (const Schedule_state<Time>& s : tbb::flattened2d<Split_states>(states)) {
#else
					for (const auto& front : space.get_states()) {
						for (const Schedule_state<Time>& s : front) {
#endif
							state_id[&s] = i++;
							out << "\tS" << state_id[&s]
								<< "[label=\"S" << state_id[&s] << ": ";
							s.print_vertex_label(out, space.jobs);
							out << "\"];" << std::endl;
						}
					}
					for (const auto& e : space.get_edges()) {
						out << "\tS" << state_id[e.source]
						    << " -> "
						    << "S" << state_id[e.target]
						    << "[label=\""
						    << "T" << e.scheduled->get_task_id()
						    << " J" << e.scheduled->get_job_id()
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
