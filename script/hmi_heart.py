#!  /usr/bin/python
# -*- coding: utf-8 -*-
#
# author: zhangys
# date  : 20140310
# ver   : 0.1
#
# this script test for net board.if you want to know the format of packet head,
#  please read net board doc.
# please do it before you execute it
# 1.change your ip address to 192.168.1.3,otherwise you can't send message as 
#   well as recieve message.
# 2.make sure assigned msg[0] and msg[1] to 0x55 and 0xAA respectively.otherwise
#   SCM of W7100 will discard this packet.
# 3.make sure msg[10] is range from 1 to 10.because FPGA only allocate 10 block
#   to simulate the full duplex virtual network,if you assign other number at 
#   here,you will not be send and recieve successfully.
#

import socket
import struct
import time
from xxd import xxd_bin

addrs = [('192.168.1.11',3000),('192.168.2.11',3000),('192.168.1.10',3000),('192.168.2.10',3000)]

s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

size = 0xff

msg = [0x01 for i in range(size)]
#head
msg[0] = 0x55
msg[1] = 0xAA
msg[10] = 0x01
msg[15] = 0x0b
msg[16] = 0x00

b = struct.pack("%dB" % size ,*tuple(msg))

while True:
    for address in addrs:
        print("sendto {0}:{1}".format(*address))
        s.sendto(b, address)

    time.sleep(0.4)

s.close()
