#ifndef _SDHCI_TYPES_H
#define _SDHCI_TYPES_H

#include <stdint.h>
#include <stdbool.h>

typedef unsigned long dma_addr_t;
typedef unsigned long phys_addr_t;
#define __le16 uint16_t
#define __le32 uint32_t
typedef unsigned long size_t;

#define SIZE_KB 1024
#define SIZE_MB (1024 * SIZE_KB)
#define SIZE_GB (1024 * SIZE_MB)

#define SIZE_ALIGN(size, align)         (((size) + (align) - 1) & ~((align) - 1))
#define SIZE_ALIGN_DOWN(size, align)    ((size) & ~((align) - 1))

#ifndef NULL
#define NULL (void*)0
#endif
#define gfp_t unsigned

#endif
