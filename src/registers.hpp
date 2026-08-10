#pragma once
#include <cstdint>
#include <array>

// ============================================================
// Registers：32 個純量暫存器 x0 ~ x31
// 重要規則：x0 永遠是 0，寫進去會被忽略。
// 之後要加 RVV 時，向量暫存器 v0~v31 放在 CPU 裡（見 cpu.hpp）。
// ============================================================
class Registers {
public:
    Registers();

    uint32_t read(uint32_t idx) const;
    void write(uint32_t idx, uint32_t val);

    void dump() const;

private:
    std::array<uint32_t, 32> x;
};
