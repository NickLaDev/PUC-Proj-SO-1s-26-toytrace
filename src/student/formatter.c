#include "student_api.h"
#include "syscall_names.h"

#include "trace_helpers.h"

#include <stdio.h>
#include <sys/syscall.h>

static void read_path_or_placeholder(pid_t pid, unsigned long addr, char *buf, size_t bufsz)
{
    if (buf == NULL || bufsz == 0) {
        return;
    }

    if (addr == 0) {
        snprintf(buf, bufsz, "NULL");
        return;
    }

    if (read_child_string(pid, addr, buf, bufsz) < 0) {
        snprintf(buf, bufsz, "<ilegivel>");
    }
}

void student_debug_raw_event(const struct syscall_event *ev, char *buf, size_t bufsz)
{
    if (ev == NULL || buf == NULL || bufsz == 0) {
        return;
    }

    snprintf(buf, bufsz, "pid=%d %s %s",
             ev->pid,
             syscall_name(ev->syscall_no),
             ev->entering ? "entrada" : "saida");
}

void student_format_event(const struct syscall_event *ev, char *buf, size_t bufsz)
{
    if (ev == NULL || buf == NULL || bufsz == 0) {
        return;
    }

    switch (ev->syscall_no) {
#ifdef SYS_read
    case SYS_read:
        snprintf(buf, bufsz, "read(%ld, %#lx, %ld) = %ld",
                 (long)ev->args[0],
                 ev->args[1],
                 (long)ev->args[2],
                 ev->ret);
        return;
#endif

#ifdef SYS_write
    case SYS_write:
        snprintf(buf, bufsz, "write(%ld, %#lx, %ld) = %ld",
                 (long)ev->args[0],
                 ev->args[1],
                 (long)ev->args[2],
                 ev->ret);
        return;
#endif

#ifdef SYS_openat
    case SYS_openat: {
        char path[256];

        read_path_or_placeholder(ev->pid, ev->args[1], path, sizeof(path));
        snprintf(buf, bufsz, "openat(%d, \"%s\", %#lx, %#lx) = %ld",
                 (int)ev->args[0],
                 path,
                 ev->args[2],
                 ev->args[3],
                 ev->ret);
        return;
    }
#endif

#ifdef SYS_execve
    case SYS_execve: {
        char path[256];

        read_path_or_placeholder(ev->pid, ev->args[0], path, sizeof(path));
        snprintf(buf, bufsz, "execve(\"%s\", ...) = %ld",
                 path,
                 ev->ret);
        return;
    }
#endif

#ifdef SYS_exit_group
    case SYS_exit_group:
        snprintf(buf, bufsz, "exit_group(%ld) = %ld",
                 (long)ev->args[0],
                 ev->ret);
        return;
#endif

    default:
        /* Mantem uma saida generica para syscalls fora do requisito principal. */
        snprintf(buf, bufsz, "%s(%#lx, %#lx, %#lx, %#lx, %#lx, %#lx) = %ld",
                 syscall_name(ev->syscall_no),
                 ev->args[0],
                 ev->args[1],
                 ev->args[2],
                 ev->args[3],
                 ev->args[4],
                 ev->args[5],
                 ev->ret);
        return;
    }
}
