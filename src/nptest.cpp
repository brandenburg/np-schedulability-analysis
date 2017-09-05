#include <iostream>
#include <fstream>



#include "OptionParser.h"

#include "schedule_space.hpp"
#include "io.hpp"
#include "clock.hpp"

struct Analysis_result {
	bool schedulable;
	unsigned long number_of_states;
	double cpu_time;
};

static Analysis_result process_stream(std::istream &in, bool want_dense)
{
	if (want_dense) {
		auto c = Processor_clock();
		auto jobs = NP::parse_file<dense_t>(in);
		c.start();
		auto space = NP::Uniproc::State_space<dense_t>::explore(jobs);
		c.stop();
		return {space.is_schedulable(), space.number_of_states(), c};
	} else {
		auto c = Processor_clock();
		auto jobs = NP::parse_file<dtime_t>(in);
		c.start();
		auto space = NP::Uniproc::State_space<dtime_t>::explore(jobs);
		c.stop();
		return {space.is_schedulable(), space.number_of_states(), c};
	}
}

static void process_file(const std::string& fname, bool want_dense)
{
	try {
		Analysis_result result;

		if (fname == "-")
			result = process_stream(std::cin, want_dense);
		else {
			auto in = std::ifstream(fname, std::ios::in);
			result = process_stream(in, want_dense);
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

	auto options = parser.parse_args(argc, argv);

	const std::string& time_model = options.get("time_model");
	bool use_dense_time = time_model == "dense";

	for (auto f : parser.args())
		process_file(f, use_dense_time);

	if (parser.args().empty())
		process_file("-", use_dense_time);

	return 0;
}
