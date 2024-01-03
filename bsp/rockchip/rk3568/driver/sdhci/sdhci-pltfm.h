/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright 2010 MontaVista Software, LLC.
 *
 * Author: Anton Vorontsov <avorontsov@ru.mvista.com>
 */

#ifndef _DRIVERS_MMC_SDHCI_PLTFM_H
#define _DRIVERS_MMC_SDHCI_PLTFM_H
#include <drivers/core/rtdm.h>
#include <drivers/ofw.h>
#include <drivers/platform.h>
#include <drivers/clk.h>
#include <drivers/reset.h>
#include "sdhci.h"

struct sdhci_pltfm_data {
	const struct sdhci_ops *ops;
	unsigned int quirks;
	unsigned int quirks2;
};

struct sdhci_pltfm_host {
	struct rt_clk *clk;
	/* migrate from sdhci_of_host */
	unsigned int clock;
	u16 xfer_mode_shadow;

	unsigned long private[] ____cacheline_aligned;
};

void sdhci_get_property(struct rt_platform_device *pdev);

static inline void sdhci_get_of_property(struct rt_platform_device *pdev)
{
	return sdhci_get_property(pdev);
}

extern struct sdhci_host *sdhci_pltfm_init(struct rt_platform_device *pdev,
					  const struct sdhci_pltfm_data *pdata,
					  size_t priv_size);
extern void sdhci_pltfm_free(struct rt_platform_device *pdev);

static inline void *sdhci_pltfm_priv(struct sdhci_pltfm_host *host)
{
	return host->private;
}

#endif /* _DRIVERS_MMC_SDHCI_PLTFM_H */
