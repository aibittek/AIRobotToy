#include <string.h>
#include <stm32f4xx.h>
#include <stm32f4xx_hal_uart.h>
#include <log.h>
#include "mw_wifi.h"
#include "mw_mqtt.h"

#define BYTE0(dwTemp)       (*( char *)(&dwTemp))
#define BYTE1(dwTemp)       (*((char *)(&dwTemp) + 1))
#define BYTE2(dwTemp)       (*((char *)(&dwTemp) + 2))
#define BYTE3(dwTemp)       (*((char *)(&dwTemp) + 3))

extern UART_HandleTypeDef huart2;

typedef enum
{
	//名字 	    值 			报文流动方向 	描述
	M_RESERVED1	=0	,	//	禁止	保留
	M_CONNECT		,	//	客户端到服务端	客户端请求连接服务端
	M_CONNACK		,	//	服务端到客户端	连接报文确认
	M_PUBLISH		,	//	两个方向都允许	发布消息
	M_PUBACK		,	//	两个方向都允许	QoS 1消息发布收到确认
	M_PUBREC		,	//	两个方向都允许	发布收到（保证交付第一步）
	M_PUBREL		,	//	两个方向都允许	发布释放（保证交付第二步）
	M_PUBCOMP		,	//	两个方向都允许	QoS 2消息发布完成（保证交互第三步）
	M_SUBSCRIBE		,	//	客户端到服务端	客户端订阅请求
	M_SUBACK		,	//	服务端到客户端	订阅请求报文确认
	M_UNSUBSCRIBE	,	//	客户端到服务端	客户端取消订阅请求
	M_UNSUBACK		,	//	服务端到客户端	取消订阅报文确认
	M_PINGREQ		,	//	客户端到服务端	心跳请求
	M_PINGRESP		,	//	服务端到客户端	心跳响应
	M_DISCONNECT	,	//	客户端到服务端	客户端断开连接
	M_RESERVED2		,	//	禁止	保留
}Emqtt_MESSAGE_T;

//连接成功服务器回应 20 02 00 00
//客户端主动断开连接 e0 00
const uint8_t parket_connetAck[] = {0x20,0x02,0x00,0x00};
const uint8_t parket_disconnet[] = {0xe0,0x00};
const uint8_t parket_heart[] = {0xc0,0x00};
const uint8_t parket_heart_reply[] = {0xc0,0x00};
const uint8_t parket_subAck[] = {0x90,0x03};

static void mqtt_sendbuf(uint8_t *buf,uint16_t len);
static void mqtt_recvbuf(uint8_t *buf, uint16_t len, uint32_t uTimeOut);

static void esp8266_begin();
static uint8_t esp8266_connect(char *ClientID,char *Username,char *Password);
static uint8_t esp8266_subscribe(char *topic,uint8_t qos,uint8_t whether);
static uint8_t esp8266_publish(char *topic, char *message, uint8_t qos);
static void esp8266_send_heart(void);
static void esp8266_disconnect(void);
static void esp8266_end(void);

_mqtt_t mqtt = 
{
	{0},
	{0},
    MQTT_BUFFER_SIZE,
    MQTT_BUFFER_SIZE,
	esp8266_begin,
	esp8266_connect,
	esp8266_subscribe,
	esp8266_publish,
	esp8266_send_heart,
	esp8266_disconnect,
    esp8266_end,
};

static void esp8266_begin()
{
	//无条件先主动断开
	mqtt.disconnect();HAL_Delay(100);
	mqtt.disconnect();HAL_Delay(100);
}

//连接服务器的打包函数
static uint8_t esp8266_connect(char *ClientID,char *Username,char *Password)
{
    int ClientIDLen = strlen(ClientID);
    int UsernameLen = strlen(Username);
    int PasswordLen = strlen(Password);
    int DataLen;
	mqtt.txlen=0;
	//可变报头+Payload  每个字段包含两个字节的长度标识
    //DataLen = 10 + (ClientIDLen+2) + (UsernameLen+2) + (PasswordLen+2);
    DataLen = 10 + (ClientIDLen+2);
	
	//固定报头
	//控制报文类型
    mqtt.txbuf[mqtt.txlen++] = 0x10;		//MQTT Message Type CONNECT
	//剩余长度(不包括固定头部)
	do
	{
		uint8_t encodedByte = DataLen % 128;
		DataLen = DataLen / 128;
		// if there are more data to encode, set the top bit of this byte
		if ( DataLen > 0 )
			encodedByte = encodedByte | 128;
		mqtt.txbuf[mqtt.txlen++] = encodedByte;
	}while ( DataLen > 0 );
    	
	//可变报头
	//协议名
    mqtt.txbuf[mqtt.txlen++] = 0;        		// Protocol Name Length MSB    
    mqtt.txbuf[mqtt.txlen++] = 4;        		// Protocol Name Length LSB    
    mqtt.txbuf[mqtt.txlen++] = 'M';        	// ASCII Code for M    
    mqtt.txbuf[mqtt.txlen++] = 'Q';        	// ASCII Code for Q    
    mqtt.txbuf[mqtt.txlen++] = 'T';        	// ASCII Code for T    
    mqtt.txbuf[mqtt.txlen++] = 'T';        	// ASCII Code for T    
	//协议级别
    mqtt.txbuf[mqtt.txlen++] = 4;        		// MQTT Protocol version = 4    
	//连接标志
    mqtt.txbuf[mqtt.txlen++] = 0x2;        	// conn flags 
    mqtt.txbuf[mqtt.txlen++] = 0;        		// Keep-alive Time Length MSB    
    mqtt.txbuf[mqtt.txlen++] = 60;        	// Keep-alive Time Length LSB  60S心跳包  
	
    mqtt.txbuf[mqtt.txlen++] = BYTE1(ClientIDLen);// Client ID length MSB    
    mqtt.txbuf[mqtt.txlen++] = BYTE0(ClientIDLen);// Client ID length LSB  	
	memcpy(&mqtt.txbuf[mqtt.txlen],ClientID,ClientIDLen);
    mqtt.txlen += ClientIDLen;
    
    if(UsernameLen > 0)
    {   
        mqtt.txbuf[mqtt.txlen++] = BYTE1(UsernameLen);		//username length MSB    
        mqtt.txbuf[mqtt.txlen++] = BYTE0(UsernameLen);    	//username length LSB    
		memcpy(&mqtt.txbuf[mqtt.txlen],Username,UsernameLen);
        mqtt.txlen += UsernameLen;
    }
    
    if(PasswordLen > 0)
    {    
        mqtt.txbuf[mqtt.txlen++] = BYTE1(PasswordLen);		//password length MSB    
        mqtt.txbuf[mqtt.txlen++] = BYTE0(PasswordLen);    	//password length LSB  
		memcpy(&mqtt.txbuf[mqtt.txlen],Password,PasswordLen);
        mqtt.txlen += PasswordLen; 
    }    
	
	uint8_t cnt=1;
	uint8_t wait;
	while(cnt--)
	{
		memset(mqtt.rxbuf,0,mqtt.rxlen);
		mqtt_sendbuf(mqtt.txbuf,mqtt.txlen);
        
        //等待3s时间
        mqtt_recvbuf(mqtt.rxbuf, sizeof(mqtt.rxbuf), 3000);
        if(mqtt.rxbuf[0]==parket_connetAck[0] && mqtt.rxbuf[1]==parket_connetAck[1]) //连接成功			   
        {
            return 1;//连接成功
        }
        HAL_Delay(100);
	}
	return 0;
}


//MQTT订阅/取消订阅数据打包函数
//topic       主题 
//qos         消息等级 
//whether     订阅/取消订阅请求包
static uint8_t esp8266_subscribe(char *topic,uint8_t qos,uint8_t whether)
{    
	mqtt.txlen=0;
    int topiclen = strlen(topic);
	
	int DataLen = 2 + (topiclen+2) + (whether?1:0);//可变报头的长度（2字节）加上有效载荷的长度
	//固定报头
	//控制报文类型
    if(whether) mqtt.txbuf[mqtt.txlen++] = 0x82; //消息类型和标志订阅
    else	mqtt.txbuf[mqtt.txlen++] = 0xA2;    //取消订阅

	//剩余长度
	do
	{
		uint8_t encodedByte = DataLen % 128;
		DataLen = DataLen / 128;
		// if there are more data to encode, set the top bit of this byte
		if ( DataLen > 0 )
			encodedByte = encodedByte | 128;
		mqtt.txbuf[mqtt.txlen++] = encodedByte;
	}while ( DataLen > 0 );	
	
	//可变报头
    mqtt.txbuf[mqtt.txlen++] = 0;				//消息标识符 MSB
    mqtt.txbuf[mqtt.txlen++] = 0x01;           //消息标识符 LSB
	//有效载荷
    mqtt.txbuf[mqtt.txlen++] = BYTE1(topiclen);//主题长度 MSB
    mqtt.txbuf[mqtt.txlen++] = BYTE0(topiclen);//主题长度 LSB   
	memcpy(&mqtt.txbuf[mqtt.txlen],topic,topiclen);
    mqtt.txlen += topiclen;
    
    if(whether)
    {
        mqtt.txbuf[mqtt.txlen++] = qos;//QoS级别
    }
	
	uint8_t cnt=1;
	uint8_t wait;
	while(cnt--)
	{
		memset(mqtt.rxbuf,0,mqtt.rxlen);
		mqtt_sendbuf(mqtt.txbuf,mqtt.txlen);
        
		//等待3s时间
        mqtt_recvbuf(mqtt.rxbuf, sizeof(mqtt.rxbuf), 3000);
        if(mqtt.rxbuf[0]==parket_connetAck[0] && mqtt.rxbuf[1]==parket_connetAck[1]) //连接成功			   
        {
            return 1;//连接成功
        }
        HAL_Delay(100);
	}
	if(cnt) return 1;	//订阅成功
	return 0;
}

//MQTT发布数据打包函数
//topic   主题 
//message 消息
//qos     消息等级 
static uint8_t esp8266_publish(char *topic, char *message, uint8_t qos)
{  
    int topicLength = strlen(topic);    
    int messageLength = strlen(message);     
    static uint16_t id=0;
	int DataLen;
	mqtt.txlen=0;
	//有效载荷的长度这样计算：用固定报头中的剩余长度字段的值减去可变报头的长度
	//QOS为0时没有标识符
	//数据长度             主题名   报文标识符   有效载荷
    if(qos)	DataLen = (2+topicLength) + 2 + messageLength;       
    else	DataLen = (2+topicLength) + messageLength;   

    //固定报头
	//控制报文类型
    mqtt.txbuf[mqtt.txlen++] = 0x30;    // MQTT Message Type PUBLISH  

	//剩余长度
	do
	{
		uint8_t encodedByte = DataLen % 128;
		DataLen = DataLen / 128;
		// if there are more data to encode, set the top bit of this byte
		if ( DataLen > 0 )
			encodedByte = encodedByte | 128;
		mqtt.txbuf[mqtt.txlen++] = encodedByte;
	}while ( DataLen > 0 );	
	
    mqtt.txbuf[mqtt.txlen++] = BYTE1(topicLength);//主题长度MSB
    mqtt.txbuf[mqtt.txlen++] = BYTE0(topicLength);//主题长度LSB 
	memcpy(&mqtt.txbuf[mqtt.txlen],topic,topicLength);//拷贝主题
    mqtt.txlen += topicLength;
        
	//报文标识符
    if(qos)
    {
        mqtt.txbuf[mqtt.txlen++] = BYTE1(id);
        mqtt.txbuf[mqtt.txlen++] = BYTE0(id);
        id++;
    }
	memcpy(&mqtt.txbuf[mqtt.txlen],message,messageLength);
    mqtt.txlen += messageLength;

	mqtt_sendbuf(mqtt.txbuf,mqtt.txlen);
    return mqtt.txlen;
}

static void esp8266_send_heart(void)
{
	mqtt_sendbuf((uint8_t *)parket_heart,sizeof(parket_heart));
}

static void esp8266_disconnect(void)
{
	mqtt_sendbuf((uint8_t *)parket_disconnet,sizeof(parket_disconnet));
}

static void esp8266_end(void)
{
	
}

static void mqtt_sendbuf(uint8_t *buf,uint16_t len)
{
	HAL_UART_Transmit(&huart2, buf, len, 0xFFFF);
}

static void mqtt_recvbuf(uint8_t *buf, uint16_t len, uint32_t uTimeOut)
{
    uint32_t uStart, uEnd;
    
    uStart = HAL_GetTick();
    for (int i = 0; i < len; i++) {
        HAL_UART_Receive(&huart2, (uint8_t *)buf + i, 1, uTimeOut);
        uEnd = HAL_GetTick();
        if ((uEnd - uStart) > uTimeOut) {
            break;
        }
    }
}

void mqtt_test()
{
    net.begin(115200);
    //net.connect_ap("AIBITPC","12348765");
    net.connect_ap("ChinaNet-TVLK","qymgv4uk");
    //net.connect_server("TCP", "139.196.48.194", 8000);
    net.connect_server("TCP", "192.168.1.6", 1883);
    
    //mqtt.begin();
    //mqtt.connect("flask_mqtt", "", "");
    mqtt.publish_data("hello", "100", 0);
    while(1);
}

/*********************************************END OF FILE********************************************/
