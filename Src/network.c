#include "string.h"
#include <stm32f4xx.h>
#include <stm32f4xx_hal_uart.h>
#include "network.h"

#define ESP8266_AT_CWMODE           "AT+CWMODE_CUR=1\r\n"
#define ESP8266_AT_CWJAP            "AT+CWJAP_CUR=\"%s\",\"%s\"\r\n"

extern UART_HandleTypeDef huart2;

uint8_t connect_ap(const char *ssid, const char *passwd);

struct network nett = {
    connect_ap
};

static uint8_t uATCommand(const char *pATCommand, const char *pSuccess, 
                    const char *pFail, int iTimeOut)
{
    uint8_t uRet = 0;
    uint32_t uStartTime, uEndTime;
    char recv_buffer[128] = {0};
    HAL_UART_Transmit(&huart2, (uint8_t *)pATCommand, strlen(pATCommand), 0xFFFF);
    
    // �ڳ�ʱʱ�䷶Χ�ڣ���δ�������ɹ�����Ϣ���ʾʧ�ܲ��˳�
    uStartTime = HAL_GetTick();
    for (int i = 0; i < 128; i++) {
        HAL_UART_Receive(&huart2, (uint8_t *)&recv_buffer[i], 1, 0xFFFF);
        // �ж���û���յ��ɹ��ķ���ֵ
        if (strstr(recv_buffer, pSuccess)) {
            uRet = 0;
            break;
        }
        // �ж���û���յ�ʧ�ܵķ���ֵ
        else if (strstr(recv_buffer, pFail)) {
            uRet = 1;
            break;
        }
        // �ж���û�г�ʱ
        uEndTime = HAL_GetTick();
        if ((uEndTime - uStartTime) > iTimeOut) {
            uRet = 2;
            break;
        }
    }
    printf("recv:%s, ret:%d\r\n", recv_buffer, uRet);
    return uRet;
}

uint8_t connect_ap(const char *ssid, const char *passwd)
{
    uint8_t uRet = 0;
    char buffer[64] = {0};
    
    // ����RST����Ϊ�ߵ�ƽ
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_6, 1);
    HAL_Delay(500);
    
    // ����ΪSTATIONģʽ
    uRet = uATCommand(ESP8266_AT_CWMODE, "OK", "FAIL", 1000);
    if (uRet) return uRet;
    
    // ���ӵ�AP�ȵ�
    snprintf(buffer, 64, ESP8266_AT_CWJAP, ssid, passwd);
    uRet = uATCommand(buffer, "OK", "FAIL", 10000);
    if (uRet) return uRet;
}
