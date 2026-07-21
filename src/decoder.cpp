#include "decoder.hpp"

// ------------------------------------------------------------
// 解碼的核心。
// 共同欄位（opcode/rd/funct3/rs1/rs2/funct7）用固定位置取出。
// 立即值 imm 依「指令格式」不同，位元散落的位置也不同，
// 所以要依 opcode 分開處理。這裡是最需要對照 RISC-V 指令格式表的地方。
//
// 小技巧：把值放到 int32_t 的高位再做「算術右移」，可以一次完成符號延伸。
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
        // I-type：OP-IMM(0x13) / LOAD(0x03) / JALR(0x67)
        case 0x13:
        case 0x03:
        case 0x67:
            d.imm = (int32_t)raw >> 20;  // 取 bits[31:20] 並符號延伸
            break;

        // S-type：STORE(0x23)
        case 0x23: {
            uint32_t imm = ((raw >> 25) << 5) | ((raw >> 7) & 0x1f);
            d.imm = (int32_t)(imm << 20) >> 20;  // 12-bit 符號延伸
            break;
        }

        // B-type：BRANCH(0x63)
        case 0x63: {
            uint32_t imm = (((raw >> 31) & 1)    << 12)
                         | (((raw >> 7)  & 1)    << 11)
                         | (((raw >> 25) & 0x3f) << 5)
                         | (((raw >> 8)  & 0xf)  << 1);
            d.imm = (int32_t)(imm << 19) >> 19;  // 13-bit 符號延伸
            break;
        }

        // U-type：LUI(0x37) / AUIPC(0x17)
        case 0x37:
        case 0x17:
            d.imm = (int32_t)(raw & 0xfffff000);
            break;

        // J-type：JAL(0x6f)
        case 0x6f: {
            uint32_t imm = (((raw >> 31) & 1)     << 20)
                         | (((raw >> 12) & 0xff)  << 12)
                         | (((raw >> 20) & 1)     << 11)
                         | (((raw >> 21) & 0x3ff) << 1);
            d.imm = (int32_t)(imm << 11) >> 11;  // 21-bit 符號延伸
            break;
        }

        default:
            d.imm = 0;  // R-type 等沒有立即值
            break;
    }
    return d;
}
