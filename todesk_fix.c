#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <dlfcn.h>

static const char *TODESK_EXEC_PATH = NULL;

#define EXEC_PREFIX "/proc/"
#define EXEC_SUFFIX "/exe"

static void _init() {
    static bool inited = false;
    if (inited) {
        return;
    }
    TODESK_EXEC_PATH = getenv("TODESK_EXEC_PATH");
    inited = true;
}

static bool _str_begin_with(const char *str, const char *prefix) {
    if (strlen(str) < strlen(prefix)) {
        return false;
    }

    for (int i = 0; i < strlen(prefix); i ++) {
        if (str[i] != prefix[i]) {
            return false;
        }
    }
    return true;
}

static bool _str_end_with(const char *str, const char *suffix) {
    if (strlen(str) < strlen(suffix)) {
        return false;
    }

    int str_offset = strlen(str) - strlen(suffix);   
    for (int i = 0; i < strlen(suffix); i ++) {
        if (str[str_offset + i] != suffix[i]) {
            return false;
        }
    }

    return true;
}

ssize_t readlink(const char *restrict pathname, char *restrict buf,
                 size_t bufsize) {
    _init();

    if (TODESK_EXEC_PATH != NULL
        && _str_begin_with(pathname, EXEC_PREFIX)
        && _str_end_with(pathname, EXEC_SUFFIX)) {
        // Just return TODESK_EXEC_PATH.
        ssize_t size = strlen(TODESK_EXEC_PATH) <= bufsize ? strlen(TODESK_EXEC_PATH) : bufsize;
        memcpy(buf, TODESK_EXEC_PATH, size);
        return size;
    }

    // Call the original readlink.
    ssize_t (*ori_readlink)(const char *, char *, size_t) = NULL;
    ori_readlink = dlsym(RTLD_NEXT, "readlink");

    if (ori_readlink == NULL) {
        fprintf(stderr, "** error: failed to load readlink %s", dlerror);
        errno = EIO;
        return -1;
    }
    return ori_readlink(pathname, buf, bufsize);
}
