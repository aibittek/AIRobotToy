#include <stdio.h>
#include <string.h>
#include <stm32f4xx.h>
#include <stm32f4xx_hal_gpio.h>
#include <stm32f4xx_hal_uart.h>
#include <log.h>
#include "mw_buffer.h"
#include "mw_wifi.h"

#define AT_DEFAULT_SPEED                115200
#define AT_HIGH_SPEED                   921600
#define AT_COMMAND_SPEED                "AT+UART_CUR=%d,8,1,0,0\r\n"
#define AT_COMMAND_STAION_MODE(v)       "AT+CWMODE="#v"\r\n"
#define AT_COMMAND_CONNECT_AP           "AT+CWJAP_CUR=\"%s\",\"%s\"\r\n"
#define AT_COMMAND_CONNECT_CHECK        "AT+CWJAP_CUR?\r\n"
#define AT_COMMAND_CIPSTART             "AT+CIPSTART=\"%s\",\"%s\",%d\r\n"
#define AT_COMMAND_CIPMODE(v)           "AT+CIPMODE="#v"\r\n"
#define AT_COMMAND_CIPCLOSE             "AT+CIPCLOSE\r\n"

extern UART_HandleTypeDef huart2;
static uint8_t uATCommand(const char *pCommand, const char *pSuc, const char *pFail, uint32_t uTimeOut)
{
    uint8_t uRet = 0;
    uint32_t uStart, uEnd;
    uint32_t uReadLen, uTotalLen;
    char buffer[128] = {0};
    HAL_UART_Transmit_DMA(&huart2, (uint8_t *)pCommand, strlen(pCommand));
    
    uStart = HAL_GetTick();
    uTotalLen = uReadLen = 0;
    while (1) {
        uReadLen = dma_buffer.count();
        if (uReadLen + uTotalLen >= sizeof(buffer)) {
            uRet = 3;
            break;
        }
        if (uReadLen > 0) {
            uTotalLen += dma_buffer.read(buffer+uTotalLen, uReadLen);
        }
        if (strstr(buffer, pSuc)) {
            uRet = 0;
            break;
        } else if (strstr(buffer, pFail)) {
            uRet = 1;
            break;
        }
        HAL_Delay(10);
        uEnd = HAL_GetTick();
        if ((uEnd - uStart) > uTimeOut) {
            uRet = 2;
            break;
        }
    }
    LOG(EDEBUG, "ret:%d, recv:#%s#", uRet, buffer);
    //dma_buffer.reset();
    //HAL_UART_DMAStop(&huart2);
    return uRet;
}

static void esp01_begin(int32_t speed)
{
    // 芯片RESET
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_Delay(500);
    
    // 打开串口的空闲中断
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
    // 打开串口的DMA接收，使用的是循环模式，采用空闲中断+DMA半中断和DMA完成中断这三个中断来获取不定长数据
    HAL_UART_Receive_DMA(&huart2, dma_buffer.cache_buffer, MW_DMA_CACHE_BUFFER_SIZE);
    
    // 关闭已经连接的网络
    net.close();
    
    // 传输速度设置
    uint8_t uRet;
    uint8_t nCount = 3;
    char buffer[64] = {0};
    extern void USART2_Init(int32_t);
    extern void UART4_Init(int32_t);
    snprintf(buffer, 64, AT_COMMAND_SPEED, speed);
    while(nCount--) {
        uRet = uATCommand(buffer, "OK", "FAIL", 1000);
        if (0 == uRet) {
            USART2_Init(speed);
            UART4_Init(speed);
            break;
        }
    }
}

// 函数指针所指向的函数
static uint8_t esp01_connect_ap(const char *ssid, const char *passwd)
{
    uint8_t uRet;
    char buffer[128] = {0};
    
    // 设置为STATION模式
    while (1) {
    uRet = uATCommand(AT_COMMAND_STAION_MODE(3), "OK", "FAIL", 1000);
    _ASSERT(0 == uRet);
        HAL_Delay(1000);
        break;
    }
    
    // 连接net热点
    snprintf(buffer, 128, AT_COMMAND_CONNECT_AP, ssid, passwd);
    uRet = uATCommand(buffer, "OK", "FAIL", 10000);
    _ASSERT(0 == uRet);
    
    uRet = uATCommand(AT_COMMAND_CONNECT_CHECK, ssid, "FAIL", 1000);
    _ASSERT(0 == uRet);
    
    return uRet;
}

static uint8_t esp01_connect_server(const char *pType, const char *pDomain, int16_t nPort)
{
    // 连接服务器
    uint8_t uRet;
    char buffer[128] = {0};
    snprintf(buffer, 128, AT_COMMAND_CIPSTART, pType, pDomain, nPort);
    uRet = uATCommand(buffer, "OK", "ERROR", 10000);
    _ASSERT(0 == uRet);
    
    // 设置透传模式
    // 透传模式有丢数据的风险，业务层规避，非透传模式下速率上不去，不使用
    uRet = uATCommand(AT_COMMAND_CIPMODE(1), "OK", "ERROR", 1000);
    _ASSERT(0 == uRet);
  
    // 发送数据
    uRet = uATCommand("AT+CIPSEND\r\n", ">", "ERROR", 1000);
    _ASSERT(0 == uRet);
    
    return uRet;
}

static uint32_t esp01_send(void *pvData, int32_t iLen)
{
    return HAL_UART_Transmit(&huart2, (uint8_t *)pvData, iLen, 0xFFFF);
}

static uint32_t esp01_recv(void *pvData, int32_t iLen, uint32_t uTimeOut)
{
    return dma_buffer.read(pvData, iLen);
}

static uint32_t esp01_close()
{
    uint8_t uRet;
    
    dma_buffer.reset();
    
    // 退出透传
    char *pCommand = "+++";
    HAL_UART_Transmit(&huart2, (uint8_t *)pCommand, strlen(pCommand), 0xFFFF);
    HAL_Delay(1500);

    // 断开连接
    uRet = uATCommand(AT_COMMAND_CIPCLOSE, "OK", "ERROR", 1000);
    _ASSERT(0 == uRet);
    
    return uRet;
}

_net_t net = {
    esp01_begin,
    esp01_connect_ap,
    esp01_connect_server,
    esp01_send,
    esp01_recv,
    esp01_close
};

#if 1
void test_net()
{
    //net.begin(AT_HIGH_SPEED);
    net.begin(AT_DEFAULT_SPEED);
    
    // 1、连接热点
    //if (0 != net.connect_ap("AIBIT","12348765")) {
    if (0 != net.connect_ap("AIBITPC","12348765")) {
    //if (0 != net.connect_ap("ChinaNet-TVLK","qymgv4uk")) {
        printf("连接net热点失败");
        return ;
    }
    
    // 2、连接服务器
    if (0 != net.connect_server("TCP", "www.baidu.com", 80)) {
        LOG(EDEBUG, "connect server failed");
        return ;
    }

    // 3、发送数据给服务器
    char *pHttpRequest = "GET / HTTP/1.1\r\nHost: www.baidu.com\r\n\r\n";
    net.send(pHttpRequest, strlen(pHttpRequest));
    
    uint32_t uRecvLen, uTotalLen = 0;
    static char recv_buffer[4096+1];
    uint32_t uStart, uEnd;
    uStart = HAL_GetTick();
    while(1) {
        uRecvLen = net.recv(recv_buffer, 4096, 1000);
        printf("%s", recv_buffer);
        memset(recv_buffer, 0, 4096);
        uTotalLen += uRecvLen;
        uEnd = HAL_GetTick();
        if (uEnd-uStart > 3000) {
            break;
        }
    }
    LOG(EDEBUG, "total get size:%d", uTotalLen);
    HAL_Delay(100);
    // 4、接收服务器数据
    
    //net.recv(recv_buffer, 100, 5000);
    //HAL_Delay(3000);
    //printf("recv_buffer:%s\r\n", recv_buffer);
    
    // 5、关闭和服务端的连接
    net.close();
}

void test_net_speed()
{
    uint32_t uStart, uEnd;
    uint32_t nCount = 0;
    uint32_t nTotalCount = 0;
    uStart = HAL_GetTick();
    static char g_buffer[] = "7890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890";
    
    // 网络初始化
    net.begin(921600);
    net.connect_ap("ChinaNet-TVLK","qymgv4uk");
    //net.connect_ap("AIBIT","12348765");
    //net.connect_server("TCP", "139.196.48.194", 9999);
    net.connect_server("TCP", "192.168.1.6", 8080);

    while (1)
    {
        uint8_t uRet = net.send(g_buffer, 1024);
        if (uRet) {
            LOG(EDEBUG, "send error");
        }
        //HAL_Delay(60);
        nCount++;
        nTotalCount++;
        //HAL_Delay(10);
        uEnd = HAL_GetTick();
        if (uEnd-uStart >= 1000) {
            LOG(EDEBUG, "%d byte/s", nCount*1024);
            nCount = 0;
            uStart = uEnd;
        }
        if (nTotalCount >= 50) {
            LOG(EDEBUG, "total:%u", nTotalCount*1024);
            HAL_Delay(2000);
            nTotalCount = 0;
        }
    }
}

#endif
