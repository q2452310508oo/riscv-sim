#include "cpu.hpp"
#include <iostream>

CPU::CPU(Memory& mem_)
    : mem(mem_), pc(0), halt(false) {}

// 抓一條指令 -> 解碼 -> 執行
void CPU::step() {
    uint32_t raw = mem.load32(pc);
    DecodedInst d = decode(raw);

    // 記錄這個位址被執行過幾次（熱點分析用）
    std::size_t slot = pc / 4;
    if (slot >= hits.size()) hits.resize(slot + 1, 0);
    hits[slot]++;

    count(d);      // 先統計（此時 pc 還沒被改掉）
    execute(d);
}

// ------------------------------------------------------------
// 指令分類 + cycle 模型。
//
// 【重要】這裡的 cycle 數是「假設值」，不是真實硬體量測。
// 寫報告時務必註明這是簡化模型。參數依據：
//   - 單週期 ALU：算術、邏輯、分支 = 1
//   - 乘法器需要多級：mul = 3
//   - 除法器很慢（逐位元運算）：div/rem = 20
//   - 記憶體存取（假設 cache 命中）：load/store = 2
// ------------------------------------------------------------
void CPU::count(const DecodedInst& d) {
    st.total++;
    switch (d.opcode) {
        case 0x13:                       // OP-IMM
            st.alu++;    st.cycles += 1; break;
        case 0x33:                       // OP（含 RV32M）
            if (d.funct7 == 0x01) {
                st.muldiv++;
                st.cycles += (d.funct3 >= 0x4) ? 20 : 3;   // div/rem 慢，mul 較快
            } else {
                st.alu++;  st.cycles += 1;
            }
            break;
        case 0x03: case 0x23:            // LOAD / STORE
            st.mem++;    st.cycles += 2; break;
        case 0x63: case 0x6f: case 0x67: // BRANCH / JAL / JALR
            st.ctrl++;   st.cycles += 1; break;
        case 0x37: case 0x17:            // LUI / AUIPC
            st.upper++;  st.cycles += 1; break;
        case 0x0f: case 0x73:            // FENCE / SYSTEM
            st.sys++;    st.cycles += 1; break;
        case 0x57:                       // RVV（W2 之後實作）
            st.vec++;    st.cycles += 2; break;
        case 0x0b:                       // custom-0（W6 之後實作）
            st.custom++; st.cycles += 2; break;
        default:
            st.cycles += 1; break;
    }
}

void CPU::run() {
    while (!halt) {
        step();
        // 安全閥：避免無窮迴圈把程式卡死
        if (st.total > 10'000'000) {
            std::cerr << "[CPU] 指令數過多，強制停止（可能是無窮迴圈）\n";
            break;
        }
    }
}

// ------------------------------------------------------------
// 執行一條指令。
// 這個大 switch 就是「這條指令到底要做什麼」的核心。
// 目前實作了 RV32I 常用的整數指令，足以跑迴圈、算術、load/store。
// 之後要擴充 RVV / 自訂指令，就在這裡新增 case。
// ------------------------------------------------------------
void CPU::execute(const DecodedInst& d) {
    uint32_t next_pc = pc + 4;  // 預設下一條就是下一個位址

    switch (d.opcode) {
        // ---- OP-IMM：暫存器與立即值運算 ----
        case 0x13: {
            uint32_t a = reg.read(d.rs1);
            uint32_t res = 0;
            switch (d.funct3) {
                case 0x0: res = a + (uint32_t)d.imm; break;                      // addi
                case 0x2: res = ((int32_t)a < d.imm) ? 1 : 0; break;             // slti
                case 0x3: res = (a < (uint32_t)d.imm) ? 1 : 0; break;            // sltiu
                case 0x4: res = a ^ (uint32_t)d.imm; break;                      // xori
                case 0x6: res = a | (uint32_t)d.imm; break;                      // ori
                case 0x7: res = a & (uint32_t)d.imm; break;                      // andi
                case 0x1: res = a << (d.imm & 0x1f); break;                      // slli
                case 0x5:
                    res = (d.funct7 == 0x20) ? ((int32_t)a >> (d.imm & 0x1f))    // srai
                                             : (a >> (d.imm & 0x1f));            // srli
                    break;
            }
            reg.write(d.rd, res);
            break;
        }

        // ---- OP：暫存器與暫存器運算 ----
        case 0x33: {
            uint32_t a = reg.read(d.rs1);
            uint32_t b = reg.read(d.rs2);
        // ---- RV32M：乘除法（funct7 = 0x01）----
        if (d.funct7 == 0x01) {
            uint32_t res = 0;
            int32_t sa = (int32_t)a, sb = (int32_t)b;
            switch (d.funct3) {
                case 0x0: res = a * b; break;                                                  // mul
                case 0x1: res = (uint32_t)(((int64_t)sa * (int64_t)sb) >> 32); break;          // mulh
                case 0x2: res = (uint32_t)(((int64_t)sa * (int64_t)(uint64_t)b) >> 32); break; // mulhsu
                case 0x3: res = (uint32_t)(((uint64_t)a * (uint64_t)b) >> 32); break;          // mulhu
                case 0x4: res = (sb == 0) ? 0xFFFFFFFFu                                        // div
                              : (sa == INT32_MIN && sb == -1) ? (uint32_t)sa
                              : (uint32_t)(sa / sb); break;
                case 0x5: res = (b == 0) ? 0xFFFFFFFFu : (a / b); break;                       // divu
                case 0x6: res = (sb == 0) ? a                                                  // rem
                              : (sa == INT32_MIN && sb == -1) ? 0
                              : (uint32_t)(sa % sb); break;
                case 0x7: res = (b == 0) ? a : (a % b); break;                                 // remu
            }
            reg.write(d.rd, res);
            break;
        }

            // ---- 基本 R-type：add / sub / sll / slt / ... ----
            uint32_t res = 0;
            switch (d.funct3) {
                case 0x0: res = (d.funct7 == 0x20) ? (a - b) : (a + b); break;   // sub / add
                case 0x1: res = a << (b & 0x1f); break;                          // sll
                case 0x2: res = ((int32_t)a < (int32_t)b) ? 1 : 0; break;        // slt
                case 0x3: res = (a < b) ? 1 : 0; break;                          // sltu
                case 0x4: res = a ^ b; break;                                    // xor
                case 0x5:
                    res = (d.funct7 == 0x20) ? ((int32_t)a >> (b & 0x1f))        // sra
                                             : (a >> (b & 0x1f));                // srl
                    break;
                case 0x6: res = a | b; break;                                    // or
                case 0x7: res = a & b; break;                                    // and
            }
            reg.write(d.rd, res);
            break;
        }

        // ---- LOAD：lb / lh / lw / lbu / lhu ----
        case 0x03: {
            uint32_t addr = reg.read(d.rs1) + (uint32_t)d.imm;
            uint32_t res = 0;
            switch (d.funct3) {
                case 0x0: res = (uint32_t)(int32_t)(int8_t)mem.load8(addr);   break; // lb  （符號延伸）
                case 0x1: res = (uint32_t)(int32_t)(int16_t)mem.load16(addr); break; // lh  （符號延伸）
                case 0x2: res = mem.load32(addr);                            break; // lw
                case 0x4: res = (uint32_t)mem.load8(addr);                   break; // lbu （零延伸）
                case 0x5: res = (uint32_t)mem.load16(addr);                  break; // lhu （零延伸）
                default:
                    std::cerr << "[CPU] 未知的 LOAD funct3=" << d.funct3 << "\n";
                    halt = true;
                    break;
            }
            reg.write(d.rd, res);
            break;
        }

        // ---- STORE：sb / sh / sw ----
        case 0x23: {
            uint32_t addr = reg.read(d.rs1) + (uint32_t)d.imm;
            uint32_t val  = reg.read(d.rs2);
            switch (d.funct3) {
                case 0x0: mem.store8(addr,  (uint8_t)(val & 0xff));    break; // sb
                case 0x1: mem.store16(addr, (uint16_t)(val & 0xffff)); break; // sh
                case 0x2: mem.store32(addr, val);                      break; // sw
                default:
                    std::cerr << "[CPU] 未知的 STORE funct3=" << d.funct3 << "\n";
                    halt = true;
                    break;
            }
            break;
        }

        // ---- BRANCH：條件分支 ----
        case 0x63: {
            uint32_t a = reg.read(d.rs1);
            uint32_t b = reg.read(d.rs2);
            bool take = false;
            switch (d.funct3) {
                case 0x0: take = (a == b); break;                               // beq
                case 0x1: take = (a != b); break;                               // bne
                case 0x4: take = ((int32_t)a <  (int32_t)b); break;             // blt
                case 0x5: take = ((int32_t)a >= (int32_t)b); break;             // bge
                case 0x6: take = (a <  b); break;                               // bltu
                case 0x7: take = (a >= b); break;                               // bgeu
            }
            if (take) next_pc = pc + (uint32_t)d.imm;
            break;
        }

        // ---- JAL：跳躍並連結 ----
        case 0x6f: {
            reg.write(d.rd, pc + 4);       // 把返回位址存進 rd
            next_pc = pc + (uint32_t)d.imm;
            break;
        }

        // ---- JALR：暫存器跳躍 ----
        case 0x67: {
            uint32_t ret = pc + 4;
            next_pc = (reg.read(d.rs1) + (uint32_t)d.imm) & ~1u;
            reg.write(d.rd, ret);
            break;
        }

        // ---- LUI ----
        case 0x37:
            reg.write(d.rd, (uint32_t)d.imm);
            break;

        // ---- AUIPC ----
        case 0x17:
            reg.write(d.rd, pc + (uint32_t)d.imm);
            break;

        // ---- FENCE / FENCE.I：記憶體與指令快取排序。
        //      單核簡單模擬器中不需真的做事，當空操作。 ----
        case 0x0f:
            // funct3==0 是 fence，funct3==1 是 fence.i，兩者都當 no-op
            break;

        // ---- SYSTEM：ecall / ebreak / CSR 指令 ----
        case 0x73: {
            uint32_t csr_addr = (d.raw >> 20) & 0xfff; // CSR 位址（12-bit）
            uint32_t zimm     = d.rs1;                  // 立即值版用 rs1 欄位當 5-bit 立即值
            switch (d.funct3) {
                case 0x0: {
                    // funct12: 0x000 = ecall，0x001 = ebreak。兩者在此都當「停止」。
                    halt = true;
                    break;
                }
                case 0x1: { // csrrw：交換
                    uint32_t t = csr[csr_addr];
                    csr[csr_addr] = reg.read(d.rs1);
                    reg.write(d.rd, t);
                    break;
                }
                case 0x2: { // csrrs：設定指定位元
                    uint32_t t = csr[csr_addr];
                    if (d.rs1 != 0) csr[csr_addr] = t | reg.read(d.rs1);
                    reg.write(d.rd, t);
                    break;
                }
                case 0x3: { // csrrc：清除指定位元
                    uint32_t t = csr[csr_addr];
                    if (d.rs1 != 0) csr[csr_addr] = t & ~reg.read(d.rs1);
                    reg.write(d.rd, t);
                    break;
                }
                case 0x5: { // csrrwi：立即值交換
                    uint32_t t = csr[csr_addr];
                    csr[csr_addr] = zimm;
                    reg.write(d.rd, t);
                    break;
                }
                case 0x6: { // csrrsi：立即值設定位元
                    uint32_t t = csr[csr_addr];
                    if (zimm != 0) csr[csr_addr] = t | zimm;
                    reg.write(d.rd, t);
                    break;
                }
                case 0x7: { // csrrci：立即值清除位元
                    uint32_t t = csr[csr_addr];
                    if (zimm != 0) csr[csr_addr] = t & ~zimm;
                    reg.write(d.rd, t);
                    break;
                }
                default:
                    std::cerr << "[CPU] 未知的 SYSTEM funct3=" << d.funct3 << "\n";
                    halt = true;
                    break;
            }
            break;
        }

        default:
            std::cerr << "[CPU] 未知的 opcode 0x" << std::hex << d.opcode
                      << " 於 pc=0x" << pc << std::dec << "\n";
            halt = true;
            break;
    }

    pc = next_pc;
}
