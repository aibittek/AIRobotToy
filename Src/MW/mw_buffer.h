#ifndef _MW_BUFFER_H
#define _MW_BUFFER_H

#include <stm32f4xx.h>
#include <ringbuffer.h>

#define MW_DMA_CACHE_BUFFER_SIZE                (512)
#define MW_CACHE_BUFFER_SIZE                    (4096)
#if (MW_CACHE_BUFFER_SIZE & (MW_CACHE_BUFFER_SIZE - 1)) != 0
#error "RING_BUFFER_SIZE must be a power of two"
#endif

typedef struct {
    ring_buffer_t       buffer;
    uint32_t            offset;
    uint8_t             *cache_buffer;
    
    uint8_t             (*begin)            (void);
    uint32_t            (*read)             (void* pvData, uint32_t uLen);
    uint32_t            (*write)            (void* pvData, uint32_t uLen);
    uint8_t             (*empty)            (void);
    uint8_t             (*full)             (void);
    uint32_t            (*count)            (void);
    void                (*reset)            (void);
    void                (*end)              (void);
}dma_buffer_t;
extern dma_buffer_t dma_buffer;

#endif
