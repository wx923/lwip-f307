#ifndef MB_SLV_PORT_GD32_H
#define MB_SLV_PORT_GD32_H

#include <stdint.h>

int  mb_slv_port_init(uint32_t baud);
void mb_slv_port_rx_start(void);
void mb_slv_port_tx_send(const uint8_t *buf, uint16_t len);
const uint8_t *mb_slv_port_get_frame_buf(void);
uint16_t mb_slv_port_get_frame_len(void);
void mb_slv_port_direction_set(uint8_t tx);

#endif
