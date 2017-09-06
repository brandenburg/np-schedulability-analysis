#ifndef SCHEDULE_SPACE_H
#define SCHEDULE_SPACE_H

#include <unordered_set>
#include <unordered_map>
#include <map>
#include <vector>
#include <deque>
#include <list>

#include <iostream>
#include <ostream>
#include <cassert>

#include "jobs.hpp"

// DM : debug message -- disable for now
// #define DM(x) std::cerr << x
#define DM(x)

#define CONFIG_COLLECT_SCHEDULE_GRAPH

namespace NP {

	template<class Time> struct Types
	{
		typedef const Job<Time>* job_uid_t;

		typedef std::unordered_set<job_uid_t> Job_uid_set;

		static job_uid_t uid_of(const Job<Time>& job)
		{
			return &job;
		}

	};

	// use pointers as a primitive form of unique ID

	namespace Uniproc {


		template<class Time> class Schedule_state
		{
			private:

			Interval<Time> finish_time;

			typename Types<Time>::Job_uid_set scheduled_jobs;
			hash_value_t lookup_key;

			Schedule_state(Time eft, Time lft,
						   const typename Types<Time>::Job_uid_set &jobs,
						   hash_value_t k)
			: finish_time{eft, lft}, scheduled_jobs{jobs}, lookup_key{k}
			{
			}

			public:

			static Schedule_state initial_state()
			{
				return Schedule_state{0, 0, typename Types<Time>::Job_uid_set(), 0};
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

			const typename Types<Time>::Job_uid_set& get_scheduled_jobs() const
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
				const Time other_certain_start = Time_model::constants<Time>::infinity()) const
			{
				// t_s'
				auto own_latest_start = std::max(latest_finish_time(),
												 j.latest_arrival());

				// IIP: TBD

				// t_R
				auto last_start_before_other = other_certain_start - Time_model::constants<Time>::epsilon();

				return std::min(own_latest_start, last_start_before_other)
					   + j.maximal_cost();
			}

			hash_value_t next_key(const Job<Time>& j) const
			{
				return get_key() ^ j.get_key();
			}

			Schedule_state<Time> schedule(
				const Job<Time>& j,
				const Time other_certain_start = Time_model::constants<Time>::infinity()) const
			{
				DM("cost: " << j.least_cost() << "--" << j.maximal_cost() <<  std::endl);
				DM("Other: " << other_certain_start <<  std::endl);
				Time eft = next_earliest_finish_time(j);
				Time lft = next_latest_finish_time(j, other_certain_start);
				DM("eft:" << eft << " lft:" << lft << std::endl);
				Schedule_state next(eft, lft, scheduled_jobs, next_key(j));
				next.scheduled_jobs.insert(Types<Time>::uid_of(j));
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


		template<class Time> class State_space
		{
			public:

			typedef Job<Time> Job;
			typedef std::vector<Job> Workload;
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

			Interval<Time> get_finish_times(const Job& j) const
			{
				auto rbounds = rta.find(Types<Time>::uid_of(j));
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
				return states.size();
			}

#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH

			struct Edge {
				const Job* scheduled;
				const State* source;
				const State* target;
			};

			const std::list<Edge>& get_edges() const
			{
				return edges;
			}

			const std::list<State>& get_states() const
			{
				return states;
			}

#endif
			private:

			typedef State* State_ref;
			typedef std::list<State> States;
			typedef std::unordered_map<hash_value_t, State_ref> States_map;

			typedef const Job* Job_ref;
			typedef std::multimap<Time, Job_ref> By_time_map;

			typedef std::deque<State_ref> Todo_queue;

			typedef Interval_lookup_table<Time, Job, Job::scheduling_window> Jobs_lut;

			typedef std::unordered_map<typename Types<Time>::job_uid_t, Interval<Time> > Response_times;

#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
			std::list<Edge> edges;
#endif

			Response_times rta;
			bool aborted;

			const Workload& jobs;

			Jobs_lut jobs_by_win;

			By_time_map jobs_by_latest_arrival;
			By_time_map jobs_by_earliest_arrival;

			States states;
			States_map states_by_key;
			Todo_queue todo;

			State_space(const Workload& jobs,
			            std::size_t num_buckets = 1000)
			: jobs_by_win(Interval<Time>{0, max_deadline(jobs)},
			              max_deadline(jobs) / num_buckets),
			  jobs(jobs), aborted(false)
			{
				for (const Job& j : jobs) {
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


			void update_finish_times(const Job& j, Interval<Time> range)
			{
				auto rbounds = rta.find(Types<Time>::uid_of(j));
				if (rbounds == rta.end()) {
					rta.emplace(std::make_pair(Types<Time>::uid_of(j), range));
					if (!(range.upto() < j.get_deadline()))
						aborted = true;
				} else {
					rbounds->second.widen(range);
					if (!(rbounds->second.upto() < j.get_deadline()))
						aborted = true;
				}
				DM("RTA " << j.get_id() << ": " << rta.find(Types<Time>::uid_of(j))->second << std::endl);
			}

			static bool incomplete(const typename Types<Time>::Job_uid_set &scheduled, const Job& j)
			{
				return scheduled.find(Types<Time>::uid_of(j)) == scheduled.end();
			}

			static bool incomplete(const State& s, const Job& j)
			{
				return incomplete(s.get_scheduled_jobs(), j);
			}

			// find next time by which a job is certainly released on
			// or after a given point in time
			Time next_certain_job_release(
				Time on_or_after,
				const typename Types<Time>::Job_uid_set &already_scheduled)
			{
				for (auto it = jobs_by_latest_arrival.lower_bound(on_or_after);
				     it != jobs_by_latest_arrival.end(); it++) {
					const Job& j = *(it->second);

					// not relevant if already scheduled
					if (!incomplete(already_scheduled, j))
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
				const typename Types<Time>::Job_uid_set &already_scheduled,
				const Job& reference_job)
			{
				for (auto it = jobs_by_latest_arrival.lower_bound(on_or_after);
				     it != jobs_by_latest_arrival.end(); it++) {
					const Job& j = *(it->second);

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
			bool exists_certainly_released_job(Time at,
				typename Types<Time>::Job_uid_set already_scheduled)
			{
				for (const Job& j : jobs_by_win.lookup(at))
					if (j.latest_arrival() <= at &&
					    incomplete(already_scheduled, j)) {
						return true;
					}
				return false;
			}

			// returns true if there is certainly some pending job of higher
			// priority at the given time ready to be scheduled
			bool exists_certainly_released_higher_prio_job(
				Time at,
				typename Types<Time>::Job_uid_set already_scheduled,
				const Job& reference_job)
			{
				for (const Job& j : jobs_by_win.lookup(at))
					if (j.latest_arrival() <= at &&
					    &j != &reference_job &&
					    incomplete(already_scheduled, j) &&
					    j.higher_priority_than(reference_job)) {
						return true;
					}
				return false;
			}

			bool priority_eligible(const State &s, const Job &j)
			{
				auto t_s = s.next_earliest_start_time(j);
				return !exists_certainly_released_higher_prio_job(
				            t_s, s.get_scheduled_jobs(), j);
			}

			bool potentially_next(const State &s, const Job &j)
			{
				auto t_latest = s.latest_finish_time();
				if (t_latest < j.earliest_arrival()) {
					// any certainly pending at t_latest
					if (exists_certainly_released_job(
					        t_latest, s.get_scheduled_jobs()))
						// if something is certainly pending at t_latest, then
						// j can't be next
						// check with IIP: TBD
						return false;

					// any certainly pending since then
					Time r = next_certain_job_release(t_latest,
					                                  s.get_scheduled_jobs());
					// if something else is certainly released before j, then
					// j can't possibly be next
					// check with IIP: TBD
					if (r < j.earliest_arrival())
						return false;
				}
				return true;
			}

			bool is_eligible_successor(const State &s, const Job &j)
			{
				if (!incomplete(s, j)) {
					DM("    * not incomplete" <<  std::endl);
					return false;
				}
				if (!priority_eligible(s, j)) {
					DM("    * not priority eligible" <<  std::endl);
					return false;
				}
				if (!potentially_next(s, j)) {
					DM("    * not potentially next" <<  std::endl);
					return false;
				}
				return true;

// 				return incomplete(s, j) // Not yet scheduled
// 				       && priority_eligible(s, j)
// 				       && potentially_next(s, j);

				// IIP eligible: TBD
			}

			State& new_state(State&& s)
			{
				states.emplace_back(s);
				State& s_ref = states.back();
				todo.push_back(&s_ref);
				states_by_key.insert(std::make_pair(s_ref.get_key(), &s_ref));
				return s_ref;
			}

			const State& next_state()
			{
				assert(!todo.empty());
				auto s = todo.front();
				todo.pop_front();
				return *s;
			}

			bool not_done()
			{
				return !todo.empty();
			}

			// naive: no state merging
			void schedule_naively(const State &s, const Job &j)
			{
				Time other_certain_start =
					next_certain_higher_priority_job_release(
						s.next_earliest_start_time(j),
						s.get_scheduled_jobs(), j);
				DM("nest=" << s.next_earliest_start_time(j) << std::endl);
				DM("other_certain_start=" << other_certain_start << std::endl);
				const State& next = new_state(s.schedule(j, other_certain_start));
				// update response times
				update_finish_times(j, next.finish_range());
#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
				edges.emplace_back(Edge{&j, &s, &next});
#endif
			}

			void explore_naively()
			{
				new_state(State::initial_state());

				while (not_done() && !aborted) {
					const State& s = next_state();

					DM("\nLooking at: " << s << std::endl);

					// Identify relevant interval for next job
					// relevant job buckets
					auto ts_min = s.earliest_finish_time();
					auto latest_idle = next_certain_job_release(ts_min,
					                                  s.get_scheduled_jobs());
					// TBD: latest_idle depends on IIP

					Interval<Time> next_range{ts_min,
					    std::max(latest_idle, s.latest_finish_time())};

					DM(" => " << next_range << std::endl);

					bool found_at_least_one = false;

					// (1) first check jobs that can be already pending
					for (const Job& j : jobs_by_win.lookup(ts_min)) {
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
						const Job& j = *it->second;
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
				}
			}


			void schedule(const State &s, const Job &j)
			{
				auto k = s.next_key(j);

				auto r = states_by_key.equal_range(k);

				if (r.first != r.second) {
					Time other_certain_start =
						next_certain_higher_priority_job_release(
							s.next_earliest_start_time(j),
							s.get_scheduled_jobs(), j);
					auto eft = s.next_earliest_finish_time(j);
					auto lft = s.next_latest_finish_time(j, other_certain_start);
					auto ftimes = Interval<Time>{eft, lft};

					for (auto it = r.first; it != r.second; it++) {
						if (ftimes.intersects(it->second->finish_range())) {

							update_finish_times(j, ftimes);
							it->second->update_finish_range(ftimes);
#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
							edges.emplace_back(Edge{&j, &s, it->second});
#endif
							return;
						}
					}
				}

				schedule_naively(s, j);
			}

			void explore()
			{
				new_state(State::initial_state());

				while (not_done() && !aborted) {
					const State& s = next_state();

					DM("\nLooking at: " << s << std::endl);

					// Identify relevant interval for next job
					// relevant job buckets
					auto ts_min = s.earliest_finish_time();
					auto latest_idle = next_certain_job_release(ts_min,
					                                  s.get_scheduled_jobs());
					// TBD: latest_idle depends on IIP

					Interval<Time> next_range{ts_min,
					    std::max(latest_idle, s.latest_finish_time())};

					DM(" => " << next_range << std::endl);

					bool found_at_least_one = false;

					// (1) first check jobs that can be already pending
					for (const Job& j : jobs_by_win.lookup(ts_min)) {
						DM("    -?? " << j << std::endl);
						// if it is potentially active in the range of
						// interest...
						if (j.scheduling_window().intersects(next_range))
							// if it can be scheduled next...
							if (is_eligible_successor(s, j)) {
								DM("        --> ok!"  << std::endl);
								// create the relevant state and continue
								schedule(s, j);
								//new_state(schedule_naively(s, j));
								found_at_least_one = true;
							}
					}
					// (2) check jobs that are released later in the interval
					for (auto it = jobs_by_earliest_arrival.upper_bound(ts_min);
					     it != jobs_by_earliest_arrival.end(); it++) {
						const Job& j = *it->second;
						// stop looking once we've left the range of interest
						DM("    -?? " << j << std::endl);
						if (!j.scheduling_window().intersects(next_range))
							break;
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
					}
				}
			}

#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
			friend std::ostream& operator<< (std::ostream& out,
			                                 const State_space<Time>& space)
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
							<< "\"]"
							<< ";"
							<< std::endl;
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
	template<class Time> struct hash<NP::Uniproc::Schedule_state<Time>>
    {
		std::size_t operator()(NP::Uniproc::Schedule_state<Time> const& s) const
        {
            return s.get_key();
        }
    };
}


#endif
