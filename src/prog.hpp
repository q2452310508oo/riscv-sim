#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <cstdlib>
#include "assembler.hpp"

// ============================================================
// Prog：帶「標籤(label)」的程式產生器。
// 手算 branch 的跳躍偏移量非常容易錯，所以改成：
//   1. 用 label("L_k") 標記位置
//   2. 用 branch(...,"L_k") 先放一個佔位指令，記下待補清單
//   3. build() 時再統一算出偏移量、把指令補完（這叫 fixup / backpatch）
// 真正的組譯器也是這樣做的（two-pass assembler）。
// ============================================================
class Prog {
public:
    // role：這條指令「為什麼存在」，用來做指令組成分析
    //   "math" 真正的運算  "addr" 算記憶體位址  "mem" 存取記憶體
    //   "loop" 迴圈控制    "setup" 初始化
    void emit(uint32_t w, const char* role = "other") {
        code.push_back(w);
        roles_.push_back(role);
    }

    // 在「目前位置」標記一個標籤
    void label(const std::string& name) {
        if (labels.count(name)) { std::cerr << "[Prog] 標籤重複: " << name << "\n"; std::exit(1); }
        labels[name] = (uint32_t)code.size() * 4;
    }

    // 發出一條分支指令，目標是某個標籤（標籤可以還沒定義）
    void branch(uint32_t f3, uint32_t rs1, uint32_t rs2, const std::string& target,
                const char* role = "loop") {
        fixups.push_back({ code.size(), target, f3, rs1, rs2 });
        code.push_back(0);  // 佔位，稍後補
        roles_.push_back(role);
    }

    // 把所有待補的分支填上正確偏移量，回傳最終機器碼
    std::vector<uint32_t> build() {
        for (const auto& f : fixups) {
            auto it = labels.find(f.target);
            if (it == labels.end()) {
                std::cerr << "[Prog] 找不到標籤: " << f.target << "\n"; std::exit(1);
            }
            // B-type 偏移量是「目標位址 - 這條分支自己的位址」
            int32_t off = (int32_t)it->second - (int32_t)(f.idx * 4);
            if (off < -4096 || off > 4094) {
                std::cerr << "[Prog] 分支距離超過 B-type 範圍: " << off << "\n"; std::exit(1);
            }
            code[f.idx] = asm_::b(asm_::BRANCH, f.f3, f.rs1, f.rs2, off);
        }
        return code;
    }

    std::size_t size() const { return code.size(); }
    const std::vector<std::string>& roles() const { return roles_; }

private:
    struct Fixup { std::size_t idx; std::string target; uint32_t f3, rs1, rs2; };
    std::vector<uint32_t>            code;
    std::map<std::string, uint32_t>  labels;
    std::vector<Fixup>               fixups;
    std::vector<std::string>         roles_;
};
