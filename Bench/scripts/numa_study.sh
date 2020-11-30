#!/bin/bash

echo "NUMA Study"
echo ""
echo "Throughput and Latency Results"
scripts/run_mongodb_ycsb.sh
sleep 2
echo ""
echo "Counter Results (ignore the throughput and latency results)"
TOOL="ipmw" scripts/run_mongodb_ycsb.sh
sleep 2
