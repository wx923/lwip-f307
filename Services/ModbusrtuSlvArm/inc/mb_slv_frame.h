#ifndef MB_SLV_FRAME_H
#define MB_SLV_FRAME_H

#include <stdint.h>

typedef enum {
    MB_SLV_FC_NONE = 0,
    MB_SLV_FC03,
    MB_SLV_FC06,
    MB_SLV_FC10,
} mb_slv_func_code_t;

typedef struct {
    uint8_t  slave_addr;
    uint8_t  raw_func_code;
    mb_slv_func_code_t func_code;
    uint16_t start_addr;
    union {
        uint16_t reg_count;
        uint16_t reg_value;
    };
    uint8_t  byte_count;
    const uint8_t *data;
} mb_slv_frame_t;

int mb_slv_parse_request(const uint8_t *buf, uint16_t len, mb_slv_frame_t *frame);

uint16_t mb_slv_build_fc03_resp(const mb_slv_frame_t *frame, const uint16_t *regs, uint8_t *tx_buf);
uint16_t mb_slv_build_fc06_resp(const mb_slv_frame_t *frame, uint8_t *tx_buf);
uint16_t mb_slv_build_fc10_resp(const mb_slv_frame_t *frame, uint8_t *tx_buf);
uint16_t mb_slv_build_exception(const mb_slv_frame_t *frame, uint8_t ex_code, uint8_t *tx_buf);

#endif
