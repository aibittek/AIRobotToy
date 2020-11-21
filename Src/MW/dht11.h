#ifndef __DS18B20_H
#define __DS18B20_H

#include <system.h>

#define DHT11_IO_IN()  {GPIOC->MODER&=~(3<<(8*2));GPIOC->MODER|=0<<(8*2);}
#define DHT11_IO_OUT() {GPIOC->MODER&=~(3<<(8*2));GPIOC->MODER|=1<<(8*2);}
 								   
#define	DHT11_DQ_OUT    PCout(8)
#define	DHT11_DQ_IN     PCin(8)
   	
uint8_t DHT11_Init(void);
uint8_t DHT11_Read_Data(uint8_t *temp,uint8_t *humi);
uint8_t DHT11_Read_Byte(void);
uint8_t DHT11_Read_Bit(void);
uint8_t DHT11_Check(void);
void DHT11_Rst(void);
#endif
