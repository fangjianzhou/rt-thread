#include <osdep/port.h>

static void _dma_sync(void *ptr, unsigned size,
                      enum dma_data_direction dir)
{
    if (dir == DMA_FROM_DEVICE)
        rt_hw_cpu_dcache_ops(RT_HW_CACHE_INVALIDATE, ptr, size);
    else
        rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, ptr, size);
}

static void _sg_sync(struct scatterlist *sg, unsigned nents, enum dma_data_direction dir)
{
    for (unsigned i = 0; i < nents; i ++)
    {
        _dma_sync(sg_virt(&sg[i]), sg[i].length, dir);
    }
}

int __sdhci_dma_map_sg(struct rt_device *dev, struct scatterlist *sg,
        unsigned nents, enum dma_data_direction dir)
{
    void *va;

    for (unsigned i = 0; i < nents; i ++)
    {
        va = sg_virt(sg);
        sg->dma_address = virt_to_phys(va);
        _dma_sync(va, sg->length, dir);
        sg ++;
    }

    return nents;
}

void dma_sync_sg_for_cpu(struct rt_device *dev,
        struct scatterlist *sg, int nelems, enum dma_data_direction dir)
{
    _sg_sync(sg, nelems, dir);
}

int dmaengine_terminate_sync(struct dma_chan *chan)
{
    pr_todo();
    return 0;
}

void *dma_alloc_coherent(struct rt_device *dev, size_t size,
        dma_addr_t *dma_handle, gfp_t gfp)
{
    void *v;

    v = rt_malloc_align(size, 2048);
    if (v)
    {
        *dma_handle = virt_to_phys(v);
        v = rt_ioremap((void *)*dma_handle, size);
    }

    return v;
}

void dma_free_coherent(struct rt_device *dev, size_t size,
        void *cpu_addr, dma_addr_t dma_handle)
{
    pr_todo();
}

int dma_mapping_error(struct rt_device *dev, dma_addr_t dma_addr)
{
    return 0;
}

dma_addr_t dma_map_single(struct rt_device *dev, void *ptr,
        size_t size, enum dma_data_direction dir)
{
    pr_todo();
    return 0;//
}

void dma_sync_single_for_device(struct rt_device *dev, dma_addr_t addr,
        size_t size, enum dma_data_direction dir)
{
    pr_todo();
}


void dma_sync_single_for_cpu(struct rt_device *dev, dma_addr_t addr, size_t size,
        enum dma_data_direction dir)
{
    pr_todo();
}

int dma_set_mask_and_coherent(struct rt_device *dev, rt_uint64_t mask)
{
    return 0;
}

void __dma_unmap_sg_attrs(struct rt_device *dev,
        struct scatterlist *sg, int nents, enum dma_data_direction dir,
        unsigned long attrs)
{
}
