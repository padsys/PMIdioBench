#!/bin/bash
### Idiosyncrasy Test 3
### Poor bandwidth for small-sized random IO

source scripts/config.sh

echo ""
echo "$0 $@"

echo "PMEM b/w"
PMEM_DIR=$PMEM_DIR1 NJOBS="16" WORKLOADS="SeqW RandW" BSIZE="64 128 256 512" PMEM_MOVNT_THRESHOLD=0 scripts/run_fio.sh i3-pmem-bw
sleep 2
NJOBS="16" WORKLOADS="SeqW RandW" BSIZE="64 128 256 512" scripts/get_fio_results.sh i3-pmem-bw
sleep 2
echo "PMEM SeqW counters"
TOOL="ipmw" PMEM_DIR=$PMEM_DIR1 NJOBS="16" WORKLOADS="SeqW" BSIZE="64 128 256 512" PMEM_MOVNT_THRESHOLD=0 scripts/run_fio.sh i3-pmem-counters
sleep 2
echo "PMEM RandW counters"
TOOL="ipmw" PMEM_DIR=$PMEM_DIR1 NJOBS="16" WORKLOADS="RandW" BSIZE="64 128 256 512" PMEM_MOVNT_THRESHOLD=0 scripts/run_fio.sh i3-pmem-counters
sleep 2
