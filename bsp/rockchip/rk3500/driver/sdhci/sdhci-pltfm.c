// SPDX-License-Identifier: GPL-2.0-only
/*
 * sdhci-pltfm.c Support for SDHCI platform devices
 * Copyright (c) 2009 Intel Corporation
 *
 * Copyright (c) 2007, 2011 Freescale Semiconductor, Inc.
 * Copyright (c) 2009 MontaVista Software, Inc.
 *
 * Authors: Xiaobo Xie <X.Xie@freescale.com>
 *      Anton Vorontsov <avorontsov@ru.mvista.com>
 */

/* Supports:
 * SDHCI platform devices
 *
 * Inspired by sdhci-pci.c, by Pierre Ossman
 */

#include "sdhci-pltfm.h"

static const struct sdhci_ops sdhci_pltfm_ops = {
    .set_clock = sdhci_set_clock,
    .set_bus_width = sdhci_set_bus_width,
    .reset = sdhci_reset,
    .set_uhs_signaling = sdhci_set_uhs_signaling,
};

static bool sdhci_wp_inverted(struct rt_device *dev)
{
    if (rt_dm_dev_prop_read_bool(dev, "sdhci,wp-inverted") ||
        rt_dm_dev_prop_read_bool(dev, "wp-inverted"))
        return true;

    /* Old device trees don't have the wp-inverted property. */
#ifdef CONFIG_PPC
    return machine_is(mpc837x_rdb) || machine_is(mpc837x_mds);
#else
    return false;
#endif /* CONFIG_PPC */
}

void sdhci_get_compatibility(struct rt_platform_device *pdev) {}

void sdhci_get_property(struct rt_platform_device *pdev)
{
    struct rt_device *dev = &pdev->parent;
    struct sdhci_host *host = pdev->priv;
    struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
    rt_uint32_t bus_width;

    if (rt_dm_dev_prop_read_bool(dev, "sdhci,auto-cmd12"))
        host->quirks |= SDHCI_QUIRK_MULTIBLOCK_READ_ACMD12;

    if (rt_dm_dev_prop_read_bool(dev, "sdhci,1-bit-only") ||
        (rt_dm_dev_prop_read_u32(dev, "bus-width", &bus_width) == 0 &&
        bus_width == 1))
        host->quirks |= SDHCI_QUIRK_FORCE_1_BIT_DATA;

    if (sdhci_wp_inverted(dev))
        host->quirks |= SDHCI_QUIRK_INVERTED_WRITE_PROTECT;

    if (rt_dm_dev_prop_read_bool(dev, "broken-cd"))
        host->quirks |= SDHCI_QUIRK_BROKEN_CARD_DETECTION;

    if (rt_dm_dev_prop_read_bool(dev, "no-1-8-v"))
        host->quirks2 |= SDHCI_QUIRK2_NO_1_8_V;

    sdhci_get_compatibility(pdev);

    rt_dm_dev_prop_read_u32(dev, "clock-frequency", &pltfm_host->clock);

    if (rt_dm_dev_prop_read_bool(dev, "keep-power-in-suspend"))
        host->mmc->pm_caps |= MMC_PM_KEEP_POWER;

    if (rt_dm_dev_prop_read_bool(dev, "wakeup-source") ||
        rt_dm_dev_prop_read_bool(dev, "enable-sdio-wakeup")) /* legacy */
        host->mmc->pm_caps |= MMC_PM_WAKE_SDIO_IRQ;
}

struct sdhci_host *sdhci_pltfm_init(struct rt_platform_device *pdev,
                    const struct sdhci_pltfm_data *pdata,
                    size_t priv_size)
{
    struct sdhci_host *host;
    struct rt_device *dev = &pdev->parent;
    void *ioaddr;
    int irq, ret;

    ioaddr = rt_dm_dev_iomap(dev, 0);
    if (IS_ERR(ioaddr)) {
        ret = PTR_ERR(ioaddr);
        goto err;
    }

    irq = rt_dm_dev_get_irq(dev, 0);
    if (irq < 0) {
        ret = irq;
        goto err;
    }

    host = sdhci_alloc_host(dev,
        sizeof(struct sdhci_pltfm_host) + priv_size);

    if (IS_ERR(host)) {
        ret = PTR_ERR(host);
        goto err;
    }

    host->ioaddr = ioaddr;
    host->irq = irq;
    host->hw_name = rt_dm_dev_get_name(dev);

    if (pdata && pdata->ops)
        host->ops = pdata->ops;
    else
        host->ops = &sdhci_pltfm_ops;

    if (pdata) {
        host->quirks = pdata->quirks;
        host->quirks2 = pdata->quirks2;
    }

    pdev->priv = host;

    return host;
err:
    dev_err(dev, "%s failed %d\n", __func__, ret);
    return ERR_PTR(ret);
}

void sdhci_pltfm_free(struct rt_platform_device *pdev)
{
    struct sdhci_host *host = pdev->priv;

    sdhci_free_host(host);
}
