/*!
    \file    bsp_debuguart.h
    \brief   Debug UART (USART0) BSP: 轮询发送，用于 printf 重定向
*/

#ifndef BSP_DEBUGUART_H
#define BSP_DEBUGUART_H

#include "gd32f30x.h"
#include <stdint.h>

/* 默认使用 USART0 (TX=PA9, RX=PA10), 波特率 115200 */
#define DEBUG_USART        USART0
#define DEBUG_USART_CLK    RCU_USART0
#define DEBUG_GPIO_PORT    GPIOA
#define DEBUG_TX_PIN       GPIO_PIN_9
#define DEBUG_RX_PIN       GPIO_PIN_10

/* 初始化 Debug UART（轮询模式，无 DMA，无中断） */
void bsp_debuguart_init(void);

#endif /* BSP_DEBUGUART_H */