import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os
import argparse

DEFAULT_CSV = os.path.join(os.path.dirname(__file__), "csv", "mesures_perso_threads__on_server_tilling.csv")
OUT_DIR = os.path.join(os.path.dirname(__file__), "report", "pics")

parser = argparse.ArgumentParser(description="Heatmap et speedup par opération")
parser.add_argument("--csv", default=DEFAULT_CSV, help="Chemin vers le fichier CSV")
parser.add_argument("--op", nargs="+", help="Opération(s) à afficher (défaut : toutes)")
args = parser.parse_args()

csv_stem = os.path.splitext(os.path.basename(args.csv))[0]

df = pd.read_csv(args.csv, skipinitialspace=True)
df.columns = df.columns.str.strip()

df[["flag", "op_name"]] = df["Config"].str.split("_", n=1, expand=True)

run_cols = ["run1", "run2", "run3", "run4", "run5"]
df[run_cols] = df[run_cols].apply(pd.to_numeric, errors="coerce")
df["mean_time"] = df[run_cols].mean(axis=1)
df["size"]      = df["M"].astype(int)
df["threads"]   = df["threads"].astype(int)

operations = df["op_name"].unique()
if args.op:
    operations = [op for op in operations if op in args.op]

for op in operations:
    subset  = df[df["op_name"] == op]
    sizes   = sorted(subset["size"].unique())
    threads = sorted(subset["threads"].unique())

    # Graphique 1 — Heatmap : taille x threads -> temps moyen
    pivot = subset.pivot_table(index="threads", columns="size", values="mean_time")

    fig, ax = plt.subplots(figsize=(9, 5))
    im = ax.imshow(pivot.values, aspect="auto", cmap="YlOrRd_r", origin="lower")
    ax.set_xticks(range(len(pivot.columns)))
    ax.set_xticklabels(pivot.columns)
    ax.set_yticks(range(len(pivot.index)))
    ax.set_yticklabels(pivot.index)
    ax.set_xlabel("Matrix size")
    ax.set_ylabel("Number of threads")
    ax.set_title(f"Execution time heatmap — {op}", fontsize=13)
    plt.colorbar(im, ax=ax, label="Mean time (s)")
    for r, t in enumerate(pivot.index):
        for c, s in enumerate(pivot.columns):
            val = pivot.loc[t, s]
            if not np.isnan(val):
                ax.text(c, r, f"{val:.3f}", ha="center", va="center", fontsize=8)
    fig.tight_layout()
    out = os.path.join(OUT_DIR, f"{csv_stem}_heatmap_{op}.svg")
    fig.savefig(out)
    print(f"Saved: {out}")

    # Graph 2 — Speedup: time(1 thread) / time(N threads)
    base = subset[subset["threads"] == 1].groupby("size")["mean_time"].mean()

    fig, ax = plt.subplots(figsize=(9, 5))
    colors = ["steelblue", "darkorange", "seagreen", "darkred", "purple", "brown"]

    for i, sz in enumerate(sizes):
        if sz not in base.index:
            continue
        grp = subset[subset["size"] == sz].sort_values("threads")
        speedup = float(base[sz]) / grp["mean_time"].values
        ax.plot(grp["threads"].values, speedup,
                marker="o", label=f"size={sz}", color=colors[i % len(colors)])

    ax.plot(threads, threads, linestyle="--", color="black", label="Ideal")
    ax.set_title(f"Speedup — operation: {op}", fontsize=13)
    ax.set_xlabel("Number of threads")
    ax.set_ylabel("Speedup (t1 / tN)")
    ax.legend(title="Size")
    ax.grid(linestyle="--", alpha=0.5)
    fig.tight_layout()
    out = os.path.join(OUT_DIR, f"{csv_stem}_speedup_{op}.svg")
    fig.savefig(out)
    print(f"Saved: {out}")

plt.show()
