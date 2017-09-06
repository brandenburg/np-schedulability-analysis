#include <iostream>
#include <sstream>
#include <fstream>

#include "OptionParser.h"

#include "schedule_space.hpp"
#include "io.hpp"
#include "clock.hpp"

// command line options
static bool want_naive;
static bool want_dense;
#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
static bool want_dot_graph;
#endif


struct Analysis_result {
	bool schedulable;
	unsigned long number_of_states;
	double cpu_time;
	std::string graph;
};

static Analysis_result process_stream(std::istream &in)
{
	if (want_dense) {
		auto c = Processor_clock();
		auto jobs = NP::parse_file<dense_t>(in);

		c.start();
		auto space = want_naive ?
			NP::Uniproc::State_space<dense_t>::explore_naively(jobs):
			NP::Uniproc::State_space<dense_t>::explore(jobs);
		c.stop();

		auto graph = std::ostringstream();
#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
		if (want_dot_graph)
			graph << space;
#endif

		return {space.is_schedulable(), space.number_of_states(), c, graph.str()};

	} else {

		auto c = Processor_clock();
		auto jobs = NP::parse_file<dtime_t>(in);

		c.start();
		auto space = want_naive ?
			NP::Uniproc::State_space<dtime_t>::explore_naively(jobs):
			NP::Uniproc::State_space<dtime_t>::explore(jobs);
		c.stop();

		auto graph = std::ostringstream();
#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
		if (want_dot_graph)
			graph << space;
#endif

		return {space.is_schedulable(), space.number_of_states(), c, graph.str()};
	}
}

static void process_file(const std::string& fname)
{
	try {
		Analysis_result result;

		if (fname == "-")
			result = process_stream(std::cin);
		else {
			auto in = std::ifstream(fname, std::ios::in);
			result = process_stream(in);
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

		std::cout << fname
		          << ",  " << (int) result.schedulable
		          << ",  " << result.number_of_states
		          << ",  " << result.cpu_time
		          << std::endl;
	} catch (std::ios_base::failure& ex) {
		std::cerr << fname << ": parse error" << std::endl;
	}
}

int main(int argc, char** argv)
{
	auto parser = optparse::OptionParser();

	parser.description("Exact NP Schedulability Tester");
	parser.usage("usage: %prog [OPTIONS]... [JOB SET FILES]...");

	parser.add_option("-t", "--time").dest("time_model").set_default("discrete")
	      .metavar("TIME-MODEL").choices({"dense", "discrete"})
	      .help("choose 'discrete' or 'dense' time (default: discrete)");

	parser.add_option("-n", "--naive").dest("naive").set_default("0")
	      .action("store_const").set_const("1")
	      .help("use the naive exploration method (default: merging)");

#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
	parser.add_option("-g", "--save-graph").dest("dot").set_default("0")
	      .action("store_const").set_const("1")
	      .help("store the state graph in Graphviz dot format (default: off)");
#endif

	auto options = parser.parse_args(argc, argv);

	const std::string& time_model = options.get("time_model");
	want_dense = time_model == "dense";
	want_naive = options.get("naive");
#ifdef CONFIG_COLLECT_SCHEDULE_GRAPH
	want_dot_graph = options.get("dot");
#endif

	for (auto f : parser.args())
		process_file(f);

	if (parser.args().empty())
		process_file("-");

	return 0;
}
