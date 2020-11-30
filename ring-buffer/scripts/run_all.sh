#!/bin/bash
### Run all RB queue tests and print consolidated results.
### Usage: ./run_all.sh

# Configuration parameters
export PMEM_DIR="/mnt/pmem1" # PMEM directory
export PMIDIOBENCH_HOME="../Bench" # PMIdioBench install dir

SLOT_SIZE=$(grep SLOT_SIZE include/config.h | awk '{ print $3 }')

# Run push test
make clean
sleep 2
make TIME_PUSH=y
sleep 2
scripts/run_tests.sh PUSH $SLOT_SIZE
sleep 5

# Run pop test
make clean
sleep 2
make TIME_POP=y
sleep 2
scripts/run_tests.sh POP $SLOT_SIZE
sleep 5

# Get PMEM counters
make clean
sleep 2
make
sleep 2
GET_STATS="false" TOOL="ipmw" scripts/run_tests.sh
sleep 5
