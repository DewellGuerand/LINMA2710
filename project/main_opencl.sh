#!/bin/bash
#SBATCH --job-name=Multiply
#SBATCH --output=logs/bench_%j.out
#SBATCH --error=logs/bench_%j.err
#
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --mem-per-cpu=500
#SBATCH --time=01:00:00
#SBATCH --partition=batch

# --- Mesures de matrix_mul GPU sur test_opencl_perso ---
# Pour chaque taille de matrice carrée, on fait 5 runs externes.
# Le binaire fait lui-même une passe de warm-up + 5 runs internes et
# sort UNE ligne CSV : M,N,R,avg_ms
#
# CSV produit : M,run1,run2,run3,run4,run5

SIZES=('25' '50' '75' '100' '200' '400' '800' '1600' '3200' '6400' '12800' '25600')
DIREC=csv
FILE=mesures_perso_opencl.csv

echo "M,run1,run2,run3,run4,run5" > ${DIREC}/${FILE}

make -s clean
make -s test_time_opencl_perso

for sz in "${SIZES[@]}"; do

    times=()
    for i in 1 2 3 4 5; do
        # stdout -> M,N,R,avg_ms   |   stderr -> diagnostic (Platform/Device lines)
        output=$(srun ./test_opencl_perso "$sz" "$sz" "$sz" 2>/dev/null)
        t=$(echo "$output" | cut -d',' -f4)
        times+=("$t")
    done

    echo -n "$sz" >> ${DIREC}/${FILE}
    for val in "${times[@]}"; do echo -n ",$val" >> ${DIREC}/${FILE}; done
    echo "" >> ${DIREC}/${FILE}

done

make -s clean
