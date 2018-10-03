#!/bin/bash

parallel-ssh -P -h ~/.pssh_hosts_all -I < shutdown.sh

