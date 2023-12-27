/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2022 NXP
 */
#ifndef __DWC_ETH_QOS_H__
#define __DWC_ETH_QOS_H__
#include <rtdevice.h>
#include "phy/phy.h"
#include <drivers/ofw.h>
#include <netif/ethernetif.h>
/* Core registers */

#define EQOS_MAC_REGS_BASE 0x000
#define ARCH_DMA_MINALIGN    128

struct eqos_mac_regs {
    rt_uint32_t configuration;                /* 0x000 */
    rt_uint32_t ex_configuration;            /* 0x004 */
    rt_uint32_t filter;                    /* 0x008 */
    rt_uint32_t unused_004[(0x070 - 0x00c) / 4];    /* 0x00c */
	rt_uint32_t q0_tx_flow_ctrl;			/* 0x070 */
	rt_uint32_t unused_070[(0x090 - 0x074) / 4];	/* 0x074 */
	rt_uint32_t rx_flow_ctrl;				/* 0x090 */
	rt_uint32_t unused_094;				/* 0x094 */
	rt_uint32_t txq_prty_map0;				/* 0x098 */
	rt_uint32_t unused_09c;				/* 0x09c */
	rt_uint32_t rxq_ctrl0;				/* 0x0a0 */
	rt_uint32_t unused_0a4;				/* 0x0a4 */
	rt_uint32_t rxq_ctrl2;				/* 0x0a8 */
	rt_uint32_t unused_0ac[(0x0dc - 0x0ac) / 4];	/* 0x0ac */
	rt_uint32_t us_tic_counter;			/* 0x0dc */
	rt_uint32_t unused_0e0[(0x11c - 0x0e0) / 4];	/* 0x0e0 */
	rt_uint32_t hw_feature0;				/* 0x11c */
	rt_uint32_t hw_feature1;				/* 0x120 */
	rt_uint32_t hw_feature2;				/* 0x124 */
	rt_uint32_t unused_128[(0x200 - 0x128) / 4];	/* 0x128 */
	rt_uint32_t mdio_address;				/* 0x200 */
	rt_uint32_t mdio_data;				/* 0x204 */
	rt_uint32_t unused_208[(0x300 - 0x208) / 4];	/* 0x208 */
	rt_uint32_t address0_high;				/* 0x300 */
	rt_uint32_t address0_low;				/* 0x304 */
};

#define EQOS_MAC_CONFIGURATION_GPSLCE			RT_BIT(23)
#define EQOS_MAC_CONFIGURATION_CST			RT_BIT(21)
#define EQOS_MAC_CONFIGURATION_ACS			RT_BIT(20)
#define EQOS_MAC_CONFIGURATION_WD			RT_BIT(19)
#define EQOS_MAC_CONFIGURATION_JD			RT_BIT(17)
#define EQOS_MAC_CONFIGURATION_JE			RT_BIT(16)
#define EQOS_MAC_CONFIGURATION_PS			RT_BIT(15)
#define EQOS_MAC_CONFIGURATION_FES			RT_BIT(14)
#define EQOS_MAC_CONFIGURATION_DM			RT_BIT(13)
#define EQOS_MAC_CONFIGURATION_LM			RT_BIT(12)
#define EQOS_MAC_CONFIGURATION_TE			RT_BIT(1)
#define EQOS_MAC_CONFIGURATION_RE			RT_BIT(0)
#define EQOS_MAC_PACKET_FILTER_RA            RT_BIT(31)

#define EQOS_MAC_Q0_TX_FLOW_CTRL_PT_SHIFT		16
#define EQOS_MAC_Q0_TX_FLOW_CTRL_PT_MASK		0xffff
#define EQOS_MAC_Q0_TX_FLOW_CTRL_TFE			RT_BIT(1)

#define EQOS_MAC_RX_FLOW_CTRL_RFE			RT_BIT(0)

#define EQOS_MAC_TXQ_PRTY_MAP0_PSTQ0_SHIFT		0
#define EQOS_MAC_TXQ_PRTY_MAP0_PSTQ0_MASK		0xff

#define EQOS_MAC_RXQ_CTRL0_RXQ0EN_SHIFT			0
#define EQOS_MAC_RXQ_CTRL0_RXQ0EN_MASK			3
#define EQOS_MAC_RXQ_CTRL0_RXQ0EN_NOT_ENABLED		0
#define EQOS_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_DCB		2
#define EQOS_MAC_RXQ_CTRL0_RXQ0EN_ENABLED_AV		1

#define EQOS_MAC_RXQ_CTRL2_PSRQ0_SHIFT			0
#define EQOS_MAC_RXQ_CTRL2_PSRQ0_MASK			0xff

#define EQOS_MAC_HW_FEATURE0_MMCSEL_SHIFT		8
#define EQOS_MAC_HW_FEATURE0_HDSEL_SHIFT		2
#define EQOS_MAC_HW_FEATURE0_GMIISEL_SHIFT		1
#define EQOS_MAC_HW_FEATURE0_MIISEL_SHIFT		0

#define EQOS_MAC_HW_FEATURE1_TXFIFOSIZE_SHIFT		6
#define EQOS_MAC_HW_FEATURE1_TXFIFOSIZE_MASK		0x1f
#define EQOS_MAC_HW_FEATURE1_RXFIFOSIZE_SHIFT		0
#define EQOS_MAC_HW_FEATURE1_RXFIFOSIZE_MASK		0x1f

#define EQOS_MAC_HW_FEATURE3_ASP_SHIFT			28
#define EQOS_MAC_HW_FEATURE3_ASP_MASK			0x3

#define EQOS_MAC_MDIO_ADDRESS_PA_SHIFT			21
#define EQOS_MAC_MDIO_ADDRESS_RDA_SHIFT			16
#define EQOS_MAC_MDIO_ADDRESS_CR_SHIFT			8
#define EQOS_MAC_MDIO_ADDRESS_CR_100_150		1
#define EQOS_MAC_MDIO_ADDRESS_CR_20_35			2
#define EQOS_MAC_MDIO_ADDRESS_CR_250_300		5
#define EQOS_MAC_MDIO_ADDRESS_SKAP			RT_BIT(4)
#define EQOS_MAC_MDIO_ADDRESS_GOC_SHIFT			2
#define EQOS_MAC_MDIO_ADDRESS_GOC_READ			3
#define EQOS_MAC_MDIO_ADDRESS_GOC_WRITE			1
#define EQOS_MAC_MDIO_ADDRESS_C45E			RT_BIT(1)
#define EQOS_MAC_MDIO_ADDRESS_GB			RT_BIT(0)

#define EQOS_MAC_MDIO_DATA_GD_MASK			0xffff

#define EQOS_MTL_REGS_BASE 0xd00
struct eqos_mtl_regs {
	rt_uint32_t txq0_operation_mode;			/* 0xd00 */
	rt_uint32_t unused_d04;				/* 0xd04 */
	rt_uint32_t txq0_debug;				/* 0xd08 */
	rt_uint32_t unused_d0c[(0xd18 - 0xd0c) / 4];	/* 0xd0c */
	rt_uint32_t txq0_quantum_weight;			/* 0xd18 */
	rt_uint32_t unused_d1c[(0xd30 - 0xd1c) / 4];	/* 0xd1c */
	rt_uint32_t rxq0_operation_mode;			/* 0xd30 */
	rt_uint32_t unused_d34;				/* 0xd34 */
	rt_uint32_t rxq0_debug;				/* 0xd38 */
};

#define EQOS_MTL_TXQ0_OPERATION_MODE_TQS_SHIFT		16
#define EQOS_MTL_TXQ0_OPERATION_MODE_TQS_MASK		0x1ff
#define EQOS_MTL_TXQ0_OPERATION_MODE_TXQEN_SHIFT	2
#define EQOS_MTL_TXQ0_OPERATION_MODE_TXQEN_MASK		3
#define EQOS_MTL_TXQ0_OPERATION_MODE_TXQEN_ENABLED	2
#define EQOS_MTL_TXQ0_OPERATION_MODE_TSF		RT_BIT(1)
#define EQOS_MTL_TXQ0_OPERATION_MODE_FTQ		RT_BIT(0)

#define EQOS_MTL_TXQ0_DEBUG_TXQSTS			RT_BIT(4)
#define EQOS_MTL_TXQ0_DEBUG_TRCSTS_SHIFT		1
#define EQOS_MTL_TXQ0_DEBUG_TRCSTS_MASK			3

#define EQOS_MTL_RXQ0_OPERATION_MODE_RQS_SHIFT		20
#define EQOS_MTL_RXQ0_OPERATION_MODE_RQS_MASK		0x3ff
#define EQOS_MTL_RXQ0_OPERATION_MODE_RFD_SHIFT		14
#define EQOS_MTL_RXQ0_OPERATION_MODE_RFD_MASK		0x3f
#define EQOS_MTL_RXQ0_OPERATION_MODE_RFA_SHIFT		8
#define EQOS_MTL_RXQ0_OPERATION_MODE_RFA_MASK		0x3f
#define EQOS_MTL_RXQ0_OPERATION_MODE_EHFC		RT_BIT(7)
#define EQOS_MTL_RXQ0_OPERATION_MODE_RSF		RT_BIT(5)

#define EQOS_MTL_RXQ0_DEBUG_PRXQ_SHIFT			16
#define EQOS_MTL_RXQ0_DEBUG_PRXQ_MASK			0x7fff
#define EQOS_MTL_RXQ0_DEBUG_RXQSTS_SHIFT		4
#define EQOS_MTL_RXQ0_DEBUG_RXQSTS_MASK			3

#define EQOS_DMA_REGS_BASE 0x1000
struct eqos_dma_regs {
	rt_uint32_t mode;					/* 0x1000 */
	rt_uint32_t sysbus_mode;				/* 0x1004 */
	rt_uint32_t unused_1008[(0x1100 - 0x1008) / 4];	/* 0x1008 */
	rt_uint32_t ch0_control;				/* 0x1100 */
	rt_uint32_t ch0_tx_control;			/* 0x1104 */
	rt_uint32_t ch0_rx_control;			/* 0x1108 */
	rt_uint32_t unused_110c;				/* 0x110c */
	rt_uint32_t ch0_txdesc_list_haddress;		/* 0x1110 */
	rt_uint32_t ch0_txdesc_list_address;		/* 0x1114 */
	rt_uint32_t ch0_rxdesc_list_haddress;		/* 0x1118 */
	rt_uint32_t ch0_rxdesc_list_address;		/* 0x111c */
	rt_uint32_t ch0_txdesc_tail_pointer;		/* 0x1120 */
	rt_uint32_t unused_1124;				/* 0x1124 */
	rt_uint32_t ch0_rxdesc_tail_pointer;		/* 0x1128 */
	rt_uint32_t ch0_txdesc_ring_length;		/* 0x112c */
	rt_uint32_t ch0_rxdesc_ring_length;		/* 0x1130 */
	rt_uint32_t ch0_interrupt_control;         /* 0x1134 */
    rt_uint32_t unused_1038[(0x1160 - 0x1138) / 4];    /* 0x1138 */
    rt_uint32_t ch0_interrupt_status;         /* 0x1160 */

};

#define EQOS_DMA_MODE_SWR				RT_BIT(0)

#define EQOS_DMA_SYSBUS_MODE_RD_OSR_LMT_SHIFT		16
#define EQOS_DMA_SYSBUS_MODE_RD_OSR_LMT_MASK		0xf
#define EQOS_DMA_SYSBUS_MODE_EAME			RT_BIT(11)
#define EQOS_DMA_SYSBUS_MODE_BLEN16			RT_BIT(3)
#define EQOS_DMA_SYSBUS_MODE_BLEN8			RT_BIT(2)
#define EQOS_DMA_SYSBUS_MODE_BLEN4			RT_BIT(1)

#define EQOS_DMA_CH0_CONTROL_DSL_SHIFT			18
#define EQOS_DMA_CH0_CONTROL_DSL_MASK			0x7
#define EQOS_DMA_CH0_CONTROL_PBLX8			RT_BIT(16)

#define EQOS_DMA_CH0_TX_CONTROL_TXPBL_SHIFT		16
#define EQOS_DMA_CH0_TX_CONTROL_TXPBL_MASK		0x3f
#define EQOS_DMA_CH0_TX_CONTROL_OSP			RT_BIT(4)
#define EQOS_DMA_CH0_TX_CONTROL_ST			RT_BIT(0)

#define EQOS_DMA_CH0_RX_CONTROL_RXPBL_SHIFT		16
#define EQOS_DMA_CH0_RX_CONTROL_RXPBL_MASK		0x3f
#define EQOS_DMA_CH0_RX_CONTROL_RBSZ_SHIFT		1
#define EQOS_DMA_CH0_RX_CONTROL_RBSZ_MASK		0x3fff
#define EQOS_DMA_CH0_RX_CONTROL_SR			RT_BIT(0)


#define EQOS_SDMEMCOMPPADCTRL_PAD_E_INPUT_OR_E_PWRD	RT_BIT(31)

#define EQOS_AUTO_CAL_CONFIG_START			RT_BIT(31)
#define EQOS_AUTO_CAL_CONFIG_ENABLE			RT_BIT(29)

#define EQOS_AUTO_CAL_STATUS_ACTIVE			RT_BIT(31)

/* Descriptors */
#define EQOS_DESCRIPTORS_TX	4
#define EQOS_DESCRIPTORS_RX	4
#define EQOS_DESCRIPTORS_NUM	(EQOS_DESCRIPTORS_TX + EQOS_DESCRIPTORS_RX)
#define EQOS_BUFFER_ALIGN	ARCH_DMA_MINALIGN
#define EQOS_MAX_PACKET_SIZE	RT_ALIGN(1568, ARCH_DMA_MINALIGN)
#define EQOS_RX_BUFFER_SIZE	(EQOS_DESCRIPTORS_RX * EQOS_MAX_PACKET_SIZE)

struct eqos_desc {
	rt_uint32_t des0;
	rt_uint32_t des1;
	rt_uint32_t des2;
	rt_uint32_t des3;
};

#define EQOS_DESC3_OWN		RT_BIT(31)
#define EQOS_DESC3_FD		RT_BIT(29)
#define EQOS_DESC3_LD		RT_BIT(28)
#define EQOS_DESC3_BUF1V	RT_BIT(24)

#define EQOS_AXI_WIDTH_32	4
#define EQOS_AXI_WIDTH_64	8
#define EQOS_AXI_WIDTH_128	16

struct eqos_config {
	rt_bool_t reg_access_always_ok;
	int mdio_wait;
	int swr_wait;
	int config_mac;
	int config_mac_mdio;
	rt_uint32_t axi_bus_width;
	phy_interface_t (*interface)(const struct rt_device *dev);
	struct eqos_ops *ops;
};

struct eqos_ops {
	void (*eqos_inval_desc)(void *desc);
	void (*eqos_flush_desc)(void *desc);
	void (*eqos_inval_buffer)(void *buf, size_t size);
	void (*eqos_flush_buffer)(void *buf, size_t size);
	int (*eqos_probe_resources)(struct rt_device *dev);
	int (*eqos_remove_resources)(struct rt_device *dev);
	int (*eqos_stop_resets)(struct rt_device *dev);
	int (*eqos_start_resets)(struct rt_device *dev);
	int (*eqos_stop_clks)(struct rt_device *dev);
	int (*eqos_start_clks)(struct rt_device *dev);
	int (*eqos_calibrate_pads)(struct rt_device *dev);
	int (*eqos_disable_calibration)(struct rt_device *dev);
	int (*eqos_set_tx_clk_speed)(struct rt_device *dev);
	int (*eqos_get_enetaddr)(struct rt_device *dev);
	ulong (*eqos_get_tick_clk_rate)(struct rt_device *dev);
};

/**
 * struct eth_pdata - Platform data for Ethernet MAC controllers
 *
 * @iobase: The base address of the hardware registers
 * @enetaddr: The Ethernet MAC address that is loaded from EEPROM or env
 * @phy_interface: PHY interface to use - see PHY_INTERFACE_MODE_...
 * @max_speed: Maximum speed of Ethernet connection supported by MAC
 * @priv_pdata: device specific plat
 */
#define ARP_HLEN 6
struct eqos_eth_data {
	rt_ubase_t iobase;
	unsigned char enetaddr[ARP_HLEN];
	int phy_interface;
	int max_speed;
	void *priv_pdata;
};
struct eqos_eth_device {
	struct eth_device parent;
    struct eqos_eth_data pdata;
	struct eqos_config config;
	uint32_t dma_rx_irq;
    uint32_t dma_tx_irq;
	rt_uint32_t id;
	void *regs;
	struct eqos_mac_regs *mac_regs;
	struct eqos_mtl_regs *mtl_regs;
	struct eqos_dma_regs *dma_regs;
	struct rt_reset_control *reset_ctl;
	rt_uint32_t phy_reset_gpio;
	rt_bool_t phy_reset_active_low;
	struct rt_clk *clk_master_bus;
	struct rt_clk *clk_rx;
	struct rt_clk *clk_ptp_rt_ref;
	struct rt_clk *clk_tx;
	struct rt_clk *clk_ck;
	struct rt_clk *clk_slave_bus;
	struct mdio_bus *mdio;
	struct phy_device *phy;
	struct rt_ofw_node *phy_of_node;
	rt_uint32_t max_speed;
	volatile struct eqos_desc *tx_descs;
	volatile struct eqos_desc *rx_descs;
	void *descs;
    void *descs_phy;
	struct eqos_desc *tx_descs_phy;
    struct eqos_desc *rx_descs_phy;
	int tx_desc_idx, rx_desc_idx;
	rt_uint32_t desc_size;
	rt_uint32_t desc_per_cacheline;
	void *tx_dma_buf;
	void *rx_dma_buf;
    void *tx_dma_buf_phy;
    void *rx_dma_buf_phy;
	rt_bool_t started;
	rt_bool_t reg_access_ok;
	rt_bool_t clk_ck_enabled;
	rt_uint32_t tx_fifo_sz, rx_fifo_sz;
	rt_uint32_t reset_delays[3];

	rt_sem_t init_sem;
    rt_sem_t rx_sem;
    rt_sem_t tx_sem; 
};

void eqos_inval_buffer_generic(void *buf, size_t length);
int eqos_null_ops(struct rt_device *dev);

extern struct eqos_config eqos_rockchip_config;

#define __iormb()        asm volatile("" : : : "memory")
#define __iowmb()        asm volatile("" : : : "memory")
# define cpu_to_le32(x)        (x)
# define le32_to_cpu(x)        (x)

static inline rt_uint32_t dwc_raw_readl(const volatile void *addr)
{
    return *(volatile rt_uint32_t *)((rt_uint64_t)addr);
}

static inline void dwc_raw_writel(uint32_t value, volatile void *addr)
{
    *(volatile rt_uint32_t *)((rt_uint64_t)addr) = (value);
    rt_hw_dsb();
}

#define out_arch(type, endian, a, v)    dwc_raw_write##type(cpu_to_##endian(v), a)
#define in_arch(type, endian, a)    endian##_to_cpu(dwc_raw_read##type(a))
#define out_le32(a, v)    out_arch(l, le32, a, v)
#define in_le32(a)    in_arch(l, le32, a)
#define clrbits(type, addr, clear) out_##type((addr), in_##type(addr) & ~(clear))
#define setbits(type, addr, set) out_##type((addr), in_##type(addr) | (set))
#define clrsetbits(type, addr, clear, set) out_##type((addr), (in_##type(addr) & ~(clear)) | (set))
#define clrbits_le32(addr, clear) clrbits(le32, addr, clear)
#define setbits_le32(addr, set) setbits(le32, addr, set)
#define clrsetbits_le32(addr, clear, set) clrsetbits(le32, addr, clear, set)
#define writel_relaxed(v,c)    dwc_raw_writel(( rt_uint32_t)(v),c)
#define writel(v,c)        ({ __iowmb(); writel_relaxed(v,c); })
#define readl_relaxed(c) ({ rt_uint32_t __r = (( rt_uint32_t) dwc_raw_readl(c)); __r; })
#define readl(c)        ({ rt_uint32_t __v = readl_relaxed(c); __iormb(); __v; })
#define EQOS_DESC3_OWN        RT_BIT(31)
#define EQOS_DESC3_FD        RT_BIT(29)
#define EQOS_DESC3_LD        RT_BIT(28)
#define EQOS_DESC3_BUF1V    RT_BIT(24)
#define EQOS_DESC3_CRC      (RT_BIT(16) | RT_BIT(17))
#define EQOS_RX_DESC3_IOC        RT_BIT(30)
#define EQOS_TX_DESC3_IOC        RT_BIT(31)


#define ENET_FRAME_MAX_FRAMELEN 1518U

#endif