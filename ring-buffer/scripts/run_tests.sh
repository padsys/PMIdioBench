#!/bin/bash
### Run RB queue tests and print consolidated results.
### Usage: ./run_tests.sh [PUSH | POP] SLOT_SIZE
### Example: ./run_tests.sh PUSH 4096

# PMEM Directory
: ${PMEM_DIR:="/mnt/pmem1"}

# PMIdioBench install dir
: ${PMIDIOBENCH_HOME:="../Bench"}

# Analysis Tool
: ${TOOL:=""} # ipmw or flame-graph

# Get stats
: ${GET_STATS:="true"}

function tool_cmdline()
{
	local TOOL=$1

	if [ "$TOOL" == "ipmw" ]; then
		echo "scripts/ipmw.sh"
	elif [ "$TOOL" == "flame-graph" ]; then
		echo "scripts/flame_graph.sh rb-"$2".svg $3"
	else
		echo ""
	fi
}

function get_tail_lat()
{
	# File containing sorted lat values
	file=$1
	# Tail latency to calculate (e.g., 99.9)
	tail_lat=$2

	# Find line no
	lines=$(wc -l $file | awk '{ print $1 }')
	line_no=$(echo "$lines*$tail_lat/100" | bc)

	# Print tail latency
	if [ $line_no -gt 0 ]; then
		sed -n "$line_no"p $file
	else
		echo "Unknown"
	fi
}

function get_stats()
{
	# Consider the first 100000 operations as warm up
	grep -v ms output.log | tail -n +100000 > $1
	sort -n --parallel=$(nproc) $1 > $1-sorted
	mv -f $1-sorted $1

	# Get latency results
	avg_lat=$(awk '{ total += $1; count++ } END { if (count > 0) { print total/count } }' $1)
	p99_lat=$(get_tail_lat $1 99)
	p99_9_lat=$(get_tail_lat $1 99.9)

	echo -e "$2\t$avg_lat\t$p99_lat\t$p99_9_lat"
}

function cleanup()
{
	rm -f $PMEM_DIR/queue
}

function main()
{
	# Uncomment to use AVX512f nt-stores
	#export PMEM_AVX512F=1

	# Uncomment to only use nt-stores
	#export PMEM_MOVNT_THRESHOLD=0

	# Uncomment to disable nt-stores
	#export PMEM_NO_MOVNT=1

	TEST=$1
	SIZE=$2

	# Cleanup
	cleanup
	sleep 5

	if [[ "$GET_STATS" == "true" ]]; then
		echo "$TEST latency (in cycles)"
		echo -e "system\tavg\tp99\tp99.9"
	fi

	# Volatile Queue
	numactl -N 0 ./p_rb_q_eadr.x false > output.log 2>&1
	sleep 2
	if [[ "$GET_STATS" == "true" ]]; then
		get_stats $TEST-lat-vq-$SIZE.log volatile
	else
		cat output.log
	fi
	sleep 2
	cleanup
	sleep 5

	# Persistent TX-free (eADR) Queue
	$(tool_cmdline $TOOL tx-free-eadr-$SIZE p_rb_q_eadr.x) \
	numactl -N 0 ./p_rb_q_eadr.x true > output.log 2>&1
	sleep 2
	if [[ "$GET_STATS" == "true" ]]; then
		get_stats $TEST-lat-tx-free-eadr-$SIZE.log TX-free-eADR
	else
		echo "TX-free-eADR"
		cat output.log
	fi
	sleep 2
	cleanup
	sleep 5

	# Persistent TX-free (ADR) Queue (experimental; does not provide linearizability)
	$(tool_cmdline $TOOL tx-free-adr-$SIZE p_rb_q_exp.x) \
	numactl -N 0 ./p_rb_q_exp.x true > output.log 2>&1
	sleep 2
	if [[ "$GET_STATS" == "true" ]]; then
		get_stats $TEST-lat-tx-free-adr-$SIZE.log TX-free-ADR
	else
		echo "TX-free-ADR"
		cat output.log
	fi
	sleep 2
	cleanup
	sleep 5

	# Persistent TX (eADR) Queue
	PMEM_NO_FLUSH=1 $(tool_cmdline $TOOL tx-eadr-$SIZE p_rb_q_adr.x) \
	numactl -N 0 ./p_rb_q_adr.x > output.log 2>&1
	sleep 2
	if [[ "$GET_STATS" == "true" ]]; then
		get_stats $TEST-lat-tx-eadr-$SIZE.log TX-eADR
	else
		echo "TX-eADR"
		cat output.log
	fi
	sleep 2
	cleanup
	sleep 5

	# Persistent TX (ADR) Queue
	$(tool_cmdline $TOOL tx-adr-$SIZE p_rb_q_adr.x) \
	numactl -N 0 ./p_rb_q_adr.x > output.log 2>&1
	sleep 2
	if [[ "$GET_STATS" == "true" ]]; then
		get_stats $TEST-lat-tx-adr-$SIZE.log TX-ADR
	else
		echo "TX-ADR"
		cat output.log
	fi
	sleep 2
	cleanup
}

main $@
