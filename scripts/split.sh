#!/bin/bash

WORKLOAD=$1

NAME=`basename $WORKLOAD`
COUNT=`wc -l $WORKLOAD | awk '{print $1}'`
for l in $(seq 2  $COUNT)
do 
    NUM_JOBS=$((l - 1))
    echo $WORKLOAD '->' ${NAME/.csv/}_split=$(printf '%04d' $NUM_JOBS).csv
    head -n $l $WORKLOAD > ${NAME/.csv/}_split=$(printf '%04d' $NUM_JOBS).csv
done
