import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os
import argparse

DEFAULT_CSV = os.path.join(os.path.dirname(__file__), "csv", "mesures_perso_on_server_tilling.csv")

parser = argparse.ArgumentParser(description="Barres groupées par opération et flag")
parser.add_argument("--csv", default=DEFAULT_CSV, help="Chemin vers le fichier CSV")
parser.add_argument("--op", nargs="+", help="Opération(s) à afficher (défaut : toutes)")
args = parser.parse_args()

csv_stem = os.path.splitext(os.path.basename(args.csv))[0]

df = pd.read_csv(args.csv, skipinitialspace=True)
df.columns = df.columns.str.strip()

df[["flag", "op_name"]] = df["Config"].str.split("_", n=1, expand=True)

run_cols = ["run1", "run2", "run3", "run4", "run5"]
df[run_cols] = df[run_cols].apply(pd.to_numeric, errors="coerce")
df["best"] = df[run_cols].mean(axis=1)
df["size"] = df["M"].astype(int)

operations = df["op_name"].unique()
if args.op:
    operations = [op for op in operations if op in args.op]

flags  = sorted(df["flag"].unique())
sizes  = sorted(df["size"].unique())

n_flags = len(flags)
x       = np.arange(len(sizes))
width   = 0.8 / n_flags
colors  = ["steelblue", "darkorange", "seagreen", "darkred", "purple", "brown"]

for op in operations:
    fig, ax = plt.subplots(figsize=(9, 5))
    subset = df[df["op_name"] == op]

    for i, flag in enumerate(flags):
        grp = subset[subset["flag"] == flag].set_index("size")
        heights = [grp.loc[s, "best"] if s in grp.index else 0.0 for s in sizes]
        offset  = (i - n_flags / 2 + 0.5) * width
        ax.bar(x + offset, heights, width=width, label=flag, color=colors[i % len(colors)])

    ax.set_title(f"Meilleur temps — opération : {op}", fontsize=13)
    ax.set_xlabel("Taille de matrice (M = N = R)")
    ax.set_ylabel("Temps (s)")
    ax.set_xticks(x)
    ax.set_xticklabels(sizes)
    ax.legend(title="Flag")
    ax.grid(axis="y", linestyle="--", alpha=0.5)
    fig.tight_layout()

    out = os.path.join(os.path.dirname(__file__), "report", "pics", f"{csv_stem}_{op}.svg")
    fig.savefig(out)
    print(f"Sauvegardé : {out}")

plt.show()
