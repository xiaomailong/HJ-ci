#!/bin/bash
#
# SCRIPT: tar.sh
# AUTHOR: zhangys
# DATE  : 20140928
# REV: 1.0.T (Valid are A, B, D, T and P)
# (For Alpha, Beta, Dev, Test and Production)
#
# PLATFORM: Linux only
#
# PURPOSE: For every time CI have crashed,most of time it will generate
#          core file,this file will be rewrite by next crash,in order
#          to avoid this situation be happen,we need this script to
#          save all these file.
#
# REV LIST      : 1.1.P
# DATE          : 20140928
# BY            : zhangys
# MODIFICATION  : create it
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

tar_prefix="ci_"
tar_suffix=".tar.gz"

tar_file_list="
          ./core
          ./cpu_peer_result_frame
          ./cpu_self_result_frame
          ./series_peer_result_frame
          ./series_self_result_frame
          "

counter=0

# desc   : Because LX3160 battery looks like ran out,BIOS time will
#          recount from 2002 year when system black out,so we use
#          a counter instead of `date` for our file suffix.
# param  : none
# return : status value,ok is 0,not ok is 1
function get_count()
{
    # reverse arrange all tar list file,its meaning first is max
    local max_counter_file=$(ls ${tar_prefix}* | sort -r | head -1)
    if [ "${max_counter_file}" == "" ]
    then
        # meaning no flle
        counter=1
        return 0
    else
        #echo ${max_counter_file}
        local count_with_suffix=$(echo "${max_counter_file}" | sed -rn "s/^${tar_prefix}//p")
        count=$(echo "${count_with_suffix}" | sed -rn "s/${tar_suffix}$//p")
        #echo ${count}
        if echo $count | egrep -q '^[0-9]+$'; then
            # $var is a number
            counter=$((${count} + 1))
            return 0
        else
            # $var isn't a number
            counter=1
            return 0
        fi
    fi

    return 1
}

# desc   : tar all file list in tar_file_list
# param  : none
# return : status value,ok is 0,not ok is 1
function tar_all_file()
{
    local file_name=${tar_prefix}${counter}${tar_suffix}
    local files=""
    for f in ${tar_file_list}
    do
        # if file exist,tar it
        if [ -f $f ]
        then
            files=${files}"${f} "
        fi
    done

    if [ "$files" == ""   ]
    then
        return 1
    else
        # add ./src/ci to this list
        files=${files}"./ci"
        tar -czf ${file_name} ${files}
    fi
}

# desc   : we don't need these file any more when finished tar
# param  : none
# return : status value,ok is 0,not ok is 1
function delete_all_file()
{
    for f in ${tar_file_list}
    do
        # if file exist,rm it
        if [ -f $f ]
        then
            rm -rf ${f}
        fi
    done
}

# desc   : main function
# param  : none
# return : status value,ok is 0,not ok is 1
function main()
{
    get_count
    tar_all_file
    delete_all_file

    return 0
}

main $*
exit 0
