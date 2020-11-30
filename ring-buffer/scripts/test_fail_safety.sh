#!/bin/bash
### Test failure safety of application
### Usage: ./test_fail_safety <runtime> <app> [app args]

TIME=$1
shift
APP=$@

# Run app
./$APP &
PID=$!

# Let it run for TIME s
sleep $TIME

# Send SIGINT to test failure safety
kill -SIGINT $PID
wait
