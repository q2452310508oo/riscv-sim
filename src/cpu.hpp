#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include "memory.hpp"
#include "registers.hpp"
#include "decoder.hpp"

// ============================================================
// CPU：整個模擬器的心臟。
// 主迴圈就是經典的 fetch -> decode -> execute。
// RVV 向量指令、還有之後要加的「自訂指令」，都加在 execute 裡。
// ============================================================
class CPU {
public:
    // --------------------------------------------------------
    // 效能統計。分成三個層次：
    //   1. 指令分類計數 —— 「執行了哪幾種指令」
    //   2. 估計 cycle 數 —— 「大概花多少時間」（模型，非實測）
    //   3. 每個 PC 的執行次數 —— 讓外部程式做更細的分析
    // --------------------------------------------------------
    struct Stats {
        uint64_t total  = 0;   // 總指令數
        uint64_t alu    = 0;   // 算術/邏輯（add, addi, slli, xor ...）
        uint64_t muldiv = 0;   // 乘除法（RV32M）
        uint64_t mem    = 0;   // load / store
        uint64_t ctrl   = 0;   // 分支與跳躍
        uint64_t upper  = 0;   // lui / auipc
        uint64_t sys    = 0;   // system / fence / CSR
        uint64_t vec    = 0;   // RVV 向量指令（W2 之後會用到）
        uint64_t custom = 0;   // 自訂指令（W6 之後會用到）
        uint64_t cycles = 0;   // 估計 cycle 數
    };

    explicit CPU(Memory& mem);

    void run();          // 一直跑到 halt
    void step();         // 執行一條指令

    bool     halted()  const { return halt; }
    uint64_t instret() const { return st.total; }
    uint64_t cycles()  const { return st.cycles; }
    const Stats&    stats() const { return st; }
    const Registers& regs() const { return reg; }

    // 每個位址被執行的次數（索引 = pc / 4），供外部做熱點分析
    const std::vector<uint64_t>& pc_hits() const { return hits; }

private:
    Memory&    mem;
    Registers  reg;
    uint32_t   pc;
    bool       halt;
    Stats      st;
    std::vector<uint64_t> hits;

    std::array<uint32_t, 4096> csr{};

    void execute(const DecodedInst& d);
    void count(const DecodedInst& d);   // 更新統計
};
