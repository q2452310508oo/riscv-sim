# 最低需要的 CMake 版本
cmake_minimum_required(VERSION 3.10)

# 專案名稱與使用的語言
project(riscv_sim CXX)

# 使用 C++17，且強制要求
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 開啟常見警告，養成好習慣、及早抓 bug
if(NOT MSVC)
    add_compile_options(-Wall -Wextra)
endif()

# 共用的模組原始碼（核心邏輯，不含任何 main）
set(CORE_SOURCES
    src/cpu.cpp
    src/memory.cpp
    src/registers.cpp
    src/decoder.cpp
)

# ---- 三個執行檔，全都共用 CORE_SOURCES ----

# 示範程式：計算 1+2+3+4+5
add_executable(riscv-sim src/main.cpp ${CORE_SOURCES})
target_include_directories(riscv-sim PRIVATE src)

# GEMM：矩陣乘法 + 效能統計（純量基準線）
add_executable(gemm src/main_gemm.cpp ${CORE_SOURCES})
target_include_directories(gemm PRIVATE src)

# 全指令自動測試（RV32IM）
add_executable(test_all tests/test_all.cpp ${CORE_SOURCES})
target_include_directories(test_all PRIVATE src)

# RVV 向量指令測試
add_executable(test_rvv tests/test_rvv.cpp ${CORE_SOURCES})
target_include_directories(test_rvv PRIVATE src)

# 讓 ctest 能跑測試
enable_testing()
add_test(NAME rv32im_all_instructions COMMAND test_all)
add_test(NAME gemm_correctness        COMMAND gemm 4)
add_test(NAME rvv_vsetvli              COMMAND test_rvv)
