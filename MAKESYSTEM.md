# 找到磁盘
ll /dev/ | grep sd
# 磁盘分区
fdisk /dev/sdc <<!
p
d
n
 
 
w
 
!
# 格式化磁盘
sudo ls /sbin/mk*
sudo /sbin/mkfs.ext2 -L / -I 128 -F -j -O dir_index /dev/sdc1
# 挂载磁盘
sudo mount -t ext2 /dev/sdc1 /mnt/usb1
cd /mnt/usb1

sudo tar -jxvf ~/kernel/emdebian-grip2-090306-x86-debootstrap-squeeze.tar.bz2 -C /mnt/usb1/

export LC_ALL=C LANGUAGE=C LANG=C 
  
sudo cp /etc/vim/vimrc /mnt/usb1/etc/vim/vimrc
# 进入root filesystem进行配置
sudo mount -o bind /proc proc
sudo mount -o bind /dev dev
sudo mount -o bind /dev/pts dev/pts
sudo mount -o bind /sys sys
sudo chroot ./
# 重新更新dpkg
dpkg-reconfigure -a
 
# 配置DNS服务器，负责无法访问网络
cat > /etc/resolv.conf
nameserver 192.168.1.1
nameserver 8.8.8.8
nameserver 8.8.4.4
^D
     
cat > /etc/hostname
primary 
^D

cat > /etc/network/interfaces
auto lo
iface lo inet loopback
auto eth0
iface eth0 inet static
address 192.168.1.203
netmask 255.255.255.0
gateway 192.168.1.1
network 192.168.1.1
dns-nameservers 192.168.1.1

^D
   
cat >> ~/.bashrc
umask 022
export LS_OPTIONS='--color=auto'
alias ls='ls $LS_OPTIONS'
alias ll='ls $LS_OPTIONS -l'
alias l='ls $LS_OPTIONS -lA'
alias rm='rm -i'
alias cp='cp -i'
alias mv='mv -i'
^D   
# 找到启动设备的uuid，并加入挂载目录
blkid
cat > /etc/fstab
UUID=94c3714b-69fb-4fac-b3dc-d8e652eaf669 / ext2 defaults 0 1
^D

cat >> /etc/apt/source.list
deb http://mirrors.163.com/debian/ squeeze main non-free contrib
deb http://mirrors.163.com/debian/ squeeze-proposed-updates main non-free contrib
deb-src http://mirrors.163.com/debian/ squeeze main non-free contrib
deb-src http://mirrors.163.com/debian/ squeeze-proposed-updates main non-free contrib
^D
aptitude update
aptitude -y install cmake make gcc gdb g++ python vim less ssh lsof libssl-dev telnet strace ntpdate tree htop

# 默认使用less
cat >> ~/.bashrc
export PAGER=less
^D

mkdir /boot/grub
cp /usr/share/zoneinfo/Asia/Shanghai /etc/localtime

apt-get install initramfs-tools 
mkinitramfs -k -o /boot/initrd.img-2.6.33.18 2.6.33.18
eixt
# 拷贝内核镜像并编译
sudo tar -jxvf ~/kernel/kernel_ioe.tar.bz2 -C /mnt/usb1/usr/src 
sudo apt-get install libncurses5-dev
make menuconfig
make
make modules
sudo chroot /mnt/usb1
cd /usr/src/linux-3.12.4
make modules_install
make install 
     
# 安装grub
apt-get remove grub
apt-get install grub2
update-grub
# 更新 MBR
grub-install /dev/sdc
# 更改passwd
passwd root <<!

zhang
!
    
sudo umount dev/pts
sudo umount dev
sudo umount sys
sudo umount proc
 
dd if=/dev/sdb of=/dev/sdc bs=4096
#编辑让eth从0重新开始
vi /etc/udev/rules.d/70-persistent-net.rules
#从大的镜像dd拷贝到小的磁盘出现的错误更正
#http://www.linuxquestions.org/questions/linux-hardware-18/size-in-superblock-is-different-from-the-physical-size-of-the-partition-298175/
resize2fs -f /dev/sdc1
#再到系统启动时进入维护模式fsck
fsck
reboot    
#其它说明
/etc/exports内有可清除项，应该删除掉
这些启动项对我们无用，可安全删除
rm  S01samba S14nfs-common S15nfs-kernel-server S13portmap
