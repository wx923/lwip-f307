#include "gd32f30x.h"
#include "gd32f30x_libopt.h"

#include "mb_slv_port_gd32.h"
#include "mb_slv_config.h"
#include "FreeRTOS.h"
#include "semphr.h"

extern SemaphoreHandle_t xSemaphore;

/*=============================================*/
/*  Hardware config  (change here for porting) */
/*=============================================*/

#define MB_SLV_UART              USART0
#define MB_SLV_UART_IRQ          USART0_IRQn
#define MB_SLV_DMA               DMA0
#define MB_SLV_DMA_RX_CH         DMA_CH4          // USART0_RX → DMA0_CH4
#define MB_SLV_DMA_TX_CH         DMA_CH3          // USART0_TX → DMA0_CH3

#define MB_SLV_TX_PORT           GPIOA
#define MB_SLV_TX_PIN            GPIO_PIN_9
#define MB_SLV_RX_PORT           GPIOA
#define MB_SLV_RX_PIN            GPIO_PIN_10

#define MB_SLV_DE_PORT           GPIOA
#define MB_SLV_DE_PIN            GPIO_PIN_8

#define RX_BUF_SIZE              256

/*=============================================*/
/*  Private data                               */
/*=============================================*/

static uint8_t  rx_buf[RX_BUF_SIZE];
static volatile uint16_t frame_len = 0;

/*=============================================*/
/*  Public API                                 */
/*=============================================*/

/*
* 初始化
* @param baud: 波特率
* @return: 0表示成功，-1表示失败
* @author: wuxiao
* @date: 2026-03-28
*/
int mb_slv_port_init(uint32_t baud)
{
    dma_parameter_struct dma_init_struct;

    /* ---------- ① Clock ---------- */
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_AF);
    rcu_periph_clock_enable(RCU_USART0);
    rcu_periph_clock_enable(RCU_DMA0);

    /* ---------- ② GPIO ---------- */
    gpio_init(MB_SLV_TX_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, MB_SLV_TX_PIN);
    gpio_init(MB_SLV_RX_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, MB_SLV_RX_PIN);
    gpio_init(MB_SLV_DE_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, MB_SLV_DE_PIN);
    gpio_bit_reset(MB_SLV_DE_PORT, MB_SLV_DE_PIN);

    /* ---------- ③ UART ---------- */
    usart_baudrate_set(MB_SLV_UART, baud);
    usart_word_length_set(MB_SLV_UART, USART_WL_8BIT);
    usart_stop_bit_set(MB_SLV_UART, USART_STB_1BIT);
    usart_parity_config(MB_SLV_UART, USART_PM_NONE);
    usart_receive_config(MB_SLV_UART, USART_RECEIVE_ENABLE);
    usart_transmit_config(MB_SLV_UART, USART_TRANSMIT_ENABLE);
    usart_dma_receive_config(MB_SLV_UART, USART_RECEIVE_DMA_ENABLE);
    usart_dma_transmit_config(MB_SLV_UART, USART_TRANSMIT_DMA_ENABLE);
    usart_interrupt_enable(MB_SLV_UART, USART_INT_IDLE);
    usart_interrupt_enable(MB_SLV_UART, USART_INT_TC);
    usart_enable(MB_SLV_UART);

    /* ---------- ④ DMA_RX (circular, always running) ---------- */
    dma_deinit(MB_SLV_DMA, MB_SLV_DMA_RX_CH);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.periph_addr   = (uint32_t)&USART_DATA(MB_SLV_UART);
    dma_init_struct.periph_width  = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.memory_addr   = (uint32_t)rx_buf;
    dma_init_struct.memory_width  = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number        = RX_BUF_SIZE;
    dma_init_struct.priority      = DMA_PRIORITY_HIGH;
    dma_init_struct.periph_inc   = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_inc    = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.direction     = DMA_PERIPHERAL_TO_MEMORY;
    dma_init(MB_SLV_DMA, MB_SLV_DMA_RX_CH, &dma_init_struct);
    dma_circulation_enable(MB_SLV_DMA, MB_SLV_DMA_RX_CH);
    dma_channel_enable(MB_SLV_DMA, MB_SLV_DMA_RX_CH);

    /* ---------- ⑤ DMA_TX (normal, triggered per-frame) ---------- */
    dma_deinit(MB_SLV_DMA, MB_SLV_DMA_TX_CH);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.periph_addr   = (uint32_t)&USART_DATA(MB_SLV_UART);
    dma_init_struct.periph_width  = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.memory_addr   = 0;
    dma_init_struct.memory_width  = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number        = 0;
    dma_init_struct.priority      = DMA_PRIORITY_HIGH;
    dma_init_struct.periph_inc    = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.memory_inc    = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.direction     = DMA_MEMORY_TO_PERIPHERAL;
    dma_init(MB_SLV_DMA, MB_SLV_DMA_TX_CH, &dma_init_struct);
    dma_circulation_disable(MB_SLV_DMA, MB_SLV_DMA_TX_CH);

    /* ---------- ⑥ NVIC ----------
     * 优先级必须 > configMAX_SYSCALL_INTERRUPT_PRIORITY(5)，
     * 才能安全调用 xSemaphoreGiveFromISR / portYIELD_FROM_ISR */
    nvic_irq_enable(MB_SLV_UART_IRQ, (configMAX_SYSCALL_INTERRUPT_PRIORITY + 1), 0);

    return 0;
}

/*
* 启动接收（重新加载DMA计数器，进入循环模式）
* @author: wuxiao
* @date: 2026-03-28
*/
void mb_slv_port_rx_start(void)
{
    dma_channel_disable(MB_SLV_DMA, MB_SLV_DMA_RX_CH);
    dma_transfer_number_config(MB_SLV_DMA, MB_SLV_DMA_RX_CH, RX_BUF_SIZE);
    dma_circulation_enable(MB_SLV_DMA, MB_SLV_DMA_RX_CH);
    dma_channel_enable(MB_SLV_DMA, MB_SLV_DMA_RX_CH);
}

/*
* 发送数据
* @param buf: 数据缓冲区
* @param len: 数据长度
* @author: wuxiao
* @date: 2026-03-28
*/
void mb_slv_port_tx_send(const uint8_t *buf, uint16_t len)
{
    dma_channel_disable(MB_SLV_DMA, MB_SLV_DMA_TX_CH);
    dma_memory_address_config(MB_SLV_DMA, MB_SLV_DMA_TX_CH, (uint32_t)buf);
    dma_transfer_number_config(MB_SLV_DMA, MB_SLV_DMA_TX_CH, len);
    gpio_bit_set(MB_SLV_DE_PORT, MB_SLV_DE_PIN);
    dma_channel_enable(MB_SLV_DMA, MB_SLV_DMA_TX_CH);
}

/*
* 获取帧缓冲区
* @return: 帧缓冲区
* @author: wuxiao
* @date: 2026-03-28
*/
const uint8_t *mb_slv_port_get_frame_buf(void)
{
    return rx_buf;
}

/*
* 获取帧长度
* @return: 帧长度
* @author: wuxiao
* @date: 2026-03-28
*/
uint16_t mb_slv_port_get_frame_len(void)
{
    return frame_len;
}

void mb_slv_port_direction_set(uint8_t tx)
{
    if (tx) {
        gpio_bit_set(MB_SLV_DE_PORT, MB_SLV_DE_PIN);
    } else {
        gpio_bit_reset(MB_SLV_DE_PORT, MB_SLV_DE_PIN);
    }
}

/*=============================================*/
/*  ISRs                                        */
/*=============================================*/

void USART0_IRQHandler(void)
{
    /* ---------- ① IDLE 帧检测 ---------- */
    if (usart_interrupt_flag_get(MB_SLV_UART, USART_INT_FLAG_IDLE) != RESET) {
        (void)USART_STAT0(MB_SLV_UART);
        (void)USART_DATA(MB_SLV_UART);

        uint16_t cnt = dma_transfer_number_get(MB_SLV_DMA, MB_SLV_DMA_RX_CH);
        frame_len = RX_BUF_SIZE - cnt;

        if (frame_len > 0 && frame_len < RX_BUF_SIZE) {
            BaseType_t hpw = pdFALSE;
            xSemaphoreGiveFromISR(xSemaphore, &hpw);
            if (hpw == pdTRUE) {
                portYIELD_FROM_ISR(hpw);
            }
        }

        dma_channel_disable(MB_SLV_DMA, MB_SLV_DMA_RX_CH);
    }

    /* ---------- ② TC (TX complete) ---------- */
    if (usart_interrupt_flag_get(MB_SLV_UART, USART_INT_FLAG_TC) != RESET) {
        usart_interrupt_flag_clear(MB_SLV_UART, USART_INT_FLAG_TC);

        dma_channel_disable(MB_SLV_DMA, MB_SLV_DMA_TX_CH);
        gpio_bit_reset(MB_SLV_DE_PORT, MB_SLV_DE_PIN);

        mb_slv_port_rx_start();
    }
}
