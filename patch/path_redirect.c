#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/stat.h>

/* * A simple hook library to solve hardcoded path issues.
 * It redirects all operations pointing to /opt/ZTE to /var/cache/ZTE
 */

// Helper function: rewrite path
// If the path starts with /opt/ZTE, replace it with /var/cache/ZTE, otherwise return the original path
#define TARGET_PREFIX "/opt/ZTE"
#define REDIRECT_PREFIX "/var/cache/ZTE"
static char *get_redirected_path(const char *path, char *buffer, size_t size)
{
  if (path && strncmp(path, TARGET_PREFIX, strlen(TARGET_PREFIX)) == 0)
  {
    snprintf(buffer, size, "%s%s", REDIRECT_PREFIX, path + strlen(TARGET_PREFIX));
    fprintf(stderr, "[PathRedirect] Redirecting: %s -> %s\n", path, buffer);
    return buffer;
  }
  return (char *)path;
}

typedef FILE *(*real_fopen_t)(const char *path, const char *mode);
FILE *fopen(const char *path, const char *mode)
{
  char new_path[4096];
  real_fopen_t real_func = (real_fopen_t)dlsym(RTLD_NEXT, "fopen");
  return real_func(get_redirected_path(path, new_path, sizeof(new_path)), mode);
}

typedef FILE *(*real_fopen64_t)(const char *path, const char *mode);
FILE *fopen64(const char *path, const char *mode)
{
  char new_path[4096];
  real_fopen64_t real_func = (real_fopen64_t)dlsym(RTLD_NEXT, "fopen64");
  return real_func(get_redirected_path(path, new_path, sizeof(new_path)), mode);
}

typedef int (*real_open_t)(const char *path, int flags, ...);
int open(const char *path, int flags, ...)
{
  char new_path[4096];
  real_open_t real_func = (real_open_t)dlsym(RTLD_NEXT, "open");

  va_list args;
  va_start(args, flags);
  int mode = va_arg(args, int);
  va_end(args);

  return real_func(get_redirected_path(path, new_path, sizeof(new_path)), flags, mode);
}

typedef int (*real_open64_t)(const char *path, int flags, ...);
int open64(const char *path, int flags, ...)
{
  char new_path[4096];
  real_open64_t real_func = (real_open64_t)dlsym(RTLD_NEXT, "open64");

  va_list args;
  va_start(args, flags);
  int mode = va_arg(args, int);
  va_end(args);

  return real_func(get_redirected_path(path, new_path, sizeof(new_path)), flags, mode);
}

typedef int (*real_mkdir_t)(const char *path, mode_t mode);
int mkdir(const char *path, mode_t mode)
{
  char new_path[4096];
  real_mkdir_t real_func = (real_mkdir_t)dlsym(RTLD_NEXT, "mkdir");
  return real_func(get_redirected_path(path, new_path, sizeof(new_path)), mode);
}

typedef int (*real_access_t)(const char *path, int mode);
int access(const char *path, int mode)
{
  char new_path[4096];
  real_access_t real_func = (real_access_t)dlsym(RTLD_NEXT, "access");
  return real_func(get_redirected_path(path, new_path, sizeof(new_path)), mode);
}

typedef int (*real_stat_t)(const char *path, struct stat *buf);
int stat(const char *path, struct stat *buf)
{
  char new_path[4096];
  real_stat_t real_func = (real_stat_t)dlsym(RTLD_NEXT, "stat");
  return real_func(get_redirected_path(path, new_path, sizeof(new_path)), buf);
}

typedef int (*real_lstat_t)(const char *path, struct stat *buf);
int lstat(const char *path, struct stat *buf)
{
  char new_path[4096];
  real_lstat_t real_func = (real_lstat_t)dlsym(RTLD_NEXT, "lstat");
  return real_func(get_redirected_path(path, new_path, sizeof(new_path)), buf);
}