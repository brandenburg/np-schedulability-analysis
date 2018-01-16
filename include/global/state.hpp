#include <iostream>
#include <ostream>
#include <cassert>
#include <algorithm>

#include <set>

#include "index_set.hpp"
#include "jobs.hpp"
#include "cache.hpp"

namespace NP {

	namespace Global {

		typedef Index_set Job_set;

		template<class Time> class Running_job
		{
			public:

			Running_job()
			: job(nullptr)
			, finish_time(0, 0)
			{
			}

			Running_job(const Running_job &other)
			: job(other.job)
			, finish_time(other.finish_time)
			{
			}

			bool is_idle() const
			{
				return job == nullptr;
			}

			void become_idle()
			{
				job = nullptr;
			}

			const Job<Time>& get_job() const
			{
				return *job;
			}

			const Job<Time>* get_job_ptr() const
			{
				return job;
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

			Interval<Time>& finish_range()
			{
				return finish_time;
			}

			Time earliest_start_time(const Job<Time>& job) const
			{
				return std::max(job.earliest_arrival(),
				                earliest_finish_time());
			}

			Time next_job(const Job<Time> &new_job, const Time latest_start)
			{
				auto earliest_start = std::max(
					earliest_finish_time(), new_job.earliest_arrival());

				finish_time = Interval<Time>(
					earliest_start + new_job.least_cost(),
					latest_start + new_job.maximal_cost());
				job = &new_job;

				return earliest_start;
			}

			void update_after_expansion(const Time horizon)
			{
				if (is_idle() || latest_finish_time() <= horizon) {
					become_idle();
					finish_time = Interval<Time>(horizon, horizon);
				} else
					finish_time.lower_bound(horizon);
			}

			void fast_forward(const Time t_min, const Time t_max)
			{
				if (latest_finish_time() <= t_min) {
					become_idle();
					finish_time = Interval<Time>(t_min, t_min);
				} else {
					finish_time.lower_bound(t_min);
				}
			}

			// sort by non-decreasing EFT
			bool operator<(const Running_job<Time>& other) const
			{
				return earliest_finish_time() < other.earliest_finish_time();
			}

			friend std::ostream& operator<< (std::ostream& stream,
			                                 const Running_job<Time>& rj)
			{
				stream << "(" << (rj.is_idle() ? "idle" : "busy") << ", "
				       << rj.earliest_finish_time() << ", "
				       << rj.latest_finish_time() << ")";
				return stream;
			}

			private:
				Interval<Time> finish_time;
				const Job<Time>* job;
		};


		template<class Time> class Running_jobs
		: public std::vector<Running_job<Time>>
		{
			public:

			// construct an empty (= initial) configuration
			Running_jobs(std::size_t num_cpus)
			: std::vector<Running_job<Time>>(num_cpus)
			{
			}

			// create a new configuration by merging two existing configurations
			Running_jobs(const Running_jobs<Time>& rjs1,
			             const Running_jobs<Time>& rjs2)
			: std::vector<Running_job<Time>>(rjs1)
			{
				auto it = this->begin();
				auto jt = rjs2.begin();
				for (;it != this->end() && jt != rjs2.end(); it++, jt++) {
					it->finish_range().widen(jt->finish_range());
				}
			}

			Time certain_core_availability_time() const
			{
				Time when = Time_model::constants<Time>::infinity();

				for (const auto& rj : *this)
					when = std::min(when, rj.latest_finish_time());

				return when;
			}

			Time possible_core_availability_time() const
			{
				Time when = Time_model::constants<Time>::infinity();

				for (const auto& rj : *this)
					when = std::min(when, rj.earliest_finish_time());

				return when;
			}

			bool idle_core_exists() const
			{
				for (const auto& rj : *this)
					if (rj.is_idle())
						return true;
				return false;
			}

			const Running_job<Time>& by_job(const Job<Time>& j) const
			{
				for (const auto& rj : *this)
					if (rj.get_job_ptr() == &j)
						return rj;
				abort();
			}

			void replace(
				std::size_t i,
				const Job<Time>& j,
				Time latest_start_time)
			{
				Running_job<Time> &rj = this->at(i);

				auto est = rj.next_job(j, latest_start_time);
				for (auto& rj : *this) {
					rj.update_after_expansion(est);
				}
			}

			void fast_forward(const Time t_min, const Time t_max)
			{
				for (auto& rj : *this)
					rj.fast_forward(t_min, t_max);
			}

			void ensure_sorted_by_eft()
			{
				std::sort(this->begin(), this->end());
			}

			bool intervals_compatible(const Running_jobs<Time> &other) const
			{
				// assumes both are sorted
				for (auto it = this->begin(), jt = other.begin();
				     it != this->end() && jt != other.end();
				     it++, jt++) {
					// check for idle / non-idle mismatch
					if (it->is_idle() ^ jt->is_idle())
						return false;
					if (std::max(it->earliest_finish_time(),
					             jt->earliest_finish_time()) >
					    std::min(it->latest_finish_time(),
					             jt->latest_finish_time()))
						return false;
				}
				return true;
			}

			unsigned int count_intervals_including(const Time t) const
			{
				unsigned int count = 0;
				for (const auto& rj : *this)
					count += rj.finish_range().contains(t);
				return count;
			}
		};

		template<class Time> class Schedule_state
		{
			public:

			// initial state -- nothing yet has finished, nothing is running
			Schedule_state(unsigned int num_processors)
			: scheduled_jobs()
			, running_jobs(num_processors)
			, lookup_key{0x9a9a9a9a9a9a9a9aUL}
			{
			}

			// transition: new state by scheduling a job in an existing state,
			//             by replacing a given running job.
			Schedule_state(
				const Schedule_state& from,
				const Job<Time>& j,
				std::size_t idx,
				std::size_t rj_to_replace,
				Time latest_start_time)
			: running_jobs(from.running_jobs)
			, scheduled_jobs{from.scheduled_jobs, idx}
			, lookup_key{from.lookup_key ^ j.get_key()}
			{
				running_jobs.replace(rj_to_replace, j, latest_start_time);
			}

			hash_value_t get_key() const
			{
				return lookup_key;
			}

			const Running_jobs<Time>& get_running_jobs() const
			{
				return running_jobs;
			}

			Running_jobs<Time>& get_running_jobs()
			{
				return running_jobs;
			}

			const Job_set& get_scheduled_jobs() const
			{
				return scheduled_jobs;
			}

			friend std::ostream& operator<< (std::ostream& stream,
			                                 const Schedule_state<Time>& s)
			{
				stream << "Global::State([";
				for (const auto& rj : s.get_running_jobs())
					stream << rj;
				stream << "], " << s.get_scheduled_jobs() << ")";
				return stream;
			}

			private:

			// set of jobs that have been dispatched (may still be running)
			const Job_set scheduled_jobs;
			// set of jobs (possibly) running in this state
			Running_jobs<Time> running_jobs;

			const hash_value_t lookup_key;

			// no accidental copies
			Schedule_state(const Schedule_state& origin)  = delete;


		};

	}
}
