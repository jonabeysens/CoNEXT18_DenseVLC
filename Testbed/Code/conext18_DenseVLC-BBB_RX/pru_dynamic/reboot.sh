#! /bin/bash
echo "-Rebooting"
	
echo "Rebooting pru-core 0"
echo "4a334000.pru0" > /sys/bus/platform/drivers/pru-rproc/unbind 2>/dev/null
echo "4a334000.pru0" > /sys/bus/platform/drivers/pru-rproc/bind

echo "Rebooting pru-core 1"
echo "4a338000.pru1" > /sys/bus/platform/drivers/pru-rproc/unbind 2>/dev/null
echo "4a338000.pru1" > /sys/bus/platform/drivers/pru-rproc/bind