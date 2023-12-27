/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-11-26     GuEe-GUI     first version
 */

#ifndef __RK8XX_H__
#define __RK8XX_H__

#include <rtthread.h>
#include <rtdevice.h>

/* CONFIG REGISTER */
#define RK805_VB_MON_REG                0x21
#define RK805_THERMAL_REG               0x22

/* POWER CHANNELS ENABLE REGISTER */
#define RK805_DCDC_EN_REG               0x23
#define RK805_SLP_DCDC_EN_REG           0x25
#define RK805_SLP_LDO_EN_REG            0x26
#define RK805_LDO_EN_REG                0x27

/* BUCK AND LDO CONFIG REGISTER */
#define RK805_BUCK_LDO_SLP_LP_EN_REG    0x2a
#define RK805_BUCK1_CONFIG_REG          0x2e
#define RK805_BUCK1_ON_VSEL_REG         0x2f
#define RK805_BUCK1_SLP_VSEL_REG        0x30
#define RK805_BUCK2_CONFIG_REG          0x32
#define RK805_BUCK2_ON_VSEL_REG         0x33
#define RK805_BUCK2_SLP_VSEL_REG        0x34
#define RK805_BUCK3_CONFIG_REG          0x36
#define RK805_BUCK4_CONFIG_REG          0x37
#define RK805_BUCK4_ON_VSEL_REG         0x38
#define RK805_BUCK4_SLP_VSEL_REG        0x39
#define RK805_LDO1_ON_VSEL_REG          0x3b
#define RK805_LDO1_SLP_VSEL_REG         0x3c
#define RK805_LDO2_ON_VSEL_REG          0x3d
#define RK805_LDO2_SLP_VSEL_REG         0x3e
#define RK805_LDO3_ON_VSEL_REG          0x3f
#define RK805_LDO3_SLP_VSEL_REG         0x40

#define RK806_POWER_EN0                 0x0
#define RK806_POWER_EN1                 0x1
#define RK806_POWER_EN2                 0x2
#define RK806_POWER_EN3                 0x3
#define RK806_POWER_EN4                 0x4
#define RK806_POWER_EN5                 0x5
#define RK806_POWER_SLP_EN0             0x6
#define RK806_POWER_SLP_EN1             0x7
#define RK806_POWER_SLP_EN2             0x8
#define RK806_POWER_DISCHRG_EN0         0x9
#define RK806_POWER_DISCHRG_EN1         0xa
#define RK806_POWER_DISCHRG_EN2         0xb
#define RK806_BUCK_FB_CONFIG            0xc
#define RK806_SLP_LP_CONFIG             0xd
#define RK806_POWER_FPWM_EN0            0xe
#define RK806_POWER_FPWM_EN1            0xf
#define RK806_BUCK1_CONFIG              0x10
#define RK806_BUCK2_CONFIG              0x11
#define RK806_BUCK3_CONFIG              0x12
#define RK806_BUCK4_CONFIG              0x13
#define RK806_BUCK5_CONFIG              0x14
#define RK806_BUCK6_CONFIG              0x15
#define RK806_BUCK7_CONFIG              0x16
#define RK806_BUCK8_CONFIG              0x17
#define RK806_BUCK9_CONFIG              0x18
#define RK806_BUCK10_CONFIG             0x19
#define RK806_BUCK1_ON_VSEL             0x1a
#define RK806_BUCK2_ON_VSEL             0x1b
#define RK806_BUCK3_ON_VSEL             0x1c
#define RK806_BUCK4_ON_VSEL             0x1d
#define RK806_BUCK5_ON_VSEL             0x1e
#define RK806_BUCK6_ON_VSEL             0x1f
#define RK806_BUCK7_ON_VSEL             0x20
#define RK806_BUCK8_ON_VSEL             0x21
#define RK806_BUCK9_ON_VSEL             0x22
#define RK806_BUCK10_ON_VSEL            0x23
#define RK806_BUCK1_SLP_VSEL            0x24
#define RK806_BUCK2_SLP_VSEL            0x25
#define RK806_BUCK3_SLP_VSEL            0x26
#define RK806_BUCK4_SLP_VSEL            0x27
#define RK806_BUCK5_SLP_VSEL            0x28
#define RK806_BUCK6_SLP_VSEL            0x29
#define RK806_BUCK7_SLP_VSEL            0x2a
#define RK806_BUCK8_SLP_VSEL            0x2b
#define RK806_BUCK9_SLP_VSEL            0x2d
#define RK806_BUCK10_SLP_VSEL           0x2e
#define RK806_BUCK_DEBUG1               0x30
#define RK806_BUCK_DEBUG2               0x31
#define RK806_BUCK_DEBUG3               0x32
#define RK806_BUCK_DEBUG4               0x33
#define RK806_BUCK_DEBUG5               0x34
#define RK806_BUCK_DEBUG6               0x35
#define RK806_BUCK_DEBUG7               0x36
#define RK806_BUCK_DEBUG8               0x37
#define RK806_BUCK_DEBUG9               0x38
#define RK806_BUCK_DEBUG10              0x39
#define RK806_BUCK_DEBUG11              0x3a
#define RK806_BUCK_DEBUG12              0x3b
#define RK806_BUCK_DEBUG13              0x3c
#define RK806_BUCK_DEBUG14              0x3d
#define RK806_BUCK_DEBUG15              0x3e
#define RK806_BUCK_DEBUG16              0x3f
#define RK806_BUCK_DEBUG17              0x40
#define RK806_BUCK_DEBUG18              0x41
#define RK806_NLDO_IMAX                 0x42
#define RK806_NLDO1_ON_VSEL             0x43
#define RK806_NLDO2_ON_VSEL             0x44
#define RK806_NLDO3_ON_VSEL             0x45
#define RK806_NLDO4_ON_VSEL             0x46
#define RK806_NLDO5_ON_VSEL             0x47
#define RK806_NLDO1_SLP_VSEL            0x48
#define RK806_NLDO2_SLP_VSEL            0x49
#define RK806_NLDO3_SLP_VSEL            0x4a
#define RK806_NLDO4_SLP_VSEL            0x4b
#define RK806_NLDO5_SLP_VSEL            0x4c
#define RK806_PLDO_IMAX                 0x4d
#define RK806_PLDO1_ON_VSEL             0x4e
#define RK806_PLDO2_ON_VSEL             0x4f
#define RK806_PLDO3_ON_VSEL             0x50
#define RK806_PLDO4_ON_VSEL             0x51
#define RK806_PLDO5_ON_VSEL             0x52
#define RK806_PLDO6_ON_VSEL             0x53
#define RK806_PLDO1_SLP_VSEL            0x54
#define RK806_PLDO2_SLP_VSEL            0x55
#define RK806_PLDO3_SLP_VSEL            0x56
#define RK806_PLDO4_SLP_VSEL            0x57
#define RK806_PLDO5_SLP_VSEL            0x58
#define RK806_PLDO6_SLP_VSEL            0x59
#define RK806_CHIP_NAME                 0x5a
#define RK806_CHIP_VER                  0x5b
#define RK806_OTP_VER                   0x5c
#define RK806_SYS_STS                   0x5d
#define RK806_SYS_CFG0                  0x5e
#define RK806_SYS_CFG1                  0x5f
#define RK806_SYS_OPTION                0x61
#define RK806_SLEEP_CONFIG0             0x62
#define RK806_SLEEP_CONFIG1             0x63
#define RK806_SLEEP_CTR_SEL0            0x64
#define RK806_SLEEP_CTR_SEL1            0x65
#define RK806_SLEEP_CTR_SEL2            0x66
#define RK806_SLEEP_CTR_SEL3            0x67
#define RK806_SLEEP_CTR_SEL4            0x68
#define RK806_SLEEP_CTR_SEL5            0x69
#define RK806_DVS_CTRL_SEL0             0x6a
#define RK806_DVS_CTRL_SEL1             0x6b
#define RK806_DVS_CTRL_SEL2             0x6c
#define RK806_DVS_CTRL_SEL3             0x6d
#define RK806_DVS_CTRL_SEL4             0x6e
#define RK806_DVS_CTRL_SEL5             0x6f
#define RK806_DVS_START_CTRL            0x70
#define RK806_SLEEP_GPIO                0x71
#define RK806_SYS_CFG3                  0x72
#define RK806_ON_SOURCE                 0x74
#define RK806_OFF_SOURCE                0x75
#define RK806_PWRON_KEY                 0x76
#define RK806_INT_STS0                  0x77
#define RK806_INT_MSK0                  0x78
#define RK806_INT_STS1                  0x79
#define RK806_INT_MSK1                  0x7a
#define RK806_GPIO_INT_CONFIG           0x7b
#define RK806_DATA_REG0                 0x7c
#define RK806_DATA_REG1                 0x7d
#define RK806_DATA_REG2                 0x7e
#define RK806_DATA_REG3                 0x7f
#define RK806_DATA_REG4                 0x80
#define RK806_DATA_REG5                 0x81
#define RK806_DATA_REG6                 0x82
#define RK806_DATA_REG7                 0x83
#define RK806_DATA_REG8                 0x84
#define RK806_DATA_REG9                 0x85
#define RK806_DATA_REG10                0x86
#define RK806_DATA_REG11                0x87
#define RK806_DATA_REG12                0x88
#define RK806_DATA_REG13                0x89
#define RK806_DATA_REG14                0x8a
#define RK806_DATA_REG15                0x8b
#define RK806_TM_REG                    0x8c
#define RK806_OTP_EN_REG                0x8d
#define RK806_FUNC_OTP_EN_REG           0x8e
#define RK806_TEST_REG1                 0x8f
#define RK806_TEST_REG2                 0x90
#define RK806_TEST_REG3                 0x91
#define RK806_TEST_REG4                 0x92
#define RK806_TEST_REG5                 0x93
#define RK806_BUCK_VSEL_OTP_REG0        0x94
#define RK806_BUCK_VSEL_OTP_REG1        0x95
#define RK806_BUCK_VSEL_OTP_REG2        0x96
#define RK806_BUCK_VSEL_OTP_REG3        0x97
#define RK806_BUCK_VSEL_OTP_REG4        0x98
#define RK806_BUCK_VSEL_OTP_REG5        0x99
#define RK806_BUCK_VSEL_OTP_REG6        0x9a
#define RK806_BUCK_VSEL_OTP_REG7        0x9b
#define RK806_BUCK_VSEL_OTP_REG8        0x9c
#define RK806_BUCK_VSEL_OTP_REG9        0x9d
#define RK806_NLDO1_VSEL_OTP_REG0       0x9e
#define RK806_NLDO1_VSEL_OTP_REG1       0x9f
#define RK806_NLDO1_VSEL_OTP_REG2       0xa0
#define RK806_NLDO1_VSEL_OTP_REG3       0xa1
#define RK806_NLDO1_VSEL_OTP_REG4       0xa2
#define RK806_PLDO_VSEL_OTP_REG0        0xa3
#define RK806_PLDO_VSEL_OTP_REG1        0xa4
#define RK806_PLDO_VSEL_OTP_REG2        0xa5
#define RK806_PLDO_VSEL_OTP_REG3        0xa6
#define RK806_PLDO_VSEL_OTP_REG4        0xa7
#define RK806_PLDO_VSEL_OTP_REG5        0xa8
#define RK806_BUCK_EN_OTP_REG1          0xa9
#define RK806_NLDO_EN_OTP_REG1          0xaa
#define RK806_PLDO_EN_OTP_REG1          0xab
#define RK806_BUCK_FB_RES_OTP_REG1      0xac
#define RK806_OTP_RESEV_REG0            0xad
#define RK806_OTP_RESEV_REG1            0xae
#define RK806_OTP_RESEV_REG2            0xaf
#define RK806_OTP_RESEV_REG3            0xb0
#define RK806_OTP_RESEV_REG4            0xb1
#define RK806_BUCK_SEQ_REG0             0xb2
#define RK806_BUCK_SEQ_REG1             0xb3
#define RK806_BUCK_SEQ_REG2             0xb4
#define RK806_BUCK_SEQ_REG3             0xb5
#define RK806_BUCK_SEQ_REG4             0xb6
#define RK806_BUCK_SEQ_REG5             0xb7
#define RK806_BUCK_SEQ_REG6             0xb8
#define RK806_BUCK_SEQ_REG7             0xb9
#define RK806_BUCK_SEQ_REG8             0xba
#define RK806_BUCK_SEQ_REG9             0xbb
#define RK806_BUCK_SEQ_REG10            0xbc
#define RK806_BUCK_SEQ_REG11            0xbd
#define RK806_BUCK_SEQ_REG12            0xbe
#define RK806_BUCK_SEQ_REG13            0xbf
#define RK806_BUCK_SEQ_REG14            0xc0
#define RK806_BUCK_SEQ_REG15            0xc1
#define RK806_BUCK_SEQ_REG16            0xc2
#define RK806_BUCK_SEQ_REG17            0xc3
#define RK806_HK_TRIM_REG1              0xc4
#define RK806_HK_TRIM_REG2              0xc5
#define RK806_BUCK_REF_TRIM_REG1        0xc6
#define RK806_BUCK_REF_TRIM_REG2        0xc7
#define RK806_BUCK_REF_TRIM_REG3        0xc8
#define RK806_BUCK_REF_TRIM_REG4        0xc9
#define RK806_BUCK_REF_TRIM_REG5        0xca
#define RK806_BUCK_OSC_TRIM_REG1        0xcb
#define RK806_BUCK_OSC_TRIM_REG2        0xcc
#define RK806_BUCK_OSC_TRIM_REG3        0xcd
#define RK806_BUCK_OSC_TRIM_REG4        0xce
#define RK806_BUCK_OSC_TRIM_REG5        0xcf
#define RK806_BUCK_TRIM_ZCDIOS_REG1     0xd0
#define RK806_BUCK_TRIM_ZCDIOS_REG2     0xd1
#define RK806_NLDO_TRIM_REG1            0xd2
#define RK806_NLDO_TRIM_REG2            0xd3
#define RK806_NLDO_TRIM_REG3            0xd4
#define RK806_PLDO_TRIM_REG1            0xd5
#define RK806_PLDO_TRIM_REG2            0xd6
#define RK806_PLDO_TRIM_REG3            0xd7
#define RK806_TRIM_ICOMP_REG1           0xd8
#define RK806_TRIM_ICOMP_REG2           0xd9
#define RK806_EFUSE_CONTROL_REGH        0xda
#define RK806_FUSE_PROG_REG             0xdb
#define RK806_MAIN_FSM_STS_REG          0xdd
#define RK806_FSM_REG                   0xde
#define RK806_TOP_RESEV_OFFR            0xec
#define RK806_TOP_RESEV_POR             0xed
#define RK806_BUCK_VRSN_REG1            0xee
#define RK806_BUCK_VRSN_REG2            0xef
#define RK806_NLDO_RLOAD_SEL_REG1       0xf0
#define RK806_PLDO_RLOAD_SEL_REG1       0xf1
#define RK806_PLDO_RLOAD_SEL_REG2       0xf2
#define RK806_BUCK_CMIN_MX_REG1         0xf3
#define RK806_BUCK_CMIN_MX_REG2         0xf4
#define RK806_BUCK_FREQ_SET_REG1        0xf5
#define RK806_BUCK_FREQ_SET_REG2        0xf6
#define RK806_BUCK_RS_MEABS_REG1        0xf7
#define RK806_BUCK_RS_MEABS_REG2        0xf8
#define RK806_BUCK_RS_ZDLEB_REG1        0xf9
#define RK806_BUCK_RS_ZDLEB_REG2        0xfa
#define RK806_BUCK_RSERVE_REG1          0xfb
#define RK806_BUCK_RSERVE_REG2          0xfc
#define RK806_BUCK_RSERVE_REG3          0xfd
#define RK806_BUCK_RSERVE_REG4          0xfe
#define RK806_BUCK_RSERVE_REG5          0xff

#define RK806_CMD_READ					0
#define RK806_CMD_WRITE					RT_BIT(7)
#define RK806_CMD_CRC_EN				RT_BIT(6)
#define RK806_CMD_CRC_DIS				0
#define RK806_CMD_LEN_MSK				0x0f
#define RK806_REG_H						0x00

#define RK808_SECONDS_REG               0x00
#define RK808_MINUTES_REG               0x01
#define RK808_HOURS_REG                 0x02
#define RK808_DAYS_REG                  0x03
#define RK808_MONTHS_REG                0x04
#define RK808_YEARS_REG                 0x05
#define RK808_WEEKS_REG                 0x06
#define RK808_ALARM_SECONDS_REG         0x08
#define RK808_ALARM_MINUTES_REG         0x09
#define RK808_ALARM_HOURS_REG           0x0a
#define RK808_ALARM_DAYS_REG            0x0b
#define RK808_ALARM_MONTHS_REG          0x0c
#define RK808_ALARM_YEARS_REG           0x0d
#define RK808_RTC_CTRL_REG              0x10
#define RK808_RTC_STATUS_REG            0x11
#define RK808_RTC_INT_REG               0x12
#define RK808_RTC_COMP_LSB_REG          0x13
#define RK808_RTC_COMP_MSB_REG          0x14
#define RK808_ID_MSB                    0x17
#define RK808_ID_LSB                    0x18
#define RK808_CLK32OUT_REG              0x20
#define RK808_VB_MON_REG                0x21
#define RK808_THERMAL_REG               0x22
#define RK808_DCDC_EN_REG               0x23
#define RK808_LDO_EN_REG                0x24
#define RK808_SLEEP_SET_OFF_REG1        0x25
#define RK808_SLEEP_SET_OFF_REG2        0x26
#define RK808_DCDC_UV_STS_REG           0x27
#define RK808_DCDC_UV_ACT_REG           0x28
#define RK808_LDO_UV_STS_REG            0x29
#define RK808_LDO_UV_ACT_REG            0x2a
#define RK808_DCDC_PG_REG               0x2b
#define RK808_LDO_PG_REG                0x2c
#define RK808_VOUT_MON_TDB_REG          0x2d
#define RK808_BUCK1_CONFIG_REG          0x2e
#define RK808_BUCK1_ON_VSEL_REG         0x2f
#define RK808_BUCK1_SLP_VSEL_REG        0x30
#define RK808_BUCK1_DVS_VSEL_REG        0x31
#define RK808_BUCK2_CONFIG_REG          0x32
#define RK808_BUCK2_ON_VSEL_REG         0x33
#define RK808_BUCK2_SLP_VSEL_REG        0x34
#define RK808_BUCK2_DVS_VSEL_REG        0x35
#define RK808_BUCK3_CONFIG_REG          0x36
#define RK808_BUCK4_CONFIG_REG          0x37
#define RK808_BUCK4_ON_VSEL_REG         0x38
#define RK808_BUCK4_SLP_VSEL_REG        0x39
#define RK808_BOOST_CONFIG_REG          0x3a
#define RK808_LDO1_ON_VSEL_REG          0x3b
#define RK808_LDO1_SLP_VSEL_REG         0x3c
#define RK808_LDO2_ON_VSEL_REG          0x3d
#define RK808_LDO2_SLP_VSEL_REG         0x3e
#define RK808_LDO3_ON_VSEL_REG          0x3f
#define RK808_LDO3_SLP_VSEL_REG         0x40
#define RK808_LDO4_ON_VSEL_REG          0x41
#define RK808_LDO4_SLP_VSEL_REG         0x42
#define RK808_LDO5_ON_VSEL_REG          0x43
#define RK808_LDO5_SLP_VSEL_REG         0x44
#define RK808_LDO6_ON_VSEL_REG          0x45
#define RK808_LDO6_SLP_VSEL_REG         0x46
#define RK808_LDO7_ON_VSEL_REG          0x47
#define RK808_LDO7_SLP_VSEL_REG         0x48
#define RK808_LDO8_ON_VSEL_REG          0x49
#define RK808_LDO8_SLP_VSEL_REG         0x4a
#define RK808_DEVCTRL_REG               0x4b
#define RK808_INT_STS_REG1              0x4c
#define RK808_INT_STS_MSK_REG1          0x4d
#define RK808_INT_STS_REG2              0x4e
#define RK808_INT_STS_MSK_REG2          0x4f
#define RK808_IO_POL_REG                0x50

#define RK809_BUCK5_CONFIG(i)           (RK817_BOOST_OTG_CFG + (i) * 1)

#define RK817_SECONDS_REG               0x00
#define RK817_MINUTES_REG               0x01
#define RK817_HOURS_REG                 0x02
#define RK817_DAYS_REG                  0x03
#define RK817_MONTHS_REG                0x04
#define RK817_YEARS_REG                 0x05
#define RK817_WEEKS_REG                 0x06
#define RK817_ALARM_SECONDS_REG         0x07
#define RK817_ALARM_MINUTES_REG         0x08
#define RK817_ALARM_HOURS_REG           0x09
#define RK817_ALARM_DAYS_REG            0x0a
#define RK817_ALARM_MONTHS_REG          0x0b
#define RK817_ALARM_YEARS_REG           0x0c
#define RK817_RTC_CTRL_REG              0xd
#define RK817_RTC_STATUS_REG            0xe
#define RK817_RTC_INT_REG               0xf
#define RK817_RTC_COMP_LSB_REG          0x10
#define RK817_RTC_COMP_MSB_REG          0x11

#define RK817_POWER_EN_REG(i)           (0xb1 + (i))
#define RK817_POWER_SLP_EN_REG(i)       (0xb5 + (i))

#define RK817_POWER_CONFIG              (0xb9)

#define RK817_BUCK_CONFIG_REG(i)        (0xba + (i) * 3)

#define RK817_BUCK1_ON_VSEL_REG         0xbb
#define RK817_BUCK1_SLP_VSEL_REG        0xbc

#define RK817_BUCK2_CONFIG_REG          0xbd
#define RK817_BUCK2_ON_VSEL_REG         0xbe
#define RK817_BUCK2_SLP_VSEL_REG        0xbf

#define RK817_BUCK3_CONFIG_REG          0xc0
#define RK817_BUCK3_ON_VSEL_REG         0xc1
#define RK817_BUCK3_SLP_VSEL_REG        0xc2

#define RK817_BUCK4_CONFIG_REG          0xc3
#define RK817_BUCK4_ON_VSEL_REG         0xc4
#define RK817_BUCK4_SLP_VSEL_REG        0xc5

#define RK817_LDO_ON_VSEL_REG(idx)      (0xcc + (idx) * 2)
#define RK817_BOOST_OTG_CFG             (0xde)

enum rk817_reg_id
{
    RK817_ID_DCDC1 = 0,
    RK817_ID_DCDC2,
    RK817_ID_DCDC3,
    RK817_ID_DCDC4,
    RK817_ID_LDO1,
    RK817_ID_LDO2,
    RK817_ID_LDO3,
    RK817_ID_LDO4,
    RK817_ID_LDO5,
    RK817_ID_LDO6,
    RK817_ID_LDO7,
    RK817_ID_LDO8,
    RK817_ID_LDO9,
    RK817_ID_BOOST,
    RK817_ID_BOOST_OTG_SW,
    RK817_NUM_REGULATORS
};

#define RK818_DCDC1                     0
#define RK818_LDO1                      4
#define RK818_NUM_REGULATORS            17

enum rk818_reg
{
    RK818_ID_DCDC1,
    RK818_ID_DCDC2,
    RK818_ID_DCDC3,
    RK818_ID_DCDC4,
    RK818_ID_BOOST,
    RK818_ID_LDO1,
    RK818_ID_LDO2,
    RK818_ID_LDO3,
    RK818_ID_LDO4,
    RK818_ID_LDO5,
    RK818_ID_LDO6,
    RK818_ID_LDO7,
    RK818_ID_LDO8,
    RK818_ID_LDO9,
    RK818_ID_SWITCH,
    RK818_ID_HDMI_SWITCH,
    RK818_ID_OTG_SWITCH,
};

#define RK818_DCDC_EN_REG               0x23
#define RK818_LDO_EN_REG                0x24
#define RK818_SLEEP_SET_OFF_REG1        0x25
#define RK818_SLEEP_SET_OFF_REG2        0x26
#define RK818_DCDC_UV_STS_REG           0x27
#define RK818_DCDC_UV_ACT_REG           0x28
#define RK818_LDO_UV_STS_REG            0x29
#define RK818_LDO_UV_ACT_REG            0x2a
#define RK818_DCDC_PG_REG               0x2b
#define RK818_LDO_PG_REG                0x2c
#define RK818_VOUT_MON_TDB_REG          0x2d
#define RK818_BUCK1_CONFIG_REG          0x2e
#define RK818_BUCK1_ON_VSEL_REG         0x2f
#define RK818_BUCK1_SLP_VSEL_REG        0x30
#define RK818_BUCK2_CONFIG_REG          0x32
#define RK818_BUCK2_ON_VSEL_REG         0x33
#define RK818_BUCK2_SLP_VSEL_REG        0x34
#define RK818_BUCK3_CONFIG_REG          0x36
#define RK818_BUCK4_CONFIG_REG          0x37
#define RK818_BUCK4_ON_VSEL_REG         0x38
#define RK818_BUCK4_SLP_VSEL_REG        0x39
#define RK818_BOOST_CONFIG_REG          0x3a
#define RK818_LDO1_ON_VSEL_REG          0x3b
#define RK818_LDO1_SLP_VSEL_REG         0x3c
#define RK818_LDO2_ON_VSEL_REG          0x3d
#define RK818_LDO2_SLP_VSEL_REG         0x3e
#define RK818_LDO3_ON_VSEL_REG          0x3f
#define RK818_LDO3_SLP_VSEL_REG         0x40
#define RK818_LDO4_ON_VSEL_REG          0x41
#define RK818_LDO4_SLP_VSEL_REG         0x42
#define RK818_LDO5_ON_VSEL_REG          0x43
#define RK818_LDO5_SLP_VSEL_REG         0x44
#define RK818_LDO6_ON_VSEL_REG          0x45
#define RK818_LDO6_SLP_VSEL_REG         0x46
#define RK818_LDO7_ON_VSEL_REG          0x47
#define RK818_LDO7_SLP_VSEL_REG         0x48
#define RK818_LDO8_ON_VSEL_REG          0x49
#define RK818_LDO8_SLP_VSEL_REG         0x4a
#define RK818_BOOST_LDO9_ON_VSEL_REG    0x54
#define RK818_BOOST_LDO9_SLP_VSEL_REG   0x55
#define RK818_DEVCTRL_REG               0x4b
#define RK818_INT_STS_REG1              0X4c
#define RK818_INT_STS_MSK_REG1          0x4d
#define RK818_INT_STS_REG2              0x4e
#define RK818_INT_STS_MSK_REG2          0x4f
#define RK818_IO_POL_REG                0x50
#define RK818_H5V_EN_REG                0x52
#define RK818_SLEEP_SET_OFF_REG3        0x53
#define RK818_BOOST_LDO9_ON_VSEL_REG    0x54
#define RK818_BOOST_LDO9_SLP_VSEL_REG   0x55
#define RK818_BOOST_CTRL_REG            0x56
#define RK818_DCDC_ILMAX                0x90
#define RK818_USB_CTRL_REG              0xa1

#define RK818_H5V_EN                    RT_BIT(0)
#define RK818_REF_RDY_CTRL              RT_BIT(1)
#define RK818_USB_ILIM_SEL_MASK         0xf
#define RK818_USB_ILMIN_2000MA          0x7
#define RK818_USB_CHG_SD_VSEL_MASK      0x70

enum
{
    RK805_ID = 0x8050,
    RK806_ID = 0x8060,
    RK808_ID = 0x0000,
    RK809_ID = 0x8090,
    RK817_ID = 0x8170,
    RK818_ID = 0x8180,
};

struct rk8xx
{
    int variant;

    int irq;
    struct rt_device *dev;

    rt_uint32_t (*read)(struct rk8xx *, rt_uint16_t reg);
    rt_err_t (*write)(struct rk8xx *, rt_uint16_t reg, rt_uint8_t data);
    rt_err_t (*update_bits)(struct rk8xx *, rt_uint16_t reg, rt_uint8_t mask,
            rt_uint8_t data);

    void *priv;
};

#define rk8xx_to_i2c_client(rk8xx) rt_container_of((rk8xx)->dev, struct rt_i2c_client, parent)
#define rk8xx_to_spi_device(rk8xx) rt_container_of((rk8xx)->dev, struct rt_spi_device, parent)

rt_inline rt_uint32_t rk8xx_read(struct rk8xx *rk8xx, rt_uint16_t reg)
{
    return rk8xx->read(rk8xx, reg);
}

rt_inline rt_err_t rk8xx_write(struct rk8xx *rk8xx, rt_uint16_t reg, rt_uint8_t data)
{
    return rk8xx->write(rk8xx, reg, data);
}

rt_inline rt_err_t rk8xx_update_bits(struct rk8xx *rk8xx, rt_uint16_t reg,
        rt_uint8_t mask, rt_uint8_t data)
{
    return rk8xx->update_bits(rk8xx, reg, mask, data);
}

#endif /* __RK8XX_H__ */
