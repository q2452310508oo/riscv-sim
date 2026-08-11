#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include "prog.hpp"
#include "assembler.hpp"
#include "gemm_scalar.hpp"

// ============================================================
// 自訂指令版 GEMM：在向量版基礎上，用自訂指令消除位址計算。
//
// 核心想法：內層 k 迴圈中，A、B 的位址每輪只是「往固定方向前進」，
// 不需要每次從 i,j,k 重算。改用「指標 + post-increment」：
//   - A 指標每輪 += 4（下一個元素）        → laddi.pi
//   - B 指標每輪 += N*4（下一列）           → vlmacc.pi（含載入+乘加+遞增）
//
// 對比向量版內層 13 條 → 自訂版內層剩 4 條。
//
// 暫存器：
//   x1=i x2=j x5=N  x3=k(計數)
//   x20=A指標 x21=B指標
//   x7=A純量  v3=累加器
// ============================================================
namespace gemm {

inline Kernel custom(uint32_t N) {
    using namespace asm_;
    Prog p;
    uint32_t VL = GEMM_VLEN / 32;

    p.emit(u(LUI, 10, A_BASE >> 12), "setup");
    p.emit(u(LUI, 11, B_BASE >> 12), "setup");
    p.emit(u(LUI, 12, C_BASE >> 12), "setup");
    p.emit(addi(5, 0, (int32_t)N), "setup");
    p.emit(addi(9, 0, (int32_t)VL), "setup");
    p.emit(vsetvli(0, 9, VSEW_32|VLMUL_1), "setup");
    // 設定 B 的跨距 = N*4（一整列的 byte 數）
    p.emit(addi(9, 0, (int32_t)(N*4)), "setup"); // x9 = N*4
    p.emit(setstride(9), "setup");               // stride = N*4

    p.emit(addi(1, 0, 0), "setup");              // i = 0

p.label("L_i");
    p.emit(addi(2, 0, 0), "loop");               // j = 0

p.label("L_j");
    p.emit(vsub_vv(3, 3, 3), "math");            // vacc = 0
    // A 指標 = A基底 + i*N*4（這一列開頭）
    p.emit(mul(20, 1, 5), "addr");               // x20 = i*N
    p.emit(i(OP_IMM, 0x1, 20, 20, 2), "addr");   // ×4
    p.emit(add(20, 20, 10), "addr");             // + A基底
    // B 指標 = B基底 + j*4（第 j 行開頭，第 0 列）
    p.emit(i(OP_IMM, 0x1, 21, 2, 2), "addr");    // x21 = j*4
    p.emit(add(21, 21, 11), "addr");             // + B基底
    p.emit(addi(3, 0, 0), "loop");               // k = 0

p.label("L_k");
    p.emit(laddi_pi(7, 20), "mem");              // x7 = A[i][k]，x20 += 4
    p.emit(vlmacc_pi(3, 21, 7), "math");         // vacc += x7 * B列，x21 += N*4
    p.emit(addi(3, 3, 1), "loop");               // k++
    p.branch(0x1, 3, 5, "L_k");                  // bne k, N, L_k

    // C[i][j..] = vacc
    p.emit(mul(6, 1, 5), "addr");
    p.emit(add(6, 6, 2), "addr");
    p.emit(i(OP_IMM, 0x1, 6, 6, 2), "addr");
    p.emit(add(6, 6, 12), "addr");
    p.emit(vse32(3, 6), "mem");

    p.emit(addi(2, 2, (int32_t)VL), "loop");     // j += VL
    p.branch(0x1, 2, 5, "L_j");

    p.emit(addi(1, 1, 1), "loop");               // i++
    p.branch(0x1, 1, 5, "L_i");

    p.emit(ecall(), "setup");
    Kernel kern;
    kern.code  = p.build();
    kern.roles = p.roles();
    return kern;
}

} // namespace gemm
