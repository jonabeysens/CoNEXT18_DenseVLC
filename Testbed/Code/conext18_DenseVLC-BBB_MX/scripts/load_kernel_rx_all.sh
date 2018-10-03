#!/bin/bash

parallel-ssh -P -h ~/.pssh_hosts_rx -I < load_kernel_rx.sh

