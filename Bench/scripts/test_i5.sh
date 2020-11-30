#!/bin/bash
### Idiosyncrasy Test 5
### Store bandwidth lower than p-store bandwidth

source scripts/config.sh

echo ""
echo "$0 $@"

rm -rf /dev/shm/*
echo "DRAM store b/w"
PMEM_DIR="/dev/shm" WORKLOADS="SeqW RandW" PMEM_NO_MOVNT=1 PMEM_NO_FLUSH=1 scripts/run_fio.sh i5-dram-store
sleep 2
WORKLOADS="SeqW RandW" scripts/get_fio_results.sh i5-dram-store
sleep 2
echo "DRAM store+clwb b/w"
PMEM_DIR="/dev/shm" WORKLOADS="SeqW RandW" PMEM_NO_MOVNT=1 scripts/run_fio.sh i5-dram-pstore
sleep 2
WORKLOADS="SeqW RandW" scripts/get_fio_results.sh i5-dram-pstore
sleep 2

echo "PMEM store b/w"
PMEM_DIR=$PMEM_DIR1 WORKLOADS="SeqW RandW" PMEM_NO_MOVNT=1 PMEM_NO_FLUSH=1 scripts/run_fio.sh i5-pmem-store
sleep 2
WORKLOADS="SeqW RandW" scripts/get_fio_results.sh i5-pmem-store
sleep 2
echo "PMEM store+clwb b/w"
PMEM_DIR=$PMEM_DIR1 WORKLOADS="SeqW RandW" PMEM_NO_MOVNT=1 scripts/run_fio.sh i5-pmem-pstore
sleep 2
WORKLOADS="SeqW RandW" scripts/get_fio_results.sh i5-pmem-pstore
sleep 2
