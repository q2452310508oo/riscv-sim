#pragma once
#include <cstdint>
#include <array>
#include "memory.hpp"
#include "registers.hpp"
#include "decoder.hpp"

// ============================================================
// CPU：整個模擬器的心臟。
// 主迴圈就是經典的 fetch -> decode -> execute。
// RVV 向量指令、還有你之後要加的「自訂指令」，都加在 execute 裡。
// ============================================================
class CPU {
public:
    explicit CPU(Memory& mem);

    void run();          // 一直跑到 halt
    void step();         // 執行一條指令

    bool     halted()  const { return halt; }
    uint64_t instret() const { return inst_count; }  // 已執行指令數（效能指標）
    const Registers& regs() const { return reg; }

private:
    Memory&    mem;
    Registers  reg;
    uint32_t   pc;          // program counter：下一條要抓的指令位址
    bool       halt;
    uint64_t   inst_count;  // 執行了幾條指令

    // 控制與狀態暫存器（CSR）。位址是 12-bit，故共 4096 個。
    // CSRRW/CSRRS/CSRRC 等指令會存取這裡。
    std::array<uint32_t, 4096> csr{};

    void execute(const DecodedInst& d);
};
