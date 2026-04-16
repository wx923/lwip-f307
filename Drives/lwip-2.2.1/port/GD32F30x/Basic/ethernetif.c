/**
 * @file
 * Ethernet Interface for standalone applications (without RTOS)
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

//启动硬件校验和
#define CHECKSUM_BY_HARDWARE

#include "lwip/mem.h"
#include "netif/etharp.h"
#include "ethernetif.h"
#include "gd32f30x_enet.h"
#include <string.h>


/* network interface name */
#define IFNAME0 'G'
#define IFNAME1 'D'

/*
来自GD32F30x_standard_peripheral/Include/gd32f30x_enet.h文件
    ENET_RXBUF_NUM: 接收描述符数量
    ENET_TXBUF_NUM: 发送描述符数量
    ENET_RXBUF_SIZE: 接收缓冲区大小
    ENET_TXBUF_SIZE: 发送缓冲区大小
    ENET_MAX_FRAME_SIZE: 最大帧大小
    ENET_DMA_TX: 发送DMA
    ENET_DMA_RX: 接收DMA
    ENET_TDES0_TCHM: 发送描述符链式模式
    ENET_RDES0_DAV: 接收描述符立即模式
    ENET_TDES0_TTSEN: 发送描述符时间戳模式
    ENET_RDES0_DAV: 接收描述符立即模式
    ENET_TDES0_TTSEN: 发送描述符时间戳模式
*/
/*描述符列表*/
extern enet_descriptors_struct  rxdesc_tab[ENET_RXBUF_NUM], txdesc_tab[ENET_TXBUF_NUM];

/* 接收缓冲区 */
extern uint8_t rx_buff[ENET_RXBUF_NUM][ENET_RXBUF_SIZE]; 

/* 发送缓冲区 */
extern uint8_t tx_buff[ENET_TXBUF_NUM][ENET_TXBUF_SIZE]; 

/* 全局发送和接收描述符指针 */
extern enet_descriptors_struct  *dma_current_txdesc;
extern enet_descriptors_struct  *dma_current_rxdesc;


void low_level_init(struct netif *netif)
{
   //设置MAC硬件地址长度
    netif->hwaddr_len = 6;

    //设置MAC硬件地址
    netif->hwaddr[0] = 0x02;
    netif->hwaddr[1] = 0x00;
    netif->hwaddr[2] = 0x00;
    netif->hwaddr[3] = 0x00;
    netif->hwaddr[4] = 0x00;
    netif->hwaddr[5] = 0x00;
    
    //设置MAC地址
    enet_mac_address_set(ENET_MAC_ADDRESS0, netif->hwaddr);

    //设置最大传输单元
    netif->mtu = 1500;

    //设置设备能力
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

    //初始化描述符列表：链式/环式模式
    enet_descriptors_chain_init(ENET_DMA_TX);
    enet_descriptors_chain_init(ENET_DMA_RX);

    //开启接收描述符立即模式中断
    for(int i=0; i<ENET_RXBUF_NUM; i++){ 
        enet_rx_desc_immediate_receive_complete_interrupt(&rxdesc_tab[i]);
    }   

    //设置发送描述符校验和  
    for(int i=0; i < ENET_TXBUF_NUM; i++){
        enet_transmit_checksum_config(&txdesc_tab[i], ENET_CHECKSUM_TCPUDPICMP_FULL);
    }

    //使能MAC和DMA传输和接收
    enet_enable();
}



static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    struct pbuf *q;
    int framelength = 0;
    uint8_t *buffer;

    //等待发送描述符空闲
    while((uint32_t)RESET != (dma_current_txdesc->status & ENET_TDES0_DAV)){}  
    //获取发送缓冲区地址
    buffer = (uint8_t *)(enet_desc_information_get(dma_current_txdesc, TXDESC_BUFFER_1_ADDR));
    
    //复制数据到发送缓冲区
    for(q = p; q != NULL; q = q->next){ 
        memcpy((uint8_t *)&buffer[framelength], q->payload, q->len);
        framelength = framelength + q->len;
    }

    //使用普通模式发送数据
    enet_frame_transmit(buffer, framelength);
  

    return ERR_OK;
}

static struct pbuf * low_level_input(struct netif *netif)
{
    struct pbuf *p, *q;
    u16_t len;
    int l =0;
    uint8_t *buffer;
     
    p = NULL;
    
    //获取接收描述符帧长度
    len = enet_desc_information_get(dma_current_rxdesc, RXDESC_FRAME_LENGTH);
    //获取接收缓冲区地址
    buffer = (uint8_t *)(enet_desc_information_get(dma_current_rxdesc, RXDESC_BUFFER_1_ADDR));
    
    //分配pbuf链
    p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    
    /* copy received frame to pbuf chain */
    if (p != NULL){
        for (q = p; q != NULL; q = q->next){ 
            memcpy((uint8_t *)q->payload, (u8_t*)&buffer[l], q->len);
            l = l + q->len;
        }    
    }
  
    //将当前接收描述符状态设置为空闲并准备接收下一帧
    enet_rxframe_drop();
  
    //返回pbuf链
    return p;
}


err_t ethernetif_input(struct netif *netif)
{
    err_t err;
    struct pbuf *p;

    //接收链表数据到pbuf链
    p = low_level_input(netif);

    //如果pbuf链为空，返回错误
    if (p == NULL) return ERR_MEM;

    //调用Lwip的以太网分发函数
    err = netif->input(p, netif);
    
    //如果分发函数返回错误，释放pbuf链
    if (err != ERR_OK){
        LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
        pbuf_free(p);
        p = NULL;
    }
    return err;
}


err_t ethernetif_init(struct netif *netif)
{
    LWIP_ASSERT("netif != NULL", (netif != NULL));

    //设置接口名称
    netif->name[0] = IFNAME0;
    netif->name[1] = IFNAME1;

    //output函数用于发送ARP请求
    netif->output = etharp_output;
    //linkoutput函数用于发送数据
    netif->linkoutput = low_level_output;

    //初始化硬件
    low_level_init(netif);

    return ERR_OK;
}
