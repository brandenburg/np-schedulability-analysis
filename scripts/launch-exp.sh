#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

INPUT_FILE="$1"
shift
CMD="$*"

NAME=`basename "$INPUT_FILE"`
OUTPUT_FILE=${NAME/.csv/.out}

$SCRIPT_DIR/run-if-not-exists.sh $OUTPUT_FILE $CMD $INPUT_FILE
