#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

POLICY="RM"

LAST=""
CMD=""
while ! [ -z "$1" ]
do
    if [ "$1" == "--policy" ]
    then
        POLICY=$2
        shift
        shift
    else
        CMD="$CMD $LAST"
        LAST=$1
        shift
    fi
done

INPUT_FILE="$LAST"

NAME=`basename $INPUT_FILE`

JOBS="/tmp/jobs_$NAME"
EDGES="/tmp/edges_$NAME"

$SCRIPT_DIR/dag-tasks-to-jobs.py -p $POLICY $INPUT_FILE $JOBS $EDGES || exit 1

$SCRIPT_DIR/../build/nptest $CMD -p $EDGES $JOBS || exit 2

rm $JOBS $EDGES
