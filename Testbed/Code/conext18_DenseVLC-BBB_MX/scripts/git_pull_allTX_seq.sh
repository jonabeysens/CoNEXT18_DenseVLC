#!/bin/bash

array=(tx1 tx2 tx3 tx4 tx5 tx6 tx7 tx8 tx9)
script=git_pullTX.sh
for	i in ${array[@]}; do
	echo "$i:"
	ssh -q $i 'bash -s' < $script
done

