# RK3500 RT-Smart 使用说明

## 介绍

- 支持 RK3568、RK3588 等平台

## 编译

- 当前代码仓库没有指定 rt-thread 内核的路径，需要手动指定，如 创建一个 env 脚本，设置编译工具链路径 `RTT_EXEC_PATH` 与内核 `RTT_ROOT` 路径

```shell
#!/bin/bash

export RTT_CC=gcc
export RTT_EXEC_PATH=/home/zhangsz/smart/gnu_gcc/aarch64-linux-musleabi_for_x86_64-pc-linux-gnu/bin/
export RTT_CC_PREFIX=aarch64-linux-musleabi-

export PATH=$PATH:$RTT_EXEC_PATH
export RTT_ROOT=/home/zhangsz/smart/smart_dd2
```

## u-boot 引导设置

### rk3568 平台

- 默认开启 ofw 设备树框架才能正常工作

- 不同的开发板对应相应的设备树，如 RK3568 Nanopi5 开发板对应 `rk3568-nanopi5-rev02.dtb`

```shell
setenv bootcmd 'ext4load mmc 0:1 0x480000 /rtthread.bin;ext4load mmc 0:1 0x8300000 rk3568-nanopi5-rev02.dtb;booti 0x480000 - 0x8300000'
setenv bootargs 'earlycon=uart8250,mmio32,0xfe660000 console=ttyFIQ0'
saveenv
reset
```

### rk3588 平台

- 不同的开发板对应相应的设备树，如 RK3588 orangepi-5-plus 开发板对应 `rk3588-orangepi-5-plus.dtb`

```shell
setenv bootargs 'earlycon=uart8250,mmio32,0xfeb50000 console=ttyFIQ0'
setenv bootcmd 'ext4load mmc 0:1 0x480000 /rtthread.bin;ext4load mmc 0:1 0x8300000 rk3588-orangepi-5-plus.dtb;booti 0x480000 - 0x8300000'
saveenv
reset
```

## mmc 分区

- 分区类型： fat 或者 ext4，建议 ext4

- SD 卡或 emmc，除了用于普通的存储，也用于存储 rt-smart 固件 `rtthread.bin` 与 设备树 `xxx.dtb`，默认保存在 第一个分区

- u-boot 等 loader 默认不放置在文件系统分区内，而是采用 RAW 无分区处理， rk 平台 u-boot 存储于 mmc 设备 8MB 偏移位置，占 4MB 空间，
 因此 mmc 分区预留 12MB 的 RAW 分区（无分区）用于存储 u-boot 在内的 loader 镜像

- rt-smart 可以分为 kernel、rootfs、download 等分区，后续可以采用 A/B 分区方案，也就是增加备份恢复分区

## 其他

- 【问题描述】SD 启动时，引导类型是 MBR 而不是 GPT，此时无法设置 `partition name` 分区名等，u-boot 下无法使用 fastboot 进行烧录

> 【建议操作】使用工具把  SD 卡启动由 MBR 改为 GPT (EFI) 类型，使用 `gparted` 或 `fdisk` 重新分区

- 【问题描述】 rt-smart 无法启动，没有任何打印信息

> 【建议操作】 检查 控制台串口设置 `bootargs`，注意设备树不使用 u-boot 下的设备树，而使用完整的设备树，如 Linux kernel 中的



