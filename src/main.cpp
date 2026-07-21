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
    //
    //   addi x1, x0, 0     # sum = 0
    //   addi x2, x0, 1     # i   = 1
    //   addi x3, x0, 6     # limit = 6
    // loop:
    //   add  x1, x1, x2    # sum += i
    //   addi x2, x2, 1     # i++
    //   bne  x2, x3, loop  # 若 i != 6 就跳回 loop（往回 2 條 = -8 bytes）
    //   ecall              # 停止
    //
    // 預期結果：x1 = 15，x2 = 6，x3 = 6
    // ========================================================
    std::vector<uint32_t> program = {
        addi(1, 0, 0),                 // addi x1, x0, 0
        addi(2, 0, 1),                 // addi x2, x0, 1
        addi(3, 0, 6),                 // addi x3, x0, 6
        add(1, 1, 2),                  // add  x1, x1, x2   <- loop
        addi(2, 2, 1),                 // addi x2, x2, 1
        b(BRANCH, 0x1, 2, 3, -8),      // bne  x2, x3, loop
        ecall(),                       // 停止
    };

    Memory mem(64 * 1024);          // 64 KB 記憶體
    mem.load_program(program, 0);   // 從位址 0 載入

    CPU cpu(mem);
    cpu.run();

    std::cout << "程式結束，共執行 " << cpu.instret() << " 條指令。\n\n";
    cpu.regs().dump();
    std::cout << "\n預期：x1 = 15（1..5 的總和），x2 = 6，x3 = 6\n";
    return 0;
}
