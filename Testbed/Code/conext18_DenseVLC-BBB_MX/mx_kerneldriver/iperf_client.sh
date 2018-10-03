#!/bin/bash

address="192.168.0."
address+=$1

iperf -c $address -u -b 33k -l 400 -p 10001  -t 100
