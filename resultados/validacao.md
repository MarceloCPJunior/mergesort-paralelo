# Validação de corretude

Gerado por `scripts/validar.sh rapido` em 2026-11-12 20:51,
código `35916fc`, máquina com 12 núcleos lógicos.

Uma versão passa num caso (n, p) se termina sem erro, imprime `ordenado=SIM` e o
**mesmo checksum do gabarito** para aquele n (`docs/contrato.md`, seção 3).

## Gabarito (sequencial)

| n | checksum de referência | confere com std::sort |
|---|---|---|
| 1 | 67366457 | sim |
| 2 | 2252160707 | sim |
| 3 | 3486334138 | sim |
| 7 | 30816663511 | sim |
| 10 | 67097535705 | sim |
| 1000 | 713889241344622 | sim |

## OpenMP (threads)

| n \ p | 1 | 2 | 3 | 4 | 5 | 7 | 8 |
|---|---|---|---|---|---|---|---|
| 1 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 2 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 3 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 7 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 10 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 1000 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |

## MPI (processos)

| n \ p | 1 | 2 | 3 | 4 | 5 | 7 | 8 |
|---|---|---|---|---|---|---|---|
| 1 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 2 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 3 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 7 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 10 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 1000 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |

## Resultado

**84 de 84 casos passaram.**
