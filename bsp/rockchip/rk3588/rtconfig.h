#ifndef RT_CONFIG_H__
#define RT_CONFIG_H__

/* Automatically generated file; DO NOT EDIT. */
/* RT-Thread Project Configuration */

/* RT-Thread Kernel */

#define RT_NAME_MAX 16
#define RT_USING_SMART
#define RT_USING_SMP
#define RT_CPUS_NR 8
#define RT_ALIGN_SIZE 8
#define RT_THREAD_PRIORITY_32
#define RT_THREAD_PRIORITY_MAX 32
#define RT_TICK_PER_SECOND 100
#define RT_USING_OVERFLOW_CHECK
#define RT_USING_HOOK
#define RT_HOOK_USING_FUNC_PTR
#define RT_USING_IDLE_HOOK
#define RT_IDLE_HOOK_LIST_SIZE 4
#define IDLE_THREAD_STACK_SIZE 4096
#define SYSTEM_THREAD_STACK_SIZE 4096
#define RT_USING_TIMER_SOFT
#define RT_TIMER_THREAD_PRIO 4
#define RT_TIMER_THREAD_STACK_SIZE 4096

/* kservice optimization */

#define RT_KPRINTF_USING_LONGLONG
#define RT_USING_DEBUG
#define RT_DEBUGING_COLOR
#define RT_DEBUGING_CONTEXT

/* Inter-Thread communication */

#define RT_USING_SEMAPHORE
#define RT_USING_MUTEX
#define RT_USING_EVENT
#define RT_USING_MAILBOX
#define RT_USING_MESSAGEQUEUE

/* Memory Management */

#define RT_PAGE_MAX_ORDER 11
#define RT_USING_MEMPOOL
#define RT_USING_SMALL_MEM
#define RT_USING_MEMHEAP
#define RT_MEMHEAP_FAST_MODE
#define RT_USING_SMALL_MEM_AS_HEAP
#define RT_USING_MEMTRACE
#define RT_USING_HEAP
#define RT_USING_DEVICE
#define RT_USING_DEVICE_OPS
#define RT_USING_INTERRUPT_INFO
#define RT_USING_CONSOLE
#define RT_CONSOLEBUF_SIZE 256
#define RT_CONSOLE_DEVICE_NAME "uart2"
#define RT_VER_NUM 0x50100
#define RT_USING_STDC_ATOMIC
#define RT_BACKTRACE_LEVEL_MAX_NR 32

/* AArch64 Architecture Configuration */

#define ARCH_TEXT_OFFSET 0x480000
#define ARCH_RAM_OFFSET 0x200000
#define ARCH_SECONDARY_CPU_STACK_SIZE 4096
#define ARCH_HAVE_EFFICIENT_UNALIGNED_ACCESS
#define ARCH_CPU_64BIT
#define RT_USING_CACHE
#define RT_USING_CPU_FFS
#define ARCH_MM_MMU
#define ARCH_ARM
#define ARCH_ARM_MMU
#define KERNEL_VADDR_START 0xffff000000000000
#define ARCH_ARMV8

/* RT-Thread Components */

#define RT_USING_COMPONENTS_INIT
#define RT_USING_USER_MAIN
#define RT_MAIN_THREAD_STACK_SIZE 8192
#define RT_MAIN_THREAD_PRIORITY 10
#define RT_USING_MSH
#define RT_USING_FINSH
#define FINSH_USING_MSH
#define FINSH_THREAD_NAME "tshell"
#define FINSH_THREAD_PRIORITY 20
#define FINSH_THREAD_STACK_SIZE 8192
#define FINSH_USING_HISTORY
#define FINSH_HISTORY_LINES 5
#define FINSH_USING_SYMTAB
#define FINSH_CMD_SIZE 80
#define MSH_USING_BUILT_IN_COMMANDS
#define FINSH_USING_DESCRIPTION
#define FINSH_ARG_MAX 10
#define FINSH_USING_OPTION_COMPLETION

/* DFS: device virtual file system */

#define RT_USING_DFS
#define DFS_USING_POSIX
#define DFS_USING_WORKDIR
#define DFS_FD_MAX 80
#define RT_USING_DFS_V2
#define RT_USING_DFS_ELMFAT

/* elm-chan's FatFs, Generic FAT Filesystem Module */

#define RT_DFS_ELM_CODE_PAGE 437
#define RT_DFS_ELM_WORD_ACCESS
#define RT_DFS_ELM_USE_LFN_3
#define RT_DFS_ELM_USE_LFN 3
#define RT_DFS_ELM_LFN_UNICODE_0
#define RT_DFS_ELM_LFN_UNICODE 0
#define RT_DFS_ELM_MAX_LFN 255
#define RT_DFS_ELM_DRIVES 2
#define RT_DFS_ELM_MAX_SECTOR_SIZE 512
#define RT_DFS_ELM_REENTRANT
#define RT_DFS_ELM_MUTEX_TIMEOUT 3000
#define RT_USING_DFS_DEVFS
#define RT_USING_DFS_PTYFS
#define RT_USING_PAGECACHE

/* page cache config */

#define RT_PAGECACHE_COUNT 4096
#define RT_PAGECACHE_ASPACE_COUNT 1024
#define RT_PAGECACHE_PRELOAD 4
#define RT_PAGECACHE_HASH_NR 1024
#define RT_PAGECACHE_GC_WORK_LEVEL 90
#define RT_PAGECACHE_GC_STOP_LEVEL 70

/* Device Drivers */

#define RT_USING_DM
#define RT_USING_DEVICE_IPC
#define RT_UNAMED_PIPE_NUMBER 64
#define RT_USING_SYSTEM_WORKQUEUE
#define RT_SYSTEM_WORKQUEUE_STACKSIZE 8192
#define RT_SYSTEM_WORKQUEUE_PRIORITY 23
#define RT_USING_NULL
#define RT_USING_ZERO
#define RT_USING_RANDOM
#define RT_USING_ADC
#define RT_ADC_ROCKCHIP_SARADC
#define RT_USING_CAN
#define RT_CAN_USING_CANFD
#define RT_CAN_CANFD_ROCKCHIP
#define RT_USING_CLK
#define RT_CLK_ROCKCHIP
#define RT_CLK_ROCKCHIP_RK3588
#define RT_USING_FIRMWARE
#define RT_FIRMWARE_PSCI
#define RT_USING_HWCRYPTO
#define RT_HWCRYPTO_DEFAULT_NAME "hwcryto"
#define RT_HWCRYPTO_IV_MAX_SIZE 16
#define RT_HWCRYPTO_KEYBIT_MAX_SIZE 256
#define RT_HWCRYPTO_USING_RNG
#define RT_HWCRYPTO_RNG_ROCKCHIP
#define RT_USING_HWTIMER
#define RT_HWTIMER_ARM_ARCH
#define RT_HWTIMER_ROCKCHIP
#define RT_USING_I2C
#define RT_USING_I2C_BITOPS
#define RT_I2C_RK3X
#define RT_USING_MFD
#define RT_MFD_SYSCON
#define RT_MFD_RK8XX
#define RT_MFD_RK8XX_I2C
#define RT_USING_OFW
#define RT_FDT_EARLYCON_MSG_SIZE 128
#define RT_USING_PHY
#define RT_USING_PIC
#define MAX_HANDLERS 1024
#define RT_PIC_ARM_GIC_V3
#define RT_USING_PINCTRL
#define RT_PINCTRL_ROCKCHIP
#define RT_USING_PIN
#define RT_PIN_ROCKCHIP
#define RT_USING_PM
#define PM_TICKLESS_THRESHOLD_TIME 2
#define RT_USING_PWM
#define RT_PWM_ROCKCHIP
#define RT_USING_REGULATOR
#define RT_REGULATOR_GPIO
#define RT_REGULATOR_RK8XX
#define RT_USING_RESET
#define RT_USING_RTC
#define RT_USING_ALARM
#define RT_RTC_HYM8563
#define RT_RTC_RK8XX
#define RT_USING_SDIO
#define RT_SDIO_STACK_SIZE 8192
#define RT_SDIO_THREAD_PRIORITY 15
#define RT_MMCSD_STACK_SIZE 8192
#define RT_MMCSD_THREAD_PREORITY 22
#define RT_MMCSD_MAX_PARTITION 16
#define RT_SDIO_SDHCI_DWCMSHC
#define RT_SDIO_DW_MMC
#define RT_SDIO_DW_MMC_ROCKCHIP
#define RT_USING_SENSOR
#define RT_USING_SENSOR_CMD
#define RT_ADC_ROCKCHIP_TSADC
#define RT_USING_SERIAL
#define RT_USING_SERIAL_V1
#define RT_SERIAL_RB_BUFSZ 64
#define RT_SERIAL_8250
#define RT_SERIAL_8250_DW
#define RT_USING_SOC
#define RT_SOC_ROCKCHIP
#define RT_SOC_ROCKCHIP_FIQ_DEBUGGER
#define RT_SOC_ROCKCHIP_IODOMAIN
#define RT_USING_SPI
#define RT_SPI_ROCKCHIP
#define RT_USING_WDT
#define RT_WDT_DW
#define RT_USING_NET
#define RT_USING_DWC_EQOS
#define RT_USING_DWC_EQOS_ROCKCHIP

/* Using USB */


/* C/C++ and POSIX layer */

/* ISO-ANSI C layer */

/* Timezone and Daylight Saving Time */

#define RT_LIBC_USING_LIGHT_TZ_DST
#define RT_LIBC_TZ_DEFAULT_HOUR 8
#define RT_LIBC_TZ_DEFAULT_MIN 0
#define RT_LIBC_TZ_DEFAULT_SEC 0

/* POSIX (Portable Operating System Interface) layer */

#define RT_USING_POSIX_FS
#define RT_USING_POSIX_DEVIO
#define RT_USING_POSIX_STDIO
#define RT_USING_POSIX_POLL
#define RT_USING_POSIX_SELECT
#define RT_USING_POSIX_EVENTFD
#define RT_USING_POSIX_EPOLL
#define RT_USING_POSIX_SIGNALFD
#define RT_SIGNALFD_MAX_NUM 10
#define RT_USING_POSIX_TIMERFD
#define RT_USING_POSIX_SOCKET
#define RT_USING_POSIX_TERMIOS
#define RT_USING_POSIX_DELAY
#define RT_USING_POSIX_CLOCK
#define RT_USING_POSIX_TIMER

/* Interprocess Communication (IPC) */


/* Socket is in the 'Network' category */


/* Network */

#define RT_USING_SAL
#define SAL_INTERNET_CHECK

/* Docking with protocol stacks */

#define SAL_USING_LWIP
#define SAL_USING_POSIX
#define RT_USING_NETDEV
#define NETDEV_USING_IFCONFIG
#define NETDEV_USING_PING
#define NETDEV_USING_NETSTAT
#define NETDEV_USING_AUTO_DEFAULT
#define NETDEV_IPV4 1
#define NETDEV_IPV6 0
#define RT_USING_LWIP
#define RT_USING_LWIP212
#define RT_USING_LWIP_VER_NUM 0x20102
#define RT_LWIP_MEM_ALIGNMENT 8
#define RT_LWIP_IGMP
#define RT_LWIP_ICMP
#define RT_LWIP_DNS
#define RT_LWIP_DHCP
#define IP_SOF_BROADCAST 1
#define IP_SOF_BROADCAST_RECV 1

/* Static IPv4 Address */

#define RT_LWIP_IPADDR "192.168.1.30"
#define RT_LWIP_GWADDR "192.168.1.1"
#define RT_LWIP_MSKADDR "255.255.255.0"
#define RT_LWIP_UDP
#define RT_LWIP_TCP
#define RT_LWIP_RAW
#define RT_MEMP_NUM_NETCONN 64
#define RT_LWIP_PBUF_NUM 256
#define RT_LWIP_RAW_PCB_NUM 4
#define RT_LWIP_UDP_PCB_NUM 4
#define RT_LWIP_TCP_PCB_NUM 4
#define RT_LWIP_TCP_SEG_NUM 40
#define RT_LWIP_TCP_SND_BUF 8196
#define RT_LWIP_TCP_WND 49152
#define RT_LWIP_TCPTHREAD_PRIORITY 10
#define RT_LWIP_TCPTHREAD_MBOX_SIZE 64
#define RT_LWIP_TCPTHREAD_STACKSIZE 8192
#define RT_LWIP_ETHTHREAD_PRIORITY 12
#define RT_LWIP_ETHTHREAD_STACKSIZE 65536
#define RT_LWIP_ETHTHREAD_MBOX_SIZE 64
#define LWIP_NETIF_STATUS_CALLBACK 1
#define LWIP_NETIF_LINK_CALLBACK 1
#define RT_LWIP_NETIF_NAMESIZE 6
#define SO_REUSE 1
#define LWIP_SO_RCVTIMEO 1
#define LWIP_SO_SNDTIMEO 1
#define LWIP_SO_RCVBUF 1
#define LWIP_SO_LINGER 0
#define LWIP_NETIF_LOOPBACK 0
#define RT_LWIP_USING_PING

/* Memory protection */


/* Utilities */

#define RT_USING_ADT
#define RT_USING_RESOURCE_ID
#define RT_USING_LWP
#define LWP_DEBUG
#define RT_LWP_MAX_NR 30
#define LWP_TASK_STACK_SIZE 16384
#define RT_CH_MSG_MAX_NR 1024
#define LWP_CONSOLE_INPUT_BUFFER_SIZE 1024
#define LWP_TID_MAX_NR 64
#define RT_LWP_SHM_MAX_NR 64
#define RT_USING_LDSO
#define LWP_USING_TERMINAL
#define LWP_PTY_MAX_PARIS_LIMIT 64

/* Memory management */


/* RT-Thread Utestcases */


/* RT-Thread online packages */

/* IoT - internet of things */


/* Wi-Fi */

/* Marvell WiFi */


/* Wiced WiFi */


/* CYW43012 WiFi */


/* BL808 WiFi */


/* CYW43439 WiFi */

#define PKG_USING_NETUTILS
#define PKG_NETUTILS_IPERF
#define PKG_USING_NETUTILS_LATEST_VERSION
#define PKG_NETUTILS_VER_NUM 0x99999

/* IoT Cloud */


/* security packages */


/* language packages */

/* JSON: JavaScript Object Notation, a lightweight data-interchange format */


/* XML: Extensible Markup Language */


/* multimedia packages */

/* LVGL: powerful and easy-to-use embedded GUI library */


/* u8g2: a monochrome graphic library */


/* tools packages */


/* system packages */

/* enhanced kernel services */

#define PKG_USING_RT_VSNPRINTF_FULL
#define PKG_VSNPRINTF_SUPPORT_DECIMAL_SPECIFIERS
#define PKG_VSNPRINTF_SUPPORT_EXPONENTIAL_SPECIFIERS
#define PKG_VSNPRINTF_SUPPORT_WRITEBACK_SPECIFIER
#define PKG_VSNPRINTF_SUPPORT_LONG_LONG
#define PKG_VSNPRINTF_CHECK_FOR_NUL_IN_FORMAT_SPECIFIER
#define PKG_VSNPRINTF_INTEGER_BUFFER_SIZE 32
#define PKG_VSNPRINTF_DECIMAL_BUFFER_SIZE 32
#define PKG_VSNPRINTF_DEFAULT_FLOAT_PRECISION 6
#define PKG_VSNPRINTF_MAX_INTEGRAL_DIGITS_FOR_DECIMAL 9
#define PKG_VSNPRINTF_LOG10_TAYLOR_TERMS 4
#define PKG_USING_RT_VSNPRINTF_FULL_LATEST_VERSION

/* acceleration: Assembly language or algorithmic acceleration packages */


/* CMSIS: ARM Cortex-M Microcontroller Software Interface Standard */


/* Micrium: Micrium software products porting for RT-Thread */


/* peripheral libraries and drivers */

/* sensors drivers */


/* touch drivers */


/* Kendryte SDK */


/* AI packages */


/* Signal Processing and Control Algorithm Packages */


/* miscellaneous packages */

/* project laboratory */

/* samples: kernel and components samples */


/* entertainment: terminal games and other interesting software packages */


/* Arduino libraries */


/* Projects and Demos */


/* Sensors */


/* Display */


/* Timing */


/* Data Processing */


/* Data Storage */

/* Communication */


/* Device Control */


/* Other */


/* Signal IO */


/* Uncategorized */

#define SOC_RK3568

#endif
