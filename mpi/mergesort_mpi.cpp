/* Versão MPI do mergesort (memória distribuída) — frente 3
 * Projeto Acadêmico CCO085 (IESB 2026/2) — Problema 07
 *
 * Decomposição de DOMÍNIO: o vetor é cortado em p blocos contíguos, um por
 * processo. Cada processo ordena o seu bloco com o msort do baseline e depois
 * os blocos ordenados são intercalados em árvore até o rank 0 ter o vetor todo.
 *
 *   1. rank 0 gera a entrada (gerar_entrada, semente 12345)      fora do cronômetro
 *   2. MPI_Scatterv entrega ~n/p elementos a cada processo      ┐
 *   3. cada processo ordena o seu bloco (msort comum)            │ cronometrado
 *   4. merge em árvore com MPI_Send/MPI_Recv, ceil(log2 p) rodadas┘
 *   5. MPI_Reduce(MAX): o tempo do programa é o do processo mais lento
 *   6. rank 0 verifica e calcula o checksum                      fora do cronômetro
 *
 * merge, msort, gerador, checksum e formato de saída vêm de comum/ (docs/contrato.md):
 * aqui muda só QUEM executa cada parte e QUANDO.
 *
 * Uso: mpirun -np P ./bin/mergesort_mpi N
 */
#include <mpi.h>
#include <algorithm>
#include <cstdlib>
#include "../comum/mergesort_comum.hpp"

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, p;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    // O mpirun passa os mesmos argumentos a todos os processos, então todos já
    // sabem n sem precisar de MPI_Bcast.
    int n = (argc > 1) ? atoi(argv[1]) : 20000000;

    // ---------------- Preparação (fora do cronômetro, como no contrato) ----------------

    // Memória distribuída: só o rank 0 tem o vetor de entrada; os outros não o enxergam.
    vector<int> global;
    if (rank == 0) global = gerar_entrada(n);

    // Quantos elementos cada rank recebe (cont) e onde o bloco dele começa (desl).
    // Os n % p primeiros ranks ganham 1 elemento a mais: nenhum elemento sobra,
    // mesmo com n não divisível por p (é por isso que usamos Scatterv e não Scatter).
    // Com n < p, alguns ranks ficam com 0 elementos, e tudo continua funcionando.
    vector<int> cont(p), desl(p);
    for (int r = 0, ini = 0; r < p; r++) {
        cont[r] = n / p + (r < n % p ? 1 : 0);
        desl[r] = ini;
        ini += cont[r];
    }

    // Na árvore, cada rank absorve os blocos dos ranks logo à sua direita:
    // rank 0 absorve todos; rank 4 (com p = 8) absorve 4, 5, 6 e 7; rank 1 só o próprio.
    // "alcance" = quantos ranks este processo vai juntar (mesma regra do laço da árvore).
    // Com isso já alocamos aqui o tamanho final, e não há realocação dentro do cronômetro.
    int alcance = 1;
    while (alcance < p && rank % (2 * alcance) == 0) alcance *= 2;
    int ultimo = std::min(rank + alcance, p) - 1;
    int capacidade = desl[ultimo] + cont[ultimo] - desl[rank];

    vector<int> local, tmp(capacidade);   // tmp: área auxiliar do merge, já no tamanho final
    local.reserve(capacidade);
    local.resize(cont[rank]);

    // ---------------- Início do cronômetro: todos largam juntos ----------------
    MPI_Barrier(MPI_COMM_WORLD);
    double t0 = MPI_Wtime();

    // 1) Distribuição (comunicação coletiva): o rank 0 manda o bloco r para o rank r.
    MPI_Scatterv(rank == 0 ? global.data() : nullptr, cont.data(), desl.data(), MPI_INT,
                 local.data(), cont[rank], MPI_INT, 0, MPI_COMM_WORLD);
    double t1 = MPI_Wtime();

    // 2) Ordenação local (computação): o MESMO msort do baseline, no bloco deste rank.
    msort(local, tmp, 0, cont[rank]);
    double t2 = MPI_Wtime();

    // 3) Merge em árvore. Rodadas com passo = 1, 2, 4, ... enquanto passo < p:
    //    - rank % (2*passo) == passo : envia o bloco para rank - passo e sai da árvore;
    //    - rank % (2*passo) == 0     : recebe o bloco de rank + passo (se ele existir) e intercala.
    //    Cada par tem um sentido só (o maior envia, o menor recebe), então não há deadlock.
    //    Com p que não é potência de 2, o parceiro rank + passo pode não existir: pula a rodada.
    //    O bloco recebido vem logo depois do meu no vetor global, então colocá-lo no fim
    //    de "local" e chamar o merge comum dá o trecho global ordenado.
    double t_arvore_comunic = 0.0, t_arvore_merge = 0.0;
    for (int passo = 1; passo < p; passo *= 2) {
        if (rank % (2 * passo) == passo) {
            double c0 = MPI_Wtime();
            MPI_Send(local.data(), (int)local.size(), MPI_INT, rank - passo, 0, MPI_COMM_WORLD);
            t_arvore_comunic += MPI_Wtime() - c0;
            break;                                   // entregou o bloco: este rank terminou
        } else if (rank % (2 * passo) == 0 && rank + passo < p) {
            int parceiro = rank + passo;
            double c0 = MPI_Wtime();
            // MPI_Probe espera a mensagem chegar e MPI_Get_count diz quantos ints ela tem,
            // sem precisar de uma mensagem separada só com o tamanho.
            MPI_Status st;
            int m;
            MPI_Probe(parceiro, 0, MPI_COMM_WORLD, &st);
            MPI_Get_count(&st, MPI_INT, &m);
            int meio = (int)local.size();
            local.resize(meio + m);                  // cabe na capacidade reservada: sem realocar
            MPI_Recv(local.data() + meio, m, MPI_INT, parceiro, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            double c1 = MPI_Wtime();
            merge(local, tmp, 0, meio, meio + m);    // o MESMO merge do baseline
            t_arvore_merge   += MPI_Wtime() - c1;
            t_arvore_comunic += c1 - c0;             // inclui a espera pelo parceiro terminar
        }
    }
    double t3 = MPI_Wtime();
    // ---------------- Fim do cronômetro: vetor completo no rank 0 ----------------

    // Tempo do programa = tempo do processo mais lento (coletiva MPI_Reduce com MPI_MAX).
    // Cada parcela também é o máximo entre os processos (por isso as parcelas não
    // precisam somar exatamente o total).
    double meus[6] = {
        t3 - t0,                               // total
        (t1 - t0) + t_arvore_comunic,          // comunicação: scatter + envios/recebimentos da árvore
        t1 - t0,                               // scatter
        t2 - t1,                               // ordenação local
        t_arvore_comunic,                      // comunicação da árvore
        t_arvore_merge                         // merges da árvore
    };
    double maxs[6];
    MPI_Reduce(meus, maxs, 6, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    // ---------------- Validação (fora do cronômetro) ----------------
    // Só o rank 0 tem o vetor final. Como ele está inteiro e na ordem global,
    // a posição i do checksum já é a posição global, como pede o contrato.
    if (rank == 0) {
        bool ok = (int)local.size() == n && verifica_ordenado(local);
        imprime_resultado("mpi", n, p, ok, checksum(local), maxs[0], false);
        printf(" t_comunicacao=%.6f t_scatter=%.6f t_sort_local=%.6f t_arvore_comunic=%.6f t_arvore_merge=%.6f\n",
               maxs[1], maxs[2], maxs[3], maxs[4], maxs[5]);
    }

    MPI_Finalize();
    return 0;
}
