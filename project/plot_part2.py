import pandas as pd
import matplotlib.pyplot as plt
import os

CSV_PATH = os.path.join(os.path.dirname(__file__), "csv", "mesures_perso_threads.csv")

df = pd.read_csv(CSV_PATH, skipinitialspace=True)
df.columns = df.columns.str.strip()

df[["flag", "op_name"]] = df["Config"].str.split("_", n=1, expand=True)

run_cols = ["run1", "run2", "run3", "run4", "run5"]
df[run_cols] = df[run_cols].apply(pd.to_numeric, errors="coerce")
df["mean_time"] = df[run_cols].mean(axis=1)
df["size"] = df["M"].astype(int)
df["threads"] = df["threads"].astype(int)

operations = df["op_name"].unique()
sizes      = sorted(df["size"].unique())
colors     = ["steelblue", "darkorange", "seagreen", "darkred", "purple", "brown"]

for op in operations:
    fig, ax = plt.subplots(figsize=(9, 5))
    subset = df[df["op_name"] == op].sort_values("threads")

    for i, sz in enumerate(sizes):
        grp = subset[subset["size"] == sz]
        if grp.empty:
            continue
        ax.plot(grp["threads"], grp["mean_time"],
                marker="o", label=f"size={sz}", color=colors[i % len(colors)])

    ax.set_title(f"Temps d'exécution — opération : {op}", fontsize=13)
    ax.set_xlabel("Nombre de threads")
    ax.set_ylabel("Temps moyen (s)")
    ax.legend(title="Taille")
    ax.grid(linestyle="--", alpha=0.5)
    fig.tight_layout()

    out = os.path.join(os.path.dirname(__file__), "csv", f"plot_threads_{op}.png")
    fig.savefig(out, dpi=150)
    print(f"Sauvegardé : {out}")

plt.show()
