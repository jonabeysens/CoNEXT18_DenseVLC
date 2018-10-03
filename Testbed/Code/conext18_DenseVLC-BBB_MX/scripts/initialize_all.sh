#!/bin/bash

echo "Initialization is going to start"

echo "Initializing MX kernel"
cd ../mx_kerneldriver
sudo sh load_mx_kerneldriver.sh
read -n 1 -p Continue?

cd ../scripts
echo "Initializing TX kernel"
sh ./load_kernel_tx_all.sh
read -n 1 -p Continue?

echo "Initializing TX PRU"
sh ./load_pru_tx_dynamic_noillum_all.sh
sh ./load_pru_tx_dynamic_noillum_all.sh
read -n 1 -p Continue?

echo "Initializing RX kernel"
sh ./load_kernel_rx_all.sh
read -n 1 -p Continue?

echo "Initializing RX PRU"
sh ./load_pru_rx_dynamic_all.sh
sh ./load_pru_rx_dynamic_all.sh

echo "Initialization done!"


