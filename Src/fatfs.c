/**
  ******************************************************************************
  * @file   fatfs.c
  * @brief  Code for fatfs applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

#include "fatfs.h"

uint8_t retSD;    /* Return value for SD */
char SDPath[4];   /* SD logical drive path */
FATFS SDFatFS;    /* File system object for SD logical drive */
FIL SDFile;       /* File object for SD */

/* USER CODE BEGIN Variables */
#include "log.h"
extern SD_HandleTypeDef hsd;
/* USER CODE END Variables */    

void MX_FATFS_Init(void) 
{
  /*## FatFS: Link the SD driver ###########################*/
  retSD = FATFS_LinkDriver(&SD_Driver, SDPath);

  /* USER CODE BEGIN Init */
  /* additional user code for init */
    retSD = f_mount(&SDFatFS, SDPath, 1);
    if (retSD == FR_NO_FILESYSTEM) {
		f_mkfs(SDPath, 1, 512, NULL, 4096);
	}
    
    HAL_SD_CardCIDTypeDef CID;
    HAL_StatusTypeDef s1 = HAL_SD_GetCardCID(&hsd, &CID);
    HAL_SD_CardCSDTypeDef CSD;
    HAL_StatusTypeDef s2 = HAL_SD_GetCardCSD(&hsd, &CSD);
    HAL_SD_CardStatusTypeDef status;
    HAL_StatusTypeDef s3 = HAL_SD_GetCardStatus(&hsd, &status);
    HAL_SD_CardInfoTypeDef cardInfo;
    HAL_StatusTypeDef s4 = HAL_SD_GetCardInfo(&hsd, &cardInfo);

    float fCardSize = 1.0*cardInfo.BlockNbr*cardInfo.BlockSize/1024/1024;
    LOG(EDEBUG, "SDPath:%s", SDPath);
    LOG(EDEBUG, "Card Size:%.2f M", fCardSize);
  /* USER CODE END Init */
}

/**
  * @brief  Gets Time from RTC 
  * @param  None
  * @retval Time in DWORD
  */
DWORD get_fattime(void)
{
  /* USER CODE BEGIN get_fattime */
  return 0;
  /* USER CODE END get_fattime */  
}

/* USER CODE BEGIN Application */
     
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
