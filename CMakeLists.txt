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

# 共用的模組原始碼（核心邏輯，不含 main）
set(CORE_SOURCES
    src/cpu.cpp
    src/memory.cpp
    src/registers.cpp
    src/decoder.cpp
)

# 主程式：示範程式
add_executable(riscv-sim src/main.cpp ${CORE_SOURCES})
target_include_directories(riscv-sim PRIVATE src)

# 測試程式：47 條指令自動測試
add_executable(test_all tests/test_all.cpp ${CORE_SOURCES})
target_include_directories(test_all PRIVATE src)

# 讓 `ctest` 或 `cmake --build build --target run_tests` 能跑測試
enable_testing()
add_test(NAME rv32i_all_instructions COMMAND test_all)
