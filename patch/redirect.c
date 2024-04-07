#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>


int *(*real_access)(const char *, int);
void *(*real_dlopen)(char const *, int);
FILE *(*real_fopen)(char const *, char const *);
FILE *(*real_fopen64)(char const *, char const *);

void hook_path(const char *func, const char **path) {
    char *redirect = NULL;

    if (strcmp(*path, "/usr/lib/libactivation.so") == 0) {
        redirect = "/app/extra/wechat/libactivation.so";
    } else if (strcmp(*path, "/etc/lsb-release-ukui") == 0) {
        redirect = "/app/extra/etc/lsb-release-ukui";
    } else if (strcmp(*path, "/etc/lsb-release") == 0) {
        redirect = "/app/extra/etc/lsb-release-ukui";
    } else if (strcmp(*path, "/etc/LICENSE") == 0) {
        redirect = "/app/extra/etc/LICENSE";
    } else if (strcmp(*path, "/etc/.kyact") == 0) {
        redirect = "/app/extra/etc/.kyact";
    }

    if (redirect != NULL) {
        printf("%s: redirect from %s to: %s\n", func, *path, redirect);
        *path = redirect;
    }
}

int *access(const char *__file, int __mode) {
    if (real_access == NULL) {
        real_access = dlsym(RTLD_NEXT, "access");
    }

    if (__file != NULL) {
        hook_path("access", &__file);
    }

    return (*real_access)(__file, __mode);
}

void *dlopen(const char *__file, int __mode) {
    if (real_dlopen == NULL) {
        real_dlopen = dlsym(RTLD_NEXT, "dlopen");
    }

    if (__file != NULL) {
        hook_path("dlopen", &__file);
    }

    return (*real_dlopen)(__file, __mode);
}

FILE *fopen(const char *__file, const char *__mode) {
    if (real_fopen == NULL) {
        real_fopen = dlsym(RTLD_NEXT, "fopen");
    }

    if (__file != NULL) {
        hook_path("fopen", &__file);
    }

    return (*real_fopen)(__file, __mode);
}

FILE *fopen64(const char *__file, const char *__mode) {
    if (real_fopen64 == NULL) {
        real_fopen64 = dlsym(RTLD_NEXT, "fopen64");
    }

    if (__file != NULL) {
        hook_path("fopen64", &__file);
    }

    return (*real_fopen64)(__file, __mode);
}
