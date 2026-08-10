# RISC-V 向量處理器模擬器（專題）

用 C++ 寫的 RISC-V 指令集模擬器（ISS）。
目前已**完整支援 RV32IM（55 條指令）**，附自動測試逐條驗證，
並實作純量版 GEMM（矩陣乘法）作為效能基準線。
下一步加入 RVV 向量指令，再設計自訂指令加速矩陣運算，
最後比較「純量 vs 向量 vs 向量+自訂指令」的效能。

## 檔案結構

```
riscv-sim/
├── CMakeLists.txt          # 建置設定（riscv-sim / gemm / test_all）
├── README.md
├── .gitignore
├── src/
│   ├── memory.hpp/.cpp     # 記憶體：byte 陣列，8/16/32-bit load/store
│   ├── registers.hpp/.cpp  # 32 個暫存器 x0~x31（x0 永遠為 0）
│   ├── decoder.hpp/.cpp    # 把 32-bit 指令拆成各欄位、算出立即值
│   ├── cpu.hpp/.cpp        # 核心：fetch→decode→execute + 全部指令 + CSR + 效能統計
│   ├── assembler.hpp       # 迷你組譯器：把指令組成機器碼
│   ├── prog.hpp            # 帶標籤的程式產生器（自動回填分支偏移量）
│   ├── gemm_scalar.hpp     # 純量 GEMM 機器碼產生器
│   ├── main.cpp            # 示範程式（計算 1+2+3+4+5）
│   └── main_gemm.cpp       # GEMM 執行 + 正確性驗證 + 效能統計
└── tests/
    └── test_all.cpp        # 自動測試：逐一驗證 55 條 RV32IM 指令
```

## 如何編譯與執行

### 用 CMake（建議）

```bash
cmake -B build
cmake --build build
./build/riscv-sim      # 示範程式（x1=15, x2=6, x3=6）
./build/test_all       # 全指令測試（55/55）
./build/gemm 4         # GEMM 4x4 + 效能統計
ctest --test-dir build # 自動跑所有測試
```

### 用 g++（不裝 CMake 也行）

```bash
# 全指令測試
g++ -std=c++17 -Wall -Wextra -Isrc tests/test_all.cpp \
    src/cpu.cpp src/memory.cpp src/registers.cpp src/decoder.cpp -o test_all && ./test_all

# GEMM（可帶參數改矩陣大小：./gemm 8）
g++ -std=c++17 -Wall -Wextra -Isrc src/main_gemm.cpp \
    src/cpu.cpp src/memory.cpp src/registers.cpp src/decoder.cpp -o gemm && ./gemm 4
```

## 已支援指令（RV32IM，共 55 條）

- RV32I（47）：算術、邏輯、load/store、分支、跳躍、lui/auipc、fence、CSR、ecall/ebreak
- RV32M（8）：mul, mulh, mulhsu, mulhu, div, divu, rem, remu

## 效能基準線（純量 GEMM）

4x4 矩陣乘法：執行 1058 條指令，其中**約 54% 用於計算記憶體位址**，
真正的乘加運算僅約 12%。這正是向量化與自訂指令要攻擊的目標。

## 開發路線

1. [完成] RV32I 全指令 + 自動測試
2. [完成] RV32M 乘除法
3. [完成] 純量 GEMM + 效能統計（指令分類、cycle 模型）
4. [進行中] RVV 向量狀態（v0~v31, vl, vtype）+ vsetvli
5. RVV 資料搬移（vle32.v, vse32.v）與運算（vadd.vv, vmacc.vx）
6. 向量版 GEMM
7. 自訂指令（custom-0）加速：融合載入+乘加+位址遞增
8. 效能比較：純量 vs 向量 vs 向量+自訂指令；掃描 VLEN
