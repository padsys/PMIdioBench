#!/bin/bash

source scripts/config.sh

: ${THREADS:="1 2 4 8 16 28"}
: ${WORKLOADS:="workloada workloadb workloadc"}
: ${TOOL:=""} # Only ipmw tool allowed

function tool_cmdline()
{
	local TOOL=$1

	if [ "$TOOL" == "ipmw" ]; then
		echo "scripts/ipmw.sh"
	else
		echo ""
	fi
}

function run_case()
{
    export INDEX_PATH=$INDEX_DIR
    for workload in ${WORKLOADS[*]}
    do
    	for threads in ${THREADS[*]}
    	do
	    echo "Running "$workload" with $threads threads"

	    # Cleanup
    	    rm -rf $DATA_DIR
    	    mkdir -p $DATA_DIR
    	    rm -rf $INDEX_DIR
    	    mkdir -p $INDEX_DIR
    	    sleep 2

	    # Start MongoDB server daemon
            numactl -N 0 $MONGODB_HOME/mongod \
		--storageEngine=pmse --dbpath=$DATA_DIR > mongod.log 2>&1 &
    	    MONGODB_PID=$!
    	    sleep 5

	    # Load data
	    echo -n "Loading Data..."
    	    numactl -N 1 $YCSB_HOME/bin/ycsb load mongodb \
                -s -P workloads/ycsb/$workload \
                -p mongodb.url=mongodb://localhost:27017/ycsb?journal=true \
                -threads $(nproc) > results/mongodb_"$workload"_load_"$threads"_"$CASE".log 2>&1
	    echo "Done."
    	    sleep 2

	    # Run workload
	    echo -n "Running Workload..."
	    $(tool_cmdline $TOOL) \
    	    numactl -N 1 $YCSB_HOME/bin/ycsb run mongodb \
                -s -P workloads/ycsb/$workload \
                -p mongodb.url=mongodb://localhost:27017/ycsb?journal=true \
                -threads $threads > results/mongodb_"$workload"_run_"$threads"_"$CASE".log 2>&1
	    echo "Done."
    	    sleep 2

	    # Kill MongoDB server daemon
    	    kill -9 $MONGODB_PID
	    wait $MONGODB_PID 2>/dev/null
    	    sleep 2
    	done
    done
}

mkdir -p results

CASE="both"
DATA_DIR=$PMEM_DIR1/mongodb
INDEX_DIR=$PMEM_DIR1/mongodb/
echo "Case: $CASE"
run_case
sleep 2
scripts/get_ycsb_results.sh both mongodb
echo ""

CASE="index"
DATA_DIR=$PMEM_DIR2/mongodb
INDEX_DIR=$PMEM_DIR1/mongodb/
echo "Case: $CASE"
run_case
sleep 2
scripts/get_ycsb_results.sh index mongodb
echo ""

CASE="record-store"
DATA_DIR=$PMEM_DIR1/mongodb
INDEX_DIR=$PMEM_DIR2/mongodb/
echo "Case: $CASE"
run_case
sleep 2
scripts/get_ycsb_results.sh record-store mongodb
echo ""

CASE="none"
DATA_DIR=$PMEM_DIR2/mongodb
INDEX_DIR=$PMEM_DIR2/mongodb/
echo "Case: $CASE"
run_case
sleep 2
scripts/get_ycsb_results.sh none mongodb
echo ""
