#!/bin/bash
iperf -u -l 800 -s -i3 -B 192.168.0.1 -p 10001
