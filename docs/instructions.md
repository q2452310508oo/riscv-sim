# 模擬器支援指令總表

本模擬器共支援 **55 條 RV32IM + 6 條 RVV + 3 條自訂 = 64 條指令**。

---

## 一、RV32I 基礎整數指令（47 條）

### 整數運算（暫存器 + 立即值，OP-IMM，opcode 0x13）
| 指令 | 動作 | 說明 |
|---|---|---|
| addi  | rd = rs1 + imm | 加立即值 |
| slti  | rd = (rs1 < imm) ? 1:0 | 有號比較 |
| sltiu | rd = (rs1 < imm) ? 1:0 | 無號比較 |
| xori  | rd = rs1 ^ imm | 互斥或 |
| ori   | rd = rs1 \| imm | 或 |
| andi  | rd = rs1 & imm | 且 |
| slli  | rd = rs1 << imm | 邏輯左移 |
| srli  | rd = rs1 >> imm | 邏輯右移 |
| srai  | rd = rs1 >> imm | 算術右移（保留符號） |

### 整數運算（暫存器 + 暫存器，OP，opcode 0x33）
| 指令 | 動作 |
|---|---|
| add / sub | 加 / 減 |
| sll | 邏輯左移 |
| slt / sltu | 有號 / 無號比較 |
| xor / or / and | 位元運算 |
| srl / sra | 邏輯 / 算術右移 |

### 載入（LOAD，opcode 0x03）
| 指令 | 動作 |
|---|---|
| lb  | 載入 1 byte（符號延伸） |
| lh  | 載入 2 bytes（符號延伸） |
| lw  | 載入 4 bytes |
| lbu | 載入 1 byte（零延伸） |
| lhu | 載入 2 bytes（零延伸） |

### 儲存（STORE，opcode 0x23）
| 指令 | 動作 |
|---|---|
| sb / sh / sw | 儲存 1 / 2 / 4 bytes |

### 分支（BRANCH，opcode 0x63）
| 指令 | 條件 |
|---|---|
| beq / bne | 相等 / 不相等 |
| blt / bge | 有號 小於 / 大於等於 |
| bltu / bgeu | 無號 小於 / 大於等於 |

### 跳躍
| 指令 | opcode | 動作 |
|---|---|---|
| jal  | 0x6f | 跳躍並把返回位址存進 rd |
| jalr | 0x67 | 用暫存器值跳躍 |

### 高位立即值
| 指令 | opcode | 動作 |
|---|---|---|
| lui   | 0x37 | 把 20-bit 放進 rd 的高位 |
| auipc | 0x17 | PC + 高位立即值 |

### 系統與其他（SYSTEM，opcode 0x73）
| 指令 | 動作 |
|---|---|
| ecall / ebreak | 停止模擬 |
| csrrw / csrrs / csrrc | CSR 讀寫 / 設位 / 清位 |
| csrrwi / csrrsi / csrrci | CSR 立即值版本 |

### 記憶體排序（FENCE，opcode 0x0f）
| 指令 | 動作 |
|---|---|
| fence / fence.i | 單核模擬器中當 no-op |

---

## 二、RV32M 乘除法擴展（8 條，opcode 0x33 + funct7=0x01）

| 指令 | 動作 | 特例 |
|---|---|---|
| mul    | 低 32 位乘積 | |
| mulh   | 有號 × 有號，取高 32 位 | |
| mulhsu | 有號 × 無號，取高 32 位 | |
| mulhu  | 無號 × 無號，取高 32 位 | |
| div    | 有號除法 | 除以 0 → 全 1；INT_MIN/-1 → INT_MIN |
| divu   | 無號除法 | 除以 0 → 全 1 |
| rem    | 有號餘數 | 除以 0 → 回傳被除數 |
| remu   | 無號餘數 | 除以 0 → 回傳被除數 |

---

## 三、RVV 向量擴展（6 條，opcode 0x57）

| 指令 | funct3 | 動作 | cycle |
|---|---|---|---|
| vsetvli | 0x7 | 設定 vl / vtype，回傳這輪能處理幾個元素 | 1 |
| vle32.v | 0x6 | 從記憶體連續載入 vl 個 32-bit 到向量暫存器 | 3 |
| vse32.v | 0x5 | 把向量暫存器 vl 個元素連續存回記憶體 | 3 |
| vadd.vv | 0x0 (funct6=0x00) | 逐 lane 向量加法 | 2 |
| vsub.vv | 0x0 (funct6=0x02) | 逐 lane 向量減法（也用來歸零累加器） | 2 |
| vmacc.vx | 0x4 (funct6=0x2d) | vd[e] += 純量 × vs2[e]，逐 lane 乘加 | 4 |

**命名慣例**：`.vv` = 向量×向量，`.vx` = 向量×純量，`.vi` = 向量×立即值。

---

## 四、自訂指令 custom-0（3 條，opcode 0x0b）—— 本專題原創

| 指令 | funct3 | 動作 | cycle |
|---|---|---|---|
| setstride | 0x0 | 設定 post-increment 跨距 = x[rs1]（byte） | 1 |
| vlmacc.pi | 0x1 | 融合：載入向量 + 乘加 + 指標 += 跨距 | 5 |
| laddi.pi  | 0x2 | 融合：載入純量 + 指標 += 4 | 2 |

**設計目標**：攻擊 RVV 沒有解決的「記憶體位址計算」開銷。
- vlmacc.pi = vle32.v + vmacc.vx + 位址遞增（處理矩陣 B，每輪跨一列）
- laddi.pi  = lw + 位址遞增（處理矩陣 A，每輪移一格）

**誠實計價原則**：融合只省「指令發出 + 位址計算」開銷，不省「運算本身」。
vlmacc.pi 記 5 cycle（= 載入3 + 乘加4 - 省下2），而非 2。

---

## cycle 模型總表（簡化模型，非實測）

| 類型 | cycle | | 類型 | cycle |
|---|---|---|---|---|
| 算術/邏輯/分支/lui | 1 | | RVV 載入/儲存 | 3 |
| mul | 3 | | RVV 加減 | 2 |
| div/rem | 20 | | vmacc.vx | 4 |
| load/store | 2 | | vlmacc.pi | 5 |
| vsetvli | 1 | | laddi.pi | 2 |

> 三個 GEMM 版本（scalar / vector / custom）使用完全相同的參數，確保比較公平。
