#pragma once
#include <cstdint>

// ============================================================
// 一條指令解碼後的樣子。
// 一個 32-bit 指令會被切成這些欄位，execute 階段再看這些欄位做事。
// ============================================================
struct DecodedInst {
    uint32_t raw    = 0;  // 原始 32-bit 指令
    uint32_t opcode = 0;  // bits[6:0]   決定指令大類別
    uint32_t rd     = 0;  // bits[11:7]  目標暫存器
    uint32_t funct3 = 0;  // bits[14:12] 細分功能
    uint32_t rs1    = 0;  // bits[19:15] 來源暫存器 1
    uint32_t rs2    = 0;  // bits[24:20] 來源暫存器 2
    uint32_t funct7 = 0;  // bits[31:25] 更細的功能（例如區分 add / sub）
    int32_t  imm    = 0;  // 立即值（已依指令格式做好符號延伸）
};

// 把一個 32-bit 原始指令拆解成 DecodedInst
DecodedInst decode(uint32_t raw);
