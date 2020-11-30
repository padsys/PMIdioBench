#!/bin/bash

function portable_nproc()
{
    # Get number of cores
    OS=$(uname -s)
    if [ "$OS" = "Linux" ]; then
        NPROCS=$(nproc --all)
    elif [ "$OS" = "Darwin" ] || \
         [ "$(echo $OS | grep -q BSD)" = "BSD" ]; then
        NPROCS=$(sysctl -n hw.ncpu)
    else
        NPROCS=$(getconf _NPROCESSORS_ONLN) # glibc/coreutils fallback
    fi
    echo "$NPROCS"
}

function portable_nsock()
{
    # Get number of sockets
    NSOCK=$(cat /proc/cpuinfo 2>/dev/null | grep "physical id" | sort -u | wc -l)
    if [ $? -ne 0 ] || [ $NSOCK -eq 0 ]; then
	# Assume it is 1 in case of error
        echo "1"
    else
        echo "$NSOCK"
    fi
}

function tool_cmdline()
{
	local TOOL=$1

	if [ "$TOOL" == "ipmw" ]; then
		echo "scripts/ipmw.sh"
	elif [ "$TOOL" == "flame-graph" ]; then
		echo "scripts/flame_graph_ll.sh ll-"$range"-"$ur".svg main"
	else
		echo ""
	fi
}

ncpu=$(portable_nproc)
nsock=$(portable_nsock)
ncpu_per_sock=$(( $ncpu / $nsock ))

# Update %
: ${UPDATES:="0 5 10 20 30 40 50"}

# Range
: ${RANGES:="256 1024 4096"}

# Duration (ms)
: ${DURATION:=15000}

# Threads
: ${THREADS:=$ncpu_per_sock}

# PMEM Directory
: ${PMEM_DIR:="/mnt/pmem1"}

# PMIdioBench install dir
: ${PMIDIOBENCH_HOME:="../Bench"}

# Analysis Tool
: ${TOOL:=""} # ipmw or flame-graph

# Uncomment to use AVX512f nt-stores
#export PMEM_AVX512F=1

# Uncomment to disable PMEM flush
#export PMEM_NO_FLUSH=1

rm -f output.log

for range in ${RANGES[*]}; do
	echo -e "\nRange: $range, Threads: $THREADS"
	echo -e "Update %\tThroughput"

	for ur in ${UPDATES[*]}; do
		# Cleanup
		rm -f $PMEM_DIR/p_llist
		rm -f $PMEM_DIR/tlog
		sleep 2

		# Run app
		echo -e "\nRange: $range, Threads: $THREADS, Updates: $ur" >> output.log
		$(tool_cmdline $TOOL) \
		numactl -N 0 ./main -d $DURATION -r $range -u $ur -n $THREADS >> output.log 2>&1

		# Get throughput
		thr=$(grep tx output.log | tail -n 1 | awk '{ print $4 }' | cut -d '(' -f 2)
		echo -e "$ur\t\t$thr"
		sleep 2
	done
done
