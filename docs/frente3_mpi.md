# Frente 3 — Versão MPI (memória distribuída)

Responsável: Thiago PP · Revisor dos seus PRs: frente 2 (David Biage da Silva) · Você revisa: frente 1, scripts e análise (Marcelo)

## Objetivo

Paralelizar o mergesort com processos MPI, com decomposição de domínio justificada, pelo menos uma operação coletiva, análise do overhead de comunicação e resultado idêntico ao baseline para qualquer número de processos. Valor no artefato: **1,0 ponto**.

## Entregas

- [x] `mpi/mergesort_mpi.cpp` usando `comum/mergesort_comum.hpp`, com saída no formato do contrato.
- [x] Distribuição com `MPI_Scatterv` (funciona com `n` não divisível por `p`).
- [x] Junção dos pedaços: merge em árvore. (A comparação com `Gatherv` + merge no rank 0 ficou só na análise teórica, na tabela de decisões.)
- [x] Tempo total e tempo de comunicação separados (`resultados/mpi_comunicacao.csv`).
- [x] Curva de speedup com `-np 1, 2, 4`, checksum validado em todas. **Medição preliminar** num Codespace; falta repetir na máquina comum das medições finais.

## Passo a passo

### 1. Estudo (semana 1)
- Aula 5 da página de aulas.
- Rode e entenda os exemplos do professor:
  ```bash
  cd material_referencia/ambiente_pp && make mpi
  mpirun -np 4 ./codigos/02_mpi/01_hello_mpi
  mpirun -np 4 ./codigos/02_mpi/02_ponto_a_ponto
  mpirun -np 4 ./codigos/02_mpi/03_coletivas_pi 100000000
  mpirun -np 4 ./codigos/02_mpi/04_scatter_gather
  ```

### 2. Protótipo: `Scatterv` → sort local → `Gatherv`
1. Rank 0 gera a entrada com `gerar_entrada` (mesma semente do baseline).
2. `MPI_Bcast` de `n`.
3. Cada rank calcula `cont[]` e `desl[]` (aula 5).
4. `MPI_Scatterv`; cada rank roda o `msort` comum no seu pedaço.
5. `MPI_Gatherv` para o rank 0.
6. Rank 0 intercala os p blocos ordenados.

### 3. Merge em árvore
Em cada rodada `passo = 1, 2, 4, …` enquanto `passo < p`:
- se `rank % (2·passo) == passo`: envia seu bloco para `rank − passo` e sai do laço;
- se `rank % (2·passo) == 0` e `rank + passo < p`: recebe o bloco de `rank + passo` e intercala com o seu.

Para intercalar dois blocos ordenados, coloque-os lado a lado num vetor e use o `merge` do código comum. O tamanho de cada bloco recebido pode ser obtido com `MPI_Probe` + `MPI_Get_count`, ou calculado a partir de `cont[]`.

### 4. Validar
```bash
make mpi
bash scripts/validar.sh rapido     # segundos: n pequenos, p = 1 a 8
bash scripts/validar.sh            # inclui n = 1.000.003 e 20.000.003
```
O `validar.sh` testa os casos difíceis: n não divisível por p, n menor que p e p que não é
potência de 2. Atenção: n ímpar **não** basta (20.000.001 é divisível por 3 e esconde o bug do
`Scatter`); o que importa é `n % p ≠ 0`, e a bateria já cobre isso.

### 5. Separar comunicação de computação
Cronometre separadamente os trechos de `Scatterv`, ordenação local e junção. Reporte o máximo entre os processos com `MPI_Reduce(..., MPI_MAX, ...)`.

## Decisões a justificar

| Decisão | Escolha | Por quê |
|---|---|---|
| Decomposição (domínio × tarefas) | **Domínio**: o vetor é cortado em `p` blocos contíguos, um por processo | Ordenar custa o mesmo para qualquer elemento, então blocos de tamanho igual dão carga igual. Blocos contíguos permitem juntar com o mesmo `merge` do baseline. Decompor por tarefas (cada chamada recursiva vira trabalho de outro processo) exigiria mandar subvetores a cada nível da recursão, com muito mais mensagens, e sem memória compartilhada isso custa caro. |
| Coletiva(s) usada(s) | `MPI_Scatterv` (distribuição), `MPI_Reduce` com `MPI_MAX` (tempo), `MPI_Barrier` (largada) | `Scatterv` aceita uma quantidade diferente por processo (`cont[]`) e o deslocamento de cada bloco (`desl[]`). `Reduce(MAX)` leva ao rank 0 o tempo do processo mais lento, que é o tempo do programa. `Barrier` faz todos começarem o cronômetro juntos. |
| Junção: árvore × Gather + merge no rank 0 | **Árvore** com `MPI_Send`/`MPI_Recv` em ⌈log₂ p⌉ rodadas | Nos dois casos o rank 0 recebe ~n(1 − 1/p) elementos. A diferença está nos merges. Com `Gatherv`, o rank 0 intercala os p blocos sozinho enquanto os outros ficam parados (intercalando um bloco por vez, ~n·p/2 elementos copiados). Na árvore, os merges de uma mesma rodada acontecem **em paralelo** em processos diferentes, e o rank 0 copia ~2n elementos no total (n/p·(2 + 4 + … + p)). A última rodada continua sendo um merge de n elementos num processo só, e isso limita o speedup (ver Análise). |
| O que entra no cronômetro | De `MPI_Barrier` + `MPI_Wtime` até o vetor completo no rank 0; reporta o **máximo** entre os processos | Como manda o contrato (seção 5): distribuição e junção são custo de paralelizar e entram. Gerar a entrada, alocar os vetores (inclusive a capacidade final de cada rank, calculada antes) e verificar/calcular o checksum ficam fora. |
| Como lidar com n não divisível por p | `cont[r] = n/p + (r < n%p ? 1 : 0)`; `desl[r]` = soma dos `cont` anteriores | Os `n % p` primeiros ranks ganham 1 elemento a mais, então nenhum elemento se perde. Com `n < p`, alguns ranks ficam com 0 elementos e continuam participando da árvore (enviam um bloco vazio). Validado por `scripts/validar.sh`: n = 1, 2, 3, 7, 10, 1000, 1.000.003 e 20.000.003 com p = 1 a 8. |
| Tamanho do bloco recebido na árvore | `MPI_Probe` + `MPI_Get_count` | O receptor descobre o tamanho pelo "envelope" da mensagem, sem uma mensagem extra só com o tamanho. |
| Deadlock | Em cada par, o rank maior só envia e o menor só recebe | Nunca dois processos fazem `Send` um para o outro ao mesmo tempo, então ninguém fica esperando quem também está esperando. |

## Resultados preliminares e análise do overhead de comunicação

**Máquina:** GitHub Codespace, AMD EPYC 7763, `nproc` = 4, que na verdade são **2 núcleos físicos × 2 threads de hardware** (SMT). Os outros dados estão em `resultados/mpi_preliminar_codespace/maquina.txt`.
**Não é a máquina das medições finais** (`resultados/maquina.txt`, Xeon), por isso estes dados ficam numa pasta separada e não entram nos gráficos. O speedup abaixo usa o baseline medido **na mesma máquina** (`mpi_preliminar_codespace/baseline.csv`).

Validação: `bash scripts/validar.sh` passou em **56 de 56 casos** (`resultados/validacao.md`), e todas as execuções do `medir.sh` conferiram o checksum do gabarito (20.000.000 → `10913123743811272712`, igual à tabela do contrato).

### Escalabilidade forte (n = 20.000.000, mediana de 4 repetições)

Gerado por `scripts/medir.sh baseline` e `scripts/medir.sh forte` (`forte.csv`, `mpi_comunicacao.csv`); a separação por trecho vem de `mpi_detalhado.csv`. Baseline do professor: **2,260 s**.

| p | tempo (s) | speedup | eficiência | scatter | sort local | comunic. árvore | merges árvore | comunicação / total |
|---|---|---|---|---|---|---|---|---|
| 1 | 2,347 | 0,96 | 0,96 | 0,007 | 2,344 | — | — | 0,3 % |
| 2 | 1,548 | 1,46 | 0,73 | 0,010 | 1,405 | 0,040 | 0,096 | 3,2 % |
| 4 | 0,912 | 2,48 | 0,62 | 0,011 | 0,687 | 0,054 | 0,147 | 7,1 % |
| 2 (um processo por núcleo físico) | 1,242 | 1,82 | 0,91 | 0,010 | 1,109 | 0,030 | 0,094 | 3,3 % |

Cada coluna de trecho é o máximo entre os processos, então os trechos não somam exatamente o total.

### Escalabilidade fraca (n = 5.000.000 × p)

`fraca.csv`: 0,540 s (p = 1), 0,745 s (p = 2), 0,902 s (p = 4). O ideal seria o tempo ficar constante.

### Leitura

1. **A computação cai com p, a junção não.** A ordenação local custa O((n/p)·log(n/p)) e de fato cai: 2,34 s → 0,69 s. Já a junção em árvore faz o rank 0 receber ~n(1 − 1/p) elementos e copiar ~2n(1 − 1/p) nos merges (rodadas com blocos de 2n/p, 4n/p, …, n). Isso é **O(n) e não diminui com p**. Medido: comunicação + merges da árvore = 0,14 s com p = 2 e 0,20 s com p = 4. A fração do tempo gasta só em comunicação dobra (3,2 % → 7,1 %). Como O(n log n) dividido por p diminui e O(n) fica, é o mesmo efeito da parte serial de Amdahl: quanto maior p, mais o tempo total é dominado pela junção. A escalabilidade fraca mostra a mesma coisa: com p = 4, o rank 0 junta 20 M elementos em vez de 5 M.
2. **A última rodada da árvore é serial.** O merge final, de n elementos, é feito por um único processo enquanto os outros já terminaram.
3. **O "comunic. árvore" inclui espera.** O `MPI_Recv` só retorna quando o parceiro termina de ordenar e envia. Qualquer desequilíbrio entre os processos aparece aqui.
4. **Por que o MPI escala pior que o OpenMP numa máquina só.** No OpenMP as threads leem o mesmo vetor, e nenhum dado é copiado para "distribuir". No MPI cada processo tem a própria memória, então o `Scatterv` copia n elementos e a árvore copia ~n de volta, mesmo dentro de uma máquina (via memória compartilhada do OpenMPI). Isso é custo que o OpenMP não tem. Entre máquinas diferentes, com rede, essa cópia seria ainda mais cara.
5. **Núcleo físico × thread de hardware.** Com p = 2, o `medir.sh` (`mpirun --use-hwthread-cpus`) coloca os dois processos nas duas threads de hardware do **mesmo** núcleo físico (`--report-bindings`: `[B./..]` e `[.B/..]`). Eles disputam o mesmo núcleo, e a ordenação local cai só para 1,41 s em vez de ~1,17 s. Com `mpirun --map-by core --bind-to core` cada processo fica num núcleo físico: tempo **1,24 s**, speedup **1,82** em vez de 1,46 (`mpi_p2_um_por_nucleo.csv`). Com p = 4 já não há escolha: são 4 processos em 2 núcleos físicos, e o SMT rende menos que núcleos de verdade, por isso a eficiência cai para 0,62. Isso não é defeito do algoritmo; é efeito da máquina e de como o `mpirun` distribui os processos.

## Armadilhas

- `Scatter` em vez de `Scatterv`: com `n` não divisível, elementos somem — e o vetor ainda sai ordenado. Só o checksum pega.
- Deadlock: dois processos fazendo `MPI_Send` grande um para o outro ao mesmo tempo. No merge em árvore, cada par tem um só sentido (um envia, o outro recebe).
- Medir só no rank 0: o tempo do programa é o do processo mais lento.
- `-np` maior que `nproc`: precisa de `--oversubscribe` e o tempo piora por motivo alheio ao algoritmo. Meça só até `nproc`.
- Compilar com `g++`: erro `mpi.h: No such file`. O Makefile usa `mpicxx`.

## Perguntas de arguição (sobre esta frente)

1. Qual a diferença entre memória compartilhada (OpenMP) e distribuída (MPI)?
2. Por que `Scatterv` em vez de `Scatter`?
3. Merge em árvore ou `Gather` com merge no rank 0: qual o custo de cada um?
4. Por que o MPI escala pior que o OpenMP numa única máquina?
5. Por que o tempo reportado é o máximo entre os processos?
6. Como o programa garante que o resultado não muda com `-np`?
7. O que é deadlock e como o seu código o evita?
