#! /bin/bash

##############################################################################
#
# Copyright (C) 2016 Zubeen Tolani <ZeekHuge - zeekhuge@gmail.com>
#
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
#	* Redistributions of source code must retain the above copyright
#	  notice, this list of conditions and the following disclaimer.
#
#	* Redistributions in binary form must reproduce the above copyright
#	  notice, this list of conditions and the following disclaimer in the
#	  documentation and/or other materials provided with the
#	  distribution
#
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#############################################################################


# The script builds the pru_blinky project and configures the pinmuxing for $HEADER$PIN_NUM

#If changing these variables, make sure the given pin can be muxed to the given pru.  
HEADER=P8_
PIN_NUMBER=45
#PRU_CORE should be either 0 or 1
PRU_CORE=1


echo "-Building project"
	make clean
	make

echo "-Placing the firmware"
	cp gen/main_pru1_fw.out /lib/firmware/am335x-pru1-fw
	cp gen/main_pru0_fw.out /lib/firmware/am335x-pru0-fw

echo "-Configuring pinmux"
	config-pin -a P8_27 pruout # TX1
	config-pin -a P8_28 pruout # TX1
	config-pin -a P8_39 pruout # TX2
	config-pin -a P8_40 pruout # TX2
	config-pin -a P8_41 pruout # TX3
	config-pin -a P8_42 pruout # TX3
	config-pin -a P8_43 pruout # TX4
	config-pin -a P8_44 pruout # TX4
	config-pin -a P9_25 pruout # debug preamble
	config-pin -a P9_27 pruout # debug preamble
	config-pin -a P9_30 pruin # MISO PD
	config-pin -a P9_31 pruout # CS PD
	config-pin -a P9_28 pruout # MOSI PD
	
	config-pin -q P8_27 # TX1
	config-pin -q P8_28 # TX1
	config-pin -q P8_39 # TX2
	config-pin -q P8_40 # TX2
	config-pin -q P8_41 # TX3
	config-pin -q P8_42 # TX3
	config-pin -q P8_43 # TX4
	config-pin -q P8_44 # TX4
	config-pin -q P9_25 # debug preamble
	config-pin -q P9_27 # debug preamble
	config-pin -q P9_30 # MISO PD
	config-pin -q P9_31 # CS PD
	config-pin -q P9_28 # MOSI PD

	

echo "-Rebooting"
	
	
		echo "Rebooting pru-core 0"
		echo "4a334000.pru0" > /sys/bus/platform/drivers/pru-rproc/unbind 2>/dev/null
		echo "4a334000.pru0" > /sys/bus/platform/drivers/pru-rproc/bind
	
		echo "Rebooting pru-core 1"
		echo "4a338000.pru1"  > /sys/bus/platform/drivers/pru-rproc/unbind 2> /dev/null
		echo "4a338000.pru1" > /sys/bus/platform/drivers/pru-rproc/bind
	

echo "Done."
