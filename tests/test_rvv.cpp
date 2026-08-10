// ============================================================
// RVV W2 測試：vsetvli
// 驗證「向量長度是執行時由硬體決定」這個 RVV 核心行為。
// VLEN=128, SEW=32 → 一個向量暫存器裝得下 4 個元素（VLMAX=4）。
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

// 跑一段程式，檢查某暫存器的值
static void check(const std::string& name, const std::vector<uint32_t>& prog,
                  uint32_t reg_idx, uint32_t expected) {
    Memory mem(64 * 1024);
    mem.load_program(prog, 0);
    CPU cpu(mem);
    cpu.run();
    uint32_t got = cpu.regs().read(reg_idx);
    if (got == expected) { ++g_pass; std::cout << "[PASS] " << name << "\n"; }
    else { ++g_fail; std::cout << "[FAIL] " << name
                               << "  得到 " << got << " 預期 " << expected << "\n"; }
}

int main() {
    // VLMAX = VLEN/SEW = 128/32 = 4

    // 還想做 10 個 → 硬體這輪只給 4（VLMAX 上限）
    check("vsetvli AVL=10 -> vl=4",
          { addi(1,0,10), vsetvli(2,1, VSEW_32|VLMUL_1), ecall() }, 2, 4);

    // 還想做 3 個 → 硬體給 3（沒超過上限，全給）
    check("vsetvli AVL=3 -> vl=3",
          { addi(1,0,3), vsetvli(2,1, VSEW_32|VLMUL_1), ecall() }, 2, 3);

    // 還想做 4 個 → 剛好給滿 4
    check("vsetvli AVL=4 -> vl=4",
          { addi(1,0,4), vsetvli(2,1, VSEW_32|VLMUL_1), ecall() }, 2, 4);

    // SEW=8 時，VLMAX = 128/8 = 16；還想做 100 個 → 給 16
    check("vsetvli SEW=8 AVL=100 -> vl=16",
          { addi(1,0,100), vsetvli(2,1, VSEW_8|VLMUL_1), ecall() }, 2, 16);

    // SEW=16 時，VLMAX = 128/16 = 8
    check("vsetvli SEW=16 AVL=100 -> vl=8",
          { addi(1,0,100), vsetvli(2,1, VSEW_16|VLMUL_1), ecall() }, 2, 8);

    // rs1 = x0 且 rd != x0 → 直接取 VLMAX（要最長）
    check("vsetvli rs1=x0 -> vl=VLMAX=4",
          { vsetvli(2,0, VSEW_32|VLMUL_1), ecall() }, 2, 4);

    std::cout << "\n=== RVV W2 (vsetvli) 通過 " << g_pass << " / "
              << (g_pass+g_fail) << " ===\n";
    return g_fail == 0 ? 0 : 1;
}
