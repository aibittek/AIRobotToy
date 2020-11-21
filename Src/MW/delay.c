#include <stm32f4xx.h>
#include <stm32f4xx_hal_tim.h>
#include "delay.h"

extern TIM_HandleTypeDef htim7;
void delay_us(uint16_t us)
{
    uint16_t differ=0xffff-us-5;
    
    HAL_TIM_Base_Start(&htim7);
    __HAL_TIM_SetCounter(&htim7,differ);
    while(differ < 0xffff-5)
    {
        differ = __HAL_TIM_GetCounter(&htim7);
    }
    HAL_TIM_Base_Stop(&htim7);

}
