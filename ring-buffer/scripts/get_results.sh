#!/bin/bash

re='^[0-9]+$'
for line in $(cat output.log); do
	if [[ $line =~ $re ]]; then
		echo $line
	fi
done
