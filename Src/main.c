/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dcmi.h"
#include "dma.h"
#include "fatfs.h"
#include "i2c.h"
#include "i2s.h"
#include "sdio.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "network.h"
#include "log.h"
#include "delay.h"
#include "dht11.h"
#include "ds18b20.h"
#include "mw_wifi.h"
#include "mw_mqtt.h"
#include "wm8978.h"
#include "mw_file.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void ADC_AN14_Init(void)
{
    /* 设置ADC功能对应的GPIO端口 */
    RCC->AHB1ENR |= 1 << 0;
    GPIOA->MODER &= ~(3 << (14 * 2));
    GPIOA->MODER |= 3 << (14 * 2);
    
    /* 配置ADC采样模式 */
    RCC->APB2ENR |= 1 << 8;     //使能ADC模块时钟    ADC1->CR1 &= ~(3 << 24);    //12位分辨率    ADC1->CR1 &= ~(1 << 8);     //非扫描模�?    ADC1->CR2 &= ~(3 << 28);    //禁止外部触发    ADC1->CR2 &= ~(1 << 11);    //右对�?    ADC1->CR2 &= ~(1 << 1);     //单次转换    ADC->CCR &= ~(3 << 16);
    ADC->CCR |= 2 << 16;        //6分频    ADC1->SMPR2 &= ~(0x07 << 6);
    ADC1->SMPR1 |= 0x07 << 12;   //480采样周期    ADC1->SQR1 &= ~(0x0f << 20); //1次转�?    ADC1->SQR3 &= ~(0x1f << 0);
    ADC1->SQR3 |= 14 << 0;     //转换的�?�道为�?�道14   
    /* 使能ADC */
    ADC1->CR2 |= 1 << 0;          //�?启ADC
}

uint16_t Get_ADC_1_CH2(void)
{
    uint8_t i,j;
    uint16_t buff[10] = {0};
    uint16_t temp;
    
    for(i = 0; i < 10; i ++)
    {
        /* �?始转�? */
        ADC1->CR2 |= 1 << 30;
		
        /* 等待转换结束 */
        while( !(ADC1->SR & (1 << 1)) )
        {
            /* 等待转换接收 */
        }
        buff[i] = ADC1->DR;    //读取转换结果    
    }
    
    /* 把读取�?�的数据按从小到大的排列 */
    for(i = 0; i < 9; i++)
    {
        for(j = 0 ; j < 9-i; j++)
        {
            if(buff[j] > buff[j+1])
            {
                temp = buff[j];
                buff[j] = buff[j+1];
                buff[j+1] = temp;
            }
        }
    }
    
    /* 求平均�?? */
    temp = 0;
    for(i = 1; i < 9; i++)
    {
        temp += buff[i];
    }
    
    return temp;
}

uint16_t Get_ADC_1_CH14(void)
{
    HAL_ADC_Start(&hadc1);
        
    
    uint8_t i,j;
    uint16_t buff[10] = {0};
    uint16_t temp;
    
    for(i = 0; i < 10; i ++)
    {
        /* �?始转�? */
        HAL_ADC_Start(&hadc1);
		
        /* 等待转换结束 */
        HAL_ADC_PollForConversion(&hadc1, 0xFFFF);
        
        buff[i] = HAL_ADC_GetValue(&hadc1);    //读取转换结果    
    }
    
    /* 把读取�?�的数据按从小到大的排列 */
    for(i = 0; i < 9; i++)
    {
        for(j = 0 ; j < 9-i; j++)
        {
            if(buff[j] > buff[j+1])
            {
                temp = buff[j];
                buff[j] = buff[j+1];
                buff[j+1] = temp;
            }
        }
    }
    
    /* 求平均�?? */
    temp = 0;
    for(i = 1; i < 9; i++)
    {
        temp += buff[i];
    }
    
    return temp/8;
}

int Print_Wave(void)
{
    int temp, i;
    
    temp = Get_ADC_1_CH14() / 30;   // 缩小�?个�?�数    
    //printf("temp:%d\r\n", temp);
    for (i = 0; i < temp; i++)
        printf("*");
    
    printf ("\r\n");
    return temp/8;
}

void test_dht11()
{
    uint8_t temperature;
    uint8_t humidity;
    char buffer[10] = {0};
    
    // 初始化DHT11温湿度传感器
    while(DHT11_Init()) {
        LOG(EERROR, "DHT11 init fail");
        HAL_Delay(1000);
    }

    // 设置网络传输的�?�度
    net.begin(115200);
    // 连接AP热点
    net.connect_ap("AIBIT", "12348765");
    // 连接TCP服务�?
    net.connect_server("TCP", "127.0.0.1", 9999);
    
    while(1) {
        // 读取温湿度信�?
        DHT11_Read_Data(&temperature, &humidity);
        snprintf(buffer, sizeof(buffer), "%d:%d", temperature, humidity);
        // 发�?�温湿度信息到服务器
        net.send(buffer, strlen(buffer));
        LOG(EDEBUG, "t:%d, h:%d", temperature, humidity);
        HAL_Delay(1000);
    }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
    uint8_t temperature;
	uint8_t humidity;
    char buffer[20] = {0};
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_UART4_Init();
  MX_TIM7_Init();
  MX_DCMI_Init();
  MX_SDIO_SD_Init();
  MX_USART3_UART_Init();
  MX_FATFS_Init();
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_TIM4_Init();
  MX_TIM1_Init();
  MX_I2S2_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);                                                                                              
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
  int pwmMinVal = 5;
  int pwmMaxVal = 25;
  int pwmStep = 1;
  int pwmVal = pwmMinVal;
  int pwmFlag = 1;
  while(1) {
      HAL_Delay(50);
      __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_1, pwmVal);
      __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_2, pwmVal);
      __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_3, pwmVal);
      __HAL_TIM_SetCompare(&htim1, TIM_CHANNEL_4, pwmVal);
      __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_1, pwmVal);
      __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_2, pwmVal);
      __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_3, pwmVal);
      __HAL_TIM_SetCompare(&htim4, TIM_CHANNEL_4, pwmVal);
      if (pwmFlag) pwmVal += pwmStep;
      else pwmVal -= pwmStep;
      if (pwmVal >= pwmMaxVal) pwmFlag = 0;
      if (pwmVal <= pwmMinVal) pwmFlag = 1;
  }
   
  extern void factory_test();
  factory_test();
      
      
    
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
    uint32_t value = 0;
    float vol = 0;
    while (1)
    {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
        //HAL_ADC_Start(&hadc1);
        //value = HAL_ADC_GetValue(&hadc1);
        //printf("value:%d\r\n", value);
        #if 1
        int vol = Print_Wave();
        //snprintf(buffer, sizeof(buffer), "%d", vol);
        //net.send(buffer, strlen(buffer)-1);
        HAL_Delay(50);
        #else
        HAL_ADC_Start(&hadc1);                      // 启动ADC
        HAL_ADC_PollForConversion(&hadc1, 0xFFFF);  // 等待AD转换结束
        value = HAL_ADC_GetValue(&hadc1);           // 获取转换的结�?
        vol = (float)(value *3.3/4096);             // 把电压�?�转换为数�??
        //snprintf(buffer, sizeof(buffer), "%.2f", vol);
        //net.send(buffer, strlen(buffer)-1);
        printf("%.2f\r\n",vol);
        //for (int i=0; i < (int)(vol*100)-100; i++) {
        //    printf("*");
        //}
        //printf("\r\n");
        HAL_Delay(50);
        #endif
        #if 0
        DHT11_Read_Data(&temperature, &humidity);
        // 整型转字符串
        snprintf(buffer, 20, "%d", humidity);
        temperature = DS18B20_Get_Temp()/10;
        if(temperature<0) {
            temperature = -temperature;	 //转为正数
        }
        snprintf(buffer, 20, "%d", temperature);
        
        //net.send(buffer, strlen(buffer));
        mqtt.publish_data("temp", buffer, 0);
        HAL_Delay(1000);
        #endif
    }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S;
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 192;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void USART2_Init(int32_t speed)
{
  huart2.Instance = USART2;
  huart2.Init.BaudRate = speed;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
}
void UART4_Init(int32_t speed)
{
  huart4.Instance = UART4;
  huart4.Init.BaudRate = speed;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  LOG(EERROR, "Error_Handler");
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
