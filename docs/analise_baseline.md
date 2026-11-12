# Análise do baseline: perfil e Lei de Amdahl

Frente 1 · Problema 07 (mergesort) · CCO085 IESB 2026/2

> **Máquinas.** A seção 3 traz o baseline da **medição final** (12/11/2026, Intel Core i5-13420H,
> 12 núcleos lógicos; `resultados/maquina.txt`), a mesma das versões paralelas. O perfil e a análise
> por nível (seções 4 e 5) foram medidos em 05/11/2026 numa máquina de nuvem (Intel Xeon @ 2,10 GHz,
> 4 núcleos): os tempos absolutos diferem, mas as **proporções** (porcentagens, custo por nível,
> speedups previstos) não dependem da máquina.

## 1. O que o programa faz

O baseline gera `n` inteiros pseudoaleatórios com um gerador congruencial linear de semente fixa
(12345), ordena com mergesort recursivo e verifica se o resultado está em ordem. Só a ordenação
é cronometrada.

O `msort` divide o intervalo ao meio, ordena cada metade recursivamente e intercala as duas metades
com `merge`. O `merge` escreve o resultado intercalado no vetor auxiliar `tmp` e depois o copia de
volta para `a`.

| Recurso | Custo | Para n = 20 milhões |
|---|---|---|
| Tempo | O(n log n): log₂n níveis, cada um com trabalho ≈ n | log₂n ≈ 24,25 níveis |
| Memória | O(n): vetores `a` e `tmp` (2 × 4 bytes × n) | 160 MB |
| Profundidade da recursão | O(log n) | ≈ 25 chamadas empilhadas |

## 2. Protocolo de medição

- Compilação com `-O2` (Makefile).
- 5 execuções por configuração; a 1ª é descartada (aquecimento de cache e de páginas de memória);
  reporta-se a **mediana** das 4 restantes.
- Cada execução é validada: `ordenado=SIM` e, nas versões paralelas, checksum igual ao do gabarito
  (`docs/contrato.md`).
- Relógio monotônico (`clock_gettime(CLOCK_MONOTONIC)`), que não é afetado por ajustes do relógio do sistema.
- Tudo automatizado em `scripts/medir.sh`; dados brutos em `resultados/baseline.csv`.

## 3. Baseline medido

Medição final (`resultados/baseline.csv`):

| n | Mediana | Menor – maior (4 repetições) |
|---|---|---|
| 5.000.000 | **0,445 s** | 0,433 – 0,458 s |
| 20.000.000 | **1,935 s** | 1,904 – 1,985 s |
| 80.000.000 | **8,326 s** | 8,293 – 8,414 s |

A dispersão entre repetições do mesmo programa com a mesma entrada chegou a ~6% nesta máquina (e
a ~10% na máquina de nuvem da medição preliminar). Por isso uma execução isolada não serve como
medida, e usamos a mediana.

**A complexidade O(n log n) aparece nos dados.** Ao multiplicar n por 4, um algoritmo O(n) ficaria 4×
mais lento; O(n log n) prevê um pouco mais, porque log n também cresce:

| Passo | Previsto: 4 × log₂(4n) / log₂n | Medido |
|---|---|---|
| 5 M → 20 M | 4 × 24,25 / 22,25 = **4,36×** | 1,935 / 0,445 = **4,35×** |
| 20 M → 80 M | 4 × 26,25 / 24,25 = **4,33×** | 8,326 / 1,935 = **4,30×** |

## 4. Perfilamento: onde o tempo é gasto

### 4.1 Por função (gprof)

`make perfil && ./bin/mergesort_seq_pg N && gprof -b -p bin/mergesort_seq_pg gmon.out`

| Função | n = 20 M | n = 80 M | Chamadas (20 M) |
|---|---|---|---|
| `merge` | **96,5%** | **96,7%** | 19.999.999 |
| `msort` (só a recursão, sem os merges) | 1,9% | 2,0% | 1 (+ chamadas recursivas) |
| `main` (gerador + verificação) | 1,5% | 1,3% | — |

O `merge` é chamado n − 1 vezes, uma para cada nó interno da árvore de recursão.

Sobre o método:
- O gprof interrompe o programa ~100 vezes por segundo e anota em que função ele estava. O resultado
  é uma **amostragem**, e as porcentagens variam um pouco entre execuções.
- Com `-O2`, o compilador embute funções umas nas outras (*inlining*) e cria clones, e o gprof
  atribuiria o tempo à função errada. O alvo `perfil` do Makefile desliga só esses comportamentos.
  Ele não usa `-fno-inline`, que desligaria também o inlining de `vector::operator[]` e deixaria o
  programa ~2× mais lento, medindo outra coisa.
- O gprof só enxerga funções **do nosso programa** compiladas com `-pg`. O tempo gasto na biblioteca
  do C (o `memset` que zera os vetores na alocação) e no sistema operacional (39 mil *page faults*
  no primeiro acesso à memória) não aparece. Por isso a seção 4.2 mede os trechos diretamente.

### 4.2 Por trecho do programa

`./bin/perfil_trechos 20000000` (mediana de 3 execuções)

| Trecho | Tempo | % do programa | Cronometrado pelo baseline? |
|---|---|---|---|
| Gerar a entrada (alocar e zerar vetores + gerador) | 0,102 s | 3,5% | Não |
| **Ordenar (`msort`)** | **2,774 s** | **95,8%** | **Sim** |
| Verificar a ordenação | 0,018 s | 0,6% | Não |
| Total | 2,896 s | 100% | |

O tempo de referência para o speedup é o do trecho **ordenar**, o único que o baseline cronometra.
As versões paralelas cronometram o mesmo trecho (`docs/contrato.md`, seção 5).

### 4.3 Por nível da árvore de recursão

O mesmo programa cronometra os merges de cada nível. O nível 0 é o merge final, com n elementos. Só
os 12 níveis de cima são cronometrados individualmente: cronometrar milhões de merges minúsculos
custaria mais do que os próprios merges.

| Nível | Merges | Tamanho de cada merge | Tempo (mediana) | % de ordenar | ns por elemento |
|---|---|---|---|---|---|
| 0 | 1 | 20.000.000 | 0,124 s | 4,5% | 6,2 |
| 1 | 2 | 10.000.000 | 0,126 s | 4,6% | 6,3 |
| 2 | 4 | 5.000.000 | 0,122 s | 4,4% | 6,1 |
| 3 | 8 | 2.500.000 | 0,121 s | 4,4% | 6,1 |
| 4 | 16 | 1.250.000 | 0,124 s | 4,5% | 6,2 |
| 5 | 32 | 625.000 | 0,114 s | 4,1% | 5,7 |
| 6 | 64 | 312.500 | 0,115 s | 4,2% | 5,8 |
| 7 | 128 | 156.250 | 0,108 s | 3,9% | 5,4 |
| 8 | 256 | 78.125 | 0,109 s | 3,9% | 5,5 |
| 9 | 512 | 39.062 | 0,103 s | 3,7% | 5,2 |
| 10 | 1.024 | 19.531 | 0,103 s | 3,7% | 5,1 |
| 11 | 2.048 | 9.765 | 0,107 s | 3,8% | 5,3 |
| 12 a 24 | — | — | 1,395 s | 50,3% | ≈ 5,7 por nível (inclui o custo das chamadas recursivas) |

Duas observações:
1. **Cada nível custa praticamente o mesmo** (≈ 4% do tempo de ordenação). É a confirmação
   experimental de que cada nível da árvore faz trabalho ≈ n.
2. **Os níveis de cima são ~15–20% mais caros por elemento** (≈ 6,2 ns contra ≈ 5,2 ns). Nos níveis
   de cima, cada merge trabalha sobre dezenas de MB, que não cabem na cache, e os dados vêm da RAM.
   Nos níveis de baixo, cada merge cabe na cache. Com 1 thread a diferença é modesta: o custo por
   elemento é dominado pela comparação e pelo desvio condicional de cada passo do merge, não pela
   memória. Com várias threads fazendo merge ao mesmo tempo, a demanda por banda de memória se
   multiplica; essa hipótese será testada na curva do OpenMP.

**Ponto quente (hotspot):** o `merge`, responsável por ~96% do tempo, distribuído igualmente entre
os ~24 níveis da árvore.

## 5. Fração paralelizável e Lei de Amdahl

### 5.1 "96% do tempo no `merge`" não significa f = 0,96

O perfil diz **onde** está o tempo, não **quanto dele pode rodar em paralelo**. O paralelismo natural
do mergesort é executar as duas chamadas recursivas ao mesmo tempo. Com esse paralelismo:
- no nível 0 há **1** merge: só 1 thread trabalha, mesmo que existam 4;
- no nível 1 há **2** merges: só 2 threads trabalham;
- do nível log₂p para baixo, há merges suficientes para todas as p threads.

Os merges do topo da árvore são a parte sequencial do problema. O tamanho dela depende de p.

### 5.2 Modelo teórico (todos os níveis com o mesmo custo)

Com L = log₂n níveis de custo ≈ n cada e p threads:
- níveis k = 0 … log₂p − 1: há 2ᵏ < p merges, que rodam em paralelo; custo n/2ᵏ cada nível,
  somando n · (1 + 1/2 + … ) = **2n · (1 − 1/p)**;
- os outros L − log₂p níveis se dividem entre as p threads: **n/p** cada.

```
S(p) = L / ( (L − log₂p)/p + 2·(1 − 1/p) )          S(∞) = L / 2
```

### 5.3 Modelo com os tempos medidos por nível

O mesmo raciocínio, usando os tempos reais da seção 4.3 (Tₖ = tempo do nível k): os níveis
k < log₂p custam Tₖ/2ᵏ; os demais, Tₖ/p. Isso já incorpora o fato de os níveis de cima serem um
pouco mais caros.

### 5.4 Speedup previsto (n = 20 M)

| p | S teórico (5.2) | Eficiência | **S com tempos medidos (5.3)** | Eficiência | Fração serial equivalente |
|---|---|---|---|---|---|
| 2 | 1,92 | 0,96 | **1,92** | 0,96 | 4,3% |
| 4 | 3,43 | 0,86 | **3,40** | 0,85 | 5,9% |
| 8 | 5,50 | 0,69 | **5,37** | 0,67 | 7,0% |
| 16 | 7,72 | 0,48 | **7,41** | 0,46 | 7,7% |
| ∞ | 12,13 | — | **11,23** | — | 8,9% |

A "fração serial equivalente" é o valor de (1 − f) que, posto na fórmula clássica de Amdahl
`S(p) = 1 / ((1 − f) + f/p)`, reproduz o speedup previsto.

**Conclusão para o Marco:** a fração paralelizável do trecho cronometrado é **f ≈ 0,91** (fração
serial ≈ 9%, que são os merges do topo da árvore), e o **speedup máximo previsto é ≈ 11**.
Com os 4 núcleos desta máquina, o teto é **S(4) ≈ 3,4**.

Repare que a fração serial equivalente **cresce com p**. No mergesort, a parte serial não é um
trecho fixo de código: quanto mais threads, mais níveis do topo ficam com menos merges do que
threads. A fórmula clássica de Amdahl supõe uma fração fixa; o modelo por níveis é mais fiel a este
algoritmo.

### 5.5 O que o modelo ignora (por que o real deve ficar abaixo)

| Fator | Efeito esperado | Em qual versão |
|---|---|---|
| Banda de memória compartilhada | Várias threads fazendo merge disputam o mesmo barramento de RAM | OpenMP e MPI |
| Custo de criar e escalonar tasks | Sem um tamanho mínimo (cutoff), o gerenciamento domina | OpenMP |
| Comunicação | Distribuir e juntar os dados custa O(n), comparável ao trabalho por nível | MPI |
| Desbalanceamento | Pedaços de tamanhos diferentes quando n não é divisível por p | MPI |

Esses fatores serão quantificados na etapa final, comparando as curvas medidas com a coluna
"S com tempos medidos" acima.

## 6. Implicações para o plano de paralelização

- **OpenMP:** paralelizar as duas chamadas recursivas com `task` + `taskwait`, com um cutoff abaixo do
  qual a recursão segue sequencial. O teto é a coluna 5.4; o merge final continua sequencial.
- **MPI:** dividir o vetor em p blocos (`Scatterv`), ordenar cada bloco localmente (os níveis de baixo,
  perfeitamente paralelos) e juntar com merge em árvore (os níveis de cima, com a mesma limitação do
  item 5.1 somada ao custo de enviar os dados).
- Melhoria opcional, fora do escopo mínimo: um merge paralelo (dividir o próprio merge entre threads
  por busca binária) atacaria a fração serial do topo.

## 7. Como reproduzir

```bash
make
bash scripts/medir.sh baseline                        # seção 3
make perfil && ./bin/mergesort_seq_pg 20000000 && gprof -b -p bin/mergesort_seq_pg gmon.out   # 4.1
./bin/perfil_trechos 20000000                         # 4.2 e 4.3
```
