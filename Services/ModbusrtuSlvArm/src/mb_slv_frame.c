#include "mb_slv_frame.h"
#include "mb_slv_crc.h"

static uint16_t frame_get_u16_be(const uint8_t *buf)
{
    return ((uint16_t)buf[0] << 8) | buf[1];
}

int mb_slv_parse_request(const uint8_t *buf, uint16_t len, mb_slv_frame_t *frame)
{
    if (len < 8) {
        return -1;
    }

    uint16_t crc_recv = ((uint16_t)buf[len - 1] << 8) | buf[len - 2];
    uint16_t crc_calc = mb_slv_crc16(buf, len - 2);
    if (crc_calc != crc_recv) {
        return -1;
    }

    frame->slave_addr = buf[0];
    uint8_t fc = buf[1];
    frame->raw_func_code = fc;

    switch (fc) {
    case 0x03: {
        if (len < 8) return -1;
        frame->start_addr = frame_get_u16_be(&buf[2]);
        frame->reg_count  = frame_get_u16_be(&buf[4]);
        if (frame->reg_count == 0 || frame->reg_count > 125) return -1;
        frame->func_code = MB_SLV_FC03;
        break;
    }
    case 0x06: {
        if (len < 8) return -1;
        frame->start_addr = frame_get_u16_be(&buf[2]);
        frame->reg_value  = frame_get_u16_be(&buf[4]);
        frame->func_code = MB_SLV_FC06;
        break;
    }
    case 0x10: {
        if (len < 9) return -1;
        frame->start_addr = frame_get_u16_be(&buf[2]);
        frame->reg_count  = frame_get_u16_be(&buf[4]);
        frame->byte_count = buf[6];
        if (frame->byte_count != frame->reg_count * 2) return -1;
        if (len != (uint16_t)(7 + frame->byte_count + 2)) return -1;
        frame->data = &buf[7];
        frame->func_code = MB_SLV_FC10;
        break;
    }
    default:
        frame->func_code = MB_SLV_FC_NONE;
        break;
    }

    return 0;
}

uint16_t mb_slv_build_fc03_resp(const mb_slv_frame_t *frame, const uint16_t *regs, uint8_t *tx_buf)
{
    uint16_t reg_count = frame->reg_count;
    tx_buf[0] = frame->slave_addr;
    tx_buf[1] = frame->raw_func_code;
    tx_buf[2] = (uint8_t)(reg_count * 2);

    for (uint16_t i = 0; i < reg_count; i++) {
        tx_buf[3 + i * 2]     = (uint8_t)(regs[i] >> 8);
        tx_buf[3 + i * 2 + 1] = (uint8_t)(regs[i] & 0xFF);
    }

    uint16_t crc = mb_slv_crc16(tx_buf, 3 + reg_count * 2);
    tx_buf[3 + reg_count * 2]     = (uint8_t)(crc & 0xFF);
    tx_buf[3 + reg_count * 2 + 1] = (uint8_t)(crc >> 8);

    return (uint16_t)(5 + reg_count * 2);
}

uint16_t mb_slv_build_fc06_resp(const mb_slv_frame_t *frame, uint8_t *tx_buf)
{
    tx_buf[0] = frame->slave_addr;
    tx_buf[1] = frame->raw_func_code;
    tx_buf[2] = (uint8_t)(frame->start_addr >> 8);
    tx_buf[3] = (uint8_t)(frame->start_addr & 0xFF);
    tx_buf[4] = (uint8_t)(frame->reg_value >> 8);
    tx_buf[5] = (uint8_t)(frame->reg_value & 0xFF);

    uint16_t crc = mb_slv_crc16(tx_buf, 6);
    tx_buf[6] = (uint8_t)(crc & 0xFF);
    tx_buf[7] = (uint8_t)(crc >> 8);

    return 8;
}

uint16_t mb_slv_build_fc10_resp(const mb_slv_frame_t *frame, uint8_t *tx_buf)
{
    tx_buf[0] = frame->slave_addr;
    tx_buf[1] = frame->raw_func_code;
    tx_buf[2] = (uint8_t)(frame->start_addr >> 8);
    tx_buf[3] = (uint8_t)(frame->start_addr & 0xFF);
    tx_buf[4] = (uint8_t)(frame->reg_count >> 8);
    tx_buf[5] = (uint8_t)(frame->reg_count & 0xFF);

    uint16_t crc = mb_slv_crc16(tx_buf, 6);
    tx_buf[6] = (uint8_t)(crc & 0xFF);
    tx_buf[7] = (uint8_t)(crc >> 8);

    return 8;
}

uint16_t mb_slv_build_exception(const mb_slv_frame_t *frame, uint8_t ex_code, uint8_t *tx_buf)
{
    tx_buf[0] = frame->slave_addr;
    tx_buf[1] = frame->raw_func_code | 0x80;
    tx_buf[2] = ex_code;

    uint16_t crc = mb_slv_crc16(tx_buf, 3);
    tx_buf[3] = (uint8_t)(crc & 0xFF);
    tx_buf[4] = (uint8_t)(crc >> 8);

    return 5;
}
