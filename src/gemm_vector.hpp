#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include "prog.hpp"
#include "assembler.hpp"
#include "gemm_scalar.hpp"   // 借用 A_BASE / B_BASE / C_BASE 和 Kernel 定義

// ============================================================
// 向量版 GEMM：C = A * B（N x N，int32）
//
// 向量化策略（一次算 C 的一整排 VL 個元素）：
//   for (i = 0; i < N; i++)
//     for (j = 0; j < N; j += VL) {       // 一次跨 VL 行
//       vacc = 0;                          // 向量累加器（v3）
//       for (k = 0; k < N; k++) {
//         a = A[i*N+k];                    // 純量
//         vb = B[k*N + j .. j+VL-1];        // B 一列的連續 VL 個（vle32）
//         vacc += a * vb;                   // vmacc.vx，一次 VL 組乘加
//       }
//       C[i*N + j .. j+VL-1] = vacc;        // vse32 存回一整排
//     }
//
// 簡化假設：N 是 VL 的倍數（VL = VLEN/32 = 4）。測試用 N=4,8 剛好。
//
// 暫存器：
//   x1=i  x2=j  x3=k  x5=N
//   x6=位址暫存  x7=A純量元素  x8=B列位址
//   x10=A基底 x11=B基底 x12=C基底
//   v1=B一列  v3=向量累加器
// ============================================================
namespace gemm {

inline Kernel vector(uint32_t N) {
    using namespace asm_;
    Prog p;
    uint32_t VL = GEMM_VLEN / 32;   // 由 VLEN 決定，一次處理幾個 int32

    p.emit(u(LUI, 10, A_BASE >> 12), "setup");   // x10 = A
    p.emit(u(LUI, 11, B_BASE >> 12), "setup");   // x11 = B
    p.emit(u(LUI, 12, C_BASE >> 12), "setup");   // x12 = C
    p.emit(addi(5, 0, (int32_t)N), "setup");     // x5 = N
    // 設定向量長度：vl = VL（用 x0=不變技巧，這裡直接載入 VL 當 AVL）
    p.emit(addi(9, 0, (int32_t)VL), "setup");    // x9 = VL
    p.emit(vsetvli(0, 9, VSEW_32|VLMUL_1), "setup"); // vl = min(VL, VLMAX) = 4

    p.emit(addi(1, 0, 0), "setup");              // i = 0

p.label("L_i");
    p.emit(addi(2, 0, 0), "loop");               // j = 0

p.label("L_j");
    // vacc(v3) = 0：用 vsub.vv v3,v3,v3 歸零（自己減自己）
    p.emit(vsub_vv(3, 3, 3), "math");            // v3 = 0
    p.emit(addi(3, 0, 0), "loop");               // k = 0  （注意：這裡 x3 當 k）

p.label("L_k");
    // ---- 純量 a = A[i*N+k] ----
    p.emit(mul(6, 1, 5), "addr");                // x6 = i*N
    p.emit(add(6, 6, 3), "addr");                // x6 += k
    p.emit(i(OP_IMM, 0x1, 6, 6, 2), "addr");     // ×4
    p.emit(add(6, 6, 10), "addr");               // + A基底
    p.emit(i(LOAD, 0x2, 7, 6, 0), "mem");        // x7 = A[i*N+k]（純量）

    // ---- 向量 vb = B[k*N + j .. j+VL-1] ----
    p.emit(mul(8, 3, 5), "addr");                // x8 = k*N
    p.emit(add(8, 8, 2), "addr");                // x8 += j
    p.emit(i(OP_IMM, 0x1, 8, 8, 2), "addr");     // ×4
    p.emit(add(8, 8, 11), "addr");               // + B基底
    p.emit(vle32(1, 8), "mem");                  // v1 = B一列的 VL 個

    // ---- vacc += a * vb ----
    p.emit(vmacc_vx(3, 7, 1), "math");           // v3 += x7 * v1

    p.emit(addi(3, 3, 1), "loop");               // k++
    p.branch(0x1, 3, 5, "L_k");                  // bne k, N, L_k

    // ---- C[i*N + j .. j+VL-1] = vacc ----
    p.emit(mul(6, 1, 5), "addr");                // x6 = i*N
    p.emit(add(6, 6, 2), "addr");                // x6 += j
    p.emit(i(OP_IMM, 0x1, 6, 6, 2), "addr");     // ×4
    p.emit(add(6, 6, 12), "addr");               // + C基底
    p.emit(vse32(3, 6), "mem");                  // 存回一整排 VL 個

    p.emit(addi(2, 2, (int32_t)VL), "loop");     // j += VL
    p.branch(0x1, 2, 5, "L_j");                  // bne j, N, L_j

    p.emit(addi(1, 1, 1), "loop");               // i++
    p.branch(0x1, 1, 5, "L_i");                  // bne i, N, L_i

    p.emit(ecall(), "setup");
    Kernel kern;
    kern.code  = p.build();
    kern.roles = p.roles();
    return kern;
}

} // namespace gemm
