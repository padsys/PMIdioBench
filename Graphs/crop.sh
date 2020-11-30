#!/bin/bash

if [[ -z $1 ]]
then
	files=`ls | grep -v crop`
else
	files=$1
fi
for file in $files
do
	pdfcrop $file
	temp=`echo "$file" | cut -d . -f 1`
	mv "$temp"-crop.pdf $file
done
