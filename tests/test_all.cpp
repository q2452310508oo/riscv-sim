// ============================================================
// RV32IM 全指令自動測試
// 逐一組出一小段程式測試每條指令，比對暫存器/記憶體結果。
// ============================================================
#include <cstdint>
#include <vector>
#include <string>
#include <iostream>
#include "memory.hpp"
#include "cpu.hpp"
#include "assembler.hpp"

using namespace asm_;

static int g_pass = 0, g_fail = 0;

static void check(const std::string& name, const std::vector<uint32_t>& prog,
                  uint32_t reg_idx, uint32_t expected) {
    Memory mem(64 * 1024);
    mem.load_program(prog, 0);
    CPU cpu(mem);
    cpu.run();
    uint32_t got = cpu.regs().read(reg_idx);
    if (got == expected) {
        ++g_pass;
        std::cout << "[PASS] " << name << "\n";
    } else {
        ++g_fail;
        std::cout << "[FAIL] " << name << "  x" << reg_idx
                  << " 得到 " << (int32_t)got << " (0x" << std::hex << got << std::dec
                  << ")，預期 " << (int32_t)expected << " (0x" << std::hex << expected << std::dec << ")\n";
    }
}

int main() {
    // ---------- OP-IMM ----------
    check("addi",  { addi(1,0,42), ecall() }, 1, 42);
    check("slti",  { addi(1,0,5),  i(OP_IMM,0x2,2,1,10), ecall() }, 2, 1);
    check("sltiu", { i(OP_IMM,0x3,2,0,1), ecall() }, 2, 1);
    check("xori",  { addi(1,0,15), i(OP_IMM,0x4,2,1,255), ecall() }, 2, 15 ^ 255);
    check("ori",   { addi(1,0,240), i(OP_IMM,0x6,2,1,15), ecall() }, 2, 255);
    check("andi",  { addi(1,0,240), i(OP_IMM,0x7,2,1,15), ecall() }, 2, 0);
    check("slli",  { addi(1,0,1),  i(OP_IMM,0x1,2,1,4), ecall() }, 2, 16);
    check("srli",  { addi(1,0,256),i(OP_IMM,0x5,2,1,4), ecall() }, 2, 16);
    check("srai",  { addi(1,0,-16),i(OP_IMM,0x5,2,1,(0x20<<5)|2), ecall() }, 2, (uint32_t)(-4));

    // ---------- OP ----------
    check("add",  { addi(1,0,7),  addi(2,0,8),  r(OP,0x0,0x00,3,1,2), ecall() }, 3, 15);
    check("sub",  { addi(1,0,20), addi(2,0,5),  r(OP,0x0,0x20,3,1,2), ecall() }, 3, 15);
    check("sll",  { addi(1,0,1),  addi(2,0,4),  r(OP,0x1,0x00,3,1,2), ecall() }, 3, 16);
    check("slt",  { addi(1,0,-1), addi(2,0,1),  r(OP,0x2,0x00,3,1,2), ecall() }, 3, 1);
    check("sltu", { addi(1,0,-1), addi(2,0,1),  r(OP,0x3,0x00,3,1,2), ecall() }, 3, 0);
    check("xor",  { addi(1,0,15), addi(2,0,255),r(OP,0x4,0x00,3,1,2), ecall() }, 3, 15 ^ 255);
    check("srl",  { addi(1,0,-16),addi(2,0,4),  r(OP,0x5,0x00,3,1,2), ecall() }, 3, 0x0FFFFFFFu);
    check("sra",  { addi(1,0,-16),addi(2,0,4),  r(OP,0x5,0x20,3,1,2), ecall() }, 3, (uint32_t)(-1));
    check("or",   { addi(1,0,240),addi(2,0,15), r(OP,0x6,0x00,3,1,2), ecall() }, 3, 255);
    check("and",  { addi(1,0,240),addi(2,0,15), r(OP,0x7,0x00,3,1,2), ecall() }, 3, 0);

    // ---------- LOAD / STORE ----------
    check("sw+lw", { addi(1,0,0x100), addi(2,0,1234), s(STORE,0x2,1,2,0),
                     i(LOAD,0x2,3,1,0), ecall() }, 3, 1234);
    check("sh+lh", { addi(1,0,0x200), addi(2,0,-5), s(STORE,0x1,1,2,0),
                     i(LOAD,0x1,3,1,0), ecall() }, 3, (uint32_t)(-5));
    check("sb+lb", { addi(1,0,0x210), addi(2,0,-2), s(STORE,0x0,1,2,0),
                     i(LOAD,0x0,3,1,0), ecall() }, 3, (uint32_t)(-2));
    check("lbu",   { addi(1,0,0x220), addi(2,0,-2), s(STORE,0x0,1,2,0),
                     i(LOAD,0x4,3,1,0), ecall() }, 3, 254);
    check("lhu",   { addi(1,0,0x230), addi(2,0,-5), s(STORE,0x1,1,2,0),
                     i(LOAD,0x5,3,1,0), ecall() }, 3, 0xFFFBu);

    // ---------- BRANCH ----------
    check("beq",  { addi(1,0,5),  addi(2,0,5),  b(BRANCH,0x0,1,2,8), addi(3,0,111), ecall() }, 3, 0);
    check("bne",  { addi(1,0,5),  addi(2,0,6),  b(BRANCH,0x1,1,2,8), addi(3,0,111), ecall() }, 3, 0);
    check("blt",  { addi(1,0,-1), addi(2,0,1),  b(BRANCH,0x4,1,2,8), addi(3,0,111), ecall() }, 3, 0);
    check("bge",  { addi(1,0,2),  addi(2,0,1),  b(BRANCH,0x5,1,2,8), addi(3,0,111), ecall() }, 3, 0);
    check("bltu", { addi(1,0,1),  addi(2,0,-1), b(BRANCH,0x6,1,2,8), addi(3,0,111), ecall() }, 3, 0);
    check("bgeu", { addi(1,0,-1), addi(2,0,1),  b(BRANCH,0x7,1,2,8), addi(3,0,111), ecall() }, 3, 0);

    // ---------- JUMP ----------
    check("jal(link)",  { j(JAL,1,8), addi(2,0,111), addi(3,0,222), ecall() }, 1, 4);
    check("jal(jump)",  { j(JAL,1,8), addi(2,0,111), addi(3,0,222), ecall() }, 2, 0);
    check("jalr", { addi(1,0,16), i(JALR,0x0,2,1,0),
                    addi(3,0,111), addi(4,0,111), addi(5,0,222), ecall() }, 5, 222);

    // ---------- UPPER IMMEDIATE ----------
    check("lui",   { u(LUI,1,0x12345), ecall() }, 1, 0x12345000u);
    check("auipc", { u(AUIPC,1,0x1), ecall() }, 1, 0x1000u);

    // ---------- SYSTEM: CSR ----------
    check("csrrw",  { addi(1,0,123), i(SYSTEM,0x1,2,1,0x340),
                      i(SYSTEM,0x1,3,0,0x340), ecall() }, 3, 123);
    check("csrrs",  { addi(1,0,0xF), i(SYSTEM,0x2,2,1,0x340),
                      i(SYSTEM,0x2,3,0,0x340), ecall() }, 3, 0xF);
    check("csrrc",  { addi(1,0,0xFF), i(SYSTEM,0x1,0,1,0x340),
                      addi(2,0,0x0F), i(SYSTEM,0x3,3,2,0x340), ecall() }, 3, 0xFF);
    check("csrrwi", { i(SYSTEM,0x5,2,5,0x340),
                      i(SYSTEM,0x2,3,0,0x340), ecall() }, 3, 5);
    check("csrrsi", { i(SYSTEM,0x5,0,0,0x340),
                      i(SYSTEM,0x6,2,3,0x340),
                      i(SYSTEM,0x2,3,0,0x340), ecall() }, 3, 3);
    check("csrrci", { i(SYSTEM,0x5,0,31,0x340),
                      i(SYSTEM,0x7,2,1,0x340),
                      i(SYSTEM,0x2,3,0,0x340), ecall() }, 3, 30);

    // ---------- ecall / ebreak ----------
    check("ecall",  { addi(1,0,7), ecall(), addi(1,0,99) }, 1, 7);
    check("ebreak", { addi(1,0,7), i(SYSTEM,0x0,0,0,1), addi(1,0,99) }, 1, 7);

    // ---------- FENCE ----------
    check("fence",   { i(FENCE,0x0,0,0,0), addi(1,0,7), ecall() }, 1, 7);
    check("fence.i", { i(FENCE,0x1,0,0,0), addi(1,0,9), ecall() }, 1, 9);

    // ========== RV32M ==========
    check("mul",    { addi(1,0,7),   addi(2,0,6),  r(OP,0x0,0x01,3,1,2), ecall() }, 3, 42);
    check("mulh",   { addi(1,0,-1),  addi(2,0,-1), r(OP,0x1,0x01,3,1,2), ecall() }, 3, 0);
    check("mulhsu", { addi(1,0,-1),  addi(2,0,2),  r(OP,0x2,0x01,3,1,2), ecall() }, 3, 0xFFFFFFFFu);
    check("mulhu",  { addi(1,0,-1),  addi(2,0,-1), r(OP,0x3,0x01,3,1,2), ecall() }, 3, 0xFFFFFFFEu);
    check("div",    { addi(1,0,-20), addi(2,0,3),  r(OP,0x4,0x01,3,1,2), ecall() }, 3, (uint32_t)(-6));
    check("divu",   { addi(1,0,20),  addi(2,0,3),  r(OP,0x5,0x01,3,1,2), ecall() }, 3, 6);
    check("rem",    { addi(1,0,-20), addi(2,0,3),  r(OP,0x6,0x01,3,1,2), ecall() }, 3, (uint32_t)(-2));
    check("remu",   { addi(1,0,20),  addi(2,0,3),  r(OP,0x7,0x01,3,1,2), ecall() }, 3, 2);
    check("div/0",  { addi(1,0,5),   addi(2,0,0),  r(OP,0x4,0x01,3,1,2), ecall() }, 3, 0xFFFFFFFFu);
    check("rem/0",  { addi(1,0,5),   addi(2,0,0),  r(OP,0x6,0x01,3,1,2), ecall() }, 3, 5);

    // ---------- 總計 ----------
    std::cout << "\n==================================\n";
    std::cout << "通過 " << g_pass << " / " << (g_pass + g_fail) << " 個測試\n";
    if (g_fail == 0) std::cout << "RV32IM 全部指令測試通過！\n";
    else             std::cout << g_fail << " 個失敗，請檢查。\n";
    std::cout << "==================================\n";
    return g_fail == 0 ? 0 : 1;
}
