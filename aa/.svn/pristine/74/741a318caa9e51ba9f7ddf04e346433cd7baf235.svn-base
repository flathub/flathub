/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "server.h"

static size_t szr; // just for quieting unused result warnings

/*
===============================================================================

OPERATOR CONSOLE ONLY COMMANDS

These commands can only be entered from stdin or by a remote operator datagram
===============================================================================
*/

/*
====================
SV_SetMaster_f

Specify a list of master servers
====================
*/
void SV_SetMaster_f (void)
{
	int		i;

	// make sure the server is listed public
	Cvar_Set ("public", "1");

	// Clear the master server status array
	memset (master_status, 0, sizeof(master_status));

	if ( dedicated && dedicated->value )
	{
		// Dedicated server, get the names from the command line arguments
		for ( i = 1; i < Cmd_Argc() && i < MAX_MASTERS + 1 ; i++ )
		{
			strncpy (master_status[i - 1].name, Cmd_Argv(i), MAX_MASTER_LEN);
		}
	}

	svs.last_heartbeat = -9999999;

	// Resolve the names and send a ping
	SV_HandleMasters ( "ping" , "ping" );
}



/*
==================
SV_SetPlayer

Sets sv_client and sv_player to the player with idnum Cmd_Argv(1)
==================
*/
qboolean SV_SetPlayer (void)
{
	client_t	*cl;
	int			i;
	int			idnum;
	char		*s;

	if (Cmd_Argc() < 2)
		return false;

	if ( svs.clients == NULL )
		return false; // unlikely, but possible, to call here before initialized

	s = Cmd_Argv(1);

	// numeric values are just slot numbers
	if (s[0] >= '0' && s[0] <= '9')
	{
		idnum = atoi(Cmd_Argv(1));
		if (idnum < 0 || idnum >= maxclients->value)
		{
			Com_Printf ("Bad client slot: %i\n", idnum);
			return false;
		}

		sv_client = &svs.clients[idnum];
		sv_player = sv_client->edict;
		if (!sv_client->state)
		{
			Com_Printf ("Client %i is not active\n", idnum);
			return false;
		}
		return true;
	}

	// check for a name match
	for (i=0,cl=svs.clients ; i<maxclients->value; i++,cl++)
	{
		if (!cl->state)
			continue;
		if (!strcmp(cl->name, s))
		{
			sv_client = cl;
			sv_player = sv_client->edict;
			return true;
		}
	}

	Com_Printf ("Userid %s is not on the server\n", s);
	return false;
}

/*
==================
SV_DemoMap_f

Puts the server in demo mode on a specific map/cinematic
==================
*/
void SV_DemoMap_f (void)
{
	SV_Map (true, Cmd_Argv(1), false );
}

/*
==================
SV_Map_f

Goes directly to a given map without any savegame archiving.
For development work
==================
*/
static char *SV_MapCommand_Completer (int argnum, int ignore)
{
	int		i;
	char	*ret = NULL;
	char	findname[MAX_QPATH];
	char	**foundmaps, **founddemos, **matches;
	int		nmaps = 0, ndemos = 0, nmatches;
	
	assert (argnum == 1);
	
	if (Cmd_Argc () <= argnum)
		return Cmd_MakeCompletedCommand (argnum, NULL);
	
	Com_sprintf (findname, sizeof (findname), "maps/%s*.bsp", Cmd_Argv (argnum));
	foundmaps = FS_ListFilesInFS (findname, &nmaps, 0, 0);
	if (nmaps < 0)
		nmaps = 0;
	
	Com_sprintf (findname, sizeof (findname), "demos/%s*", Cmd_Argv (argnum));
	founddemos = FS_ListFilesInFS (findname, &ndemos, 0, 0);
	if (ndemos < 0)
		ndemos = 0;
	
	// We might filter out some of the demo results, so nmatches isn't
	// necessarily equal to nmaps + ndemos. But we allocate that many pointers
	// anyway.
	matches = Z_Malloc (sizeof (char *) * (nmaps + ndemos));
	nmatches = 0;
	
	if (foundmaps != NULL)
	{
		for (i = 0; i < nmaps; i++)
		{
			char *find = strrchr (foundmaps[i], '/') + 1;
			memmove (foundmaps[i], find, strlen (find) + 1);
			
			COM_StripExtension (foundmaps[i], foundmaps[i]);
			matches[nmatches++] = foundmaps[i]; 
		}
		
		free (foundmaps);
	}
	
	if (founddemos != NULL)
	{
		for (i = 0; i < ndemos; i++)
		{
			char *find = strrchr (founddemos[i], '/') + 1;
			memmove (founddemos[i], find, strlen (find) + 1);
			
			if (COM_HasExtension (founddemos[i], ".dm2"))
				matches[nmatches++] = founddemos[i];
			else
				free (founddemos[i]);
		}
		
		free (founddemos);
	}
	
	ret = Cmd_CompleteWithInexactMatch (argnum, nmatches, matches);
	
	for (i = 0; i < nmatches; i++)
		free (matches[i]);
	
	Z_Free (matches);
	
	return ret;
}

static qboolean SV_MapCommand_CompletionChecker (int argnum, int ignore)
{
	char	findname[MAX_QPATH];
	
	assert (argnum == 1);
	
	if (Cmd_Argc () <= argnum)
		return true;
	
	if (COM_HasExtension (Cmd_Argv (argnum), ".dm2"))
		Com_sprintf (findname, sizeof (findname), "demos/%s", Cmd_Argv (argnum));
	else
		Com_sprintf (findname, sizeof (findname), "maps/%s.bsp", Cmd_Argv (argnum));
	return FS_FileExists (findname);
}

void SV_Map_f (void)
{
	char	*map;
	char	expanded[MAX_QPATH];

	// if not a pcx, demo, or cinematic, check to make sure the level exists
	map = Cmd_Argv(1);
	if (!strstr (map, "."))
	{
		Com_sprintf (expanded, sizeof(expanded), "maps/%s", map);
		if (FS_LoadFile (expanded, NULL) == -1)
		{
			Com_sprintf (expanded, sizeof(expanded), "maps/%s.bsp", map);
			if (FS_LoadFile (expanded, NULL) == -1)
			{
				Com_Printf ("Cannot find %s\n", expanded);
				return;
			}
		}
	}

	// start up the next map
	SV_Map (false, Cmd_Argv(1), false );

	// archive server state
	strncpy (svs.mapcmd, Cmd_Argv(1), sizeof(svs.mapcmd)-1);
}
void SV_StartMap_f (void)
{
	char	*map;
	char	expanded[MAX_QPATH];

	// if not a pcx, demo, or cinematic, check to make sure the level exists
	map = Cmd_Argv(1);
	if (!strstr (map, "."))
	{
		Com_sprintf (expanded, sizeof(expanded), "maps/%s", map);
		if (FS_LoadFile (expanded, NULL) == -1)
		{
			Com_sprintf (expanded, sizeof(expanded), "maps/%s.bsp", map);
			if (FS_LoadFile (expanded, NULL) == -1)
			{
				Com_Printf ("Cannot find %s\n", expanded);
				return;
			}
		}
	}

	// start up the next map(and initialize a server)
	SV_Map (false, Cmd_Argv(1), true );

	// archive server state
	strncpy (svs.mapcmd, Cmd_Argv(1), sizeof(svs.mapcmd)-1);
}
//===============================================================

/*
==================
SV_Kick_f

Kick a user off of the server
==================
*/
void SV_Kick_f (void)
{
	if (!svs.initialized)
	{
		Com_Printf ("No server running.\n");
		return;
	}

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: kick <userid>\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	SV_BroadcastPrintf (PRINT_HIGH, "%s was kicked\n", sv_client->name);
	// print directly, because the dropped client won't get the
	// SV_BroadcastPrintf message
	SV_ClientPrintf (sv_client, PRINT_HIGH, "You were kicked from the game\n");
	SV_DropClient (sv_client);
	sv_client->lastmessage = svs.realtime;	// min case there is a funny zombie
}


/*
================
SV_Status_f
================
*/
void SV_Status_f (void)
{
	int		i, j, k , l;
	client_t	*cl;
	char		*s;
	int		ping;
	qboolean	expectColor;

	if (!svs.clients)
	{
		Com_Printf ("No server running.\n");
		return;
	}
	Com_Printf ("map              : %s\n", sv.name);

	Com_Printf ("num score ping name            lastmsg address               qport \n");
	Com_Printf ("--- ----- ---- --------------- ------- --------------------- ------\n");
	for (i=0,cl=svs.clients ; i<maxclients->value; i++,cl++)
	{
		if (!cl->state)
			continue;
		Com_Printf ("%3i ", i);

		if ( cl->edict->client->ps.stats[STAT_SPECTATOR] == 0 )
		{
			Com_Printf ("%5i ", cl->edict->client->ps.stats[STAT_FRAGS]);
		}
		else
		{
			Com_Printf ("%s ", "SPEC");
		}

		if (cl->state == cs_connected)
			Com_Printf ("CNCT ");
		else if (cl->state == cs_zombie)
			Com_Printf ("ZMBI ");
		else
		{
			ping = cl->ping < 9999 ? cl->ping : 9999;
			Com_Printf ("%4i ", ping);
		}

		Com_Printf ("%s", cl->name);
		l = strlen( cl->name );
		expectColor = false;
		for ( j = k = 0 ; j < l ; j ++ ) {
			if ( expectColor ) {
				if ( cl->name[ j ] == '^' ) {
					k ++;
				}
				expectColor = false;
			} else if ( cl->name[ j ] == '^' ) {
				expectColor = true;
			} else {
				k ++;
			}
		}
		if ( expectColor ) {
			k ++;
		}
		l = 16 - k;
		for (j=0 ; j<l ; j++)
			Com_Printf (" ");

		Com_Printf ("%7i ", svs.realtime - cl->lastmessage );

		s = NET_AdrToString ( cl->netchan.remote_address);
		Com_Printf ("%s", s);
		l = 22 - strlen(s);
		for (j=0 ; j<l ; j++)
			Com_Printf (" ");

		Com_Printf ("%5i", cl->netchan.qport);

		Com_Printf ("\n");
	}
	Com_Printf ("\n");
}

/*
==================
SV_ConSay_f
==================
*/
void SV_ConSay_f(void)
{
	client_t *client;
	int		j;
	char	*p;
	char	text[1024];

	if (Cmd_Argc () < 2)
		return;

	strcpy (text, "console: ");
	p = Cmd_Args();

	if (*p == '"')
	{
		p++;
		p[strlen(p)-1] = 0;
	}

	strcat(text, p);

	for (j = 0, client = svs.clients; j < maxclients->value; j++, client++)
	{
		if (client->state != cs_spawned)
			continue;
		SV_ClientPrintf(client, PRINT_CHAT, "%s\n", text);
	}
}


/*
==================
SV_Heartbeat_f
==================
*/
void SV_Heartbeat_f (void)
{
	svs.last_heartbeat = -9999999;
}


/*
===========
SV_Serverinfo_f

  Examine or change the serverinfo string
===========
*/
extern netadr_t *CL_GetRemoteServer (void);
void SV_Serverinfo_f (void)
{
	netadr_t *remoteserver;
	Com_Printf ("Server info settings:\n");
	if (!(svs.clients || dedicated->value)) {
		remoteserver = CL_GetRemoteServer ();
		if (remoteserver) {
			Netchan_OutOfBandPrint (NS_CLIENT, *remoteserver, va("status %i", PROTOCOL_VERSION));
			return;
		}
		Com_Printf ("(No remote server connected, printing local cvars.)\n");
	}
	Info_Print (Cvar_Serverinfo());
}


/*
===========
SV_DumpUser_f

Examine all a users info strings
===========
*/
void SV_DumpUser_f (void)
{
	if (Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: info <userid>\n");
		return;
	}

	if (!SV_SetPlayer ())
		return;

	Com_Printf ("userinfo\n");
	Com_Printf ("--------\n");
	Info_Print (sv_client->userinfo);

}


/*
==============
SV_ServerRecord_f

Begins server demo recording.  Every entity and every message will be
recorded, but no playerinfo will be stored.  Primarily for demo merging.
==============
*/
void SV_ServerRecord_f (void)
{
	char	name[MAX_OSPATH];
	char	buf_data[32768];
	sizebuf_t	buf;
	int		len;
	int		i;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("serverrecord <demoname>\n");
		return;
	}

	if (svs.demofile)
	{
		Com_Printf ("Already recording.\n");
		return;
	}

	if (sv.state != ss_game)
	{
		Com_Printf ("You must be in a level to record.\n");
		return;
	}

	//
	// open the demo file
	//
	Com_sprintf (name, sizeof(name), "%s/demos/%s.dm2", FS_Gamedir(), Cmd_Argv(1));

	Com_Printf ("recording to %s.\n", name);
	FS_CreatePath (name);
	svs.demofile = fopen (name, "wb");
	if (!svs.demofile)
	{
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}

	// setup a buffer to catch all multicasts
	SZ_Init (&svs.demo_multicast, svs.demo_multicast_buf, sizeof(svs.demo_multicast_buf));
	SZ_SetName (&svs.demo_multicast, "Demo multicast buffer", true);

	//
	// write a single giant fake message with all the startup info
	//
	SZ_Init (&buf, (byte *)buf_data, (int)(sizeof(buf_data)));
	SZ_SetName (&buf, "Giant fake msg buffer", true);

	//
	// serverdata needs to go over for all types of servers
	// to make sure the protocol is right, and to set the gamedir
	//
	// send the serverdata
	MSG_WriteByte (&buf, svc_serverdata);
	MSG_WriteLong (&buf, PROTOCOL_VERSION);
	MSG_WriteLong (&buf, svs.spawncount);
	// 2 means server demo
	MSG_WriteByte (&buf, 2);	// demos are always attract loops
	MSG_WriteString (&buf, Cvar_VariableString ("gamedir"));
	MSG_WriteShort (&buf, -1);
	// send full levelname
	MSG_WriteString (&buf, sv.configstrings[CS_NAME]);

	for (i=0 ; i<MAX_CONFIGSTRINGS ; i++)
		if (sv.configstrings[i][0])
		{
			MSG_WriteByte (&buf, svc_configstring);
			MSG_WriteShort (&buf, i);
			MSG_WriteString (&buf, sv.configstrings[i]);
		}

	// write it to the demo file
	Com_DPrintf ("signon message length: %i\n", buf.cursize);
	len = LittleLong (buf.cursize);
	szr = fwrite (&len, 4, 1, svs.demofile);
	szr = fwrite (buf.data, buf.cursize, 1, svs.demofile);

	// the rest of the demo file will be individual frames
}


/*
==============
SV_ServerStop_f

Ends server demo recording
==============
*/
void SV_ServerStop_f (void)
{
	if (!svs.demofile)
	{
		Com_Printf ("Not doing a serverrecord.\n");
		return;
	}
	fclose (svs.demofile);
	svs.demofile = NULL;
	Com_Printf ("Recording completed.\n");
}


/*
===============
SV_KillServer_f

Kick everyone off, possibly in preparation for a new game

===============
*/
void SV_KillServer_f (void)
{
	if (!svs.initialized)
		return;
	SV_Shutdown ("Game Over.\n", false);
	NET_Config ( false );	// close network sockets
}

/*
===============
SV_ServerCommand_f

Let the game dll handle a command
===============
*/
void SV_ServerCommand_f (void)
{
	if (!ge)
	{
		Com_Printf ("No game loaded.\n");
		return;
	}

	ge->ServerCommand();
}

//===========================================================

/*
==================
SV_InitOperatorCommands
==================
*/
void SV_InitOperatorCommands (void)
{
	Cmd_AddCommand ("heartbeat", SV_Heartbeat_f);
	Cmd_AddCommand ("kick", SV_Kick_f);
	Cmd_AddCommand ("status", SV_Status_f);
	Cmd_AddCommand ("serverinfo", SV_Serverinfo_f);
	Cmd_AddCommand ("dumpuser", SV_DumpUser_f);

	Cmd_AddCommand ("map", SV_Map_f);
	Cmd_SetCompleter ("map", SV_MapCommand_Completer, SV_MapCommand_CompletionChecker, 0);
	Cmd_AddCommand ("startmap", SV_StartMap_f);
	Cmd_SetCompleter ("startmap", SV_MapCommand_Completer, SV_MapCommand_CompletionChecker, 0);
	Cmd_AddCommand ("demomap", SV_DemoMap_f);
	Cmd_SetCompleter ("demomap", SV_MapCommand_Completer, SV_MapCommand_CompletionChecker, 0);
	Cmd_AddCommand ("setmaster", SV_SetMaster_f);

	if ( dedicated->value )
		Cmd_AddCommand ("say", SV_ConSay_f);

	Cmd_AddCommand ("serverrecord", SV_ServerRecord_f);
	Cmd_AddCommand ("serverstop", SV_ServerStop_f);

	Cmd_AddCommand ("killserver", SV_KillServer_f);

	Cmd_AddCommand ("sv", SV_ServerCommand_f);
}

