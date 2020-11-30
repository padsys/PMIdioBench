#!/bin/bash

COUNTERS=("bytes_read" "bytes_written" "read_hit_ratio" "write_hit_ratio" \
          "wdb_merge_percent" "media_read_ops" "media_write_ops" \
          "read_64B_ops_received" "write_64B_ops_received" "ddrt_read_ops" \
          "ddrt_write_ops")

# Configuration Parameters
: ${IPMW_HOME:=/opt/intel/ipmwatch} # ipmwatch install dir
: ${NUMA_NODE:=0} # NUMA node to get metrics for
: ${MAX_BW:=8192} # Max PMEM B/W in MB/s

function get_total_val()
{
	local NUM=$(( $1 + 3 ))
	local FILE=$2
	local TOTAL_VAL=0
	local DIMM_CNT=0

	for (( i = 0; i < $NUMA_NODE; i++ )); do
		DIMM_CNT=$(ipmctl show -topology -socket $i | grep "Non-Volatile" | wc -l)
		NUM=$(( $NUM + ( $DIMM_CNT * 11 ) ))
	done

	tail -n +8 $FILE > _tmp

	DIMM_CNT=$(ipmctl show -topology -socket $NUMA_NODE | grep "Non-Volatile" | wc -l)
	for (( i = 0; i < $DIMM_CNT; i++ )); do
		DIMM_VAL=$(cat _tmp | cut -f"$NUM" | paste -s -d+ | bc)
		NUM=$(( $NUM + 11 ))
		TOTAL_VAL=$(echo "$TOTAL_VAL+$DIMM_VAL" | bc)
	done

	echo "$TOTAL_VAL"
	rm -f _tmp
}

function get_pmem_stats()
{
    (
	local FILE=$1

	# Print header
	echo -e "pmem-counter\tvalue\thuman-readable-value"

	# Get total bytes_written
	total_bytes_written=$(get_total_val 1 $FILE)
	total_bytes_written_hr=$(numfmt --to=iec-i --suffix=B $total_bytes_written)
	echo -e "total_bytes_written\t$total_bytes_written\t$total_bytes_written_hr"

	# Get total bytes_read
	total_bytes_read=$(get_total_val 0 $FILE)
	if [ $total_bytes_read -ge $total_bytes_written ]; then
		total_bytes_read=$(( $total_bytes_read - $total_bytes_written ))
	fi
	total_bytes_read_hr=$(numfmt --to=iec-i --suffix=B $total_bytes_read)
	echo -e "total_bytes_read\t$total_bytes_read\t$total_bytes_read_hr"

	# Get total media_write_ops
	media_write_ops=$(get_total_val 6 $FILE)
	media_write_ops_hr=$(numfmt --to=si $media_write_ops)
	echo -e "total_media_write_ops\t$media_write_ops\t$media_write_ops_hr"

	# Get total media_read_ops
	media_read_ops=$(get_total_val 5 $FILE)
	if [ $media_read_ops -ge $media_write_ops ]; then
		media_read_ops=$(( $media_read_ops - $media_write_ops ))
	fi
	media_read_ops_hr=$(numfmt --to=si $media_read_ops)
	echo -e "total_media_read_ops\t$media_read_ops\t$media_read_ops_hr"

	# Get total ddrt_write_ops
	ddrt_write_ops=$(get_total_val 10 $FILE)
	ddrt_write_ops_hr=$(numfmt --to=si $ddrt_write_ops)
	echo -e "total_ddrt_write_ops\t$ddrt_write_ops\t$ddrt_write_ops_hr"

	# Get total ddrt_read_ops
	ddrt_read_ops=$(get_total_val 9 $FILE)
	if [ $ddrt_read_ops -ge $ddrt_write_ops ]; then
		ddrt_read_ops=$(( $ddrt_read_ops - $ddrt_write_ops ))
	fi
	ddrt_read_ops_hr=$(numfmt --to=si $ddrt_read_ops)
	echo -e "total_ddrt_read_ops\t$ddrt_read_ops\t$ddrt_read_ops_hr"

	#Formulas:
	#read_hit_ratio : (ddrt_read_ops - media_read_ops) / ddrt_read_ops
	#write_hit_ratio : (ddrt_write_ops - media_write_ops) / ddrt_write_ops
	#achieved bw : total_bytes_written / total_time
	#ebr : achieved_bw / max_bw
	#ewr : media_write_ops / ddrt_write_ops
	#rwr : media_read_ops / media_write_ops

	if [ $ddrt_read_ops -ge $media_read_ops ]; then
		read_hit_ratio=$(echo "scale=3;($ddrt_read_ops - $media_read_ops) / $ddrt_read_ops" | bc)
	else
		read_hit_ratio=0
	fi
	if [ $ddrt_write_ops -ge $media_write_ops ]; then
		write_hit_ratio=$(echo "scale=3;($ddrt_write_ops - $media_write_ops) / $ddrt_write_ops" | bc)
	else
		write_hit_ratio=0
	fi
	total_time=$(tail -n +8 $FILE | wc -l)
	achieved_bw=$(echo "scale=3;$total_bytes_written / $total_time" | bc)
	achieved_bw_hr=$(numfmt --to=iec-i --suffix=B/s $achieved_bw)
	ebr=$(echo "scale=3;$achieved_bw / 1024.0 / 1024.0 / $MAX_BW" | bc)
	ewr=$(echo "scale=3;$media_write_ops / $ddrt_write_ops" | bc)
	rwr=$(echo "scale=3;$media_read_ops / $media_write_ops" | bc)

	echo -e "read_hit_ratio\t$read_hit_ratio\t$read_hit_ratio"
	echo -e "write_hit_ratio\t$write_hit_ratio\t$write_hit_ratio"
	echo -e "achieved_bw\t$achieved_bw\t$achieved_bw_hr"
	echo -e "ebr\t$ebr\t$ebr"
	echo -e "ewr\t$ewr\t$ewr"
	echo -e "rwr\t$rwr\t$rwr"
    ) > _temp

	column -t _temp
	rm -f _temp
}

function main()
{
	if [[ -z "$@" ]]; then
		get_pmem_stats ipmw.tsv
		exit 0
	fi

	# Start ipmwatch tool in the background
	$IPMW_HOME/bin64/ipmwatch 1 -td -f ipmw 2>/dev/null &
	PID=$!

	# Run app
	"$@"

	# Kill ipmwatch tool
	kill -9 $PID
	wait $PID 2>/dev/null
	sleep 2

	# Get PMEM stats
	get_pmem_stats ipmw.tsv
}

main $@
