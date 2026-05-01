import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np
import math
import os
import argparse

DEFAULT_CSV = os.path.join(os.path.dirname(__file__), "csv", "mesures_perso_node.csv")
OUT_DIR = os.path.join(os.path.dirname(__file__), "report", "pics")

parser = argparse.ArgumentParser(description="Plots benchmarks MPI distribués")
parser.add_argument("--csv", default=DEFAULT_CSV, help="Chemin vers le fichier CSV")
args = parser.parse_args()

csv_stem = os.path.splitext(os.path.basename(args.csv))[0]

df = pd.read_csv(args.csv, skipinitialspace=True)
df.columns = df.columns.str.strip()

df[["np_label", "metric"]] = df["Config"].str.split("_", n=1, expand=True)
df["n_procs"] = df["np_label"].str.replace("np", "").astype(int)

run_cols = ["run1", "run2", "run3", "run4", "run5"]
df[run_cols] = df[run_cols].apply(pd.to_numeric, errors="coerce")
df["mean_time"] = df[run_cols].mean(axis=1)
df["std_time"]  = df[run_cols].std(axis=1)
df["size"] = df["M"].astype(int)

sizes = sorted(df["size"].unique())
procs = sorted(df["n_procs"].unique())

compute = df[df["metric"] == "t_compute"].pivot_table(index="n_procs", columns="size", values="mean_time")
comm    = df[df["metric"] == "t_comm"].pivot_table(index="n_procs", columns="size", values="mean_time")
total   = df[df["metric"] == "t_total"].pivot_table(index="n_procs", columns="size", values="mean_time")

colors_compute = "#4878CF"
colors_comm    = "#D65F5F"
COLORS = ["#4878CF", "#6ACC65", "#D65F5F", "#B47CC7", "#C4AD66", "#77BEDB"]

MAX_COLS = 4
CELL_W, CELL_H = 3.5, 3.2   # inches per subplot cell


def make_grid(n):
    ncols = min(n, MAX_COLS)
    nrows = math.ceil(n / ncols)
    fig, axes = plt.subplots(nrows, ncols,
                             figsize=(ncols * CELL_W, nrows * CELL_H),
                             squeeze=False)
    flat = axes.flatten()
    for ax in flat[n:]:          # hide unused cells
        ax.set_visible(False)
    return fig, list(flat[:n])


# ============================================================ #
# Figure 1 — Stacked bar: compute vs comm per size             #
# ============================================================ #
fig, axs = make_grid(len(sizes))

for ax, sz in zip(axs, sizes):
    x = np.arange(len(procs))
    t_comp  = [compute.loc[p, sz] if p in compute.index else 0 for p in procs]
    t_comm_ = [comm.loc[p, sz]    if p in comm.index    else 0 for p in procs]

    ax.bar(x, t_comp,  color=colors_compute, label="Compute", zorder=3)
    ax.bar(x, t_comm_, bottom=t_comp, color=colors_comm, label="Comm", zorder=3)

    ax.set_xticks(x)
    ax.set_xticklabels([f"np{p}" for p in procs], fontsize=7)
    ax.set_title(f"size = {sz}", fontsize=9)
    ax.set_xlabel("MPI procs", fontsize=8)
    ax.grid(axis="y", linestyle="--", alpha=0.5, zorder=0)
    ax.yaxis.set_major_formatter(ticker.FormatStrFormatter("%.3f"))
    ax.tick_params(axis="y", labelsize=7)

handles, labels = axs[0].get_legend_handles_labels()
fig.legend(handles, labels, loc="upper right", fontsize=9)
fig.suptitle("Compute vs Communication time — multiplyTransposed", fontsize=11)
axs[0].set_ylabel("Time (s)", fontsize=8)
fig.tight_layout()
out = os.path.join(OUT_DIR, f"{csv_stem}_stacked_compute_comm.svg")
fig.savefig(out)
print(f"Saved: {out}")

# ============================================================ #
# Figure 2 — Comm fraction: comm / total vs n_procs            #
# ============================================================ #
fig, ax = plt.subplots(figsize=(MAX_COLS * CELL_W, 2 * CELL_H))

for i, sz in enumerate(sizes):
    fracs = []
    for p in procs:
        if p in comm.index and p in total.index:
            fracs.append(comm.loc[p, sz] / total.loc[p, sz] * 100)
        else:
            fracs.append(np.nan)
    ax.plot(procs, fracs, marker="o", label=f"size={sz}", color=COLORS[i % len(COLORS)])

ax.set_xlabel("Number of MPI processes")
ax.set_ylabel("Comm / total (%)")
ax.set_title("Communication overhead vs number of processes", fontsize=11)
ax.set_xticks(procs)
ax.yaxis.set_major_formatter(ticker.FormatStrFormatter("%.1f%%"))
ax.legend(title="Matrix size", fontsize=8, ncol=2)
ax.grid(linestyle="--", alpha=0.5)
fig.tight_layout()
out = os.path.join(OUT_DIR, f"{csv_stem}_comm_fraction.svg")
fig.savefig(out)
print(f"Saved: {out}")

# ============================================================ #
# Figure 3 — Speedup: compute vs comm vs total, one per size   #
# ============================================================ #
fig, axs = make_grid(len(sizes))
base_np = min(procs)

for ax, sz in zip(axs, sizes):
    ref_comp  = compute.loc[base_np, sz] if base_np in compute.index else None
    ref_comm  = comm.loc[base_np, sz]    if base_np in comm.index    else None
    ref_total = total.loc[base_np, sz]   if base_np in total.index   else None

    sp_comp  = [ref_comp  / compute.loc[p, sz] for p in procs if p in compute.index]
    sp_comm  = [ref_comm  / comm.loc[p, sz]    for p in procs if p in comm.index]
    sp_total = [ref_total / total.loc[p, sz]   for p in procs if p in total.index]
    ideal    = [p / base_np for p in procs]

    ax.plot(procs, sp_comp,  marker="o", color=colors_compute, label="Compute")
    ax.plot(procs, sp_comm,  marker="s", color=colors_comm,    label="Comm",  linestyle="--")
    ax.plot(procs, sp_total, marker="^", color="seagreen",     label="Total", linestyle="-.")
    ax.plot(procs, ideal,    linestyle=":", color="black",      label="Ideal")

    ax.set_xticks(procs)
    ax.set_xticklabels([f"{p}" for p in procs], fontsize=7)
    ax.set_title(f"size = {sz}", fontsize=9)
    ax.set_xlabel("MPI procs", fontsize=8)
    ax.tick_params(axis="y", labelsize=7)
    ax.grid(linestyle="--", alpha=0.5)

handles, labels = axs[0].get_legend_handles_labels()
fig.legend(handles, labels, loc="upper right", fontsize=9)
fig.suptitle(f"Speedup breakdown (relative to np{base_np})", fontsize=11)
axs[0].set_ylabel("Speedup", fontsize=8)
fig.tight_layout()
out = os.path.join(OUT_DIR, f"{csv_stem}_speedup_breakdown.svg")
fig.savefig(out)
print(f"Saved: {out}")

# ============================================================ #
# Figure 4 — Absolute times: compute & comm vs size (log-log)  #
# ============================================================ #
fig, axs = make_grid(len(procs))

for ax, p in zip(axs, procs):
    t_comp  = [compute.loc[p, sz] for sz in sizes if p in compute.index]
    t_comm_ = [comm.loc[p, sz]    for sz in sizes if p in comm.index]

    ax.loglog(sizes, t_comp,  marker="o", color=colors_compute, label="Compute")
    ax.loglog(sizes, t_comm_, marker="s", color=colors_comm,    label="Comm", linestyle="--")

    ax.set_title(f"np = {p}", fontsize=9)
    ax.set_xlabel("Matrix size", fontsize=8)
    ax.grid(which="both", linestyle="--", alpha=0.4)
    ax.set_xticks(sizes)
    ax.get_xaxis().set_major_formatter(ticker.ScalarFormatter())
    ax.tick_params(axis="both", labelsize=7)
    plt.setp(ax.get_xticklabels(), rotation=45, ha="right")

handles, labels = axs[0].get_legend_handles_labels()
fig.legend(handles, labels, loc="upper right", fontsize=9)
fig.suptitle("Compute vs Communication time vs matrix size (log-log)", fontsize=11)
axs[0].set_ylabel("Time (s)", fontsize=8)
fig.tight_layout()
out = os.path.join(OUT_DIR, f"{csv_stem}_time_vs_size.svg")
fig.savefig(out)
print(f"Saved: {out}")

plt.show()
