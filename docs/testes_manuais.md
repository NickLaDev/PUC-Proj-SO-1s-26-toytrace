# Testes Manuais – toytrace

## Compilacao

```
make
```

Resultado esperado: binario `toytrace` gerado sem erros ou warnings.

---

## Ajuda da CLI

```
./toytrace --help
```

Resultado: exibe os comandos disponiveis e flags aceitas.

---

## Execucao direta do alvo

```
./tests/targets/hello_write
```

Resultado: imprime `ola toytrace` no terminal, sem o tracer.

---

## Trace basico – hello_write

```
./toytrace trace -- ./tests/targets/hello_write
```

Resultado esperado: linha do programa (`ola toytrace`) seguida das syscalls formatadas, incluindo `write(1, ...) = 13` e `exit_group(0) = 0`.

---

## Eventos crus – hello_write

```
./toytrace trace --raw-events -- ./tests/targets/hello_write
```

Resultado esperado: cada syscall aparece duas vezes, com sufixo `entrada` e `saida`, no formato `pid=<N> <nome> entrada/saida`.

---

## Trace com openat – open_hosts

```
./toytrace trace -- ./tests/targets/open_hosts
```

Resultado esperado: linha `openat(-100, "/etc/hosts", 0x0, 0x0) = 3` e `read(3, ...) = 32`.
Verifica que o dirfd AT_FDCWD aparece como `-100` e que o caminho e lido corretamente da memoria do processo monitorado.

---

## Trace com openat falho – failed_open

```
./toytrace trace -- ./tests/targets/failed_open
```

Resultado esperado: linha `openat(-100, "/arquivo/que/nao/deve/existir/toytrace", 0x0, 0x0) = -2`.
Verifica que erros de syscall (retorno negativo) sao exibidos corretamente.

---

## Trace com execve – exec_echo

```
./toytrace trace -- ./tests/targets/exec_echo
```

Resultado esperado: duas linhas `execve("<ilegivel>", ...) = 0` (a do proprio alvo e a do echo executado internamente). O caminho aparece como `<ilegivel>` porque apos o exec o espaco de memoria antigo nao esta mais disponivel – comportamento esperado conforme o enunciado.

---

## Suite automatizada

```
make test
```

Resultado esperado:
```
testes unitarios concluidos
3 passed in X.XXs
```

Todos os testes unitarios e de integracao passam.
