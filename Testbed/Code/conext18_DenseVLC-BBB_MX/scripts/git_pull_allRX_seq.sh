#!/bin/bash

array=(rx2 rx3 rx4)
script=git_pullRX.sh
for	i in ${array[@]}; do
	echo "$i:"
	ssh -q $i 'bash -s' < $script
done

