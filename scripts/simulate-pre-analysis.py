#!/usr/bin/env python3

import argparse
import csv
import random
import tempfile
import os
import subprocess
import sys


JOBSET_COLS = [
    "Task ID",
    "Job ID",
    "Arrival min",
    "Arrival max",
    "Cost min",
    "Cost max",
    "Deadline",
    "Priority"
]

ACTIONS_COLS = [
    'TID',
    'JID',
    'Tmin',
    'Tmax',
    'Cmin',
    'Cmax'
]

ANALYSIS_COLS = [
    "Task ID",
    "Job ID",
    "BCCT",
    "WCCT",
    "BCRT",
    "WCRT"
]

def load_job_set(jobs_file):
    rows = csv.DictReader(jobs_file, fieldnames=JOBSET_COLS,
                          skipinitialspace=True)
    next(rows) # skip header
    return list(rows)

def load_actions(actions_file):
    rows = csv.DictReader(actions_file, fieldnames=ACTIONS_COLS,
                          skipinitialspace=True)
    next(rows) # skip header
    return list(rows)

def mkcopy(list_of_things):
    if list_of_things is None:
        return None
    else:
        return [x.copy() for x in list_of_things]

def fix_max_cost(jobs):
    for j in jobs:
        j['Cost min'] = j['Cost max']

def fix_min_cost(jobs):
    for j in jobs:
        j['Cost max'] = j['Cost min']

def fix_min_arrival(jobs):
    for j in jobs:
        j['Arrival max'] = j['Arrival min']

def fix_max_arrival(jobs):
    for j in jobs:
        j['Arrival min'] = j['Arrival max']

def fix_random_arrival(jobs):
    for j in jobs:
        arr = random.randint(int(j['Arrival min']), int(j['Arrival max']))
        j['Arrival min'] = j['Arrival max'] = arr

def fix_max_abort_cost(actions):
    if actions:
        for a in actions:
            a['Cmin'] = a['Cmax']

def fix_min_abort_cost(actions):
    if actions:
        for a in actions:
            a['Cmax'] = a['Cmin']

def fix_max_abort_jitter(actions):
    if actions:
        for a in actions:
            a['Tmin'] = a['Tmax']

def fix_min_abort_jitter(actions):
    if actions:
        for a in actions:
            a['Tmax'] = a['Tmin']

def fix_random_abort_jitter(actions):
    if actions:
        for a in actions:
            arr = random.randint(int(a['Tmin']), int(a['Tmax']))
            a['Tmax'] = a['Tmin'] = arr

def create_variants(orig_jobs, orig_actions, num_random_arrivals=10):
    variants = []

    # (1) no jitter, max WCET
    jobs = mkcopy(orig_jobs)
    acts = mkcopy(orig_actions)
    fix_max_cost(jobs)
    fix_max_abort_cost(acts)
    fix_min_arrival(jobs)
    fix_min_abort_jitter(acts)
    variants.append((jobs, acts))

    # (2) max jitter, max WCET
    jobs = mkcopy(orig_jobs)
    acts = mkcopy(orig_actions)
    fix_max_cost(jobs)
    fix_max_abort_cost(acts)
    fix_max_arrival(jobs)
    fix_max_abort_jitter(acts)
    variants.append((jobs, acts))

    # (3) max jitter, min WCET
    jobs = mkcopy(orig_jobs)
    acts = mkcopy(orig_actions)
    fix_min_cost(jobs)
    fix_min_abort_cost(acts)
    fix_max_arrival(jobs)
    fix_max_abort_jitter(acts)
    variants.append((jobs, acts))

    # (4) min jitter, min WCET
    jobs = mkcopy(orig_jobs)
    acts = mkcopy(orig_actions)
    fix_min_cost(jobs)
    fix_min_abort_cost(acts)
    fix_min_arrival(jobs)
    fix_min_abort_jitter(acts)
    variants.append((jobs, acts))

    # finally, a bunch of random arrivals with max WCET
    for i in range(num_random_arrivals):
        jobs = mkcopy(orig_jobs)
        acts = mkcopy(orig_actions)
        fix_max_cost(jobs)
        fix_max_abort_cost(acts)
        fix_random_arrival(jobs)
        fix_random_abort_jitter(acts)
        variants.append((jobs, acts))

    return variants

debug_invocations = False

def run_analysis(nptest, jobset_name, actions_name, extra_args):
    args = [nptest, jobset_name, '-r']
    if actions_name:
        args += ['-a', actions_name]
    args += extra_args
    if debug_invocations:
        print("Invoking:", *args, file=sys.stderr)
    return subprocess.Popen(args, stdout=subprocess.DEVNULL)

def load_analysis_results(dir, idx):
    fname = os.path.join(dir, 'v%02d_jobs.rta.csv' % idx)
    rows = csv.DictReader(open(fname, 'r'), fieldnames=ANALYSIS_COLS,
                          skipinitialspace=True)
    next(rows) # skip header
    return list(rows)

def simulate(idx, dir, jobset, actions, nptest, extra_args):
    # (1) write job set
    jobset_name = os.path.join(dir, 'v%02d_jobs.csv' % idx)
    f = open(jobset_name, 'w')
    out = csv.DictWriter(f, fieldnames=JOBSET_COLS)
    out.writeheader()
    out.writerows(jobset)
    del out
    del f

    # (2) write actions
    if actions:
        actions_name = os.path.join(dir, 'v%02d_actions.csv' % idx)
        f = open(actions_name, 'w')
        out = csv.DictWriter(f, fieldnames=ACTIONS_COLS)
        out.writeheader()
        out.writerows(actions)
        del out
        del f
    else:
        actions_name = None

    # invoke schedulability analysis
    return run_analysis(nptest, jobset_name, actions_name, extra_args)

def simulate_variants(orig_jobs, variants, nptest, timeout, extra_args):
#     dir = '/tmp/debug'
#     if True:
   with tempfile.TemporaryDirectory() as dir:
        simulations = []
        for i, jobs_acts in enumerate(variants):
            jobs, acts = jobs_acts
            idx = i + 1
            simulations.append(simulate(idx, dir, jobs, acts, nptest,
                                        extra_args))

        for sim in simulations:
            sim.wait(timeout)
            assert sim.returncode is 0

        results = load_analysis_results(dir, 1)
        for i in range(1, len(variants)):
            for overall, new in zip(results, load_analysis_results(dir, i + 1)):
                if int(overall['BCCT']) > int(new['BCCT']):
                    overall['BCCT'] = new['BCCT']
                if int(overall['WCCT']) < int(new['WCCT']):
                    overall['WCCT'] = new['WCCT']

        # compute correct response times
        for job, times in zip(orig_jobs, results):
            assert job['Task ID'] == times['Task ID']
            assert job['Job ID'] == times['Job ID']
            times['BCRT'] = int(times['BCCT']) - int(job['Arrival min'])
            times['WCRT'] = int(times['WCCT']) - int(job['Arrival min'])
            assert times['BCRT'] >= 0
            assert times['WCRT'] >= 0
            assert times['WCRT'] >= times['BCRT']

        return results


def parse_args():
    parser = argparse.ArgumentParser(
        description="simulate a few specific execution scenarios")

    parser.add_argument('extra_args', nargs='*',
        metavar='extra argument',
        help="arguments passed through to nptest "
             "(note: pass after '--' to separate from arguments for this script)")

    parser.add_argument('--jobs', default=None,
                        type=argparse.FileType('r'), metavar="FILE",
                        required=True,
                        help='file holding the job set')

    parser.add_argument('--actions', default=None,
                        type=argparse.FileType('r'), metavar="FILE",
                        help='file holding the actions')

    parser.add_argument('-n', '--num-random-releases', default=100,
                        action='store', type=int,
                        help='how many random releases to try (default: 100)')

    parser.add_argument('-t', '--timeout', default=100,
                        action='store', type=int,
                        help='how long may a simulation take (in seconds, '
                             'default: 60s)')

    parser.add_argument('--nptest', default='nptest',
                        action='store',
                        metavar='BINARY',
                        help='path to the nptest binary')

    parser.add_argument('-o', '--output', default=sys.stdout,
                        type=argparse.FileType('w'), metavar="FILE",
                        help='where to store the simulation results')

    parser.add_argument('-d', '--debug', default=False, action='store_true',
                        help='show nptest command invocations')

    return parser.parse_args()

def main():
    opts = parse_args()

    jobs = load_job_set(opts.jobs)
    acts = load_actions(opts.actions) if opts.actions else None

    global debug_invocations
    debug_invocations = opts.debug

    variants = create_variants(jobs, acts, opts.num_random_releases)
    results = simulate_variants(jobs, variants, opts.nptest, opts.timeout,
                                opts.extra_args)

    out = csv.DictWriter(opts.output, ANALYSIS_COLS)
    out.writeheader()
    out.writerows(results)


if __name__ == '__main__':
    main()

