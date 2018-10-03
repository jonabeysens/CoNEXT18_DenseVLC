#! /bin/bash

echo "-Turning off PRU pins"
	config-pin P8_27 lo # TX1
	config-pin P8_28 lo # TX1
	config-pin P8_39 lo # TX2
	config-pin P8_40 lo # TX2
	config-pin P8_41 lo # TX3
	config-pin P8_42 lo # TX3
	config-pin P8_43 lo # TX4
	config-pin P8_44 lo # TX4
	
	config-pin -q P8_27 # TX1
	config-pin -q P8_28 # TX1
	config-pin -q P8_39 # TX2
	config-pin -q P8_40 # TX2
	config-pin -q P8_41 # TX3
	config-pin -q P8_42 # TX3
	config-pin -q P8_43 # TX4
	config-pin -q P8_44 # TX4

	

