#include "main.h"
#include "mw_buffer.h"
#include "stm32f4xx.h"

extern DMA_HandleTypeDef hdma_usart2_rx;

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    //printf("tc\r\n");
}

void HAL_UART_TxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    //printf("ht\r\n");
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    uint32_t uWriten;
    uint8_t *pDMA = dma_buffer.cache_buffer + dma_buffer.offset;
    uint32_t uRecvLen = MW_DMA_CACHE_BUFFER_SIZE - dma_buffer.offset;
    uWriten = dma_buffer.write(pDMA, uRecvLen);
    dma_buffer.offset = 0;
    //printf("rx:%s, l:%d, w:%d\r\n", pDMA, uRecvLen, uWriten);
    //printf("%d\r\n", dma_buffer.count());
}

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
    uint32_t uWriten;
    uint8_t *pDMA = dma_buffer.cache_buffer + dma_buffer.offset;
    uint32_t uRecvLen = MW_DMA_CACHE_BUFFER_SIZE/2 - dma_buffer.offset;
    uWriten = dma_buffer.write(pDMA, uRecvLen);
    dma_buffer.offset += uWriten;
    //printf("hr:%s, l:%d, w:%d\r\n", pDMA, uRecvLen, uWriten);
}
