#!/bin/bash
# Mesure de consommation energetique OpenCL pour plusieurs tailles de matrices.
# Usage: bash main_energy.sh
# Resultats: csv/energy_opencl.csv  +  csv/codecarbon_emissions.csv

VENV="$HOME/.venv/codecarbon/bin/python"
SCRIPT="measure_energy.py"
SIZES=(25 50 100 200 400 800 1600)
RUNS=3
OUTPUT="csv/energy_opencl.csv"

# --- Verifications ---
if [ ! -f "$VENV" ]; then
    echo "ERREUR: venv introuvable. Lance d'abord:"
    echo "  python3 -m venv ~/.venv/codecarbon && ~/.venv/codecarbon/bin/pip install codecarbon"
    exit 1
fi

if [ ! -f "./test_opencl_perso" ]; then
    echo "Compilation..."
    make -s test_time_opencl_perso || { echo "ERREUR: compilation echouee"; exit 1; }
fi

mkdir -p csv

# Repart d'un fichier vide (le script Python ecrit le header lui-meme)
rm -f "$OUTPUT"
# Idem pour le log codecarbon
rm -f csv/codecarbon_emissions.csv

echo "============================================"
echo " Mesure energie OpenCL - $(date '+%Y-%m-%d %H:%M')"
echo " Tailles : ${SIZES[*]}"
echo " Runs par taille : $RUNS"
echo " Resultats -> $OUTPUT"
echo "============================================"

for sz in "${SIZES[@]}"; do
    echo ""
    echo ">>> Matrice ${sz}x${sz} ..."
    "$VENV" "$SCRIPT" "$sz" "$sz" "$sz" \
        --runs "$RUNS" \
        --output "$OUTPUT"

    if [ $? -ne 0 ]; then
        echo "  ECHEC pour sz=$sz (trop grande pour le GPU ?), on continue."
    fi
done

echo ""
echo "============================================"
echo " Termine. Resultats dans $OUTPUT"
echo "============================================"
