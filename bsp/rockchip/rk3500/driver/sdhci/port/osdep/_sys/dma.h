#ifndef _SDHCI_DMA_H
#define _SDHCI_DMA_H

#include <ioremap.h>
#include "types.h"

struct device;
struct scatterlist;

struct dma_chan
{
    const char *name;
};

enum dma_data_direction
{
    DMA_BIDIRECTIONAL = 0,
    DMA_TO_DEVICE = 1,
    DMA_FROM_DEVICE = 2,
    DMA_NONE = 3,
};

#define DMA_BIT_MASK(n) (((n) == 64) ? ~0ULL : ((1ULL<<(n))-1))

int dma_set_mask_and_coherent(struct rt_device *dev, rt_uint64_t mask);
void dma_free_coherent(struct rt_device *dev, size_t size,
        void *cpu_addr, dma_addr_t dma_handle);
void *dma_alloc_coherent(struct rt_device *dev, size_t size,
        dma_addr_t *dma_handle, gfp_t gfp);
dma_addr_t dma_map_single(struct rt_device *dev, void *ptr,
        size_t size, enum dma_data_direction dir);

int __sdhci_dma_map_sg(struct rt_device *dev, struct scatterlist *sg,
        unsigned nents, enum dma_data_direction dir);
void __dma_unmap_sg_attrs(struct rt_device *dev,
        struct scatterlist *sg, int nents, enum dma_data_direction dir,
        unsigned long attrs);

#define dma_map_sg(d, s, n, r) __sdhci_dma_map_sg(d, s, n, r)
#define dma_unmap_sg(d, s, n, r) __dma_unmap_sg_attrs(d, s, n, r, 0)

void dma_sync_sg_for_cpu(struct rt_device *dev,
        struct scatterlist *sg, int nelems, enum dma_data_direction dir);
int dma_mapping_error(struct rt_device *dev, dma_addr_t dma_addr);
int dmaengine_terminate_sync(struct dma_chan *chan);
void dma_sync_single_for_cpu(struct rt_device *dev, dma_addr_t addr, size_t size,
        enum dma_data_direction dir);
void dma_sync_single_for_device(struct rt_device *dev, dma_addr_t addr,
        size_t size, enum dma_data_direction dir);

#endif
