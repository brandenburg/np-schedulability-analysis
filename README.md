# NP Schedulability Test

This repository contains the implementation of the **exact and sustainable schedulability test** for sets of **non-preemptive jobs** scheduled upon a **uniprocessor** presented in the paper

- M. Nasri and B. Brandenburg, “[An Exact and Sustainable Analysis of Non-Preemptive Scheduling](https://people.mpi-sws.org/~bbb/papers/pdf/rtss17.pdf)”, *Proceedings of the 38th IEEE Real-Time Systems Symposium (RTSS 2017)*, December 2017.


## Dependencies

- A modern C++ compiler supporting the **C++14 standard**. Recent versions of `clang` and `g++` on Linux and macOS are known to work. 

- The [CMake](https://cmake.org) build system.

- A POSIX OS. Linux and macOS are both known to work.

## Build Instructions

These instructions assume a Linux or macOS host.

To compile the tool, first generate an appropriate `Makefile` with `cmake` and then use it to actually build the source tree.

	# (1) create a build directory
	make build
	# (2) enter the build directory
	cd build
	# (3) generate the Makefile
	cmake ..
	# (4) build everything
	make -j

The last step yields two binaries:

- `nptest`, the actually schedulability analysis tool, and
- `runtests`, the unit test suite (described next). 

## Unit Tests

The tool comes with a test driver (based on [C++ doctest](https://github.com/onqtam/doctest)) named `runtests`. After compiling everything, just run the tool to make sure everything works. 

```
$ ./runtests 
[doctest] doctest version is "1.2.1"
[doctest] run with "--help" for options
===============================================================================
[doctest] test cases:     27 |     27 passed |      0 failed |      0 skipped
[doctest] assertions:    260 |    260 passed |      0 failed |
[doctest] Status: SUCCESS!
```


## Input Format

The tool operates on CSV files with a fixed column order. Each input CSV file is expected to contain a set of jobs, where each row specifies one job. The following columns are expected.

1.   **Task ID** — an arbitrary numeric ID to identify the task to which a job belongs
2.   **Job ID** — a unique numeric ID that identifies the job
3.   **Arrival min** — the earliest-possible release time of the job
4.   **Arrival max** — the latest-possible release time of the job
5.   **Cost min** — the best-case execution time of the job (can be zero)
6.   **Cost max** — the worst-case execution time of the job
7.   **Deadline** — the absolute deadline of the job
8.   **Priority** — the priority of the job (EDF: set it equal to the deadline)

All numeric parameters can be 64-bit integers (preferred) or floating point values (slower, not recommended). 

For example input files, see the files in the `examples/` folder (e.g., [examples/fig1a.csv](examples/fig1a.csv)).

## Analyzing a Job Set

To run the tool on a given job set, just pass the filename as an argument. For example:

```
$ build/nptest examples/fig1a.csv 
examples/fig1a.csv,  0,  9,  6,  5,  1,  0.000329,  820.000000,  0
```

The output format is explained below.  

If no input files are provided, or if `-` is provided as an input file name, `nptest` will read the job set description from standard input, which allows applying filters etc. For example:

```
$ cat examples/fig1a.csv | grep -v '3, 9' | build/nptest 
-,  1,  8,  9,  8,  0,  0.000069,  816.000000,  0
```

To activate an *idle-time insertion policy* (IIP, see paper for details), use the `-i` option. For example, to use the CW-EDF IIP, pass the `-i CW` option:

```
$ build/nptest -i CW examples/fig1a.csv 
examples/fig1a.csv,  1,  9,  10,  9,  0,  0.000121,  848.000000,  0, 1
```

When analyzing a job set with **dense-time parameters** (i.e., time values specified as floating-point numbers), the option `-t dense` **must** be passed. 

See the builtin help (`nptest -h`) for further options.

## Output Format

The output is provided in CSV format and consists of the following columns:

1. The input file name.
2. The schedulability result: 1 if the job is *is* schedulable, 0 if it is *not*.
3. The number of jobs in the job set.
4. The number of states that were explored.
5. The number of edges that were discovered. 
6. The maximum “exploration front width” of the schedule graph, which is the maximum number of unprocessed states  that are queued for exploration (at any point in time). 
7. The CPU time used in the analysis (in seconds).
8. The peak amount of memory used (as reported by `getrusage()`), divided by 1024. Due to non-portable differences in `getrusage()`, on Linux this reports the memory usage in megabytes, whereas on macOS it reports the memory usage in kilobytes.
9. A timeout indicator: 1 if the state-space exploration was aborted due to reaching the time limit (as set with the `-l` option); 0 otherwise. 
10. The number of processors assumed during the analysis. 

## Questions, Patches, or Suggestions

In case of questions, please contact [Björn Brandenburg](https://people.mpi-sws.org/~bbb/) (bbb at  mpi-sws dot org).

Patches and feedback welcome — please send a pull request or open a ticket. 


