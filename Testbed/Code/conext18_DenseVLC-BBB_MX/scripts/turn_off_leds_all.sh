#!/bin/bash

parallel-ssh -P -h ~/.pssh_hosts_tx -I < turn_off_leds.sh

