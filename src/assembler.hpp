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


// ============================================================
// RVV（向量擴展）指令編碼。opcode 一律是 0x57。
// ============================================================
constexpr uint32_t OP_V = 0x57;

// ---- vtype 立即值的組法 ----
// vtype[2:0] = vlmul, vtype[5:3] = vsew
// 這裡提供常數方便閱讀（LMUL=1，SEW 可選）
constexpr uint32_t VSEW_8  = 0 << 3;
constexpr uint32_t VSEW_16 = 1 << 3;
constexpr uint32_t VSEW_32 = 2 << 3;
constexpr uint32_t VSEW_64 = 3 << 3;
constexpr uint32_t VLMUL_1 = 0;   // LMUL = 1

// vsetvli rd, rs1, vtype_imm
//   funct3 = 0x7，bit[31]=0，vtype 放 bits[30:20]
inline uint32_t vsetvli(uint32_t rd, uint32_t rs1, uint32_t vtype_imm) {
    return ((vtype_imm & 0x7ff) << 20) | (rs1 << 15) | (0x7 << 12) | (rd << 7) | OP_V;
}


// ---- 向量載入 / 儲存（unit-stride, 32-bit）----
// vle32.v vd, (rs1)   funct3=0x6
inline uint32_t vle32(uint32_t vd, uint32_t rs1) {
    // 高位欄位（nf/mew/mop/vm/lumop）在單純情況都為 0，width=0x6 放 funct3
    return (rs1 << 15) | (0x6 << 12) | (vd << 7) | OP_V;
}
// vse32.v vs3, (rs1)  funct3=0x5  （來源向量放在 vs3=bits[11:7]）
inline uint32_t vse32(uint32_t vs3, uint32_t rs1) {
    return (rs1 << 15) | (0x5 << 12) | (vs3 << 7) | OP_V;
}

// ---- 向量整數運算（OPIVV, funct3=0x0）----
// 通用：funct6 決定運算；vd=bits[11:7], vs1=bits[19:15], vs2=bits[24:20]
// vm=1（bit25）表示無遮罩（所有 lane 都算）
inline uint32_t opivv(uint32_t funct6, uint32_t vd, uint32_t vs2, uint32_t vs1) {
    return (funct6 << 26) | (1u << 25) | (vs2 << 20) | (vs1 << 15)
         | (0x0 << 12) | (vd << 7) | OP_V;
}
// vadd.vv vd, vs2, vs1   (vd = vs2 + vs1)
inline uint32_t vadd_vv(uint32_t vd, uint32_t vs2, uint32_t vs1) { return opivv(0x00, vd, vs2, vs1); }
// vsub.vv vd, vs2, vs1   (vd = vs2 - vs1)
inline uint32_t vsub_vv(uint32_t vd, uint32_t vs2, uint32_t vs1) { return opivv(0x02, vd, vs2, vs1); }


// ---- 向量-純量乘加 vmacc.vx vd, rs1, vs2  (vd += x[rs1] * vs2) ----
// funct3=0x4 (OPIVX 空間), funct6=0x2d, vm=1
inline uint32_t vmacc_vx(uint32_t vd, uint32_t rs1, uint32_t vs2) {
    return (0x2d << 26) | (1u << 25) | (vs2 << 20) | (rs1 << 15)
         | (0x4 << 12) | (vd << 7) | OP_V;
}

} // namespace asm_
