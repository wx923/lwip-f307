#include "mb_slv_func.h"
#include "mb_slv_data.h"

int mb_func_dispatch(const mb_slv_frame_t *frame, uint8_t *tx_buf, uint16_t *tx_len)
{
    uint16_t regs[125];

    switch (frame->func_code) {

    case MB_SLV_FC03: {
        if (frame->reg_count == 0 || frame->reg_count > 125) {
            *tx_len = mb_slv_build_exception(frame, 0x03, tx_buf);
            return -1;
        }
        if (mb_data_read(frame->start_addr, frame->reg_count, regs) != 0) {
            *tx_len = mb_slv_build_exception(frame, 0x02, tx_buf);
            return -1;
        }
        *tx_len = mb_slv_build_fc03_resp(frame, regs, tx_buf);
        return 0;
    }

    case MB_SLV_FC06: {
        if (mb_data_write(frame->start_addr, 1, (const uint8_t *)&frame->reg_value) != 0) {
            *tx_len = mb_slv_build_exception(frame, 0x02, tx_buf);
            return -1;
        }
        *tx_len = mb_slv_build_fc06_resp(frame, tx_buf);
        return 0;
    }

    case MB_SLV_FC10: {
        if (frame->reg_count == 0 || frame->reg_count > 123) {
            *tx_len = mb_slv_build_exception(frame, 0x03, tx_buf);
            return -1;
        }
        if (mb_data_write(frame->start_addr, frame->reg_count, frame->data) != 0) {
            *tx_len = mb_slv_build_exception(frame, 0x02, tx_buf);
            return -1;
        }
        *tx_len = mb_slv_build_fc10_resp(frame, tx_buf);
        return 0;
    }

    default: {
        *tx_len = mb_slv_build_exception(frame, 0x01, tx_buf);
        return -1;
    }
    }
}
