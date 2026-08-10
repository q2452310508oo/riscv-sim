// ============================================================
// RVV W2 測試：vsetvli
// 驗證「向量長度是執行時由硬體決定」這個 RVV 核心行為。
// VLEN=128, SEW=32 → 一個向量暫存器裝得下 4 個元素（VLMAX=4）。
// ============================================================
#include <cstdint>
#include <vector>
#include <string>
#include <utility>
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


// 跑一段程式，檢查記憶體某位址的 32-bit 值
static void check_mem(const std::string& name, const std::vector<uint32_t>& prog,
                      uint32_t addr, uint32_t expected,
                      const std::vector<std::pair<uint32_t,uint32_t>>& preset = {}) {
    Memory mem(64 * 1024);
    mem.load_program(prog, 0);
    for (auto& p : preset) mem.store32(p.first, p.second);  // 預先放資料
    CPU cpu(mem);
    cpu.run();
    uint32_t got = mem.load32(addr);
    if (got == expected) { ++g_pass; std::cout << "[PASS] " << name << "\n"; }
    else { ++g_fail; std::cout << "[FAIL] " << name
                               << "  記憶體[0x" << std::hex << addr << "]="
                               << std::dec << got << " 預期 " << expected << "\n"; }
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


    // ========== W3: 向量載入 / 儲存 / 加法 ==========
    // 資料放在 0x1000，載入 v1，加到 v2（也從 0x1000 載），存回 0x2000。

    // 測試 1：來回搬移（load → store），驗證 vle32/vse32 正確
    //   在 0x1000 放 [11,22,33,44]，載入 v1，存回 0x2000，檢查 0x2000 第一個是 11
    check_mem("vle32+vse32 搬移 [0]",
        { addi(1,0,4), vsetvli(0,1, VSEW_32|VLMUL_1),   // vl=4
          i(OP_IMM,0,10,0,0), u(LUI,10,1),              // x10 = 0x1000
          vle32(1, 10),                                  // v1 <- mem[0x1000..]
          i(OP_IMM,0,11,0,0), u(LUI,11,2),              // x11 = 0x2000
          vse32(1, 11),                                  // mem[0x2000..] <- v1
          ecall() },
        0x2000, 11, {{0x1000,11},{0x1004,22},{0x1008,33},{0x100c,44}});

    // 測試 2：搬移後檢查第 3 個元素（0x2000+8）是 33
    check_mem("vle32+vse32 搬移 [2]",
        { addi(1,0,4), vsetvli(0,1, VSEW_32|VLMUL_1),
          u(LUI,10,1), vle32(1, 10),
          u(LUI,11,2), vse32(1, 11),
          ecall() },
        0x2008, 33, {{0x1000,11},{0x1004,22},{0x1008,33},{0x100c,44}});

    // 測試 3：vadd.vv —— v3 = v1 + v2，逐 lane 相加
    //   v1 <- [10,20,30,40]（0x1000）, v2 <- [1,2,3,4]（0x1100）
    //   v3 = v1+v2 = [11,22,33,44]，存回 0x2000，檢查第 2 個 = 22
    check_mem("vadd.vv [1]=20+2=22",
        { addi(1,0,4), vsetvli(0,1, VSEW_32|VLMUL_1),
          u(LUI,10,1), vle32(1, 10),                     // v1 <- 0x1000
          addi(11,0,0x100), u(LUI,12,1), add(11,11,12),  // x11 = 0x1100
          vle32(2, 11),                                  // v2 <- 0x1100
          vadd_vv(3, 1, 2),                              // v3 = v1 + v2
          u(LUI,13,2), vse32(3, 13),                     // mem[0x2000] <- v3
          ecall() },
        0x2004, 22,
        {{0x1000,10},{0x1004,20},{0x1008,30},{0x100c,40},
         {0x1100,1},{0x1104,2},{0x1108,3},{0x110c,4}});

    // 測試 4：vadd.vv 檢查最後一個 lane = 40+4 = 44
    check_mem("vadd.vv [3]=40+4=44",
        { addi(1,0,4), vsetvli(0,1, VSEW_32|VLMUL_1),
          u(LUI,10,1), vle32(1, 10),
          addi(11,0,0x100), u(LUI,12,1), add(11,11,12),
          vle32(2, 11),
          vadd_vv(3, 1, 2),
          u(LUI,13,2), vse32(3, 13),
          ecall() },
        0x200c, 44,
        {{0x1000,10},{0x1004,20},{0x1008,30},{0x100c,40},
         {0x1100,1},{0x1104,2},{0x1108,3},{0x110c,4}});

    // 測試 5：只設 vl=2，vadd 只算前 2 個 lane，第 3 個維持 0
    check_mem("vl=2 只算前兩個",
        { addi(1,0,2), vsetvli(0,1, VSEW_32|VLMUL_1),    // vl=2
          u(LUI,10,1), vle32(1, 10),
          addi(11,0,0x100), u(LUI,12,1), add(11,11,12),
          vle32(2, 11),
          vadd_vv(3, 1, 2),
          u(LUI,13,2), vse32(3, 13),
          ecall() },
        0x2000, 11,   // 第一個 lane 有算 = 10+1
        {{0x1000,10},{0x1004,20},{0x1100,1},{0x1104,2}});

    std::cout << "\n=== RVV W2+W3 通過 " << g_pass << " / "
              << (g_pass+g_fail) << " ===\n";
    return g_fail == 0 ? 0 : 1;
}
