#!/usr/bin/env bash
set -euo pipefail

N=20000000
P=4
CUTOFFS=(1024 4096 16384 65536)
REPETICOES=5

mkdir -p resultados
CSV="resultados/cutoff.csv"

make -s

# Obtém checksum do gabarito
REF=$(./bin/gabarito "$N")
CK_REF=$(echo "$REF" | sed -n 's/.*checksum=\([0-9]*\).*/\1/p')

if [[ -z "$CK_REF" ]]; then
    echo "Erro: não foi possível obter checksum do gabarito."
    exit 1
fi

echo "cutoff,repeticao,tempo_s" > "$CSV"

for C in "${CUTOFFS[@]}"; do
    echo "== cutoff=$C =="

    for R in $(seq 1 "$REPETICOES"); do
        OUT=$(OMP_NUM_THREADS="$P" ./bin/mergesort_omp "$N" "$P" "$C")

        CK=$(echo "$OUT" | sed -n 's/.*checksum=\([0-9]*\).*/\1/p')
        OK=$(echo "$OUT" | sed -n 's/.*ordenado=\([^ ]*\).*/\1/p')
        T=$(echo "$OUT" | sed -n 's/.*tempo=\([0-9.]*\).*/\1/p')

        if [[ "$CK" != "$CK_REF" || "$OK" != "SIM" || -z "$T" ]]; then
            echo "Falha na validação: cutoff=$C repetição=$R"
            echo "$OUT"
            exit 1
        fi

        # Primeira execução é aquecimento: não entra no CSV
        if [[ "$R" -gt 1 ]]; then
            REP=$((R - 1))
            echo "$C,$REP,$T" >> "$CSV"
            echo "  repetição $REP: $T s"
        else
            echo "  aquecimento: $T s"
        fi
    done

    # Mediana das quatro execuções válidas
    MED=$(awk -F, -v c="$C" '$1==c {print $3}' "$CSV" |
          sort -n |
          awk '{v[NR]=$1} END {print (v[2]+v[3])/2}')

    echo "  mediana cutoff=$C: $MED s"
done

echo
echo "Resultados gravados em $CSV"
