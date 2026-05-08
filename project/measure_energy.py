"""
Measure energy consumption of the OpenCL matrix multiply binary using CodeCarbon.

Single size:
    python measure_energy.py 512 512 512 [--runs 3]

Sweep all default sizes:
    python measure_energy.py --sweep [--runs 3] [--output energy_results.csv]
"""

import subprocess
import sys
import argparse
import csv
import os
from codecarbon import EmissionsTracker

BINARY = "./test_opencl_perso"
DEFAULT_SIZES = [25, 50, 100, 200, 400, 800, 1600, 3200]


def run_once(m, n, r):
    result = subprocess.run(
        [BINARY, str(m), str(n), str(r)],
        capture_output=True, text=True
    )
    if result.returncode != 0:
        print(result.stderr.strip(), file=sys.stderr)
        raise RuntimeError(f"Binary failed (exit {result.returncode})")
    print(result.stderr.strip(), file=sys.stderr)
    return result.stdout.strip()  # "M,N,R,time_ms"


def measure_size(m, n, r, runs, output):
    print(f"\n{'='*50}", file=sys.stderr)
    print(f"Matrice {m}x{n} * {n}x{r}  ({runs} runs)", file=sys.stderr)

    print("  Warm-up...", file=sys.stderr)
    try:
        run_once(m, n, r)
    except RuntimeError as e:
        print(f"  ECHEC warm-up: {e} -> skip", file=sys.stderr)
        return

    tracker = EmissionsTracker(
        project_name="opencl_matmul",
        output_file="csv/codecarbon_raw.csv",
        log_level="error",
        measure_power_secs=1,
        save_to_file=True,
    )

    tracker.start()
    timing_rows = []
    try:
        for i in range(runs):
            row = run_once(m, n, r)
            timing_rows.append(row)
            t = row.split(",")[3]
            print(f"  Run {i+1}/{runs}: {t} ms", file=sys.stderr)
    except RuntimeError as e:
        tracker.stop()
        print(f"  ECHEC pendant mesure: {e} -> skip", file=sys.stderr)
        return

    emissions = tracker.stop()

    last       = tracker.final_emissions_data
    energy_kwh = last.energy_consumed
    cpu_energy = last.cpu_energy
    gpu_energy = last.gpu_energy
    ram_energy = last.ram_energy
    duration_s = last.duration

    avg_time_ms = sum(float(r.split(",")[3]) for r in timing_rows) / runs

    print(f"  Duree         : {duration_s:.1f} s", file=sys.stderr)
    print(f"  Energie totale: {energy_kwh*1e6:.3f} mWh", file=sys.stderr)
    print(f"    CPU         : {cpu_energy*1e6:.3f} mWh", file=sys.stderr)
    print(f"    GPU         : {gpu_energy*1e6:.3f} mWh", file=sys.stderr)
    print(f"    RAM         : {ram_energy*1e6:.3f} mWh", file=sys.stderr)
    print(f"  CO2           : {emissions*1e6:.4f} mg CO2-eq", file=sys.stderr)
    print(f"  Temps moyen   : {avg_time_ms:.3f} ms", file=sys.stderr)

    write_header = not os.path.exists(output)
    with open(output, "a", newline="") as f:
        writer = csv.writer(f)
        if write_header:
            writer.writerow([
                "M", "N", "R", "runs",
                "avg_time_ms", "duration_s",
                "total_energy_kwh", "cpu_energy_kwh", "gpu_energy_kwh", "ram_energy_kwh",
                "energy_per_run_kwh", "co2_kg"
            ])
        writer.writerow([
            m, n, r, runs,
            f"{avg_time_ms:.3f}", f"{duration_s:.3f}",
            f"{energy_kwh:.8f}", f"{cpu_energy:.8f}", f"{gpu_energy:.8f}", f"{ram_energy:.8f}",
            f"{energy_kwh/runs:.8f}", f"{emissions:.8f}"
        ])
    print(f"  -> sauvegarde dans {output}", file=sys.stderr)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("M", type=int, nargs="?")
    parser.add_argument("N", type=int, nargs="?")
    parser.add_argument("R", type=int, nargs="?")
    parser.add_argument("--sweep", action="store_true",
                        help="Mesure toutes les tailles par defaut")
    parser.add_argument("--sizes", type=int, nargs="+",
                        help="Tailles personnalisees pour --sweep (ex: --sizes 128 256 512)")
    parser.add_argument("--runs", type=int, default=3)
    parser.add_argument("--output", default="csv/energy_results.csv")
    args = parser.parse_args()

    if not os.path.exists(BINARY):
        print(f"Binaire introuvable: {BINARY}. Lance: make test_time_opencl_perso", file=sys.stderr)
        sys.exit(1)

    os.makedirs("csv", exist_ok=True)

    if args.sweep:
        sizes = args.sizes if args.sizes else DEFAULT_SIZES
        print(f"Sweep sur tailles: {sizes}", file=sys.stderr)
        for sz in sizes:
            measure_size(sz, sz, sz, args.runs, args.output)
    elif args.M and args.N and args.R:
        measure_size(args.M, args.N, args.R, args.runs, args.output)
    else:
        parser.print_help()
        sys.exit(1)

    print(f"\nTermine. Resultats dans {args.output}", file=sys.stderr)


if __name__ == "__main__":
    main()
