#include <rtthread.h>

#include <string.h>
#include <mm_aspace.h>
#include <osdep/port.h>

int __sdhci_ilog2(int v)
{
    return __rt_ffs(v);
}

struct scatterlist *__sdhci_sg_next(struct scatterlist *sg)
{
    if (sg_is_last(sg))
        return 0;

    sg ++;

    return sg;
}

size_t sg_copy_to_buffer(struct scatterlist *sgl, unsigned int nents,
			 void *buf, size_t buflen)
{
	pr_todo();
	return 0;
}

size_t sg_copy_from_buffer(struct scatterlist *sgl, unsigned int nents,
			   const void *buf, size_t buflen)
{
	pr_todo();
	return 0;
}

void sg_miter_stop(struct sg_mapping_iter *miter)
{
    pr_todo();
}

bool sg_miter_next(struct sg_mapping_iter *miter)
{
    pr_todo();
    return true;
}

void sg_miter_start(struct sg_mapping_iter *miter, struct scatterlist *sgl,
		    unsigned int nents, unsigned int flags)
{
     pr_todo();
}

void sg_init_one(struct scatterlist *sg, const void *buf, unsigned int buflen)
{
    sg->pg.page_link = (unsigned long)buf & ~(0xfffUL);
    sg->offset = (unsigned long)buf & 0xfff;
    sg->length = buflen;
    sg->dma_address = 0;
    sg->flags |= SG_END;
}

void sg_init_table(struct scatterlist *sgl, unsigned int nents)
{
    memset(sgl, 0, sizeof(*sgl) * nents);
    sgl[nents - 1].flags |= SG_END;
}

void sg_set_buf(struct scatterlist *sg, const void *buf,
			      unsigned int buflen)
{
    sg->pg.page_link = (unsigned long)buf & ~(0xfffUL);
    sg->offset = (unsigned long)buf & 0xfff;
    sg->length = buflen;
    sg->dma_address = 0;
}

struct page *sg_page(struct scatterlist *sg)
{
    return &sg->pg;
}

void *kmap_atomic(struct page *page)
{
    return (void*)page->page_link;
}

void kunmap_atomic(struct page *page)
{
}

unsigned int swiotlb_max_segment(void)
{
	return 0;
}

void __iomem *
devm_platform_ioremap_resource(struct platform_device *pdev,
			       unsigned int index)
{
    return ioremap(pdev->base, 0x1000);
}

const char *dev_name(const struct rt_device *dev)
{
	if (dev)
	    return dev->parent.name;

	return "";
}

#include <ioremap.h>
void *__ioremap(phys_addr_t offset, size_t size)
{
    return rt_ioremap((void*)offset, size);
}

void __iounmap(volatile void __iomem *addr)
{
    rt_iounmap(addr);
}

unsigned long __virt_to_phys(volatile void *address)
{
    return (unsigned long)((u64)address + PV_OFFSET);
}
