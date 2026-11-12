# Resumo da escalabilidade forte (n = 20000000)

Gerado por scripts/graficos.py a partir de resultados/*.csv (medianas).

| versão | p | tempo (s) | speedup | eficiência | previsto (Amdahl) |
|---|---|---|---|---|---|
| baseline | 1 | 1,935 | 1,00 | 1,00 | 1,00 |
| OpenMP | 1 | 1,909 | 1,01 | 1,01 | 1,00 |
| OpenMP | 2 | 1,003 | 1,93 | 0,96 | 1,92 |
| OpenMP | 4 | 0,756 | 2,56 | 0,64 | 3,43 |
| OpenMP | 8 | 0,598 | 3,24 | 0,40 | 5,50 |
| MPI | 1 | 1,943 | 1,00 | 1,00 | 1,00 |
| MPI | 2 | 1,042 | 1,86 | 0,93 | 1,92 |
| MPI | 4 | 0,623 | 3,11 | 0,78 | 3,43 |
| MPI | 8 | 0,475 | 4,07 | 0,51 | 5,50 |
