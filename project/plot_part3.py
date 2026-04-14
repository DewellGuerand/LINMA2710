import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os

CSV_PATH = os.path.join(os.path.dirname(__file__), "csv", "mesures_perso_node.csv")
OUT_DIR  = os.path.join(os.path.dirname(__file__), "csv")

df = pd.read_csv(CSV_PATH, skipinitialspace=True)
df.columns = df.columns.str.strip()

# Config format: np4_t_compute, np8_t_comm, etc.
df[["np_label", "metric"]] = df["Config"].str.split("_", n=1, expand=True)
df["n_procs"] = df["np_label"].str.replace("np", "").astype(int)

run_cols = ["run1", "run2", "run3", "run4", "run5"]
df[run_cols] = df[run_cols].apply(pd.to_numeric, errors="coerce")
df["mean_time"] = df[run_cols].mean(axis=1)
df["size"] = df["M"].astype(int)

metrics = df["metric"].unique()

for metric in metrics:
    subset  = df[df["metric"] == metric]
    sizes   = sorted(subset["size"].unique())
    procs   = sorted(subset["n_procs"].unique())

    # ------------------------------------------------------------------ #
    # Graphique 1 — Heatmap : taille x n_procs -> temps moyen            #
    # ------------------------------------------------------------------ #
    pivot = subset.pivot_table(index="n_procs", columns="size", values="mean_time")

    fig, ax = plt.subplots(figsize=(9, 5))
    im = ax.imshow(pivot.values, aspect="auto", cmap="YlOrRd_r", origin="lower")

    ax.set_xticks(range(len(pivot.columns)))
    ax.set_xticklabels(pivot.columns)
    ax.set_yticks(range(len(pivot.index)))
    ax.set_yticklabels(pivot.index)
    ax.set_xlabel("Taille de matrice")
    ax.set_ylabel("Nombre de processus MPI")
    ax.set_title(f"Heatmap temps d'exécution — {metric}", fontsize=13)
    plt.colorbar(im, ax=ax, label="Temps moyen (s)")

    for r, t in enumerate(pivot.index):
        for c, s in enumerate(pivot.columns):
            val = pivot.loc[t, s]
            if not np.isnan(val):
                ax.text(c, r, f"{val:.4f}", ha="center", va="center", fontsize=8)

    fig.tight_layout()
    out = os.path.join(OUT_DIR, f"heatmap_{metric}.png")
    fig.savefig(out, dpi=150)
    print(f"Sauvegardé : {out}")

    # ------------------------------------------------------------------ #
    # Graphique 2 — Speedup : temps(min procs) / temps(N procs)          #
    # ------------------------------------------------------------------ #
    base_np = min(procs)
    base = subset[subset["n_procs"] == base_np].groupby("size")["mean_time"].mean()

    fig, ax = plt.subplots(figsize=(9, 5))
    colors = ["steelblue", "darkorange", "seagreen", "darkred", "purple", "brown"]

    for i, sz in enumerate(sizes):
        if sz not in base.index:
            continue
        grp = subset[subset["size"] == sz].sort_values("n_procs")
        speedup = float(base[sz]) / grp["mean_time"].values
        ax.plot(grp["n_procs"].values, speedup,
                marker="o", label=f"size={sz}", color=colors[i % len(colors)])

    # Ligne speedup idéal (normalisée par rapport au min de procs)
    ideal_x = np.array(procs)
    ideal_y = ideal_x / base_np
    ax.plot(ideal_x, ideal_y, linestyle="--", color="black", label="Idéal")

    ax.set_title(f"Speedup — métrique : {metric}", fontsize=13)
    ax.set_xlabel("Nombre de processus MPI")
    ax.set_ylabel(f"Speedup (np{base_np} / npN)")
    ax.set_xticks(procs)
    ax.legend(title="Taille")
    ax.grid(linestyle="--", alpha=0.5)
    fig.tight_layout()

    out = os.path.join(OUT_DIR, f"speedup_{metric}.png")
    fig.savefig(out, dpi=150)
    print(f"Sauvegardé : {out}")

plt.show()