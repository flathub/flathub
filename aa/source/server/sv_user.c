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
// sv_user.c -- server code for moving users

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "server.h"

edict_t	*sv_player;

/*
============================================================

USER STRINGCMD EXECUTION

sv_client and sv_player will be valid.
============================================================
*/

/*
==================
SV_BeginDemoServer
==================
*/
void SV_BeginDemoserver (void)
{
	char		name[MAX_OSPATH];

	Com_sprintf (name, sizeof(name), "demos/%s", sv.name);
	FS_FOpenFile (name, &sv.demofile);
	if (!sv.demofile)
		Com_Error (ERR_DROP, "Couldn't open %s\n", name);
	sv.demo_ofs = 0;
	sv.demosize = FS_LoadFile (name, (void **)&sv.demobuf);
}

/*
================
SV_New_f

Sends the first message from the server to a connected client.
This will be sent on the initial connection and upon each server load.
================
*/
void SV_New_f (void)
{
	char		*gamedir;
	int			playernum;
	edict_t		*ent;

	Com_DPrintf ("New() from %s\n", sv_client->name);

	if (sv_client->state != cs_connected)
	{
		Com_Printf ("New not valid -- already spawned\n");
		return;
	}

	// demo servers just dump the file message
	if (sv.state == ss_demo)
	{
		SV_BeginDemoserver ();
		return;
	}

	//
	// serverdata needs to go over for all types of servers
	// to make sure the protocol is right, and to set the gamedir
	//
	gamedir = Cvar_VariableString ("gamedir");

	// send the serverdata
	MSG_WriteByte (&sv_client->netchan.message, svc_serverdata);
	MSG_WriteLong (&sv_client->netchan.message, PROTOCOL_VERSION);
	MSG_WriteLong (&sv_client->netchan.message, svs.spawncount);
	MSG_WriteByte (&sv_client->netchan.message, sv.attractloop);
	MSG_WriteString (&sv_client->netchan.message, gamedir);

	if (sv.state == ss_cinematic || sv.state == ss_pic)
		playernum = -1;
	else
		playernum = sv_client - svs.clients;
	MSG_WriteShort (&sv_client->netchan.message, playernum);

	// send full levelname
	MSG_WriteString (&sv_client->netchan.message, sv.configstrings[CS_NAME]);

	//
	// game server
	//
	if (sv.state == ss_game)
	{
		// set up the entity for the client
		ent = EDICT_NUM(playernum+1);
		ent->s.number = playernum+1;
		sv_client->edict = ent;
		memset (&sv_client->lastcmd, 0, sizeof(sv_client->lastcmd));

		// begin fetching configstrings
		MSG_WriteByte (&sv_client->netchan.message, svc_stufftext);
		MSG_WriteString (&sv_client->netchan.message, va("cmd configstrings %i 0\n",svs.spawncount) );
	}

}

/*
==================
SV_Configstrings_f
==================
*/
void SV_Configstrings_f (void)
{
	int			start;

	Com_DPrintf ("Configstrings() from %s\n", sv_client->name);

	if (sv_client->state != cs_connected)
	{
		Com_Printf ("configstrings not valid -- already spawned\n");
		return;
	}

	// handle the case of a level changing while a client was connecting
	if ( atoi(Cmd_Argv(1)) != svs.spawncount )
	{
		Com_Printf ("SV_Configstrings_f from different level\n");
		SV_New_f ();
		return;
	}

	start = atoi(Cmd_Argv(2));
	if( start < 0 ) {
		start = 0;	// sku - catch negative offsets
	}

	// write a packet full of data

	while ( sv_client->netchan.message.cursize < MAX_MSGLEN/4
		&& start < MAX_CONFIGSTRINGS)
	{
		if (sv.configstrings[start][0])
		{
			int length;

			// sku - write configstrings that exceed MAX_QPATH in proper-sized chunks
			length = strlen( sv.configstrings[start] );
			if( length > MAX_QPATH ) {
				length = MAX_QPATH;
			}

			MSG_WriteByte (&sv_client->netchan.message, svc_configstring);
			MSG_WriteShort (&sv_client->netchan.message, start);
			SZ_Write (&sv_client->netchan.message, sv.configstrings[start], length);
			MSG_WriteByte (&sv_client->netchan.message, 0);
		}
		start++;
	}

	// send next command

	if (start == MAX_CONFIGSTRINGS)
	{
		MSG_WriteByte (&sv_client->netchan.message, svc_stufftext);
		MSG_WriteString (&sv_client->netchan.message, va("cmd baselines %i 0\n",svs.spawncount) );
	}
	else
	{
		MSG_WriteByte (&sv_client->netchan.message, svc_stufftext);
		MSG_WriteString (&sv_client->netchan.message, va("cmd configstrings %i %i\n",svs.spawncount, start) );
	}
}

/*
==================
SV_Baselines_f
==================
*/
void SV_Baselines_f (void)
{
	int		start;
	entity_state_t	nullstate;
	entity_state_t	*base;

	Com_DPrintf ("Baselines() from %s\n", sv_client->name);

	if (sv_client->state != cs_connected)
	{
		Com_Printf ("baselines not valid -- already spawned\n");
		return;
	}

	// handle the case of a level changing while a client was connecting
	if ( atoi(Cmd_Argv(1)) != svs.spawncount )
	{
		Com_Printf ("SV_Baselines_f from different level\n");
		SV_New_f ();
		return;
	}

	start = atoi(Cmd_Argv(2));
	if( start < 0 ) {
		start = 0;
	}

	memset (&nullstate, 0, sizeof(nullstate));

	// write a packet full of data

	while ( sv_client->netchan.message.cursize <  MAX_MSGLEN/4
		&& start < MAX_EDICTS)
	{
		base = &sv.baselines[start];
		if (base->modelindex || base->sound || base->effects)
		{
			MSG_WriteByte (&sv_client->netchan.message, svc_spawnbaseline);
			MSG_WriteDeltaEntity (&nullstate, base, &sv_client->netchan.message, true, true);
		}
		start++;
	}

	// send next command

	if (start == MAX_EDICTS)
	{
		MSG_WriteByte (&sv_client->netchan.message, svc_stufftext);
		MSG_WriteString (&sv_client->netchan.message, va("precache %i\n", svs.spawncount) );
	}
	else
	{
		MSG_WriteByte (&sv_client->netchan.message, svc_stufftext);
		MSG_WriteString (&sv_client->netchan.message, va("cmd baselines %i %i\n",svs.spawncount, start) );
	}
}

/*
==================
SV_Begin_f
==================
*/
void SV_Begin_f (void)
{
	Com_DPrintf ("Begin() from %s\n", sv_client->name);

	// handle the case of a level changing while a client was connecting
	if ( atoi(Cmd_Argv(1)) != svs.spawncount )
	{
		Com_Printf ("SV_Begin_f from different level\n");
		SV_New_f ();
		return;
	}

	sv_client->state = cs_spawned;

	// call the game begin function
	ge->ClientBegin (sv_player);

	Cbuf_InsertFromDefer ();
}

//=============================================================================

/*
==================
SV_NextDownload_f
==================
*/
void SV_NextDownload_f (void)
{
	int		r;
	int		percent;
	int		size;

	if (!sv_client->download)
		return;

	r = sv_client->downloadsize - sv_client->downloadcount;

	if (r > 1280) //was 1024 should speed up
			r = 1280;
	MSG_WriteByte (&sv_client->netchan.message, svc_download);
	MSG_WriteShort (&sv_client->netchan.message, r);

	sv_client->downloadcount += r;
	size = sv_client->downloadsize;
	if (!size)
		size = 1;
	percent = sv_client->downloadcount*100/size;
	MSG_WriteByte (&sv_client->netchan.message, percent);
	SZ_Write (&sv_client->netchan.message,
		sv_client->download + sv_client->downloadcount - r, r);

	if (sv_client->downloadcount != sv_client->downloadsize)
		return;

	FS_FreeFile (sv_client->download);
	sv_client->download = NULL;
}

/*
==================
SV_BeginDownload_f
==================
*/
void SV_BeginDownload_f(void)
{
	char	*name;
	extern	cvar_t *allow_download;
	extern	cvar_t *allow_download_players;
	extern	cvar_t *allow_download_models;
	extern	cvar_t *allow_download_sounds;
	extern	cvar_t *allow_download_maps;
	int		name_length; // For getting the final character.
	int offset = 0;

	name = Cmd_Argv(1);

	if (Cmd_Argc() > 2)
		offset = atoi(Cmd_Argv(2)); // downloaded offset

	// hacked by zoid to allow more conrol over download
	// first off, no .. or global allow check
	if (strstr (name, "..") || !allow_download->value
		// prevent config downloading on Win32 systems
		|| name[0] == '\\'
		// negative offset causes crashing
		|| offset < 0
		// leading dot is no good
		|| *name == '.'
		// leading slash bad as well, must be in subdir
		|| *name == '/'
		// next up, skin check
		|| (strncmp(name, "players/", 8) == 0 && !allow_download_players->value)
		// now models
		|| (strncmp(name, "models/", 7) == 0 && !allow_download_models->value)
		// now sounds
		|| (strncmp(name, "sound/", 6) == 0 && !allow_download_sounds->value)
		// now maps (note special case for maps, must not be in pak)
		|| (strncmp(name, "maps/", 5) == 0 && !allow_download_maps->value)
		// MUST be in a subdirectory
		|| !strstr (name, "/") )
	{	// don't allow anything with .. path
		MSG_WriteByte (&sv_client->netchan.message, svc_download);
		MSG_WriteShort (&sv_client->netchan.message, -1);
		MSG_WriteByte (&sv_client->netchan.message, 0);
		return;
	}

	// If the name ends in a slash or dot, hack it off. Continue to do so just
    // in case some tricky fellow puts multiple slashes or dots.
    while (name[(name_length = strlen(name))] == '.' || name[name_length] == '/' )
        name[name_length] = '\0';

	if (sv_client->download)
		FS_FreeFile (sv_client->download);

	sv_client->downloadsize = FS_LoadFile (name, (void **)&sv_client->download);
	sv_client->downloadcount = offset;

	if (offset > sv_client->downloadsize)
		sv_client->downloadcount = sv_client->downloadsize;

	if ( !sv_client->download )
	{
		Com_DPrintf ("Could not download %s to %s\n", name, sv_client->name);
		if (sv_client->download) {
			FS_FreeFile (sv_client->download);
			sv_client->download = NULL;
		}

		MSG_WriteByte (&sv_client->netchan.message, svc_download);
		MSG_WriteShort (&sv_client->netchan.message, -1);
		MSG_WriteByte (&sv_client->netchan.message, 0);
		return;
	}

	SV_NextDownload_f ();
	Com_DPrintf ("Downloading %s to %s\n", name, sv_client->name);
}



//============================================================================


/*
=================
SV_Disconnect_f

The client is going to disconnect, so remove the connection immediately
=================
*/
void SV_Disconnect_f (void)
{
//	SV_EndRedirect ();
	SV_DropClient (sv_client);
}


/*
==================
SV_ShowServerinfo_f

Dumps the serverinfo info string
==================
*/
void SV_ShowServerinfo_f (void)
{
	//Info_Print (Cvar_Serverinfo());
}


void SV_Nextserver (void)
{
	char	*v;

	//ZOID, ss_pic can be nextserver'd in coop mode
	if (sv.state == ss_game || sv.state == ss_pic)
		return;		// can't nextserver while playing a normal game

	svs.spawncount++;	// make sure another doesn't sneak in
	v = Cvar_VariableString ("nextserver");
	if (!v[0])
		Cbuf_AddText ("killserver\n");
	else
	{
		Cbuf_AddText (v);
		Cbuf_AddText ("\n");
	}
	Cvar_Set ("nextserver","");
}

/*
==================
SV_Nextserver_f

A cinematic has completed or been aborted by a client, so move
to the next server,
==================
*/
void SV_Nextserver_f (void)
{
	if ( atoi(Cmd_Argv(1)) != svs.spawncount ) {
		Com_DPrintf ("Nextserver() from wrong level, from %s\n", sv_client->name);
		return;		// leftover from last server
	}

	Com_DPrintf ("Nextserver() from %s\n", sv_client->name);

	SV_Nextserver ();
}

typedef struct
{
	char	*name;
	void	(*func) (void);
} ucmd_t;

ucmd_t ucmds[] =
{
	// auto issued
	{"new", SV_New_f},
	{"configstrings", SV_Configstrings_f},
	{"baselines", SV_Baselines_f},
	{"begin", SV_Begin_f},

	{"nextserver", SV_Nextserver_f},

	{"disconnect", SV_Disconnect_f},

	// issued by hand at client consoles
	{"info", SV_ShowServerinfo_f},

	{"download", SV_BeginDownload_f},
	{"nextdl", SV_NextDownload_f},

	{NULL, NULL}
};

/*
==================
SV_ExecuteUserCommand
==================
*/
void SV_ExecuteUserCommand (char *s)
{
	ucmd_t	*u;

	Cmd_TokenizeString (s, false);
	sv_player = sv_client->edict;

//	SV_BeginRedirect (RD_CLIENT);

	for (u=ucmds ; u->name ; u++)
		if (!strcmp (Cmd_Argv(0), u->name) )
		{
			u->func ();
			break;
		}

	if (!u->name && sv.state == ss_game)
		ge->ClientCommand (sv_player);

//	SV_EndRedirect ();
}

/*
===========================================================================

USER CMD EXECUTION

===========================================================================
*/



void SV_ClientThink (client_t *cl, usercmd_t *cmd)

{
	cl->commandMsec -= cmd->msec;

	ge->ClientThink (cl->edict, cmd);
}



#define	MAX_STRINGCMDS	8
/*
===================
SV_ExecuteClientMessage

The current net_message is parsed for the given client
===================
*/
int sys_msec_as_of_packet_read = 0;
int sys_lasthang = 0;


static char *timestamp( void )
{
	static char utc_string[256];
	struct tm *utc;
	time_t now;

	now = time( NULL );
	utc = gmtime( &now );
	sprintf( utc_string, "%s", asctime( utc ) );
	utc_string[ strlen(utc_string)-1] =  '\0'; // remove \n

	return utc_string;
}

static char *ipaddr( client_t *cl )
{
	return NET_AdrToString( cl->netchan.remote_address );
}


void SV_ExecuteClientMessage (client_t *cl)
{
	int		c;
	char	*s;

	usercmd_t	nullcmd;
	usercmd_t	oldest, oldcmd, newcmd;
	int		net_drop;
	int		stringCmdCount;
	int		checksum, calculatedChecksum;
	int		checksumIndex;
	qboolean	move_issued;
	int		lastframe;
	float	timeratio;

	sv_client = cl;
	sv_player = sv_client->edict;

	// only allow one move command
	move_issued = false;
	stringCmdCount = 0;

	while (1)
	{
		if (net_message.readcount > net_message.cursize)
		{
			Com_Printf( "PACKET: readcount > cursize from %s[%s] (UTC: %s)\n",
					cl->name, ipaddr(cl), timestamp() );
			//formerly was drop client and return
			break;
		}

		c = MSG_ReadByte (&net_message);
		if (c == -1)
			break;

		switch (c)
		{

		default:
			Com_Printf ("PACKET: invalid command byte from %s[%s] (UTC: %s)\n",
					cl->name, ipaddr(cl), timestamp() );
			//formerly was drop client and return
			break;

		case clc_nop:
			break;

		case clc_userinfo:
			Q_strncpyz2( cl->userinfo, MSG_ReadString (&net_message), sizeof(cl->userinfo) );
			SV_UserinfoChanged (cl);
			break;

		case clc_move:
			checksumIndex = net_message.readcount;
			checksum      = MSG_ReadByte (&net_message);
			lastframe     = MSG_ReadLong (&net_message);
			if ( lastframe != cl->lastframe )
			{
				cl->lastframe = lastframe;
				if ( cl->lastframe > 0 )
				{
					cl->frame_latency[cl->lastframe & (LATENCY_COUNTS - 1)]
						= svs.realtime - cl->frames[cl->lastframe & UPDATE_MASK].senttime;
				}
			}

			memset (&nullcmd, 0, sizeof(nullcmd));
			MSG_ReadDeltaUsercmd (&net_message, &nullcmd, &oldest);
			MSG_ReadDeltaUsercmd (&net_message, &oldest, &oldcmd);
			MSG_ReadDeltaUsercmd (&net_message, &oldcmd, &newcmd);

			// checks the lastframe and deltauser parts of command
			calculatedChecksum = COM_BlockSequenceCRCByte(
					net_message.data + checksumIndex + 1,      // base
					net_message.readcount - checksumIndex - 1, // length
					cl->netchan.incoming_sequence );           // sequence
			if ( calculatedChecksum != checksum )
			{
				Com_Printf(
					"PACKET: Failed command checksum for %s[%s] (%d != %d)/%d (UTC: %s)\n",
					cl->name, ipaddr(cl), calculatedChecksum, checksum,
					cl->netchan.incoming_sequence, timestamp() );
				break;
			}

			if (cl->state == cs_spawned)
			{
				if ( move_issued )
				{ //only one clc_move per message
					Com_Printf(
						"EXPLOIT: multiple clc_move from %s[%s] (UTC: %s)\n",
						cl->name, ipaddr(cl), timestamp() );
					SV_KickClient(cl, "illegal multiple clc_move commands", NULL );
					return;
				}
				move_issued = true;

				//client clamps the msec byte to 250. should never issue msec
				//byte of zero.
				if (newcmd.msec > 250)
				{
					Com_Printf(
						"EXPLOIT: illegal msec value from %s[%s], %d (UTC: %s)\n",
						cl->name, ipaddr(cl), newcmd.msec, timestamp() );
					SV_KickClient (cl, "illegal pmove msec detected", NULL);
					return;
				}
				else if (newcmd.msec == 0)
				{
					Com_Printf(
						"EXPLOIT: 0 msec move from %s[%s] (UTC: %s)\n",
						cl->name, ipaddr(cl), timestamp() );
					SV_KickClient (cl, "zero pmove msec detected", NULL);
					return;
				}

				if (sv_enforcetime->integer)
				{
					//Speed cheat detection-- speed cheats work by increasing
					//the amount of time a client says to simulate. This code
					//does a sanity check; if over any period of 12 seconds,
					//the client claims significantly more time than actually
					//elapsed, it is probably cheating.
					cl->claimedmsec += newcmd.msec;

					if (cl->lasthang > sys_lasthang) {
						//This can happen with an integer overflow
						cl->lasthang = sys_lasthang;
					}

					if (sys_msec_as_of_packet_read < cl->lastresettime) {
						//This can happen with either a map change or an
						//integer overflow after around 25 days on the same
						//map. In this case, we just throw out all the data.
						cl->lastresettime = sys_msec_as_of_packet_read;
						cl->claimedmsec = 0;
					} else if (sys_lasthang > cl->lasthang) {
						//The server has hung on the boundary of the 12-
						//second interval. The data is invalid.
						cl->lastresettime = sys_msec_as_of_packet_read;
						cl->claimedmsec = 0;
						cl->lasthang = sys_lasthang;
					} else if (sys_msec_as_of_packet_read - cl->lastresettime >= 12000) {
						//This ratio should almost never be more than 1. If
						//it's more than 1.05, someone's probably trying to
						//cheat.
						timeratio = (float)cl->claimedmsec/(float)(sys_msec_as_of_packet_read - cl->lastresettime);

						if (timeratio > 1.05) {
							Com_Printf (
								"EXPLOIT: Client %s[%s] is trying to go approximately %4.2f times faster than normal! (UTC: %s)\n",
								cl->name, ipaddr( cl ), timeratio, timestamp() );
							SV_KickClient (cl, va ("illegal pmove msec scaling by %4.2f detected", timeratio), NULL);
							return;
						}

						cl->lastresettime = sys_msec_as_of_packet_read;
						cl->claimedmsec = 0;
					}
				}

			}

			if ( cl->state != cs_spawned )
			{
				cl->lastframe = -1;
				break;
			}

			if (!sv_paused->value)
			{
				net_drop = cl->netchan.dropped;
				if ( net_drop > 0 )
				{
					/* when packets are dropped, or out of sequence
					 *  execute backup commands. if drop is more than
					 *  2, there is a problem; formerly, was executing
					 *  the last command multiple times, but that does
					 *  not seem to make sense.
					 */
					if ( net_drop > 2 )
					{
						Com_Printf(
							"PACKET: net_drop is %i for %s[%s] (UTC: %s)\n",
							net_drop, cl->name, ipaddr( cl ), timestamp() );
					}
					if (net_drop > 1)
						SV_ClientThink (cl, &oldest);
					SV_ClientThink (cl, &oldcmd);
				}
				SV_ClientThink (cl, &newcmd);
			}
			cl->lastcmd = newcmd;
			break;

		case clc_stringcmd:

			s = MSG_ReadString (&net_message);

			// malicious users may try using too many string commands
			if (++stringCmdCount < MAX_STRINGCMDS)
				SV_ExecuteUserCommand (s);
			else
			{
				Com_Printf(
					"EXPLOIT: too many clc_stringcmd from %s[%s] (UTC: %s)\n",
					cl->name, ipaddr( cl ), timestamp() );
			}

			if (cl->state == cs_zombie)
				return;	// disconnect command
			break;
		}
	}
}

