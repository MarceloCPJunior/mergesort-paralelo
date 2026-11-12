# Frente 1 — Baseline, perfil e análise

Responsável: Marcelo (MarceloCPJunior) · Revisor dos seus PRs: frente 3 (Gabriel Paiva) · Você revisa: frente 2, OpenMP (David Biage da Silva e Thiago PP)

## Objetivo

Estabelecer o tempo de referência, mostrar onde o programa gasta tempo, prever o ganho máximo com Amdahl e produzir a infraestrutura de medição e os gráficos que as três frentes usam. Valor no artefato: **1,0 ponto** (baseline, perfilamento e Amdahl), e você conduz boa parte do critério de análise experimental (outro 1,0).

## Entregas

- [ ] Tabela do baseline para 5 M, 20 M e 80 M, com o protocolo do contrato. *(preliminar em `resultados/baseline.csv`, máquina de nuvem de 4 núcleos; refazer na máquina final)*
- [x] Tabela de perfil (gprof): % de tempo por função. Mais: tempo por trecho e por nível da árvore (`sequencial/perfil_trechos.cpp`). Ver `docs/analise_baseline.md`.
- [x] Estimativa de Amdahl com o modelo da árvore (aula 3) para p = 2, 4, 8 e o teto S(∞), teórica e com os tempos medidos por nível (`docs/analise_baseline.md`, seção 5).
- [x] `sequencial/gabarito.cpp` e os checksums de referência registrados em `docs/contrato.md`.
- [x] `scripts/medir.sh`: roda seq, OpenMP e MPI, grava os CSVs do contrato (experimentos `baseline`, `forte`, `fraca`; a varredura de cutoff fica com a frente 2).
- [x] `scripts/validar.sh`: bateria de corretude (n pequenos, n não divisível por p, n < p, p = 1 a 8) com relatório em `resultados/validacao.md`.
- [x] `scripts/graficos.py`: tempo, speedup e eficiência (forte), tempo (fraca), com a curva de Amdahl prevista sobreposta. Curvas de OpenMP e MPI aparecem quando houver dados.
- [ ] Texto da análise: real × Amdahl, e por que se afastam. *(esqueleto pronto em `docs/analise_experimental.md`; falta preencher com a medição final, que depende do OpenMP e do MPI)*

## Passo a passo

### 1. Entender o baseline (dias 1–2)
- Leia `sequencial/mergesort_seq.cpp` até conseguir explicar cada linha do `merge`.
- Desenhe a árvore de recursão para n = 8 e conte quantas comparações cada nível faz.

### 2. Medir o baseline
```bash
make seq
for n in 5000000 20000000 80000000; do
  for r in 1 2 3 4 5; do ./bin/mergesort_seq $n; done
done
```
Descarte a 1ª de cada `n`, use a mediana. Anote `nproc` e `lscpu | grep "Model name"`. Esse loop vira a primeira parte do `medir.sh`.

### 3. Perfilar
```bash
make perfil
./bin/mergesort_seq_pg 20000000
gprof -b -p bin/mergesort_seq_pg gmon.out
```
Esperado: `merge` ≈ 96%, `msort` ≈ 3%. Saiba explicar por que o Makefile usa aquelas flags (`PGFLAGS`) e não `-fno-inline`.

### 4. Amdahl com o modelo da árvore
```
S(p) ≈ log₂n / ( (log₂n − log₂p)/p + 2·(1 − 1/p) )
```
Para n = 20 M: S(2) ≈ 1,92, S(4) ≈ 3,43, S(8) ≈ 5,50, S(∞) ≈ 12. Refaça a conta você mesmo e explique de onde vem cada termo.

### 5. Gabarito e checksum (com o trio)
Escreva `sequencial/gabarito.cpp` incluindo `../comum/mergesort_comum.hpp`, rode para os três tamanhos e preencha a tabela do contrato.

### 6. `scripts/medir.sh`
Use como modelo o `material_referencia/ambiente_pp/scripts/medir_speedup.sh` do professor (descarte da 1ª repetição, `nproc`, CSV). Ele precisa:
- rodar cada configuração 5 vezes e gravar da 2ª em diante;
- extrair `tempo=` da linha do contrato;
- **conferir o checksum** em toda execução e abortar se divergir do gabarito;
- não passar de `nproc` threads/processos.

### 7. `scripts/graficos.py`
pandas + matplotlib. Agrupe por `(versao, n, p)` e use a **mediana**. Gráficos mínimos:
1. tempo × p (forte), OpenMP e MPI no mesmo gráfico;
2. speedup × p, com a reta ideal (S = p) e a curva de Amdahl prevista;
3. eficiência × p;
4. tempo × p na escalabilidade fraca.

## Decisões e justificativas

| Decisão | Escolha | Por quê |
|---|---|---|
| Número de repetições | 5 por configuração, descartando a 1ª | Um tempo é uma observação com ruído (medimos ~5–10% de variação entre execuções iguais). A 1ª pode pegar a CPU em baixa frequência ou a cache fria; descartá-la é um seguro barato |
| Mediana ou média | Mediana das 4 restantes | Uma execução isolada atrapalhada pelo sistema (ex.: atualização em segundo plano) puxa a média, mas não muda a mediana |
| Tamanhos de n | 5 M, 20 M e 80 M (fatores de 4) | O enunciado pede pelo menos três; fatores iguais deixam ver o crescimento n·log n (medido: ~4,3× a cada ×4). 20 M é o n da escalabilidade forte; 5 M é a base da fraca |
| Trecho cronometrado | Só a ordenação (`msort`) | É o que o baseline do professor cronometra; todas as versões medem o mesmo trecho. No MPI, distribuir e juntar os dados entra, porque é custo de paralelizar |
| Relógio | `clock_gettime(CLOCK_MONOTONIC)` | Só anda para frente; ajustes do relógio do sistema não estragam a medida |
| Compilação | `-O2 -Wall` para tudo, via Makefile | Sem otimização o código fica ~4× mais lento (medido) e o speedup sai inflado; `-Wall` mostra todos os avisos |
| T₁ do speedup | Tempo do baseline do professor | A versão paralela com 1 thread carrega o custo do runtime; usá-la como referência inflaria o speedup |
| p testados na medição | 1, 2, 4, 8… só até `nproc` | Acima disso as threads dividem núcleos e o tempo piora por motivo alheio ao algoritmo |
| Validação | Checksum Σ (i+1)·a[i] em `uint64_t`, comparado ao gabarito em toda execução | `ordenado=SIM` não detecta elementos perdidos ou duplicados. O peso de posição pega trocas e erros que se compensam; i+1 evita o ponto cego da posição 0; inteiro sem sinal dá o mesmo valor em qualquer ordem de soma (`double` não daria) |
| Confiança no gabarito | Mesmo algoritmo do professor + comparação completa com `std::sort` | Duas ordenações independentes que concordam posição por posição |
| Código comum | `comum/mergesort_comum.hpp`, conferido com `diff` contra o baseline | O arquivo do professor tem `main` e não pode ser incluído nem alterado; copiar o `merge` em cada versão arriscaria divergências |
| Flags do perfil | `-pg` com inlining de funções inteiras e clones (IPA) desligados, mas sem `-fno-inline` | O `-O2` cola o `merge` dentro do `msort` e o gprof erraria a atribuição; `-fno-inline` também desligaria o inlining de `vector::operator[]` e mediria um programa ~2× mais lento |
| Bateria de corretude | `validar.sh` com n = 1…1000, 1.000.003 e 20.000.003, p = 1…8 | Os bugs de divisão se escondem em n divisível por p; esses n não são divisíveis por nenhum p de 2 a 8, e n < p testa processos sem elementos |

## Armadilhas

- Medir em máquina com 1 ou 2 núcleos: não aparece speedup. Confira `nproc` no contêiner e aumente as CPUs no Docker Desktop.
- Confundir "96% do tempo em `merge`" com "96% paralelizável".
- Na escalabilidade fraca, esperar tempo constante: o trabalho é n log n, então cresce um pouco a cada dobra de n.
- Gráfico sem a reta ideal e sem Amdahl: não mostra o afastamento que o enunciado pede para discutir.

## Perguntas de arguição (sobre esta frente)

1. O gprof diz que `merge` é 96% do tempo. Por que isso não significa que 96% do programa é paralelizável?
2. Por que o speedup é calculado contra o baseline e não contra a versão paralela com 1 thread?
3. Por que mediana e não média? Por que descartar a primeira execução?
4. De onde vem o teto S(∞) ≈ 12 para n = 20 M?
5. Qual a diferença entre escalabilidade forte e fraca? O que cada gráfico mostra?
6. Por que o speedup real fica abaixo até da previsão de Amdahl?
