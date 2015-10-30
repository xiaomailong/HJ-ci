#!/bin/Bash
#
# SCRIPT: build bootable device
# AUTHOR: zhangys
# DATE: 20140126
# REV: 1.1.A (Valid are A, B, D, T and P)
# (For Alpha, Beta, Dev, Test and Production)
#
# PLATFORM: Linux only
#
# PURPOSE: 联锁机上面使用CF卡作为硬盘，给这块硬盘上面安装一个操作系统可以有
#          有多种方案，但在emdebian当中推荐我们使用debootstrap或者multistrap
#          制作root filesystem方式安装操作系统，使用debootstrap安装系统需要
#          很多步骤，并且花费时间特别长，使用脚本将这些工作流程化，将会省下
#          很多时间。
#
# REV LIST: 1.1.
# DATE: 20140126
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


if []
then
fi
# pass in the block device file from parameter
dev_node=/dev/sdb4
mkfs -t ext4 $(dev_node)

mount_position=/mnt/usb1
mkdir $(mount_position)
mount -t ext4 $(dev_node) $(mount_position)

root_filesystem_file=/home/zhangys/kernel/emdebian-grip2-090306-x86-debootstrap-squeeze.tar.bz2
tar -jxvf $(root_filesystem_file) -C $(mount_position)

linux_kernel=/home/zhangys/kernel/linux-2.6.33.18.tar.bz2
linux_version=$(basename linux_kernel .tar.bz2)
tar -jxvf $(linux_kernel) -C $(mount_position)/usr/src/$(linux_version)

sudo LC_ALL=C chroot $(mount_position)
cd /usr/src/$(linux_version)
# config,make,install
make menuconfig
make
make modules
make modules_install
make install
# install grub
apt-get --reinstall install grub
update-grub
# update MBR
grub-install $(dev_node)
# change passwd
passwd root <<!
root
!<<



