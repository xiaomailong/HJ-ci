#!/bin/bash
#
# SCRIPT: profile of ci
# AUTHOR: zhangys
# DATE  : 20140320
# REV: 1.1.D (Valid are A, B, D, T and P)
# (For Alpha, Beta, Dev, Test and Production)
#
# PLATFORM: Linux only
#
# PURPOSE: device driver need by ci should be load at boot time.
#          we use this script to auto load it.
#
# REV LIST  : 1.1.D
# DATE      : 20140320
# BY        : zhangys
# MODIFICATION: create it
#
# REV LIST  : 1.2
# DATE      : 20150713
# BY        : zhangys
# MODIFICATION: add ntp date update
#
# set -n # Uncomment to check script syntax, without execution.
# # NOTE: Do not forget to put the comment back in or
# # the shell script will not execute!
# set -x # Uncomment to debug this shell script
#
##########################################################
# DEFINE FILES AND VARIABLES HERE
##########################################################

# update date
# bash /root/ci/script/ntpdate_update.sh

# load dpram_driver
DPRAM_HOME="/root/driver/cpudpram"
bash ${DPRAM_HOME}/dpram.sh start

# load fiber driver
FIBER_HOME="/root/driver/fiber"
bash ${FIBER_HOME}/fiber.sh start

# load net driver
NET_HOME="/root/driver/net"
bash ${NET_HOME}/net.sh start

# load uart
UART_HOME="/root/driver/uart"
bash ${UART_HOME}/uart.sh start

# load can
CAN_HOME="/root/driver/can"
bash ${CAN_HOME}/can.sh start

# watchdog
WATCHDOG_HOME="/root/driver/watchdog"
bash ${WATCHDOG_HOME}/lx3160_wdt.sh start

# led
LED_HOME="/root/driver/led/"
bash ${LED_HOME}/led.sh start

# ci_mem_sniffer
CI_MEM_SNIFFER_HOME="/root/CiMemSniffer/"
bash ${CI_MEM_SNIFFER_HOME}/driver/ci_mem_sniffer.sh start

# To avoid DOUBLE MASTER SERIES problem.master sleep 1min,slave sleep
# more time than master.
# bug: FPGA haven't init OK before ci start up if sleep time too short.
#    this will cause some problem,such as,HMI couldn't receive data,
#    but ci think it is work ok.
sleep 120

# run tail logger
nohup /root/TailLogger/taillogger > /dev/null&

# run ci at background
CI_HOME="/root/ci"
cd ${CI_HOME}/build
# copy core if system crashed
./tar.sh > /dev/null&
# run ci
./ci -a > /dev/null&

exit 0

