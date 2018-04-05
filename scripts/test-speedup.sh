#!/bin/bash

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

NUM_CPUS=`getconf _NPROCESSORS_ONLN`

echo "#threads, real, user, sys" > speedup.csv
for N in `seq -w 1 ${NUM_CPUS}`
do 
    echo "====================[ $N ]===================="
    
    (time $SCRIPT_DIR/../build/nptest --threads $N $* 2> speedup_threads=$N.log ) 2> time_threads=$N.log
    
    REAL_TIME=$(grep real time_threads=$N.log | sed 's/^[^0-9]*//')
    USER_TIME=$(grep user time_threads=$N.log | sed 's/^[^0-9]*//')
    SYS_TIME=$(grep sys time_threads=$N.log | sed 's/^[^0-9]*//')    
    echo "$N, $REAL_TIME, $USER_TIME, $SYS_TIME" >> speedup.csv
    cat time_threads=$N.log
done
