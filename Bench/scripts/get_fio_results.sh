#!/bin/bash

# Number of threads
: ${NJOBS:="1 2 4 8 16 28"}

# Block (IO) size
: ${BSIZE:="64 128 256 512 1024 2048 4096"}

# Workloads
: ${WORKLOADS:="SeqR SeqW RandR RandW"}

# Engine
: ${ENGINE:="Libpmem"}

case=$1

for workload in ${WORKLOADS[*]}; do
	echo "--------------"
	echo "$workload"
	echo "--------------"
	echo ""
	echo -n -e "\t"
	for bsize in ${BSIZE[*]}; do	
		echo -n -e "$bsize\t"
	done
	echo ""

	for njobs in ${NJOBS[*]}; do
		echo -n -e "$njobs\t"
		for bsize in ${BSIZE[*]}; do
			bw=$(grep bw results/"$ENGINE""$workload"-"$njobs"-"$bsize"-"$case".log | grep avg | tail -n 1 | cut -d , -f 4 | cut -d = -f 2)
			if (( $(echo "$bw > 80000" | bc -l) )); then
				bw=$(echo "scale=2;$bw / 1024" | bc -l)
			fi
			echo -n -e "$bw\t"
		done
		echo ""
	done
	echo ""
done
