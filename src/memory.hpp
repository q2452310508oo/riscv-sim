#pragma once
#include <cstdint>
#include <vector>

// ============================================================
// Memory：模擬處理器的記憶體
// 內部就是一大塊 byte 陣列。CPU 抓指令、load/store 都透過它。
// RISC-V 是 little-endian（低位元組放在低位址）。
// ============================================================
class Memory {
public:
    explicit Memory(std::size_t size_bytes);

    uint8_t  load8(uint32_t addr) const;
    uint16_t load16(uint32_t addr) const;
    uint32_t load32(uint32_t addr) const;

    void store8(uint32_t addr, uint8_t val);
    void store16(uint32_t addr, uint16_t val);
    void store32(uint32_t addr, uint32_t val);

    void load_program(const std::vector<uint32_t>& words, uint32_t start_addr = 0);

    std::size_t size() const { return mem.size(); }

private:
    std::vector<uint8_t> mem;
};
