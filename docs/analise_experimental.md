# Análise experimental: speedup medido × previsto

Problema 07 (mergesort) · CCO085 IESB 2026/2 · conduzida pela frente 1, com OpenMP (frente 2) e MPI (frente 3)

Dados: `resultados/*.csv` (medidos com `scripts/medir.sh` e `scripts/medir_cutoff.sh`), tabela em
`resultados/resumo.md`, gráficos em `graficos/`. Todas as estatísticas são **medianas** de 4
repetições (5 execuções, a 1ª descartada como aquecimento).

## 1. Ambiente da medição

| Item | Valor |
|---|---|
| Data | 12/11/2026, 20:53 |
| CPU | Intel Core i5-13420H (13ª geração, notebook) |
| Núcleos lógicos (`nproc`) | 12 |
| Núcleos físicos | 8: 4 de desempenho (P) com hyperthreading (8 threads) + 4 de eficiência (E), sem hyperthreading (4 threads). Fonte: especificação da Intel |
| Compilador / MPI | g++ 13.3.0 (`-O2`) / OpenMPI 4.1.6 (`mpirun --use-hwthread-cpus --bind-to none`) |
| Repetições | 5 por configuração, 1ª descartada, mediana das 4 restantes |

Baseline, OpenMP e MPI foram medidos **na mesma máquina e na mesma sessão**, com o mesmo script.
Duas características da máquina pesam na análise: os núcleos são **heterogêneos** (um núcleo E é
mais lento que um P) e há só **4 núcleos P**. Com p = 8, parte do trabalho roda obrigatoriamente em
núcleos E ou nas duas threads de hardware de um mesmo núcleo P.

## 2. Corretude

`bash scripts/validar.sh` (modo completo, código final): **112 de 112 casos passaram**, 56 do OpenMP e
56 do MPI (`resultados/validacao.md`). A bateria compara o checksum de cada versão com o do gabarito
para n = 1, 2, 3, 7, 10, 1000, 1.000.003 e 20.000.003 e p = 1, 2, 3, 4, 5, 7, 8, cobrindo n não
divisível por p, n menor que p e p que não é potência de 2. Além disso, o `medir.sh` conferiu o
checksum em **todas** as execuções medidas.

## 3. Previsão (Lei de Amdahl, modelo da árvore)

Paralelizando as chamadas recursivas, os níveis do topo da árvore têm menos merges que threads e
não se dividem; o restante se divide entre as p threads (`docs/analise_baseline.md`, seção 5):

```
S(p) ≈ log₂n / ( (log₂n − log₂p)/p + 2·(1 − 1/p) )        n = 20 M → log₂n ≈ 24,25
```

| p | 1 | 2 | 4 | 8 | 12 | ∞ |
|---|---|---|---|---|---|---|
| Speedup previsto | 1,00 | 1,92 | 3,43 | 5,50 | 6,82 | ≈ 12 |
| Eficiência prevista | 1,00 | 0,96 | 0,86 | 0,69 | 0,57 | → 0 |

O modelo considera só a parte serial do algoritmo; ignora memória, hardware e comunicação.

## 4. Escalabilidade forte (n = 20 M)

Gráficos: `graficos/forte_tempo.png`, `graficos/forte_speedup.png`, `graficos/forte_eficiencia.png`.
T₁ = baseline do professor = **1,935 s**.

| Versão | p | Tempo (s) | Speedup | Eficiência | Previsto | Medido / previsto |
|---|---|---|---|---|---|---|
| OpenMP | 1 | 1,909 | 1,01 | 1,01 | 1,00 | 101% |
| OpenMP | 2 | 1,003 | **1,93** | 0,96 | 1,92 | **100%** |
| OpenMP | 4 | 0,756 | **2,56** | 0,64 | 3,43 | 74% |
| OpenMP | 8 | 0,598 | **3,24** | 0,40 | 5,50 | 59% |
| MPI | 1 | 1,943 | 1,00 | 1,00 | 1,00 | 100% |
| MPI | 2 | 1,042 | **1,86** | 0,93 | 1,92 | **97%** |
| MPI | 4 | 0,623 | **3,11** | 0,78 | 3,43 | 90% |
| MPI | 8 | 0,475 | **4,07** | 0,51 | 5,50 | 74% |

**Como ler:** no gráfico de speedup há três níveis. Entre o **ideal** (S = p) e o **previsto** está a
parte serial do algoritmo (os merges do topo). Entre o **previsto** e o **medido** está o que o
modelo não considera, discutido abaixo.

Com **p = 2 as duas versões atingem a previsão** (100% e 97%): até aí o afastamento do ideal é
explicado só pela árvore. A partir de p = 4 o medido se afasta do previsto, e mais com p = 8.

### 4.1 OpenMP
- Speedup de 1,93 / 2,56 / 3,24 com 2 / 4 / 8 threads.
- **Variabilidade com p = 4:** as 4 repetições se dividem em dois grupos (0,71 e 0,80 s), e na
  varredura de cutoff, também com p = 4, os tempos foram de 0,70 a 0,88 s. Hipótese: o sistema
  operacional ora coloca as 4 threads em núcleos P, ora coloca alguma num núcleo E, mais lento; como
  o `taskwait` faz cada nível esperar a metade mais lenta, uma thread lenta atrasa o todo.
- **Cutoff** (`resultados/cutoff.csv`, p = 4): medianas de 0,81 a 0,86 s para cutoffs de 1.024 a
  65.536, com faixas que se sobrepõem. **As diferenças estão dentro do ruído**: nessa faixa o cutoff
  não muda o tempo de forma mensurável, e o custo de criar tasks não é o fator dominante. O padrão
  de 16.384 está dentro dela.

### 4.2 MPI
- Speedup de 1,86 / 3,11 / 4,07 com 2 / 4 / 8 processos.
- **Comunicação** (`resultados/mpi_comunicacao.csv`: `Scatterv` + envios e esperas da árvore):

| p | Tempo total (s) | Comunicação (s) | Fração |
|---|---|---|---|
| 1 | 1,943 | 0,009 | 0,4% |
| 2 | 1,042 | 0,034 | 3,3% |
| 4 | 0,623 | 0,043 | 6,9% |
| 8 | 0,475 | 0,060 | **12,7%** |

  A fração **quase dobra a cada dobra de p**. A ordenação local custa O((n/p)·log(n/p)) e cai com p;
  distribuir e juntar os dados custa O(n) e não cai. Quanto maior p, mais o tempo é dominado pela
  comunicação, que funciona como uma parte serial adicional.

### 4.3 OpenMP × MPI
Com p ≥ 4, o **MPI foi mais rápido que o OpenMP** (3,11 × 2,56 com p = 4; 4,07 × 3,24 com p = 8),
o contrário do que se esperaria só pelo custo de comunicação. O mesmo apareceu na medição
preliminar em outra máquina (4 núcleos Xeon: 2,94 × 2,25 com p = 4), então o efeito é sistemático,
não ruído. A causa **não foi isolada**; hipóteses, a testar:
- **Localidade de cache:** no MPI, cada processo ordena o seu bloco inteiro na própria memória até a
  fase da árvore. No OpenMP, as tasks são pegas por threads quaisquer, e o merge de um nível costuma
  ser feito por uma thread diferente da que ordenou as metades: os dados passam de um núcleo para
  outro.
- **Posicionamento das threads** nos núcleos P e E (seção 4.1), que no OpenMP mostrou variabilidade
  maior que no MPI.
- **Duas tasks por divisão:** a versão cria uma task para cada metade e a thread criadora espera;
  criar uma só e fazer a outra metade na própria thread reduziria o número de tasks.

Experimentos para separar as causas: fixar as threads (`OMP_PROC_BIND=close`, `OMP_PLACES=cores`),
medir com uma task por divisão e medir faltas de cache com `perf stat`.

## 5. Escalabilidade fraca (n = 5 M × p)

Gráfico: `graficos/fraca_tempo.png`. Na fraca, cada trabalhador fica com 5 M elementos, mas o
merge final cresce com p: com p = 8, um único núcleo intercala 40 M elementos. O modelo da árvore
prevê o crescimento do tempo:

```
T(p) / T(1) = ( log₂(n₀·p) − log₂p + 2·(p − 1) ) / log₂(n₀)        n₀ = 5 M
```

| p | n | OpenMP (s) | MPI (s) | OpenMP / p=1 | MPI / p=1 | Previsto (árvore) | Ideal só pelo log n |
|---|---|---|---|---|---|---|---|
| 1 | 5 M | 0,438 | 0,442 | 1,00 | 1,00 | 1,00 | 1,00 |
| 2 | 10 M | 0,480 | 0,503 | 1,10 | 1,14 | **1,09** | 1,05 |
| 4 | 20 M | 0,746 | 0,645 | 1,70 | 1,46 | **1,27** | 1,09 |
| 8 | 40 M | 1,150 | 1,018 | 2,63 | 2,31 | **1,63** | 1,14 |

- **Com p = 2, as duas versões batem com o modelo** (1,10 e 1,14 × 1,09).
- **A maior parte do crescimento é esperada:** mesmo com paralelismo perfeito nos níveis de baixo,
  o merge do topo é serial e cresce com n, e o modelo já prevê 1,63× com p = 8. O "ideal só pelo
  log n" (a linha tracejada do gráfico) ignora essa parte serial.
- **O que passa do modelo** com p = 4 e 8 cresce nas **duas** versões, o que aponta para uma causa
  comum de hardware: mais dados trafegando pela memória (40 M elementos ocupam 2 × 160 MB) e o uso
  dos núcleos E. Também aqui o MPI ficou à frente do OpenMP.

## 6. Por que o medido se afasta da previsão

| Fator | Evidência nos dados | Conclusão |
|---|---|---|
| **Parte serial (Amdahl)** | p = 2 bate com o modelo nas duas versões (forte e fraca); a fraca segue a forma prevista pela árvore | **Confirmado**: é a base do afastamento do ideal |
| **Núcleos heterogêneos / hyperthreading** | A máquina tem 4 núcleos P; com p = 8 a eficiência cai para 0,40 (OpenMP) e 0,51 (MPI). Tempos do OpenMP com p = 4 em dois grupos | **Provável** para p ≥ 4. Não medido diretamente (exigiria fixar threads por tipo de núcleo) |
| **Banda de memória** | Na fraca, o excesso sobre o modelo cresce com p nas duas versões; o `merge` lê e escreve muito e calcula pouco | **Provável**. Não medido diretamente (exigiria contadores de hardware) |
| **Overhead de comunicação** | Fração do tempo do MPI: 3,3% (p = 2) → 12,7% (p = 8) | **Confirmado** no MPI, crescente com p |
| **Custo de criar tasks** | Varredura de cutoff de 1.024 a 65.536: diferenças dentro do ruído | **Não dominante** nessa faixa |
| **Desbalanceamento de carga** | n = 20 M é divisível por 1, 2, 4 e 8: no MPI os blocos têm exatamente o mesmo tamanho; no OpenMP as metades diferem em no máximo 1 elemento | **Desprezível** |
| **False sharing** | Cada task escreve num trecho grande e contíguo; só as fronteiras podem cair na mesma linha de cache | **Desprezível** |

## 7. Conclusão

As duas versões estão corretas (112 de 112 casos, checksum conferido em todas as execuções) e
aceleram: **3,24× com OpenMP e 4,07× com MPI** usando 8 de 12 núcleos lógicos. O modelo da árvore
explica o comportamento até p = 2 com precisão e dá o teto do algoritmo. Acima disso, o afastamento
vem do hardware (só 4 núcleos P, núcleos E mais lentos, memória compartilhada) e, no MPI, da
comunicação, que chega a 12,7% do tempo com p = 8. O MPI superou o OpenMP nesta máquina, um
resultado sistemático cuja causa exata fica como hipótese a investigar (seção 4.3).

## 8. Como reproduzir

```bash
make
bash scripts/validar.sh          # tem de terminar com "TUDO CERTO"
bash scripts/medir.sh            # baseline + forte + fraca
bash scripts/medir_cutoff.sh     # varredura do cutoff (OpenMP, p = 4)
python3 scripts/graficos.py      # gráficos e resultados/resumo.md
```
