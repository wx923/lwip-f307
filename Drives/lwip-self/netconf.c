/*!
    \file    netconf.c
    \brief   network connection configuration

    \version 2026-2-6, V3.0.3, firmware for GD32F30x
*/

/*
    Copyright (c) 2025, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/tcpip.h"
#include "netif/etharp.h"
#include "ethernetif.h"
#include "netconf.h"
#include "gd32f30x_enet.h"
#include <stdio.h>

/* 全局网络接口 */
struct netif g_mynetif;

/* static 函数，供 tcpip_init_done_callback 内部调用 */
static void netif_setup_callback(void *arg);


static void tcpip_init_done_callback(void *arg)
{
    (void)arg;
    tcpip_callback(netif_setup_callback, NULL);
}

static void netif_setup_callback(void *arg)
{
    ip_addr_t ipaddr, netmask, gw;

    (void)arg;

    IP4_ADDR(&ipaddr,  192, 168, 1, 100);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw,      192, 168, 1, 1);

    netif_add(&g_mynetif, &ipaddr, &netmask, &gw,
              NULL, &ethernetif_init, &tcpip_input);

    netif_set_default(&g_mynetif);
    netif_set_link_up(&g_mynetif);
    netif_set_up(&g_mynetif);
}

/*!
    \brief  初始化 LwIP 协议栈（tcpip_thread 模式）
    \note   此函数在 main 线程中调用，完成：
             1. 内存堆/内存池初始化
             2. 创建 tcpip_thread
             3. 在 tcpip 线程中添加 netif
            所有后续 LwIP 网络操作均在线程安全的 tcpip_thread 中执行
*/
void lwip_stack_init(void)
{
    /* 初始化内存堆和内存池——必须在 tcpip_init 之前 */
    mem_init();
    memp_init();

    /* 启动 tcpip 线程
     * tcpip_init 内部会创建 FreeRTOS 任务（使用静态内存），
     * 完成后自动调用 tcpip_init_done_callback */
    tcpip_init(tcpip_init_done_callback, NULL);
}


/* gd32f30x_enet.c 中的全局 DMA 描述符指针（未在头文件中 extern 声明） */
extern enet_descriptors_struct *dma_current_rxdesc;

/*!
    \brief  ENET 中断服务程序
    \note   处理 DMA 接收完成中断，将收到的帧送入 LwIP tcpip_thread。
            此函数由硬件自动调用，必须尽可能精简以减少中断延迟。
            LwIP 的 input 函数会通过 FreeRTOS 消息队列将数据安全传递到
            tcpip_thread 上下文中。
*/
void ENET_IRQHandler(void)
{
    /* 检查接收状态中断标志：DMA 接收到一帧数据后置位 */
    if (SET == enet_interrupt_flag_get(ENET_DMA_INT_FLAG_RS)) {
        /* 循环读取所有已接收的帧（描述符 DAV=0 表示有数据） */
        while ((dma_current_rxdesc->status & ENET_RDES0_DAV) == 0) {
            /* 读取 DMA 缓冲区的帧，送入 LwIP 协议栈。
             * ethernetif_input 内部会：
             *   1. 分配 pbuf
             *   2. 复制 DMA 缓冲区数据到 pbuf
             *   3. 调用 tcpip_input() 将 pbuf 发往 tcpip_thread
             *   4. 归还描述符给 DMA
             * 注意：ethernetif_input 中的 pbuf_free 逻辑足以处理 mbox 满的情况
             */
            ethernetif_input(&g_mynetif);
        }
        /* 清除接收状态中断标志 */
        enet_interrupt_flag_clear(ENET_DMA_INT_FLAG_RS_CLR);
    }

    /* 处理发送状态中断（可选：用于统计或 TX 完成通知） */
    if (SET == enet_interrupt_flag_get(ENET_DMA_INT_FLAG_TS)) {
        enet_interrupt_flag_clear(ENET_DMA_INT_FLAG_TS_CLR);
    }
}