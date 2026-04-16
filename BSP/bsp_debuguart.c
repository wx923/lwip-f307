/*!
    \file    bsp_debuguart.c
    \brief   Debug UART (USART0) BSP: 轮询发送，用于 printf 重定向

    \version 2026-4-9, V1.0
*/

/* Includes */
#include "bsp_debuguart.h"
#include "gd32f30x.h"
#include <stdio.h>

/* -------------------- Private defines -------------------- */

/* -------------------- Public functions -------------------- */

/*!
    \brief      初始化 Debug UART（轮询发送模式）
    \note       只用于 printf 重定向。TX 仅发送，RX 未启用。
    \param[in]  none
    \param[out] none
    \retval     none
*/
void bsp_debuguart_init(void)
{
    /* 1. 使能 GPIO 和 USART 时钟 */
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_AF);
    rcu_periph_clock_enable(RCU_USART0);

    /* 2. 配置 TX 引脚 (PA9) 为复用推挽，50MHz */
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);

    /* 3. 配置 USART 参数 */
    usart_baudrate_set(DEBUG_USART, 115200);
    usart_word_length_set(DEBUG_USART, USART_WL_8BIT);
    usart_stop_bit_set(DEBUG_USART, USART_STB_1BIT);
    usart_parity_config(DEBUG_USART, USART_PM_NONE);

    /* 发送使能，接收不使能（调试只发不收） */
    usart_transmit_config(DEBUG_USART, USART_TRANSMIT_ENABLE);
    usart_receive_config(DEBUG_USART, USART_RECEIVE_DISABLE);

    /* 4. 使能 USART */
    usart_enable(DEBUG_USART);
}

/*!
    \brief      通过轮询方式发送一个字节
    \param[in]  ch: 要发送的字符
    \param[out] none
    \retval     none
*/
void debug_uart_putc(uint8_t ch)
{
    /* 等待发送缓冲区为空 */
    while (usart_flag_get(DEBUG_USART, USART_FLAG_TBE) == RESET);
    usart_data_transmit(DEBUG_USART, ch);

    /* 等待最后一个字符完全发送完成（TC 标志） */
    while (usart_flag_get(DEBUG_USART, USART_FLAG_TC) == RESET);
}

/* -------------------- 重定向 printf -------------------- */

/*!
    \brief      覆盖 fputc，将 stdout 重定向到 USART0
    \note       Keil MDK 使用 newlib/newlib-nano 时，printf 底层调用此函数
    \param[in]  ch: 要输出的字符
    \param[in]  f:  文件流（未使用，固定为 stdout）
    \retval     输出的字符
*/
int fputc(int ch, FILE *f)
{
    (void)f;
    debug_uart_putc((uint8_t)ch);
    return ch;
}