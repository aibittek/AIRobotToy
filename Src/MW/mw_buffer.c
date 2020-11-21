#include "mw_buffer.h"

static uint8_t dma_cache_buffer[MW_DMA_CACHE_BUFFER_SIZE];
static uint8_t cache_buffer[MW_CACHE_BUFFER_SIZE];

static uint8_t             mw_buffer_begin(void);
static uint32_t            mw_buffer_read(void* pvData, uint32_t uLen);
static uint32_t            mw_buffer_write(void* pvData, uint32_t uLen);
static uint8_t             mw_buffer_empty(void);
static uint8_t             mw_buffer_full(void);
static uint32_t            mw_buffer_count(void);
static void                mw_buffer_reset(void);
static void                mw_buffer_end(void);

dma_buffer_t dma_buffer = {
    .buffer = {
        .buffer         = (char *)cache_buffer,
        .total_size     = MW_CACHE_BUFFER_SIZE,
        .mask           = MW_CACHE_BUFFER_SIZE - 1,
        .tail_index     = 0,
        .head_index     = 0,
    },
    .offset             = 0,
    .cache_buffer       = dma_cache_buffer,
    .begin              = mw_buffer_begin,
    .read               = mw_buffer_read,
    .write              = mw_buffer_write,
    .empty              = mw_buffer_empty,
    .full               = mw_buffer_full,
    .count              = mw_buffer_count,
    .reset              = mw_buffer_reset,
    .end                = mw_buffer_end
};

uint8_t             mw_buffer_begin(void)
{
    
    return 0;
}
uint32_t            mw_buffer_read(void* pvData, uint32_t uLen)
{
    return ring_buffer_dequeue_arr(&dma_buffer.buffer, pvData, uLen);
}
uint32_t            mw_buffer_write(void* pvData, uint32_t uLen)
{
    ring_buffer_queue_arr(&dma_buffer.buffer, pvData, uLen);
    return uLen;
}
uint8_t             mw_buffer_empty(void)
{
    return ring_buffer_is_empty(&dma_buffer.buffer);
}
uint8_t             mw_buffer_full(void)
{
    return ring_buffer_is_full(&dma_buffer.buffer);
}
uint32_t            mw_buffer_count(void)
{
    return ring_buffer_num_items(&dma_buffer.buffer);
}
void                mw_buffer_reset(void)
{
    memset(dma_buffer.buffer.buffer, 0, dma_buffer.buffer.total_size);
    dma_buffer.offset = 0;
    dma_buffer.buffer.head_index = dma_buffer.buffer.tail_index = 0;
}
void                mw_buffer_end(void)
{
    dma_buffer.offset = 0;
    dma_buffer.buffer.total_size = 0;
    dma_buffer.buffer.head_index = dma_buffer.buffer.tail_index = 0;
}