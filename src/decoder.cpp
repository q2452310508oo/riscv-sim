#include "decoder.hpp"

// ------------------------------------------------------------
// 解碼核心。共同欄位固定位置取出；立即值依指令格式散落不同位置，
// 依 opcode 分開處理。小技巧：放到 int32_t 高位再算術右移 = 符號延伸。
// ------------------------------------------------------------
DecodedInst decode(uint32_t raw) {
    DecodedInst d;
    d.raw    = raw;
    d.opcode = raw & 0x7f;
    d.rd     = (raw >> 7)  & 0x1f;
    d.funct3 = (raw >> 12) & 0x7;
    d.rs1    = (raw >> 15) & 0x1f;
    d.rs2    = (raw >> 20) & 0x1f;
    d.funct7 = (raw >> 25) & 0x7f;

    switch (d.opcode) {
        case 0x13: case 0x03: case 0x67:              // I-type
            d.imm = (int32_t)raw >> 20;
            break;
        case 0x23: {                                   // S-type
            uint32_t imm = ((raw >> 25) << 5) | ((raw >> 7) & 0x1f);
            d.imm = (int32_t)(imm << 20) >> 20;
            break;
        }
        case 0x63: {                                   // B-type
            uint32_t imm = (((raw >> 31) & 1)    << 12)
                         | (((raw >> 7)  & 1)    << 11)
                         | (((raw >> 25) & 0x3f) << 5)
                         | (((raw >> 8)  & 0xf)  << 1);
            d.imm = (int32_t)(imm << 19) >> 19;
            break;
        }
        case 0x37: case 0x17:                          // U-type
            d.imm = (int32_t)(raw & 0xfffff000);
            break;
        case 0x6f: {                                   // J-type
            uint32_t imm = (((raw >> 31) & 1)     << 20)
                         | (((raw >> 12) & 0xff)  << 12)
                         | (((raw >> 20) & 1)     << 11)
                         | (((raw >> 21) & 0x3ff) << 1);
            d.imm = (int32_t)(imm << 11) >> 11;
            break;
        }
        default:
            d.imm = 0;
            break;
    }
    return d;
}
