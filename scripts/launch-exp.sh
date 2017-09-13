#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

LAST=""
CMD=""
while ! [ -z "$1" ]
do
    CMD="$CMD $LAST"
    LAST=$1
    shift
done

INPUT_FILE="$LAST"

NAME=`basename "$INPUT_FILE"`
OUTPUT_FILE=${NAME/.csv/.out}

$SCRIPT_DIR/run-if-not-exists.sh $OUTPUT_FILE $CMD $INPUT_FILE
