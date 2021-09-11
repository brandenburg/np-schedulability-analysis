# NP Schedulability Test

This repository contains the implementations of schedulability tests for **sets of non-preemptive jobs** with **precedence constraints** scheduled on either **uniprocessors** or **globally scheduled identical multiprocessors**. The analyses are described in the following papers:

- M. Nasri and B. Brandenburg, “[An Exact and Sustainable Analysis of Non-Preemptive Scheduling](https://people.mpi-sws.org/~bbb/papers/pdf/rtss17.pdf)”, *Proceedings of the 38th IEEE Real-Time Systems Symposium (RTSS 2017)*, pp. 12–23, December 2017.
- M. Nasri, G. Nelissen, and B. Brandenburg, “[Response-Time Analysis of Limited-Preemptive Parallel DAG Tasks under Global Scheduling](http://drops.dagstuhl.de/opus/volltexte/2019/10758/pdf/LIPIcs-ECRTS-2019-21.pdf)”, *Proceedings of the 31st Euromicro Conference on Real-Time Systems (ECRTS 2019)*, pp. 21:1–21:23, July 2019.

An [earlier version of this tool](https://github.com/brandenburg/np-schedulability-analysis/releases/tag/ECRTS18-last) (i.e., up to tag [`ECRTS18-last`](https://github.com/brandenburg/np-schedulability-analysis/releases/tag/ECRTS18-last)) implemented the analysis for independent jobs on globally scheduled multiprocessors presented at ECRTS'18.

- M. Nasri, G. Nelissen, and B. Brandenburg,  
“[A Response-Time Analysis for Non-Preemptive Job Sets under Global Scheduling](http://drops.dagstuhl.de/opus/volltexte/2018/8994/pdf/LIPIcs-ECRTS-2018-9.pdf)”,  
*Proceedings of the 30th Euromicro Conference on Real-Time Systems (ECRTS 2018)*, pp. 9:1–9:23, July 2018.


The uniprocessor analysis (Nasri & Brandenburg, 2017) is exact (in the absence of precedence constraints); the multiprocessor analyses (Nasri et al., 2018, 2019) are not. 

## Dependencies

- A modern C++ compiler supporting the **C++14 standard**. Recent versions of `clang` and `g++` on Linux and macOS are known to work. 

- The [CMake](https://cmake.org) build system.

- A POSIX OS. Linux and macOS are both known to work.

- The [Intel Thread Building Blocks (TBB)](https://www.threadingbuildingblocks.org) library and parallel runtime. 

- The [jemalloc](http://jemalloc.net) scalable memory allocator. Alternatively, the TBB allocator can be used instead; see build options below.

## Build Instructions

These instructions assume a Linux or macOS host.

To compile the tool, first generate an appropriate `Makefile` with `cmake` and then use it to actually build the source tree.

	# (1) enter the build directory
	cd build
	# (2) generate the Makefile
	cmake ..
	# (3) build everything
	make -j

The last step yields two binaries:

- `nptest`, the actually schedulability analysis tool, and
- `runtests`, the unit-test suite. 

## Build Options

The build can be configured in a number of ways by passing options via the `-D` flag to `cmake` in step (2). 

To enable debug builds, pass the `DEBUG` option to `cmake` .

    cmake -DDEBUG=yes ..

To enable the collection of schedule graphs (the `-g` option in `nptest`), set the option `COLLECT_SCHEDULE_GRAPHS` to `yes`. 

    cmake -DCOLLECT_SCHEDULE_GRAPHS=yes  ..

Note that enabling `COLLECT_SCHEDULE_GRAPHS` turns off parallel analysis, i.e., the analysis becomes single-threaded, so don't turn it on by default. It is primarily a debugging aid. 

By default, `nptest` uses `jemalloc`. To instead use the parallel allocator that comes with Intel TBB, set `USE_JE_MALLOC` to `no` and `USE_TBB_MALLOC` to `yes`.

    cmake -DUSE_JE_MALLOC=no -DUSE_TBB_MALLOC=yes ..

To use the default `libc` allocator (which is a tremendous scalability bottleneck), set both options to `no`.

    cmake -DUSE_JE_MALLOC=no -DUSE_TBB_MALLOC=no ..

## Unit Tests

The tool comes with a test driver (based on [C++ doctest](https://github.com/onqtam/doctest)) named `runtests`. After compiling everything, just run the tool to make sure everything works. 

```
$ ./runtests 
[doctest] doctest version is "1.2.6"
[doctest] run with "--help" for options
===============================================================================
[doctest] test cases:     57 |     57 passed |      0 failed |      0 skipped
[doctest] assertions:    537 |    537 passed |      0 failed |
[doctest] Status: SUCCESS!
```

## Input Format

The tool operates on CSV files with a fixed column order. There are three main input formats: *job sets*, *precedence constraints*, and *abort actions*. 

### Job Sets

Job set input CSV files describe a set of jobs, where each row specifies one job. The following columns are required.

1.   **Task ID** — an arbitrary numeric ID to identify the task to which a job belongs
2.   **Job ID** — a unique numeric ID that identifies the job
3.   **Release min** — the earliest-possible release time of the job (equivalently, this is the arrival time of the job)
4.   **Release max** — the latest-possible release time of the job (equivalently, this is the arrival time plus maximum jitter of the job)
5.   **Cost min** — the best-case execution time of the job (can be zero)
6.   **Cost max** — the worst-case execution time of the job
7.   **Deadline** — the absolute deadline of the job
8.   **Priority** — the priority of the job (EDF: set it equal to the deadline)

All numeric parameters can be 64-bit integers (preferred) or floating point values (slower, not recommended). 

Example job set input files are provided in the `examples/` folder (e.g., [examples/fig1a.csv](examples/fig1a.csv)).

### Precedence Constraints

A precedent constraints CSV files define a DAG on the set of jobs provided in a job set CSV file in a straightforward manner. Each row specifies a forward edge. The following four columns are required.

1. **Predecessor task ID** - the task ID of the source of the edge
2. **Predecessor job ID** - the job ID of the source of the edge
3. **Successor task ID** - the task ID of the target of the edge
4. **Successor job ID** - the job ID of the target of the edge

An example precedent constraints file is provided in the `examples/` folder (e.g., [examples/fig1a.prec.csv](examples/fig1a.prec.csv)).


### Abort Actions

A file containing abort actions lists for (some of) the jobs comprising a workload a *trigger interval* and a *cleanup cost interval*, with the semantics that if a job is still executing during its trigger interval, the runtime system will trigger a cleanup routine that aborts and discards the job. Each row specifies the abort time and cost for one job. The following six columns are required.

1. **Task ID** - the task ID of the job that has an abort action
2. **Job ID** - the job ID of the job that has an abort action
3. **Earliest Trigger Time** - the earliest time the abort action can trigger (= the job’s absolute deadline, usually)
4. **Latest Trigger Time** - the latest time the abort action will trigger if the job is still running (= usually the job’s absolute deadline + any imprecision of the runtime environment in aborting jobs)
5. **Least Cleanup Cost** - the minimum time required by the runtime system to abort the job
6. **Maximum Cleanup Cost** - the maximum time required by the runtime system to abort the job

An example abort actions file is provided in the `examples/` folder (e.g., [examples/abort.actions.csv](examples/abort.actions.csv)).

## Analyzing a Job Set

To run the tool on a given set, just pass the filename as an argument. For example:

```
$ build/nptest examples/fig1a.csv 
examples/fig1a.csv,  0,  9,  6,  5,  1,  0.000329,  820.000000,  0
```

By default, the tool assumes that *jobs are independent* (i.e., there no precedence constraints) and runs the *uniprocessor analysis* (RTSS'17). To specify precedence constraints or to run the (global) multiprocessor analysis (ECRTS'18), the `-p` and  `-m`  options need to be specified, respectively, as discussed below.

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

To use the multiprocessor analysis, use the `-m` option. 

See the builtin help (`nptest -h`) for further options.

### Global Multiprocessor Analysis

To run the analysis for globally scheduled multiprocessors, simply provide the number of (identical) processors via the `-m` option. For example: 

```
$ build/nptest -m 2 examples/fig1a.csv 
examples/fig1a.csv,  1,  9,  9,  9,  1,  0.000379,  1760.000000,  0,  2
```

**NOTE**: While invoking `nptest` with `-m 1` specifies a uniprocessor platform, it is *not* the same as running the uniprocessor analysis. The uniprocessor analysis (RTSS'17) is activated *in the absence* of the `-m` option; providing `-m 1` activates the multiprocessor analysis (ECRTS'18) assuming there is a single processor. 

### Precedence Constraints

To impose precedence constraints on the job set, provide the DAG structure in a separate CSV file via the `-p` option. For example:

```
$ build/nptest examples/fig1a.csv -p examples/fig1a.prec.csv 
examples/fig1a.csv,  1,  9,  10,  9,  0,  0.000135,  1784.000000,  0,  1
```

The tool does not check whether the provided structure actually forms a DAG. If a cycle is (accidentally) introduced, which essentially represents a deadlock as no job that is part of the cycle can be scheduled first, the analysis will simply discover and report that the workload is unschedulable. 

### Aborting Jobs Past a Certain Point

The uniprocessor analysis now also supports so-called **abort actions**, which allow specifying that if a job executes past a certain point in time, it will be forcefully stopped and discarded by the runtime environment. To enable this support, pass a CSV file containing a list of abort actions using the `-a` option. For example:

```
$ build/nptest -g examples/abort.jobs.csv -c -a examples/abort.actions.csv
examples/abort.jobs.csv,  1,  4,  5,  4,  0,  0.000089,  1796.000000,  0,  1
```

**NOTE**: Aborted jobs are considered completed: thus if all tardy jobs are aborted by their deadline, then the tool will report the workload to be schedulable (as in the above example)! This can be observed by omitting the abort actions (`-a` option): 

```
$ build/nptest -g examples/abort.jobs.csv -c
examples/abort.jobs.csv,  0,  4,  5,  4,  0,  0.000088,  1760.000000,  0,  1
```

Without the job abort action specified in [examples/abort.actions.csv](examples/abort.actions.csv), the workload can indeed miss deadlines and is thus unschedulable.

## Output Format

The output is provided in CSV format and consists of the following columns:

1. The input file name.
2. The schedulability result:
    - 1 if the job is *is* schedulable (i.e., the tool could prove the absence of deadline misses),
    - 0 if it is *not*, or if the analysis timed out, if it reached the depth limit, or if the analysis cannot prove the absence of deadline misses (while the RTSS'17 analysis is exact, the ECRTS'19 analysis is only sufficient, but not exact). 
3. The number of jobs in the job set.
4. The number of states that were explored.
5. The number of edges that were discovered. 
6. The maximum “exploration front width” of the schedule graph, which is the maximum number of unprocessed states  that are queued for exploration (at any point in time). 
7. The CPU time used in the analysis (in seconds).
8. The peak amount of memory used (as reported by `getrusage()`), divided by 1024. Due to non-portable differences in `getrusage()`, on Linux this reports the memory usage in megabytes, whereas on macOS it reports the memory usage in kilobytes.
9. A timeout indicator: 1 if the state-space exploration was aborted due to reaching the time limit (as set with the `-l` option); 0 otherwise. 
10. The number of processors assumed during the analysis. 

Pass the `--header` flag to `nptest` to print out column headers. 

## Obtaining Response Times

The analysis computes for each job the earliest and latest possible completion times, from which it is trivial to infer minimum and maximum response times. To obtain this information, pass the `-r` option to `nptest`. 

If invoked on an input file named `foo.csv`, the completion times will be stored in a file `foo.rta.csv` and follow the following format, where each row corresponds to one job in the input job set. 

1. Task ID
2. Job ID
3. BCCT, the best-case completion time
4. WCCT, the worst-case completion time 
5. BCRT, the best-case response time (relative to the minimum release time)
6. WCRT, the worst-case response time (relative to the minimum release time)

Note that the analysis by default aborts after finding the first deadline miss, in which case some of the rows may report nonsensical default values.  To force the analysis to run to completion despite deadline misses, pass the `-c` flag to `nptest`.

## Questions, Patches, or Suggestions

In case of questions, please contact [Geoffrey Nelissen](https://www.tue.nl/en/research/researchers/geoffrey-nelissen/), the current maintainer of the project.

Patches and feedback welcome — please send a pull request or open a ticket. 

## License 

The code is released under a 3-clause BSD license. 


## Credits

The software was originally developed by [Björn Brandenburg](https://people.mpi-sws.org/~bbb/). Joan Marcè and Sayra Ranjha contributed analysis improvements. It is now being maintained by [Geoffrey Nelissen](https://www.tue.nl/en/research/researchers/geoffrey-nelissen/).

When using this software in academic work, please cite the papers listed at the top of this file. 
