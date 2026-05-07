# Mapa inicial do coigo - toytrace 

# teste 

  Colocamos um printf temporário no trace_runtime.c , dentro da função trace_program(), antes da chamada
  de launch_tracee(), assim confirmando se o fluxo do programa realmente chegava ao runtime antes
  de tentar criar o processo alvo. A mensagem usada foi fprintf(stderr, "[DBG] antes de launch\n");
  Assim compilamos com make clean e depois make, assim quando executamos o comando ./toytrace trace
  -- ./tests/targets/hello_write. Quando apareceu a mensagem de debug antes do errom , podemos 
  concluir que passava por main() , interpretava os argumentos na CLI e chegava corretamente até o 
  Runtime. E usamos o git diff para ver as alterações e removemos o teste temporário com git restore. 


# localização dos arquivos do aluno 
  Identificamos na pasta src/student os arquivos principais , sendo eles pairer.c e formatter.c 
  Além desses, identificamos src/trace_runtime.c como parte essencial da implementação, pois nele
  estão os TODOs relacionados ao uso de ptrace, criação do processo alvo, espera por paradas, 
  avanço até syscalls e leitura dos registradores.
  O arquivo include/student_api.h é um arquivo de consulta, pois define a interface das funções 
  que devem ser implementadas.

# Como o runtime anexa o processo monitorado 
  O runtime do projeto não anexa a um processo já existente com ptrace_attach. Em vez disso, ele usa 
  o modelo com ptrace_tracem. Isso quer dizer que o próprio filho informa ao kernel que será rastreado
  pelo processo pai 
  O toytrace cria um filho com fork(). No filho, são chamados ptrace_traceme, raise(sigstop) e depois
  execvp() para executar o programa alvo. O pai espera essa parada com waitpid(), configura o ptrace
  e usa ptrace_syscall para avançar até cada syscall. A cada parada, lÊ os registradores com 
  ptrace_getregs, monta um syscall_event e entrega o evento ao callback

# Onde o programa começa 
   Identificamos a função main() no arquivo src/main.c . A partir dela o programa chama parse_args()
   para interpretar os argumentos da linha de comandos da main , depois chama trace_program() para 
   iniciar o tracing. 

# Onde o processo alvo é criado 
  Nosso processo alvo é criado em src/trace_runtime.c, dentro da função launch_tracee(). 
  Nessa acontece o fork() , e no processo filho o programa usa o ptrace_traceme , para com 
  sigstop e depois executa o alvo com execvp(). 

# Onde o runtime chama o Callback 
  Identificamos que o runtime chama o callback no arquivo src/trace_runtime.c, dentro da função 
  trace_program(). O callback é recebido pelo observer e é chamado quando o runtime monta
  um evento de syscall (struct syscall_event) 
  Assim ele executa --> observer(&ev, userdata) ;
  Assim a chamada entrega manda pelo ptrace para a função trace_observer(), que  
  vimos no src/main.c que manda o evento para funções de pareamento e formatação que estão no student

# Onde o processo filgo é criado 
  Achamos em --> src/trace_runtime.c — funcao launch_tracee() (linha 37).,
  que é um TODO que vamos fazer na Semana 2. Essa função vai chamar fork() para dividir a 
  execução em pai e filho.
  No processo filho, o código rastreia com ptrace(PTRACE_TRACEME, ...), e para temporariamente 
  com raise(SIGSTOP) e, em seguida, executar o programa alvo com execvp(). 
  No processo pai, a função deve retornar o PID do filho, permitindo que o runtime continue o controle
  do tracing.
 

# Onde o callback recebe eventos
  Achamos em --> src/main.c — funcao trace_observer() (linha 12), é chamada pelo loop principal 
  em trace_program() para cada parada de syscall. Se --raw-events estiver ativo, 
  chama student_debug_raw_event().   Caso contrario, passa o evento pelo pairer e, quando 
  completo, ele formata com student_format_event().

# Arquivos que o grupo vai modificar

  - src/trace_runtime.c / launch_tracee, wait_for_initial_stop, 
    configure_trace_options, resume_until_next_syscall, wait_for_syscall_stop, fill_event_from_regs

  - src/student/pairer.c / student_pair_syscall — juntar entrada e saida da mesma syscall 

  - src/student/formatter.c / student_format_event — formatar syscalls em texto legivel 
 
  - docs/testes_manuais.md - Registrar testes executados 

  - docs/relatorio.md - Relatorio final 

# Qual TODO aparece Primeiro ao executar o Scaffold

  O primeiro TODO que aparece é exatamente : 
  	- erro: TODO Semana 2: implementar launch_tracee()
  Encontramos em srctrace_runtime.c, na função: static pid_t launch_tracee( char *const argv[]) . 
  Assim o launch_tracee() ainda está marcado como TODO 

# Principal Dúvida técnica 
 
  Como o toytrace vai diferenciar uma parada causada por syscall de uma parada causada por outro sinal do processo?
  ASsim ,como reconhecemos corretamente quando a parada representa uma entrada ou saida da syscall

# Comandos que usamos 

 Durante a exploração inicial executamos os comando: 
	- make 
	- ./toytrace --help
	- ./tests/targets/hello_write
	- ./toytrace trace -- ./tests/targets/hello_write

 Compilamos o projeto com o make. O comando ./toytrace --help mostrou a ajuda da CLI. 
 Verificamos que o programa alvo hello_write executou diretamente. 
 Ao executar o scaffold com ./toytrace trace -- ./tests/targets/hello_write, o programa 
 ainda para no primeiro TODO esperado, assim no proximo passo teremos que a implementacao 
 da função launch_tracee().

# Organização do grupo para planejamento das implementações 

  1.	Implementar launch_tracee() para criar o processo filho.
  2.	Implementar a espera pela parada inicial com waitpid().
  3.	Configurar PTRACE_O_TRACESYSGOOD.
  4.	Avancar o processo com PTRACE_SYSCALL.
  5.	Detectar paradas de syscall no loop com waitpid().
  6.	Ler registradores com PTRACE_GETREGS.
  7.	Preencher struct syscall_event.
  8.	Parear entrada e saida em student_pair_syscall().
  9.	Formatar as syscalls em student_format_event().