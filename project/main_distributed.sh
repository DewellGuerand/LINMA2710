#!/bin/bash
#SBATCH --job-name=multiplyTransposed_bench
#SBATCH --output=logs/bench_%j.out
#SBATCH --error=logs/bench_%j.err
#
#SBATCH --ntasks=16          # max de tes NODES = 8
#SBATCH --cpus-per-task=1
#SBATCH --mem-per-cpu=500
#SBATCH --time=01:00:00
#SBATCH --partition=batch   # adapte selon ton cluster


# --- Mesures de multiplyTransposed sur test_distributed_perso ---
# Pour chaque nombre de noeuds MPI et chaque taille, on fait 5 runs.
# La fonction multiplyTransposed sort directement t_compute et t_comm.
# Le main mesure t_total (temps mur avec MPI_Barrier avant/après).
#
# CSV : Config, run1, run2, run3, run4, run5, M, N, R, metric
# metric : t_compute | t_comm | t_total

NODES=(1 2 4 8 16)
SIZES=('25' '50' '75' '100' '200' '400' '800' '1600')
DIREC=csv
FILES=mesures_perso_node__.csv

echo "Config, run1, run2, run3, run4, run5, M, N, R, metric" > ${DIREC}/${FILES}

# Compilation unique (le binaire ne change pas selon np)
make -s clean
make -s test_distributed_perso

for np in ${NODES[@]}; do
    for sz in ${SIZES[@]}; do

        t_compute_runs=()
        t_comm_runs=()
        t_total_runs=()

        for i in 1 2 3 4 5; do
            output=$(srun -n "$np" ./test_distributed_perso "$sz" "$sz" "$sz" | grep -v '^#')
            t_compute_runs+=( "$(echo "$output" | sed -n '1p' | awk '{print $1}')" )
            t_comm_runs+=(    "$(echo "$output" | sed -n '1p' | awk '{print $2}')" )
            t_total_runs+=(   "$(echo "$output" | sed -n '2p')" )
        done

        echo -n "np${np}_t_compute" >> ${DIREC}/${FILES}
        for val in "${t_compute_runs[@]}"; do echo -n ", $val" >> ${DIREC}/${FILES}; done
        echo ", $sz, $sz, $sz, t_compute" >> ${DIREC}/${FILES}

        echo -n "np${np}_t_comm" >> ${DIREC}/${FILES}
        for val in "${t_comm_runs[@]}"; do echo -n ", $val" >> ${DIREC}/${FILES}; done
        echo ", $sz, $sz, $sz, t_comm" >> ${DIREC}/${FILES}

        echo -n "np${np}_t_total" >> ${DIREC}/${FILES}
        for val in "${t_total_runs[@]}"; do echo -n ", $val" >> ${DIREC}/${FILES}; done
        echo ", $sz, $sz, $sz, t_total" >> ${DIREC}/${FILES}

    done
done

# Nettoyage final
make -s clean
