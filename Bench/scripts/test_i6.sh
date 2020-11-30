#!/bin/bash
### Idiosyncrasy Test 6
### Sequential IO faster than random IO

source scripts/config.sh

echo ""
echo "$0 $@"

rm -rf /dev/shm/*
echo "DRAM Seq and Rand Read b/w"
NJOBS=16 PMEM_DIR="/dev/shm" WORKLOADS="SeqR RandR" scripts/run_fio.sh i6-dram-bw
sleep 2
NJOBS=16 WORKLOADS="SeqR RandR" scripts/get_fio_results.sh i6-dram-bw
sleep 2

echo "PMEM Seq and Rand Read b/w"
NJOBS=16 PMEM_DIR=$PMEM_DIR1 WORKLOADS="SeqR RandR" scripts/run_fio.sh i6-pmem-bw
sleep 2
NJOBS=16 WORKLOADS="SeqR RandR" scripts/get_fio_results.sh i6-pmem-bw
sleep 2

echo "Flash Seq and Rand Read b/w"
PMEM_DIR=$FLASH_DIR ENGINE="Sync" NJOBS=16 WORKLOADS="SeqR RandR" BSIZE="4096 8192 16384 32768 65536 131072 262144" scripts/run_fio.sh i6-flash-bw
sleep 2
ENGINE="Sync" NJOBS=16 WORKLOADS="SeqR RandR" BSIZE="4096 8192 16384 32768 65536 131072 262144" scripts/get_fio_results.sh i6-flash-bw
sleep 2

echo "PMEM Seq Read counters"
TOOL="ipmw" NJOBS=16 BSIZE=256 PMEM_DIR=$PMEM_DIR1 WORKLOADS="SeqR" scripts/run_fio.sh i6-pmem-counters
sleep 2
echo "PMEM Rand Read counters"
TOOL="ipmw" NJOBS=16 BSIZE=256 PMEM_DIR=$PMEM_DIR1 WORKLOADS="RandR" scripts/run_fio.sh i6-pmem-counters
sleep 2
