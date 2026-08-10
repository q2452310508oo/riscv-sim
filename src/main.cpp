#include <cstdint>
#include <vector>
#include <iostream>
#include "memory.hpp"
#include "cpu.hpp"
#include "assembler.hpp"

using namespace asm_;

int main() {
    // ========================================================
    // 示範程式：計算 1 + 2 + 3 + 4 + 5 = 15
    // 預期結果：x1 = 15，x2 = 6，x3 = 6
    // ========================================================
    std::vector<uint32_t> program = {
        addi(1, 0, 0),                 // sum = 0
        addi(2, 0, 1),                 // i   = 1
        addi(3, 0, 6),                 // limit = 6
        add(1, 1, 2),                  // sum += i   <- loop
        addi(2, 2, 1),                 // i++
        b(BRANCH, 0x1, 2, 3, -8),      // bne i, limit, loop
        ecall(),                       // 停止
    };

    Memory mem(64 * 1024);
    mem.load_program(program, 0);

    CPU cpu(mem);
    cpu.run();

    std::cout << "程式結束，共執行 " << cpu.instret() << " 條指令。\n\n";
    cpu.regs().dump();
    std::cout << "\n預期：x1 = 15（1..5 的總和），x2 = 6，x3 = 6\n";
    return 0;
}
