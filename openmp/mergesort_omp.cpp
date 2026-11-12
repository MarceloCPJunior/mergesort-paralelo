/* OpenMP 07 — MergeSort com tasks
 * IESB 2026/2 — CCO085
 *
 * Compilar: g++ -O2 -fopenmp -o mergesort_omp mergesort_omp.cpp
 * Executar: OMP_NUM_THREADS=4 ./mergesort_omp 2000000
 */
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cstdint>
#include <ctime>
#include <omp.h>
using namespace std;

static double agora() {
    timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

static void merge(vector<int>& a, vector<int>& tmp, int lo, int mid, int hi) {
    int i = lo, j = mid, k = lo;
    while (i < mid && j < hi) tmp[k++] = (a[i] <= a[j]) ? a[i++] : a[j++];
    while (i < mid) tmp[k++] = a[i++];
    while (j < hi)  tmp[k++] = a[j++];
    for (int t = lo; t < hi; ++t) a[t] = tmp[t];
}

// MergeSort sequencial usado nas folhas (mantém o algoritmo do baseline).
static void msort_seq(vector<int>& a, vector<int>& tmp, int lo, int hi) {
    if (hi - lo <= 1) return;
    int mid = lo + (hi - lo) / 2;
    msort_seq(a, tmp, lo, mid);
    msort_seq(a, tmp, mid, hi);
    merge(a, tmp, lo, mid, hi);
}

static const int CUTOFF = 1 << 14;

static void msort_omp(vector<int>& a, vector<int>& tmp, int lo, int hi) {
    if (hi - lo <= 1) return;
    if (hi - lo <= CUTOFF) {
        msort_seq(a, tmp, lo, hi);
        return;
    }

    int mid = lo + (hi - lo) / 2;
    #pragma omp task shared(a, tmp) firstprivate(lo, mid)
    msort_omp(a, tmp, lo, mid);

    #pragma omp task shared(a, tmp) firstprivate(mid, hi)
    msort_omp(a, tmp, mid, hi);

    #pragma omp taskwait
    merge(a, tmp, lo, mid, hi);
}

int main(int argc, char** argv) {
    int n = (argc > 1) ? atoi(argv[1]) : 20000000;
    if (n < 0) { fprintf(stderr, "Tamanho invalido.\\n"); return 1; }

    vector<int> a(n), tmp(n);
    uint64_t s = 12345;
    for (int i = 0; i < n; ++i) {
        s = s * 6364136223846793005ULL + 1;
        a[i] = (int)(s >> 33);
    }

    double t0 = agora();
    #pragma omp parallel
    {
        #pragma omp single
        msort_omp(a, tmp, 0, n);
    }
    double t1 = agora();

    bool ok = true;
    for (int i = 1; i < n; ++i) if (a[i-1] > a[i]) { ok = false; break; }
    uint64_t checksum = 0;
    for (int x : a) checksum += (uint32_t)x;

    printf("n=%d  ordenado=%s  checksum=%llu  threads=%d  tempo=%.6f s\\n",
           n, ok ? "SIM" : "NAO", (unsigned long long)checksum,
           omp_get_max_threads(), t1-t0);
    return ok ? 0 : 1;
}
