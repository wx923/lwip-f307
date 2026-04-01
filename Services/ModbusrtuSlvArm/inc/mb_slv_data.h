#ifndef MB_SLV_DATA_H
#define MB_SLV_DATA_H

#include <stdint.h>

int mb_data_init(void);

int mb_data_read(uint16_t start_addr, uint16_t count, uint16_t *out_buf);
int mb_data_write(uint16_t start_addr, uint16_t count, const uint8_t *data);

typedef void (*mb_data_write_cb_t)(uint16_t addr, uint16_t count);
void mb_data_set_write_callback(mb_data_write_cb_t cb);

#endif
