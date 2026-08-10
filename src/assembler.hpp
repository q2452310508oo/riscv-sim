#pragma once
#include <cstdint>

// ============================================================
// 迷你組譯器：把指令欄位組成 32-bit 機器碼。
// 依 RISC-V 六種指令格式（R/I/S/B/U/J）分別提供編碼函式。
// main / gemm / 測試都用它來手動組出指令。
// ============================================================
namespace asm_ {

// R-type：add, sub, sll, ..., mul, div ...
inline uint32_t r(uint32_t op, uint32_t f3, uint32_t f7,
                  uint32_t rd, uint32_t rs1, uint32_t rs2) {
    return (f7 << 25) | (rs2 << 20) | (rs1 << 15) | (f3 << 12) | (rd << 7) | op;
}

// I-type：addi, andi, lw, jalr, csrrw ...
inline uint32_t i(uint32_t op, uint32_t f3,
                  uint32_t rd, uint32_t rs1, int32_t imm) {
    return ((uint32_t)(imm & 0xfff) << 20) | (rs1 << 15) | (f3 << 12) | (rd << 7) | op;
}

// S-type：sb, sh, sw
inline uint32_t s(uint32_t op, uint32_t f3,
                  uint32_t rs1, uint32_t rs2, int32_t imm) {
    uint32_t u = (uint32_t)imm;
    return (((u >> 5) & 0x7f) << 25) | (rs2 << 20) | (rs1 << 15)
         | (f3 << 12) | ((u & 0x1f) << 7) | op;
}

// B-type：beq, bne, blt, bge, bltu, bgeu
inline uint32_t b(uint32_t op, uint32_t f3,
                  uint32_t rs1, uint32_t rs2, int32_t imm) {
    uint32_t u = (uint32_t)imm;
    return (((u >> 12) & 1)  << 31) | (((u >> 5) & 0x3f) << 25)
         | (rs2 << 20) | (rs1 << 15) | (f3 << 12)
         | (((u >> 1) & 0xf) << 8)   | (((u >> 11) & 1) << 7) | op;
}

// U-type：lui, auipc
inline uint32_t u(uint32_t op, uint32_t rd, uint32_t imm20) {
    return ((imm20 & 0xfffff) << 12) | (rd << 7) | op;
}

// J-type：jal
inline uint32_t j(uint32_t op, uint32_t rd, int32_t imm) {
    uint32_t v = (uint32_t)imm;
    return (((v >> 20) & 1)   << 31) | (((v >> 1) & 0x3ff) << 21)
         | (((v >> 11) & 1)   << 20) | (((v >> 12) & 0xff) << 12)
         | (rd << 7) | op;
}

// ---- 常用 opcode / funct 常數 ----
constexpr uint32_t OP_IMM = 0x13, OP = 0x33, LOAD = 0x03, STORE = 0x23;
constexpr uint32_t BRANCH = 0x63, JAL = 0x6f, JALR = 0x67;
constexpr uint32_t LUI = 0x37, AUIPC = 0x17, SYSTEM = 0x73, FENCE = 0x0f;

// ---- 便捷包裝 ----
inline uint32_t addi(uint32_t rd, uint32_t rs1, int32_t imm) { return i(OP_IMM, 0x0, rd, rs1, imm); }
inline uint32_t add (uint32_t rd, uint32_t rs1, uint32_t rs2){ return r(OP, 0x0, 0x00, rd, rs1, rs2); }
inline uint32_t sub (uint32_t rd, uint32_t rs1, uint32_t rs2){ return r(OP, 0x0, 0x20, rd, rs1, rs2); }
inline uint32_t mul (uint32_t rd, uint32_t rs1, uint32_t rs2){ return r(OP, 0x0, 0x01, rd, rs1, rs2); }
inline uint32_t ecall() { return 0x00000073u; }

} // namespace asm_
