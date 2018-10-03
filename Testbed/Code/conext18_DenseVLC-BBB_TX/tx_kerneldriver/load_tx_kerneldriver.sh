#!/bin/bash

rmmod vlc
#watch -n 0.1 "dmesg | tail -n $((LINES-6))"

find -exec touch \{\} \;

make clean

make

a=$(ifconfig eth0 | grep -Eio '[0-9]*\s B' | cut -d ' ' -f1)
#TODO: Change the interface
a=$((a - 30)) # 30 is the start index of the TX
echo Transmitter ID: $a

# Insert the driver
insmod vlc.ko self_id=$a

# Configure the IP address of the new interface
ifconfig vlc0 192.168.0.1
