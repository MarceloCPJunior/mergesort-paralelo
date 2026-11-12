# Frente 2 — MergeSort paralelo com OpenMP

## 1. Objetivo

Implementar e avaliar uma versão paralela do MergeSort utilizando OpenMP,
preservando a lógica sequencial de intercalação e ordenação nas folhas.

A implementação utiliza tarefas recursivas para explorar o paralelismo
entre as duas metades do vetor.

## 2. Estratégia de paralelização

O vetor é dividido recursivamente em duas regiões:

- região esquerda: [lo, mid);
- região direita: [mid, hi).

Cada região pode ser processada por uma tarefa independente.
Após a conclusão das duas tarefas, o algoritmo realiza a intercalação
das metades ordenadas.

## 3. Diretivas OpenMP utilizadas

| Diretiva | Aplicação | Justificativa |
|---|---|---|
| parallel | Região paralela principal | Disponibiliza equipe de threads |
| single | Inicialização da recursão | Evita iniciar o MergeSort várias vezes |
| task | Chamadas recursivas esquerda e direita | Permite execução concorrente das regiões |
| taskwait | Antes do merge da região pai | Garante que as duas regiões terminaram |

## 4. Condições de corrida e sincronização

As tarefas esquerda e direita operam em intervalos disjuntos do vetor.
Consequentemente, não escrevem simultaneamente nas mesmas posições
de a ou tmp.

A diretiva taskwait garante que as tarefas filhas terminem antes
da intercalação realizada pela tarefa pai.

Não foi necessário utilizar critical ou atomic na ordenação,
pois o algoritmo não possui atualizações concorrentes sobre uma
mesma posição de memória.

### Escolha entre critical, atomic e reduction

| Recurso | Uso | Decisão |
|---|---|---|
| critical | Exclusão mútua de uma região | Não utilizado na ordenação; criaria serialização desnecessária |
| atomic | Proteção de uma operação simples | Não utilizado; não existem incrementos concorrentes compartilhados no algoritmo |
| reduction | Combinação de resultados privados | Não utilizado; checksum é calculado após a região paralela |

A ausência dessas diretivas na ordenação é uma decisão baseada na
estrutura do algoritmo, não uma ausência de sincronização.
A sincronização necessária é realizada com taskwait.

## 5. Estratégias de escalonamento

| Estratégia | Funcionamento | Aplicabilidade |
|---|---|---|
| static | Distribuição previamente definida de iterações | Adequada a loops paralelos regulares |
| dynamic | Distribuição de blocos conforme threads ficam disponíveis | Útil quando iterações têm custos diferentes |
| guided | Blocos inicialmente maiores e progressivamente menores | Busca equilibrar overhead e distribuição |
| tasks | Unidades de trabalho independentes agendadas pelo runtime | Adequada à recursão do MergeSort |

A implementação utiliza tasks, pois o MergeSort apresenta uma estrutura
recursiva de divisão em subproblemas.

Static, dynamic e guided são políticas de schedule para loops;
não são substitutos diretos das tarefas recursivas utilizadas aqui.

## 6. Cutoff e granularidade

O cutoff determina quando a recursão deixa de gerar novas tarefas
e passa a executar o MergeSort sequencial.

Cutoff pequeno:
- permite maior quantidade de tarefas;
- aumenta o paralelismo potencial;
- pode elevar o overhead de criação e gerenciamento.

Cutoff grande:
- reduz a quantidade de tarefas;
- diminui o overhead;
- pode limitar o paralelismo disponível.

O valor adequado depende do tamanho da entrada, da máquina e do
custo de gerenciamento das tarefas.

## 7. Metodologia experimental do cutoff

O experimento utiliza:
- N = 20.000.000 elementos;
- P = 4 threads;
- cutoffs = 1024, 4096, 16384 e 65536;
- uma execução de aquecimento;
- quatro execuções consideradas;
- mediana das quatro execuções válidas.

A corretude é conferida pelo checksum do gabarito sequencial.

Os resultados são registrados em resultados/cutoff.csv.

## 8. Resultados

Os resultados experimentais serão apresentados na tabela de cutoff,
permitindo comparar o tempo de execução para cada granularidade.

## 9. Conclusão

A implementação explora o paralelismo recursivo com tasks e utiliza
taskwait para garantir a dependência necessária antes da intercalação.

O cutoff controla a granularidade das tarefas e permite avaliar
experimentalmente o equilíbrio entre paralelismo e overhead.

## 8.1 Resultados do experimento de cutoff

Experimento realizado com N = 20.000.000 e P = 4 threads.
Foi descartada a primeira execução de cada configuração e calculada
a mediana das quatro execuções restantes.

| Cutoff | Mediana (s) |
|---:|---:|
| 1024 | 0.828756 |
| 4096 | 0.820618 |
| 16384 | 0.857641 |
| 65536 | 0.807908 |

O menor tempo mediano observado foi obtido com cutoff = 65536,
correspondendo a 0.807908 segundos.

A diferença entre o menor e o maior tempo mediano foi de
aproximadamente 6.15%.

Os resultados demonstram que o cutoff influencia o desempenho
da implementação, mas não apresenta uma relação monotônica
entre seu valor e o tempo de execução.

O cutoff = 65536 foi o melhor valor observado neste experimento.
Essa conclusão é restrita à máquina, ao tamanho de entrada e
à quantidade de threads utilizados.

As diferenças de desempenho devem ser interpretadas considerando
a variabilidade das execuções e o overhead de gerenciamento
das tarefas.
