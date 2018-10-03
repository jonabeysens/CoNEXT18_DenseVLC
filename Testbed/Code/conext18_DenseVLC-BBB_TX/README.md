This repository contains all necessarry code for the transmitter (TX). It contains three folders: 


1. The tx_kerneldriver folder contains the code for the kernel module written in C. In order to load the module, run the "load_tx_kerneldriver.sh" script as sudo. 

2. The pru_dynamic folder contains the PRU code for the transmitter with illumination functionality. To load the PRU code, run the "deploy.sh" script

3. The pru_dynanic_noillum folder contains the same functional PRU code but without the illumination. 
This can be useful for debugging purposes, because when the illumination functionality is on, the human eye cannot distinguish when a packet is sent over the channel. 
