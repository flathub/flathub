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

server_static_t	svs;				// persistant server info
server_t		sv;					// local server

extern cvar_t	*public_server;
cvar_t		*sv_excessive;
cvar_t		*sv_playerspeed;
cvar_t		*sv_tactical;
extern float pm_maxspeed;
extern void SV_SetMaster_f(void);

/*
================
SV_FindIndex

================
*/
int SV_FindIndex (char *name, int start, int max, qboolean create)
{
	int		i;

	if (!name || !name[0])
		return 0;

	for (i=1 ; i<max && sv.configstrings[start+i][0] ; i++)
		if (!strcmp(sv.configstrings[start+i], name))
			return i;

	if (!create)
		return 0;

	if (i == max)
	{
		Com_Printf("*Index: overflow at %i %s\n", i, name);
		return 0;
	}

	strncpy (sv.configstrings[start+i], name, sizeof(sv.configstrings[i]));

	if (sv.state != ss_loading)
	{	// send the update to everyone
		SZ_Clear (&sv.multicast);
		MSG_WriteChar (&sv.multicast, svc_configstring);
		MSG_WriteShort (&sv.multicast, start+i);
		MSG_WriteString (&sv.multicast, name);
		SV_Multicast (vec3_origin, MULTICAST_ALL_R);
	}

	return i;
}


int SV_ModelIndex (char *name)
{
	return SV_FindIndex (name, CS_MODELS, MAX_MODELS, true);
}

int SV_SoundIndex (char *name)
{
	return SV_FindIndex (name, CS_SOUNDS, MAX_SOUNDS, true);
}

int SV_ImageIndex (char *name)
{
	return SV_FindIndex (name, CS_IMAGES, MAX_IMAGES, true);
}


/*
=================
PF_Check*Index

For checking if assets are already loaded - return 0 if they are not
=================
*/

int SV_CheckModelIndex (char *name)
{
	return SV_FindIndex (name, CS_MODELS, MAX_MODELS, false);
}

int SV_CheckSoundIndex (char *name)
{
	return SV_FindIndex (name, CS_SOUNDS, MAX_SOUNDS, false);
}

int SV_CheckImageIndex (char *name)
{
	return SV_FindIndex (name, CS_IMAGES, MAX_IMAGES, false);
}


/*
================
SV_CreateBaseline

Entity baselines are used to compress the update messages
to the clients -- only the fields that differ from the
baseline will be transmitted
================
*/
void SV_CreateBaseline (void)
{
	edict_t			*svent;
	int				entnum;

	for (entnum = 1; entnum < ge->num_edicts ; entnum++)
	{
		svent = EDICT_NUM(entnum);
		if (!svent->inuse)
			continue;
		if (!svent->s.modelindex && !svent->s.sound && !svent->s.effects)
			continue;
		svent->s.number = entnum;

		//
		// take current state as baseline
		//
		VectorCopy (svent->s.origin, svent->s.old_origin);
		sv.baselines[entnum] = svent->s;
	}
}

/*
================
SV_SpawnServer

Change the server to a new map, taking all connected
clients along with it.

================
*/
void SV_SpawnServer (char *server, char *spawnpoint, server_state_t serverstate, qboolean attractloop, qboolean loadgame)
{
	int			i;
	unsigned	checksum;

	if (attractloop)
		Cvar_Set ("paused", "0");

	Com_Printf ("------- Server Initialization -------\n");

	Com_DPrintf ("SpawnServer: %s\n",server);
	if (sv.demofile)
	{
	    FS_FreeFile (sv.demobuf);
	    sv.demosize = sv.demo_ofs = 0;
		fclose (sv.demofile);
	}

	svs.spawncount++;		// any partially connected client will be
							// restarted
	sv.state = ss_dead;
	Com_SetServerState (sv.state);

	// wipe the entire per-level structure
	memset (&sv, 0, sizeof(sv));
	svs.realtime = 0;
	sv.attractloop = attractloop;

	// save name for levels that don't set message
	strcpy (sv.configstrings[CS_NAME], server);
	if ((Cvar_VariableValue ("deathmatch")) || (Cvar_VariableValue ("ctf")))
	{
		sprintf(sv.configstrings[CS_AIRACCEL], "%g", sv_airaccelerate->value);
		pm_airaccelerate = sv_airaccelerate->value;
	}
	else
	{
		strcpy(sv.configstrings[CS_AIRACCEL], "0");
		pm_airaccelerate = 0;
	}

	SZ_Init (&sv.multicast, sv.multicast_buf, sizeof(sv.multicast_buf));
	SZ_SetName (&sv.multicast, "Server multicast buffer", true);

	strcpy (sv.name, server);

	// leave slots at start for clients only
	for (i=0 ; i<maxclients->value ; i++)
	{
		// needs to reconnect
		if (svs.clients[i].state > cs_connected)
			svs.clients[i].state = cs_connected;
		svs.clients[i].lastframe = -1;
	}

	sv.time = 1000;

	strcpy (sv.name, server);
	strcpy (sv.configstrings[CS_NAME], server);

	if (serverstate != ss_game)
	{
		sv.models[1] = CM_LoadMap ("", false, &checksum);	// no real map
	}
	else
	{
		Com_sprintf (sv.configstrings[CS_MODELS+1],sizeof(sv.configstrings[CS_MODELS+1]),
			"maps/%s", server);
		sv.models[1] = CM_LoadMap (sv.configstrings[CS_MODELS+1], false, &checksum);
		Com_sprintf (sv.configstrings[CS_MODELS+1],sizeof(sv.configstrings[CS_MODELS+1]),
			"%s", map_name);
	}
	Com_sprintf (sv.configstrings[CS_MAPCHECKSUM],sizeof(sv.configstrings[CS_MAPCHECKSUM]),
		"%i", checksum);

	//
	// clear physics interaction links
	//
	SV_ClearWorld ();

	for (i=1 ; i< CM_NumInlineModels() ; i++)
	{
		Com_sprintf (sv.configstrings[CS_MODELS+1+i], sizeof(sv.configstrings[CS_MODELS+1+i]),
			"*%i", i);
		sv.models[i+1] = CM_InlineModel (sv.configstrings[CS_MODELS+1+i]);
	}

	//
	// spawn the rest of the entities on the map
	//

	// precache and static commands can be issued during
	// map initialization
	sv.state = ss_loading;
	Com_SetServerState (sv.state);

	// load and spawn all other entities
	ge->SpawnEntities ( sv.name, CM_EntityString(), spawnpoint );

	// run two frames to allow everything to settle
	ge->RunFrame ();
	ge->RunFrame ();

	// all precaches are complete
	sv.state = serverstate;
	Com_SetServerState (sv.state);

	// create a baseline for more efficient communications
	SV_CreateBaseline ();

	// set serverinfo variable
	Cvar_FullSet ("mapname", sv.name, CVAR_SERVERINFO | CVAR_NOSET);

	//get number of bots for bot kicking threshold cases
	sv_numbots = ge->ACESP_FindBotNum();

	Com_Printf ("-------------------------------------\n");
}

/*
==============
SV_InitGame

A brand new game has been started
==============
*/
void SV_InitGame (void)
{
	int		i;
	edict_t	*ent;

	if (svs.initialized)
	{
		// cause any connected clients to reconnect
		SV_Shutdown ("Server restarted\n", true);
	}
	else
	{
		// make sure the client is down
		CL_Drop ();
		SCR_BeginLoadingPlaque ();
	}

	// get any latched variable changes (maxclients, etc)
	Cvar_GetLatchedVars ();

	svs.initialized = true;

	// dedicated servers are always deathmatch
	if (dedicated->value)
	{
		Cvar_FullSet ("deathmatch", "1",  CVAR_SERVERINFO | CVAR_LATCH);
	}

	//set player move speed according to excessive value
	sv_excessive = Cvar_Get ("excessive", "0", CVAR_LATCH|CVAR_GAMEINFO);
	sv_playerspeed = Cvar_Get ("playerspeed", "0", CVAR_LATCH|CVAR_GAMEINFO);
	sv_tactical = Cvar_Get ("g_tactical", "0", CVAR_LATCH|CVAR_GAMEINFO);
	if(sv_excessive->value || sv_playerspeed->value)
		pm_maxspeed = 450;
	else if(sv_tactical->value)
		pm_maxspeed = 200;
	else
		pm_maxspeed = 300;

	// init clients
	if (Cvar_VariableValue ("deathmatch") || Cvar_VariableValue ("ctf"))
	{
		if (maxclients->value <= 0)
			Cvar_FullSet ("maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH);
		else if (maxclients->value > MAX_CLIENTS)
			Cvar_FullSet ("maxclients", va("%i", MAX_CLIENTS), CVAR_SERVERINFO | CVAR_LATCH);
	}

	svs.spawncount = rand();
	svs.clients = Z_Malloc (sizeof(client_t)*maxclients->value);
	svs.num_client_entities = maxclients->value*UPDATE_BACKUP*64;
	svs.client_entities = Z_Malloc (sizeof(entity_state_t)*svs.num_client_entities);

	// init network stuff
	NET_Config (maxclients->value > 1 || dedicated->integer);

	// heartbeats will always be sent to the COR master
	svs.last_heartbeat = -999999;		// send immediately

	// init game
	SV_InitGameProgs ();
	for (i=0 ; i<maxclients->value ; i++)
	{
		ent = EDICT_NUM(i+1);
		ent->s.number = i+1;
		svs.clients[i].edict = ent;
		memset (&svs.clients[i].lastcmd, 0, sizeof(svs.clients[i].lastcmd));
	}
	if (!dedicated->value && Cvar_VariableValue ("deathmatch") && public_server->value) {
		SV_SetMaster_f(); //for listen servers
	}
}


/*
======================
SV_Map

  the full syntax is:

  map [*]<map>$<startspot>+<nextserver>

command from the console or progs.
Map can also be a.cin, .pcx, or .dm2 file
Nextserver is used to allow a cinematic to play, then proceed to
another level:

	map tram.cin+jail_e3
======================
*/
void SV_Map (qboolean attractloop, char *levelstring, qboolean loadgame)
{
	char	level[MAX_QPATH];
	char	*ch;
	int		l;
	char	spawnpoint[MAX_QPATH];

	sv.attractloop = attractloop;

	if ( loadgame || !svs.initialized )
	{
		SV_InitGame(); // the game is just starting
	}

	strcpy (level, levelstring);

	// if there is a + in the map, set nextserver to the remainder
	ch = strstr(level, "+");
	if (ch)
	{
		*ch = 0;
			Cvar_Set ("nextserver", va("map \"%s\"", ch+1));
	}
	else
		Cvar_Set ("nextserver", "");

	// if there is a $, use the remainder as a spawnpoint
	ch = strstr(level, "$");
	if (ch)
	{
		*ch = 0;
		strcpy (spawnpoint, ch+1);
	}
	else
		spawnpoint[0] = 0;

	// skip the end-of-unit flag if necessary
	if (level[0] == '*')
		strcpy (level, level+1);

	l = strlen( level );
	if ( l > 4 && !strcmp( level + l - 4, ".dm2" ) )
	{ // --- Run client-side demo ---
		if ( !dedicated->value )
		{ //this prevents the client from restarting
			SCR_BeginLoadingPlaque(); // for local system
		}
		SV_BroadcastCommand( "changing\n" );
		// make sure attract loop is set. loadgame may be unused.
		// disable warmup sequence! otherwise, countdown announcements mess up
		//   configstrings
		attractloop = true;
		loadgame    = false;
		Cvar_ForceSet( "warmuptime", "0" );
		SV_SpawnServer( level, spawnpoint, ss_demo, attractloop, loadgame );
	}
	else
	{
		if ( !dedicated->value ) //this prevents the client from restarting
			SCR_BeginLoadingPlaque(); // for local system
		SV_BroadcastCommand( "changing\n" );
		SV_SendClientMessages();
		SV_SpawnServer( level, spawnpoint, ss_game, attractloop, loadgame );
		Cbuf_CopyToDefer();
	}

	SV_BroadcastCommand ("reconnect\n");
}
