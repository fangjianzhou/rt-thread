#!/usr/bin/python
# -*- coding: utf-8 -*-
import os,sys

opt=sys.argv

graphic_cfg=""" \
	-serial stdio -device ramfb \
	-device virtio-gpu-device,xres=800,yres=600 \
	-device virtio-keyboard-device \
	-device virtio-mouse-device \
	-device virtio-tablet-device \
"""

q_gic=2
q_dumpdtb=""
q_el=1
q_smp=4
q_mem=128
q_graphic="-nographic"
q_debug=""
q_bootargs="console=ttyAMA0 earlycon root=block0 rootfstype=elm rootwait rw"
q_initrd=""
q_sd="sd.bin"
q_net="user,id=net0"
q_flash="flash.bin"

def is_opt(key, inkey):
	if str("-"+key) == inkey:
		return True
	return False

for i in range(len(opt)):
	if i == 0:
		continue
	inkey=opt[i]

	if is_opt("gic", inkey): q_gic=int(opt[i+1])
	if is_opt("dumpdtb", inkey): q_dumpdtb=str(",dumpdtb=" + opt[i+1])
	if is_opt("el", inkey): q_el=int(opt[i+1])
	if is_opt("smp", inkey): q_smp=int(opt[i+1])
	if is_opt("mem", inkey): q_mem=int(opt[i+1])
	if is_opt("debug", inkey): q_debug="-S -s"
	if is_opt("bootargs", inkey): q_bootargs=opt[i+1]
	if is_opt("initrd", inkey): q_initrd=str("-initrd " + opt[i+1])
	if is_opt("graphic", inkey): q_graphic=graphic_cfg
	if is_opt("sd", inkey): q_sd=opt[i+1]
	if is_opt("tap", inkey): q_net="tap,id=net0,ifname=tap0"
	if is_opt("flash", inkey): q_flash=opt[i+1]

if q_smp > 8:
	q_gic=3

if q_el == 1:
	q_el=""
elif q_el == 2:
	q_el=",virtualization=on"
	if q_gic == 3:
		q_gic="max"
elif q_el == 3:
	q_el=",secure=on"
else:
	print("Invalid -el {}".format(q_el));
	exit(0)

bin_list = [q_sd, q_flash]

if sys.platform.startswith('win'):
	for bin in bin_list:
		if not os.path.exists(bin):
			os.system("qemu-img create -f raw {} 64M".format(bin))
else:
	for bin in bin_list:
		if not os.path.exists(bin):
			os.system("dd if=/dev/zero of={} bs=1024 count=65536".format(bin))

os.system("""
qemu-system-aarch64 \
	-M virt,acpi=on,iommu=smmuv3,its=on,gic-version={}{}{} \
	-cpu max \
	-smp {} \
	-m {} \
	-kernel rtthread.bin \
	-append "{}" \
	{} \
	{} \
	{} \
	-drive if=none,file={},format=raw,id=blk0 \
		-device virtio-blk-device,drive=blk0 \
	-netdev {} \
		-device virtio-net-device,netdev=net0 \
	-device virtio-rng-device \
	-device intel-hda \
	-device hda-duplex \
	-drive file={},format=raw,if=pflash,index=1 \
	-device virtio-serial-device \
		-chardev socket,host=127.0.0.1,port=4321,server=on,wait=off,telnet=on,id=console0 \
		-device virtserialport,chardev=console0 \
	-device pci-serial,chardev=console1 \
		-chardev socket,host=127.0.0.1,port=4322,server=on,wait=off,telnet=on,id=console1
""".format(q_gic, q_dumpdtb, q_el, q_smp, q_mem, q_bootargs, q_initrd, q_graphic, q_debug, q_sd, q_net, q_flash))

if len(q_dumpdtb) != 0:
	dtb=q_dumpdtb.split('=')[-1]
	os.system("dtc -I dtb -O dts -@ -A {} -o {}".format(dtb, dtb.replace(".dtb", ".dts")))
