#!/bin/bash

rmmod vlc
#watch -n 0.1 "dmesg | tail -n $((LINES-6))"

find -exec touch \{\} \;

make clean

make

# get the RX ID
a=$(ifconfig wlan0 | grep -Eio '[0-9]*\s B' | cut -d ' ' -f1)
a_last=$((a - 20)) # 20 is the start index of the RX, but not used further
echo RX ID: $a_last

# Insert the driver
insmod vlc.ko self_id=$a

# Configure the IP address of the new interface
b="192.168.0."
b+=$a
echo VLC IP address: $b

ifconfig vlc0 $b
