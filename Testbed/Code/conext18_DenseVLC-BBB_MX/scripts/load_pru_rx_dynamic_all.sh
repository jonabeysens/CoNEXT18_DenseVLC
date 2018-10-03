#!/bin/bash

parallel-ssh -P -h ~/.pssh_hosts_rx -I < load_pru_rx_dynamic.sh

