#include "memory.hpp"
#include <iostream>
#include <cstdlib>

Memory::Memory(std::size_t size_bytes) : mem(size_bytes, 0) {}

// 讀一個 byte。若位址超出範圍就報錯中止（新手階段這樣最好抓 bug）。
uint8_t Memory::load8(uint32_t addr) const {
    if (addr >= mem.size()) {
        std::cerr << "[Memory] load 位址越界: 0x" << std::hex << addr << "\n";
        std::exit(1);
    }
    return mem[addr];
}

// 讀 2 個 byte 組成一個 16-bit 值（little-endian）
uint16_t Memory::load16(uint32_t addr) const {
    return (uint16_t)((uint32_t)load8(addr)
        | ((uint32_t)load8(addr + 1) << 8));
}

// 讀 4 個 byte 組成一個 32-bit 值（little-endian：低位址是最低位元組）
uint32_t Memory::load32(uint32_t addr) const {
    return  (uint32_t)load8(addr)
         | ((uint32_t)load8(addr + 1) << 8)
         | ((uint32_t)load8(addr + 2) << 16)
         | ((uint32_t)load8(addr + 3) << 24);
}

void Memory::store8(uint32_t addr, uint8_t val) {
    if (addr >= mem.size()) {
        std::cerr << "[Memory] store 位址越界: 0x" << std::hex << addr << "\n";
        std::exit(1);
    }
    mem[addr] = val;
}

// 把 16-bit 值拆成 2 個 byte 依序寫入（little-endian）
void Memory::store16(uint32_t addr, uint16_t val) {
    store8(addr,     (uint8_t)(val & 0xff));
    store8(addr + 1, (uint8_t)((val >> 8) & 0xff));
}

// 把 32-bit 值拆成 4 個 byte 依序寫入（little-endian）
void Memory::store32(uint32_t addr, uint32_t val) {
    store8(addr,     (uint8_t)(val & 0xff));
    store8(addr + 1, (uint8_t)((val >> 8) & 0xff));
    store8(addr + 2, (uint8_t)((val >> 16) & 0xff));
    store8(addr + 3, (uint8_t)((val >> 24) & 0xff));
}

void Memory::load_program(const std::vector<uint32_t>& words, uint32_t start_addr) {
    uint32_t addr = start_addr;
    for (uint32_t w : words) {
        store32(addr, w);
        addr += 4;
    }
}
