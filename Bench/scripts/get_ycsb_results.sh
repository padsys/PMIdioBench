#!/bin/bash

source scripts/config.sh

: ${THREADS:="1 2 4 8 16 28"}
: ${WORKLOADS:="workloada workloadb workloadc"}

case=$1
system=$2

rm -f _temp

for workload in ${WORKLOADS[*]}
do
	echo "$workload" >> _temp
	echo -e "threads\tthroughput" >> _temp
	for threads in ${THREADS[*]}
	do
		value=$(grep OVERALL results/"$system"_"$workload"_run_"$threads"_"$case".log | grep Throughput | awk '{ print $3 }')
		echo -e "$threads\t$value" >> _temp
	done
	echo -e "threads\tread_avg_lat\tread_p95_lat\tread_p99_lat" >> _temp
	for threads in ${THREADS[*]}
	do
		avg=$(grep READ results/"$system"_"$workload"_run_"$threads"_"$case".log | grep -v FAILED | grep Average | awk '{ print $3 }')
		p95=$(grep READ results/"$system"_"$workload"_run_"$threads"_"$case".log | grep -v FAILED | grep 95th | awk '{ print $3 }')
		p99=$(grep READ results/"$system"_"$workload"_run_"$threads"_"$case".log | grep -v FAILED | grep 99th | awk '{ print $3 }')
		echo -e "$threads\t$avg\t$p95\t$p99" >> _temp
	done
	if [[ "$workload" != "workloadc" ]]; then
		echo -e "threads\tinsert_avg_lat\tinsert_p95_lat\tinsert_p99_lat" >> _temp
		for threads in ${THREADS[*]}
		do
			avg=$(grep INSERT results/"$system"_"$workload"_run_"$threads"_"$case".log | grep -v FAILED | grep Average | awk '{ print $3 }')
			p95=$(grep INSERT results/"$system"_"$workload"_run_"$threads"_"$case".log | grep -v FAILED | grep 95th | awk '{ print $3 }')
			p99=$(grep INSERT results/"$system"_"$workload"_run_"$threads"_"$case".log | grep -v FAILED | grep 99th | awk '{ print $3 }')
			echo -e "$threads\t$avg\t$p95\t$p99" >> _temp
		done
	fi
done

column -t _temp
rm -f _temp
