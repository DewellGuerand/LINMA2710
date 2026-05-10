import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np
import os
import argparse

DEFAULT_CSV_SILLY  = os.path.join(os.path.dirname(__file__), "csv", "energy_output_silly.csv")
DEFAULT_CSV_CLEVER = os.path.join(os.path.dirname(__file__), "csv", "energy_output_clever.csv")
OUT_DIR = os.path.join(os.path.dirname(__file__), "report", "pics")

parser = argparse.ArgumentParser(description="Energy & CO2 plots for OpenCL kernels (part 4)")
parser.add_argument("--silly",  default=DEFAULT_CSV_SILLY,  help="Energy CSV for the naive kernel")
parser.add_argument("--clever", default=DEFAULT_CSV_CLEVER, help="Energy CSV for the optimised kernel")
args = parser.parse_args()

os.makedirs(OUT_DIR, exist_ok=True)

KWH_TO_MWH = 1e6   # 1 kWh = 1 000 000 mWh
KG_TO_MG   = 1e6   # 1 kg  = 1 000 000 mg

def load(path, label):
    df = pd.read_csv(path, skipinitialspace=True)
    df.columns = df.columns.str.strip()
    df["size"]              = df["M"].astype(int)
    df["total_mwh"]         = df["total_energy_kwh"]  * KWH_TO_MWH
    df["cpu_mwh"]           = df["cpu_energy_kwh"]    * KWH_TO_MWH
    df["gpu_mwh"]           = df["gpu_energy_kwh"]    * KWH_TO_MWH
    df["ram_mwh"]           = df["ram_energy_kwh"]    * KWH_TO_MWH
    df["per_run_mwh"]       = df["energy_per_run_kwh"] * KWH_TO_MWH
    df["co2_mg"]            = df["co2_kg"]            * KG_TO_MG
    df["label"]             = label
    return df.sort_values("size")

silly  = load(args.silly,  "Naive")
clever = load(args.clever, "Optimised")

sizes       = sorted(set(silly["size"]) | set(clever["size"]))
COLOR_SILLY  = "#D65F5F"
COLOR_CLEVER = "#4878CF"
COLOR_CPU    = "#4878CF"
COLOR_GPU    = "#D65F5F"
COLOR_RAM    = "#6ACC65"

stem = "part4_energy"


# ============================================================ #
# Figure 1 — Total energy per run vs matrix size (log-log)     #
# ============================================================ #
fig, ax = plt.subplots(figsize=(8, 4.5))

ax.loglog(silly["size"],  silly["per_run_mwh"],  marker="o", color=COLOR_SILLY,
          label="Naive",      linewidth=1.8)
ax.loglog(clever["size"], clever["per_run_mwh"], marker="s", color=COLOR_CLEVER,
          label="Optimised", linewidth=1.8)

for df, color in [(silly, COLOR_SILLY), (clever, COLOR_CLEVER)]:
    log_s = np.log(df["size"].values.astype(float))
    log_e = np.log(df["per_run_mwh"].values.astype(float))
    slope, intercept = np.polyfit(log_s, log_e, 1)
    fit = np.exp(intercept) * df["size"].values ** slope
    ax.loglog(df["size"], fit, linestyle=":", color=color, linewidth=1.0,
              label=f"Fit slope = {slope:.2f} ({df['label'].iloc[0]})")

ax.set_xticks(sizes)
ax.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
plt.setp(ax.get_xticklabels(), rotation=45, ha="right", fontsize=8)
ax.set_xlabel("Matrix size (M)", fontsize=10)
ax.set_ylabel("Energy per run (mWh)", fontsize=10)
ax.set_title("Energy per run vs matrix size — naive vs optimised (log-log)", fontsize=11)
ax.legend(fontsize=8, ncol=2)
ax.grid(which="both", linestyle="--", alpha=0.4)
fig.tight_layout()
out = os.path.join(OUT_DIR, f"{stem}_energy_per_run_loglog.svg")
fig.savefig(out)
print(f"Saved: {out}")


# ============================================================ #
# Figure 2 — CO2 emissions vs matrix size                      #
# ============================================================ #
fig, ax = plt.subplots(figsize=(8, 4.5))

ax.plot(silly["size"],  silly["co2_mg"],  marker="o", color=COLOR_SILLY,
        label="Naive",      linewidth=1.8)
ax.plot(clever["size"], clever["co2_mg"], marker="s", color=COLOR_CLEVER,
        label="Optimised", linewidth=1.8)
ax.fill_between(clever["size"], clever["co2_mg"], silly["co2_mg"],
                alpha=0.15, color="grey", label="Savings")

ax.set_xticks(sizes)
ax.set_xticklabels([str(s) for s in sizes], rotation=45, ha="right", fontsize=8)
ax.set_xlabel("Matrix size (M)", fontsize=10)
ax.set_ylabel("CO₂ emissions (mg CO₂-eq)", fontsize=10)
ax.set_title("CO₂ emissions per measurement — naive vs optimised", fontsize=11)
ax.legend(fontsize=9)
ax.grid(linestyle="--", alpha=0.4)
fig.tight_layout()
out = os.path.join(OUT_DIR, f"{stem}_co2.svg")
fig.savefig(out)
print(f"Saved: {out}")


# ============================================================ #
# Figure 3 — Energy breakdown: CPU / GPU / RAM (stacked bars)  #
#             side-by-side for naive and optimised             #
# ============================================================ #
common_sizes = sorted(set(silly["size"]) & set(clever["size"]))
silly_idx  = silly.set_index("size")
clever_idx = clever.set_index("size")

n  = len(common_sizes)
x  = np.arange(n)
w  = 0.35

fig, ax = plt.subplots(figsize=(10, 5))

for offset, df_idx, edge in [(-w/2, silly_idx, COLOR_SILLY), (w/2, clever_idx, COLOR_CLEVER)]:
    cpu = [df_idx.loc[s, "cpu_mwh"] for s in common_sizes]
    gpu = [df_idx.loc[s, "gpu_mwh"] for s in common_sizes]
    ram = [df_idx.loc[s, "ram_mwh"] for s in common_sizes]
    label = df_idx["label"].iloc[0]
    ax.bar(x + offset, cpu, w, color=COLOR_CPU, edgecolor=edge, linewidth=1.2,
           label=f"CPU ({label})" if offset < 0 else f"CPU ({label})")
    ax.bar(x + offset, gpu, w, bottom=cpu, color=COLOR_GPU, edgecolor=edge, linewidth=1.2,
           label=f"GPU ({label})" if offset < 0 else f"GPU ({label})")
    ax.bar(x + offset, ram, w, bottom=[c+g for c,g in zip(cpu,gpu)],
           color=COLOR_RAM, edgecolor=edge, linewidth=1.2,
           label=f"RAM ({label})" if offset < 0 else f"RAM ({label})")

ax.set_xticks(x)
ax.set_xticklabels([str(s) for s in common_sizes], rotation=45, ha="right", fontsize=8)
ax.set_xlabel("Matrix size (M)", fontsize=10)
ax.set_ylabel("Total energy (mWh)", fontsize=10)
ax.set_title("Energy breakdown: CPU / GPU / RAM — naive vs optimised", fontsize=11)
ax.legend(fontsize=7, ncol=2, loc="upper left")
ax.grid(axis="y", linestyle="--", alpha=0.4)
fig.tight_layout()
out = os.path.join(OUT_DIR, f"{stem}_breakdown.svg")
fig.savefig(out)
print(f"Saved: {out}")


# ============================================================ #
# Figure 4 — Energy overhead ratio: naive / optimised          #
# ============================================================ #
ratios = []
for s in common_sizes:
    ratios.append(silly_idx.loc[s, "per_run_mwh"] / clever_idx.loc[s, "per_run_mwh"])

fig, ax = plt.subplots(figsize=(8, 4))

bars = ax.bar(np.arange(n), ratios, color=COLOR_SILLY, zorder=3)
ax.axhline(1.0, linestyle="--", color="black", linewidth=1.0, label="No overhead (ratio = 1)")
for bar, r in zip(bars, ratios):
    ax.text(bar.get_x() + bar.get_width() / 2, r + 0.02, f"{r:.2f}×",
            ha="center", va="bottom", fontsize=8)

ax.set_xticks(np.arange(n))
ax.set_xticklabels([str(s) for s in common_sizes], rotation=45, ha="right", fontsize=8)
ax.set_xlabel("Matrix size (M)", fontsize=10)
ax.set_ylabel("Energy ratio (naive / optimised)", fontsize=10)
ax.set_title("Energy overhead of naive kernel vs optimised kernel", fontsize=11)
ax.legend(fontsize=9)
ax.grid(axis="y", linestyle="--", alpha=0.4)
fig.tight_layout()
out = os.path.join(OUT_DIR, f"{stem}_ratio.svg")
fig.savefig(out)
print(f"Saved: {out}")


# ============================================================ #
# Figure 5 — Execution time vs energy per run (scatter)        #
# ============================================================ #
fig, ax = plt.subplots(figsize=(7, 5))

sc_s = ax.scatter(silly["avg_time_ms"],  silly["per_run_mwh"],
                  c=COLOR_SILLY,  s=60, zorder=3, label="Naive")
sc_c = ax.scatter(clever["avg_time_ms"], clever["per_run_mwh"],
                  c=COLOR_CLEVER, s=60, marker="s", zorder=3, label="Optimised")

for df, color in [(silly, COLOR_SILLY), (clever, COLOR_CLEVER)]:
    for _, row in df.iterrows():
        ax.annotate(str(int(row["size"])),
                    (row["avg_time_ms"], row["per_run_mwh"]),
                    textcoords="offset points", xytext=(5, 3), fontsize=7, color=color)

ax.set_xlabel("Average execution time (ms)", fontsize=10)
ax.set_ylabel("Energy per run (mWh)", fontsize=10)
ax.set_title("Execution time vs energy per run", fontsize=11)
ax.legend(fontsize=9)
ax.grid(linestyle="--", alpha=0.4)
fig.tight_layout()
out = os.path.join(OUT_DIR, f"{stem}_time_vs_energy.svg")
fig.savefig(out)
print(f"Saved: {out}")


plt.show()
