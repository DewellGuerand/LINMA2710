"""
Measure energy consumption of the OpenCL matrix multiply binary using CodeCarbon.
Usage:
    python measure_energy.py <M> <N> <R> [--runs N]

Example:
    python measure_energy.py 512 512 512 --runs 5
"""

import subprocess
import sys
import argparse
import csv
import os
from codecarbon import EmissionsTracker

BINARY = "./test_opencl_perso"


def run_once(m, n, r):
    result = subprocess.run(
        [BINARY, str(m), str(n), str(r)],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        print(result.stderr, file=sys.stderr)
        raise RuntimeError(f"Binary failed with code {result.returncode}")
    # stderr has device info, stdout has the CSV row
    print(result.stderr.strip(), file=sys.stderr)
    return result.stdout.strip()  # "M,N,R,time_ms"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("M", type=int)
    parser.add_argument("N", type=int)
    parser.add_argument("R", type=int)
    parser.add_argument("--runs", type=int, default=5)
    parser.add_argument("--output", default="energy_results.csv")
    args = parser.parse_args()

    if not os.path.exists(BINARY):
        print(f"Binary not found: {BINARY}. Run: make test_time_opencl_perso", file=sys.stderr)
        sys.exit(1)

    # Warm-up run (not tracked) to avoid JIT overhead in energy measurement
    print("Warm-up run...", file=sys.stderr)
    run_once(args.M, args.N, args.R)

    tracker = EmissionsTracker(
        project_name="opencl_matmul",
        output_file="codecarbon_emissions.csv",
        log_level="error",   # suppress verbose logging
        measure_power_secs=1,
        save_to_file=True,
    )

    print(f"Measuring {args.runs} run(s) of {args.M}x{args.N} * {args.N}x{args.R}...", file=sys.stderr)

    tracker.start()
    timing_rows = []
    for i in range(args.runs):
        row = run_once(args.M, args.N, args.R)
        timing_rows.append(row)
        print(f"  Run {i+1}/{args.runs}: {row} (M,N,R,time_ms)", file=sys.stderr)

    emissions = tracker.stop()  # kg CO2-eq

    # Retrieve detailed energy data
    last = tracker.final_emissions_data
    energy_kwh = last.energy_consumed    # kWh
    cpu_energy = last.cpu_energy         # kWh
    gpu_energy = last.gpu_energy         # kWh
    ram_energy = last.ram_energy         # kWh
    duration_s = last.duration           # seconds

    total_time_ms = sum(float(r.split(",")[3]) for r in timing_rows)
    avg_time_ms   = total_time_ms / args.runs

    print("\n--- CodeCarbon Results ---", file=sys.stderr)
    print(f"  Duration          : {duration_s:.2f} s", file=sys.stderr)
    print(f"  Total energy      : {energy_kwh*1e6:.3f} mWh  ({energy_kwh:.6f} kWh)", file=sys.stderr)
    print(f"    CPU energy      : {cpu_energy*1e6:.3f} mWh", file=sys.stderr)
    print(f"    GPU energy      : {gpu_energy*1e6:.3f} mWh", file=sys.stderr)
    print(f"    RAM energy      : {ram_energy*1e6:.3f} mWh", file=sys.stderr)
    print(f"  CO2 emissions     : {emissions*1e6:.4f} mg CO2-eq", file=sys.stderr)
    print(f"  Avg matmul time   : {avg_time_ms:.3f} ms", file=sys.stderr)
    print(f"  Energy per run    : {energy_kwh/args.runs*1e6:.3f} mWh", file=sys.stderr)

    # Write summary CSV
    write_header = not os.path.exists(args.output)
    with open(args.output, "a", newline="") as f:
        writer = csv.writer(f)
        if write_header:
            writer.writerow([
                "M", "N", "R", "runs",
                "avg_time_ms", "duration_s",
                "total_energy_kwh", "cpu_energy_kwh", "gpu_energy_kwh", "ram_energy_kwh",
                "energy_per_run_kwh", "co2_kg"
            ])
        writer.writerow([
            args.M, args.N, args.R, args.runs,
            f"{avg_time_ms:.3f}", f"{duration_s:.3f}",
            f"{energy_kwh:.8f}", f"{cpu_energy:.8f}", f"{gpu_energy:.8f}", f"{ram_energy:.8f}",
            f"{energy_kwh/args.runs:.8f}", f"{emissions:.8f}"
        ])
    print(f"\nResults appended to {args.output}", file=sys.stderr)


if __name__ == "__main__":
    main()
