# Relatório Final – toytrace

**Disciplina:** Projetos de Sistemas Operacionais – 1s/2026  
**Professor:** Lucas Reis  
**Repositório:** https://github.com/NickLaDev/PUC-Proj-SO-1s-26-toytrace

**Membros do grupo:**
- Bernardo Duque Souza Atadia – 24003650
- Nicolas Laredo Alves de Araújo – 24001613
- Pedro Castro Souza – 23003429

---

## Resumo do trabalho realizado

O projeto consistiu na implementação de uma ferramenta de monitoramento de chamadas de sistema (syscalls) em Linux chamada `toytrace`, inspirada no `strace`. A ferramenta utiliza a API `ptrace` para observar um processo alvo, capturando cada syscall em sua entrada e saída, e imprimindo as chamadas em formato legível.

A base de código fornecida pelo professor incluía a CLI, a interface de eventos de syscall e funções auxiliares. O grupo implementou o núcleo funcional da ferramenta nos arquivos `src/trace_runtime.c` e `src/student/`.

---

## Histórico de commits

**349be4c – First commit**  
Commit inicial do repositório base fornecido pelo professor. Continha o esqueleto do projeto: Makefile, estruturas de dados, CLI e funções auxiliares já implementadas.

**96c3a97 – Semana 1: subindo mapa do codigo**  
Exploração da base de código e construção de um mapa mental do projeto. Identificamos onde o programa começa (main.c), onde o processo alvo é criado (launch_tracee em trace_runtime.c), onde o runtime chama o callback e quais arquivos o grupo deveria modificar. O arquivo docs/mapa_do_codigo.md foi criado com essas anotações.

**23e67fe – Semana 2: processo alvo criado e parado antes do tracing**  
Implementação da função launch_tracee() em src/trace_runtime.c. O processo filho é criado com fork(). No filho, chamamos ptrace(PTRACE_TRACEME, ...) para informar ao kernel que será rastreado, em seguida raise(SIGSTOP) para pausar e aguardar o pai configurar o tracing, e por fim execvp() para executar o programa alvo. No pai, retornamos o PID do filho.

**c5b00e3 – Semana 3: runtime – esperar parada inicial do filho**  
Implementação das funções de controle do loop de tracing: wait_for_initial_stop() espera a parada inicial do filho via waitpid, verifica WIFSTOPPED e WSTOPSIG == SIGSTOP. configure_trace_options() configura PTRACE_O_TRACESYSGOOD para distinguir paradas de syscall de outros sinais. resume_until_next_syscall() avança o filho até a próxima syscall com PTRACE_SYSCALL. wait_for_syscall_stop() implementa o loop principal: a cada iteração verifica se o filho terminou (WIFEXITED/WIFSIGNALED), foi morto por sinal, ou parou em syscall (detectado com WSTOPSIG & 0x80).

**d25bf78 – Semana 4 pt1: preencher eventos a partir dos regs**  
Implementação da leitura dos registradores x86_64 com ptrace(PTRACE_GETREGS, ...) e preenchimento da estrutura struct syscall_event. Os campos mapeados foram: orig_rax → syscall_no, rax → ret, e rdi, rsi, rdx, r10, r8, r9 → args[0..5]. O campo entering controla se o evento é de entrada ou saída, alternando a cada parada.

**3963c71 – Semana 4 pt2: parear entrada e saída de syscalls**  
Implementação de student_pair_syscall() em src/student/pairer.c. Na entrada da syscall, o evento é salvo em pairer->entry. Na saída, o evento completo é montado combinando os argumentos da entrada com o valor de retorno da saída (ev->ret). Retorna 0 na entrada (aguardando) e 1 quando o par está completo.

**c24522d – Semana 5 pt1: formatação genérica de syscalls**  
Implementação do formato genérico em student_format_event() para syscalls não tratadas especificamente: nome(arg0, arg1, arg2, arg3, arg4, arg5) = ret. Utiliza syscall_name() para converter o número da syscall em nome legível.

**7f7459a – Semana 5 pt2: formatar syscalls obrigatórias**  
Implementação dos casos especiais em student_format_event(): read(fd, buf, count) = ret, write(fd, buf, count) = ret, openat(dirfd, "path", flags, mode) = ret com read_child_string() para ler o caminho da memória do processo monitorado, execve("path", ...) = ret com read_child_string() para ler o caminho (exibe `<ilegivel>` se a leitura falhar, comportamento esperado após o exec substituir o espaço de memória), exit_group(status) = ret.

**c508d5a – Semana 6 pt1: revisar erros e limpar mensagens temporárias**  
Revisão geral do código: remoção de mensagens de debug temporárias, verificação de casos de erro, limpeza do código. Execução da suite de testes completa confirmando todos os testes passando.

**2fe3114 – Semana 6 pt2: corrigir cast do dirfd e adicionar gitignore**  
Correção na formatação do primeiro argumento de openat: o valor AT_FDCWD (-100) era exibido como número positivo grande por ser tratado como inteiro sem sinal. O cast correto para inteiro com sinal passou a exibir -100 como esperado. Adicionado .gitignore para evitar que binários e arquivos objeto sejam versionados acidentalmente. Adicionados docs/testes_manuais.md e docs/relatorio.md.

---

## 1. Divisão do trabalho

A divisão abaixo reflete a autoria dos commits no repositório:

- **Nicolas Laredo Alves de Araújo:** exploração inicial da base de código e criação do docs/mapa_do_codigo.md (Semana 1); leitura dos registradores x86_64 com PTRACE_GETREGS e preenchimento da struct syscall_event (Semana 4 pt1); pareamento de entrada e saída de syscalls em student_pair_syscall (Semana 4 pt2); formatação genérica e formatação das syscalls obrigatórias em student_format_event, incluindo read_child_string para openat/execve (Semana 5 pt1 e pt2).

- **Bernardo Duque Souza Atadia:** implementação da criação do processo alvo em launch_tracee — fork(), PTRACE_TRACEME, raise(SIGSTOP) e execvp() — garantindo que o filho seja parado antes do início do tracing (Semana 2).

- **Pedro Castro Souza:** implementação das funções de controle do loop de tracing — wait_for_initial_stop, configure_trace_options, resume_until_next_syscall e wait_for_syscall_stop (Semana 3); revisão geral do código, remoção de mensagens de debug temporárias, tratamento de casos de erro e execução da suíte de testes (Semana 6 pt1).

---

## 2. Desafios de implementação

**Coordenar a sincronização inicial entre pai e filho.** No começo o tracing não funcionava de forma confiável porque o pai tentava configurar as opções de ptrace antes do filho efetivamente parar. Foi preciso entender que o filho deve chamar PTRACE_TRACEME e em seguida raise(SIGSTOP) para garantir que o pai só configure o tracing depois de o filho estar pausado, detectado com waitpid + WIFSTOPPED/WSTOPSIG == SIGSTOP. Sem essa ordem, ocorriam condições de corrida difíceis de reproduzir.

**Distinguir paradas de syscall de outros sinais.** Inicialmente o loop tratava qualquer parada como se fosse uma syscall, gerando saída incorreta. A solução foi ativar PTRACE_O_TRACESYSGOOD e testar o bit 0x80 em WSTOPSIG (WSTOPSIG & 0x80), que marca especificamente as paradas de syscall e as separa de sinais comuns entregues ao processo.

**Entender que uma syscall gera duas paradas.** Levou tempo perceber que cada syscall provoca uma parada na entrada e outra na saída, e que os argumentos só estão disponíveis na entrada enquanto o valor de retorno só aparece na saída. Isso obrigou a guardar o evento de entrada (pairer->entry) e combiná-lo com o de saída para montar a chamada completa. Alternar corretamente o campo entering a cada parada foi essencial.

**Ler strings da memória do processo monitorado.** Para exibir caminhos em openat e execve foi necessário ler a memória do processo alvo palavra por palavra, já que os registradores só contêm o ponteiro, não o conteúdo da string. Tratar o terminador nulo e os limites da leitura exigiu cuidado.

**O caminho de execve ilegível na saída.** Um comportamento confuso a princípio: na saída do execve a leitura do caminho falhava. Entendemos que isso ocorre porque o exec substitui completamente o espaço de endereçamento do processo, invalidando o ponteiro original. O grupo passou a exibir `<ilegivel>` nesse caso, tratando-o como comportamento esperado e não como bug.

---

## 3. Maiores aprendizados

O projeto consolidou de forma prática vários conceitos centrais de Sistemas Operacionais:

**Como processos se comunicam com o kernel via syscalls.** Antes do projeto, syscalls eram um conceito abstrato; ver cada chamada sendo interceptada em tempo real deixou claro que toda interação relevante de um programa com o mundo externo (arquivos, processos, E/S) passa obrigatoriamente pelo kernel através dessa interface.

**O funcionamento interno de debuggers e do strace.** Implementar a ferramenta mostrou que ptrace é o mecanismo que sustenta ferramentas como gdb e strace. Entender o ciclo de parar o processo, inspecionar registradores e retomar a execução desmistificou como um depurador consegue observar e controlar outro processo.

**A fronteira entre espaço de usuário e espaço de kernel.** Ficou evidente por que não basta ler um ponteiro de string diretamente: a memória do processo alvo está em outro espaço de endereçamento, e o acesso precisa ser mediado pelo kernel via ptrace. A transição usuário/kernel deixou de ser teoria e passou a ser algo observável.

**A ABI de syscalls do Linux em x86_64.** Mapear orig_rax para o número da syscall e rdi, rsi, rdx, r10, r8, r9 para os argumentos ensinou na prática a convenção de chamada de syscalls da arquitetura, além de reforçar o papel dos registradores e dos descritores de arquivo no funcionamento do sistema.

---

## 4. Incrementos sugeridos para o toytrace

**Suporte a múltiplas threads e processos filhos.** Atualmente a ferramenta rastreia apenas o processo alvo direto. Usando opções como PTRACE_O_TRACEFORK, PTRACE_O_TRACEVFORK e PTRACE_O_TRACECLONE, seria possível seguir automaticamente filhos e threads criados durante a execução, aproximando o comportamento do strace -f.

**Exibir o conteúdo real dos buffers de read/write.** Hoje apenas o ponteiro e o tamanho são mostrados. Lendo a memória do processo alvo (como já é feito para os caminhos de openat/execve), seria possível exibir um trecho do conteúdo transferido, com truncamento e escape de caracteres não imprimíveis.

**Filtragem de syscalls por nome.** Uma opção de linha de comando como --filter write,openat permitiria restringir a saída apenas às syscalls de interesse, tornando a ferramenta mais útil em programas com muita atividade de E/S.

**Estatísticas ao final da execução.** Acumular contagem e, idealmente, tempo gasto por tipo de syscall permitiria emitir um resumo final (semelhante ao strace -c), útil para análise de desempenho.

**Decodificação simbólica de flags.** Em vez de imprimir valores numéricos crus, traduzir flags de open/openat e mmap para nomes simbólicos (ex.: O_RDONLY, O_CREAT, PROT_READ) tornaria a saída muito mais legível.

**Attach em processo já em execução.** Suportar PTRACE_ATTACH (ou PTRACE_SEIZE) permitiria monitorar um processo já existente a partir do seu PID, sem precisar iniciá-lo pela própria ferramenta.

---

*Projeto Final de Sistemas Operacionais – PUC-Campinas, 1s/2026.*
