#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include "prog.hpp"
#include "assembler.hpp"

// ============================================================
// 純量版 GEMM：C = A * B  （N x N 的 int32 矩陣）
//
// 對應的 C 語言邏輯：
//   for (i = 0; i < N; i++)
//     for (j = 0; j < N; j++) {
//       sum = 0;
//       for (k = 0; k < N; k++)
//         sum += A[i*N+k] * B[k*N+j];
//       C[i*N+j] = sum;
//     }
//
// 暫存器分配：
//   x1=i  x2=j  x3=k  x4=sum  x5=N  x6=位址暫存
//   x7=A元素  x8=B元素  x9=乘積
//   x10=A基底  x11=B基底  x12=C基底
// ============================================================
// GEMM 產生器用的 VLEN（要跟 CPU 的 VLEN 一致）。
// 掃描實驗時用 -DVLEN_BITS=256 同時覆蓋 CPU 和這裡。
#ifndef VLEN_BITS
#define VLEN_BITS 128
#endif
#ifndef GEMM_VLEN
#define GEMM_VLEN VLEN_BITS
#endif

namespace gemm {

constexpr uint32_t A_BASE = 0x1000;
constexpr uint32_t B_BASE = 0x2000;
constexpr uint32_t C_BASE = 0x3000;

struct Kernel {
    std::vector<uint32_t>    code;
    std::vector<std::string> roles;
};

inline Kernel scalar(uint32_t N) {
    using namespace asm_;
    Prog p;

    // ---- 初始化基底位址（lui 把值放進 bits[31:12]）----
    p.emit(u(LUI, 10, A_BASE >> 12), "setup");   // x10 = A
    p.emit(u(LUI, 11, B_BASE >> 12), "setup");   // x11 = B
    p.emit(u(LUI, 12, C_BASE >> 12), "setup");   // x12 = C
    p.emit(addi(5, 0, (int32_t)N), "setup");     // x5 = N
    p.emit(addi(1, 0, 0), "setup");              // i = 0

p.label("L_i");
    p.emit(addi(2, 0, 0), "loop");               // j = 0

p.label("L_j");
    p.emit(addi(4, 0, 0), "loop");               // sum = 0
    p.emit(addi(3, 0, 0), "loop");               // k = 0

p.label("L_k");
    // ---- 載入 A[i*N+k] ----
    p.emit(mul(6, 1, 5), "addr");                // x6 = i*N
    p.emit(add(6, 6, 3), "addr");                // x6 += k
    p.emit(i(OP_IMM, 0x1, 6, 6, 2), "addr");     // slli x6, x6, 2  （×4 換成 byte）
    p.emit(add(6, 6, 10), "addr");               // x6 += A基底
    p.emit(i(LOAD, 0x2, 7, 6, 0), "mem");        // lw x7, 0(x6)

    // ---- 載入 B[k*N+j] ----
    p.emit(mul(6, 3, 5), "addr");                // x6 = k*N
    p.emit(add(6, 6, 2), "addr");                // x6 += j
    p.emit(i(OP_IMM, 0x1, 6, 6, 2), "addr");     // slli x6, x6, 2
    p.emit(add(6, 6, 11), "addr");               // x6 += B基底
    p.emit(i(LOAD, 0x2, 8, 6, 0), "mem");        // lw x8, 0(x6)

    // ---- sum += A元素 * B元素 ----
    p.emit(mul(9, 7, 8), "math");                // x9 = x7 * x8
    p.emit(add(4, 4, 9), "math");                // sum += x9

    p.emit(addi(3, 3, 1), "loop");               // k++
    p.branch(0x1, 3, 5, "L_k");            // bne k, N, L_k

    // ---- C[i*N+j] = sum ----
    p.emit(mul(6, 1, 5), "addr");
    p.emit(add(6, 6, 2), "addr");
    p.emit(i(OP_IMM, 0x1, 6, 6, 2), "addr");
    p.emit(add(6, 6, 12), "addr");
    p.emit(s(STORE, 0x2, 6, 4, 0), "mem");        // sw x4, 0(x6)

    p.emit(addi(2, 2, 1), "loop");               // j++
    p.branch(0x1, 2, 5, "L_j");            // bne j, N, L_j

    p.emit(addi(1, 1, 1), "loop");               // i++
    p.branch(0x1, 1, 5, "L_i");            // bne i, N, L_i

    p.emit(ecall(), "setup");                    // 停止
    Kernel k;
    k.code  = p.build();
    k.roles = p.roles();
    return k;
}

} // namespace gemm
