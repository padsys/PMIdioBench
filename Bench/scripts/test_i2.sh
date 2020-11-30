#!/bin/bash
### Idiosyncrasy Test 2
### Asymmetric load/p-store bandwidth

source scripts/config.sh

echo ""
echo "$0 $@"

rm -rf /dev/shm/*
echo "DRAM load and nt-store b/w"
PMEM_DIR="/dev/shm" NJOBS="28" WORKLOADS="SeqR SeqW" PMEM_MOVNT_THRESHOLD=0 scripts/run_fio.sh i2-dram-nt
sleep 2
NJOBS="28" WORKLOADS="SeqR SeqW" scripts/get_fio_results.sh i2-dram-nt
sleep 2
echo "DRAM store+clwb b/w"
PMEM_DIR="/dev/shm" NJOBS="28" WORKLOADS="SeqW" PMEM_NO_MOVNT=1 scripts/run_fio.sh i2-dram-s
sleep 2
NJOBS="28" WORKLOADS="SeqW" scripts/get_fio_results.sh i2-dram-s
sleep 2

echo "PMEM load b/w"
PMEM_DIR=$PMEM_DIR1 NJOBS="28" WORKLOADS="SeqR" PMEM_MOVNT_THRESHOLD=0 scripts/run_fio.sh i2-pmem-nt
sleep 2
NJOBS="28" WORKLOADS="SeqR" scripts/get_fio_results.sh i2-pmem-nt
sleep 2
echo "PMEM nt-store b/w"
PMEM_DIR=$PMEM_DIR1 NJOBS="8" WORKLOADS="SeqW" PMEM_MOVNT_THRESHOLD=0 scripts/run_fio.sh pmem-i2-nt
sleep 2
NJOBS="8" WORKLOADS="SeqW" scripts/get_fio_results.sh pmem-i2-nt
sleep 2
echo "PMEM store+clwb b/w"
PMEM_DIR=$PMEM_DIR1 NJOBS="8" WORKLOADS="SeqW" PMEM_NO_MOVNT=1 scripts/run_fio.sh i2-pmem-s
sleep 2
NJOBS="8" WORKLOADS="SeqW" scripts/get_fio_results.sh i2-pmem-s
sleep 2

echo "Flash read and write b/w"
PMEM_DIR=$FLASH_DIR ENGINE="Sync" NJOBS="28" WORKLOADS="SeqR SeqW" BSIZE="4096 8192 16384 32768 65536 131072 262144" scripts/run_fio.sh i2-flash
sleep 2
ENGINE="Sync" NJOBS="28" WORKLOADS="SeqR SeqW" BSIZE="4096 8192 16384 32768 65536 131072 262144" scripts/get_fio_results.sh i2-flash
sleep 2
