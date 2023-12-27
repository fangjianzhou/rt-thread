#!/usr/bin/expect
spawn picocom -b 115200 /dev/ttyUSB0
expect "U-Boot"
send " "
sleep 1
send " "
sleep 1
send " "
expect "0:Exit to console"
send "0"
expect "=>"
send "dhcp;setenv serverip 10.23.8.70;tftp 0x400000 rtthread.bin;tftp 0xa200000 uInitrd;tftp 0xa100000 OK3588-C-Linux.dtb;booti 0x400000 0xa200000 0xa100000\r"
expect "=>"
send "booti 0x400000 0xa200000 0xa100000\r"
interact
