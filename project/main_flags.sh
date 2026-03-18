#!/bin/bash

SECONDD=/usr/bin/time

FLAGS=('O1' 'O2' 'O3' 'fastmath' 'native' 'fO3' 'fO3native')


# --- Mesures des opérations sur test_matrix_perso (temps de l'opération seule via chrono interne) ---
# Mesure que de de la perf de notre implémentation 
#
# 1=addition  2=soustraction  3=matmul  4=scalaire  5=transpose  6=sub_mul

OPERATIONS=(1 2 3 4 5 6)
OP_NAMES=('add' 'sub' 'matmul' 'scalar' 'transpose' 'sub_mul')
SIZES=('100' '500' '1000' '2000' '4000')
DIREC=csv
FILES=mesures_perso_.csv

echo "Config, run1, run2, run3, run4, run5, M, N, R, operation" > csv/${FILES}

for k in ${FLAGS[@]}; do
    # Compiler une seule fois par flag
    make -s clean
    make -s test_matrix_perso_$k

    for idx in "${!OPERATIONS[@]}"; do
        op=${OPERATIONS[$idx]}
        name=${OP_NAMES[$((op - 1))]}

        for sz in ${SIZES[@]}; do
        # ON limite la taille des matrices pour la mult 
            if [ $op -eq 3 ] && [ $sz -gt 500 ]; then
                continue
            fi

            echo -n "${k}_${name}" >> ${DIREC}/${FILES}
            for i in 1 2 3 4 5; do
                TIME=$(./test_matrix_perso_$k $op $sz $sz $sz)
                echo -n ", $TIME" >> ${DIREC}/${FILES}
            done
            echo ", $sz, $sz, $sz, $op" >> ${DIREC}/${FILES}
        done
    done
done

# Nettoyage final
make -s clean
