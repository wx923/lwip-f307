#include "mb_slv_data.h"
#include "mb_slv_config.h"

//定义寄存器数组
static uint16_t hold_regs[HOLD_REG_COUNT];

//定义写回调函数
static mb_data_write_cb_t write_cb = 0;

/*
* 初始化寄存器
* @return: 0表示成功，-1表示失败
* @author: wuxiao
* @date: 2026-03-28
*/
int mb_data_init(void)
{
    for (uint16_t i = 0; i < HOLD_REG_COUNT; i++) {
        hold_regs[i] = 0;
    }
    return 0;
}

/*
* 读取寄存器值
* @param start_addr: 起始地址
* @param count: 寄存器数量
* @param out_buf: 输出缓冲区
* @return: 0表示成功，-1表示失败
* @author: wuxiao
* @date: 2026-03-28
*/
int mb_data_read(uint16_t start_addr, uint16_t count, uint16_t *out_buf)
{
    if (start_addr + count > HOLD_REG_COUNT) {
        return -1;
    }
    for (uint16_t i = 0; i < count; i++) {
        out_buf[i] = hold_regs[start_addr + i];
    }
    return 0;
}

/*
* 写入寄存器值
* @param start_addr: 起始地址
* @param count: 寄存器数量
* @param data: 数据缓冲区
* @return: 0表示成功，-1表示失败
* @author: wuxiao
* @date: 2026-03-28
*/
int mb_data_write(uint16_t start_addr, uint16_t count, const uint8_t *data)
{
    if (start_addr + count > HOLD_REG_COUNT) {
        return -1;
    }
    for (uint16_t i = 0; i < count; i++) {
        hold_regs[start_addr + i] = ((uint16_t)data[i * 2] << 8) | data[i * 2 + 1];
    }
    if (write_cb) {
        write_cb(start_addr, count);
    }
    return 0;
}

/*
* 设置写回调函数
* @param cb: 写回调函数
* @author: wuxiao
* @date: 2026-03-28
*/
void mb_data_set_write_callback(mb_data_write_cb_t cb)
{
    write_cb = cb;
}
