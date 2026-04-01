#include "mb_slv.h"
#include "mb_slv_data.h"
#include "mb_slv_frame.h"
#include "mb_slv_func.h"
#include "mb_slv_port_gd32.h"
#include "mb_slv_config.h"

#include "FreeRTOS.h"
#include "semphr.h"

#define TX_BUF_SIZE  256

SemaphoreHandle_t xSemaphore;          // 供 ISR extern
static StaticSemaphore_t s_sem_buf;

static uint8_t s_slave_addr;

static uint8_t s_tx_buf[TX_BUF_SIZE];

/*=============================================*/
/*  Public API                                 */
/*=============================================*/

/*
* 初始化从机
* @param addr: 从机地址
* @param baud: 波特率
* @return: 0表示成功，-1表示失败
* @author: wuxiao
* @date: 2026-03-28
*/
int mb_slave_init(uint8_t addr, uint32_t baud)
{
    // 设置从机地址
    s_slave_addr = addr;

    // 初始化数据
    mb_data_init();

    xSemaphore = xSemaphoreCreateBinaryStatic(&s_sem_buf);

    if (mb_slv_port_init(baud) != 0) {
        return -1;
    }

    mb_slv_port_rx_start();

    return 0;
}

/*
* 轮询从机
* @author: wuxiao
* @date: 2026-03-28
*/
void mb_slave_poll(void)
{
    uint16_t len;
    const uint8_t *buf;
    mb_slv_frame_t frame;
    uint16_t tx_len = 0;

    // 阻塞等待信号量，直到 ISR 里释放它
    if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdPASS) {
        // 获取帧缓冲区
        buf = mb_slv_port_get_frame_buf();
        // 获取帧长度
        len = mb_slv_port_get_frame_len();

        if (buf[0] != s_slave_addr) {
            mb_slv_port_rx_start();
            return;
        }

        // 解析请求
        if (mb_slv_parse_request(buf, len, &frame) != 0) {
            mb_slv_port_rx_start();
            return;
        }

        // 分发请求
        mb_func_dispatch(&frame, s_tx_buf, &tx_len);

        mb_slv_port_tx_send(s_tx_buf, tx_len);
        // 接收重启由 TC ISR 统一处理（mb_slv_port_gd32.c）
    }
}
