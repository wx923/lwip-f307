/*!
    \file    bsp_init.c
    \brief   BSP 层初始化入口
*/

#include "bsp_uart.h"

void bsp_init(void)
{
    bsp_uart1_init();
    bsp_uart2_init();
}
