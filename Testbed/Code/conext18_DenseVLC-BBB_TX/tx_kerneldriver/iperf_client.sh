#!/bin/bash
iperf -c 192.168.0.1 -u -b 300k -l 800 -p 10001  -t 10
