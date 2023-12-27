// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2016, NVIDIA CORPORATION.
 *
 * Portions based on U-Boot's rtl8169.c.
 */

/*
 * This driver supports the Synopsys Designware Ethernet QOS (Quality Of
 * Service) IP block. The IP supports multiple options for bus type, clocking/
 * reset structure, and feature list.
 *
 * The driver is written such that generic core logic is kept separate from
 * configuration-specific logic. Code that interacts with configuration-
 * specific resources is split out into separate functions to avoid polluting
 * common code. If/when this driver is enhanced to support multiple
 * configurations, the core code should be adapted to call all configuration-
 * specific functions through function pointers, with the definition of those
 * function pointers being supplied by struct rt_device_id eqos_ids[]'s .data
 * field.
 */

#include <rtthread.h>
#include <mm_page.h>
#include <mmu.h>
#include "wait_bits.h"
#include "dwc_eth_qos.h"
#include "phy/phy.h"
#include "phy/mdio.h"

#define DBG_TAG "drv.net"
#define DBG_LVL DBG_WARNING
#include <rtdbg.h>

void eqos_inval_buffer_generic(void *buf, size_t length)
{
    unsigned long start = (unsigned long)buf & ~(ARCH_DMA_MINALIGN - 1);
    unsigned long end = RT_ALIGN(start + length, ARCH_DMA_MINALIGN);
    unsigned long size = end - start;
    rt_hw_cpu_dcache_ops(RT_HW_CACHE_INVALIDATE,(void *)start, (int)size);
}

#define DMA_CHAN_INTR_DEFAULT_RX RT_BIT(6)
#define DMA_CHAN_INTR_DEFAULT_TX RT_BIT(0)
void eqos_enable_dma_tx_irq(struct eqos_eth_device *eqos)
{
    setbits_le32(&eqos->dma_regs->ch0_interrupt_control, DMA_CHAN_INTR_DEFAULT_TX);
}

void eqos_enable_dma_rx_irq(struct eqos_eth_device *eqos)
{
    setbits_le32(&eqos->dma_regs->ch0_interrupt_control, DMA_CHAN_INTR_DEFAULT_RX);
}

void eqos_enable_dma_sum_irq(struct eqos_eth_device *eqos)
{
    setbits_le32(&eqos->dma_regs->ch0_interrupt_control, 0xc000);
}

void eqos_disable_dma_tx_irq(struct eqos_eth_device *eqos)
{
    clrbits_le32(&eqos->dma_regs->ch0_interrupt_control, DMA_CHAN_INTR_DEFAULT_TX);
}

void eqos_disable_dma_rx_irq(struct eqos_eth_device *eqos)
{
    clrbits_le32(&eqos->dma_regs->ch0_interrupt_control, DMA_CHAN_INTR_DEFAULT_RX);
}

void eqos_disable_dma_sum_irq(struct eqos_eth_device *eqos)
{
    clrbits_le32(&eqos->dma_regs->ch0_interrupt_control, 0xc000);
}

static int eqos_mdio_wait_idle(struct eqos_eth_device *eqos)
{
	return wait_for_bit_le32(&eqos->mac_regs->mdio_address,
				 EQOS_MAC_MDIO_ADDRESS_GB, RT_FALSE,
				 1000000);
}

static int eqos_mdio_read(struct mdio_bus *bus, int mdio_addr, int mdio_devad,
			  int mdio_reg)
{
	struct eqos_eth_device *eqos = bus->priv;
	rt_uint32_t val;
	int ret;

	ret = eqos_mdio_wait_idle(eqos);
	if (ret)
	{
		LOG_E("MDIO not idle at entry");
		return ret;
	}

	val = readl(&eqos->mac_regs->mdio_address);
	val &= EQOS_MAC_MDIO_ADDRESS_SKAP |
		EQOS_MAC_MDIO_ADDRESS_C45E;
	val |= (mdio_addr << EQOS_MAC_MDIO_ADDRESS_PA_SHIFT) |
		(mdio_reg << EQOS_MAC_MDIO_ADDRESS_RDA_SHIFT) |
		(eqos->config.config_mac_mdio <<
		 EQOS_MAC_MDIO_ADDRESS_CR_SHIFT) |
		(EQOS_MAC_MDIO_ADDRESS_GOC_READ <<
		 EQOS_MAC_MDIO_ADDRESS_GOC_SHIFT) |
		EQOS_MAC_MDIO_ADDRESS_GB;
	writel(val, &eqos->mac_regs->mdio_address);

	rt_hw_us_delay(eqos->config.mdio_wait);

	ret = eqos_mdio_wait_idle(eqos);
	if (ret)
	{
		LOG_E("MDIO read didn't complete");
		return ret;
	}

	val = readl(&eqos->mac_regs->mdio_data);
	val &= EQOS_MAC_MDIO_DATA_GD_MASK;

	return val;
}

static int eqos_mdio_write(struct mdio_bus *bus, int mdio_addr, int mdio_devad,
			   int mdio_reg, rt_uint16_t mdio_val)
{
	struct eqos_eth_device *eqos = bus->priv;
	rt_uint32_t val;
	int ret;

	ret = eqos_mdio_wait_idle(eqos);
	if (ret)
	{
		LOG_E("MDIO not idle at entry");
		return ret;
	}

	writel(mdio_val, &eqos->mac_regs->mdio_data);

	val = readl(&eqos->mac_regs->mdio_address);
	val &= EQOS_MAC_MDIO_ADDRESS_SKAP |
		EQOS_MAC_MDIO_ADDRESS_C45E;
	val |= (mdio_addr << EQOS_MAC_MDIO_ADDRESS_PA_SHIFT) |
		(mdio_reg << EQOS_MAC_MDIO_ADDRESS_RDA_SHIFT) |
		(eqos->config.config_mac_mdio <<
		 EQOS_MAC_MDIO_ADDRESS_CR_SHIFT) |
		(EQOS_MAC_MDIO_ADDRESS_GOC_WRITE <<
		 EQOS_MAC_MDIO_ADDRESS_GOC_SHIFT) |
		EQOS_MAC_MDIO_ADDRESS_GB;
	writel(val, &eqos->mac_regs->mdio_address);

	rt_hw_us_delay(eqos->config.mdio_wait);

	ret = eqos_mdio_wait_idle(eqos);
	if (ret)
	{
		LOG_E("MDIO read didn't complete");
		return ret;
	}

	return 0;
}

static int eqos_set_full_duplex(struct rt_device *dev)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;

	LOG_D("%s(dev=%p):\n", __func__, dev);

	setbits_le32(&eqos->mac_regs->configuration, EQOS_MAC_CONFIGURATION_DM);

	return 0;
}

static int eqos_set_half_duplex(struct rt_device *dev)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;

	LOG_D("%s(dev=%p):\n", __func__, dev);

	clrbits_le32(&eqos->mac_regs->configuration, EQOS_MAC_CONFIGURATION_DM);

	/* WAR: Flush TX queue when switching to half-duplex */
	setbits_le32(&eqos->mtl_regs->txq0_operation_mode,
		     EQOS_MTL_TXQ0_OPERATION_MODE_FTQ);

	return 0;
}

static int eqos_set_gmii_speed(struct rt_device *dev)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;

	LOG_D("%s(dev=%p):\n", __func__, dev);

	clrbits_le32(&eqos->mac_regs->configuration,
		     EQOS_MAC_CONFIGURATION_PS | EQOS_MAC_CONFIGURATION_FES);

	return 0;
}

static int eqos_set_mii_speed_100(struct rt_device *dev)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;

	LOG_D("%s(dev=%p):\n", __func__, dev);

	setbits_le32(&eqos->mac_regs->configuration,
		     EQOS_MAC_CONFIGURATION_PS | EQOS_MAC_CONFIGURATION_FES);

	return 0;
}

static int eqos_set_mii_speed_10(struct rt_device *dev)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;

	LOG_D("%s(dev=%p):\n", __func__, dev);

	clrsetbits_le32(&eqos->mac_regs->configuration,
			EQOS_MAC_CONFIGURATION_FES, EQOS_MAC_CONFIGURATION_PS);

	return 0;
}

static int eqos_adjust_link(struct rt_device *dev)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;
	int ret;
	bool en_calibration;

	LOG_D("%s(dev=%p):\n", __func__, dev);

	if (eqos->phy->duplex)
		ret = eqos_set_full_duplex(dev);
	else
		ret = eqos_set_half_duplex(dev);
	if (ret < 0) {
		LOG_E("eqos_set_*_duplex() failed: %d", ret);
		return ret;
	}

	switch (eqos->phy->speed) {
	case SPEED_1000:
		en_calibration = RT_TRUE;
		ret = eqos_set_gmii_speed(dev);
		break;
	case SPEED_100:
		en_calibration = RT_TRUE;
		ret = eqos_set_mii_speed_100(dev);
		break;
	case SPEED_10:
		en_calibration = RT_FALSE;
		ret = eqos_set_mii_speed_10(dev);
		break;
	default:
		LOG_E("invalid speed %d", eqos->phy->speed);
		return -RT_EINVAL;
	}
	if (ret < 0) {
		LOG_E("eqos_set_*mii_speed*() failed: %d", ret);
		return ret;
	}

	if (en_calibration) {
		ret = eqos->config.ops->eqos_calibrate_pads(dev);
		if (ret < 0) {
			LOG_E("eqos_calibrate_pads() failed: %d",
			       ret);
			return ret;
		}
	} else {
		ret = eqos->config.ops->eqos_disable_calibration(dev);
		if (ret < 0) {
			LOG_E("eqos_disable_calibration() failed: %d",
			       ret);
			return ret;
		}
	}
	ret = eqos->config.ops->eqos_set_tx_clk_speed(dev);
	if (ret < 0) {
		LOG_E("eqos_set_tx_clk_speed() failed: %d", ret);
		return ret;
	}

	return 0;
}

static int eqos_get_phy_addr(struct eqos_eth_device *priv, struct rt_device *dev)
{
	struct rt_ofw_node *phy_node;
	rt_uint32_t reg;
	int ret;

	phy_node = rt_ofw_parse_phandle(dev->ofw_node, "phy-handle", 0);
	if (!phy_node) {
		LOG_D("Failed to find phy-handle");
		return -RT_ERROR;
	}
	if (phy_node) {
		ret = rt_ofw_prop_read_u32(phy_node, "reg", &reg);
		if (ret)
			reg = -1;
	}

	return reg;
}
static void rt_hw_gmac_isr(int irqno, void *param)
{
	
    int status;
    struct eqos_eth_device *eqos = (struct eqos_eth_device *)param;
LOG_I("gmac isr status %x ch0 status %x", *(rt_uint32_t*)(eqos->regs+0xb0),eqos->dma_regs->ch0_interrupt_status);
    status = readl(&eqos->dma_regs->ch0_interrupt_status);
    if (status & DMA_CHAN_INTR_DEFAULT_RX)
    {
		LOG_I("RX intr");
        eqos_disable_dma_rx_irq(eqos);
        clrbits_le32(&eqos->dma_regs->ch0_interrupt_status, DMA_CHAN_INTR_DEFAULT_RX);
        eth_device_ready(&eqos->parent);
    }
    if (status & DMA_CHAN_INTR_DEFAULT_TX)
    {
		LOG_I("TX intr");
        clrbits_le32(&eqos->dma_regs->ch0_interrupt_status, DMA_CHAN_INTR_DEFAULT_TX);
    }
    /*clear all interrupt status*/
    setbits_le32(&eqos->dma_regs->ch0_interrupt_status, 0);
}

static int eqos_start(struct rt_device *dev)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;
	int ret, i;
	rt_ubase_t rate;
	rt_uint32_t val, tx_fifo_sz, rx_fifo_sz, tqs, rqs, pbl;
	rt_ubase_t last_rx_desc;

	LOG_D("%s(dev=%p):\n", __func__, dev);

	eqos->tx_desc_idx = 0;
	eqos->rx_desc_idx = 0;

	ret = eqos->config.ops->eqos_start_resets(dev);
	if (ret < 0) {
		LOG_E("eqos_start_resets() failed: %d", ret);
		goto err;
	}

	rt_hw_us_delay(10);

	eqos->reg_access_ok = RT_TRUE;

	/*
	 * Assert the SWR first, the actually reset the MAC and to latch in
	 * e.g. i.MX8M Plus GPR[1] content, which selects interface mode.
	 */
	setbits_le32(&eqos->dma_regs->mode, EQOS_DMA_MODE_SWR);

	ret = wait_for_bit_le32(&eqos->dma_regs->mode,
				EQOS_DMA_MODE_SWR, RT_FALSE,
				eqos->config.swr_wait);
	if (ret) {
		LOG_E("EQOS_DMA_MODE_SWR stuck");
		goto err_stop_resets;
	}

	ret = eqos->config.ops->eqos_calibrate_pads(dev);
	if (ret < 0) {
		LOG_E("eqos_calibrate_pads() failed: %d", ret);
		goto err_stop_resets;
	}

	if (eqos->config.ops->eqos_get_tick_clk_rate) {
		rate = eqos->config.ops->eqos_get_tick_clk_rate(dev);

		val = (rate / 1000000) - 1;
		writel(val, &eqos->mac_regs->us_tic_counter);
	}

	/*
	 * if PHY was already connected and configured,
	 * don't need to reconnect/reconfigure again
	 */
	if (!eqos->phy) {
		int addr = -1;

		if (!eqos->phy) {
			addr = eqos_get_phy_addr(eqos, dev);
			eqos->phy = phy_device_get_by_bus(eqos->mdio, addr, MDIO_DEVAD_NONE,eqos->pdata.phy_interface);
		}

		if (!eqos->phy) {
			LOG_E("phy_connect() failed");
			ret = -RT_ERROR;
			goto err_stop_resets;
		}

		if (eqos->max_speed) {
			ret = phy_set_supported(eqos->phy, eqos->max_speed);
			if (ret) {
				LOG_E("phy_set_supported() failed: %d", ret);
				goto err_shutdown_phy;
			}
		}

		eqos->phy->advertising = 0x2ff;
        eqos->phy->supported = 0x2ff;
		eqos->phy->parent.ofw_node = eqos->phy_of_node;
		ret = phy_config(eqos->phy);
		if (ret < 0) {
			LOG_E("phy_config() failed: %d", ret);
			goto err_shutdown_phy;
		}
	}

	/* Configure MTL */

	/* Enable Store and Forward mode for TX */
	/* Program Tx operating mode */
	setbits_le32(&eqos->mtl_regs->txq0_operation_mode,
		     EQOS_MTL_TXQ0_OPERATION_MODE_TSF |
		     (EQOS_MTL_TXQ0_OPERATION_MODE_TXQEN_ENABLED <<
		      EQOS_MTL_TXQ0_OPERATION_MODE_TXQEN_SHIFT));

	/* Transmit Queue weight */
	writel(0x10, &eqos->mtl_regs->txq0_quantum_weight);

	/* Enable Store and Forward mode for RX, since no jumbo frame */
	setbits_le32(&eqos->mtl_regs->rxq0_operation_mode,
		     EQOS_MTL_RXQ0_OPERATION_MODE_RSF);

	/* Transmit/Receive queue fifo size; use all RAM for 1 queue */
	val = readl(&eqos->mac_regs->hw_feature1);
	tx_fifo_sz = (val >> EQOS_MAC_HW_FEATURE1_TXFIFOSIZE_SHIFT) &
		EQOS_MAC_HW_FEATURE1_TXFIFOSIZE_MASK;
	rx_fifo_sz = (val >> EQOS_MAC_HW_FEATURE1_RXFIFOSIZE_SHIFT) &
		EQOS_MAC_HW_FEATURE1_RXFIFOSIZE_MASK;

	/* r/tx_fifo_sz is encoded as log2(n / 128). Undo that by shifting */
	tx_fifo_sz = 128 << tx_fifo_sz;
	rx_fifo_sz = 128 << rx_fifo_sz;

	/* Allow platform to override TX/RX fifo size */
	if (eqos->tx_fifo_sz)
		tx_fifo_sz = eqos->tx_fifo_sz;
	if (eqos->rx_fifo_sz)
		rx_fifo_sz = eqos->rx_fifo_sz;

	/* r/tqs is encoded as (n / 256) - 1 */
	tqs = tx_fifo_sz / 256 - 1;
	rqs = rx_fifo_sz / 256 - 1;

	clrsetbits_le32(&eqos->mtl_regs->txq0_operation_mode,
			EQOS_MTL_TXQ0_OPERATION_MODE_TQS_MASK <<
			EQOS_MTL_TXQ0_OPERATION_MODE_TQS_SHIFT,
			tqs << EQOS_MTL_TXQ0_OPERATION_MODE_TQS_SHIFT);
	clrsetbits_le32(&eqos->mtl_regs->rxq0_operation_mode,
			EQOS_MTL_RXQ0_OPERATION_MODE_RQS_MASK <<
			EQOS_MTL_RXQ0_OPERATION_MODE_RQS_SHIFT,
			rqs << EQOS_MTL_RXQ0_OPERATION_MODE_RQS_SHIFT);

	/* Flow control used only if each channel gets 4KB or more FIFO */
	if (rqs >= ((4096 / 256) - 1)) {
		rt_uint32_t rfd, rfa;

		setbits_le32(&eqos->mtl_regs->rxq0_operation_mode,
			     EQOS_MTL_RXQ0_OPERATION_MODE_EHFC);

		/*
		 * Set Threshold for Activating Flow Contol space for min 2
		 * frames ie, (1500 * 1) = 1500 bytes.
		 *
		 * Set Threshold for Deactivating Flow Contol for space of
		 * min 1 frame (frame size 1500bytes) in receive fifo
		 */
		if (rqs == ((4096 / 256) - 1)) {
			/*
			 * This violates the above formula because of FIFO size
			 * limit thert_refore overflow may occur inspite of this.
			 */
			rfd = 0x3;	/* Full-3K */
			rfa = 0x1;	/* Full-1.5K */
		} else if (rqs == ((8192 / 256) - 1)) {
			rfd = 0x6;	/* Full-4K */
			rfa = 0xa;	/* Full-6K */
		} else if (rqs == ((16384 / 256) - 1)) {
			rfd = 0x6;	/* Full-4K */
			rfa = 0x12;	/* Full-10K */
		} else {
			rfd = 0x6;	/* Full-4K */
			rfa = 0x1E;	/* Full-16K */
		}

		clrsetbits_le32(&eqos->mtl_regs->rxq0_operation_mode,
				(EQOS_MTL_RXQ0_OPERATION_MODE_RFD_MASK <<
				 EQOS_MTL_RXQ0_OPERATION_MODE_RFD_SHIFT) |
				(EQOS_MTL_RXQ0_OPERATION_MODE_RFA_MASK <<
				 EQOS_MTL_RXQ0_OPERATION_MODE_RFA_SHIFT),
				(rfd <<
				 EQOS_MTL_RXQ0_OPERATION_MODE_RFD_SHIFT) |
				(rfa <<
				 EQOS_MTL_RXQ0_OPERATION_MODE_RFA_SHIFT));
	}

	/* Configure MAC */

	clrsetbits_le32(&eqos->mac_regs->rxq_ctrl0,
			EQOS_MAC_RXQ_CTRL0_RXQ0EN_MASK <<
			EQOS_MAC_RXQ_CTRL0_RXQ0EN_SHIFT,
			eqos->config.config_mac <<
			EQOS_MAC_RXQ_CTRL0_RXQ0EN_SHIFT);

	/* Multicast and Broadcast Queue Enable */
	setbits_le32(&eqos->mac_regs->unused_0a4,
		     0x00100000);
	/* enable promise mode */
	setbits_le32(&eqos->mac_regs->unused_004[1],
		     0x1);

	/* Set TX flow control parameters */
	/* Set Pause Time */
	setbits_le32(&eqos->mac_regs->q0_tx_flow_ctrl,
		     0xffff << EQOS_MAC_Q0_TX_FLOW_CTRL_PT_SHIFT);
	/* Assign priority for TX flow control */
	clrbits_le32(&eqos->mac_regs->txq_prty_map0,
		     EQOS_MAC_TXQ_PRTY_MAP0_PSTQ0_MASK <<
		     EQOS_MAC_TXQ_PRTY_MAP0_PSTQ0_SHIFT);
	/* Assign priority for RX flow control */
	clrbits_le32(&eqos->mac_regs->rxq_ctrl2,
		     EQOS_MAC_RXQ_CTRL2_PSRQ0_MASK <<
		     EQOS_MAC_RXQ_CTRL2_PSRQ0_SHIFT);
	/* Enable flow control */
	setbits_le32(&eqos->mac_regs->q0_tx_flow_ctrl,
		     EQOS_MAC_Q0_TX_FLOW_CTRL_TFE);
	setbits_le32(&eqos->mac_regs->rx_flow_ctrl,
		     EQOS_MAC_RX_FLOW_CTRL_RFE);

	clrsetbits_le32(&eqos->mac_regs->configuration,
			EQOS_MAC_CONFIGURATION_GPSLCE |
			EQOS_MAC_CONFIGURATION_WD |
			EQOS_MAC_CONFIGURATION_JD |
			EQOS_MAC_CONFIGURATION_JE,
			EQOS_MAC_CONFIGURATION_CST |
			EQOS_MAC_CONFIGURATION_ACS);

	//eqos_write_hwaddr(dev);

	/* Configure DMA */

	/* Enable OSP mode */
	setbits_le32(&eqos->dma_regs->ch0_tx_control,
		     EQOS_DMA_CH0_TX_CONTROL_OSP);

	/* RX buffer size. Must be a multiple of bus width */
	clrsetbits_le32(&eqos->dma_regs->ch0_rx_control,
			EQOS_DMA_CH0_RX_CONTROL_RBSZ_MASK <<
			EQOS_DMA_CH0_RX_CONTROL_RBSZ_SHIFT,
			EQOS_MAX_PACKET_SIZE <<
			EQOS_DMA_CH0_RX_CONTROL_RBSZ_SHIFT);
	/*
	 * Burst length must be < 1/2 FIFO size.
	 * FIFO size in tqs is encoded as (n / 256) - 1.
	 * Each burst is n * 8 (PBLX8) * 16 (AXI width) == 128 bytes.
	 * Half of n * 256 is n * 128, so pbl == tqs, modulo the -1.
	 */
	pbl = tqs + 1;
	if (pbl > 32)
		pbl = 32;
	clrsetbits_le32(&eqos->dma_regs->ch0_tx_control,
			EQOS_DMA_CH0_TX_CONTROL_TXPBL_MASK <<
			EQOS_DMA_CH0_TX_CONTROL_TXPBL_SHIFT,
			pbl << EQOS_DMA_CH0_TX_CONTROL_TXPBL_SHIFT);

	clrsetbits_le32(&eqos->dma_regs->ch0_rx_control,
			EQOS_DMA_CH0_RX_CONTROL_RXPBL_MASK <<
			EQOS_DMA_CH0_RX_CONTROL_RXPBL_SHIFT,
			8 << EQOS_DMA_CH0_RX_CONTROL_RXPBL_SHIFT);

	/* DMA performance configuration */
	val = (2 << EQOS_DMA_SYSBUS_MODE_RD_OSR_LMT_SHIFT) |
		EQOS_DMA_SYSBUS_MODE_EAME | EQOS_DMA_SYSBUS_MODE_BLEN16 |
		EQOS_DMA_SYSBUS_MODE_BLEN8 | EQOS_DMA_SYSBUS_MODE_BLEN4;
	writel(val, &eqos->dma_regs->sysbus_mode);

	/* Set up descriptors */
	rt_memset(eqos->descs, 0, RT_ALIGN(EQOS_DESCRIPTORS_NUM * 4*4, ARCH_DMA_MINALIGN));
#ifdef NO_MEM_COPY_TO_LWIP
    void *rx_dma_buffer = NULL;
    struct pbuf *p = NULL;
    for (i = 0; i < EQOS_DESCRIPTORS_RX; i++) 
    {
        p = pbuf_alloc(PBUF_RAW, ENET_FRAME_MAX_FRAMELEN, PBUF_POOL);
        if (!p) 
		{
            LOG_E("unable to alloc pbuf in init_dma\r\n");
            return -1;
        }
        rx_dma_buffer = p->payload;
        volatile struct eqos_desc *rx_desc = &(eqos->rx_descs[i]);

        rx_desc->des0 = (uint32_t)(unsigned long)(virtual_to_physical(rx_dma_buffer));
		rx_desc->des3 |= EQOS_DESC3_OWN | EQOS_RX_DESC3_IOC | EQOS_DESC3_BUF1V;

        rt_hw_dsb();
        eqos->config.ops->eqos_inval_buffer(rx_dma_buffer, ENET_FRAME_MAX_FRAMELEN);
        rx_pbufs_storage[i] = p;        
    }    
#else
    void *rx_dma_buffer = RT_NULL;
    for (i = 0; i < EQOS_DESCRIPTORS_RX; i++)
    {
        volatile struct eqos_desc *rx_desc = &(eqos->rx_descs[i]);
        rx_desc->des0 = (uint32_t)(unsigned long)(eqos->rx_dma_buf_phy + (i * EQOS_MAX_PACKET_SIZE));
        rx_dma_buffer = eqos->rx_dma_buf + (i * EQOS_MAX_PACKET_SIZE);
        rx_desc->des3 |= EQOS_DESC3_OWN | EQOS_RX_DESC3_IOC | EQOS_DESC3_BUF1V;

        rt_hw_dsb();
        eqos->config.ops->eqos_inval_buffer(rx_dma_buffer, ENET_FRAME_MAX_FRAMELEN);
    }
#endif
    writel(0, &eqos->dma_regs->ch0_txdesc_list_haddress);
    writel((unsigned int)(unsigned long)eqos->tx_descs_phy, &eqos->dma_regs->ch0_txdesc_list_address);
    writel(EQOS_DESCRIPTORS_TX - 1, &eqos->dma_regs->ch0_txdesc_ring_length);

    writel(0, &eqos->dma_regs->ch0_rxdesc_list_haddress);
    writel((unsigned int)(unsigned long)eqos->rx_descs_phy, &eqos->dma_regs->ch0_rxdesc_list_address);
    writel(EQOS_DESCRIPTORS_RX - 1, &eqos->dma_regs->ch0_rxdesc_ring_length);

    /* Recv All if necessary */
    setbits_le32(&eqos->mac_regs->filter, EQOS_MAC_PACKET_FILTER_RA);
#define mmc_rx_interrupt_mask 0x70c
#define mmc_tx_interrupt_mask 0x710
#define mmc_ipc_rx_interrupt_mask 0x800 
#define mmc_fpe_tx_interrupt_mask 0x8a4 
    writel(0xffffffff, ((void *)eqos->mac_regs) + mmc_rx_interrupt_mask);
    writel(0xffffffff, ((void *)eqos->mac_regs) + mmc_tx_interrupt_mask);
    writel(0xffffffff, ((void *)eqos->mac_regs) + mmc_ipc_rx_interrupt_mask);
    writel(0xffffffff, ((void *)eqos->mac_regs) + mmc_fpe_tx_interrupt_mask);


    /* Enable everything */
	setbits_le32(&eqos->dma_regs->ch0_tx_control, EQOS_DMA_CH0_TX_CONTROL_ST);
	setbits_le32(&eqos->dma_regs->ch0_rx_control, EQOS_DMA_CH0_RX_CONTROL_SR);
	setbits_le32(&eqos->mac_regs->configuration, EQOS_MAC_CONFIGURATION_TE | EQOS_MAC_CONFIGURATION_RE);

    /* TX tail pointer not written until we need to TX a packet */
    /*
     * Point RX tail pointer at last descriptor. Ideally, we'd point at the
     * first descriptor, implying all descriptors were available. However,
     * that's not distinguishable from none of the descriptors being
     * available.
     */
    last_rx_desc = (unsigned long)&(eqos->rx_descs_phy[(EQOS_DESCRIPTORS_RX - 1)]);
    writel(last_rx_desc, &eqos->dma_regs->ch0_rxdesc_tail_pointer);
    
    eqos->started = RT_TRUE;

    eqos_enable_dma_rx_irq(eqos);
    eqos_enable_dma_sum_irq(eqos);

	rt_uint32_t irq = rt_ofw_get_irq_by_name(dev->ofw_node, "macirq");
	char *irq_name = rt_malloc(10);
	rt_sprintf(irq_name, "eqos-gmac-%d", eqos->id);
	rt_hw_interrupt_install(irq, rt_hw_gmac_isr, eqos, irq_name);
	rt_hw_interrupt_umask(irq);

#ifdef RT_USING_SMP
//    rt_hw_interrupt_set_target_cpus(irq, 1 << 0);
#endif
#ifdef INSTALL_TX_IRQ
    eqos_enable_dma_tx_irq(eqos);
#endif

    rt_sem_release(eqos->init_sem);
    rt_sem_release(eqos->rx_sem);
    rt_sem_release(eqos->tx_sem);
	return 0;
err_shutdown_phy:

	phy_shutdown(eqos->phy);
err_stop_resets:
	eqos->config.ops->eqos_stop_resets(dev);
err:
	LOG_E("FAILED: %d", ret);
	return ret;
}

uint16_t eqos_recv(struct eqos_eth_device *eqos, void *buffer)
{
    volatile struct eqos_desc *rx_desc;
    uint16_t length;
    void *packetp = NULL;
    void *packetp_phy = NULL;

    rx_desc = &(eqos->rx_descs[eqos->rx_desc_idx]);
    if (rx_desc->des3 & EQOS_DESC3_OWN)
    {
        eqos_enable_dma_rx_irq(eqos);
        return 0;
    }

    packetp = eqos->rx_dma_buf + eqos->rx_desc_idx * EQOS_MAX_PACKET_SIZE;
    packetp_phy = eqos->rx_dma_buf_phy + eqos->rx_desc_idx * EQOS_MAX_PACKET_SIZE;

    length = rx_desc->des3 & 0x7fff;
    length = length/* - 4*/;
    eqos->config.ops->eqos_inval_buffer(packetp, length);
    rt_hw_dsb();
    memcpy(buffer, packetp, length);
    rx_desc->des0 = (uint32_t)(unsigned long)packetp_phy;
    rx_desc->des1 = 0;
    rx_desc->des2 = 0;

    /*
     * Make sure that if HW sees the _OWN write below, it will see all the
     * writes to the rest of the descriptor too.
     */

    rt_hw_dsb();
    rx_desc->des3 |= EQOS_DESC3_OWN | EQOS_RX_DESC3_IOC | EQOS_DESC3_BUF1V;

    rt_hw_dsb();

    eqos->rx_desc_idx++;
    eqos->rx_desc_idx %= EQOS_DESCRIPTORS_RX;

    writel((unsigned long)(&(eqos->rx_descs_phy[eqos->rx_desc_idx])), &eqos->dma_regs->ch0_rxdesc_tail_pointer);

    return length;
}

#define TX_BUFFER_INDEX_NUM (6)
#define RX_BUFFER_INDEX_NUM (6)
#define TX_BD_INDEX_NUM (2)
#define RX_BD_INDEX_NUM (2)
static int eqos_probe_resources_core(struct rt_device *dev)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;
    void *tx_rx_descs = RT_NULL;
    void *tx_rx_descs_nocache = RT_NULL;
    void *tx_rx_descs_phy = RT_NULL;

	char *init_sem = rt_malloc(10);
	char *rx_sem = rt_malloc(10);
	char *tx_sem = rt_malloc(10);

    rt_sprintf(init_sem, "eqos_init-%d", eqos->id);
    rt_sprintf(rx_sem, "eqos_rx-%d", eqos->id);
    rt_sprintf(tx_sem, "eqos_tx-%d", eqos->id);

    tx_rx_descs = rt_pages_alloc(RX_BD_INDEX_NUM);
    if (!tx_rx_descs)
    {
        LOG_E("ERROR: rx buff page alloc failed\n");
        return RT_ERROR;
    }
    tx_rx_descs_nocache = (void *)rt_ioremap_nocache(rt_kmem_v2p(tx_rx_descs), (ARCH_PAGE_SIZE << RX_BD_INDEX_NUM));
    tx_rx_descs_phy = (void *)rt_kmem_v2p(tx_rx_descs);

    void *rx_dma_buffer = RT_NULL;

    rx_dma_buffer = rt_pages_alloc(RX_BUFFER_INDEX_NUM);
    if (!rx_dma_buffer)
    {
        LOG_E("ERROR: rx buff page alloc failed\n");
        return RT_ERROR;
    }

    void *tx_dma_buffer = RT_NULL;

    tx_dma_buffer = rt_pages_alloc(TX_BUFFER_INDEX_NUM);
    if (!tx_dma_buffer)
    {
        LOG_E("ERROR: rx buff page alloc failed\n");
        return RT_ERROR;
    }

    eqos->descs = (void *)tx_rx_descs_nocache;
    eqos->descs_phy = tx_rx_descs_phy;

    eqos->tx_descs = (struct eqos_desc *)eqos->descs;
	LOG_I("eqos->tx_des = %p phy = %p\n",eqos->tx_descs, rt_kmem_v2p(eqos->tx_descs));
    eqos->rx_descs = (eqos->tx_descs + EQOS_DESCRIPTORS_TX);

    eqos->tx_descs_phy = (struct eqos_desc *)eqos->descs_phy;
    eqos->rx_descs_phy = (eqos->tx_descs_phy + EQOS_DESCRIPTORS_TX);

    eqos->tx_dma_buf = (void *)tx_dma_buffer;
    eqos->rx_dma_buf = (void *)rx_dma_buffer;

    eqos->tx_dma_buf_phy = (void *)rt_kmem_v2p(tx_dma_buffer);
    eqos->rx_dma_buf_phy = (void *)rt_kmem_v2p(rx_dma_buffer);

	eqos->init_sem = rt_sem_create(init_sem, 0, RT_IPC_FLAG_PRIO);
    if (eqos->init_sem == RT_NULL)
    {
        LOG_E("create dynamic semaphore failed.\n");
        return -1;
    }
    eqos->rx_sem = rt_sem_create(rx_sem, 0, RT_IPC_FLAG_PRIO);
    if (eqos->rx_sem == RT_NULL)
    {
        LOG_E("create gmac_rx semaphore failed.\n");
        return -1;
    }
    eqos->tx_sem = rt_sem_create(tx_sem, 0, RT_IPC_FLAG_PRIO);
    if (eqos->tx_sem == RT_NULL)
    {
        LOG_E("create gmac_tx semaphore failed.\n");
        return -1;
    }
    return 0;
}

static int eqos_remove_resources_core(struct rt_device *dev)
{
	struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;

	LOG_D("%s(dev=%p):\n", __func__, dev);
	
	rt_pages_free(eqos->descs, RX_BD_INDEX_NUM);
	rt_pages_free(eqos->rx_dma_buf, RX_BUFFER_INDEX_NUM);
	rt_pages_free(eqos->tx_dma_buf, TX_BUFFER_INDEX_NUM);
	rt_sem_delete(eqos->init_sem);
	rt_sem_delete(eqos->rx_sem);
	rt_sem_delete(eqos->tx_sem);

	LOG_D("%s: OK\n", __func__);
	return 0;
}





























#ifdef RT_USING_LWIP
struct pbuf *rt_eqos_eth_rx(rt_device_t dev)
{
    static struct pbuf *p_s = RT_NULL;
    struct pbuf *p = RT_NULL;
    uint16_t length = 0;

    struct eqos_eth_device *eqos = (struct eqos_eth_device *)(dev);
    if (p_s == RT_NULL)
    {
        p_s = pbuf_alloc(PBUF_RAW, ENET_FRAME_MAX_FRAMELEN, PBUF_POOL);
        if (p_s == RT_NULL)
        {
            LOG_E("pbuf alloc failed\n");
            return RT_NULL;
        }
    }
    p = p_s;
    length = eqos_recv(eqos, p->payload);
    if (length == 0)
    {
        return NULL;
    }
    pbuf_realloc(p, length);
    p_s = RT_NULL;
    return p;
}

/* transmit data*/
rt_err_t rt_eqos_eth_tx(rt_device_t dev, struct pbuf *p)
{
    struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;
    volatile struct eqos_desc *tx_desc;
    struct eqos_desc *tx_desc_phy;
    struct pbuf *q = NULL;
    int offset = 0;
    int ret = -1;
    int wait_hw_count = 0;

    ret = rt_sem_take(eqos->tx_sem, RT_WAITING_FOREVER);
    if (ret != RT_EOK)
    {
        LOG_E("take rx_sem, failed.\n");
    }
    tx_desc = &(eqos->tx_descs[eqos->tx_desc_idx]);
    while((tx_desc->des3 & EQOS_DESC3_OWN))
    {
        rt_thread_delay(1);
        wait_hw_count++;
        if(wait_hw_count > 2)
        {
            rt_sem_release(eqos->tx_sem);
            return RT_ERROR;            
        }
    }

    for (q = p; q != RT_NULL; q = q->next)
    {
        memcpy(eqos->tx_dma_buf + offset + eqos->tx_desc_idx * EQOS_MAX_PACKET_SIZE, q->payload, q->len);
        offset = q->len;
    }
    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH,eqos->tx_dma_buf + eqos->tx_desc_idx * EQOS_MAX_PACKET_SIZE, p->tot_len);

    tx_desc->des0 = (unsigned long)eqos->tx_dma_buf_phy + eqos->tx_desc_idx * EQOS_MAX_PACKET_SIZE;
    tx_desc->des1 = 0;
    tx_desc->des2 = EQOS_TX_DESC3_IOC | p->tot_len;

    /*
     * Make sure that if HW sees the _OWN write below, it will see all the
     * writes to the rest of the descriptor too.
     */
    rt_hw_dsb();
#ifdef RT_LWIP_USING_HW_CHECKSUM    
    tx_desc->des3 = EQOS_DESC3_OWN | EQOS_DESC3_FD | EQOS_DESC3_LD | EQOS_DESC3_CRC |p->tot_len;
#else
    tx_desc->des3 = EQOS_DESC3_OWN | EQOS_DESC3_FD | EQOS_DESC3_LD | p->tot_len;
#endif

    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH,(void *)eqos->tx_dma_buf + eqos->tx_desc_idx * EQOS_MAX_PACKET_SIZE, p->tot_len);

    rt_hw_dsb();

    eqos->tx_desc_idx++;
    eqos->tx_desc_idx %= EQOS_DESCRIPTORS_TX;
    tx_desc_phy = &eqos->tx_descs_phy[eqos->tx_desc_idx];

    writel((unsigned long)(tx_desc_phy), &eqos->dma_regs->ch0_txdesc_tail_pointer);

    rt_hw_dsb();
    rt_sem_release(eqos->tx_sem);

    return 0;
}
#endif



/* EMAC initialization function */
static rt_err_t rt_dwc_eqos_eth_init(rt_device_t dev)
{
    struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;
    eqos_start((rt_device_t)eqos);
    return RT_EOK;
}

static rt_err_t rt_dwc_eqos_eth_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t rt_dwc_eqos_eth_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_ssize_t rt_dwc_eqos_eth_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    uint16_t length = 0;
    struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;
    length = eqos_recv(eqos, buffer);
    if (length == 0)
    {
        return (int)length;
    }
    return (int)length;
}

static rt_ssize_t rt_dwc_eqos_eth_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;
    volatile struct eqos_desc *tx_desc;
    struct eqos_desc *tx_desc_phy;
    int ret = -1;

    ret = rt_sem_take(eqos->tx_sem, RT_WAITING_FOREVER);
    if (ret != RT_EOK)
    {
        LOG_E("take rx_sem, failed.\n");
    }
    tx_desc = &(eqos->tx_descs[eqos->tx_desc_idx]);

    if (tx_desc->des3 & EQOS_DESC3_OWN)
    {
        rt_sem_release(eqos->tx_sem);
        return RT_ERROR;
    }

    memcpy(eqos->tx_dma_buf + eqos->tx_desc_idx * EQOS_MAX_PACKET_SIZE, buffer, size);
    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH,eqos->tx_dma_buf + eqos->tx_desc_idx * EQOS_MAX_PACKET_SIZE, size);

    tx_desc->des0 = (unsigned long)eqos->tx_dma_buf_phy + eqos->tx_desc_idx * EQOS_MAX_PACKET_SIZE;
    tx_desc->des1 = 0;
    tx_desc->des2 = EQOS_TX_DESC3_IOC | size;

    /*
     * Make sure that if HW sees the _OWN write below, it will see all the
     * writes to the rest of the descriptor too.
     */
    rt_hw_dsb();
#ifdef RT_LWIP_USING_HW_CHECKSUM    
    tx_desc->des3 = EQOS_DESC3_OWN | EQOS_DESC3_FD | EQOS_DESC3_LD | EQOS_DESC3_CRC | size;
#else
    tx_desc->des3 = EQOS_DESC3_OWN | EQOS_DESC3_FD | EQOS_DESC3_LD | size;
#endif
    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH,(void *)eqos->tx_dma_buf + eqos->tx_desc_idx * EQOS_MAX_PACKET_SIZE, size);

    rt_hw_dsb();
    eqos->tx_desc_idx++;
    eqos->tx_desc_idx %= EQOS_DESCRIPTORS_TX;
    tx_desc_phy = &eqos->tx_descs_phy[eqos->tx_desc_idx];

    writel((unsigned long)(tx_desc_phy), &eqos->dma_regs->ch0_txdesc_tail_pointer);
    rt_hw_dsb();
    rt_sem_release(eqos->tx_sem);

    return 0;
}

static rt_err_t rt_dwc_eqos_eth_control(rt_device_t dev, int cmd, void *args)
{
    struct eqos_eth_device *eqos = (struct eqos_eth_device *)dev;
    switch (cmd)
    {
    case NIOCTL_GADDR:
        /* get MAC address */
        if (args)
        {
			LOG_I("get mac addr");
			rt_uint8_t addr[6];
			addr[0] = 0x08;
			addr[1] = 0x00;
			addr[2] = 0x27;
			addr[3] = 0xae;
			addr[4] = 0x56;
			addr[5] = 0x59 + eqos->id;
            //memcpy(args, eqos->pdata.enetaddr, ARP_HLEN);
			memcpy(args, addr, ARP_HLEN);
			LOG_I("mac %x", *(rt_ubase_t*)args);
        }
        else
        {
            return -RT_ERROR;
        }
        break;

    default:
        break;
    }
    return RT_EOK;
}
struct rt_device_ops _k_enet_ops =
{
        rt_dwc_eqos_eth_init,
        rt_dwc_eqos_eth_open,
        rt_dwc_eqos_eth_close,
        rt_dwc_eqos_eth_read,
        rt_dwc_eqos_eth_write,
        rt_dwc_eqos_eth_control,
};

static void phy_detect_thread_entry(void *param)
{
    int old_link;
    rt_err_t ret;
    struct eqos_eth_device *eqos = (struct eqos_eth_device *)param;

    while (eqos->init_sem == RT_NULL)
    {
        rt_thread_delay(RT_TICK_PER_SECOND);
    }

    ret = rt_sem_take(eqos->init_sem, RT_WAITING_FOREVER);
    if (ret != RT_EOK)
    {
        LOG_E("take semaphore, failed.\n");
        rt_sem_delete(eqos->init_sem);
        return;
    }

    ret = phy_startup(eqos->phy);
    if (ret < 0) 
    {
        LOG_E("phy_startup() failed: %d", ret);
        return;
    }
    ret = eqos_adjust_link((rt_device_t)eqos);
    if (ret < 0) {
        LOG_E("eqos_adjust_link() failed: %d", ret);
    }

    if (!eqos->phy->link) 
    {
        LOG_E("eth No link \n");
    }
    else
    {
        LOG_I("eth link up\n");
        eth_device_linkchange(&eqos->parent, RT_TRUE);
    }

    old_link = eqos->phy->link;
    while (1)
    {
        eqos->phy->link = phy_get_link_status(eqos->phy);
        if (old_link != eqos->phy->link)
        {
            if (eqos->phy->link)
            {
                phy_startup(eqos->phy);
                eqos_adjust_link((rt_device_t)eqos);
                if (eqos->phy->link)
                {
                    LOG_D("phy link up\n");
                    eth_device_linkchange(&eqos->parent, RT_TRUE);
                }
            }
            else
            {
                LOG_D("phy link down\n");
                eth_device_linkchange(&eqos->parent, RT_FALSE);
            }
        }
        old_link = eqos->phy->link;
        rt_thread_delay(RT_TICK_PER_SECOND);
    }
}

static int dwc_eqos_eth_init(struct eqos_eth_device *eqos)
{
    rt_err_t state = RT_EOK;

    eqos->parent.parent.ops = &_k_enet_ops;


#ifdef RT_USING_LWIP
    eqos->parent.eth_rx = rt_eqos_eth_rx;
    eqos->parent.eth_tx = rt_eqos_eth_tx;
#endif

	rt_sprintf(eqos->parent.parent.parent.name, "e%d", eqos->id);

    /* register eth device */
    state = eth_device_init(&eqos->parent, eqos->parent.parent.parent.name);

	LOG_I("init %s\n", eqos->parent.parent.parent.name);
    if (RT_EOK == state)
    {
        LOG_D("emac device init success");
    }
    else
    {
        LOG_E("emac device init failed: %d", state);
        state = -RT_ERROR;
    }
    /* start phy link detect */
    rt_thread_t phy_link_tid;
	char link_detect[15];
	rt_sprintf(link_detect, "link_detect_e%d", eqos->id);
    phy_link_tid = rt_thread_create(link_detect,
                                    phy_detect_thread_entry,
                                    eqos,
                                    4096,
                                    21,
                                    2);
    if (phy_link_tid != RT_NULL)
    {
        rt_thread_startup(phy_link_tid);
    }
    return state;
}
static rt_err_t eqos_probe(struct rt_platform_device *pdev)
{
	struct eqos_eth_device *eqos = rt_malloc(sizeof(struct eqos_eth_device));
	rt_memset(eqos, 0, sizeof(struct eqos_eth_device));
	rt_dm_dev_bind_fwdata(&eqos->parent.parent, pdev->parent.ofw_node, &eqos->parent);
	struct rt_device *dev = (struct rt_device *)eqos;
	rt_err_t ret;

	rt_memcpy(&eqos->config, pdev->id->data, sizeof(struct eqos_config));

	eqos->regs = rt_ofw_iomap(dev->ofw_node, 0);

	eqos->mac_regs = (void *)(eqos->regs + EQOS_MAC_REGS_BASE);
	eqos->mtl_regs = (void *)(eqos->regs + EQOS_MTL_REGS_BASE);
	eqos->dma_regs = (void *)(eqos->regs + EQOS_DMA_REGS_BASE);

	ret = rt_ofw_prop_read_u32(dev->ofw_node, "max-speed", &eqos->max_speed);
	if (ret)
		eqos->max_speed = 0;

	ret = eqos->config.ops->eqos_probe_resources(dev);
	if (ret < 0) {
		LOG_E("eqos_probe_resources() failed: %d", ret);
		goto err_remove_resources_core;
	}

	ret = eqos_probe_resources_core(dev);
	if (ret < 0) {
		LOG_E("eqos_probe_resources_core() failed: %d", ret);
		return ret;
	}

	ret = eqos->config.ops->eqos_start_clks(dev);
	if (ret < 0) {
		LOG_E("eqos_start_clks() failed: %d", ret);
		goto err_remove_resources_tegra;
	}

	eqos->mdio = mdio_alloc();
	if (!eqos->mdio) {
		LOG_E("mdio_alloc() failed");
		ret = -RT_ENOMEM;
		goto err_stop_clks;
	}
	eqos->mdio->read = eqos_mdio_read;
	eqos->mdio->write = eqos_mdio_write;
	eqos->mdio->priv = eqos;
	
	rt_sprintf(eqos->mdio->name, "eqos_mii-e%d", eqos->id);
	ret = mdio_register(eqos->mdio);
	if (ret < 0)
	{
		LOG_E("mdio_register() failed: %d", ret);
		goto err_free_mdio;
	}
	extern int general_phy_driver_register(struct rt_list_node *list);
    general_phy_driver_register(&eqos->mdio->phy_driver_list);

    rt_hw_us_delay(10);
    eqos->reg_access_ok = RT_TRUE;
    writel((eqos->config.ops->eqos_get_tick_clk_rate((rt_device_t)eqos) / 1000000) - 1, &eqos->mac_regs->us_tic_counter);

	dwc_eqos_eth_init(eqos);
	LOG_D("%s: OK\n", __func__);
	return 0;

err_free_mdio:
	mdio_free(eqos->mdio);
err_stop_clks:
	eqos->config.ops->eqos_stop_clks(dev);
err_remove_resources_tegra:
	eqos->config.ops->eqos_remove_resources(dev);
err_remove_resources_core:
	eqos_remove_resources_core(dev);

	LOG_D("%s: returns %d\n", __func__, ret);
	return ret;
}

int eqos_null_ops(struct rt_device *dev)
{
	return 0;
}

static const struct rt_ofw_node_id rk_gmac_ofw_ids[] =
{
    { .compatible = "rockchip,rk3588-gmac", .data = &eqos_rockchip_config},
	{ .compatible = "rockchip,rk3568-gmac", .data = &eqos_rockchip_config},
    { /* sentinel */ }
};

static struct rt_platform_driver gmac_eqos_driver =
{
    .name = "rk_gmac-dwmac",
    .ids = rk_gmac_ofw_ids,

    .probe = eqos_probe,
};

RT_PLATFORM_DRIVER_EXPORT(gmac_eqos_driver);
