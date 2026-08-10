#include <cstdint>
#include <vector>
#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <map>
#include <string>
#include "memory.hpp"
#include "cpu.hpp"
#include "gemm_scalar.hpp"

// 把矩陣寫進模擬器的記憶體
static void put_matrix(Memory& mem, uint32_t base, const std::vector<int32_t>& m) {
    for (std::size_t idx = 0; idx < m.size(); ++idx)
        mem.store32(base + (uint32_t)idx * 4, (uint32_t)m[idx]);
}

// 從模擬器記憶體讀回矩陣
static std::vector<int32_t> get_matrix(const Memory& mem, uint32_t base, uint32_t n) {
    std::vector<int32_t> out(n * n);
    for (uint32_t idx = 0; idx < n * n; ++idx)
        out[idx] = (int32_t)mem.load32(base + idx * 4);
    return out;
}

// 用 C++ 直接算一次，當作「標準答案」
static std::vector<int32_t> ref_gemm(const std::vector<int32_t>& A,
                                     const std::vector<int32_t>& B, uint32_t n) {
    std::vector<int32_t> C(n * n, 0);
    for (uint32_t i = 0; i < n; ++i)
        for (uint32_t j = 0; j < n; ++j) {
            int32_t sum = 0;
            for (uint32_t k = 0; k < n; ++k) sum += A[i*n+k] * B[k*n+j];
            C[i*n+j] = sum;
        }
    return C;
}

static void print_matrix(const char* name, const std::vector<int32_t>& m, uint32_t n) {
    std::cout << name << " =\n";
    for (uint32_t i = 0; i < n; ++i) {
        std::cout << "  ";
        for (uint32_t j = 0; j < n; ++j) std::cout << std::setw(7) << m[i*n+j];
        std::cout << "\n";
    }
}

int main(int argc, char** argv) {
    const uint32_t N = (argc > 1) ? (uint32_t)std::atoi(argv[1]) : 4;

    // 準備測試資料：A[i][j] = i+j+1，B 是「轉置遞增」，隨便但不對稱，比較容易抓出索引寫反的 bug
    std::vector<int32_t> A(N*N), B(N*N);
    for (uint32_t i = 0; i < N; ++i)
        for (uint32_t j = 0; j < N; ++j) {
            A[i*N+j] = (int32_t)(i + j + 1);
            B[i*N+j] = (int32_t)(i * N + j) - 5;   // 含負數，順便驗證有號乘法
        }

    Memory mem(64 * 1024);
    auto kern = gemm::scalar(N);
    mem.load_program(kern.code, 0);
    put_matrix(mem, gemm::A_BASE, A);
    put_matrix(mem, gemm::B_BASE, B);

    CPU cpu(mem);
    cpu.run();

    auto C_sim = get_matrix(mem, gemm::C_BASE, N);
    auto C_ref = ref_gemm(A, B, N);

    print_matrix("A", A, N);
    print_matrix("B", B, N);
    print_matrix("C（模擬器算的）", C_sim, N);

    bool ok = (C_sim == C_ref);
    std::cout << "\n正確性：" << (ok ? "PASS（與 C++ 標準答案一致）" : "FAIL") << "\n";
    if (!ok) print_matrix("C（標準答案）", C_ref, N);

    // ---------------- 效能統計 ----------------
    const auto& s = cpu.stats();
    std::cout << "\n---- 效能基準線 ----\n";
    std::cout << "矩陣大小       : " << N << " x " << N << "\n";
    std::cout << "程式碼長度     : " << kern.code.size() << " 條指令（靜態）\n";
    std::cout << "實際執行指令數 : " << s.total << " 條（動態）\n";
    std::cout << "估計 cycle 數  : " << s.cycles << "\n";
    std::cout << "每個輸出元素   : " << (double)s.total / (N*N) << " 條指令\n";

    // ---------------- 依「指令種類」分類 ----------------
    auto pct = [&](uint64_t v) { return 100.0 * (double)v / (double)s.total; };
    std::cout << "\n---- 指令種類分布 ----\n" << std::fixed << std::setprecision(1);
    std::cout << "  算術/邏輯 : " << std::setw(8) << s.alu    << "  (" << pct(s.alu)    << "%)\n";
    std::cout << "  乘除法    : " << std::setw(8) << s.muldiv << "  (" << pct(s.muldiv) << "%)\n";
    std::cout << "  記憶體    : " << std::setw(8) << s.mem    << "  (" << pct(s.mem)    << "%)\n";
    std::cout << "  分支跳躍  : " << std::setw(8) << s.ctrl   << "  (" << pct(s.ctrl)   << "%)\n";
    std::cout << "  其他      : " << std::setw(8) << (s.upper + s.sys) << "  ("
              << pct(s.upper + s.sys) << "%)\n";

    // ---------------- 依「指令用途」分類（重點）----------------
    // 把每個位址的執行次數，依照產生時標記的 role 加總起來
    std::map<std::string, uint64_t> by_role;
    const auto& hits = cpu.pc_hits();
    for (std::size_t idx = 0; idx < kern.roles.size() && idx < hits.size(); ++idx)
        by_role[kern.roles[idx]] += hits[idx];

    const char* order[] = { "math", "mem", "addr", "loop", "setup" };
    const char* zh[]    = { "真正的運算  ", "記憶體存取  ", "算記憶體位址", "迴圈控制    ", "初始化      " };
    std::cout << "\n---- 指令用途分布（自訂指令要攻擊的目標）----\n";
    for (int t = 0; t < 5; ++t) {
        uint64_t v = by_role[order[t]];
        std::cout << "  " << zh[t] << " : " << std::setw(8) << v
                  << "  (" << pct(v) << "%)\n";
    }
    return ok ? 0 : 1;
}
