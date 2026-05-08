import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np
import math
import os
import argparse

DEFAULT_CSV = os.path.join(os.path.dirname(__file__), "csv", "mesures_perso_opencl_fast.csv")
OUT_DIR = os.path.join(os.path.dirname(__file__), "report", "pics")

parser = argparse.ArgumentParser(description="Plots benchmarks OpenCL GPU")
parser.add_argument("--csv", default=DEFAULT_CSV, help="Chemin vers le fichier CSV")
args = parser.parse_args()

csv_stem = os.path.splitext(os.path.basename(args.csv))[0]

os.makedirs(OUT_DIR, exist_ok=True)

df = pd.read_csv(args.csv, skipinitialspace=True)
df.columns = df.columns.str.strip()

run_cols = ["run1", "run2", "run3", "run4", "run5"]
df[run_cols] = df[run_cols].apply(pd.to_numeric, errors="coerce")

# Drop rows where all runs are NaN (e.g. 25600 with no data)
df = df.dropna(subset=run_cols, how="all")

df["mean_time"] = df[run_cols].mean(axis=1)
df["std_time"]  = df[run_cols].std(axis=1)
df["min_time"]  = df[run_cols].min(axis=1)
df["max_time"]  = df[run_cols].max(axis=1)
df["size"]      = df["M"].astype(int)

sizes = sorted(df["size"].unique())

COLOR_MEAN = "#4878CF"
COLOR_STD  = "#D65F5F"
COLOR_MIN  = "#6ACC65"
COLOR_MAX  = "#B47CC7"
COLORS     = ["#4878CF", "#6ACC65", "#D65F5F", "#B47CC7", "#C4AD66", "#77BEDB"]

MAX_COLS = 4
CELL_W, CELL_H = 3.5, 3.2


def make_grid(n):
    ncols = min(n, MAX_COLS)
    nrows = math.ceil(n / ncols)
    fig, axes = plt.subplots(nrows, ncols,
                             figsize=(ncols * CELL_W, nrows * CELL_H),
                             squeeze=False)
    flat = axes.flatten()
    for ax in flat[n:]:
        ax.set_visible(False)
    return fig, list(flat[:n])


# ============================================================ #
# Figure 1 — Bar chart: mean time per size + error bars        #
# ============================================================ #
fig, ax = plt.subplots(figsize=(MAX_COLS * CELL_W, 2 * CELL_H))

x = np.arange(len(sizes))
bars = ax.bar(x, df["mean_time"], color=COLOR_MEAN, zorder=3, label="Mean time")
ax.errorbar(x, df["mean_time"], yerr=df["std_time"],
            fmt="none", color=COLOR_STD, capsize=4, linewidth=1.5, label="±std", zorder=4)

ax.set_xticks(x)
ax.set_xticklabels([str(s) for s in sizes], fontsize=8, rotation=45, ha="right")
ax.set_xlabel("Matrix size (M)", fontsize=9)
ax.set_ylabel("Time (µs)", fontsize=9)
ax.set_title("OpenCL mean execution time vs matrix size", fontsize=11)
ax.legend(fontsize=9)
ax.grid(axis="y", linestyle="--", alpha=0.5, zorder=0)
ax.yaxis.set_major_formatter(ticker.FormatStrFormatter("%.1f"))
fig.tight_layout()
out = os.path.join(OUT_DIR, f"{csv_stem}_mean_time_bar.svg")
fig.savefig(out)
print(f"Saved: {out}")


# ============================================================ #
# Figure 2 — Log-log: mean time + min/max band vs size         #
# ============================================================ #
fig, ax = plt.subplots(figsize=(MAX_COLS * CELL_W, 2 * CELL_H))

ax.loglog(sizes, df["mean_time"], marker="o", color=COLOR_MEAN, label="Mean", zorder=3)
ax.fill_between(sizes, df["min_time"], df["max_time"],
                color=COLOR_MEAN, alpha=0.2, label="Min–Max range", zorder=2)
ax.loglog(sizes, df["min_time"], marker="v", linestyle="--",
          color=COLOR_MIN, linewidth=0.8, label="Min", zorder=3)
ax.loglog(sizes, df["max_time"], marker="^", linestyle="--",
          color=COLOR_MAX, linewidth=0.8, label="Max", zorder=3)

# Fit a power-law on mean (log-log linear regression)
log_s = np.log(sizes)
log_t = np.log(df["mean_time"].values)
slope, intercept = np.polyfit(log_s, log_t, 1)
fit_vals = np.exp(intercept) * np.array(sizes) ** slope
ax.loglog(sizes, fit_vals, linestyle=":", color="black",
          label=f"Fit slope = {slope:.2f}", zorder=5)

ax.set_xticks(sizes)
ax.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
plt.setp(ax.get_xticklabels(), rotation=45, ha="right", fontsize=7)
ax.set_xlabel("Matrix size (M)", fontsize=9)
ax.set_ylabel("Time (µs)", fontsize=9)
ax.set_title("OpenCL execution time vs matrix size (log-log)", fontsize=11)
ax.legend(fontsize=9)
ax.grid(which="both", linestyle="--", alpha=0.4)
fig.tight_layout()
out = os.path.join(OUT_DIR, f"{csv_stem}_loglog.svg")
fig.savefig(out)
print(f"Saved: {out}")


# ============================================================ #
# Figure 3 — Variability: std / mean (CV%) per size            #
# ============================================================ #
fig, ax = plt.subplots(figsize=(MAX_COLS * CELL_W, 2 * CELL_H))

cv = df["std_time"] / df["mean_time"] * 100

ax.bar(np.arange(len(sizes)), cv, color=COLOR_STD, zorder=3)
ax.set_xticks(np.arange(len(sizes)))
ax.set_xticklabels([str(s) for s in sizes], fontsize=8, rotation=45, ha="right")
ax.set_xlabel("Matrix size (M)", fontsize=9)
ax.set_ylabel("CV = std/mean (%)", fontsize=9)
ax.set_title("Run-to-run variability (coefficient of variation)", fontsize=11)
ax.yaxis.set_major_formatter(ticker.FormatStrFormatter("%.2f%%"))
ax.grid(axis="y", linestyle="--", alpha=0.5, zorder=0)
fig.tight_layout()
out = os.path.join(OUT_DIR, f"{csv_stem}_variability.svg")
fig.savefig(out)
print(f"Saved: {out}")


# ============================================================ #
# Figure 4 — Per-size boxplot of 5 runs                        #
# ============================================================ #
fig, ax = plt.subplots(figsize=(MAX_COLS * CELL_W, 2 * CELL_H))

run_data = [df.loc[df["size"] == s, run_cols].values.flatten() for s in sizes]
bp = ax.boxplot(run_data, patch_artist=True,
                boxprops=dict(facecolor="#AED6F1", color=COLOR_MEAN),
                medianprops=dict(color=COLOR_STD, linewidth=2),
                whiskerprops=dict(color=COLOR_MEAN),
                capprops=dict(color=COLOR_MEAN),
                flierprops=dict(marker="x", color=COLOR_STD))

ax.set_xticklabels([str(s) for s in sizes], fontsize=8, rotation=45, ha="right")
ax.set_yscale("log")
ax.set_xlabel("Matrix size (M)", fontsize=9)
ax.set_ylabel("Time (µs, log scale)", fontsize=9)
ax.set_title("Distribution of 5 runs per matrix size", fontsize=11)
ax.grid(axis="y", which="both", linestyle="--", alpha=0.4)
fig.tight_layout()
out = os.path.join(OUT_DIR, f"{csv_stem}_boxplot.svg")
fig.savefig(out)
print(f"Saved: {out}")


# ============================================================ #
# Figure 5 — Empirical log-log slope between consecutive sizes #
# ============================================================ #
fig, ax = plt.subplots(figsize=(MAX_COLS * CELL_W, 2 * CELL_H))

slopes = []
slope_labels = []
for i in range(1, len(sizes)):
    s0, s1 = sizes[i-1], sizes[i]
    t0, t1 = df.loc[df["size"] == s0, "mean_time"].values[0], \
              df.loc[df["size"] == s1, "mean_time"].values[0]
    s = math.log(t1 / t0) / math.log(s1 / s0)
    slopes.append(s)
    slope_labels.append(f"{s0}→{s1}")

x = np.arange(len(slopes))
bars = ax.bar(x, slopes, color=COLOR_MEAN, zorder=3)
ax.axhline(3.0, linestyle=":", color="red",   linewidth=1.2, label="Slope = 3 (O(N³))")
ax.axhline(2.0, linestyle=":", color="green", linewidth=1.2, label="Slope = 2 (O(N²))")
ax.set_xticks(x)
ax.set_xticklabels(slope_labels, fontsize=7, rotation=45, ha="right")
ax.set_xlabel("Size transition", fontsize=9)
ax.set_ylabel("Empirical slope (log-log)", fontsize=9)
ax.set_title("Empirical complexity exponent between consecutive sizes", fontsize=11)
ax.legend(fontsize=9)
ax.grid(axis="y", linestyle="--", alpha=0.5, zorder=0)
fig.tight_layout()
out = os.path.join(OUT_DIR, f"{csv_stem}_empirical_slope.svg")
fig.savefig(out)
print(f"Saved: {out}")


# ============================================================ #
# Figure 6 — All 5 individual runs overlaid (log-log)          #
# ============================================================ #
fig, ax = plt.subplots(figsize=(MAX_COLS * CELL_W, 2 * CELL_H))

for i, col in enumerate(run_cols):
    ax.loglog(sizes, df[col].values, marker="o", linestyle="-",
              color=COLORS[i % len(COLORS)], linewidth=1.0,
              markersize=4, label=col, alpha=0.8)
ax.loglog(sizes, df["mean_time"], marker="o", linestyle="-",
          color="black", linewidth=2.0, markersize=5, label="Mean", zorder=5)

ax.set_xticks(sizes)
ax.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
plt.setp(ax.get_xticklabels(), rotation=45, ha="right", fontsize=7)
ax.set_xlabel("Matrix size (M)", fontsize=9)
ax.set_ylabel("Time (µs)", fontsize=9)
ax.set_title("All individual runs + mean (log-log)", fontsize=11)
ax.legend(fontsize=9, ncol=3)
ax.grid(which="both", linestyle="--", alpha=0.4)
fig.tight_layout()
out = os.path.join(OUT_DIR, f"{csv_stem}_all_runs_loglog.svg")
fig.savefig(out)
print(f"Saved: {out}")

plt.show()