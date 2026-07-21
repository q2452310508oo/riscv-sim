# RISC-V 向量處理器模擬器（專題）

用 C++ 寫的 RISC-V 指令集模擬器（ISS）。
目前已**完整支援 RV32I 全部 47 條指令**，並附有自動測試逐條驗證。
下一步會加入 RVV 向量指令，再設計自訂指令加速矩陣乘法等運算，
最後比較純量版與向量版的效能差異。

## 檔案結構

```
riscv-sim/
├── CMakeLists.txt          # CMake 建置設定（含測試）
├── README.md
├── .gitignore
├── src/
│   ├── memory.hpp/.cpp     # 記憶體：byte 陣列，8/16/32-bit load/store
│   ├── registers.hpp/.cpp  # 32 個暫存器 x0~x31（x0 永遠為 0）
│   ├── decoder.hpp/.cpp    # 把 32-bit 指令拆成各欄位、算出立即值
│   ├── cpu.hpp/.cpp        # 核心：fetch→decode→execute + 全部指令 + CSR
│   ├── assembler.hpp       # 迷你組譯器：把指令組成機器碼（main 與測試共用）
│   └── main.cpp            # 進入點：示範程式（計算 1+2+3+4+5）
└── tests/
    └── test_all.cpp        # 自動測試：逐一驗證 47 條指令
```

依賴關係：`main / test → cpu → { decoder, registers, memory }`

## 如何編譯與執行

### 方法 A：直接用 g++（最簡單）

```bash
# 示範程式
g++ -std=c++17 -Wall -Wextra src/*.cpp -o riscv-sim
./riscv-sim

# 全指令測試
g++ -std=c++17 -Wall -Wextra -Isrc tests/test_all.cpp \
    src/cpu.cpp src/memory.cpp src/registers.cpp src/decoder.cpp -o test_all
./test_all
```

### 方法 B：用 CMake（檔案變多後建議）

```bash
cmake -B build
cmake --build build
./build/riscv-sim      # 示範程式
./build/test_all       # 全指令測試
ctest --test-dir build # 或用 ctest 跑測試
```

示範程式應印出 `x1 = 15`、`x2 = 6`、`x3 = 6`。
測試程式應印出「全部 47 條 RV32I 指令測試通過！」。

## 已支援的指令（完整 RV32I，共 47 條）

- 算術/邏輯（reg-reg，10）：add, sub, sll, slt, sltu, xor, srl, sra, or, and
- 立即值運算（9）：addi, slti, sltiu, xori, ori, andi, slli, srli, srai
- Load（5）：lb, lh, lw, lbu, lhu
- Store（3）：sb, sh, sw
- 分支（6）：beq, bne, blt, bge, bltu, bgeu
- 跳躍（2）：jal, jalr
- 高位立即值（2）：lui, auipc
- 記憶體屏障（2）：fence, fence.i（簡單模擬器中為空操作）
- 系統/CSR（8）：ecall, ebreak, csrrw, csrrs, csrrc, csrrwi, csrrsi, csrrci

> 註：ecall / ebreak 在本模擬器中都當作「停止執行」。
> CSR 指令有一組 4096 個的 CSR 儲存空間可讀寫。

## 下一步（開發路線）

1. 改成從檔案讀入真正的程式（用 riscv-gnu-toolchain 編譯出 binary）。
2. 拿 Spike（riscv-isa-sim）當「標準答案」對照暫存器結果。
   —— 測試程式只用「已實作的指令」，就不會跟 Spike 對不上。
3. 加入向量暫存器 v0~v31 與 vl/vtype 狀態（改 registers）。
4. 解碼 RVV 指令（opcode 0x57，改 decoder）。
5. 實作 RVV：vsetvli, vle32, vse32, vadd.vv, vmul.vv, vmacc（改 cpu）。
6. 選運算目標：向量點積 → 矩陣乘法（GEMM）。
7. 設計自訂指令（custom-0 opcode 空間）加速乘加。
8. 加上指令數/cycle 統計，比較純量 vs 向量 vs 自訂指令的效能。
