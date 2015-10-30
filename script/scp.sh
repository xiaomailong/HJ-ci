#!  /bin/bash

# set -x

function transfer()
{
    echo "transfer to ${1}"
    # scp -r /root/ci/src/platform/series_manage.* root@${1}:/root/ci/src/platform/
    # scp -r /root/ci/src/platform/cfg_compare.c root@${1}:/root/ci/src/platform/
    scp -r /root/driver/fiber/* root@${1}:/root/driver/fiber/
}

transfer 192.168.1.201
#transfer 192.168.1.202
#transfer 192.168.1.203
transfer 192.168.1.204
transfer 192.168.1.205


