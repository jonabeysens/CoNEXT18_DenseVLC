#!/bin/bash

parallel-ssh -P -h ~/.pssh_hosts_tx -I < load_kernel_tx.sh

