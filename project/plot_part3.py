import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np
import os

CSV_PATH = os.path.join(os.path.dirname(__file__), "csv", "mesures_perso_node.csv")
OUT_DIR  = os.path.join(os.path.dirname(__file__), "report", "pics")

df = pd.read_csv(CSV_PATH, skipinitialspace=True)
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

# Pivot tables for compute and comm
compute = df[df["metric"] == "t_compute"].pivot_table(index="n_procs", columns="size", values="mean_time")
comm    = df[df["metric"] == "t_comm"].pivot_table(index="n_procs", columns="size", values="mean_time")
total   = df[df["metric"] == "t_total"].pivot_table(index="n_procs", columns="size", values="mean_time")

colors_compute = "#4878CF"
colors_comm    = "#D65F5F"
COLORS = ["#4878CF", "#6ACC65", "#D65F5F", "#B47CC7", "#C4AD66", "#77BEDB"]

# ============================================================ #
# Figure 1 — Stacked bar: compute vs comm per size             #
# One subplot per matrix size, x-axis = n_procs                #
# ============================================================ #
fig, axes = plt.subplots(1, len(sizes), figsize=(4 * len(sizes), 5), sharey=False)
if len(sizes) == 1:
    axes = [axes]

for ax, sz in zip(axes, sizes):
    x = np.arange(len(procs))
    t_comp = [compute.loc[p, sz] if p in compute.index else 0 for p in procs]
    t_comm_ = [comm.loc[p, sz]   if p in comm.index   else 0 for p in procs]

    bars_comp = ax.bar(x, t_comp,  color=colors_compute, label="Compute", zorder=3)
    bars_comm = ax.bar(x, t_comm_, bottom=t_comp, color=colors_comm, label="Comm (Allreduce)", zorder=3)

    ax.set_xticks(x)
    ax.set_xticklabels([f"np{p}" for p in procs])
    ax.set_title(f"size = {sz}", fontsize=11)
    ax.set_xlabel("MPI processes")
    ax.grid(axis="y", linestyle="--", alpha=0.5, zorder=0)
    ax.yaxis.set_major_formatter(ticker.FormatStrFormatter("%.3f"))

axes[0].set_ylabel("Time (s)")
handles, labels = axes[0].get_legend_handles_labels()
fig.legend(handles, labels, loc="upper right", fontsize=10)
fig.suptitle("Compute vs Communication time — multiplyTransposed\n(stacked bars per matrix size)", fontsize=13)
fig.tight_layout()
out = os.path.join(OUT_DIR, "part3_stacked_compute_comm.png")
fig.savefig(out, dpi=150)
print(f"Saved: {out}")

# ============================================================ #
# Figure 2 — Comm fraction: comm / total vs n_procs            #
# One line per matrix size                                      #
# ============================================================ #
fig, ax = plt.subplots(figsize=(8, 5))

for i, sz in enumerate(sizes):
    fracs = []
    for p in procs:
        if p in comm.index and p in total.index:
            fracs.append(comm.loc[p, sz] / total.loc[p, sz] * 100)
        else:
            fracs.append(np.nan)
    ax.plot(procs, fracs, marker="o", label=f"size={sz}", color=COLORS[i % len(COLORS)])

ax.set_xlabel("Number of MPI processes")
ax.set_ylabel("Communication fraction (% of total time)")
ax.set_title("Communication overhead — comm / total time\nvs number of processes and matrix size", fontsize=12)
ax.set_xticks(procs)
ax.yaxis.set_major_formatter(ticker.FormatStrFormatter("%.1f%%"))
ax.legend(title="Matrix size")
ax.grid(linestyle="--", alpha=0.5)
fig.tight_layout()
out = os.path.join(OUT_DIR, "part3_comm_fraction.png")
fig.savefig(out, dpi=150)
print(f"Saved: {out}")

# ============================================================ #
# Figure 3 — Speedup: compute vs comm vs total                 #
# One subplot per matrix size                                   #
# ============================================================ #
fig, axes = plt.subplots(1, len(sizes), figsize=(4 * len(sizes), 5), sharey=False)
if len(sizes) == 1:
    axes = [axes]

base_np = min(procs)

for ax, sz in zip(axes, sizes):
    ref_comp  = compute.loc[base_np, sz] if base_np in compute.index else None
    ref_comm  = comm.loc[base_np, sz]    if base_np in comm.index    else None
    ref_total = total.loc[base_np, sz]   if base_np in total.index   else None

    sp_comp  = [ref_comp  / compute.loc[p, sz] for p in procs if p in compute.index]
    sp_comm  = [ref_comm  / comm.loc[p, sz]    for p in procs if p in comm.index]
    sp_total = [ref_total / total.loc[p, sz]   for p in procs if p in total.index]

    ax.plot(procs, sp_comp,  marker="o", color=colors_compute, label="Compute speedup")
    ax.plot(procs, sp_comm,  marker="s", color=colors_comm,    label="Comm speedup",  linestyle="--")
    ax.plot(procs, sp_total, marker="^", color="seagreen",     label="Total speedup", linestyle="-.")

    ideal = [p / base_np for p in procs]
    ax.plot(procs, ideal, linestyle=":", color="black", label="Ideal")

    ax.set_xticks(procs)
    ax.set_xticklabels([f"np{p}" for p in procs])
    ax.set_title(f"size = {sz}", fontsize=11)
    ax.set_xlabel("MPI processes")
    ax.grid(linestyle="--", alpha=0.5)

axes[0].set_ylabel(f"Speedup (relative to np{base_np})")
handles, labels = axes[0].get_legend_handles_labels()
fig.legend(handles, labels, loc="upper right", fontsize=9)
fig.suptitle("Speedup breakdown: Compute / Comm / Total\n(relative to single process)", fontsize=13)
fig.tight_layout()
out = os.path.join(OUT_DIR, "part3_speedup_breakdown.png")
fig.savefig(out, dpi=150)
print(f"Saved: {out}")

# ============================================================ #
# Figure 4 — Absolute times: compute & comm vs matrix size     #
# One subplot per n_procs, log-log scale                       #
# ============================================================ #
fig, axes = plt.subplots(1, len(procs), figsize=(4 * len(procs), 5), sharey=False)
if len(procs) == 1:
    axes = [axes]

for ax, p in zip(axes, procs):
    t_comp  = [compute.loc[p, sz] for sz in sizes if p in compute.index]
    t_comm_ = [comm.loc[p, sz]    for sz in sizes if p in comm.index]

    ax.loglog(sizes, t_comp,  marker="o", color=colors_compute, label="Compute")
    ax.loglog(sizes, t_comm_, marker="s", color=colors_comm,    label="Comm", linestyle="--")

    ax.set_title(f"np = {p}", fontsize=11)
    ax.set_xlabel("Matrix size")
    ax.grid(which="both", linestyle="--", alpha=0.4)
    ax.set_xticks(sizes)
    ax.get_xaxis().set_major_formatter(ticker.ScalarFormatter())

axes[0].set_ylabel("Time (s) — log scale")
handles, labels = axes[0].get_legend_handles_labels()
fig.legend(handles, labels, loc="upper right", fontsize=10)
fig.suptitle("Compute vs Communication time vs matrix size (log-log)\nper number of MPI processes", fontsize=13)
fig.tight_layout()
out = os.path.join(OUT_DIR, "part3_time_vs_size.png")
fig.savefig(out, dpi=150)
print(f"Saved: {out}")

plt.show()
