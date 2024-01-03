#ifndef _SDHCI_SG_H
#define _SDHCI_SG_H

struct page
{
    unsigned long page_link;
};

#define SG_END		0x02UL

struct scatterlist 
{
	struct page     pg;
	unsigned int	offset:24;
	unsigned int	flags:8;
	unsigned int	length;
	unsigned long	dma_address;
};

struct sg_mapping_iter 
{
    unsigned			length;
	unsigned consumed;
	void			*addr;
};

#define SG_MITER_ATOMIC		(1 << 0)	 /* use kmap_atomic */
#define SG_MITER_TO_SG		(1 << 1)	/* flush back to phys on unmap */
#define SG_MITER_FROM_SG	(1 << 2)	/* nop */

struct scatterlist *__sdhci_sg_next(struct scatterlist *sg);
#define sg_next __sdhci_sg_next

#define for_each_sg(sglist, sg, nr, __i)	\
	for (__i = 0, sg = (sglist); __i < (nr); __i++, sg = sg_next(sg))

#define sg_dma_address(sg)	(((struct scatterlist*)(sg))->dma_address)
#define sg_dma_len(sg)		(((struct scatterlist*)(sg))->length)

#define sg_is_last(sg)		((sg)->flags & SG_END)

void sg_init_one(struct scatterlist *sg, const void *buf, unsigned int buflen);
struct page *sg_page(struct scatterlist *sg);
void sg_set_buf(struct scatterlist *sg, const void *buf,
			      unsigned int buflen);
void sg_init_table(struct scatterlist *sgl, unsigned int nents);

static inline void *sg_virt(struct scatterlist *sg)
{
    return (void*)(sg->pg.page_link + sg->offset);
}

struct page *sg_page(struct scatterlist *sg);
size_t sg_copy_from_buffer(struct scatterlist *sgl, unsigned int nents,
			   const void *buf, size_t buflen);
size_t sg_copy_to_buffer(struct scatterlist *sgl, unsigned int nents,
			 void *buf, size_t buflen);
void sg_miter_start(struct sg_mapping_iter *miter, struct scatterlist *sgl,
		    unsigned int nents, unsigned int flags);
void sg_miter_stop(struct sg_mapping_iter *miter);
bool sg_miter_next(struct sg_mapping_iter *miter);

#endif
