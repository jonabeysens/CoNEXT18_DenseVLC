#!/bin/bash

array=($1 $2)
script=load_pru_rx_dynamic.sh
for	i in ${array[@]}; do
	echo "$i:"
	ssh -q $i 'bash -s' < $script
done

