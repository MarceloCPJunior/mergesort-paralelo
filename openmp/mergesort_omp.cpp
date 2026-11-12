
/* OpenMP 07 — MergeSort com tasks
 * IESB 2026/2 — CCO085
 */

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cstdint>
#include <omp.h>

#include "../comum/mergesort_comum.hpp"

using namespace std;

// MergeSort paralelo com OpenMP Tasks
static void msort_omp(vector<int>& a, vector<int>& tmp,
                      int lo, int hi, int cutoff) {

    if (hi - lo <= 1)
        return;

    // Quando o trecho é pequeno, executa sequencialmente
    if (hi - lo <= cutoff) {
        msort(a, tmp, lo, hi);
        return;
    }

    int mid = lo + (hi - lo) / 2;

    // Cria tarefa para a metade esquerda
    #pragma omp task shared(a, tmp) firstprivate(lo, mid, cutoff)
    msort_omp(a, tmp, lo, mid, cutoff);

    // Cria tarefa para a metade direita
    #pragma omp task shared(a, tmp) firstprivate(mid, hi, cutoff)
    msort_omp(a, tmp, mid, hi, cutoff);

    // Espera as duas metades terminarem
    #pragma omp taskwait

    // Intercala as duas metades ordenadas
    merge(a, tmp, lo, mid, hi);
}

int main(int argc, char** argv) {

    // Argumentos: N THREADS [CUTOFF]
    int n = (argc > 1) ? atoi(argv[1]) : 20000000;
    int p = (argc > 2) ? atoi(argv[2]) : omp_get_max_threads();
    int cutoff = (argc > 3) ? atoi(argv[3]) : (1 << 14);

    if (n < 0 || p < 1 || cutoff < 1) {
        fprintf(stderr, "Argumentos invalidos.\n");
        return 1;
    }

    // Define o número de threads
    omp_set_num_threads(p);

    // Entrada determinística compartilhada com o baseline
    vector<int> a = gerar_entrada(n);
    vector<int> tmp(n);

    // Mede somente a execução paralela
    double t0 = agora();

    #pragma omp parallel
    {
        #pragma omp single
        msort_omp(a, tmp, 0, n, cutoff);
    }

    double t1 = agora();

    // Validação e checksum fora do tempo medido
    bool ok = verifica_ordenado(a);
    uint64_t ck = checksum(a);

    // Saída padronizada do contrato
    imprime_resultado("omp", n, p, ok, ck, t1 - t0);

    return ok ? 0 : 1;
}
