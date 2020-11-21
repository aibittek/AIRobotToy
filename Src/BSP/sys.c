#include "sys.h"

void MY_NVIC_SetVectorTable(uint32_t NVIC_VectTab, uint32_t Offset)
{
	SCB->VTOR = NVIC_VectTab | (Offset & (uint32_t)0xFFFFFE00); 
}

void MY_NVIC_PriorityGroupConfig(uint8_t NVIC_Group)
{
	uint32_t temp, temp1;
	temp1 = (~NVIC_Group) & 0x07;
	temp1 <<= 8;
	temp = SCB->AIRCR;
	temp &= 0X0000F8FF; 
	temp |= 0X05FA0000; 
	temp |= temp1;
	SCB->AIRCR = temp;
}

void MY_NVIC_Init(uint8_t NVIC_PreemptionPriority, uint8_t NVIC_SubPriority, uint8_t NVIC_Channel, uint8_t NVIC_Group)
{
	uint32_t temp;
	MY_NVIC_PriorityGroupConfig(NVIC_Group);
	temp = NVIC_PreemptionPriority << (4 - NVIC_Group);
	temp |= NVIC_SubPriority & (0x0f >> NVIC_Group);
	temp &= 0xf;
	NVIC->ISER[NVIC_Channel / 32] |= 1 << NVIC_Channel % 32; 
	NVIC->IP[NVIC_Channel] |= temp << 4;
}

void Ex_NVIC_Config(uint8_t GPIOx, uint8_t BITx, uint8_t TRIM)
{
	uint8_t EXTOFFSET = (BITx % 4) * 4;
	RCC->APB2ENR |= 1 << 14;
	SYSCFG->EXTICR[BITx / 4] &= ~(0x000F << EXTOFFSET); 
	SYSCFG->EXTICR[BITx / 4] |= GPIOx << EXTOFFSET;

	EXTI->IMR |= 1 << BITx;
	if (TRIM & 0x01)
		EXTI->FTSR |= 1 << BITx;
	if (TRIM & 0x02)
		EXTI->RTSR |= 1 << BITx;
}

void GPIO_AF_Set(GPIO_TypeDef *GPIOx, uint8_t BITx, uint8_t AFx)
{
	GPIOx->AFR[BITx >> 3] &= ~(0X0F << ((BITx & 0X07) * 4));
	GPIOx->AFR[BITx >> 3] |= (uint32_t)AFx << ((BITx & 0X07) * 4);
}

void GPIO_Set(GPIO_TypeDef *GPIOx, uint32_t BITx, uint32_t MODE, uint32_t OTYPE, uint32_t OSPEED, uint32_t PUPD)
{
	uint32_t pinpos = 0, pos = 0, curpin = 0;
	for (pinpos = 0; pinpos < 16; pinpos++)
	{
		pos = 1 << pinpos;	
		curpin = BITx & pos;
		if (curpin == pos)
		{
			GPIOx->MODER &= ~(3 << (pinpos * 2));
			GPIOx->MODER |= MODE << (pinpos * 2);
			if ((MODE == 0X01) || (MODE == 0X02))
			{
				GPIOx->OSPEEDR &= ~(3 << (pinpos * 2));
				GPIOx->OSPEEDR |= (OSPEED << (pinpos * 2));
				GPIOx->OTYPER &= ~(1 << pinpos);
				GPIOx->OTYPER |= OTYPE << pinpos;
			}
			GPIOx->PUPDR &= ~(3 << (pinpos * 2));
			GPIOx->PUPDR |= PUPD << (pinpos * 2);
		}
	}
}

//THUMB指令不支持汇编内联
//采用如下方法实现执行汇编指令WFI
void WFI_SET(void)
{
	__ASM volatile("wfi");
}

//关闭所有中断
void INTX_DISABLE(void)
{
	__ASM volatile("cpsid i");
}
//开启所有中断
void INTX_ENABLE(void)
{
	__ASM volatile("cpsie i");
}
//设置栈顶地址
//addr:栈顶地址
void MSR_MSP(uint32_t addr)
{
	__ASM volatile("MSR MSP, r0"); //set Main Stack value
	__ASM volatile("BX r14");
}

struct __FILE
{
	int handle;
	/* Whatever you require here. If the only file you are using is */
	/* standard output using printf() for debugging, no file handling */
	/* is required. */
};

#include <stdio.h>
FILE __stdout;
int fputc(int ch, FILE *f)
{
	while ((UART4->SR & 0X40) == 0)
		;
	UART4->DR = (uint8_t)ch;
	return ch;
}
