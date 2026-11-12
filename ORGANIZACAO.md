# Organização do trio

Guia de trabalho do projeto. Leia este arquivo primeiro; depois, o guia da sua frente em `docs/`.

| Arquivo | Para quê |
|---|---|
| `ORGANIZACAO.md` | Este guia: papéis, fluxo de trabalho, cronograma, regras |
| `docs/contrato.md` | Decisões comuns às três frentes (entrada, validação, saída, CSV, medição). **Fonte única da verdade** |
| `docs/frente1_baseline.md` | Passo a passo da frente 1: baseline, perfil, Amdahl, scripts e gráficos |
| `docs/frente2_openmp.md` | Passo a passo da frente 2: versão OpenMP |
| `docs/frente3_mpi.md` | Passo a passo da frente 3: versão MPI |
| `AGENTS.md` / `CLAUDE.md` | Instruções para ferramentas de IA que forem usadas no repositório |

## 1. Quem faz o quê

| Frente | Responsável | Entrega principal |
|---|---|---|
| 1. Baseline, perfil e análise | Marcelo (MarceloCPJunior) | Tempos do baseline, perfil gprof, estimativa de Amdahl, `scripts/medir.sh`, `scripts/graficos.py` |
| 2. OpenMP | David Biage da Silva e Thiago PP | `openmp/mergesort_omp.cpp`, estudo do cutoff, tratamento de corrida, curva por threads |
| 3. MPI | Gabriel Paiva (GabrielPaivaM) | `mpi/mergesort_mpi.cpp`, coletivas, overhead de comunicação, curva por processos |

Os mesmos nomes estão no `AUTORES.md`, exigido pelo enunciado.

**Todos precisam entender o projeto inteiro.** Na arguição, cada um responde sobre a frente de **outro** colega, sem IA e sem ajuda. A especialização é para dividir o trabalho, não o conhecimento.

## 2. O que é feito em conjunto

Estas partes são decididas e escritas pelo trio, na **semana 1**, antes das frentes começarem a medir:

1. **Código comum** (`comum/mergesort_comum.hpp`): gerador da entrada, `merge`, `msort`, relógio e checksum. OpenMP e MPI incluem o mesmo arquivo.
2. **Validação**: checksum que bate com o do baseline.
3. **Contrato de saída e de medição**: formato da linha impressa, colunas do CSV, o que entra no cronômetro, tamanhos de `n`.

Tudo isso está especificado em `docs/contrato.md`. Se alguém precisar mudar o contrato, abre um PR só para isso e os outros dois aprovam.

## 3. Fluxo de trabalho no Git

O enunciado avalia o histórico de commits e penaliza "commit único" ou "trabalho concentrado numa pessoa". Por isso:

- **Cada um commita com a própria conta do GitHub**, no próprio computador.
- **Uma branch por frente:**
  - `frente1/baseline` (scripts, gráficos, análise)
  - `frente2/openmp`
  - `frente3/mpi`
  - `comum/contrato` (código comum da semana 1)
- **Nada vai direto para `main`.** Abra um pull request e peça revisão.
- **Revisão em rodízio** (revisar obriga a entender o código do colega antes da arguição):

  | Autor do PR | Quem revisa |
  |---|---|
  | Frente 1 (Marcelo) | Frente 3 (Gabriel Paiva) |
  | Frente 2 (David Biage da Silva e Thiago PP) | Frente 1 (Marcelo) |
  | Frente 3 (Gabriel Paiva) | Frente 2 (David Biage da Silva e Thiago PP) |

- **Commits pequenos e frequentes**, com mensagem em português dizendo o que mudou e por quê. Exemplo: `OpenMP: adiciona cutoff de 10000 após varredura (ver resultados/cutoff.csv)`.
- **Antes de abrir um PR:** `make` sem warnings, e a validação do `docs/contrato.md` passando.

Comandos do dia a dia:

```bash
git checkout main && git pull            # atualiza
git checkout -b frente2/openmp           # primeira vez
git checkout frente2/openmp              # nas próximas
git merge main                           # traz o que os outros já integraram
git add <arquivos> && git commit -m "..."
git push -u origin frente2/openmp        # depois abre o PR no GitHub
```

## 4. Cronograma

| Período | Frente 1 | Frente 2 | Frente 3 |
|---|---|---|---|
| **22/10 – 28/10** | Baseline em 3 tamanhos, perfil, rascunho de Amdahl. **Puxa a reunião do contrato** | Estuda tasks; escreve o checksum em `comum/` | Estuda `02_mpi/01–04`; protótipo `Scatterv` → sort → `Gatherv` |
| **29/10 – 04/11** | `medir.sh` e `graficos.py` funcionando; revisa o OpenMP | Versão com tasks + varredura de cutoff; revisa o MPI | Merge em árvore, `n` não divisível, tempo de comunicação; revisa os scripts |
| **05/11 – 09/11** | Roda todas as medições na mesma máquina multinúcleo | Texto: saturação, false sharing, escalonamento | Texto: overhead de comunicação × p |
| **10/11 – 11/11** | Gráficos finais e discussão de Amdahl | Revisão final | Revisão final |
| **12/11** | **Entrega do Marco 2**: README que roda do zero, AUTORES preenchido | | |
| **13/11 – 18/11** | Ensaio da demonstração (~9 min) e simulados de arguição cruzada | | |

Datas do enunciado: escala das apresentações em **05/11**; apresentação em **19/11** ou **26/11**.

## 5. Definição de "pronto" para cada frente

Uma frente só está pronta quando:

- [ ] o código compila com `make` sem warnings, no contêiner da disciplina;
- [ ] imprime a linha no formato do contrato, com `ordenado=SIM` e o **mesmo checksum do baseline** para todos os `n`, threads e processos testados;
- [ ] os dados de medição estão em `resultados/*.csv`, gerados pelo script e não digitados à mão;
- [ ] as decisões estão justificadas por escrito no guia da frente (seção "Decisões");
- [ ] outro membro revisou o PR e consegue explicar o código sem ajuda.

## 6. Como usar IA sem se prejudicar

O enunciado permite IA para escrever código, mas a defesa é individual e sem IA. Um código que você não sabe explicar zera a sua nota de arguição. Regras do trio:

1. **Peça explicação, não só código.** Bom pedido: "explique por que o `taskwait` é necessário aqui e o que aconteceria sem ele". Mau pedido: "faz a versão OpenMP".
2. **Nunca aceite um número que você não mediu.** Tempos e speedups vêm do `scripts/medir.sh`, rodado por vocês.
3. **Antes de commitar código gerado por IA, consiga responder:** o que cada linha faz? Por que essa escolha e não outra? Como eu sei que está certo?
4. O arquivo `AGENTS.md` já instrui a IA a seguir o contrato, não mexer no baseline e explicar cada decisão. Se a ferramenta não ler esse arquivo sozinha, cole o conteúdo dele no início da conversa.

## 7. Estudo e simulados

- Material base: aulas 1 a 3 para todos; aulas 4 e 5 aprofundadas pela frente correspondente (página "Aulas base do projeto mergesort", compartilhada pelo Marcelo).
- **Simulado semanal:** cada um responde, sem consulta, uma pergunta da seção "Perguntas de arguição" do guia de **outra** frente.
