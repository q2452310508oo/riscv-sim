#include "registers.hpp"
#include <iostream>
#include <iomanip>

// ABI 名稱，dump 時方便對照（純顯示用）
static const char* kAbiName[32] = {
    "zero", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
    "s0",   "s1", "a0", "a1", "a2", "a3", "a4", "a5",
    "a6",   "a7", "s2", "s3", "s4", "s5", "s6", "s7",
    "s8",   "s9", "s10","s11","t3", "t4", "t5", "t6"
};

Registers::Registers() { x.fill(0); }

uint32_t Registers::read(uint32_t idx) const {
    // x0 永遠讀出 0
    if (idx == 0) return 0;
    return x[idx];
}

void Registers::write(uint32_t idx, uint32_t val) {
    // 寫 x0 沒有效果
    if (idx == 0) return;
    x[idx] = val;
}

void Registers::dump() const {
    std::cout << "---- Registers ----\n";
    for (uint32_t i = 0; i < 32; ++i) {
        std::cout << "x" << std::setw(2) << std::left << i
                  << "(" << std::setw(4) << std::left << kAbiName[i] << ") = "
                  << std::setw(11) << std::left << (int32_t)read(i)
                  << " (0x" << std::hex << read(i) << std::dec << ")\n";
    }
    std::cout << "-------------------\n";
}
