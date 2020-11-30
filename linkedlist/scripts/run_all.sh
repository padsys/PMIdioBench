#!/bin/bash

# Configuration parameters
export PMEM_DIR="/mnt/pmem1"

# Get ADR throughput results
make clean
sleep 2
make
sleep 2
echo "ADR Throughput Results"
scripts/run.sh
sleep 5
TOOL="flame-graph" scripts/run.sh > _temp
sleep 5
rm -f _temp

# Get eADR throughput results
make clean
sleep 2
make EADR_AVAILABLE=y
sleep 2
echo "eADR Throughput Results"
scripts/run.sh
sleep 5

# Get ADR latency results
make clean
sleep 2
make ENABLE_TIMER=y
sleep 2
echo "ADR Latency Results"
scripts/run.sh
sleep 5
mv output.log adr-latency.log

# Get eADR latency results
make clean
sleep 2
make EADR_AVAILABLE=y ENABLE_TIMER=y
sleep 2
echo "eADR Latency Results"
scripts/run.sh
sleep 5
mv output.log eadr-latency.log
