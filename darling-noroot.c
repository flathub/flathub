/* Runs Darling without root, for sandboxes such as Flatpak.
 *
 * Darling's `darling` client is setuid root and starts `darlingserver` as
 * root. The server uses root only to create a mount namespace, mount a
 * fresh /dev/shm, merge the macOS root into the prefix with overlayfs,
 * start launchd in its own PID namespace and mount /proc for it. Inside a
 * sandbox none of that is allowed, and none of it is needed:
 *
 *  - DARLING_NOOVERLAYFS=1 makes darlingserver copy the macOS root into the
 *    prefix instead of mounting overlayfs (its WSL1 mode);
 *  - unshare() and the mounts report success and keep the sandbox's own
 *    /dev/shm; the prefix's /proc becomes a link to the host /proc;
 *  - launchd starts as an ordinary child (clone without CLONE_NEWPID) and
 *    sees itself as PID 1 through launchd_pid1.dylib (QUADRA_PID1_DYLIB),
 *    which the server copies into the prefix; Mach-O programs load from
 *    the prefix (a full copy of the macOS root here) instead of the
 *    read-only system copy, so such additions are visible;
 *  - the root checks and privilege switches are answered as if they
 *    succeeded, the process keeps running as the user.
 *
 * Loaded with LD_PRELOAD into `darling` and `darlingserver` only; in every
 * other process (Darling's own, which inherit the environment) the
 * wrappers pass straight through. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <pwd.h>
#include <sched.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

enum { OTHER, CLIENT, SERVER };
static int role = -1;
/* darlingserver switches between root and the user while starting; this
 * mirrors which one it believes it is. */
static int server_is_root = 1;

static int current_role(void) {
    if (role < 0) {
        char exe[4096];
        ssize_t length = readlink("/proc/self/exe", exe, sizeof exe - 1);
        exe[length > 0 ? length : 0] = 0;
        const char *name = strrchr(exe, '/');
        name = name ? name + 1 : exe;
        role = strcmp(name, "darlingserver") == 0 ? SERVER
             : strcmp(name, "darling") == 0 || strncmp(name, "darling-", 8) == 0 ? CLIENT
             : OTHER;
    }
    return role;
}

/* Real functions, looked up once when the library loads: dlsym after a
 * raw fork (Darling forks without glibc) crashed the child. */
#define REAL(name) (real_##name)
#define DECLARE_REAL(name) static __typeof__(&name) real_##name
DECLARE_REAL(getuid);
DECLARE_REAL(getgid);
DECLARE_REAL(geteuid);
DECLARE_REAL(getegid);
DECLARE_REAL(setuid);
DECLARE_REAL(setgid);
DECLARE_REAL(seteuid);
DECLARE_REAL(setegid);
DECLARE_REAL(setresuid);
DECLARE_REAL(setresgid);
DECLARE_REAL(getpwuid);
DECLARE_REAL(fchownat);
DECLARE_REAL(chown);
DECLARE_REAL(lchown);
DECLARE_REAL(setns);
DECLARE_REAL(unshare);
DECLARE_REAL(umount);
DECLARE_REAL(mount);
DECLARE_REAL(syscall);
DECLARE_REAL(setenv);
DECLARE_REAL(exit);
#ifdef QUADRA_NIX_STORE
DECLARE_REAL(mkdir);
DECLARE_REAL(chmod);
DECLARE_REAL(fchmod);
DECLARE_REAL(fchmodat);

/* Nix normalizes store directories to 0555 and files to 0444/0555.
 * Darling's no-overlay copy preserves these modes. Its private copy needs
 * owner write permission so it can populate directories and update the guest.
 * Keep this limited to the runtime processes and the Nix build of the helper. */
int mkdir(const char *path, mode_t mode) {
    if (current_role() != OTHER)
        mode |= S_IWUSR;
    return REAL(mkdir)(path, mode);
}

int chmod(const char *path, mode_t mode) {
    if (current_role() != OTHER)
        mode |= S_IWUSR;
    return REAL(chmod)(path, mode);
}

int fchmod(int fd, mode_t mode) {
    if (current_role() != OTHER)
        mode |= S_IWUSR;
    return REAL(fchmod)(fd, mode);
}

int fchmodat(int fd, const char *path, mode_t mode, int flags) {
    if (current_role() != OTHER)
        mode |= S_IWUSR;
    return REAL(fchmodat)(fd, path, mode, flags);
}
#endif

uid_t getuid(void) {
    if (current_role() == SERVER && server_is_root)
        return 0;
    return REAL(getuid)();
}

gid_t getgid(void) {
    if (current_role() == SERVER && server_is_root)
        return 0;
    return REAL(getgid)();
}

uid_t geteuid(void) {
    if (current_role() == CLIENT || (current_role() == SERVER && server_is_root))
        return 0;
    return REAL(geteuid)();
}

gid_t getegid(void) {
    if (current_role() == CLIENT || (current_role() == SERVER && server_is_root))
        return 0;
    return REAL(getegid)();
}

static int fake_identity(uid_t target) {
    if (current_role() == SERVER)
        server_is_root = target == 0;
    return 0;
}

int setuid(uid_t uid) { return current_role() == OTHER ? REAL(setuid)(uid) : fake_identity(uid); }
int setgid(gid_t gid) { return current_role() == OTHER ? REAL(setgid)(gid) : 0; }
int seteuid(uid_t uid) { return current_role() == OTHER ? REAL(seteuid)(uid) : fake_identity(uid); }
int setegid(gid_t gid) { return current_role() == OTHER ? REAL(setegid)(gid) : 0; }
int setresuid(uid_t r, uid_t e, uid_t s) {
    return current_role() == OTHER ? REAL(setresuid)(r, e, s) : fake_identity(e);
}
int setresgid(gid_t r, gid_t e, gid_t s) {
    return current_role() == OTHER ? REAL(setresgid)(r, e, s) : 0;
}

/* `darling` names the home inside the prefix after the owner of its
 * effective uid, which is root here. A Flatpak's /etc/passwd has no root
 * entry, and the getlogin() fallback needs a login uid that some display
 * managers never set: "Cannot determine your user name". Root is the user. */
struct passwd *getpwuid(uid_t uid) {
    if (current_role() != OTHER && uid == 0)
        uid = REAL(getuid)();
    return REAL(getpwuid)(uid);
}

/* The prefix is ours already; the copied macOS root would be chowned to
 * root, which only root may do. */
int fchownat(int dir, const char *path, uid_t uid, gid_t gid, int flags) {
    int result = REAL(fchownat)(dir, path, uid, gid, flags);
    return result < 0 && (errno == EPERM || errno == EINVAL) && current_role() != OTHER ? 0 : result;
}
int chown(const char *path, uid_t uid, gid_t gid) {
    int result = REAL(chown)(path, uid, gid);
    return result < 0 && (errno == EPERM || errno == EINVAL) && current_role() != OTHER ? 0 : result;
}
int lchown(const char *path, uid_t uid, gid_t gid) {
    int result = REAL(lchown)(path, uid, gid);
    return result < 0 && (errno == EPERM || errno == EINVAL) && current_role() != OTHER ? 0 : result;
}

int setns(int fd, int type) {
    return current_role() == OTHER ? REAL(setns)(fd, type) : 0;
}

int unshare(int flags) {
    return current_role() == OTHER ? REAL(unshare)(flags) : 0;
}

int umount(const char *target) {
    return current_role() == OTHER ? REAL(umount)(target) : 0;
}

int mount(const char *source, const char *target, const char *type, unsigned long flags,
          const void *data) {
    if (current_role() == OTHER)
        return REAL(mount)(source, target, type, flags, data);
    const char *keep_proc = getenv("QUADRA_KEEP_PREFIX_PROC");
    if (type && strcmp(type, "proc") == 0 && target && !(keep_proc && keep_proc[0] == '1')) {
        /* launchd's /proc: the host one, seen through Darling's path
         * translation. The child execs launchd next; it must not inherit
         * this library. */
        unlink(target);
        rmdir(target);
        symlink("/Volumes/SystemRoot/proc", target);
    }
    if (type && strcmp(type, "proc") == 0)
        unsetenv("LD_PRELOAD");
    return 0;
}

static void install_pid1_library(void);

/* darlingserver starts launchd with syscall(SYS_clone, CLONE_NEWPID | SIGCHLD). */
long syscall(long number, ...) {
    va_list args;
    va_start(args, number);
    long a[6];
    for (int i = 0; i < 6; i++)
        a[i] = va_arg(args, long);
    va_end(args);
    if (number == SYS_clone && current_role() == SERVER) {
        a[0] &= ~(long)(CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWIPC);
        /* A new prefix only exists by now. */
        install_pid1_library();
    }
    return REAL(syscall)(number, a[0], a[1], a[2], a[3], a[4], a[5]);
}

__attribute__((constructor(101))) static void resolve_real_functions(void) {
#define RESOLVE(name) real_##name = (__typeof__(&name))dlsym(RTLD_NEXT, #name)
    RESOLVE(getuid);
    RESOLVE(getgid);
    RESOLVE(geteuid);
    RESOLVE(getegid);
    RESOLVE(setuid);
    RESOLVE(setgid);
    RESOLVE(seteuid);
    RESOLVE(setegid);
    RESOLVE(setresuid);
    RESOLVE(setresgid);
    RESOLVE(getpwuid);
    RESOLVE(fchownat);
    RESOLVE(chown);
    RESOLVE(lchown);
    RESOLVE(setns);
    RESOLVE(unshare);
    RESOLVE(umount);
    RESOLVE(mount);
    RESOLVE(syscall);
    RESOLVE(setenv);
    RESOLVE(exit);
#ifdef QUADRA_NIX_STORE
    RESOLVE(mkdir);
    RESOLVE(chmod);
    RESOLVE(fchmod);
    RESOLVE(fchmodat);
#endif
}

/* The prefix, from darlingserver's command line (argv[1]). */
static const char *server_prefix(void) {
    static char prefix[4096];
    if (!prefix[0]) {
        char buffer[8192];
        FILE *file = fopen("/proc/self/cmdline", "r");
        size_t length = file ? fread(buffer, 1, sizeof buffer - 1, file) : 0;
        if (file)
            fclose(file);
        buffer[length] = 0;
        size_t first = strlen(buffer) + 1;
        if (first < length)
            snprintf(prefix, sizeof prefix, "%s", buffer + first);
    }
    return prefix;
}

/* darlingserver points Mach-O loading at the read-only system root; with a
 * copied prefix, load from the prefix. */
int setenv(const char *name, const char *value, int overwrite) {
    const char *keep_root = getenv("QUADRA_KEEP_DYLD_ROOT");
    if (current_role() == SERVER && strcmp(name, "__mldr_DYLD_ROOT_PATH") == 0 && server_prefix()[0] &&
        !(keep_root && keep_root[0] == '1'))
        value = server_prefix();
    return real_setenv(name, value, overwrite);
}

/* Copy `source` to `target` through a temporary file renamed into place:
 * a launchd left over from an earlier server may still have the old copy
 * mapped (truncating it in place would crash it), and a short copy must not
 * replace a good one. */
static int copy_file(const char *source, const char *target) {
    char temporary[4200];
    snprintf(temporary, sizeof temporary, "%s.%ld.tmp", target, (long)getpid());
    FILE *in = fopen(source, "rb");
    if (!in)
        return 0;
    FILE *out = fopen(temporary, "wb");
    if (!out) {
        fclose(in);
        return 0;
    }
    char block[65536];
    size_t got;
    int ok = 1;
    while (ok && (got = fread(block, 1, sizeof block, in)) > 0)
        ok = fwrite(block, 1, got, out) == got;
    ok = ok && !ferror(in);
    fclose(in);
    ok = fclose(out) == 0 && ok;
    ok = ok && rename(temporary, target) == 0;
    if (!ok)
        unlink(temporary);
    return ok;
}

static void install_pid1_library(void) {
    static int installed;
    const char *source = getenv("QUADRA_PID1_DYLIB");
    if (installed || !source || !server_prefix()[0])
        return;
    char target[4096];
    if (snprintf(target, sizeof target, "%s/usr/lib/quadra_launchd_pid1.dylib", server_prefix()) >=
        (int)sizeof target)
        return;
    if (!copy_file(source, target)) {
        /* No usr/lib yet: a new prefix, copied again before launchd starts. */
        if (errno != ENOENT)
            fprintf(stderr, "[noroot] could not copy %s to %s: %s\n", source, target, strerror(errno));
        return;
    }
    installed = 1;
    /* Inherited by launchd, the only process this server starts. launchd
     * removes it from its own environment (launchd_pid1.c), so the
     * daemons and programs it starts do not load the library. */
    setenv("DYLD_INSERT_LIBRARIES", "/usr/lib/quadra_launchd_pid1.dylib", 1);
}

__attribute__((constructor)) static void setup(void) {
    if (current_role() == SERVER) {
        setenv("DARLING_NOOVERLAYFS", "1", 1);
        install_pid1_library();
    }
    else if (current_role() == CLIENT) {
        /* `darling shell` hands its environment to shellspawn; keep this
         * library out of the programs it starts, only darlingserver (execl
         * below) gets it back. */
        const char *preload = getenv("LD_PRELOAD");
        if (preload) {
            real_setenv("QUADRA_NOROOT_PRELOAD", preload, 1);
            unsetenv("LD_PRELOAD");
        }
    }
}

/* The client starts darlingserver with execl(); that one needs the library. */
int execl(const char *path, const char *arg, ...) {
    char *argv[64];
    va_list args;
    int count = 0;
    argv[count++] = (char *)arg;
    va_start(args, arg);
    while (count < 63 && (argv[count] = va_arg(args, char *)) != NULL)
        count++;
    va_end(args);
    argv[count] = NULL;
    const char *preload = getenv("QUADRA_NOROOT_PRELOAD");
    const char *name = strrchr(path, '/');
    if (preload && name && strcmp(name + 1, "darlingserver") == 0)
        real_setenv("LD_PRELOAD", preload, 1);
    return execv(path, argv);
}

/* QUADRA_NOROOT_DEBUG=1: say where the client exits from. */
void exit(int status) {
    if (current_role() == CLIENT && getenv("QUADRA_NOROOT_DEBUG")) {
        void *caller = __builtin_return_address(0);
        Dl_info info;
        if (dladdr(caller, &info) && info.dli_fname)
            fprintf(stderr, "[noroot] exit(%d) from %s+0x%lx\n", status, info.dli_fname,
                    (unsigned long)((char *)caller - (char *)info.dli_fbase));
    }
    /* Resolved when the library loaded: dlsym here would crash a child of
     * Darling's raw fork (see above). */
    if (real_exit)
        real_exit(status);
    _exit(status);
}
