# Roteiro de demonstração

Testes para rodar e mostrar ao professor, na ordem. Para cada um: comando, saída esperada e o que
ele prova.

- Os **checksums** são determinísticos: têm de dar **exatamente** os números abaixo em qualquer máquina.
- Os **tempos** dependem da máquina. Os de referência abaixo são da medição final (i5-13420H, 12
  núcleos lógicos).
- **p (threads ou processos) só até o `nproc`** da máquina da demonstração.

## 0. Preparação

```bash
git checkout main && git pull
nproc            # quantos núcleos lógicos; o speedup precisa de vários
make             # compila as 5 versões
ls bin/          # gabarito  mergesort_mpi  mergesort_omp  mergesort_seq  perfil_trechos
```

Se o `nproc` der 1 ou 2 no contêiner: Docker Desktop → Settings → Resources → CPUs, e reabrir.

## 1. As três versões chegam ao mesmo resultado

```bash
./bin/gabarito 20000000
OMP_NUM_THREADS=4 ./bin/mergesort_omp 20000000 4
mpirun -np 4 ./bin/mergesort_mpi 20000000
```

**Esperado:** as três linhas com `ordenado=SIM` e **`checksum=10913123743811272712`**; o gabarito
também com `confere_std_sort=SIM`.

**Dizer:** "Os três ordenaram o mesmo vetor de 20 milhões de números: um núcleo, 4 threads com
memória compartilhada e 4 processos com memória separada. Os três deram o mesmo checksum, que
depende de cada valor e da sua posição: as versões paralelas chegam exatamente ao resultado do
baseline."

## 2. As versões paralelas aceleram

```bash
./bin/mergesort_seq 20000000
for p in 1 2 4 8; do OMP_NUM_THREADS=$p ./bin/mergesort_omp 20000000 $p; done
for p in 1 2 4 8; do mpirun --use-hwthread-cpus --bind-to none -np $p ./bin/mergesort_mpi 20000000; done
```

**Referência** (medição final, n = 20 M; baseline 1,935 s):

| p | OpenMP | MPI | Previsto (Amdahl) |
|---|---|---|---|
| 2 | 1,00 s (1,93×) | 1,04 s (1,86×) | 1,92× |
| 4 | 0,76 s (2,56×) | 0,62 s (3,11×) | 3,43× |
| 8 | 0,60 s (3,24×) | 0,48 s (4,07×) | 5,50× |

Uma execução isolada varia; os números oficiais são medianas do `medir.sh`.

**Dizer:** "Com 2 núcleos o speedup bate com a previsão de Amdahl. Acima disso fica abaixo: os merges
do topo da árvore são seriais, a máquina tem só 4 núcleos de desempenho e, no MPI, a comunicação
cresce com o número de processos."

No MPI, os campos depois de `tempo=` mostram a divisão do tempo: `t_scatter` (distribuir),
`t_sort_local` (ordenar cada pedaço), `t_arvore_comunic` e `t_arvore_merge` (juntar).

## 3. A bateria de corretude

```bash
bash scripts/validar.sh rapido     # ~15 s
```

**Esperado:** as matrizes do OpenMP e do MPI só com ✓ e `TUDO CERTO: 84 de 84 casos passaram`.

**Dizer:** "Testa n pequenos e p de 1 a 8, inclusive n menor que p e n não divisível por p, onde os
erros de divisão aparecem. A versão completa (`bash scripts/validar.sh`, ~1 min) inclui n de 1 milhão
e 20 milhões e deu 112 de 112."

## 4. O gráfico de speedup

Abrir `graficos/forte_speedup.png` (também aparece no README do GitHub).

**Dizer:** "Três níveis: o ideal (S = p), o previsto pela árvore e o medido. A distância entre o
ideal e o previsto é a parte serial do algoritmo; entre o previsto e o medido, o que o modelo não
considera: hardware e comunicação."

## 5. (Opcional) O cutoff do OpenMP

```bash
OMP_NUM_THREADS=4 ./bin/mergesort_omp 20000000 4 1024
OMP_NUM_THREADS=4 ./bin/mergesort_omp 20000000 4 65536
```

O terceiro argumento é o cutoff: abaixo dele, a recursão segue sequencial. Na medição, de 1.024 a
65.536 as diferenças ficaram dentro do ruído.

## 6. (Opcional) Medir e gerar os gráficos ao vivo

```bash
bash scripts/medir.sh baseline 3 && bash scripts/medir.sh forte 3     # ~2 min
python3 scripts/graficos.py
```

Meça o baseline **na mesma rodada**: o speedup usa o tempo do baseline como T₁, e um baseline de
outra sessão ou máquina distorce a conta.

Para medir tudo do jeito oficial: `bash scripts/medir.sh` (~5 min) e depois `python3 scripts/graficos.py`.

## 7. Se pedirem o perfil (etapa 3)

```bash
make perfil && ./bin/mergesort_seq_pg 20000000 && gprof -b -p bin/mergesort_seq_pg gmon.out | head -8
```

`merge` ≈ 96% do tempo. Frase-chave: "o perfil diz onde está o tempo, não quanto dele é
paralelizável: o merge do topo da árvore roda numa thread só."

## Depois de qualquer teste: limpar

Os testes 3 e 6 reescrevem arquivos em `resultados/` e `graficos/`. Para voltar ao que está no
repositório (a medição oficial):

```bash
git checkout -- resultados/ graficos/
git status        # deve dizer "nothing to commit, working tree clean"
```

Só faça commit desses arquivos se for uma nova medição oficial.

## Perguntas prováveis

| Pergunta | Resposta curta |
|---|---|
| Como sabem que as versões estão corretas? | Checksum com peso de posição, igual ao do gabarito, em 112 casos (vários n e p) e em toda execução medida |
| Por que o speedup não é 4 com 4 núcleos? | O merge do topo roda numa thread só (Amdahl prevê 3,43); acima disso, hardware e comunicação |
| Por que o MPI foi mais rápido que o OpenMP? | Resultado sistemático, causa não isolada; hipóteses: localidade de cache, posição das threads nos núcleos P e E, número de tasks |
| Qual o T₁ do speedup? | O tempo do baseline do professor, não o da versão paralela com 1 thread |
| Por que mediana e descartar a 1ª? | A 1ª é aquecimento; a mediana ignora execuções atrapalhadas por fatores externos |
| Por que `Scatterv` no MPI? | Com n não divisível por p, o `Scatter` perderia `n % p` elementos |
| Por que `taskwait` no OpenMP? | O merge só pode começar com as duas metades prontas |

## Problemas comuns

| Sintoma | Solução |
|---|---|
| `git pull` reclama de arquivos em `resultados/` | `git checkout -- resultados/` e apagar os CSVs novos que os scripts criaram |
| `mpirun` recusa rodar como root | No contêiner do projeto já está liberado; fora dele: `export OMPI_ALLOW_RUN_AS_ROOT=1 OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1` |
| `$'\r': command not found` | `sed -i 's/\r$//' scripts/*.sh` (quebra de linha do Windows) |
| `Permission denied` em `./scripts/...` | Use `bash scripts/...` |
| `nproc` mostra 1 ou 2 | Docker Desktop → Settings → Resources → CPUs |
