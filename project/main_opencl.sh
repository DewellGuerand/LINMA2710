#!/bin/bash
#SBATCH --job-name=Multiply_opencl
#SBATCH --output=logs/bench_opencl_%j.out
#SBATCH --error=logs/bench_opencl_%j.err
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem-per-cpu=2000
#SBATCH --time=02:00:00
#SBATCH --partition=batch
#SBATCH --gres=gpu:1

# CSV produit : M,run1,run2,run3,run4,run5

SIZES=('25' '50' '75' '100' '200' '400' '800' '1600' '3200' '6400')
DIREC=csv
FILE=mesures_perso_opencl_fast.csv

echo "M,run1,run2,run3,run4,run5" > ${DIREC}/${FILE}

make -s clean
make -s test_time_opencl_perso

for sz in "${SIZES[@]}"; do

    times=()
    for i in 1 2 3 4 5; do
        # stdout -> "M,N,R,avg_ms"   stderr -> diagnostics (bench_opencl_*.err)
        output=$(srun ./test_opencl_perso "$sz" "$sz" "$sz" 2>/dev/null)
        t=$(echo "$output" | cut -d',' -f4)
        times+=("$t")
    done

    echo -n "$sz" >> ${DIREC}/${FILE}
    for val in "${times[@]}"; do echo -n ",$val" >> ${DIREC}/${FILE}; done
    echo "" >> ${DIREC}/${FILE}

done

make -s clean
