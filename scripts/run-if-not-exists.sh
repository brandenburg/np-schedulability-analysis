#!/bin/bash

OUTPUT_FILE="$1"
shift
CMD="$*"

set -o noclobber
# atomically create file if it does not yet exist
if [ ! -e "$OUTPUT_FILE" ] && { > "$OUTPUT_FILE" ; } &> /dev/null
then
    echo "[ii] Running '$CMD > $OUTPUT_FILE'..."
    set +o noclobber
    exec $CMD > $OUTPUT_FILE
else
    echo "[ii] Skipping '$CMD'; '$OUTPUT_FILE' exists."
fi
