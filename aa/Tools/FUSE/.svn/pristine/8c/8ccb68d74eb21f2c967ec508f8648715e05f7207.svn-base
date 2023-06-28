#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <gtk/gtk.h>
#include <glade/glade.h>

#define MAXMASTERS  16
#define MAXGAMES    16

typedef struct _tGameConfig
{
    gchar   *protocol;
    gchar   *launch;
    gchar   *exewin;
    gchar   *exelin;
    gchar   *pathwin;
    gchar   *pathlin;
    gchar   *masters[MAXMASTERS+1];     /* NULL terminated array of strings */
    gchar   *gameservers[MAXGAMES+1];   /* NULL terminated array of strings */
} tGameConfig;

gboolean Config_LoadGames(GData **gameconfigs, gchar *filename);

#endif
