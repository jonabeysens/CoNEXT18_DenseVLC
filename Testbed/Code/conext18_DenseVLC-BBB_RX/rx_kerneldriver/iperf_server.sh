#!/bin/bash

# get the RX ID
a=$(ifconfig wlan0 | grep -Eio '[0-9]*\s B' | cut -d ' ' -f1)
b="192.168.0."
b+=$a

# start iperf
iperf -u -l 400 -s -i3 -B $b -p 10001
