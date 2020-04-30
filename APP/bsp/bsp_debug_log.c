/**
  ******************************************************************************
  * @file			bsp_debug_log.c
  * @brief			bsp_debug_log function
  * @author			Xli
  * @email			xieliyzh@163.com
  * @version		1.0.0
  * @date			2020-04-30
  * @copyright		2020, EVECCA Co.,Ltd. All rights reserved
  ******************************************************************************
**/

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "bsp_debug_log.h"
#include "stm32f4xx.h" 
#include "stm32f4xx_usart.h"


/* Private constants ---------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private typedef -----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static uint8_t uart_tx_buffer[LOG_BUFFER_MAX_SIZE];

/* Private function ----------------------------------------------------------*/

/**=============================================================================
 * @brief           串口初始化
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static void _uart_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	/* 1、使能GPIOB、USART1、DMA1时钟 */
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE); 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
 
	/* 2、串口1对应引脚复用映射 */
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_USART1); 
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_USART1);
	
	/* 3、USART1端口配置 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; 
	GPIO_Init(GPIOB, &GPIO_InitStructure);

   /* 4、USART1初始化设置 */
	USART_InitStructure.USART_BaudRate = 115200;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	
    USART_Init(USART1, &USART_InitStructure);
	
    /* 5、使能串口 */
    USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);
    USART_Cmd(USART1, ENABLE);  //使能串口1 
	
	//USART_ClearFlag(USART1, USART_FLAG_TC);
	
	/* 6、接收使能 */
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority= 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;		
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			
	NVIC_Init(&NVIC_InitStructure);
}

/**=============================================================================
 * @brief           DMA发送初始化
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static void _dma_tx_init(void)
{
	DMA_InitTypeDef DMA_InitStructure; 
	NVIC_InitTypeDef NVIC_InitStructure;	
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
	
	DMA_DeInit(DMA2_Stream7);
	
	while (DMA_GetCmdStatus(DMA2_Stream7) != DISABLE){};

    /* DMA发送 */
	DMA_InitStructure.DMA_Channel = DMA_Channel_4;  
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(USART1->DR);
	DMA_InitStructure.DMA_Memory0BaseAddr =  (uint32_t)uart_tx_buffer;
	DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;
	DMA_InitStructure.DMA_BufferSize = LOG_BUFFER_MAX_SIZE;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; 
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DMA2_Stream7, &DMA_InitStructure);
	
	DMA_ITConfig(DMA2_Stream7, DMA_IT_TC, ENABLE);
	
	USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);
	
	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream7_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
}

/**=============================================================================
 * @brief           DMA接收初始化
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
static void _dma_rx_init(uint8_t *rx_buf)
{
	DMA_InitTypeDef DMA_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
	
	DMA_DeInit(DMA2_Stream2);

	while (DMA_GetCmdStatus(DMA2_Stream2) != DISABLE){};	
	DMA_InitStructure.DMA_Channel = DMA_Channel_4; 						
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&(USART1->DR);		
	DMA_InitStructure.DMA_Memory0BaseAddr =  (uint32_t)rx_buf;	
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;				
	DMA_InitStructure.DMA_BufferSize = LOG_BUFFER_MAX_SIZE;					
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; 	
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;					
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte; 
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;			
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;							
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;					
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;   				
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;		
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;				
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;		
	DMA_Init(DMA2_Stream2, &DMA_InitStructure);
			
	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);							
	DMA_Cmd(DMA2_Stream2, ENABLE);											

	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

/**=============================================================================
 * @brief           Log 输出
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
int bsp_debug_log_send(uint8_t *pbuf, uint16_t size)
{

#if 1
    /* 参数校验 */
	if (!pbuf || (size > LOG_BUFFER_MAX_SIZE)) return -1;
    /* DMA非空闲 */
	if (DMA_GetFlagStatus(DMA2_Stream7, DMA_FLAG_TCIF7) != RESET) return -1;
    
    /* 拷贝发送数据 */
	memcpy(uart_tx_buffer, pbuf, size);
	DMA_Cmd(DMA2_Stream7, DISABLE);
	/* 设置传输数据大小 */
	DMA_SetCurrDataCounter(DMA2_Stream7, size);
	DMA_Cmd(DMA2_Stream7, ENABLE);
#else
	for(uint16_t i=0;i< size; i++)
	{
		while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
		USART1->DR = (u8) pbuf[i];  
	}
#endif
	return 0;
}

/**=============================================================================
 * @brief           串口1中断
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
// void USART1_IRQHandler(void)
// {
// 	if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)	/*!< 串口空闲中断 */
// 	{
// 		DMA_Cmd(DMA2_Stream2, DISABLE);
// 	}
// }

/**=============================================================================
 * @brief           DMA发送完成中断
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
void DMA2_Stream7_IRQHandler(void)
{
	if(DMA_GetFlagStatus(DMA2_Stream7,DMA_FLAG_TCIF7)!=RESET)	    
	{
		DMA_Cmd(DMA2_Stream7,DISABLE);
		DMA_ClearFlag(DMA2_Stream7,DMA_FLAG_TCIF7| 					
								   DMA_FLAG_FEIF7 | 				
								   DMA_FLAG_DMEIF7| 				
								   DMA_FLAG_TEIF7 | 				
								   DMA_FLAG_HTIF7);										
	}
}

/**=============================================================================
 * @brief           log init
 *
 * @param[in]       none
 *
 * @return          none
 *============================================================================*/
void bsp_debug_log_init(uint8_t *rx_buf)
{
    _uart_init();
    _dma_tx_init();
    //_dma_rx_init(rx_buf);
}
