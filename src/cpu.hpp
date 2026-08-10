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

public:
    // --------------------------------------------------------
    // RVV 向量狀態
    // --------------------------------------------------------
    // VLEN：每個向量暫存器的位元寬度。這是硬體的固定參數，
    //       之後要做「向量寬度 vs 效能」的實驗，就掃描這個值。
    static constexpr uint32_t VLEN  = 128;         // 128 bits
    static constexpr uint32_t VLENB = VLEN / 8;    // = 16 bytes

    uint32_t vl()    const { return v_vl; }        // 這輪處理幾個元素
    uint32_t vtype() const { return v_vtype; }     // SEW / LMUL 編碼
    uint32_t stride() const { return v_stride; }   // 自訂指令的跨距

    // 讀某個向量暫存器的第 e 個 32-bit 元素（測試/除錯用）
    uint32_t vread32(uint32_t vreg, uint32_t e) const;
    // 寫某個向量暫存器的第 e 個 32-bit 元素（測試/除錯用）
    void vwrite32(uint32_t vreg, uint32_t e, uint32_t val);

private:
    Memory&    mem;
    Registers  reg;
    uint32_t   pc;
    bool       halt;
    Stats      st;
    std::vector<uint64_t> hits;

    std::array<uint32_t, 4096> csr{};

    // 向量暫存器 v0~v31：每個當成一排 byte（VLENB 個），
    // 要當 32-bit 元素用時再重新解讀。這樣 SEW 改變時最有彈性。
    std::array<std::array<uint8_t, VLENB>, 32> vreg{};
    uint32_t v_vl    = 0;   // vector length：這輪實際處理幾個元素
    uint32_t v_stride = 4;  // 自訂指令用：post-increment 的位元組跨距（預設一個元素）
    uint32_t v_vtype = 0;   // vtype：編碼了 SEW 與 LMUL

    // 從 vtype 解出目前的 SEW（每個元素幾 bytes）
    uint32_t cur_sew_bytes() const;

    void execute(const DecodedInst& d);
    void count(const DecodedInst& d);   // 更新統計
};
