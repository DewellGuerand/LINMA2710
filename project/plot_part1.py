import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys
import os

CSV_PATH = os.path.join(os.path.dirname(__file__), "csv", "mesures_perso_Mmatrix_silly.csv")

# --- Chargement ---
df = pd.read_csv(CSV_PATH, skipinitialspace=True)
df.columns = df.columns.str.strip()

# Extraire flag (O1/O2/O3) et opération depuis la colonne Config (ex: "O1_add")
df[["flag", "op_name"]] = df["Config"].str.split("_", n=1, expand=True)

# Colonnes de temps
run_cols = ["run1", "run2", "run3", "run4", "run5"]
df[run_cols] = df[run_cols].apply(pd.to_numeric, errors="coerce")

# Meilleur temps = minimum des 5 runs
df["best"] = df[run_cols].mean(axis=1)

# Taille de matrice (on utilise M)
df["size"] = df["M"].astype(int)

# --- Tracé ---
operations = df["op_name"].unique()
flags      = sorted(df["flag"].unique())   # O1, O2, O3
sizes      = sorted(df["size"].unique())

n_flags  = len(flags)
x        = np.arange(len(sizes))
width    = 0.8 / n_flags                  # largeur d'un bâtonnet
colors   = ["steelblue", "darkorange", "seagreen" , "darkred" , "purple" , "brown"  ]

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

    out = os.path.join(os.path.dirname(__file__), "csv", f"plot_{op}.png")
    fig.savefig(out, dpi=150)
    print(f"Sauvegardé : {out}")

plt.show()
