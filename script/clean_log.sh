#!/bin/bash
#
# SCRIPT: clean sys log
# AUTHOR: zhangys
# DATE  : 20140320
# REV: 1.1.D (Valid are A, B, D, T and P)
# (For Alpha, Beta, Dev, Test and Production)
#
# PLATFORM: Linux only
#
# PURPOSE: LX3160 only has 8GB storage space,when system run
#          long time,it will generate too many log information.
#          we use this script to clean it.
#
# REV LIST  : 1.1.D
# DATE      : 20140320
# BY        : zhangys
# MODIFICATION: create it
#
#
# set -n # Uncomment to check script syntax, without execution.
# # NOTE: Do not forget to put the comment back in or
# # the shell script will not execute!
# set -x # Uncomment to debug this shell script
#
##########################################################
# DEFINE FILES AND VARIABLES HERE
##########################################################

rm -rf /var/log/*.gz
rm -rf /var/log/*.1
echo "" > /var/log/dmesg
echo "" > /var/log/kern.log
echo "" > /var/log/messages
echo "" > /var/log/syslog

exit 0

