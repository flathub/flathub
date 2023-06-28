#ifndef _SLINK_H_
#define _SLINK_H_

#include <gtk/gtk.h>
#include <glib.h>

#define MAXSERVERS 256

typedef enum
{
    MASTER,
    GAME
} eServerType;

#define PLAYERNAME_LEN 64

typedef struct _tPlayer {
	gchar playername[PLAYERNAME_LEN];
	guint ping;
	gint  score;
} tPlayer;

typedef struct _tServer {
    eServerType     type; /* MASTER or GAME */
    guint           ip;
    gushort         port;
    guint           ping;
    gulong          socket;
    gdouble         start_ping;
    gchar           **info;     /* Contains either server config or master list */
    tPlayer         **player;    /* Contains player information */
} tServer;

/* Returns nothing - will return number of servers that responded */
guint ServerLinkQuery(GData **serverdata, gchar **servers, gchar *protocol, eServerType type, gboolean reping);
gchar **ServerLinkMergeMasters(GData **serverdata, gchar **manualservers);
void Open_Winsock(void);
void Close_Winsock(void);

//tServer *GetServerByIpPort(SLHANDLE *handle, gchar *ip_port_str);
gchar *GetServerInfo(tServer *server, gchar *query);

#endif
