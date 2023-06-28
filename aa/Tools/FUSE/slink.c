/*
FUSE FPS server browser
Copyright (C) 2008  Tony Jackson

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdio.h>

#define FD_SETSIZE 512  /* This sets the size of fd_set structure in winsock.h */
#ifdef WINDOWS
#define PING_RETEST_THRESH 150 /* ping times, in ms, that will be retested accurately */
#endif

#ifdef LINUX
#define PING_RETEST_THRESH 300 /* ping times, in ms, that will be retested accurately */
#endif

//#define DEBUG
//#define DEBUGCONNECTION

#ifdef WINDOWS
#include <winsock.h>
#endif

#ifdef LINUX
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib/gprintf.h>

#include "global.h"
#include "slink.h"

/* Local prototypes */
void TidyServerInfo             (tServer *server);
void Process_Payload            (tServer *server, guchar *payload, gint size);
void SERVER_Send_UDP            (GQuark key_id, gpointer data, gpointer user_data);
void SERVER_Ping_UDP            (GQuark key_id, gpointer data, gpointer user_data);
void SERVER_Purge_Nonresponsive (GQuark key_id, gpointer data, gpointer user_data);
void SERVER_free                (gpointer ptr);
gboolean isName                 (gchar *str);
gboolean strToIPPort            (gchar *str, gulong *ip, guint *port);
void MasterSum                  (GQuark key_id, gpointer data, gpointer user_data);
void MasterMerge                (GQuark key_id, gpointer data, gpointer user_data);

#ifdef LINUX
#define SOCKADDR_IN struct sockaddr_in
#define HOSTENT     struct hostent
#define SOCKET_ERROR (-1)
#define INVALID_SOCKET (-1)
#define SET_IP(addr,ip) ((addr).sin_addr.s_addr = (ip))
#define closesocket(X) (close(X))
void Open_Winsock(void) {}
void Close_Winsock(void) {}
#endif

#ifdef WINDOWS
#define SET_IP(addr,ip) ((addr).sin_addr.S_un.S_addr = (ip))
void Open_Winsock(void)
{
	WSADATA ws;
	int error;

	/* initialize Winsock with sockets version 1.1 */
	error = WSAStartup ((WORD)MAKEWORD(1,1), &ws);

	if (error)
    {
        POPUP("Couldn't load Winsock");
	}
}

void Close_Winsock(void)
{
	WSACleanup();
}
#endif

/* Private structure declaration */
typedef struct _tUserData
{
   	GTimer *timer;
   	gdouble now;         /* Used to sample time when select() shows UDP packets arrived */
	fd_set  masterfds;   /* Master set of file (socket) descriptors */
	fd_set  readfds;     /* Copy of the master list, which select() will modify */
    guint   maxfd;       /* For select() - biggest file descriptor (socket) seen */
    gchar  *querystring; /* String to send to servers */
    guint   responded;   /* Running total - number of servers that responded */
    GData **datalist;    /* For self-referential foreach() functions */
} tUserData;

/* Process payloads depending on whether this is master response or a games
   server.

   Master response is of the form:
   12 byte header
    4 byte IP
    2 byte port
    4 byte IP
    2 byte port
    ...

   Game server responses are of the form:
    "ÿÿÿÿprint\n\\mapname\\ctf-killbox\\needpass\\0\\gamedate\\Jan 31 2007\\gamename\\data1\\maxspectators\\4\\Admin\\Forsaken\\website\\http://www.alienarena.info\\sv_joustmode\\0\\maxclients\\16\\protocol\\34\\cheats\\0\\timelimit\\0\\fraglimit\\10\\dmflags\\2641944\\deathmatch\\1\\version\\6.03 x86 Jan  7 2007 Win32 RELEASE\\hostname\\Alienarena.info - CTF\\gamedir\\arena\\game\\arena\n"
    "ÿÿÿÿprint\n\\mapname\\DM-OMEGA\\needpass\\0\\maxspectators\\4\\gamedate\\Jan  9 2007\\gamename\\data1\\sv_joustmode\\0\\maxclients\\8\\protocol\\34\\cheats\\0\\timelimit\\0\\fraglimit\\10\\dmflags\\16\\version\\6.03 x86 Jan  7 2007 Win32 RELEASE\\hostname\\pufdogs hell\\gamedir\\arena\\game\\arena\n3 17 \"test chap\" \"loopback\"\n0 0 \"Cyborg\" \"127.0.0.1\"\n3 0 \"Squirtney\" \"127.0.0.1\"\n0 0 \"Butthead\" \"127.0.0.1\"\n"
*/

void Process_Payload(tServer *server, guchar *payload, gint size)
{
    guint i, num;

    switch(server->type)
    {
        case MASTER:
        	if ((payload == NULL) || (size <= 12))
        	    return;

            num = (size - 12) / 6;
            server->info = g_malloc(sizeof(gchar *)*(num+1));
            if(server->info == NULL)
               return;

            payload += 12;  /* Skip header string */
            for(i = 0; i < num; i++)
            {
                /* Allocate string big enough to hold XXX.XXX.XXX.XXX:XXXXX (21 chars) */
                server->info[i] = g_malloc(sizeof(gchar) * 22);
                /* In case of malloc failure, naturally NULL terminates list - neat! */
                if(server->info[i] == NULL)
                    return;
                g_sprintf(server->info[i], "%u.%u.%u.%u:%u",
                            *(payload+0),
                            *(payload+1),
                            *(payload+2),
                            *(payload+3),
                            (*(payload+4)<<8) | *(payload+5));
                payload+=6;
                /*POPUP("Extracted server %s", server->info[i]);*/
            }

            /* Add NULL terminator */
            server->info[num] = NULL;
        break;

        case GAME:
        {
            gchar **split;
            gchar  *token;
            guint   len;
            guint   numplayers;
            guint   i;
            payload[size] = 0; /* Very, very important to null terminate the
                                  game string! */

/*           POPUP("Game string => %s", payload); */

            split = g_strsplit((gchar *)payload, "\n", 0);

            len = g_strv_length(split);

            if(len < 2)
            {   /* Incomplete */
                g_strfreev(split);
                return;
            }

            /* First line is response header (no need to check) */
            /* Second line is server information */
            server->info = g_strsplit(split[1], "\\", 0);
            /* We keep this data - it has to be freed in SERVER_Free() */

            /* Lines 3-n contain player information */
            /* Allocate memory for player information */
            numplayers = len - 3;
            server->player = g_malloc(sizeof(tPlayer *) * (numplayers + 1));

            for (i = 0; i < numplayers; i++)
            {
                server->player[i] = g_malloc(sizeof(tPlayer));
                if(server->player[i] == NULL)
                    break;

				/* Establish string and get the first token: */
				token = strtok( split[i+2], " ");
				server->player[i]->score = atoi(token);

				token = strtok( NULL, " ");
				server->player[i]->ping = atoi(token);

				token = strtok( NULL, "\"");
				//token++;

				if (token)
					g_strlcpy (server->player[i]->playername, token, PLAYERNAME_LEN-1);
				else
					server->player[i]->playername[0] = '\0';
            }

            server->player[i] = NULL;

            g_strfreev(split);
            TidyServerInfo(server);
        }
        break;

        default:
            return;
        break;
    }
}

/* Send UDP packet to server */
void SERVER_Send_UDP(GQuark key_id, gpointer data, gpointer user_data)
{
	SOCKADDR_IN addr;
	gint result;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    tServer *server = (tServer *)data;
    tUserData *ud = (tUserData *)user_data;
    SET_IP(addr, server->ip);
	addr.sin_port = htons(server->port);

    server->start_ping = g_timer_elapsed(ud->timer, NULL); /* Time how long it takes to get a response */

	result = sendto (server->socket, ud->querystring, strlen(ud->querystring), 0, (struct sockaddr *)&addr, sizeof(addr));
	if (result == SOCKET_ERROR)
	{
        #ifdef DEBUGCONNECTION
        POPUP("Send error = %d", WSAGetLastError() );
        #endif
        closesocket(server->socket);
        /* Delete this entry from the list */
        g_datalist_id_remove_data(ud->datalist, key_id);
	}
	else
	{
#ifdef DEBUG
        POPUP("Sent to %d", server->ip);
#endif
		FD_SET(server->socket, &(ud->masterfds)); /* Add socket to master set */
//		socketcount++; /* Record of how many sockets are open, for searching in select() */
		if(server->socket > ud->maxfd) /* Work out the maximum file descriptor used for select() */
			ud->maxfd = server->socket;
	}
}

/* Read from ready sockets */
void SERVER_Receive_UDP(GQuark key_id, gpointer data, gpointer user_data)
{
	gint size;
	static gchar payload[4096]; /* Don't put this on the stack each time */

    tServer *server = (tServer *)data;
    tUserData *ud = (tUserData *)user_data;
#ifndef OFFLINE
    struct sockaddr dgFrom;
    guint fromlen;
#endif

    #ifdef OFFLINE
    {
        gchar filename[80];
        FILE *file;

        g_stpcpy(filename, g_quark_to_string(key_id));
        g_strdelimit (filename, ":", '-');
        g_sprintf(filename, "offline/%s.udp", filename);
        file = fopen(filename, "rb");
        if (file == NULL)
           return;

        size = fread(payload, 1, 2048, file);
        fclose(file);
        Process_Payload(server, payload, size);
        ud->responded++;
    }
    #else
    if(FD_ISSET(server->socket, &ud->readfds))
    {
        server->ping = (ud->now - server->start_ping) * 1000;
    	/* Ready to read */
    	fromlen = sizeof(dgFrom);
    	size = recvfrom (server->socket, payload, sizeof(payload)-1, 0, &dgFrom, &fromlen);

        /* Debug WinSock errors */
        #ifdef DEBUGCONNECTION
        if(size < 0)
            POPUP("Receive error = %d", WSAGetLastError() );
        #endif

        closesocket(server->socket);
		FD_CLR(server->socket, &ud->masterfds);
        if(size >= 0)
        {
            #ifdef DEBUGCONNECTION
            gchar debugmsg[8192];
            guint i;

            debugmsg[0] = 0;
            for(i = 0; i < size; i++)
            {
                if(i%16 == 0)
                    g_sprintf(debugmsg+strlen(debugmsg), "\n%04X:", i);
                g_sprintf(debugmsg+strlen(debugmsg), "%02X ", (guchar)payload[i]);
            }
            g_snprintf(debugmsg+strlen(debugmsg), size+11, "\n\nString = %s", payload);

			POPUP("Payload received - %d bytes\n%s", size, debugmsg);
            #endif
            Process_Payload(server, (guchar *)payload, size);
    	    ud->responded++;
        }
        else
        {
            /* Delete this entry from the list */
            g_datalist_id_remove_data(ud->datalist, key_id);
        }
    }
    #endif

#ifdef DEBUG
    switch(server->type)
    {
        case MASTER:
             POPUP("Found %d servers on %s", g_strv_length(server->info), g_quark_to_string(key_id));
        break;

        case GAME:
             POPUP("Found %d players on %s", g_strv_length(server->player), g_quark_to_string(key_id));
        break;

        default:
            return;
        break;
    }
#endif

}

/* Send UDP packet to server */
void SERVER_Ping_UDP(GQuark key_id, gpointer data, gpointer user_data)
{
    tServer *server = (tServer *)data;
    tUserData *ud = (tUserData *)user_data;

	SOCKADDR_IN addr;
	gint result;
    SET_IP(addr,server->ip);
	addr.sin_port = htons(server->port);
   	struct timeval delay;

    if((g_strv_length(server->info) == 0) || (server->ping >= PING_RETEST_THRESH))
        return;

/*    POPUP("Re-ping %s, old ping was %d", g_quark_to_string(key_id), server->ping); */

	FD_ZERO(&ud->masterfds);
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;

	server->socket = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(server->socket == INVALID_SOCKET)
        return;

    SET_IP(addr, server->ip);
	addr.sin_port = htons(server->port);

	server->start_ping = g_timer_elapsed(ud->timer, NULL); /* Time how long it takes to get a response */

	result = sendto (server->socket, ud->querystring, strlen(ud->querystring), 0, (struct sockaddr *)&addr, sizeof(addr));

	if (result == SOCKET_ERROR)
	{
	   closesocket(server->socket);
       return;
    }

	FD_SET(server->socket, &ud->masterfds); /* Add socket to master set */

	delay.tv_sec = 1;
	delay.tv_usec = 0;

	if((result = select (server->socket+1, &ud->masterfds, NULL, NULL, &delay)) > 0)
	{
		server->ping = (g_timer_elapsed(ud->timer, NULL) - server->start_ping) * 1000;
	}

	closesocket(server->socket);
}

void SERVER_Purge_Nonresponsive(GQuark key_id, gpointer data, gpointer user_data)
{
    tServer *server = (tServer *)data;
    tUserData *ud = (tUserData *)user_data;
    if(server->info == NULL)
    {
        closesocket(server->socket);
        /* Delete this entry from the list */
        g_datalist_id_remove_data(ud->datalist, key_id);
    }
}

/* Free tServer data and any associated memory */
void SERVER_free(gpointer ptr)
{
    tServer *server = (tServer *)ptr;
    guint i = 0;
    if(server != NULL)
    {
         g_strfreev(server->info);

         if(server->player != NULL)
             for(i = 0; server->player[i] != NULL; i++)
                   g_free(server->player[i]);
         g_free(server);
    }
}

/* Checks if input string is dotted IP or name string */
gboolean isName(gchar *str)
{
    guint i;

    for(i = 0; str[i] != '\0'; i++)
        if(g_ascii_isalpha(str[i]))
            return TRUE;

    return FALSE;
}

/* Function to convert string of form:
    "192.168.1.1:27910"
    "master.corservers.com:27900"
   into network IP and port, resolving name if necessary.

   Returns FALSE if unable to resolve name, or other error.
*/
gboolean strToIPPort(gchar *str, gulong *ip, guint *port)
{
    HOSTENT *hp;
    gchar **split;

    split = g_strsplit(str, ":", 2);

    if(g_strv_length(split) < 2)
    {   /* Incomplete (maybe missing port specification) */
        g_strfreev(split); /* OK to call on empty vector */
        return FALSE;
    }
    if(isName(split[0]))
    {   /* Name string to resolve */
        hp = gethostbyname(split[0]);
        if(hp == NULL)
        {
/*            POPUP("Unable to resolve %s", str); */
            return FALSE; /* Try next one in list */
        }
        *ip = *(gulong *) hp->h_addr_list[0];
    }
    else
    {   /* IP address dotted string */
        guint ipv4[4]; /* Has to be int for sscanf %d to work */
        guint result;
        result = sscanf(split[0], "%u.%u.%u.%u", &ipv4[3], &ipv4[2], &ipv4[1], &ipv4[0]);
        if(result == 4)
        {
            /* Network order is reverse of this */
            *ip =   (ipv4[0] << 24) |
                    (ipv4[1] << 16) |
                    (ipv4[2] << 8) |
                    (ipv4[3]);
        }
        else
        {
            /* Incomplete string */
            g_strfreev(split); /* OK to call on empty vector */
            return FALSE;
        }
    }

    *port = atoi(split[1]);
    g_strfreev(split);
    return TRUE;
}

guint ServerLinkQuery(GData **serverdata, gchar **servers, gchar *protocol, eServerType type, gboolean reping)
{
    gint i;
    gint len;
    tUserData ud;
#ifndef OFFLINE
   	struct timeval delay;
#endif

    g_datalist_clear(serverdata);

	FD_ZERO(&ud.masterfds);
	FD_ZERO(&ud.readfds);
    ud.timer = g_timer_new();
    ud.maxfd = 0;
    ud.responded = 0;
    ud.datalist = serverdata; /* So g_datalist_foreach() functions can self-modify list */

    switch (type)
    {
        case MASTER:
			if(g_strcasecmp(protocol, "q2") == 0)
	            ud.querystring = "query";
			else if(g_strcasecmp(protocol, "q2w") == 0)
	            ud.querystring = "ÿÿÿÿgetservers";
			else if(g_strcasecmp(protocol, "trem") == 0)
	            ud.querystring = "ÿÿÿÿgetservers 69";
	            /* Can specify " 67 empty full" */
	            /* 67    => darkplaces client
                   empty => no empty servers
                   full  => no full servers */
			else /* Try Q2 style */
	            ud.querystring = "query";
        break;

        case GAME:
            ud.querystring = "ÿÿÿÿstatus\n";
        break;

        default:
            return 0;
        break;
    }


    /* Find out how many servers we are querying */
    len = g_strv_length(servers);
    if(len == 0)
        return 0;

	/* Create data entries and open sockets */
    for(i = 0; i < len; i++)
    {
        tServer *server;
        gulong  ip;
        guint   port;

        if(!strToIPPort(servers[i], &ip, &port))
            continue; /* Unable to convert, or resolve name, or other error */

/*	POPUP("Server %s => %d.%d.%d.%d:%d",
		servers[i],
		(ip & 0x000000FF) >> 0,
		(ip & 0x0000FF00) >> 8,
		(ip & 0x00FF0000) >> 16,
		(ip & 0xFF000000) >> 24,
		port);*/

        server = g_malloc(sizeof(tServer));
        if(server == NULL)
            continue;
        server->type = type;
        server->info = NULL;
        server->player = NULL;
        server->ping = 0;
        #ifdef OFFLINE
        server->socket = INVALID_SOCKET;
        #else
        server->socket = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        server->ip = ip;
        server->port = port;
        #endif
        #ifndef OFFLINE
        if(server->socket == INVALID_SOCKET)
        {
            g_free(server);
        }
        else
        #endif
        {
            g_datalist_set_data_full(serverdata, servers[i], server, SERVER_free);
        }
    }

    g_timer_start(ud.timer);

#ifdef OFFLINE /* Offline mode */
    /* This will check for data in files rather than waiting for UDP packets to arrive */
    g_datalist_foreach(serverdata, SERVER_Receive_UDP, &ud);
#else

    g_datalist_foreach(serverdata, SERVER_Send_UDP, &ud);

	/* Use a copy of the master file descriptor list, as select() modifies the SET bits to indicate which sockets have an event */
	memcpy(&ud.readfds, &ud.masterfds, sizeof(fd_set));

	/* Set timeout - this is reset every time select() receives a packet, so is not an absolute time across all servers */
	delay.tv_sec = 0;
#ifdef WINDOWS
	delay.tv_usec = 250000l; /* 250ms - so far OK for Windows users */
#endif
#ifdef LINUX
	delay.tv_usec = 750000l; /* 750ms - some Linux users report lack of servers - this seems to fix it */
#endif
	while(select(ud.maxfd+10000, &ud.readfds, NULL, NULL, &delay) > 0) /* Result is number of sockets ready to read */
    {
        /* Grab current time */
   		ud.now = g_timer_elapsed(ud.timer, NULL);  /* Grab time to measure server ping */
        /* This foreach receives data on waiting sockets, and clears & closes
           completed sockets from the master FD set */
        g_datalist_foreach(serverdata, SERVER_Receive_UDP, &ud);
        /* Restore read fd list from master (modified by select call) */
        memcpy(&ud.readfds, &ud.masterfds, sizeof(fd_set));
    }
#endif

    /* Close remaining sockets/delete empty data entries from list */
    g_datalist_foreach(serverdata, SERVER_Purge_Nonresponsive, &ud);

#ifndef OFFLINE
    if(reping) /* Re-ping servers for more accurate ping values */
        g_datalist_foreach(serverdata, SERVER_Ping_UDP, &ud);
#endif
    g_timer_destroy(ud.timer);

    return ud.responded;
}

gchar *GetServerInfo(tServer *server, gchar *query)
{
    guint i = 0;

    /* Checks for valid data */
    if(server == NULL || query == NULL)
        return NULL;

    if(server->info == NULL)
        return NULL;

    if(server->info[0] == NULL)
        return NULL;

    /* Entries are stored as pairs, IE:
       server->info[1] => "mapname"
       server->info[2] => "dm-dynamo2"
       ...
       Note that server->info[0] is always "" */

    i++; /* Skip server->info[0] */

    /* By C standard, if first evaluation in an && fails, the second is not performed
       so this is safe to do */
    while(server->info[i] != NULL && server->info[i+1] != NULL)
    {
        if (!g_ascii_strcasecmp (server->info[i], query))
           return server->info[i+1];
        i += 2;
    }

    return NULL;
}

/* Run through server data and clean up case and anything else required */
void TidyServerInfo(tServer *server)
{
    guint i = 0;

    /* Checks for valid data */
    if(server == NULL)
        return;

    if(server->info == NULL)
        return;

    if(server->info[0] == NULL)
        return;

    /* Entries are stored as pairs, IE:
       server->info[1] => "mapname"
       server->info[2] => "dm-dynamo2"
       ...
       Note that server->info[0] is always "" */

    i++; /* Skip server->info[0] */

    /* By C standard, if first evaluation in an && fails, the second is not performed
       so this is safe to do */
    while(server->info[i] != NULL && server->info[i+1] != NULL)
    {
        if (!g_ascii_strcasecmp (server->info[i], "mapname"))
        {
            gchar *temp;
            temp = g_ascii_strdown(server->info[i+1], -1);
			g_stpcpy (server->info[i+1], temp);
			g_free(temp);
        }
        i += 2;
    }
}

typedef struct _tMerge
{
    guint numservers;
    gchar **mergelist;
} tMerge;

/* Sums count of servers listed on master */
void MasterSum(GQuark key_id, gpointer data, gpointer user_data)
{
    tServer *server = (tServer *)data;
    tMerge  *ud     = (tMerge *)user_data;
    ud->numservers += g_strv_length(server->info);
}

/* Add servers to list */
void MasterMerge(GQuark key_id, gpointer data, gpointer user_data)
{
    tServer *server = (tServer *)data;
    tMerge  *ud     = (tMerge *)user_data;
    guint i, j, len;
    len = g_strv_length(ud->mergelist);
    if(server->info != NULL)
    {
        for(i = 0; server->info[i] != NULL; i++)
        {
           for(j = 0; j < len; j++)
           {
               if(!g_ascii_strcasecmp(server->info[i], ud->mergelist[j]))
                   break;
           }
           if(j == len) /* Not found */
                ud->mergelist[len++] = server->info[i];
        }
        ud->mergelist[len] = NULL;
    }
}

/* Returns NULL terminated list of servers from all masters
   and manually added servers
   !!!CAUTION!!!
   Does not copy string data from GData list, but holds pointers alone.
   DO NOT CALL g_strfreev() ON THIS LIST!
   To free, just use g_free on the returned pointer.
*/
gchar **ServerLinkMergeMasters(GData **serverdata, gchar **manualservers)
{
    gchar **result;
    guint nummanuals = 0;
    guint i, j, len;
    tMerge ud;
    ud.numservers = 0;
    nummanuals = g_strv_length(manualservers);

    g_datalist_foreach(serverdata, MasterSum, &ud);
    ud.numservers += nummanuals;

    /* Allocate big array in case all servers are unique */
    ud.mergelist = g_malloc(sizeof(gchar *) * (ud.numservers + 1));
    ud.mergelist[0] = NULL;


    /* First add any manually specified servers (and check for dupes) */
    len = 0;
    for(i = 0; manualservers[i] != NULL; i++)
    {
        for(j = 0; j < len; j++) /* See if already in list */
        {
            if(!g_ascii_strcasecmp(ud.mergelist[j], manualservers[i]))
                break;
        }
        if(j == len) /* Not found */
            ud.mergelist[len++] = manualservers[i];
    }
    ud.mergelist[len] = NULL;

    /* Now append server lists from masters (and check for dupes) */
    g_datalist_foreach(serverdata, MasterMerge, &ud);

    /* Now we know length, copy pointers to smaller array */
    ud.numservers = g_strv_length(ud.mergelist);
    result = g_memdup(ud.mergelist, sizeof(gchar *) * (ud.numservers + 1));

//    POPUP("Unique servers = %d", g_strv_length(result));
    g_free(ud.mergelist);
    return result;
}
