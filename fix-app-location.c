#define _GNU_SOURCE
#define __USE_LARGEFILE64

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <dlfcn.h>

#if DEBUG > 0
#define D(x...) printf(x)
#else
#define D(x...)
#endif

static int (*func_chdir)(const char *path) = 0;

static void
init (void)
{
	func_chdir = dlsym(RTLD_NEXT, "chdir");
}

int
chdir (const char *path)
{
	if (!func_chdir)
		init();

	D("chdir(\"%s\")\n", path);
	if (strncmp (path, "/opt/scanner", strlen("/opt/scanner")) == 0) {
		char dest[PATH_MAX];

		memset (dest, 0, PATH_MAX);
		strcat (dest, "/app/extra/scanner");
		strcat (dest, path + strlen ("/opt/scanner"));
		D("redirection %s to %s\n", path, dest);
		return (*func_chdir) (dest);
	}

	return (*func_chdir) (path);
}
