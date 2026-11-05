/* Makes launchd PID 1 inside Darling without a PID namespace.
 *
 * launchd only runs as the system launchd, which starts Darling's daemons
 * (shellspawn among them), when getpid() == 1. Without root Darling cannot
 * give it its own PID namespace (see darling-noroot.c), so launchd keeps
 * its real PID; this library, inserted into launchd only, answers getpid()
 * with 1 there, and launchd's own kill(1, ...) goes to itself, not to the
 * sandbox's PID 1. launchd removes the library from its environment, so the
 * daemons and programs it starts run without it (their getpid() and kill()
 * are Darling's own).
 *
 * launchd also becomes the child subreaper, as PID 1 of Darling's own
 * namespace would be: a daemon that double-forks (Roblox's crash handler)
 * then stays a descendant of darlingserver. Without root, darlingserver may
 * only read and write the memory of its descendants (Yama ptrace_scope 1);
 * reparented to the sandbox's init, the crash handler failed every call. */
typedef int pid_t;

extern pid_t getpid(void);
extern int kill(pid_t, int);
extern const char *getprogname(void);
extern int strcmp(const char *, const char *);
extern int unsetenv(const char *);
/* Darling processes are Linux processes: the syscall instruction reaches
 * the Linux kernel directly (Darling's own linux_syscall is not exported). */
static long linux_prctl(long option, long value) {
    long result;
    __asm__ volatile("syscall" : "=a"(result) : "a"(157L /* prctl */), "D"(option), "S"(value)
                     : "rcx", "r11", "memory");
    return result;
}
#define PR_SET_CHILD_SUBREAPER 36

#define DYLD_INTERPOSE(_replacement, _replacee) \
    __attribute__((used)) static struct { const void *replacement; const void *replacee; } \
    _interpose_##_replacee __attribute__((section("__DATA,__interpose"))) = \
        {(const void *)(unsigned long)&_replacement, (const void *)(unsigned long)&_replacee};

static int in_launchd;
static pid_t launchd_pid;

__attribute__((constructor)) static void setup(void) {
    const char *name = getprogname();
    in_launchd = name && strcmp(name, "launchd") == 0;
    if (in_launchd) {
        launchd_pid = getpid();
        /* Only launchd needs this library; keep it out of its children. */
        unsetenv("DYLD_INSERT_LIBRARIES");
        linux_prctl(PR_SET_CHILD_SUBREAPER, 1);
    }
}

static pid_t quadra_getpid(void) {
    return in_launchd ? 1 : getpid();
}
DYLD_INTERPOSE(quadra_getpid, getpid)

static int quadra_kill(pid_t pid, int signal) {
    if (pid == 1 && in_launchd)
        pid = launchd_pid;
    return kill(pid, signal);
}
DYLD_INTERPOSE(quadra_kill, kill)
