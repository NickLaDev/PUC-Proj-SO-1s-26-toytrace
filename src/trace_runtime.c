#include "trace_runtime.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#if !defined(__x86_64__)
#error "Este runtime didatico suporta apenas Linux x86_64."
#endif

static void fill_event_from_regs(pid_t pid, int entering,
                                 const struct user_regs_struct *regs,
                                 struct syscall_event *ev)
{
    if (ev == NULL) {
        return;
    }

    memset(ev, 0, sizeof(*ev));

    if (regs == NULL) {
        return;
    }

    ev->pid = pid;
    ev->entering = entering;
    ev->syscall_no = regs->orig_rax;
    ev->ret = regs->rax;

    ev->args[0] = regs->rdi;
    ev->args[1] = regs->rsi;
    ev->args[2] = regs->rdx;
    ev->args[3] = regs->r10;
    ev->args[4] = regs->r8;
    ev->args[5] = regs->r9;
}

static pid_t launch_tracee(char *const argv[])
{
    pid_t child = fork();

    if (child < 0) {
        perror("fork");
        return -1;
    }

    if (child == 0) {
        if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) {
            perror("ptrace(PTRACE_TRACEME)");
            _exit(127);
        }

        raise(SIGSTOP);

        execvp(argv[0], argv);
        perror("execvp");
        _exit(127);
    }

    return child;
}

static int wait_for_initial_stop(pid_t child)
{
    int status;
    pid_t waited;

    do {
        waited = waitpid(child, &status, 0);
    } while (waited < 0 && errno == EINTR);

    if (waited < 0) {
        perror("waitpid");
        return -1;
    }

    if (!WIFSTOPPED(status)) {
        fprintf(stderr, "erro: processo monitorado nao parou inicialmente\n");
        return -1;
    }

    if (WSTOPSIG(status) != SIGSTOP) {
        fprintf(stderr, "erro: parada inicial inesperada: sinal %d\n", WSTOPSIG(status));
        return -1;
    }

    return 0;
}

static int configure_trace_options(pid_t child)
{
    if (ptrace(PTRACE_SETOPTIONS, child, NULL,
               (void *)(long)PTRACE_O_TRACESYSGOOD) < 0) {
        perror("ptrace(PTRACE_SETOPTIONS)");
        return -1;
    }

    return 0;
}

static int resume_until_next_syscall(pid_t child, int signal_to_deliver)
{
    if (ptrace(PTRACE_SYSCALL, child, NULL,
               (void *)(long)signal_to_deliver) < 0) {
        perror("ptrace(PTRACE_SYSCALL)");
        return -1;
    }

    return 0;
}

static int wait_for_syscall_stop(pid_t child, int *status)
{
    if (status == NULL) {
        fprintf(stderr, "erro: status invalido\n");
        return -1;
    }

    while (1) {
        pid_t waited;
        int sig;
        int signal_to_deliver;

        do {
            waited = waitpid(child, status, 0);
        } while (waited < 0 && errno == EINTR);

        if (waited < 0) {
            perror("waitpid");
            return -1;
        }

        if (WIFEXITED(*status) || WIFSIGNALED(*status)) {
            return 0;
        }

        if (!WIFSTOPPED(*status)) {
            fprintf(stderr, "erro: estado inesperado do processo monitorado\n");
            return -1;
        }

        sig = WSTOPSIG(*status);

        if (sig == (SIGTRAP | 0x80)) {
            return 1;
        }

        /*
         * SIGTRAP comum pode aparecer, por exemplo, depois do exec.
         * Ele nao deve ser reenviado ao filho. Outros sinais reais
         * podem ser entregues no proximo PTRACE_SYSCALL.
         */
        signal_to_deliver = (sig == SIGTRAP) ? 0 : sig;

        if (resume_until_next_syscall(child, signal_to_deliver) < 0) {
            return -1;
        }
    }
}

int trace_program(char *const argv[],
                  trace_observer_fn observer,
                  void *userdata)
{
    pid_t child;
    int status = 0;
    int entering = 1;

    if (argv == NULL || argv[0] == NULL) {
        fprintf(stderr, "erro: programa alvo ausente\n");
        return -1;
    }

    child = launch_tracee(argv);
    if (child < 0) {
        return -1;
    }

    if (wait_for_initial_stop(child) < 0) {
        return -1;
    }

    if (configure_trace_options(child) < 0) {
        return -1;
    }

    if (resume_until_next_syscall(child, 0) < 0) {
        return -1;
    }

    while (1) {
        struct user_regs_struct regs;
        struct syscall_event ev;
        int stop_kind;

        stop_kind = wait_for_syscall_stop(child, &status);
        if (stop_kind < 0) {
            return -1;
        }

        if (stop_kind == 0) {
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            }
            if (WIFSIGNALED(status)) {
                return 128 + WTERMSIG(status);
            }
            return 0;
        }

        memset(&regs, 0, sizeof(regs));

        if (ptrace(PTRACE_GETREGS, child, NULL, &regs) < 0) {
            perror("ptrace(PTRACE_GETREGS)");
            return -1;
        }

        fill_event_from_regs(child, entering, &regs, &ev);

        if (observer != NULL) {
            observer(&ev, userdata);
        }

        entering = !entering;

        if (resume_until_next_syscall(child, 0) < 0) {
            return -1;
        }
    }
}
