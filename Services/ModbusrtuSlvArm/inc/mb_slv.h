#ifndef MB_SLV_H
#define MB_SLV_H

#include <stdint.h>

int  mb_slave_init(uint8_t addr, uint32_t baud);
void mb_slave_poll(void);

#endif
