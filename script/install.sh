#!/bin/bash
#
# SCRIPT: install.sh
# AUTHOR: zhangys
# DATE  : 20150428
# REV: 1.0.T (Valid are A, B, D, T and P)
# (For Alpha, Beta, Dev, Test and Production)
#
# PLATFORM: Linux only
#
# PURPOSE: this script make its easy to install ci to very PC104
#
# REV LIST      : 1.1.P
# DATE          : 20150428
# BY            : zhangys
# MODIFICATION  : create it
#
#
#set -n # Uncomment to check script syntax, without execution.
# # NOTE: Do not forget to put the comment back in or
# # the shell script will not execute!
# set -x # Uncomment to debug this shell script
#
##########################################################
# DEFINE FILES AND VARIABLES HERE
##########################################################


series_state=""
cpu_state=""

CI_HOME=/root/ci/
DRIVER_HOME=/root/driver

# desc   : copy ci profile
# param  : none
# return : none
function install_ci_profile()
{
    ci_profile_source_path=""
    ci_profile_dist_path=${CI_HOME}/script/ci_profile.sh

    # copy ci_profile.sh
    if [ "master" = "${series_state}" ]
    then
        ci_profile_source_path=${CI_HOME}/script/ci_profile_master.sh
        [ -f ${ci_profile_source_path} ] || (echo "${ci_profile_source_path} not exist"; return 1)
    elif [ "spare" = "${series_state}" ]
    then
        ci_profile_source_path=${CI_HOME}/script/ci_profile_spare.sh
        [ -f ${ci_profile_source_path} ] || (echo "${ci_profile_source_path} not exist"; return 1)
    fi

    echo "${ci_profile_source_path}=>${ci_profile_dist_path}"
    cp ${ci_profile_source_path} ${ci_profile_dist_path}

    return 0
}

# desc   : copy bashrc
# param  : none
# return : none
function install_bashrc()
{
    bashrc_source_path=${CI_HOME}/script/bashrc_sample
    bashrc_dist_path=~/.bashrc

    echo "${bashrc_source_path}=>${bashrc_dist_path}"

    [ -f ${bashrc_source_path} ] || (echo "${bashrc_source_path} not exist"; return 1)

    cp ${bashrc_source_path} ${bashrc_dist_path}

    return 0
}
# desc   : copy rc local
# param  : none
# return : none
function install_rc_local()
{
    rc_local_source_path=${CI_HOME}/script/rc.local.example
    rc_local_dist_path=/etc/rc.local

    echo "${rc_local_source_path}=>${rc_local_dist_path}"

    [ -f ${rc_local_source_path} ] || (echo "${rc_local_source_path} not exist"; return 1)

    cp ${rc_local_source_path} ${rc_local_dist_path}

    return 0
}
# desc   : copy cpudpram_sh
# param  : none
# return : none
function install_cpudpram_sh()
{
    cpudpram_source_path=""
    cpudpram_dist_path=${DRIVER_HOME}/cpudpram/dpram.sh

    if [ "master" = ${cpu_state} ]
    then
        cpudpram_source_path=${DRIVER_HOME}/cpudpram/dpram_master.sh
        [ -f ${cpudpram_source_path} ] || (echo "${cpudpram_source_path} not exist"; return 1)
    elif [ "slave" = ${cpu_state} ]
    then
        cpudpram_source_path=${DRIVER_HOME}/cpudpram/dpram_slave.sh
        [ -f ${cpudpram_source_path} ] || (echo "${cpudpram_source_path} not exist"; return 1)
    fi

    echo "${cpudpram_source_path}=>${cpudpram_dist_path}"
    cp ${cpudpram_source_path} ${cpudpram_dist_path}

    return 0
}
# desc   : copy tar.sh
# param  : none
# return : none
function install_tar()
{
    tar_source_path=${CI_HOME}/script/tar.sh
    tar_dist_path=${CI_HOME}/build/tar.sh

    echo "${tar_source_path}=>${tar_dist_path}"

    [ -f ${tar_source_path} ] || (echo "${tar_source_path} not exist"; return 1)

    cp ${tar_source_path} ${tar_dist_path}
}
# desc   : do install
# param  : none
# return : none
function do_install()
{
    install_rc_local || (echo "install rc local fail";return 1)
    install_bashrc || (echo "install bashrc fail";return 1)
    install_ci_profile || (echo "install ci_profile.sh fail";return 1)
    install_cpudpram_sh || (echo "install cpudpram.sh fail";return 1)
    install_tar || (echo "install tar.sh fail";return 1)

    return 0
}

# desc   : main function
# param  : none
# return : status value,ok is 0,not ok is 1
function main()
{
    if [ $# -lt 2 ]
    then
        echo "Usage: $0 {master|spare} {master|slave}"
        exit 1
    fi

    case "$1" in
        master)
            echo  "series state:master"
            series_state="master"
            ;;
        spare)
            echo  "series state:spare"
            series_state="spare"
            ;;
        *)
            echo "Usage: $0 {master|spare} {master|slave}"
            exit 1
    esac

    case "$2" in
        master)
            echo  "cpu state:master"
            cpu_state="master"
            ;;
        slave)
            echo  "cpu state:slave"
            cpu_state="slave"
            ;;
        *)
            echo "Usage: $0 {master|spare} {master|slave}"
            exit 1

    esac

    # finished parse parameter,install now
    do_install

    echo "install finished."
    echo "but configure of ${CI_HOME}/settings/platform.xml need you copy yourself"
    return 0
}

main $*

exit 0
