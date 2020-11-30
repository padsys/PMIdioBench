#!/bin/bash
### Idiosyncrasy Test 4
### Poor p-store bandwidth on remote NUMA PMEM

source scripts/config.sh

echo ""
echo "$0 $@"

rm -rf /dev/shm/*
echo "DRAM local NUMA b/w"
PMEM_DIR="/dev/shm" WORKLOADS="SeqW RandW" PMEM_NO_MOVNT=1 scripts/run_fio.sh i4-dram-local
sleep 2
WORKLOADS="SeqW RandW" scripts/get_fio_results.sh i4-dram-local
sleep 2
echo "DRAM remote NUMA b/w"
NUMA_NODE=1 PMEM_DIR="/dev/shm" WORKLOADS="SeqW RandW" PMEM_NO_MOVNT=1 scripts/run_fio.sh i4-dram-remote
sleep 2
WORKLOADS="SeqW RandW" scripts/get_fio_results.sh i4-dram-remote
sleep 2

echo "PMEM local NUMA b/w"
PMEM_DIR=$PMEM_DIR1 WORKLOADS="SeqW RandW" PMEM_NO_MOVNT=1 scripts/run_fio.sh i4-pmem-local
sleep 2
WORKLOADS="SeqW RandW" scripts/get_fio_results.sh i4-pmem-local
sleep 2
echo "PMEM remote NUMA b/w"
NUMA_NODE=1 PMEM_DIR=$PMEM_DIR1 WORKLOADS="SeqW RandW" PMEM_NO_MOVNT=1 scripts/run_fio.sh i4-pmem-remote
sleep 2
WORKLOADS="SeqW RandW" scripts/get_fio_results.sh i4-pmem-remote
sleep 2
