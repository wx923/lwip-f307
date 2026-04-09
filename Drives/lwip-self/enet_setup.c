/*!
    \file    enet_setup.c
    \brief   ethernet hardware configuration

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

#include "gd32f30x_enet.h"
#include "enet_setup.h"
#include "netconf.h"
#include "ethernetif.h"

static __IO uint32_t enet_init_status = 0;



/*!
    \brief      setup ethernet system(GPIOs, clocks, MAC, DMA, systick)
    \param[in]  none
    \param[out] none
    \retval     none
    \notes      RMII signal mapping:
                | Signal Name  | MCU Pin | GPIO Mode          | Description                                        |
                |---------------|---------|--------------------|----------------------------------------------------|
                | MDC           | PC1     | AF_PP / 50MHz      | Management Data Clock: clock from CPU to PHY       |
                | REF_CLK       | PA1     | IN_FLOATING        | Reference Clock: 50MHz sync clock for RMII        |
                | MDIO          | PA2     | AF_PP / 50MHz      | Management Data I/O: bidirectional data line       |
                | CRS_DV        | PA7     | IN_FLOATING        | Carrier Sense / Data Valid: valid RX data signal   |
                | RMII_RXD0     | PC4     | IN_FLOATING        | Receive Data bit 0                                 |
                | RMII_RXD1     | PC5     | IN_FLOATING        | Receive Data bit 1                                 |
                | PPS_OUT       | PB5     | AF_PP / 50MHz      | Pulse Per Second: IEEE 1588 PTP sync output        |
                | RMII_TX_EN    | PB11    | AF_PP / 50MHz      | Transmit Enable: start of TX frame                 |
                | RMII_TXD0     | PB12    | AF_PP / 50MHz      | Transmit Data bit 0                                |
                | RMII_TXD1     | PB13    | AF_PP / 50MHz      | Transmit Data bit 1                                |
                | CKOUT (PLL2)  | PA8     | AF_PP / MAX        | PHY reference clock output from PLL2 (50MHz)      |
*/
void enet_system_setup(void)
{
    ErrStatus reval_state = ERROR;

   //配置中断
    nvic_irq_enable(ENET_IRQn, 0, 0);

    //配置时钟
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_AF);

    /* PA1 receives external 50MHz RMII reference clock, no PLL2 needed */

    //PHY选择RMII模式
    gpio_ethernet_phy_select(GPIO_ENET_PHY_RMII);

    
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_1);
    gpio_init(GPIOA, GPIO_MODE_AF_PP,       GPIO_OSPEED_50MHZ, GPIO_PIN_2);
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_7);
    gpio_init(GPIOC, GPIO_MODE_AF_PP,       GPIO_OSPEED_50MHZ, GPIO_PIN_1);
    gpio_init(GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_4);
    gpio_init(GPIOC, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_5);
    gpio_init(GPIOB, GPIO_MODE_AF_PP,       GPIO_OSPEED_50MHZ, GPIO_PIN_5);
    gpio_init(GPIOB, GPIO_MODE_AF_PP,       GPIO_OSPEED_50MHZ, GPIO_PIN_11);
    gpio_init(GPIOB, GPIO_MODE_AF_PP,       GPIO_OSPEED_50MHZ, GPIO_PIN_12);
    gpio_init(GPIOB, GPIO_MODE_AF_PP,       GPIO_OSPEED_50MHZ, GPIO_PIN_13);

    //使能ETH时钟
    rcu_periph_clock_enable(RCU_ENET);
    rcu_periph_clock_enable(RCU_ENETTX);
    rcu_periph_clock_enable(RCU_ENETRX);

    //重置ETH
    enet_deinit();
    //重置ETH DMA
    reval_state = enet_software_reset();
    if (reval_state == ERROR) {
        while (1);
    }

    //初始化ETH
    enet_init_status = enet_init(ENET_AUTO_NEGOTIATION,
                                 ENET_AUTOCHECKSUM_DROP_FAILFRAMES,
                                 ENET_BROADCAST_FRAMES_PASS);

    //检查ETH初始化结果
    if (enet_init_status == 0) {
        while (1);
    }

    //使能DMA中断
    enet_interrupt_enable(ENET_DMA_INT_NIE);
    enet_interrupt_enable(ENET_DMA_INT_RIE);

    //使能增强描述符模式
#if SELECT_DESCRIPTORS_ENHANCED_MODE
    enet_desc_select_enhanced_mode();
#endif

    //
    /* SysTick is configured by FreeRTOS */
}
