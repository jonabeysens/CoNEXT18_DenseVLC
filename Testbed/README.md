# Overview testbed

The testbed consists of three main components: the controller, the transmitters, and the receivers. Pictures are included in the folder [Pictures](Pictures). 

The system contains 1 BBB (controller (MX)) + 9 BBBs (to transmit) + 4 BBB wireless (to receive). 
Every BBB for transmitting controls four TXs. Each RX is equipped with a single BBB Wireless. All BBBs are equipped with a receiver cape, as discussed in the paper.  

A local WLAN network should be set up, so the controller can communicate with the transmitters via Ethernet and with the receivers via WiFi. 
Furthermore, a connection is set up between the controller and a Matlab script in order to transfer data for further analysis.

## Part 1: Channel measurments

In the following will be presented how channel measurements can be performed on the testbed: 


#### 1. initialize all components (controller (MX), transmitters, receivers) 
- in the MX repository, go to the folder conext18_DenseVLC_MX
- go to the folder scripts
- initialize all components by running "./initalize_all.sh". This will initialize the MX kernel, TX kernels and RX kernels, as well as the TX PRU and RX PRU code


#### 2. run the Matlab script
- open "comm_mx.m" script in Matlab R2017b
- run the script
- enter the command "1" to perform a channel measurement without sending the reply back to the MX
- enter the command "4" to perform a channel measurement from a specific TX and send back the reply to the MX


## Part 2: Iperf measurements

In the following, the steps for performing an iperf measurement with synchronization are listed: 


#### 1. initialize all components
- see above

#### 2. run the Matlab script
- open "comm_mx.m" script in Matlab R2017b
- run the script
- enter the command "2" to set the TXs that should be used for the iperf measurment (for example TX1, TX2, TX7, TX8)
- enter the command "3" to set the leading TX required for the synchronization


#### 3. run the client and server code
- in the RX repository, go to the rx_kerneldriver folder and run the script "iperf_server.sh"
- in the MX repository, go to the mx_kernelderiver folder and run the script "iperf_client.sh"
- the measurement starts!
