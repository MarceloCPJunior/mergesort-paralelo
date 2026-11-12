#!/usr/bin/env bash
# Coleta de tempos do projeto mergesort — CCO085 IESB 2026/2 (frente 1)
# Segue docs/contrato.md: 5 repetições, descarta a 1ª (aquecimento), grava CSV,
# confere ordenado=SIM e o checksum do gabarito em TODA execução.
#
# Uso:   bash scripts/medir.sh [experimento] [repeticoes]
#   experimento: baseline | forte | fraca | todos   (padrão: todos)
#   repeticoes : padrão 5 (a 1ª é descartada)
#
# Exemplos:
#   bash scripts/medir.sh baseline        # só o baseline do professor (5M, 20M, 80M)
#   bash scripts/medir.sh forte 3         # escalabilidade forte, 3 repetições
#
# Versões que ainda não existem (bin/mergesort_omp, bin/mergesort_mpi) são puladas
# com um aviso, então o script já funciona antes do OpenMP e do MPI ficarem prontos.
set -euo pipefail

cd "$(dirname "$0")/.."               # roda a partir da raiz do projeto
EXPERIMENTO=${1:-todos}
REP=${2:-5}
SAIDA=resultados
# --use-hwthread-cpus: aceita até nproc processos (núcleos lógicos).
# --bind-to none: o sistema operacional decide onde cada processo roda e espalha
# primeiro pelos núcleos físicos, como faz com as threads do OpenMP. Sem isso, o
# mpirun pode prender 2 processos nas duas threads de hardware do MESMO núcleo.
MPIRUN=${MPIRUN:-"mpirun --use-hwthread-cpus --bind-to none"}

TAMANHOS_BASELINE="5000000 20000000 80000000"
N_FORTE=20000000                      # escalabilidade forte: n fixo
N_BASE_FRACA=5000000                  # escalabilidade fraca: n = 5M x p
NUCLEOS=$(nproc)
ARQ_COMUNIC=""                        # só o experimento forte grava o tempo de comunicação

# p = 1, 2, 4, 8... só até nproc: acima disso os núcleos seriam divididos e o
# tempo pioraria por um motivo que não é do algoritmo.
LISTA_P=""
for p in 1 2 4 8 16 32; do if (( p <= NUCLEOS )); then LISTA_P+="$p "; fi; done

erro() { echo "ERRO: $*" >&2; exit 1; }

# campo NOME "linha"  ->  imprime o valor de NOME=valor (ou nada, se não houver)
campo() { sed -nE "s/(^|.* )$1=([^ ]+).*/\2/p" <<< "$2"; }

# Checksum de referência de um n, calculado uma vez pelo gabarito e guardado.
declare -A GABARITO
carrega_gabarito() {
    local n=$1 linha
    if [[ -z "${GABARITO[$n]:-}" ]]; then
        linha=$(./bin/gabarito "$n")
        [[ $(campo confere_std_sort "$linha") == SIM ]] || erro "gabarito não confere com std::sort (n=$n)"
        GABARITO[$n]=$(campo checksum "$linha")
    fi
    REF=${GABARITO[$n]}
}

# roda VERSAO N P ARQUIVO comando...
# Executa o comando REP vezes, valida cada saída e grava as repetições 2..REP.
roda() {
    local versao=$1 n=$2 p=$3 arq=$4; shift 4
    carrega_gabarito "$n"
    local r linha t ck
    for r in $(seq 1 "$REP"); do
        linha=$("$@") || erro "falhou: $*"
        [[ $(campo ordenado "$linha") == SIM ]] || erro "$versao n=$n p=$p não ordenou: $linha"
        # O baseline do professor não imprime checksum; o gabarito (mesma lógica) cobre isso.
        if [[ $versao != seq ]]; then
            ck=$(campo checksum "$linha")
            [[ $ck == "$REF" ]] || erro "$versao n=$n p=$p checksum $ck != gabarito $REF"
        fi
        t=$(campo tempo "$linha")
        [[ -n $t ]] || erro "sem campo tempo= na saída: $linha"
        if (( r > 1 )); then                     # a 1ª é aquecimento
            echo "$versao,$n,$p,$((r-1)),$t" >> "$arq"
            local tc; tc=$(campo t_comunicacao "$linha")
            if [[ -n $tc && -n $ARQ_COMUNIC ]]; then echo "$versao,$n,$p,$((r-1)),$t,$tc" >> "$ARQ_COMUNIC"; fi
        fi
    done
    printf "  %-4s n=%-9s p=%-2s ok (checksum confere)\n" "$versao" "$n" "$p"
}

novo_csv() { echo "versao,n,p,repeticao,tempo_s" > "$1"; }

# Mediana por (versao, n, p): ordena por chave e tempo, depois pega o meio de cada grupo.
resumo() {
    echo "  versao  n          p   mediana_s  (repetições)"
    tail -n +2 "$1" | sort -t, -k1,1 -k2,2n -k3,3n -k5,5g | awk -F, '
        function fecha() { if (k) printf "  %-6s  %-9s  %-2s  %9.6f  (%d)\n", c[1], c[2], c[3], (k % 2 ? v[(k+1)/2] : (v[k/2] + v[k/2+1]) / 2), k }
        { chave = $1 "," $2 "," $3
          if (chave != ant) { fecha(); k = 0; split(chave, c, ","); ant = chave }
          v[++k] = $5 }
        END { fecha() }'
}

registra_maquina() {
    {
        echo "data: $(date '+%Y-%m-%d %H:%M')"
        echo "nproc: $NUCLEOS"
        echo "cpu: $(lscpu 2>/dev/null | sed -nE 's/^Model name: *//p' | head -1)"
        echo "compilador: $(g++ --version | head -1)"
        echo "mpi: $(mpirun --version 2>/dev/null | head -1)"
        echo "repeticoes: $REP (1a descartada)"
    } > "$SAIDA/maquina.txt"
}

# ---------------------------------------------------------------------------
exp_baseline() {
    echo "== baseline (bin/mergesort_seq) — n = $TAMANHOS_BASELINE"
    local arq=$SAIDA/baseline.csv; novo_csv "$arq"
    for n in $TAMANHOS_BASELINE; do roda seq "$n" 1 "$arq" ./bin/mergesort_seq "$n"; done
    resumo "$arq"
}

# Para forte e fraca: roda OpenMP e MPI, se existirem, para cada p da lista.
roda_paralelas() {   # roda_paralelas ARQ N_FIXO_OU_VAZIO
    local arq=$1 n_fixo=$2 p n
    for p in $LISTA_P; do
        n=${n_fixo:-$(( N_BASE_FRACA * p ))}
        if [[ -x bin/mergesort_omp ]]; then
            roda omp "$n" "$p" "$arq" env OMP_NUM_THREADS="$p" ./bin/mergesort_omp "$n" "$p"
        fi
        if [[ -x bin/mergesort_mpi ]]; then
            roda mpi "$n" "$p" "$arq" $MPIRUN -np "$p" ./bin/mergesort_mpi "$n"
        fi
    done
}

aviso_faltando() {
    [[ -x bin/mergesort_omp ]] || echo "  (bin/mergesort_omp não existe ainda: OpenMP pulado)"
    [[ -x bin/mergesort_mpi ]] || echo "  (bin/mergesort_mpi não existe ainda: MPI pulado)"
}

exp_forte() {
    echo "== escalabilidade forte — n = $N_FORTE, p = $LISTA_P"
    aviso_faltando
    local arq=$SAIDA/forte.csv; novo_csv "$arq"
    ARQ_COMUNIC=$SAIDA/mpi_comunicacao.csv
    echo "versao,n,p,repeticao,tempo_s,t_comunicacao_s" > "$ARQ_COMUNIC"
    roda_paralelas "$arq" "$N_FORTE"
    ARQ_COMUNIC=""
    if [[ $(wc -l < "$arq") -gt 1 ]]; then resumo "$arq"; fi
}

exp_fraca() {
    echo "== escalabilidade fraca — n = $N_BASE_FRACA x p, p = $LISTA_P"
    aviso_faltando
    local arq=$SAIDA/fraca.csv; novo_csv "$arq"
    roda_paralelas "$arq" ""
    if [[ $(wc -l < "$arq") -gt 1 ]]; then resumo "$arq"; fi
}

# ---------------------------------------------------------------------------
make -s                                  # garante binários atualizados (-O2)
mkdir -p "$SAIDA"
registra_maquina
echo "Máquina: $NUCLEOS núcleos lógicos — detalhes em $SAIDA/maquina.txt"

case $EXPERIMENTO in
    baseline) exp_baseline ;;
    forte)    exp_forte ;;
    fraca)    exp_fraca ;;
    todos)    exp_baseline; exp_forte; exp_fraca ;;
    *)        erro "experimento desconhecido: $EXPERIMENTO (use baseline | forte | fraca | todos)" ;;
esac
echo "Dados brutos em $SAIDA/*.csv"
