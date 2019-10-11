#define _GNU_SOURCE
#define __USE_LARGEFILE64

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <dlfcn.h>

#if DEBUG > 0
#define D(x...) printf(x)
#else
#define D(x...)
#endif

typedef struct {
	char *dirname;
	char *orig_name;
	char *dest_name;
} ReDirected;

#define NUM_DIRS (sizeof (dirs) / sizeof ((dirs)[0]))
#include "directories.h"

static DIR *(*func_opendir)(const char *name) = 0;
static FILE *(*func_fopen64)(const char *pathname, const char *mode) = 0;
static int (*func_xstat64)(int __ver, const char *__filename, struct stat64 *__stat_buf);
static int (*func_mkdir)(const char *path, mode_t mode);

static void
init (void)
{
	unsigned int i;
	const char *home;

	func_opendir = dlsym(RTLD_NEXT, "opendir");
	func_fopen64 = dlsym(RTLD_NEXT, "fopen64");
	func_xstat64 = dlsym(RTLD_NEXT, "__xstat64");
	func_mkdir = dlsym(RTLD_NEXT, "mkdir");

	home = getenv ("HOME");
	assert (home);

	for (i = 0; i < NUM_DIRS; i++) {
		char buffer[PATH_MAX];

		/* eg. ~/.adobe */
		memset (buffer, 0, PATH_MAX);
		strcat (buffer, home);
		if (buffer[strlen (buffer) - 1] != '/')
			strcat (buffer, "/");
		strcat (buffer, dirs[i].dirname);

		dirs[i].orig_name = strdup (buffer);
		D("orig name: %s\n", dirs[i].orig_name);

		/* eg. ~/.local/share/com.adobe.Flash-Player-Projector/.adobe */
		memset (buffer, 0, PATH_MAX);
		strcat (buffer, home);
		if (buffer[strlen (buffer) - 1] != '/')
			strcat (buffer, "/");
		strcat (buffer, REDIRECT_DIR);
		strcat (buffer, dirs[i].dirname);

		dirs[i].dest_name = strdup (buffer);
		D("dest name: %s\n", dirs[i].dest_name);
	}
}

DIR *
opendir (const char *name)
{
	unsigned int i;

	if (!func_opendir)
		init();

	D("opendir(\"%s\")\n", name);

	for (i = 0; i < NUM_DIRS; i++) {
		if (strncmp (name, dirs[i].orig_name, strlen (dirs[i].orig_name)) == 0) {
			char dest[PATH_MAX];

			memset (dest, 0, PATH_MAX);
			strcat (dest, dirs[i].dest_name);
			strcat (dest, name + strlen (dirs[i].orig_name));
			D("redirection %s to %s\n", name, dest);
			return (*func_opendir) (dest);
		}
	}

	return (*func_opendir) (name);
}

FILE *fopen64(const char *pathname, const char *mode)
{
	unsigned int i;

	if (!func_fopen64)
		init();

	D("fopen64(\"%s\", \"%s\")\n", pathname, mode);

	for (i = 0; i < NUM_DIRS; i++) {
		if (strncmp (pathname, dirs[i].orig_name, strlen (dirs[i].orig_name)) == 0) {
			char dest[PATH_MAX];

			memset (dest, 0, PATH_MAX);
			strcat (dest, dirs[i].dest_name);
			strcat (dest, pathname + strlen (dirs[i].orig_name));
			D("redirection %s to %s\n", pathname, dest);
			return (*func_fopen64) (dest, mode);
		}
	}

	return (*func_fopen64) (pathname, mode);
}

int __xstat64(int __ver, const char *__filename, struct stat64 *__stat_buf)
{
	unsigned int i;

	if (!func_xstat64)
		init();

	D("xstat64(\"%s\", ...)\n", __filename);

	for (i = 0; i < NUM_DIRS; i++) {
		if (strncmp (__filename, dirs[i].orig_name, strlen (dirs[i].orig_name)) == 0) {
			char dest[PATH_MAX];

			memset (dest, 0, PATH_MAX);
			strcat (dest, dirs[i].dest_name);
			strcat (dest, __filename + strlen (dirs[i].orig_name));
			D("redirection %s to %s\n", __filename, dest);
			return (*func_xstat64) (__ver, dest, __stat_buf);
		}
	}

	return (*func_xstat64) (__ver, __filename, __stat_buf);
}

int mkdir(const char *path, mode_t mode)
{
	unsigned int i;

	if (!func_mkdir)
		init();

	D("mkdir(\"%s\", 0%o)\n", path, mode);

	for (i = 0; i < NUM_DIRS; i++) {
		if (strncmp (path, dirs[i].orig_name, strlen (dirs[i].orig_name)) == 0) {
			char dest[PATH_MAX];

			memset (dest, 0, PATH_MAX);
			strcat (dest, dirs[i].dest_name);
			strcat (dest, path + strlen (dirs[i].orig_name));
			D("redirection %s to %s\n", path, dest);
			return (*func_mkdir) (dest, mode);
		}
	}

	return (*func_mkdir) (path, mode);
}
