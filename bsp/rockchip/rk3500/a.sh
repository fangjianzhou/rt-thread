#!/usr/bin/bash
sudo mount /dev/sdb5 ~/sd
sudo cp ./rtthread.bin ~/sd/rtthread.bin
ls -l ~/sd
sync
sudo umount ~/sd
