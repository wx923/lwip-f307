#ifndef __MODBUS_RTU_H
#define __MODBUS_RTU_H

#include <stdint.h>

#define MB_FC_READ_HOLDING  0x03
#define MB_FC_WRITE_SINGLE  0x06
#define MB_FC_WRITE_MULTI   0x10

#define MB_EXC_ILLEGAL_FUNC  0x01
#define MB_EXC_ILLEGAL_ADDR  0x02
#define MB_EXC_ILLEGAL_VAL   0x03
#define MB_EXC_SLAVE_FAIL    0x04

uint16_t mb_crc16(const uint8_t *data, uint16_t len);
int      mb_check_crc(const uint8_t *frame, uint16_t len);

int mb_build_rsp_read(uint8_t *tx, uint16_t max_len,
                       uint8_t id, const uint16_t *regs, uint16_t count);

int mb_build_rsp_write_single(uint8_t *tx, uint16_t max_len,
                               uint8_t id, uint16_t addr, uint16_t val);

int mb_build_rsp_write_multi(uint8_t *tx, uint16_t max_len,
                              uint8_t id, uint16_t addr, uint16_t count);

int mb_build_rsp_exc(uint8_t *tx, uint16_t max_len,
                      uint8_t id, uint8_t exc_code);

int mb_build_req_read(uint8_t *tx, uint16_t max_len,
                       uint8_t id, uint16_t addr, uint16_t count);

int mb_build_req_write_single(uint8_t *tx, uint16_t max_len,
                                uint8_t id, uint16_t addr, uint16_t val);

int mb_build_req_write_multi(uint8_t *tx, uint16_t max_len,
                              uint8_t id, uint16_t addr,
                              const uint16_t *vals, uint16_t count);

int mb_parse_rsp_read(const uint8_t *frame, uint16_t len,
                       uint16_t *out_count, uint16_t *out_vals);

int mb_parse_rsp_ok(const uint8_t *frame, uint16_t len);

int mb_parse_rsp_write_single(const uint8_t *frame, uint16_t len,
                               uint16_t *out_addr, uint16_t *out_val);

int mb_parse_rsp_write_multi(const uint8_t *frame, uint16_t len,
                              uint16_t *out_addr, uint16_t *out_count);

#endif
