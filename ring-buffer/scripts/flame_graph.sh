#!/bin/bash
### Script to run a command (application) and generate its flame graph.
### For best results, compile your application without optimization (-O0),
### and with the -g and -fno-omit-frame-pointer flags.
### Usage: ./flame_graph.sh OUTFILE EXENAME command [args]
### Example: ./flame_graph.sh test.svg test ./test

set -e

: ${FLAME_GRAPH_HOME:=~/FlameGraph}

function cleanup()
{
	# Cleanup
	rm -f temp.perf temp.folded perf.data
}

trap cleanup EXIT

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
grep "^$EXE_NAME" temp.folded | $FLAME_GRAPH_HOME/flamegraph.pl > $FILE
