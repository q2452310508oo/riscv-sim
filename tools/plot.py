#!/usr/bin/env python3
# ============================================================
# 讀 results.csv，畫出報告用的圖表。
# 用法：python3 tools/plot.py      產出：docs/fig_*.png
# ============================================================
import csv, os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

rows = list(csv.DictReader(open("results.csv")))
data = {}
for r in rows:
    v = r["version"]; n = int(r["N"])
    data.setdefault(v, {})[n] = {
        "instret": int(r["instret"]), "cycles": int(r["cycles"]),
        "math": float(r["math_pct"]), "mem": float(r["mem_pct"]),
        "addr": float(r["addr_pct"]), "loop": float(r["loop_pct"]),
    }

os.makedirs("docs", exist_ok=True)
versions = [v for v in ["scalar","vector","custom"] if v in data]
Ns = sorted(data[versions[0]].keys())
label_zh = {"scalar":"純量 (scalar)","vector":"向量 (vector)","custom":"自訂 (custom)"}
colors = {"scalar":"#888780","vector":"#1D9E75","custom":"#534AB7"}

zh_ok = False
for f in ["Noto Sans CJK TC","Noto Sans CJK SC","WenQuanYi Zen Hei","SimHei","Droid Sans Fallback"]:
    try:
        matplotlib.font_manager.findfont(f, fallback_to_default=False)
        plt.rcParams["font.sans-serif"] = [f]; zh_ok = True; break
    except Exception: pass
T = (lambda zh,en: zh if zh_ok else en)

# 圖1：指令數 vs N
plt.figure(figsize=(7,5))
for v in versions:
    ys = [data[v][n]["instret"] for n in Ns]
    plt.plot(Ns, ys, "o-", color=colors[v], label=label_zh[v] if zh_ok else v)
plt.xlabel("N " + T("（矩陣邊長）","(matrix size)"))
plt.ylabel(T("動態指令數","Dynamic instruction count"))
plt.title(T("指令數 vs 矩陣大小","Instructions vs matrix size"))
plt.yscale("log"); plt.xscale("log", base=2)
plt.xticks(Ns,[str(n) for n in Ns]); plt.grid(True,which="both",alpha=0.3); plt.legend()
plt.tight_layout(); plt.savefig("docs/fig1_instret.png", dpi=150); plt.close()

# 圖2：加速比
if "vector" in data:
    plt.figure(figsize=(7,5))
    x = np.arange(len(Ns)); w = 0.35
    inst_sp = [data["scalar"][n]["instret"]/data["vector"][n]["instret"] for n in Ns]
    cyc_sp  = [data["scalar"][n]["cycles"]/data["vector"][n]["cycles"] for n in Ns]
    plt.bar(x-w/2, inst_sp, w, color="#85B7EB", label=T("指令數加速","Instr speedup"))
    plt.bar(x+w/2, cyc_sp, w, color="#5DCAA5", label=T("cycle 加速","Cycle speedup"))
    plt.axhline(1, color="#888780", ls="--", lw=1)
    plt.xlabel("N"); plt.ylabel(T("加速倍數（純量=1）","Speedup (scalar=1)"))
    plt.title(T("向量化加速比","Vectorization speedup"))
    plt.xticks(x,[str(n) for n in Ns]); plt.legend()
    for i,val in enumerate(cyc_sp): plt.text(x[i]+w/2, val+0.05, f"{val:.1f}x", ha="center", fontsize=9)
    plt.grid(True, axis="y", alpha=0.3)
    plt.tight_layout(); plt.savefig("docs/fig2_speedup.png", dpi=150); plt.close()

# 圖3：指令組成
Nref = 16 if 16 in Ns else Ns[-1]
plt.figure(figsize=(7,5))
cats = ["math","mem","addr","loop"]
cat_zh = {"math":T("運算","math"),"mem":T("記憶體","mem"),"addr":T("位址計算","addr calc"),"loop":T("迴圈控制","loop")}
cat_c = {"math":"#5DCAA5","mem":"#85B7EB","addr":"#D85A30","loop":"#FAC775"}
xs = np.arange(len(versions)); bottom = np.zeros(len(versions))
for c in cats:
    vals = [data[v][Nref][c] for v in versions]
    plt.bar(xs, vals, 0.5, bottom=bottom, label=cat_zh[c], color=cat_c[c])
    bottom += np.array(vals)
plt.xticks(xs,[label_zh[v] if zh_ok else v for v in versions])
plt.ylabel(T("指令佔比 (%)","Instruction mix (%)"))
plt.title(T(f"指令組成對比（N={Nref}）",f"Instruction mix (N={Nref})"))
plt.legend(loc="upper right"); plt.ylim(0,100)
plt.tight_layout(); plt.savefig("docs/fig3_mix.png", dpi=150); plt.close()

print("已產生：")
for f in ["fig1_instret","fig2_speedup","fig3_mix"]:
    if os.path.exists(f"docs/{f}.png"): print(f"  docs/{f}.png")
print("中文字型：" + ("可用" if zh_ok else "不可用（改用英文標題，不影響數據）"))

# ============================================================
# 圖4：VLEN 掃描（需要 results_vlen.csv）
# 展示「同一份程式，向量寬度加倍 → 效能加倍」的長度無關特性
# ============================================================
import os
if os.path.exists("results_vlen.csv"):
    vrows = list(csv.DictReader(open("results_vlen.csv")))
    # 整理成 vdata[version][N][vlen] = cycles
    vdata = {}
    for r in vrows:
        if not r["cycles"]: continue
        vdata.setdefault(r["version"], {}).setdefault(int(r["N"]), {})[int(r["vlen"])] = int(r["cycles"])

    plt.figure(figsize=(7,5))
    Nplot = 32
    vlens = [128, 256, 512]
    for ver, col, mk in [("vector","#1D9E75","o-"), ("custom","#534AB7","s-")]:
        if ver in vdata and Nplot in vdata[ver]:
            base = vdata[ver][Nplot][128]
            ys = [base / vdata[ver][Nplot][vl] for vl in vlens]  # 相對 128 的加速
            plt.plot(vlens, ys, mk, color=col,
                     label=(label_zh[ver] if zh_ok else ver))
    # 理想線（VLEN 加倍→加速加倍）
    ideal = [1, 2, 4]
    plt.plot(vlens, ideal, "k--", alpha=0.4, label=T("理想線性","ideal linear"))
    plt.xlabel("VLEN (bits)")
    plt.ylabel(T("cycle 加速（相對 VLEN=128）","Speedup vs VLEN=128"))
    plt.title(T(f"向量寬度掃描（N={Nplot}）",f"VLEN scan (N={Nplot})"))
    plt.xscale("log", base=2); plt.xticks(vlens,[str(v) for v in vlens])
    plt.grid(True, which="both", alpha=0.3); plt.legend()
    plt.tight_layout(); plt.savefig("docs/fig4_vlen.png", dpi=150); plt.close()
    print("  docs/fig4_vlen.png")
