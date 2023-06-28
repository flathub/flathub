/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2011 COR Entertainment, LLC.

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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/
// cl_main.c  -- client main loop

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "client.h"
#if defined WIN32_VARIANT
#include <winsock.h>
#endif

#if defined UNIX_VARIANT
#if defined HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <errno.h>
#endif

#if defined HAVE_PUTENV && !defined HAVE__PUTENV
#define _putenv putenv
#endif


cvar_t	*freelook;

cvar_t	*adr0;
cvar_t	*adr1;
cvar_t	*adr2;
cvar_t	*adr3;
cvar_t	*adr4;
cvar_t	*adr5;
cvar_t	*adr6;
cvar_t	*adr7;
cvar_t	*adr8;

cvar_t	*cl_stereo_separation;
cvar_t	*cl_stereo;

cvar_t	*rcon_client_password;
cvar_t	*rcon_address;

cvar_t	*cl_noskins;
cvar_t	*cl_autoskins;
cvar_t	*cl_footsteps;
cvar_t	*cl_timeout;
cvar_t	*cl_predict;
cvar_t	*cl_maxfps;
cvar_t	*cl_gun;
cvar_t  *cl_brass;
cvar_t  *cl_showPlayerNames;
cvar_t	*cl_playtaunts;
cvar_t	*cl_centerprint;
cvar_t	*cl_precachecustom;
cvar_t	*cl_simpleitems;
cvar_t	*cl_flicker;

cvar_t	*cl_paindist;
cvar_t	*cl_explosiondist;
cvar_t	*cl_raindist;

cvar_t	*cl_add_particles;
cvar_t	*cl_add_lights;
cvar_t	*cl_add_entities;
cvar_t	*cl_add_blend;

cvar_t	*cl_shownet;
cvar_t	*cl_showmiss;
cvar_t	*cl_showclamp;

cvar_t	*cl_paused;
cvar_t	*cl_timedemo;
cvar_t	*cl_demoquit;

cvar_t	*lookspring;
cvar_t	*lookstrafe;
cvar_t	*sensitivity;
cvar_t	*menu_sensitivity;

cvar_t	*m_pitch;
cvar_t	*m_yaw;
cvar_t	*m_forward;
cvar_t	*m_side;

cvar_t	*cl_test;

//
// userinfo
//
cvar_t	*info_password;
cvar_t	*info_spectator;
cvar_t  *info_spectator_password;
cvar_t	*name;
cvar_t  *stats_password;
cvar_t	*pw_hashed;
cvar_t	*skin;
cvar_t	*rate;
cvar_t	*fov;
cvar_t	*msg;
cvar_t	*hand;
cvar_t	*gender;
cvar_t	*gender_auto;

cvar_t	*cl_vwep;

cvar_t  *background_music;
cvar_t	*background_music_vol;

cvar_t	*scriptsloaded;

//master server
cvar_t  *cl_master;
cvar_t  *cl_master2;

//custom huds
cvar_t	*cl_hudimage1;
cvar_t	*cl_hudimage2;
cvar_t  *cl_hudimage3;

//blood
cvar_t  *cl_noblood;

//beam color for disruptor
cvar_t  *cl_disbeamclr;

cvar_t  *cl_dmlights;

//Stats
cvar_t  *cl_stats_server;

//latest version of the game available
cvar_t	*cl_latest_game_version_url;

cvar_t	*cl_speedrecord;
cvar_t	*cl_alltimespeedrecord;

client_static_t	cls;
client_state_t	cl;

centity_t		cl_entities[MAX_EDICTS];

entity_state_t	cl_parse_entities[MAX_PARSE_ENTITIES];

extern	cvar_t *allow_download;
extern	cvar_t *allow_download_players;
extern	cvar_t *allow_download_models;
extern	cvar_t *allow_download_sounds;
extern	cvar_t *allow_download_maps;

#if defined WIN32_VARIANT
extern	char map_music[MAX_PATH];
extern  char map_music_sec[MAX_PATH];
#else
extern	char map_music[MAX_OSPATH];
extern  char map_music_sec[MAX_OSPATH];
#endif

extern void RS_FreeAllScripts(void);

typedef struct _PLAYERINFO {
	char playername[PLAYERNAME_SIZE];
	int ping;
	int score;
	char team[8];
} PLAYERINFO;

typedef struct _SERVERINFO {
	char ip[32];
	unsigned short port;
	char szHostName[256];
	char szMapName[256];
	int curClients;
	int maxClients;
	int starttime;
	int	ping;
	PLAYERINFO players[64];
	int temporary;
} SERVERINFO;

SERVERINFO servers[64];
static int numServers = 0;

static size_t szr; // just for unused result warnings

int server_tickrate = 10; //what framerate is the server running(default to 10 for servers that don't have the info(legacy)).

//
// Fonts
//
FNT_auto_t			CL_gameFont;
static struct FNT_auto_s	_CL_gameFont;
FNT_auto_t			CL_centerFont;
static struct FNT_auto_s	_CL_centerFont;
FNT_auto_t			CL_consoleFont;
static struct FNT_auto_s	_CL_consoleFont;
FNT_auto_t			CL_menuFont;
static struct FNT_auto_s	_CL_menuFont;


//======================================================================


/*
====================
CL_WriteDemoMessage

Dumps the current net message, prefixed by the length
====================
*/
void CL_WriteDemoMessage (void)
{
	int		len, swlen;

	// the first eight bytes are just packet sequencing stuff
	len = net_message.cursize-8;
	swlen = LittleLong(len);
	if (!swlen)
		return;

	szr = fwrite (&swlen, 4, 1, cls.demofile);
	szr = fwrite (net_message.data+8,	len, 1, cls.demofile);
}


/*
====================
CL_Stop_f

stop recording a demo
====================
*/
void CL_Stop_f (void)
{
	int		len;

	if (!cls.demorecording)
	{
		Com_Printf ("Not recording a demo.\n");
		return;
	}

// finish up
	len = -1;
	szr = fwrite (&len, 4, 1, cls.demofile);
	fclose (cls.demofile);
	cls.demofile = NULL;
	cls.demorecording = false;
	Com_Printf ("Stopped demo.\n");
}

/*
====================
CL_Record_f

record <demoname>

Begins recording a demo from the current position
====================
*/
void CL_Record_f (void)
{
	char	name[MAX_OSPATH];
	char	buf_data[MAX_MSGLEN];
	sizebuf_t	buf;
	int		i;
	int		len;
	entity_state_t	*ent;
	entity_state_t	nullstate;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("record <demoname>\n");
		return;
	}

	if (cls.demorecording)
	{
		Com_Printf ("Already recording.\n");
		return;
	}

	if (cls.state != ca_active)
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
	cls.demofile = fopen (name, "wb");
	if (!cls.demofile)
	{
		Com_Printf ("ERROR: couldn't open.\n");
		return;
	}
	cls.demorecording = true;

	// don't start saving messages until a non-delta compressed message is received
	cls.demowaiting = true;

	//
	// write out messages to hold the startup information
	//
	SZ_Init (&buf, (byte *)buf_data, sizeof(buf_data));
	SZ_SetName ( &buf, "CL_Record_f", false );

	// send the serverdata
	MSG_WriteByte (&buf, svc_serverdata);
	MSG_WriteLong (&buf, PROTOCOL_VERSION);
	MSG_WriteLong (&buf, 0x10000 + cl.servercount);
	MSG_WriteByte (&buf, 1);	// demos are always attract loops
	MSG_WriteString (&buf, cl.gamedir);
	MSG_WriteShort (&buf, cl.playernum);

	MSG_WriteString (&buf, cl.configstrings[CS_NAME]);

	// configstrings
	for (i=0 ; i<MAX_CONFIGSTRINGS ; i++)
	{
		if (cl.configstrings[i][0])
		{
			if (buf.cursize + strlen (cl.configstrings[i]) + 32 > buf.maxsize)
			{	// write it out
				len = LittleLong (buf.cursize);
				szr = fwrite (&len, 4, 1, cls.demofile);
				szr = fwrite (buf.data, buf.cursize, 1, cls.demofile);
				buf.cursize = 0;
			}

			MSG_WriteByte (&buf, svc_configstring);
			MSG_WriteShort (&buf, i);
			MSG_WriteString (&buf, cl.configstrings[i]);
		}

	}

	// baselines
	memset (&nullstate, 0, sizeof(nullstate));
	for (i=0; i<MAX_EDICTS ; i++)
	{
		ent = &cl_entities[i].baseline;
		if (!ent->modelindex)
			continue;

		if (buf.cursize + 64 > buf.maxsize)
		{	// write it out
			len = LittleLong (buf.cursize);
			szr = fwrite (&len, 4, 1, cls.demofile);
			szr = fwrite (buf.data, buf.cursize, 1, cls.demofile);
			buf.cursize = 0;
		}

		MSG_WriteByte (&buf, svc_spawnbaseline);
		MSG_WriteDeltaEntity (&nullstate, &cl_entities[i].baseline, &buf, true, true);
	}

	MSG_WriteByte (&buf, svc_stufftext);
	MSG_WriteString (&buf, "precache\n");

	// write it to the demo file

	len = LittleLong (buf.cursize);
	szr = fwrite (&len, 4, 1, cls.demofile);
	szr = fwrite (buf.data, buf.cursize, 1, cls.demofile);

	// the rest of the demo file will be individual frames
}

//======================================================================



/*
===================
Cmd_ForwardToServer

adds the current command line as a clc_stringcmd to the client message.
things like godmode, noclip, etc, are commands directed to the server,
so when they are typed in at the console, they will need to be forwarded.
===================
*/
void Cmd_ForwardToServer (void)
{
	char	*cmd;

	cmd = Cmd_Argv(0);
	if (cls.state <= ca_connected || *cmd == '-' || *cmd == '+')
	{
		Com_Printf ("Unknown command \"%s\"\n", cmd);
		return;
	}

	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	SZ_Print (&cls.netchan.message, cmd);
	if (Cmd_Argc() > 1)
	{
		SZ_Print (&cls.netchan.message, " ");
		SZ_Print (&cls.netchan.message, Cmd_Args());
	}
}

void CL_Setenv_f( void )
{
	int argc = Cmd_Argc();

	if ( argc > 2 )
	{
		char buffer[1000];
		int i;

		strcpy( buffer, Cmd_Argv(1) );
		strcat( buffer, "=" );

		for ( i = 2; i < argc; i++ )
		{
			strcat( buffer, Cmd_Argv( i ) );
			strcat( buffer, " " );
		}

		_putenv( buffer );
	}
	else if ( argc == 2 )
	{
		char *env = getenv( Cmd_Argv(1) );

		if ( env )
		{
			Com_Printf( "%s=%s\n", Cmd_Argv(1), env );
		}
		else
		{
			Com_Printf( "%s undefined\n", Cmd_Argv(1), env );
		}
	}
}


/*
==================
CL_ForwardToServer_f
==================
*/
void CL_ForwardToServer_f (void)
{
	if (cls.state != ca_connected && cls.state != ca_active)
	{
		Com_Printf ("Can't \"%s\", not connected\n", Cmd_Argv(0));
		return;
	}

	// don't forward the first argument
	if (Cmd_Argc() > 1)
	{
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		SZ_Print (&cls.netchan.message, Cmd_Args());
	}
}


/*
==================
CL_Pause_f
==================
*/
void CL_Pause_f (void)
{
	// never pause in multiplayer
	if (Cvar_VariableValue ("maxclients") > 1 || !Com_ServerState ())
	{
		Cvar_SetValue ("paused", 0);
		return;
	}

	Cvar_SetValue ("paused", !cl_paused->integer);
}

/*
==================
CL_Quit_f
==================
*/
void CL_Quit_f (void)
{
	S_StopAllSounds ();
	CL_Disconnect ();
	Com_Quit ();
}

/*
================
CL_Drop

Called after an ERR_DROP was thrown
================
*/
void CL_Drop (void)
{
	if (cls.state == ca_uninitialized)
		return;
	if (cls.state != ca_disconnected)
		CL_Disconnect ();

	// drop loading plaque unless this is the initial game start
	if (cls.disable_servercount != -1)
		SCR_EndLoadingPlaque ();	// get rid of loading plaque
}


/*
=======================
CL_SendConnectPacket

We have gotten a challenge from the server, so try and
connect.
======================
*/
void CL_SendConnectPacket (void)
{
	netadr_t	adr;
	int		port;

	if (!NET_StringToAdr (cls.servername, &adr))
	{
		Com_Printf ("Bad server address\n");
		cls.connect_time = 0;
		return;
	}
	if (adr.port == 0)
		adr.port = BigShort (PORT_SERVER);

	port = Cvar_VariableValue ("qport");
	userinfo_modified = false;

	Netchan_OutOfBandPrint (NS_CLIENT, adr, "connect %i %i %i \"%s\"\n",
		PROTOCOL_VERSION, port, cls.challenge, Cvar_Userinfo() );
}

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out
=================
*/
void CL_CheckForResend (void)
{
	netadr_t	adr;

	// if the local server is running and we aren't
	// then connect
	if (cls.state == ca_disconnected && Com_ServerState() )
	{
		cls.state = ca_connecting;
		strncpy (cls.servername, "localhost", sizeof(cls.servername)-1);
		// we don't need a challenge on the localhost
		CL_SendConnectPacket ();
		return;
//		cls.connect_time = -99999;	// CL_CheckForResend() will fire immediately
	}

	// resend if we haven't gotten a reply yet
	if (cls.state != ca_connecting)
		return;

	if (cls.realtime - cls.connect_time < 3000)
		return;

	if (!NET_StringToAdr (cls.servername, &adr))
	{
		Com_Printf ("Bad server address\n");
		cls.state = ca_disconnected;
		return;
	}
	if (adr.port == 0)
		adr.port = BigShort (PORT_SERVER);

	cls.connect_time = cls.realtime;	// for retransmit requests

	Com_Printf ("Connecting to %s...\n", cls.servername);

	Netchan_OutOfBandPrint (NS_CLIENT, adr, "getchallenge\n");
}


/*
================
CL_Connect_f

================
*/
void CL_Connect_f (void)
{
	char	*server;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("usage: connect <server>\n");
		return;
	}

	if (Com_ServerState ())
	{	// if running a local server, kill it and reissue
		SV_Shutdown (va("Server quit\n", msg), false);
	}
	else
	{
		CL_Disconnect ();
	}

	server = Cmd_Argv (1);

	NET_Config (true);		// allow remote

	CL_Disconnect ();

	cls.state = ca_connecting;
	strncpy (cls.servername, server, sizeof(cls.servername)-1);
	cls.connect_time = -99999;	// CL_CheckForResend() will fire immediately

	if (STATS_ENABLED) {
		STATS_RequestVerification();
	}
}


/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
void CL_Rcon_f (void)
{
	char		message[1024];
	sizebuf_t	buffer;
	int		i;
	netadr_t	to;

	if ( !(rcon_client_password->string && rcon_client_password->string[0]) && Cmd_Argc() < 3)
	{
		Com_Printf ("You must set 'rcon_password' before issuing an rcon command.\n");
		return;
	}

	NET_Config (true);		// allow remote

	SZ_Init( &buffer, (byte *) message, 1024 );
	buffer.allowoverflow = true;
	SZ_SetName( &buffer, "RCon buffer", false );

	SZ_Print (&buffer, "\xff\xff\xff\xffrcon ");
	if ( rcon_client_password->string && rcon_client_password->string[0] )
	{
		SZ_Print (&buffer, "\"");
		SZ_Print (&buffer, rcon_client_password->string);
		SZ_Print (&buffer, "\" ");
	}

	for (i=1 ; i<Cmd_Argc() ; i++)
	{
		SZ_Print (&buffer, "\"");
		SZ_Print (&buffer, Cmd_Argv(i));
		SZ_Print (&buffer, "\" ");
	}

	if ( buffer.overflowed )
	{
		Com_Printf ("Rcon command too long\n");
		return;
	}

	if (cls.state >= ca_connected)
		to = cls.netchan.remote_address;
	else
	{
		if (!strlen(rcon_address->string))
		{
			Com_Printf ("You must either be connected, or set the 'rcon_address' cvar to issue rcon commands\n");

			return;
		}
		NET_StringToAdr (rcon_address->string, &to);
		if (to.port == 0)
			to.port = BigShort (PORT_SERVER);
	}

	NET_SendPacket (NS_CLIENT, strlen(message)+1, message, to);
}


/*
=====================
CL_ClearState

=====================
*/
void CL_ClearState (void)
{
	S_StopAllSounds ();
	CL_ClearEffects ();
	CL_ClearTEnts ();

// wipe the entire cl structure
	memset (&cl, 0, sizeof(cl));
	memset (cl_entities, 0, sizeof(cl_entities));

	SZ_Clear (&cls.netchan.message);

}

/*
=====================
CL_Disconnect

Goes from a connected state to full screen console state
Sends a disconnect message to the server
This is also called on Com_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect (void)
{
	byte	final[32];
	int repeat;

	if (cls.state == ca_disconnected)
		return;

	if (cl_timedemo && cl_timedemo->integer)
	{
		int	time;

		time = Sys_Milliseconds () - cl.timedemo_start;
		if (time > 0)
			Com_Printf ("%i frames, %3.1f seconds: %3.1f fps\n", cl.timedemo_frames,
			time/1000.0, cl.timedemo_frames*1000.0 / time);
		cl.timedemo_start = 0;
	}

	VectorClear (cl.refdef.blend);

	M_ForceMenuOff ();

	cls.connect_time = 0;

	if (cls.demorecording)
		CL_Stop_f ();

	// send a disconnect message to the server
	final[0] = clc_stringcmd;
	strcpy ((char *)final+1, "disconnect");
	for ( repeat = 3 ; repeat-- ;)
		Netchan_Transmit( &cls.netchan, (int)strlen( (char*)final ), final );

	CL_ClearState ();

	// stop download
	if(cls.download){
		if(cls.downloadhttp)  // clean up http downloads
			CL_HttpDownloadCleanup();
		else  // or just stop legacy ones
			fclose(cls.download);
		cls.download = NULL;
	}

	cls.state = ca_disconnected;

	if 		(cl_demoquit && cl_demoquit->integer && 
			cl_timedemo && cl_timedemo->integer) {
		CL_Quit_f();
	}
}

void CL_Disconnect_f (void)
{
	Com_Error (ERR_DROP, "Disconnected from server");
}


/*
=================
CL_Changing_f

Just sent as a hint to the client that they should
drop to full console
=================
*/
void CL_Changing_f (void)
{
	//ZOID
	//if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if (cls.download)
		return;

	SCR_BeginLoadingPlaque ();
	cls.state = ca_connected;	// not active anymore, but not disconnected
	Com_Printf ("\nChanging map...\n");
}


/*
=================
CL_Reconnect_f

The server is changing levels
=================
*/
void CL_Reconnect_f (void)
{
	//ZOID
	//if we are downloading, we don't change!  This so we don't suddenly stop downloading a map
	if (cls.download)
		return;

	S_StopAllSounds ();
	if (cls.state == ca_connected) {
		Com_Printf ("reconnecting...\n");
		cls.state = ca_connected;
		MSG_WriteChar (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message, "new");
		return;
	}

	if (*cls.servername) {
		if (cls.state >= ca_connected) {
			CL_Disconnect();
			cls.connect_time = cls.realtime - 1500;
		} else
			cls.connect_time = -99999; // fire immediately

		cls.state = ca_connecting;
		Com_Printf ("reconnecting...\n");
	}
}

/*
===================
GetServerList

Get a list of servers from the master
===================
*/
static void GetServerList (void)
{
	char *requeststring;
	netadr_t adr;

	// this function may block => sound channels might not be updated
	// while it does so => prevent 'looping sound effect' while waiting
	// -JR / 20050731 / 2
	S_StopAllSounds ();

	requeststring = va( "query" );

	// send a broadcast packet
	Com_Printf( "querying %s...\n", cl_master->string );

	if( NET_StringToAdr( cl_master->string, &adr ) ) {
		if( !adr.port )
			adr.port = BigShort( PORT_MASTER );
		Netchan_OutOfBandPrint( NS_CLIENT, adr, requeststring );
	}
	else
	{
		Com_Printf( "Bad address: %s\n", cl_master->string);
	}

	// Query secondary server
	Com_Printf( "querying %s...\n", cl_master2->string );
	if ( NET_StringToAdr( cl_master2->string, &adr ) ) {
		if ( !adr.port )
			adr.port = BigShort( PORT_MASTER );
		Netchan_OutOfBandPrint( NS_CLIENT, adr, requeststring );
	}
	else
	{
		Com_Printf( "Bad address: %s\n", cl_master2->string);
	}

	// Reset server list for incoming responses.
	numServers = 0;
}
/*
=================
CL_ParseGetServersResponse

Handle a reply from getservers message to master server
=================
*/
static void CL_ParseGetServersResponse()
{
	cvar_t		*noudp;
	cvar_t		*noipx;
	netadr_t	adr;
	char		adrString[32];
	byte		addr[4];
	byte		port[2];
	int			idx;
	unsigned short	uport;
	qboolean	dupe;

	MSG_BeginReading (&net_message);
	MSG_ReadLong (&net_message);	// skip the -1

	noudp = Cvar_Get ("noudp", "0", CVAR_NOSET);
	if (!noudp->integer)
	{
		adr.type = NA_BROADCAST;
	}
	noipx = Cvar_Get ("noipx", "0", CVAR_NOSET);
	if (!noipx->integer)
	{
		adr.type = NA_BROADCAST_IPX;
	}

	if ( net_message.readcount + 8 < net_message.cursize )
		net_message.readcount += 8;

	while( net_message.readcount +6 <= net_message.cursize )
	{
		if (numServers == 64)
			break;		// No room left.

		MSG_ReadData( &net_message, addr, 4 );
		Com_sprintf( adrString, sizeof( adrString ), "%u.%u.%u.%u",
				addr[0], addr[1], addr[2], addr[3]);
		MSG_ReadData( &net_message, port, 2 ); /* network byte order (bigendian) */
		/* convert to unsigned short in host byte order */
		uport = (unsigned short) ((port[0] << 8) + port[1]);

		dupe = false;
		// Check that we don't have a duplicate entry.
		for (idx = 0; idx < numServers; idx++) {
			if (	!strcmp(servers[idx].ip, adrString) &&
					servers[idx].port == uport) {
				Com_Printf("Already have: %s:%u\n", adrString,
						uport);
				dupe = true;
				break;
			}
		}
		if (dupe)
			continue;

		memcpy( &servers[numServers].ip, adrString, sizeof(servers[numServers].ip) );
		servers[numServers].port = uport;

		if (!NET_StringToAdr (servers[numServers].ip, &adr))
		{
			Com_Printf ("Bad address: %s\n", servers[numServers].ip);
			continue;
		}

		Com_Printf ("pinging %s:%hu...\n",
				servers[numServers].ip, servers[numServers].port );

		/* adr.port is in network byte order (bigendian) */
		adr.port = (unsigned short)BigShort( servers[numServers].port );
		if (!adr.port) {
			adr.port = BigShort(PORT_SERVER);
		}
		Netchan_OutOfBandPrint (NS_CLIENT, adr, va("status %i", PROTOCOL_VERSION));
		servers[numServers].starttime = Sys_Milliseconds();

		++numServers;
	}
}

/*
=================
CL_GetPingStartTime

Return ping starttime for server given its address.  Returns 0 if not found.
=================
*/
int CL_GetPingStartTime(netadr_t adr)
{
	int		idx;
	char		adrString[32];
	unsigned short	port;

	Com_sprintf(adrString, sizeof(adrString), "%u.%u.%u.%u",
			adr.ip[0], adr.ip[1], adr.ip[2], adr.ip[3]);
	port = (unsigned short) BigShort(adr.port);
	for (idx = 0; idx < numServers; idx++) {
		if (! strcmp(adrString, servers[idx].ip) &&
				port == servers[idx].port) {
			return servers[idx].starttime;
		}
	}
	return 0;
}

/*
=================
CL_PingServers_f
=================
*/
static void CL_ReadPackets ();
void CL_PingServers_f (void)
{

	int			i;
	netadr_t	adr;
	char		name[32];
	char		*adrstring;
	cvar_t		*noudp;
	cvar_t		*noipx;

	NET_Config (true);		// allow remote

	GetServerList();		//get list from COR master server

	// send a broadcast packet
	Com_Printf ("pinging broadcast...\n");

	noudp = Cvar_Get ("noudp", "0", CVAR_NOSET);
	if (!noudp->integer)
	{
		adr.type = NA_BROADCAST;
		adr.port = BigShort(PORT_SERVER);
		Netchan_OutOfBandPrint (NS_CLIENT, adr, va("status %i", PROTOCOL_VERSION));

	}
	noipx = Cvar_Get ("noipx", "0", CVAR_NOSET);
	if (!noipx->integer)
	{
		adr.type = NA_BROADCAST_IPX;
		adr.port = BigShort(PORT_SERVER);
		Netchan_OutOfBandPrint (NS_CLIENT, adr, va("status %i", PROTOCOL_VERSION));
	}

	// send a packet to each address book entry
	for (i=0 ; i<16 ; i++)
	{
		Com_sprintf (name, sizeof(name), "adr%i", i);
		adrstring = Cvar_VariableString (name);
		if (!adrstring || !adrstring[0])
			continue;

		Com_Printf ("pinging %s...\n", adrstring);
		if (!NET_StringToAdr (adrstring, &adr))
		{
			Com_Printf ("Bad address: %s\n", adrstring);
			continue;
		}
		if (!adr.port)
			adr.port = BigShort(PORT_SERVER);

		Netchan_OutOfBandPrint (NS_CLIENT, adr, va("status %i", PROTOCOL_VERSION));

	}
	
	// Note that all we have done thus far is 
	// - Request server lists from the two master servers
	// - Sent a broadcast to the LAN looking for local servers
	// When CL_ReadPackets gets the responses from the master server, it will
	// automatically ping all the game servers. Ideally, if the net connection
	// is pretty good, we can get that all done in the following loop. The 
	// game's main loop will catch any stragglers.
	
	// Read packets at 5 ms intervals 60 times. That's 300 ms. Figure maybe 
	// 100-150 ms round trip to the master server, then another 100-150 ms 
	// round trip to the game servers. So we should be able to grab any 
	// servers 150 ping or less and gauge their pings to within 5 ms or so.
#define PING_LOOP_TOTAL_MS		300
#define PING_LOOP_INTERVAL_MS	5
#define PING_LOOP_ITERATIONS	(PING_LOOP_TOTAL_MS/PING_LOOP_INTERVAL_MS)
	for (i = 0; i < PING_LOOP_ITERATIONS; i++)
	{
		#if defined UNIX_VARIANT
			usleep (PING_LOOP_INTERVAL_MS*1000);
		#elif defined WIN32_VARIANT
			Sleep (PING_LOOP_INTERVAL_MS);
		#else
			#warning	client/cl_main.c: CL_PingServers_f (): \
						Do not know what sleep function to use!
			break;
		#endif
		CL_ReadPackets ();
	}

	// -JD restart the menu music that was stopped during this procedure
	S_StartMenuMusic();
}


/*
=================
CL_Skins_f

Load or download any custom player skins and models
=================
*/
void CL_Skins_f (void)
{
	int		i;

	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		if (!cl.configstrings[CS_PLAYERSKINS+i][0])
			continue;
		Com_Printf ("client %i: %s\n", i, cl.configstrings[CS_PLAYERSKINS+i]);
		SCR_UpdateScreen ();
		Sys_SendKeyEvents ();	// pump message loop
		CL_ParseClientinfo (i);
	}
}


/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
static void CL_ConnectionlessPacket (void)
{
	char	*s;
	char	*c;
	char	*net_from_str;
	qboolean suppress_command_print = false;

	MSG_BeginReading (&net_message);
	MSG_ReadLong (&net_message);	// skip the -1

	s = MSG_ReadStringLine (&net_message);

	Cmd_TokenizeString (s, false);

	c = Cmd_Argv(0);
	
	net_from_str = NET_AdrToString (net_from);

	if (!strcmp (c, "print"))
	{
		netadr_t to;
		char	*to_str;
		
		if (cls.state >= ca_connected)
			to = cls.netchan.remote_address;
		else if (rcon_address != NULL)
			NET_StringToAdr (rcon_address->string, &to);
		
		// Convert back to string to populate the default port, expand short
		// IPs like 127.1 to 127.0.0.1, etc.
		to_str = NET_AdrToString (to);
		
		suppress_command_print = !strcmp (to_str, net_from_str);
	}
	
	if (!suppress_command_print)
		Com_Printf ("%s: %s\n", net_from_str, c);

	if(!strncmp(c, "servers", 7))
	{
		CL_ParseGetServersResponse();
		return;
	}

	if(!strncmp(c, "vstring", 7))
	{
		s = Cmd_Argv(1);
		switch(currLoginState.requestType)
		{
			case STATSLOGIN:
				STATS_AuthenticateStats(s);
				break;
			case STATSPWCHANGE:
				STATS_ChangePassword(s);
				break;
		}
		return;
	}

	if(!strncmp(c, "validated", 9))
	{
		//in cases of changed passwords
		if(currLoginState.requestType == STATSPWCHANGE)
		{
			//make sure the password is stored for future use
			Q_strncpyz2(currLoginState.old_password, stats_password->string, sizeof(currLoginState.old_password));

			Com_Printf("Password change successful!\n");
		}
		else
		{
			Com_Printf("Account validated!\n");
			currLoginState.validated = true;
		}
		return;
	}

	// server connection
	if (!strcmp(c, "client_connect"))
	{
		if (cls.state == ca_connected)
		{
			Com_Printf ("Dup connect received.  Ignored.\n");
			return;
		}
		Netchan_Setup (NS_CLIENT, &cls.netchan, net_from, cls.quakePort);
		MSG_WriteChar (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message, "new");
		cls.state = ca_connected;

		memset(cls.downloadurl, 0, sizeof(cls.downloadurl));
		if(Cmd_Argc() == 2)  // http download url
		{
			strncpy(cls.downloadurl, Cmd_Argv(1), sizeof(cls.downloadurl) - 1);
		}
		is_localhost = !strcmp(cls.servername, "localhost");
		Netchan_OutOfBandPrint (NS_CLIENT, net_from, va("status %i", PROTOCOL_VERSION));
		return;
	}

	// remote command from gui front end
	if (!strcmp(c, "cmd"))
	{
		if (!NET_IsLocalAddress(net_from))
		{
			Com_Printf ("Command packet from remote host.  Ignored.\n");
			return;
		}
		Sys_AppActivate ();
		s = MSG_ReadString (&net_message);
		Cbuf_AddText (s);
		Cbuf_AddText ("\n");
		return;
	}

	// print command from somewhere
	if (!strcmp(c, "print"))
	{
		s = MSG_ReadString (&net_message);
		if (s[0] == '\\')
		{
			char *playerinfo_start;
			if (cls.state >= ca_connected && 
				!memcmp(&net_from, &cls.netchan.remote_address, sizeof(netadr_t)))
				M_UpdateConnectedServerInfo (net_from, s);
			if (cls.key_dest == key_menu)
			{
				M_AddToServerList (net_from, s); //add net_from so we have the addy
			}
			else
			{
				// If someone called pingservers () directly from the console,
				// chances are he wants to read the server list manually 
				// anyway.
				playerinfo_start = strchr (s, '\n');
				*playerinfo_start++ = '\0';
				Info_Print (s);
				Com_Printf ("%s", playerinfo_start);
			}
		}
		else 
			Com_Printf ("%s", s);
		return;
	}
	
	// ping from somewhere
	if (!strcmp(c, "ping"))
	{
		Netchan_OutOfBandPrint (NS_CLIENT, net_from, "ack");
		return;
	}

	// challenge from the server we are connecting to
	if (!strcmp(c, "challenge"))
	{
		cls.challenge = atoi(Cmd_Argv(1));
		CL_SendConnectPacket ();
		return;
	}

	// echo request from server
	if (!strcmp(c, "echo"))
	{
		Netchan_OutOfBandPrint (NS_CLIENT, net_from, "%s", Cmd_Argv(1) );
		return;
	}

	if (!strcmp(c, "teamgame"))
	{
		server_is_team = atoi (Cmd_Argv(1));
		return;
	}

	if (!strcmp(c, "tickrate"))
	{
		server_tickrate = atoi (Cmd_Argv(1));
		return;
	}

	Com_Printf ("Unknown command.\n");
}


/*
=================
CL_DumpPackets

A vain attempt to help bad TCP stacks that cause problems
when they overflow
=================
*/
void CL_DumpPackets (void)
{
	while (NET_GetPacket (NS_CLIENT, &net_from, &net_message))
	{
		Com_Printf ("dumping a packet\n");
	}
}

/*
=================
CL_ReadPackets
=================
*/
int c_incoming_bytes = 0;
static void CL_ReadPackets (void)
{
	while (NET_GetPacket (NS_CLIENT, &net_from, &net_message))
	{
		c_incoming_bytes += net_message.cursize;

		//
		// remote command packet
		//
		if (*(int *)net_message.data == -1)
		{
			CL_ConnectionlessPacket ();
			continue;
		}

		if (cls.state == ca_disconnected || cls.state == ca_connecting)
			continue;		// dump it if not connected

		if (net_message.cursize < 8)
		{
			Com_Printf ("%s: Runt packet\n",NET_AdrToString(net_from));
			continue;
		}

		//
		// packet from server
		//
		if (!NET_CompareAdr (net_from, cls.netchan.remote_address))
		{
			Com_DPrintf ("%s:sequenced packet without connection\n"
				,NET_AdrToString(net_from));
			continue;
		}
		if (!Netchan_Process(&cls.netchan, &net_message))
			continue;		// wasn't accepted for some reason
		CL_ParseServerMessage ();
	}

	//
	// check timeout
	//
	if (cls.state >= ca_connected
	 && cls.realtime - cls.netchan.last_received > cl_timeout->value*1000)
	{
		if (++cl.timeoutcount > 5)	// timeoutcount saves debugger
		{
			Com_Printf ("\nServer connection timed out.\n");
			CL_Disconnect ();
			return;
		}
	}
	else
		cl.timeoutcount = 0;

}


//=============================================================================

/*
==============
CL_FixUpGender_f
==============
*/
void CL_FixUpGender(void)
{
	char *p;
	char sk[80];

	if (gender_auto->integer)
	{
		if (gender->modified)
		{
			// was set directly, don't override the user
			gender->modified = false;
			return;
		}

		strncpy(sk, skin->string, sizeof(sk) - 2);
		sk[sizeof(sk) - 1] = '\0'; // in case skin->string is 79 chars or more
		if ((p = strchr(sk, '/')) != NULL)
			*p = 0;
		if (Q_strcasecmp(sk, "male") == 0 || Q_strcasecmp(sk, "cyborg") == 0)
			Cvar_Set ("gender", "male");
		else if (Q_strcasecmp(sk, "female") == 0 || Q_strcasecmp(sk, "crackhor") == 0)
			Cvar_Set ("gender", "female");
		else
			Cvar_Set ("gender", "none");
		gender->modified = false;
	}
}

/*
==============
CL_Userinfo_f
==============
*/
void CL_Userinfo_f (void)
{
	Com_Printf ("User info settings:\n");
	Info_Print (Cvar_Userinfo());
}

/*
=================
CL_Snd_Restart_f

Restart the sound subsystem so it can pick up
new parameters and flush all sounds
=================
*/
void CL_Snd_Restart_f (void)
{
	S_Shutdown ();
	S_Init ();
	CL_RegisterSounds ();
}

int precache_check; // for autodownload of precache items
int precache_spawncount;
int precache_tex;
int precache_model_skin;

byte *precache_model; // used for skin checking in alias models

#define PLAYER_MULT 5

// ENV_CNT is map load, ENV_CNT+1 is first env map
#define ENV_CNT (CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT)
#define TEXTURE_CNT (ENV_CNT+13)
#define SCRIPT_CNT (TEXTURE_CNT+999)

static const char *env_suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};

void CL_RequestNextDownload (void)
{
	unsigned	map_checksum;		// for detecting cheater maps
	char fn[MAX_OSPATH];
	char map[MAX_OSPATH];
	char script[MAX_OSPATH];
	dmdl_t *pheader;
	int i, j;

	if (cls.state != ca_connected)
		return;

	if (!allow_download->integer && precache_check < ENV_CNT)
		precache_check = ENV_CNT;

	if (precache_check == CS_MODELS) // confirm map
	{
		precache_check = CS_MODELS+2; // 0 isn't used
		if (allow_download_maps->integer)
			if (!CL_CheckOrDownloadFile(cl.configstrings[CS_MODELS+1]))
				return; // started a download
	}
	
redoSkins:
	if (precache_check >= CS_MODELS && precache_check < CS_MODELS+MAX_MODELS)
	{
		if (allow_download_models->integer)
		{
			while (precache_check < CS_MODELS+MAX_MODELS &&
				cl.configstrings[precache_check][0])
			{
				if (cl.configstrings[precache_check][0] == '*' ||
					cl.configstrings[precache_check][0] == '#')
				{
					precache_check++;
					continue;
				}
				//do not download player models from this section
				else if (cl.configstrings[precache_check][0] == 'p' &&
					cl.configstrings[precache_check][1] == 'l' &&
					cl.configstrings[precache_check][2] == 'a' &&
					cl.configstrings[precache_check][3] == 'y' &&
					cl.configstrings[precache_check][4] == 'e' &&
					cl.configstrings[precache_check][5] == 'r' &&
					cl.configstrings[precache_check][6] == 's')
				{
					precache_check++;
					continue;
				}
				if (precache_model_skin == 0)
				{
					if (!CL_CheckOrDownloadFile(cl.configstrings[precache_check]))
					{
						precache_model_skin = 1;
						return; // started a download
					}
					precache_model_skin = 1;
				}

				// checking for skins in the model
				if (!precache_model)
				{
					FS_LoadFile (cl.configstrings[precache_check], (void **)&precache_model);
					if (!precache_model)
					{
						precache_model_skin = 0;
						precache_check++;
						continue; // couldn't load it
					}
					if (LittleLong(*(unsigned *)precache_model) != IDALIASHEADER)
					{
						// not an alias model
						FS_FreeFile(precache_model);
						precache_model = 0;
						precache_model_skin = 0;
						precache_check++;
						continue;
					}
					pheader = (dmdl_t *)precache_model;
					if (LittleLong (pheader->version) != ALIAS_VERSION)
					{
						precache_check++;
						precache_model_skin = 0;
						continue; // couldn't load it
					}
				}

				pheader = (dmdl_t *)precache_model;

				while (precache_model_skin - 1 < LittleLong(pheader->num_skins))
				{
					if (!CL_CheckOrDownloadFile((char *)precache_model +
						LittleLong(pheader->ofs_skins) +
						(precache_model_skin - 1)*MAX_SKINNAME))
					{
						precache_model_skin++;
						return; // started a download
					}
					precache_model_skin++;
				}
				if (precache_model)
				{
					FS_FreeFile(precache_model);
					precache_model = 0;
				}
				precache_model_skin = 0;
				precache_check++;
			}
		}
		if (precache_model)
		{
			precache_check = CS_MODELS + 2;
			precache_model_skin = 0;

			goto redoSkins;
		}
		precache_check = CS_SOUNDS;
	}

	if (precache_check >= CS_SOUNDS && precache_check < CS_SOUNDS+MAX_SOUNDS)
	{
		if (allow_download_sounds->integer)
		{
			if (precache_check == CS_SOUNDS)
				precache_check++; // zero is blank
			while (precache_check < CS_SOUNDS+MAX_SOUNDS &&
				cl.configstrings[precache_check][0])
			{
				if (cl.configstrings[precache_check][0] == '*')
				{
					precache_check++;
					continue;
				}
				Com_sprintf(fn, sizeof(fn), "sound/%s", cl.configstrings[precache_check++]);
				if (!CL_CheckOrDownloadFile(fn))
					return; // started a download
			}
		}
		precache_check = CS_IMAGES;
	}
	if (precache_check >= CS_IMAGES && precache_check < CS_IMAGES+MAX_IMAGES)
	{
		if (precache_check == CS_IMAGES)
			precache_check++; // zero is blank
		while (precache_check < CS_IMAGES+MAX_IMAGES &&
			cl.configstrings[precache_check][0])
		{
			Com_sprintf(fn, sizeof(fn), "pics/%s.tga", cl.configstrings[precache_check++]);
			if (!CL_CheckOrDownloadFile(fn))
				return; // started a download
		}
		precache_check = CS_PLAYERSKINS;
	}
	if (precache_check >= CS_PLAYERSKINS && precache_check < CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT)
	{
		if (allow_download_players->integer)
		{
			while (precache_check < CS_PLAYERSKINS + MAX_CLIENTS * PLAYER_MULT)
			{
				int i, n;
				char model[MAX_QPATH], skin[MAX_QPATH], *p;

				i = (precache_check - CS_PLAYERSKINS)/PLAYER_MULT;
				n = (precache_check - CS_PLAYERSKINS)%PLAYER_MULT;

				if (!cl.configstrings[CS_PLAYERSKINS+i][0])
				{
					precache_check = CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
					continue;
				}

				if ((p = strchr(cl.configstrings[CS_PLAYERSKINS+i], '\\')) != NULL)
					p++;
				else
					p = cl.configstrings[CS_PLAYERSKINS+i];
				strcpy(model, p);
				p = strchr(model, '/');
				if (!p)
					p = strchr(model, '\\');
				if (p)
				{
					*p++ = 0;
					strcpy(skin, p);
				} else
					*skin = 0;

				switch (n)
				{
					case 0: // model
						Com_sprintf(fn, sizeof(fn), "players/%s/tris.iqm", model);
						if (!CL_CheckOrDownloadFile(fn))
						{
							precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 1;
							return; // started a download
						}
						n++;
						/*FALL THROUGH*/

					case 1: // weapon model
						Com_sprintf(fn, sizeof(fn), "players/%s/weapon.iqm", model);
						if (!CL_CheckOrDownloadFile(fn)) {
							precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 2;
							return; // started a download
						}
						n++;
						/*FALL THROUGH*/

					case 2: // weapon skin
						Com_sprintf(fn, sizeof(fn), "players/%s/weapon.jpg", model);
						if (!CL_CheckOrDownloadFile(fn))
						{
							precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 3;
							return; // started a download
						}
						n++;
						/*FALL THROUGH*/

					case 3: // skin
						Com_sprintf(fn, sizeof(fn), "players/%s/%s.jpg", model, skin);
						if (!CL_CheckOrDownloadFile(fn))
						{
							precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 4;
							return; // started a download
						}
						n++;
						/*FALL THROUGH*/

					case 4: // skin_i
						Com_sprintf(fn, sizeof(fn), "players/%s/%s_i.jpg", model, skin);
						if (!CL_CheckOrDownloadFile(fn))
						{
							precache_check = CS_PLAYERSKINS + i * PLAYER_MULT + 5;
							return; // started a download
						}
						// move on to next model
						precache_check = CS_PLAYERSKINS + (i + 1) * PLAYER_MULT;
				}
			}
		}
		// precache phase completed
		precache_check = ENV_CNT;
	}

	if (precache_check == ENV_CNT)
	{
		precache_check = ENV_CNT + 1;

		//FIXME: support for new map specification files
		CM_LoadBSP (cl.configstrings[CS_MODELS+1], true, &map_checksum);

		if (map_checksum != atoi(cl.configstrings[CS_MAPCHECKSUM]))
		{
			Com_Error (ERR_DROP, "Local map version differs from server: %i != '%s'\n",
				map_checksum, cl.configstrings[CS_MAPCHECKSUM]);
			return;
		}
	}

	if (precache_check > ENV_CNT && precache_check < TEXTURE_CNT)
	{
			if (allow_download->integer && allow_download_maps->integer)
			{
				while (precache_check < TEXTURE_CNT)
				{
					int n = precache_check++ - ENV_CNT - 1;

					Com_sprintf(fn, sizeof(fn), "env/%s%s.jpg",
							cl.configstrings[CS_SKY], env_suf[n/2]);
					if (!CL_CheckOrDownloadFile(fn))
						return; // started a download
				}
			}
			precache_check = TEXTURE_CNT;
	}
	if (precache_check == TEXTURE_CNT)
	{
		precache_check = TEXTURE_CNT+1;
		precache_tex = 0;
	}

	// confirm existance of textures, download any that don't exist
	if (precache_check == TEXTURE_CNT+1)
	{
		// from qcommon/cmodel.c
		extern int			numtexinfo;
		extern mapsurface_t	map_surfaces[];

		if (allow_download->integer && allow_download_maps->integer)
		{
			while (precache_tex < numtexinfo)
			{
				char fn[MAX_OSPATH];
				/* 2015-12-14 default extension for textures is now
				 * jpg instead of tga.  CL_CheckOrDownloadFile will
				 * check for tga if jpg is not found.
				 */
				sprintf(fn, "textures/%s.jpg", map_surfaces[precache_tex++].rname);
				if (!CL_CheckOrDownloadFile(fn))
					return; // started a download
			}
		}
		precache_check = SCRIPT_CNT;
	}

	//get map related scripts
	if (precache_check == SCRIPT_CNT)
	{
		precache_check = SCRIPT_CNT+1;
		if (allow_download_maps->integer)
		{
			//get fog files
			COM_StripExtension ( cl.configstrings[CS_MODELS+1], fn );

			//remove "maps/" from string
			for(i = 5, j = 0; i < strlen(fn); i++, j++)
			{
				map[j] = fn[i];
			}
			map[i-5] = 0;

			Com_sprintf(script, sizeof(script), "maps/scripts/%s.fog", map);
			if (!CL_CheckOrDownloadFile(script))
				return; // started a download
		}
	}

	if (precache_check == SCRIPT_CNT+1)
	{
		precache_check = SCRIPT_CNT+2;
		if (allow_download_maps->integer)
		{
			//get mus files

			COM_StripExtension ( cl.configstrings[CS_MODELS+1], fn );

			//remove "maps/" from string
			for(i = 5, j = 0; i < strlen(fn); i++, j++)
				map[j] = fn[i];
			map[i-5] = 0;

			Com_sprintf(script, sizeof(script), "maps/scripts/%s.mus", map);
			if (!CL_CheckOrDownloadFile(script))
				return; // started a download
		}
	}

	if (precache_check == SCRIPT_CNT+2)
	{
		precache_check = SCRIPT_CNT+3;
		if (allow_download_maps->integer)
		{
			//get rscript files
			COM_StripExtension ( cl.configstrings[CS_MODELS+1], fn );

			//remove "maps/" from string
			for(i = 5, j = 0; i < strlen(fn); i++, j++)
				map[j] = fn[i];
			map[i-5] = 0;

			Com_sprintf(script, sizeof(script), "scripts/maps/%s.rscript", map);
			if (!CL_CheckOrDownloadFile(script))
				return; // started a download
		}
	}
	
	if (precache_check == SCRIPT_CNT+3) // try downloading lightmap
	{
		precache_check = SCRIPT_CNT+4;
		if (allow_download_maps->integer)
		{
			char *extension;
			strncpy (fn, cl.configstrings[CS_MODELS+1], MAX_OSPATH-1-strlen(".lightmap")+strlen(".bsp"));
			extension = strstr (fn, ".bsp");
			if (extension)
				*extension = 0;
			strcat (fn, ".lightmap"); 
			if (!CL_CheckOrDownloadFile(fn))
				return; // started a download
		}
	}

//ZOID
	CL_RegisterSounds ();
	CL_PrepRefresh ();

	MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
	MSG_WriteString (&cls.netchan.message, va("begin %i\n", precache_spawncount) );

	{
		netadr_t	adr;

		if (!NET_StringToAdr (cls.servername, &adr))
		{
			Com_Printf ("Bad address: %s\n",  cls.servername);
			return;
		}

		// default true to avoid messing up legacy ctf servers
		// legacy DM servers don't send visibility lights anyway
		server_is_team = true;
		Netchan_OutOfBandPrint (NS_CLIENT, adr, "teamgame\n");

		Netchan_OutOfBandPrint (NS_CLIENT, adr, "tickrate\n");
	}
}


netadr_t *CL_GetRemoteServer (void) {
	static netadr_t remoteserver;
	if (cls.state >= ca_connected) {
		remoteserver = cls.netchan.remote_address;
		return &remoteserver;
	}
	return NULL;
}


/*
=================
CL_Precache_f

The server will send this command right
before allowing the client into the server
=================
*/
void CL_Precache_f (void)
{
	//Yet another hack to let old demos work
	//the old precache sequence
	if (Cmd_Argc() < 2)
	{
		unsigned	map_checksum;		// for detecting cheater maps

		//FIXME: support for new map specification files
		CM_LoadBSP (cl.configstrings[CS_MODELS+1], true, &map_checksum);
		CL_RegisterSounds ();
		CL_PrepRefresh ();
		return;
	}

	precache_check = CS_MODELS;
	precache_spawncount = atoi(Cmd_Argv(1));
	precache_model = 0;
	precache_model_skin = 0;

	CL_RequestNextDownload();
}

/*
In case equal to the old default, then change to the new default.
Only usable for string cvars for now.
*/
void CL_RepairStringCvarIfDefault(cvar_t *cvar, char *oldDefault, char *newDefault)
{
	if (!Q_strcasecmp(cvar->string, oldDefault)) {
		Com_Printf("%s changed from %s to %s\n", cvar->name, cvar->string, newDefault);
		Cvar_Set(cvar->name, newDefault);
		strcpy(cvar->string, newDefault);
	}
}

/*
In case not equal to the new default, then change to the new default.
Only usable for string cvars for now.
*/
void CL_RepairStringCvar(cvar_t *cvar, char *newDefault)
{
	if (Q_strcasecmp(cvar->string, newDefault)) {
		Com_Printf("%s changed from %s to %s\n", cvar->name, cvar->string, newDefault);
		Cvar_Set(cvar->name, newDefault);
		strcpy(cvar->string, newDefault);
	}
}

/*
=================
CL_InitLocal
=================
*/
void CL_InitLocal (void)
{
	cls.state = ca_disconnected;
	cls.realtime = Sys_Milliseconds ();

	CL_InitInput ();

	CL_InitHttpDownload();

	CL_IRCSetup( );

	adr0 = Cvar_Get( "adr0", "", CVAR_ARCHIVE );
	adr1 = Cvar_Get( "adr1", "", CVAR_ARCHIVE );
	adr2 = Cvar_Get( "adr2", "", CVAR_ARCHIVE );
	adr3 = Cvar_Get( "adr3", "", CVAR_ARCHIVE );
	adr4 = Cvar_Get( "adr4", "", CVAR_ARCHIVE );
	adr5 = Cvar_Get( "adr5", "", CVAR_ARCHIVE );
	adr6 = Cvar_Get( "adr6", "", CVAR_ARCHIVE );
	adr7 = Cvar_Get( "adr7", "", CVAR_ARCHIVE );
	adr8 = Cvar_Get( "adr8", "", CVAR_ARCHIVE );

//
// register our variables
//
	cl_stereo_separation = Cvar_Get( "cl_stereo_separation", "0.4", CVAR_ARCHIVE | CVARDOC_FLOAT );
	cl_stereo = Cvar_Get( "cl_stereo", "0", CVARDOC_BOOL );
	background_music = Cvar_Get("background_music", "1", CVAR_ARCHIVE | CVARDOC_BOOL);
	background_music_vol = Cvar_Get("background_music_vol", "0.5", CVAR_ARCHIVE | CVARDOC_FLOAT);

	cl_add_blend = Cvar_Get ("cl_blend", "1", CVAR_ARCHIVE | CVARDOC_BOOL);
	cl_add_lights = Cvar_Get ("cl_lights", "1", CVARDOC_BOOL);
	cl_add_particles = Cvar_Get ("cl_particles", "1", CVARDOC_BOOL);
	cl_add_entities = Cvar_Get ("cl_entities", "1", CVARDOC_BOOL);
	cl_gun = Cvar_Get ("cl_gun", "1", CVAR_ARCHIVE | CVARDOC_BOOL);
	cl_footsteps = Cvar_Get ("cl_footsteps", "1", CVARDOC_BOOL);
	cl_noskins = Cvar_Get ("cl_noskins", "0", CVAR_ARCHIVE | CVARDOC_BOOL);
	Cvar_Describe (cl_noskins, "for all players to be shown with the same skin.");
	cl_autoskins = Cvar_Get ("cl_autoskins", "0", CVARDOC_BOOL);
	cl_predict = Cvar_Get ("cl_predict", "1", CVARDOC_BOOL);
	cl_maxfps = Cvar_Get ("cl_maxfps", "60", CVAR_ARCHIVE | CVARDOC_INT);
	Cvar_Describe (cl_maxfps, "limit the frames drawn per second using a busy-wait loop.");
	cl_showPlayerNames = Cvar_Get ("cl_showplayernames", "0", CVAR_ARCHIVE | CVARDOC_INT);
	Cvar_Describe (cl_showPlayerNames, "0 means no nametags, 1 means show one nametag in the center of the screen, 2 means show a nametag over each player.");
	cl_noblood = Cvar_Get ("cl_noblood", "0", CVAR_ARCHIVE | CVARDOC_BOOL);
	cl_disbeamclr = Cvar_Get("cl_disbeamclr", "0", CVAR_ARCHIVE | CVARDOC_INT);
	Cvar_Describe (cl_disbeamclr, "beam color for the disruptor weapon. 0 = green, 1 = blue, 2 = red, 3 = yellow, 4 = purple.");
	cl_brass = Cvar_Get ("cl_brass", "1", CVAR_ARCHIVE | CVARDOC_BOOL);
	cl_dmlights = Cvar_Get("cl_dm_lights", "0", CVAR_ARCHIVE | CVARDOC_BOOL);
	cl_playtaunts = Cvar_Get ("cl_playtaunts", "1", CVAR_ARCHIVE | CVARDOC_BOOL);
	cl_centerprint = Cvar_Get ("cl_centerprint", "1", CVAR_ARCHIVE | CVARDOC_BOOL);
	Cvar_Describe (cl_centerprint, "enable text in the center of the screen.");
	cl_precachecustom = Cvar_Get ("cl_precachecustom", "0", CVAR_ARCHIVE | CVARDOC_BOOL);
	Cvar_Describe (cl_precachecustom, "precache 3rd-party and custom player skins at the first map load.");
	cl_simpleitems = Cvar_Get ("cl_simpleitems", "0", CVAR_ARCHIVE | CVARDOC_BOOL);
	Cvar_Describe (cl_simpleitems, "show sprites instead of models for pickup items.");
	cl_flicker = Cvar_Get ("cl_flicker", "1", CVAR_ARCHIVE | CVARDOC_BOOL);
	Cvar_Describe (cl_flicker, "enable flickering world lighting.");

	cl_paindist = Cvar_Get ("cl_paindist", "1", CVAR_ARCHIVE);
	Cvar_Describe (cl_paindist, "distort the screen when you take damage.");
	cl_explosiondist = Cvar_Get ("cl_explosiondist", "1", CVAR_ARCHIVE);
	Cvar_Describe (cl_explosiondist, "distort the screen for explosions.");
	cl_raindist = Cvar_Get ("cl_raindist", "1", CVAR_ARCHIVE);
	Cvar_Describe (cl_raindist, "rain running down your visor.");

	cl_upspeed = Cvar_Get ("cl_upspeed", "200", 0);
	cl_forwardspeed = Cvar_Get ("cl_forwardspeed", "200", 0);
	cl_sidespeed = Cvar_Get ("cl_sidespeed", "200", 0);
	cl_yawspeed = Cvar_Get ("cl_yawspeed", "140", 0);
	cl_pitchspeed = Cvar_Get ("cl_pitchspeed", "150", 0);
	cl_anglespeedkey = Cvar_Get ("cl_anglespeedkey", "1.5", 0);

	cl_run = Cvar_Get ("cl_run", "0", CVAR_ARCHIVE | CVARDOC_BOOL);
	freelook = Cvar_Get( "freelook", "0", CVAR_ARCHIVE | CVARDOC_BOOL);
	lookspring = Cvar_Get ("lookspring", "0", CVAR_ARCHIVE | CVARDOC_BOOL);
	lookstrafe = Cvar_Get ("lookstrafe", "0", CVAR_ARCHIVE | CVARDOC_BOOL);
	m_accel	= Cvar_Get ("m_accel", "1",	CVAR_ARCHIVE | CVARDOC_BOOL);
	sensitivity = Cvar_Get ("sensitivity", "3", CVAR_ARCHIVE | CVARDOC_FLOAT);
	menu_sensitivity = Cvar_Get("menu_sensitivity", "3", CVAR_ARCHIVE | CVARDOC_FLOAT);

	m_pitch = Cvar_Get ("m_pitch", "0.022", CVAR_ARCHIVE | CVARDOC_FLOAT);
	m_yaw = Cvar_Get ("m_yaw", "0.022", CVAR_ARCHIVE | CVARDOC_FLOAT);
	m_forward = Cvar_Get ("m_forward", "1", 0);
	m_side = Cvar_Get ("m_side", "1", 0);

	cl_shownet = Cvar_Get ("cl_shownet", "0", 0);
	cl_showmiss = Cvar_Get ("cl_showmiss", "0", 0);
	cl_showclamp = Cvar_Get ("showclamp", "0", 0);
	cl_timeout = Cvar_Get ("cl_timeout", "120", 0);
	cl_paused = Cvar_Get ("paused", "0", 0);
	cl_timedemo = Cvar_Get ("timedemo", "0", CVARDOC_BOOL);
	Cvar_Describe (cl_timedemo, "play back demos (recorded games) in benchmark mode.");
	cl_demoquit = Cvar_Get ("demoquit", "0", CVARDOC_BOOL);
	Cvar_Describe (cl_demoquit, "quit automatically after a demo has finished.");
	
	cl_speedrecord = Cvar_Get ("cl_speedrecord", "450", CVARDOC_INT);
	cl_alltimespeedrecord = Cvar_Get ("cl_alltimespeedrecord", "450", CVAR_ARCHIVE | CVARDOC_INT);

	rcon_client_password = Cvar_Get ("rcon_password", "", CVARDOC_STR);
	rcon_address = Cvar_Get ("rcon_address", "", CVARDOC_STR);
	
	cl_test = Cvar_Get ("cl_test", "0", CVAR_ARCHIVE);

	//
	// userinfo
	//
	info_password = Cvar_Get ("password", "", CVAR_USERINFO | CVARDOC_STR);
	info_spectator = Cvar_Get ("spectator", "0", CVAR_USERINFO | CVARDOC_BOOL);
	info_spectator_password = Cvar_Get ( "spectator_password", "", CVAR_USERINFO | CVARDOC_STR);
	name = Cvar_Get ("name", "unnamed", CVAR_USERINFO | CVAR_ARCHIVE | CVARDOC_STR);
	stats_password = Cvar_Get("stats_password", "password", CVAR_PROFILE | CVARDOC_STR);
	Q_strncpyz2(currLoginState.old_password, stats_password->string, sizeof(currLoginState.old_password));
	currLoginState.hashed = false;
	pw_hashed = Cvar_Get("stats_pw_hashed", "0", CVAR_PROFILE | CVARDOC_BOOL);
	/* */
	ValidatePlayerName( name->string, (strlen(name->string)+1) );
	/* */
	skin = Cvar_Get ("skin", "male/grunt", CVAR_USERINFO | CVAR_ARCHIVE | CVARDOC_STR);
	rate = Cvar_Get ("rate", "25000", CVAR_USERINFO | CVAR_ARCHIVE | CVARDOC_INT);	// FIXME
	msg = Cvar_Get ("msg", "1", CVAR_USERINFO | CVAR_ARCHIVE);
	hand = Cvar_Get ("hand", "0", CVAR_USERINFO | CVAR_ARCHIVE);
	fov = Cvar_Get ("fov", "90", CVAR_USERINFO | CVAR_ARCHIVE | CVARDOC_INT);
	Cvar_Describe (fov, "horizontal field of view (in degrees.) Maximum 160.");
	gender = Cvar_Get ("gender", "male", CVAR_USERINFO | CVAR_ARCHIVE | CVARDOC_STR);
	gender_auto = Cvar_Get ("gender_auto", "1", CVAR_ARCHIVE);
	gender->modified = false; // clear this so we know when user sets it manually

	cl_vwep = Cvar_Get ("cl_vwep", "1", CVAR_ARCHIVE | CVARDOC_BOOL);

	cl_master = Cvar_Get ("cl_master", DEFAULT_MASTER_1, CVAR_ARCHIVE | CVARDOC_STR);
	cl_master2 = Cvar_Get ("cl_master2", DEFAULT_MASTER_2, CVAR_ARCHIVE | CVARDOC_STR);	
	if (USE_ALIENARENA_ORG) {
		CL_RepairStringCvar(cl_master, DEFAULT_MASTER_1);
		CL_RepairStringCvar(cl_master2, DEFAULT_MASTER_2);
	}

	// custom huds
	cl_hudimage1 = Cvar_Get("cl_hudimage1", "pics/i_health.tga", CVAR_ARCHIVE | CVARDOC_STR);
	cl_hudimage2 = Cvar_Get("cl_hudimage2", "pics/i_score.tga", CVAR_ARCHIVE | CVARDOC_STR);
	cl_hudimage3 = Cvar_Get("cl_hudimage3", "pics/i_ammo.tga", CVAR_ARCHIVE | CVARDOC_STR);

	// stats server
	cl_stats_server = Cvar_Get("cl_stats_server", DEFAULT_STATS_URL, CVAR_ARCHIVE | CVARDOC_STR);

	// update checker
	cl_latest_game_version_url = Cvar_Get("cl_latest_game_version_server", DEFAULT_VERSION_CHECK_URL, CVAR_ARCHIVE | CVARDOC_STR);	
	if (USE_ALIENARENA_ORG) {
		CL_RepairStringCvar(cl_latest_game_version_url, VERSION_CHECK_ALIENARENA_ORG_URL);
	}

	// throwaway cvars
	Cvar_Get("g_dm_lights", "1", CVAR_ARCHIVE | CVAR_GAMEINFO | CVARDOC_BOOL); //mark this as archived even if game code doesn't run.

	//
	// register our commands
	//
	Cmd_AddCommand ("cmd", CL_ForwardToServer_f);
	Cmd_AddCommand ("pause", CL_Pause_f);
	Cmd_AddCommand ("pingservers", CL_PingServers_f);
	Cmd_AddCommand ("skins", CL_Skins_f);

	Cmd_AddCommand ("userinfo", CL_Userinfo_f);
	Cmd_AddCommand ("snd_restart", CL_Snd_Restart_f);

	Cmd_AddCommand ("changing", CL_Changing_f);
	Cmd_AddCommand ("disconnect", CL_Disconnect_f);
	Cmd_AddCommand ("record", CL_Record_f);
	Cmd_AddCommand ("stop", CL_Stop_f);

	Cmd_AddCommand ("quit", CL_Quit_f);

	Cmd_AddCommand ("connect", CL_Connect_f);
	Cmd_AddCommand ("reconnect", CL_Reconnect_f);

	Cmd_AddCommand ("rcon", CL_Rcon_f);

	Cmd_AddCommand ("setenv", CL_Setenv_f );

	Cmd_AddCommand ("precache", CL_Precache_f);

	Cmd_AddCommand ("download", CL_Download_f);
	Cmd_AddCommand ("downloadhttp", CL_Download_f);
	Cmd_AddCommand ("installmap", CL_InstallMap);

	Cmd_AddCommand ("irc_connect", CL_InitIRC);
	Cmd_AddCommand ("irc_quit", CL_IRCInitiateShutdown);
	Cmd_AddCommand ("irc_say", CL_IRCSay);

	//
	// forward to server commands
	//
	// the only thing this does is allow command completion
	// to work -- all unknown commands are automatically
	// forwarded to the server. It also prevents the commands
	// from being ignored if they are issued in 'forced' mode.
	Cmd_AddCommand ("wave", NULL);
	Cmd_AddCommand ("inven", NULL);
	Cmd_AddCommand ("kill", NULL);
	Cmd_AddCommand ("use", NULL);
	Cmd_AddCommand ("drop", NULL);
	Cmd_AddCommand ("say", NULL);
	Cmd_AddCommand ("say_team", NULL);
	Cmd_AddCommand ("info", NULL);
	Cmd_AddCommand ("prog", NULL);
	Cmd_AddCommand ("give", NULL);
	Cmd_AddCommand ("god", NULL);
	Cmd_AddCommand ("notarget", NULL);
	Cmd_AddCommand ("noclip", NULL);
	Cmd_AddCommand ("invuse", NULL);
	Cmd_AddCommand ("invprevw", NULL);
	Cmd_AddCommand ("invnextw", NULL);
	Cmd_AddCommand ("invprevp", NULL);
	Cmd_AddCommand ("invnextp", NULL);
	Cmd_AddCommand ("invprev", NULL);
	Cmd_AddCommand ("invnext", NULL);
	Cmd_AddCommand ("invdrop", NULL);
	Cmd_AddCommand ("weapnext", NULL);
	Cmd_AddCommand ("weapprev", NULL);
	Cmd_AddCommand ("weaplast", NULL);
	Cmd_AddCommand ("chatbubble", NULL);
	Cmd_AddCommand ("players", NULL);
	Cmd_AddCommand ("score", NULL);
	Cmd_AddCommand ("vote", NULL);
	Cmd_AddCommand ("putaway", NULL);
	Cmd_AddCommand ("playerlist", NULL);

	Cvar_SetValue("scriptsloaded", 0);

	strcpy(map_music, "music/none.wav");

	//register all our menu gfx
	(void)R_RegisterPic("m_main");
	(void)R_RegisterPic("m_options");
	(void)R_RegisterPic("menu_back");
	(void)R_RegisterPic("m_video");
	(void)R_RegisterPic("m_controls");
	(void)R_RegisterPic("m_player");
	(void)R_RegisterPic("m_bots");
	(void)R_RegisterPic("m_startserver");
	(void)R_RegisterPic("m_dmoptions");
	(void)R_RegisterPic("m_mutators");
	(void)R_RegisterPic("m_single");
	(void)R_RegisterPic("m_quit");
	(void)R_RegisterPic("m_main_mont0");
	(void)R_RegisterPic("m_main_mont1");
	(void)R_RegisterPic("m_main_mont2");
	(void)R_RegisterPic("m_main_mont3");
	(void)R_RegisterPic("m_main_mont4");
	(void)R_RegisterPic("m_main_mont5");
	(void)R_RegisterPic("blood_ring");

	remoteserver_runspeed = 300; //default
}



/*
===============
CL_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void CL_WriteConfiguration (void)
{
	FILE	*f;
	char	path[MAX_OSPATH];

	if (cls.state == ca_uninitialized)
		return;

#if defined WIN32_VARIANT
	FS_WriteAPPDATA( path, sizeof(path), "config.cfg");
#else
	FS_FullWritePath( path, sizeof(path), "config.cfg");
#endif	

	f = fopen (path, "w");
	if (!f)
	{
		Com_Printf ("Couldn't write config.cfg.\n");
		return;
	}

	fprintf (f, "// generated by Alien Arena. Use autoexec.cfg for custom settings.\n");
	Key_WriteBindings (f);
	fclose (f);

	Cvar_WriteVariables (path);
}

/*
===============
CL_WriteProfile

Writes profile information to profile.cfg
===============
*/
void CL_WriteProfile (void)
{
	FILE	*f;
	char	path[MAX_OSPATH];

	if (cls.state == ca_uninitialized)
		return;

	if(!currLoginState.hashed)
		return; //We don't ever want to write out non-hashed passwords, period!

#if defined WIN32_VARIANT
	FS_WriteAPPDATA( path, sizeof(path), "profile.cfg");
#else
	FS_FullWritePath( path, sizeof(path), "profile.cfg");
#endif	

	f = fopen (path, "w");
	if (!f)
	{
		Com_Printf ("Couldn't write profile.cfg.\n");
		return;
	}

	fprintf (f, "// generated by Alien Arena.\n");
	fprintf (f, "set stats_password %s\n", stats_password->string);
	fprintf (f, "set stats_pw_hashed %i\n", pw_hashed->integer);
	fclose (f);
}



/*
==================
CL_FixCvarCheats

==================
*/

typedef struct
{
	char	*name;
	char	*value;
	cvar_t	*var;
} cheatvar_t;

cheatvar_t cheatvars[] =
{
	{"timescale",				"1", NULL},
	{"timedemo",				"0", NULL},
	{"r_drawworld",				"1", NULL},
	{"cl_testlights",			"0", NULL},
	{"r_fullbright",			"0", NULL},
	{"r_drawflat",				"0", NULL},
	{"paused",					"0", NULL},
	{"fixedtime",				"0", NULL},
	{"gl_lightmap",				"0", NULL},
	{"gl_showpolys",			"0", NULL},
	{"gl_showtris",				"0", NULL},
	{"gl_showcollisionmesh",	"0", NULL},
	{NULL, NULL, NULL}
};

void CL_FixCvarCheats (void)
{
	static int numcheatvars = 0;
	int			i;
	cheatvar_t	*var;

	if ( !strcmp(cl.configstrings[CS_MAXCLIENTS], "1")
		|| !cl.configstrings[CS_MAXCLIENTS][0]
		|| cl.attractloop )
		return;		// single player or demo playback can cheat

	// find all the cvars if we haven't done it yet
	if (!numcheatvars)
	{
		while (cheatvars[numcheatvars].name)
		{
			cheatvars[numcheatvars].var = Cvar_Get (cheatvars[numcheatvars].name,
					cheatvars[numcheatvars].value, 0);
			numcheatvars++;
		}
	}

	// make sure they are all set to the proper values
	for (i=0, var = cheatvars ; i<numcheatvars ; i++, var++)
	{
		if ( cl.attractloop &&
			(!strcmp( var->name, "timedemo" )
			||
			!strcmp( var->name, "timescale"))
			)
			continue; // allow these when running a .dm2 demo

		if ( strcmp (var->var->string, var->value) )
		{
			Cvar_Set (var->name, var->value);
		}
	}
}

void CL_CheckFlagStatus( void )
{
	if(r_gotFlag)
	{
		//start the new music
		S_StartMusic(map_music_sec); 
	}
	else if(r_lostFlag)
	{
		//start the original map music back up
		S_StartMusic(map_music);
	}
}

//============================================================================

qboolean send_packet_now = false; // instant packets. used during downloads
extern float    r_frametime;      // TODO: move to appropriate .h
extern unsigned sys_frame_time;   // TODO: ditto

/**
 * @brief Top level client-side routine for main loop.
 *
 * @param msec  the time since the previous CL_Frame.
 */
/*
 * Notes on time variables:
 * cls.frametime :
 *  float seconds since last render
 *  clamped for packet processing, now unclamped for rendering
 *  used in client-to-server move message
 *  in CL_AddClEntities(): used in bouncing entities (brass) calculations
 *
 * r_frametime :
 *  unclamped float seconds since last render. Used in particle rendering.
 *
 * cl.time :
 *  critical global timer used in various places.
 *  clamped to [cl.frame.servertime-100, cl.frame.servertime] in CL_ParseFrame()
 *
 * cls.realtime :
 *   set to time at start of frame. Updated anywhere else?
 *
 *  cl.frame.servertime:
 *    equivalent to server frame number times 100msecs. see CL_ParseFrame()
 */

/* Packet Rate Limiting Cap in milliseconds
 *  msecs=PPS :: 12=83, 13=76, 14=71, 15=66, 16=62
 * Current choice is 16msec/62PPS nominal.
 *  This matches the default cl_maxfps setting.
 *  Which is 60, of course, but because of msec rounding, the result is 62
 *  This results in 6 packets per server frame, mostly.
 * Packet rate limiting is not invoked unless the PPS is set higher than the FPS.
 * Seems like a good idea not to invoke packet rate limiting unless the
 *  cl_maxfps is set higher than the default.
 *
 *  PKTRATE_EARLY is the minimum msecs for catching up when packets are delayed
 *
 *  If neither the packet nor the render code will be triggered in the next
 *  YIELD_MSEC milliseconds, sleep the process for a fraction of that time in
 *  order to reduce the CPU usage of the process. Has to be a small fraction,
 *  to allow the OS enough time to switch back to this process.
 */
#define PKTRATE_CAP 16
#define PKTRATE_EARLY 12
#define YIELD_MSEC 4

void CL_Frame( int msec )
{
	// static int lasttimecalled = 0; // TODO: see below, obsolete logging?

	static int frcjitter[4]   = { 0, -1, 0, 1 };
	static int frcjitter_ix   = 0;
	static int framerate_cap  = 0;
	static int packetrate_cap = 0;
	static int packet_timer   = 0;
	static int render_timer   = 0;
	static int render_counter = 0;
	static int packet_delay = 0;
	int render_trigger;
	int packet_trigger;
	static perftest_t *speedometer = NULL;
	static perftest_t *accelerometer = NULL;
	float FRAMETIME = 1.0/(float)server_tickrate;

	if ( dedicated->integer )
	{ // crx running as dedicated server crashes without this.
		return;
	}

	cls.realtime  = curtime; // time at start of Qcommon_Frame()
	cls.frametime = 0.0f;    // zero here for debug purposes, set below
	cl.time += msec; // clamped to [cl.frame.servertime-100,cl.frame.servertime]

	/* local timers for decoupling frame rate from packet rate */
	packet_timer += msec;
	render_timer += msec;
	
    if (!speedometer) {
        speedometer = get_perftest("speedometer");
        if (speedometer) {
            speedometer->is_timerate = false;
            speedometer->cvar = Cvar_Get("cl_showspeedometer", "0", CVAR_ARCHIVE);
            strcpy (speedometer->format, "speed %4.0f u/s");
            speedometer->scale = 1.0f;///12.3f;
        }
    }
    
    if (!accelerometer) {
    	accelerometer = get_perftest("acceleromter");
    	if (accelerometer) {
    		accelerometer->is_timerate = true;
    		accelerometer->cvar = Cvar_Get("cl_showaccelerometer", "0", CVAR_ARCHIVE);
    		strcpy (accelerometer->format, "accel %4.0f u/s/s");
    		accelerometer->scale = 1.0f;
    	}
    }
    

	/*
	 * update maximum FPS.
	 * framerate_cap is in msecs/frame and is user specified.
	 * Note: Quake2World sets a hard lower limit of 30. Not sure if
	 *   that is needed; to be determined if that is a good thing to do.
	 */
	if ( cl_maxfps->modified  )
	{
		if ( cl_maxfps->value < 30.0f )
		{
			Com_Printf("Warning: cl_maxfps set to less than 30.\n");
			if ( cl_maxfps->value < 1.0f )
			{
				Cvar_ForceSet( "cl_maxfps", "1" );
			}
		}
		cl_maxfps->modified = false;
		framerate_cap = 0; // force a recalculation
	}
	if ( framerate_cap < 1 )
	{
		framerate_cap = (int)(ceil( 1000.0f / cl_maxfps->value ));
		if ( framerate_cap < 1 )
		{
			framerate_cap = 1;
		}
		Com_DPrintf("framerate_cap set to %i msec\n", framerate_cap );
	}

	/*
	 * Set nominal milliseconds-per-packet for client-to-server messages.
	 * Idea is to be timely in getting and transmitting player input without
	 *   congesting the server.
	 * Plan is to not implement a user setting for this unless a need for that
	 *   is discovered.
	 * If the cl_maxfps is set low, then FPS will limit PPS, and
	 *  packet rate limiting is bypassed.
	 */
	if ( cls.state == ca_connected )
	{ // receiving configstrings from the server, run at nominal 10PPS
		// avoids unnecessary load on the server
		if ( packetrate_cap != 1000*FRAMETIME )
			Com_DPrintf("packet rate change: 10 PPS\n");
		packetrate_cap = 1000*FRAMETIME;
	}
	else if ( framerate_cap >= PKTRATE_CAP )
	{ // do not to throttle packet sending, run in sync with render
		if ( packetrate_cap != -1)
			Com_DPrintf("packetrate change: framerate\n");
		packetrate_cap = -1;
	}
	else
	{ // packet rate limiting
		if ( packetrate_cap != PKTRATE_CAP )
			Com_DPrintf("packetrate change: %iPPS\n", 1000/PKTRATE_CAP);
		packetrate_cap = PKTRATE_CAP;
	}

	/* local triggers for decoupling framerate from packet rate  */
	render_trigger = 0;
	packet_trigger = 0;

	if ( cl_timedemo->integer == 1 )
	{ /* accumulate timed demo statistics, free run both packet and render */
		/* setting render_trigger to 1 forces timedemo_start to be set if it
		 * hasn't been already. It also forces cl.timedemo_frames to be 
		 * incremented.
		 */
		render_trigger = 1; 
		packet_trigger = 1;
	}
	else
	{ /* normal operation. */
		/* Sometimes, the packetrate_cap can be "in phase" with
		 *  the frame rate affecting the average packets-per-server-frame.
		 *  A little jitter in the framerate_cap counteracts that.
		 */
		if ( render_timer >= (framerate_cap + frcjitter[frcjitter_ix]) )
		{
			if ( ++frcjitter_ix > 3 ) frcjitter_ix = 0;
			render_trigger = 1;
		}
		if ( packetrate_cap == -1 )
		{ // flagged to run same as framerate_cap
			packet_trigger = render_trigger;
		}
		else if ( packet_timer >= packetrate_cap )
		{ // normal packet trigger
			packet_trigger = 1;
		}
		else if ( packet_delay > 0 )
		{ // packet sent in previous loop was delayed
			if ( packet_timer >= PKTRATE_EARLY )
			{ // should be ok to send a packet early
				/* idea is to try to maintain a steady number of
				 * client-to-server packets per server frame.
				 * If render is triggered, it is good to poll input and
				 *  send a packet to avoid more delay.
				 * If adding the delay to the timer reaches the cap then
				 *  try to catch up
				 * Otherwise, do nothing, the next loop should occur soon.
				 */
				if ( render_trigger
					|| ((packet_timer + packet_delay) >= packetrate_cap) )
				{
					packet_trigger = 1;
				}
			}
		}
	}
	
#ifdef UNIX_VARIANT
	// TODO: make this work on Windows
	// TODO: disable this if vsync is on
	if (	!packet_trigger && !send_packet_now && !cls.download &&
			!render_trigger &&
			packet_timer + packet_delay >= YIELD_MSEC &&
			render_timer >= YIELD_MSEC )
	{
		usleep ((YIELD_MSEC * 1000) / 6);
		return;
	}
#endif

	if ( packet_trigger || send_packet_now || cls.download)
	{
		send_packet_now = false; // used during downloads

		if ( packetrate_cap > 0 && packet_timer > packetrate_cap )
		{ // difference between cap and timer, a delayed packet
			packet_delay = packet_timer - packetrate_cap;
		}
		else
		{
			packet_delay = 0;
		}

		render_counter = 0; // for counting renders since last packet

		/* let the mouse activate or deactivate */
		IN_Frame();

		/*
		 * calculate frametime in seconds for packet procedures
		 * cls.frametime is source for the cmd.msecs byte
		 *  in the client-to-server move message.
		 */
		cls.frametime  = ((float)packet_timer) / 1000.0f;
		if ( cls.frametime >= 0.250f )
		{ /* very long delay */
			/*
			 * server checks for cmd.msecs to be <= 250
			 */
			Com_DPrintf("CL_Frame(): cls.frametime clamped from %0.8f to 0.24999\n",
					cls.frametime );
			cls.frametime = 0.24999f ;
			/*
			 * try to throttle the video frame rate by overriding the
			 * render trigger.
			 */
			render_trigger = false;
			render_timer = 0;
		}

		/* process server-to-client packets */
		CL_ReadPackets();

		/* execute pending commands */
		Cbuf_Execute();

		/* run cURL downloads */
		CL_HttpDownloadThink();

		/* 
		 * system dependent keyboard and mouse input event polling
		 * accumulate keyboard and mouse events
		 */
		Sys_SendKeyEvents();

		/* joystick input. may or may not be working. */
		IN_Commands();

		/* execute pending commands */
		Cbuf_Execute();

		/*
		 * send client commands to server
		 * these are construced from accumulated keyboard and mouse events,
		 * which are then reset
		 */
		CL_SendCmd();

		/* clear various cvars unless single player */
		CL_FixCvarCheats ();

		/* resend a connection request if necessary */
		CL_CheckForResend();

		/* retrigger packet send timer */
		packet_timer = 0;

		/*
		 * predict movement for un-acked client-to-server packets
		 * [The Quake trick that keeps players view smooth in on-line play.]
		 */
		CL_PredictMovement (packet_timer);
		
		if (speedometer && speedometer->cvar->integer) {
	        speedometer->counter = sqrt(
	            cl.predicted_velocity[0]*cl.predicted_velocity[0]+
	            cl.predicted_velocity[1]*cl.predicted_velocity[1]
	        );
	        if (speedometer->counter > cl_speedrecord->value) {
	        	Cvar_SetValue ("cl_speedrecord", speedometer->counter);
	        	if (speedometer->counter > cl_alltimespeedrecord->value) 
	        		Cvar_SetValue ("cl_alltimespeedrecord", speedometer->counter);
	        }
	    }
	    
	    if (accelerometer && accelerometer->cvar->integer) {
	    	static float old_vel;
	    	float new_vel;
	    	new_vel = sqrt(
	            cl.predicted_velocity[0]*cl.predicted_velocity[0]+
	            cl.predicted_velocity[1]*cl.predicted_velocity[1]
	        );
	        accelerometer->counter += new_vel-old_vel;
	        old_vel = new_vel;
	    }
	}

	/*
	 * refresh can occur on different frames than client-to-server messages.
	 *  when packet rate limiting is in effect
	 */
	if ( render_trigger )
	{
		++render_counter; // counting renders since last packet
		
		if (!packet_trigger && cl_test->integer) //return cl_test - this was causing major issues with menu mouse in windows build
		{
			/* 
			 * system dependent keyboard and mouse input event polling
			 * accumulate keyboard and mouse events
			 */
			cls.frametime  = ((float)packet_timer) / 1000.0f;
			Sys_SendKeyEvents();
			
			CL_PredictMovement (packet_timer);
		}

		/*
		 * calculate cls.frametime in seconds for render procedures.
		 *
		 * May not need to clamp for rendering.
		 * Only would affect things if framerate went below 4 FPS.
		 *
		 * Using a simple lowpass filter, to smooth out irregular timing.
		 */
		cls.frametime = (float)(render_timer) / 1000.0f;
		r_frametime = (r_frametime + cls.frametime + cls.frametime) / 3.0f;
		cls.frametime = r_frametime;

		//  Update the display
		VID_CheckChanges();
		if ( !cl.refresh_prepped && cls.state == ca_active )
		{ // re-initialize video configuration
			CL_PrepRefresh();
		}
		else
		{ // regular screen update
			if ( host_speeds->integer )
				time_before_ref = Sys_Milliseconds(); // TODO: obsolete test?
			SCR_UpdateScreen();
			if ( host_speeds->integer )
				time_after_ref = Sys_Milliseconds();
		}

		// check for flag and update music src if possesed or lost
		CL_CheckFlagStatus();

		// update audio.
		S_Update( cl.refdef.vieworg, cl.v_forward, cl.v_right, cl.v_up );

		// advance local effects for next frame
		CL_RunDLights();
		CL_RunLightStyles();
		SCR_RunConsole ();

		++cls.framecount; // does not appear to be used anywhere

		/* developer test for observing timers */
		// Com_DPrintf("rt: %i cft: %f\n", render_timer, cls.frametime );

		// retrigger render timing
		render_timer = 0;

#if 0
		/* TODO: Check if this still works and/or is useful.
		 */
		if ( log_stats->value )
		{
			if ( cls.state == ca_active )
			{
				if ( !lasttimecalled )
				{
					lasttimecalled = Sys_Milliseconds();
					if ( log_stats_file )
						fprintf( log_stats_file, "0\n" );
				}
				else
				{
					int now = Sys_Milliseconds();

					if ( log_stats_file )
						fprintf( log_stats_file, "%d\n", now - lasttimecalled );
					lasttimecalled = now;
				}
			}
		}
#endif

	}

}

//============================================================================

/*
====================
CL_Init
====================
*/
void CL_Init (void)
{
	if (dedicated->integer)
		return;		// nothing running on the client

	// Initialise fonts
	CL_gameFont = &_CL_gameFont;
	FNT_AutoInit( CL_gameFont , "freesans" , 0 , 65 , 8 , 48 );
	CL_gameFont->faceVar = Cvar_Get( "fnt_game" , "orbitron" , CVAR_ARCHIVE );
	CL_gameFont->sizeVar = Cvar_Get( "fnt_game_size" , "0" , CVAR_ARCHIVE );
	FNT_AutoRegister( CL_gameFont );

	CL_centerFont = &_CL_centerFont;
	FNT_AutoInit( CL_centerFont , "freesans" , 0 , 45 , 16 , 64 );
	CL_centerFont->faceVar = CL_gameFont->faceVar;
	CL_centerFont->sizeVar = Cvar_Get( "fnt_center_size" , "0" , CVAR_ARCHIVE );
	FNT_AutoRegister( CL_centerFont );

	CL_consoleFont = &_CL_consoleFont;
	FNT_AutoInit( CL_consoleFont , "freesans" , 0 , 52 , 8 , 48 );
	CL_consoleFont->faceVar = Cvar_Get( "fnt_console" , "freemono" , CVAR_ARCHIVE );
	CL_consoleFont->sizeVar = Cvar_Get( "fnt_console_size" , "0" , CVAR_ARCHIVE );
	FNT_AutoRegister( CL_consoleFont );
	
	CL_menuFont = &_CL_menuFont;
	FNT_AutoInit( CL_menuFont , "freesans" , 0 , 48 , 8 , 48 );
	CL_menuFont->faceVar = Cvar_Get( "fnt_menu" , "freesans" , CVAR_ARCHIVE );
	CL_menuFont->sizeVar = Cvar_Get( "fnt_menu_size" , "0" , CVAR_ARCHIVE );
	FNT_AutoRegister( CL_menuFont );

	// all archived variables will now be loaded

	CON_Initialise( );

	VID_Init ();
	S_Init ();

	V_Init ();

	net_message.data = net_message_buffer;
	net_message.maxsize = sizeof(net_message_buffer);

	M_Init ();

	SCR_Init ();
	cls.disable_screen = true;	// don't draw yet

	CL_InitLocal ();
	IN_Init ();

	FS_ExecAutoexec (); // add commands from autoexec.cfg
	Cbuf_Execute ();

	if ( name && name->string[0] )
	{
		ValidatePlayerName( name->string, (strlen(name->string)+1) );
	}

	// init Steam data
	STATS_ST_Init();
}


/*
===============
CL_Shutdown

FIXME: this is a callback from Sys_Quit and Com_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void CL_Shutdown(void)
{
	static qboolean isdown = false;

	if (isdown)
	{
		printf ("recursive shutdown\n");
		return;
	}
	isdown = true;

	if (STATS_ENABLED) {
		STATS_Logout();
	}

	STATS_ST_Write();

	CL_IRCInitiateShutdown();
	CL_ShutdownHttpDownload();
	CL_WriteConfiguration ();
	CL_WriteProfile();

	S_Shutdown();
	IN_Shutdown ();
	VID_Shutdown();
	CL_IRCWaitShutdown( );

	NET_Shutdown();

	RS_FreeAllScripts();
}


