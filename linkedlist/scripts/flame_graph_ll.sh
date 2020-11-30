#!/bin/bash
### Script to run a command (application) and generate its flame graph.
### For best results, compile your application without optimization (-O0),
### and with the -g and -fno-omit-frame-pointer flags.
### Usage: ./flame_graph.sh OUTFILE EXENAME command [args]
### Example: ./flame_graph.sh test.svg test ./test

: ${FLAME_GRAPH_HOME:=~/FlameGraph}

# Get output file name
FILE=$1
shift

# Get executable name
EXE_NAME=$1
shift

# Run command
perf record -F 99 -a -g --all-user -- "$@"

# Parse perf output and generate flame graph
perf script > temp.perf
$FLAME_GRAPH_HOME/stackcollapse-perf.pl temp.perf > temp.folded
sed -i 's/p_list_//g' temp.folded
sed -i 's/add/insert/g' temp.folded
sed -i 's/remove/delete/g' temp.folded
sed -i 's/find/search/g' temp.folded
sed -i 's/contains/find/g' temp.folded
sed -i 's/\[unknown\];//g' temp.folded

rm -f temp2.folded
while IFS= read -r l; do
	echo "$l" | rev | cut -d ' ' -f 2- | rev | cut -d ';' -f 2- | tr -d '[:space:]' | tr ';' '\n' > temp
	LINE=$(echo "$l" | cut -d ';' -f 1)
	COUNT=$(echo "$l" | rev | cut -d ' ' -f 1 | rev)
	for f in $(cat temp); do
		FUNC=$(echo "$f" | grep -v "Invo" | grep -v "invo" | grep -v "void")
		if [[ "$FUNC" != "" ]]; then
			LINE=$(echo "$LINE;$FUNC" | tr -d '\n')
		fi
	done
	echo "$LINE $COUNT" >> temp2.folded
done < temp.folded
sed -i -- 's/SOFTList<long>\:\://g' temp2.folded
grep "^$EXE_NAME" temp2.folded | $FLAME_GRAPH_HOME/flamegraph.pl --width 400 --height 20 --fontsize 14 --bgcolors grey --cp > $FILE

# Cleanup
rm -f perf.data temp.perf temp.folded temp2.folded
