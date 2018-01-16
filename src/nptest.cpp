#include <iostream>
#include <sstream>
#include <fstream>

 #include <sys/resource.h>

#include "OptionParser.h"

#include "schedule_space.hpp"
#include "io.hpp"
#include "clock.hpp"

// command line options
static bool want_naive;
static bool want_dense;
static bool want_prm_iip;
static bool want_cw_iip;

static bool want_precedence = false;
static std::string precedence_file;

#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
static bool want_dot_graph;
#endif
static double timeout;

struct Analysis_result {
	bool schedulable;
	bool timeout;
	unsigned long number_of_states, number_of_edges, max_width, number_of_jobs;
	double cpu_time;
	std::string graph;
};

template<class Time, class Space>
static Analysis_result analyze(std::istream &in, std::istream &dag_in)
{
	auto jobs = NP::parse_file<Time>(in);
	auto dag = NP::parse_dag_file(dag_in);

	NP::validate_prec_refs<Time>(dag, jobs);

	auto space = want_naive ?
		Space::explore_naively(jobs, timeout, jobs.size(), dag) :
		Space::explore(jobs, timeout, jobs.size(), dag);

	auto graph = std::ostringstream();
#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
	if (want_dot_graph)
		graph << space;
#endif

	return {
		space.is_schedulable(),
		space.was_timed_out(),
		space.number_of_states(),
		space.number_of_edges(),
		space.max_exploration_front_width(),
		jobs.size(),
		space.get_cpu_time(),
		graph.str()
	};
}

static Analysis_result process_stream(std::istream &in, std::istream &dag_in)
{
	if (want_dense && want_prm_iip)
		return analyze<dense_t, NP::Uniproc::State_space<dense_t, NP::Uniproc::Precatious_RM_IIP<dense_t>>>(in, dag_in);
	else if (want_dense && want_cw_iip)
		return analyze<dense_t, NP::Uniproc::State_space<dense_t, NP::Uniproc::Critical_window_IIP<dense_t>>>(in, dag_in);
	else if (want_dense && !want_prm_iip)
		return analyze<dense_t, NP::Uniproc::State_space<dense_t>>(in, dag_in);
	else if (!want_dense && want_prm_iip)
		return analyze<dtime_t, NP::Uniproc::State_space<dtime_t, NP::Uniproc::Precatious_RM_IIP<dtime_t>>>(in, dag_in);
	else if (!want_dense && want_cw_iip)
		return analyze<dtime_t, NP::Uniproc::State_space<dtime_t, NP::Uniproc::Critical_window_IIP<dtime_t>>>(in, dag_in);
	else
		return analyze<dtime_t, NP::Uniproc::State_space<dtime_t>>(in, dag_in);
}

static void process_file(const std::string& fname,
                         const std::string& prec_dag_fname)
{
	try {
		Analysis_result result;

		auto empty_dag_stream = std::istringstream("\n");
		auto dag_stream = std::ifstream();

		if (want_precedence)
			dag_stream.open(prec_dag_fname);

		std::istream &dag_in = want_precedence ?
			static_cast<std::istream&>(dag_stream) :
			static_cast<std::istream&>(empty_dag_stream);

		if (fname == "-")
			result = process_stream(std::cin, dag_in);
		else {
			auto in = std::ifstream(fname, std::ios::in);
			result = process_stream(in, dag_in);
#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
			if (want_dot_graph) {
				std::string dot_name = fname;
				auto p = dot_name.find(".csv");
				if (p != std::string::npos) {
					dot_name.replace(p, std::string::npos, ".dot");
					auto out  = std::ofstream(dot_name,  std::ios::out);
					out << result.graph;
					out.close();
				}
			}
#endif
		}

		struct rusage u;
		long mem_used = 0;
		if (getrusage(RUSAGE_SELF, &u) == 0)
			mem_used = u.ru_maxrss;

		std::cout << fname
		          << ",  " << (int) result.schedulable
		          << ",  " << result.number_of_jobs
		          << ",  " << result.number_of_states
		          << ",  " << result.number_of_edges
		          << ",  " << result.max_width
		          << ",  " << std::fixed << result.cpu_time
		          << ",  " << ((double) mem_used) / (1024.0)
		          << ",  " << (int) result.timeout
		          << std::endl;
	} catch (std::ios_base::failure& ex) {
		std::cerr << fname;
		if (want_precedence)
			std::cerr << " + " << prec_dag_fname;
		std::cerr <<  ": parse error" << std::endl;
	} catch (NP::InvalidJobReference& ex) {
		std::cerr << prec_dag_fname << ": bad job reference: job "
		          << ex.ref.job << " of task " << ex.ref.task
			      << " is not part of the job set given in "
			      << fname
			      << std::endl;
	} catch (std::exception& ex) {
		std::cerr << fname << ": '" << ex.what() << "'" << std::endl;
	}
}

int main(int argc, char** argv)
{
	auto parser = optparse::OptionParser();

	parser.description("Exact NP Schedulability Tester");
	parser.usage("usage: %prog [OPTIONS]... [JOB SET FILES]...");

	parser.add_option("-t", "--time").dest("time_model")
	      .metavar("TIME-MODEL")
	      .choices({"dense", "discrete"}).set_default("discrete")
	      .help("choose 'discrete' or 'dense' time (default: discrete)");

	parser.add_option("-l", "--time-limit").dest("timeout")
	      .help("maximum CPU time allowed (in seconds, zero means none)")
	      .set_default("0");

	parser.add_option("-n", "--naive").dest("naive").set_default("0")
	      .action("store_const").set_const("1")
	      .help("use the naive exploration method (default: merging)");

	parser.add_option("-i", "--iip").dest("iip")
	      .choices({"none", "P-RM", "CW"}).set_default("none")
	      .help("the IIP to use (default: none)");

	parser.add_option("-p", "--precedence").dest("precedence_file")
	      .help("name of the file that contains the job set's precedence DAG")
	      .set_default("");

#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
	parser.add_option("-g", "--save-graph").dest("dot").set_default("0")
	      .action("store_const").set_const("1")
	      .help("store the state graph in Graphviz dot format (default: off)");
#endif

	auto options = parser.parse_args(argc, argv);

	const std::string& time_model = options.get("time_model");
	want_dense = time_model == "dense";

	const std::string& iip = options.get("iip");
	want_prm_iip = iip == "P-RM";
	want_cw_iip = iip == "CW";

	want_naive = options.get("naive");

	timeout = options.get("timeout");

	want_precedence = options.is_set_by_user("precedence_file");
	if (want_precedence && parser.args().size() > 1) {
		std::cerr << "[!!] Warning: multiple job sets "
		          << "with a single precedence DAG specified."
		          << std::endl;
	}

#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
	want_dot_graph = options.get("dot");
#endif

	for (auto f : parser.args())
		process_file(f, options.get("precedence_file"));

	if (parser.args().empty())
		process_file("-", options.get("precedence_file"));

	return 0;
}
