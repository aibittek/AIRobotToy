#include "main.h"
#include "stm32f4xx.h"

extern volatile uint8_t wavtransferend;
extern volatile uint8_t recordend;
/**
  * @brief  Tx and Rx Transfer half completed callback
  * @param  hi2s I2S handle
  * @retval None
  */
void HAL_I2SEx_TxRxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    // HAL库在进入中断前会清除对应的DMA中断标志位，根据这个原理判断产生的中断源
    
    __I uint32_t rx_reg = DMA1->LISR;
    if (!(rx_reg & DMA_LISR_HTIF3)) {
        // DMA1的Stream3用于录音
        //printf("rx\r\n");
        //recordend = 1;
    }
    
    __I uint32_t tx_reg = DMA1->HISR;
    if (!(tx_reg & DMA_HISR_HTIF4)) {
        // DMA1的Stream4用于播放
        //printf("ht\r\n");
        //wavtransferend = 1;
    }
}

/**
  * @brief  Tx and Rx Transfer completed callback
  * @param  hi2s I2S handle
  * @retval None
  */
void HAL_I2SEx_TxRxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    // 全双工并且循环模式下不会调用
    //printf("tx\r\n");
    //recordend = 2;
    //wavtransferend = 2;
}

void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    //printf("hr\r\n");
    //recordend = 1;
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    //printf("rx\r\n");
    //recordend = 2;
}

/**
  * @brief  Tx Transfer Half completed callbacks
  * @param  hi2s pointer to a I2S_HandleTypeDef structure that contains
  *         the configuration information for I2S module
  * @retval None
  */
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    //printf("i2s ht\r\n");
    wavtransferend = 1;
}

/**
  * @brief  Tx Transfer completed callbacks
  * @param  hi2s pointer to a I2S_HandleTypeDef structure that contains
  *         the configuration information for I2S module
  * @retval None
  */
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    //printf("i2s tx\r\n");
    wavtransferend = 2;
}

/**
  * @brief  I2S error callbacks
  * @param  hi2s pointer to a I2S_HandleTypeDef structure that contains
  *         the configuration information for I2S module
  * @retval None
  */
void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
  printf("i2sErr\r\n");
}

