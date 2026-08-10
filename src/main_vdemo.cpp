#include <cstdint>
#include <vector>
#include <iostream>
#include <iomanip>
#include "memory.hpp"
#include "cpu.hpp"
#include "assembler.hpp"

using namespace asm_;

// 印出記憶體某段的 4 個 int32
static void show(const char* label, Memory& mem, uint32_t addr) {
    std::cout << label << " = [ ";
    for (int e = 0; e < 4; ++e)
        std::cout << std::setw(4) << (int32_t)mem.load32(addr + e*4) << " ";
    std::cout << "]\n";
}

int main() {
    Memory mem(64 * 1024);

    // 準備兩組資料
    uint32_t A = 0x1000, B = 0x1100, C = 0x2000;
    int32_t a[4] = { 10, 20, 30, 50 };
    int32_t b[4] = {  1,  2,  3,  5 };
    for (int e = 0; e < 4; ++e) {
        mem.store32(A + e*4, (uint32_t)a[e]);
        mem.store32(B + e*4, (uint32_t)b[e]);
    }

    // 組一段程式：v1<-A, v2<-B, v3=v1+v2, 存回 C
    std::vector<uint32_t> prog = {
        addi(1,0,4), vsetvli(0,1, VSEW_32|VLMUL_1),   // vl = 4
        u(LUI,10,1),                    vle32(1,10),  // v1 <- mem[0x1000]
        addi(11,0,0x100), u(LUI,12,1), add(11,11,12), vle32(2,11), // v2 <- mem[0x1100]
        vadd_vv(3,1,2),                                // v3 = v1 + v2
        u(LUI,13,2),                    vse32(3,13),  // mem[0x2000] <- v3
        ecall()
    };
    mem.load_program(prog, 0);

    CPU cpu(mem);
    cpu.run();

    std::cout << "=== 向量加法示範（一條 vadd.vv 同時算 4 個）===\n\n";
    show("v1 (A)", mem, A);
    show("v2 (B)", mem, B);
    std::cout << "        +  ------------------------\n";
    show("v3 = v1+v2", mem, C);
    std::cout << "\n共執行 " << cpu.instret() << " 條指令。\n";
    return 0;
}
