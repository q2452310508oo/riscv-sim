#!/bin/bash
# 掃描不同 VLEN，收集到 results_vlen.csv
# 只用 N >= VL 的組合（N 必須是 VL 的倍數）
SRCS="src/main_gemm.cpp src/cpu.cpp src/memory.cpp src/registers.cpp src/decoder.cpp"
OUT=results_vlen.csv
rm -f $OUT results.csv
echo "vlen,version,N,instret,cycles" > $OUT

for vlen in 128 256 512; do
    g++ -std=c++17 -Wall -Wextra -DVLEN_BITS=$vlen -Isrc $SRCS -o /tmp/gemm_vlen 2>/dev/null
    for v in vector custom; do
        for n in 16 32; do   # 用 16,32 確保 >= VL（最大 VL=16）
            out=$(/tmp/gemm_vlen $n $v 2>/dev/null)
            instr=$(echo "$out" | grep 實際執行 | grep -oE '[0-9]+' | head -1)
            cyc=$(echo "$out" | grep "估計 cycle" | grep -oE '[0-9]+' | head -1)
            echo "$vlen,$v,$n,$instr,$cyc" >> $OUT
        done
    done
done
rm -f results.csv  # 清掉掃描產生的
echo "=== $OUT ==="
cat $OUT
