#ifndef _SDHCI_TYPES_H
#define _SDHCI_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#define u32 uint32_t
#define u16 uint16_t
#define u8 uint8_t
typedef unsigned long dma_addr_t;
typedef unsigned long phys_addr_t;
#define __le16 uint16_t
#define __le32 uint32_t
#define u64 uint64_t
typedef unsigned long size_t;
#ifndef NULL
#define NULL (void*)0
#endif
#define gfp_t unsigned
#define s32 int32_t

#endif
