#!/bin/bash
#
# SCRIPT: auto update system date from internet service
# AUTHOR: zhangys
# DATE: 20140306
# REV: 1.1.D (Valid are A, B, D, T and P)
# (For Alpha, Beta, Dev, Test and Production)
#
# PLATFORM: Linux only
#
# PURPOSE: because the Jumper of battery on PC104 is not work,
#       the date of system will be initialized to 01/01/2002,
#       which cause some program can't work correctly,such as
#       make,in order to avoid update date manually,we write this
#       script update date from internet when system is launch.
#
# REV LIST: 1.1.E
# DATE: 20140306
# BY: zhangys
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

for i in $(seq 2)
do
    echo "time:"$i

    ntpdate asia.pool.ntp.org

    ret=$?

    if [ ${ret} -eq 127 ]
    then
        echo "ntpdate not install,try install..."
        aptitude -y install ntpdate
    elif [ ${ret} -eq 0 ]
    then
        echo "time updated ok"
        break
    else
        # waiting for network interface up
        sleep 1
    fi
done

exit 0


