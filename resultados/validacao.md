# Validação de corretude

Gerado por `scripts/validar.sh completo` em 2026-11-12 04:51,
código `60d4a7f`, máquina com 4 núcleos lógicos.

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
| 1000003 | 14535403353695342139 | sim |
| 20000003 | 11014411462828395110 | sim |

## MPI (processos)

| n \ p | 1 | 2 | 3 | 4 | 5 | 7 | 8 |
|---|---|---|---|---|---|---|---|
| 1 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 2 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 3 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 7 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 10 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 1000 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 1000003 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| 20000003 | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |

## Resultado

**56 de 56 casos passaram.**
