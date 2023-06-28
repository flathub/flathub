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

master_sv_t	master_status[MAX_MASTERS];	// status of master servers

client_t	*sv_client;			// current client

cvar_t	*sv_paused;
cvar_t	*sv_timedemo;

cvar_t	*sv_enforcetime;

cvar_t	*timeout;				// seconds without any message
cvar_t	*zombietime;			// seconds to sink messages after disconnect

cvar_t	*rcon_password;			// password for remote server commands

cvar_t	*allow_download;
cvar_t	*allow_download_players;
cvar_t	*allow_download_models;
cvar_t	*allow_download_sounds;
cvar_t	*allow_download_maps;
cvar_t	*allow_overwrite_maps;

cvar_t	*sv_airaccelerate;

cvar_t	*sv_joustmode;
cvar_t	*sv_tactical;
cvar_t  *sv_excessive;

cvar_t	*sv_noreload;			// don't reload level state when reentering

cvar_t	*maxclients;			// FIXME: rename sv_maxclients
cvar_t	*sv_showclamp;

cvar_t	*hostname;
cvar_t	*public_server;			// should heartbeats be sent

cvar_t	*sv_reconnect_limit;	// minimum seconds between connect messages

cvar_t	*sv_ratelimit_status;   //new security measures
cvar_t	*sv_iplimit;

cvar_t	*sv_downloadurl;

cvar_t	*sv_iplogfile;			// Log file by IP address

cvar_t  *sv_tickrate;			//server frame rate

int		sv_numbots;

void Master_Shutdown (void);

short   ShortSwap (short l);

//============================================================================


/*
=====================
SV_LogEvent

Logs an event to the IP log.
=====================
*/
static void SV_LogEvent( netadr_t address , const char * event , const char * name )
{
	FILE * file;

	if ( !( sv_iplogfile && sv_iplogfile->string[0] ) )
		return;


	file = fopen( sv_iplogfile->string , "a" );
	if ( !file ) {
		Com_DPrintf( "Failed to write to IP log file '%s'\n" , sv_iplogfile->string );
		return;
	}
	fprintf( file , "%s\t%s\t%d\t%s\r\n" , NET_AdrToString(address) , event ,
			( name != NULL ) , ( name != NULL ) ? name : "" );
	fclose( file );
}




/*
=====================
SV_DropClient

Called when the player is totally leaving the server, either willingly
or unwillingly.  This is NOT called if the entire server is quiting
or crashing.
=====================
*/
void SV_DropClient (client_t *drop)
{
	// add the disconnect

	MSG_WriteByte (&drop->netchan.message, svc_disconnect);

	if (drop->state == cs_spawned)
	{
		// call the prog function for removing a client
		// this will remove the body, among other things
		ge->ClientDisconnect (drop->edict);
	}

	if (drop->download)
	{
		FS_FreeFile (drop->download);
		drop->download = NULL;
	}

	SV_LogEvent( drop->netchan.remote_address , "DCN" , drop->name );
	drop->state = cs_zombie;		// become free in a few seconds
	drop->name[0] = 0;
}



/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

/*
===============
SV_StatusString

Builds the string that is sent as heartbeats and status replies
===============
*/
char *SV_StatusString (void)
{
	static char status[MAX_MSGLEN - 16]; // static buffer for the status string
	qboolean msg_overflow = false;

	char      player[MAX_INFO_STRING];
	client_t *cl;
	size_t    count;

	// server info string. MAX_INFO_STRING is always < sizeof(status)
	strcpy( status, Cvar_Serverinfo() );
	strcat( status, "\n" );

	// real player score info
	for ( cl=svs.clients, count=maxclients->integer ; count-- ; cl++ )
	{
		if (cl->state == cs_connected || cl->state == cs_spawned )
		{
			/* send score of 0 for spectators and players not yet in game
			 *  statistics program uses this data when polling for scores
			 */
			int cl_score = 0;
			if ( cl->state == cs_spawned
					&& cl->edict->client->ps.stats[STAT_SPECTATOR] == 0 )
			{
				cl_score = cl->edict->client->ps.stats[STAT_FRAGS];;
			}
			/*
			 * send color characters
			 * do not send actual ip addresses for security/privacy reasons
			 */
			Com_sprintf( player, sizeof(player),
					"%i %i \"%s\" \"127.0.0.1\" %i\n",
					cl_score, cl->ping, cl->name, cl->edict->dmteam);
			if ( (strlen(status) + strlen(player) + 1) < sizeof(status) )
			{
				strncat( status, player, strlen(player) );
			}
			else
			{
				msg_overflow = true;
				break;
			}
		}
	}

	// bot score info
	if ( !msg_overflow )
	{
		for ( cl=svs.clients, count=maxclients->integer ; count-- ; cl++ )
		{
			size_t bot_count;
			bot_count = cl->edict->client->ps.botnum;
			if ( bot_count )
			{ // normally, first client contains the bot names and scores
				bot_t* ps_bot;
				for ( ps_bot = cl->edict->client->ps.bots ; bot_count-- ; ps_bot++ )
				{
					int bot_score = ps_bot->score;
					Com_sprintf( player, sizeof(player),
							"%i %i \"%s\" \"127.0.0.1\" %i\n",
							bot_score,
							0, // bot ping
							ps_bot->name, ps_bot->dmteam);
					if ( (strlen(status) + strlen(player) + 1) < sizeof(status) )
					{
						strncat( status, player, strlen(player) );
					}
					else
					{
						msg_overflow = true;
						break;
					}
				}
			}
			break;
		}
	}
	if ( msg_overflow )
	{
		Com_DPrintf("SV_StatusString overflowed\n");
	}

	return status;
}

/*
================
SVC_Status

Responds with all the info that qplug or qspy can see
================
*/
/*void SVC_Status (void)
{
	Netchan_OutOfBandPrint (NS_SERVER, net_from, "print\n%s", SV_StatusString());
#if 0
	Com_BeginRedirect (RD_PACKET, sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);
	Com_Printf (SV_StatusString());
	Com_EndRedirect ();
#endif
}*/
static qboolean RateLimited (ratelimit_t *limit, int maxCount)
{
	int diff;

	diff = sv.time - limit->time;

	//a new sampling period
	if (diff > limit->period || diff < 0)
	{
		limit->time = sv.time;
		limit->count = 0;
	}
	else
	{
		if (limit->count >= maxCount)
			return true;
	}

	return false;
}

static void RateSample (ratelimit_t *limit)
{
	int diff;

	diff = sv.time - limit->time;

	//a new sampling period
	if (diff > limit->period || diff < 0)
	{
		limit->time = sv.time;
		limit->count = 1;
	}
	else
	{
		limit->count++;
	}
}

static void SVC_Status (void)
{

	RateSample (&svs.ratelimit_status);

	if (RateLimited (&svs.ratelimit_status, sv_ratelimit_status->integer))
	{
		Com_DPrintf ("SVC_Status: Dropped status request from %s\n", NET_AdrToString (net_from));
		return;
	}

	Netchan_OutOfBandPrint (NS_SERVER, net_from, "print\n%s", SV_StatusString());
}

/*
================
SVC_Ack

================
*/
void SVC_Ack (void)
{
	int		i;
	Com_Printf ("Ping acknowledge from %s\n", NET_AdrToString(net_from));
	for ( i = 0 ; i < MAX_MASTERS ; i ++ ) {
		if ( master_status[i].name[0] == 0 )
			break;

		if ( master_status[i].addr.port == 0 )
			continue;

		if ( NET_CompareAdr (master_status[i].addr, net_from) )
			master_status[i].last_ping_ack = 2;
	}
}

/*
================
SVC_Info

Responds with short info for broadcast scans
The second parameter should be the current protocol version number.
================
*/
void SVC_Info (void)
{
	char	string[64];
	int		i, count;
	int		version;
	client_t	*cl;

	if (maxclients->integer == 1)
		return;		// ignore in single player

	version = atoi (Cmd_Argv(1));

	if (version != PROTOCOL_VERSION) {
		Com_sprintf (string, sizeof(string), "%s: wrong version\n", hostname->string, sizeof(string));
		//r1: return instead of sending another packet. prevents spoofed udp packet
		//    causing server <-> server info loops.
		return;
	}
	else
	{
		count = 0;
		for (i=0 ; i<maxclients->integer ; i++)
			if (svs.clients[i].state >= cs_connected)
				count++;

		//bot score info
		for (i=0 ; i<maxclients->integer ; i++)
		{
			cl = &svs.clients[i];
			if(cl->edict->client->ps.botnum > 0)
			{
				count += cl->edict->client->ps.botnum; //add the bots
				break;
			}
		}
		//end bot score info

		Com_sprintf (string, sizeof(string), "%16s %8s %2i/%2i\n", hostname->string, sv.name, count, maxclients->integer);
	}

	Netchan_OutOfBandPrint (NS_SERVER, net_from, "info\n%s", string);
}

/*
================
SVC_Ping

Just responds with an acknowledgement
================
*/
void SVC_Ping (void)
{
	Netchan_OutOfBandPrint (NS_SERVER, net_from, "ack");
}

void SV_KickClient (client_t *cl, const char /*@null@*/*reason, const char /*@null@*/*cprintf)
{
	if (reason && cl->state == cs_spawned && cl->name[0])
		SV_BroadcastPrintf (PRINT_HIGH, "%s was dropped: %s\n", cl->name, reason);
	if (cprintf)
		SV_ClientPrintf (cl, PRINT_HIGH, "%s", cprintf);
	Com_Printf ("Dropping %s, %s.\n", cl->name, reason ? reason : "SV_KickClient");
	SV_DropClient (cl);
}

/*
=================
SVC_GetChallenge

Returns a challenge number that can be used
in a subsequent client_connect command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.
=================
*/
void SVC_GetChallenge (void)
{
	int		i;
	int		oldest;
	int		oldestTime;

	oldest = 0;
	oldestTime = 0x7fffffff;

	// see if we already have a challenge for this ip
	for (i = 0 ; i < MAX_CHALLENGES ; i++)
	{
		if (NET_CompareBaseAdr (net_from, svs.challenges[i].adr))
			break;
		if (svs.challenges[i].time < oldestTime)
		{
			oldestTime = svs.challenges[i].time;
			oldest = i;
		}
	}

	if (i == MAX_CHALLENGES)
	{
		// overwrite the oldest
		svs.challenges[oldest].challenge = rand() & 0x7fff;
		svs.challenges[oldest].adr = net_from;
		svs.challenges[oldest].time = curtime;
		i = oldest;
	}

	// send it back
	Netchan_OutOfBandPrint (NS_SERVER, net_from, "challenge %i", svs.challenges[i].challenge);
}

/*
==================
SVC_DirectConnect

A connection request that did not come from the master
==================
*/
void SVC_DirectConnect (void)
{
	char		userinfo[MAX_INFO_STRING];
	netadr_t	adr;
	int			i;
	client_t	*cl, *newcl;
	client_t	temp;
	edict_t		*ent;
	int			edictnum;
	int			version;
	int			qport;
	int			challenge;
	int			previousclients;
	int			botnum, botkick;

	adr = net_from;

	Com_DPrintf ("SVC_DirectConnect ()\n");

	version = atoi(Cmd_Argv(1));
	if (version != PROTOCOL_VERSION)
	{
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nServer is protocol version %i\n", PROTOCOL_VERSION);
		Com_DPrintf ("    rejected connect from protocol version %i\n", version);
		SV_LogEvent( adr , "RVR" , NULL );
		return;
	}

	qport = atoi(Cmd_Argv(2));

	challenge = atoi(Cmd_Argv(3));

	//security, overflow fixes

	//limit connections from a single IP
	previousclients = 0;
	for (i=0,cl=svs.clients ; i<maxclients->integer ; i++,cl++)
	{
		if (cl->state == cs_free)
			continue;
		if (NET_CompareBaseAdr (adr, cl->netchan.remote_address))
		{
			//zombies are less dangerous
			if (cl->state == cs_zombie)
				previousclients++;
			else
				previousclients += 2;
		}
	}

	if (previousclients >= sv_iplimit->integer * 2)
	{
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nToo many connections from your host.\n");
		Com_DPrintf ("    too many connections\n");
		SV_LogEvent( adr , "R00" , NULL );
		return;
	}

	// sku - reserve 32 bytes for the IP address
	strncpy (userinfo, Cmd_Argv(4), sizeof(userinfo)-32);
	userinfo[sizeof(userinfo) - 32] = 0;

	//check it is not overflowed, save enough bytes for /ip/111.222.333.444:55555
	if (strlen(userinfo) + 25 >= sizeof(userinfo)-1)
	{
		Com_DPrintf ("    userinfo length exceeded\n");
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nUserinfo string length exceeded.\n");
		SV_LogEvent( adr , "R01" , NULL );
		return;
	}
	else if (!userinfo[0])
	{
		Com_DPrintf ("    empty userinfo string\n");
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nBad userinfo string.\n");
		SV_LogEvent( adr , "R02" , NULL );
		return;
	}

	//block anyone trying to use the end-of-message-in-string exploit
	if (strchr(userinfo, '\xFF'))
	{
		char		*ptr;
		ptr = strchr (userinfo, '\xFF');
		ptr -= 8;
		if (ptr < userinfo)
			ptr = userinfo;
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nConnection refused due to attempted exploit!\n");
		SV_LogEvent( adr , "R03" , NULL );
		return;
	}
	if (Info_KeyExists (userinfo, "ip"))
	{
		char	*p;
		p = Info_ValueForKey(userinfo, "ip");
		Com_Printf ("EXPLOIT: Client %s attempted to spoof IP address: %s\n", Info_ValueForKey (userinfo, "name"), NET_AdrToString(adr));
		SV_LogEvent( adr , "R04" , NULL );
		return;
	}

	// force the IP key/value pair so the game can filter based on ip
	Info_SetValueForKey (userinfo, "ip", NET_AdrToString(net_from));

	// attractloop servers are ONLY for local clients
	if (sv.attractloop)
	{
		if (!NET_IsLocalAddress (adr))
		{
			Com_Printf ("Remote connect in attract loop.  Ignored.\n");
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nConnection refused.\n");
			SV_LogEvent( adr , "R05" , NULL );
			return;
		}
	}

	// see if the challenge is valid
	if (!NET_IsLocalAddress (adr))
	{
		for (i=0 ; i<MAX_CHALLENGES ; i++)
		{
			if (NET_CompareBaseAdr (net_from, svs.challenges[i].adr))
			{
				if (challenge == svs.challenges[i].challenge)
					break;		// good
				Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nBad challenge.\n");
				SV_LogEvent( adr , "R06" , NULL );
				return;
			}
		}
		if (i == MAX_CHALLENGES)
		{
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nNo challenge for address.\n");
			SV_LogEvent( adr , "R07" , NULL );
			return;
		}
	}

	newcl = &temp;
	memset (newcl, 0, sizeof(client_t));

	// if there is already a slot for this ip, reuse it
	for (i=0,cl=svs.clients ; i<maxclients->integer ; i++,cl++)
	{
		if (cl->state == cs_free)
			continue;

		if (NET_CompareAdr (adr, cl->netchan.remote_address))
		{

			//r1: !! fix nasty bug where non-disconnected clients (from dropped disconnect
			//packets) could be overwritten!
			if (cl->state != cs_zombie)
			{
				Com_DPrintf ("    client already found\n");
				Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nPlayer '%s' is already connected from %s.\n", cl->name, NET_AdrToString(adr));
				SV_LogEvent( adr , "R08" , cl->name );
				return;
			}

			if (!NET_IsLocalAddress (adr) && (svs.realtime - cl->lastconnect) < ((int)sv_reconnect_limit->integer * 1000))
			{
				Com_DPrintf ("%s:reconnect rejected : too soon\n", NET_AdrToString (adr));
				SV_LogEvent( adr , "R09" , NULL );
				return;
			}
			Com_Printf ("%s:reconnect\n", NET_AdrToString (adr));
			SV_LogEvent( adr , "RCN" , NULL );

			newcl = cl;
			goto gotnewcl;
		}
	}

	// find a client slot

	//get number of bots
	for (i=botnum=0 ; i<maxclients->integer ; i++)
	{
		cl = &svs.clients[i];
		botnum = cl->edict->client->ps.botnum;
		if(botnum > 0)
			break;
	}

	//still need to reserve one slot
	newcl = NULL;

	//are we using botkickthreshold?
	botkick = Cvar_VariableValue("sv_botkickthreshold");

	//prevent client slot overwrites with bots rejoining after map change
	if(botkick) {

		if(botkick < sv_numbots)
			botnum = botkick;
		else
			botnum = sv_numbots;
	}

	for (i=0,cl=svs.clients ; i<maxclients->integer-botnum; i++,cl++)
	{
		if (cl->state == cs_free)
		{
			newcl = cl;
			break;
		}
	}
	if (!newcl)
	{
		Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nServer is full.\n");
		Com_DPrintf ("Rejected a connection.\n");
		SV_LogEvent( adr , "R10" , NULL );
		return;
	}
	SV_LogEvent( adr , "NCN" , NULL );

gotnewcl:

	// build a new connection
	// accept the new client
	// this is the only place a client_t is ever initialized
	*newcl = temp;
	sv_client = newcl;
	edictnum = (newcl-svs.clients)+1;
	ent = EDICT_NUM(edictnum);
	newcl->edict = ent;
	newcl->challenge = challenge; // save challenge for checksumming

	// get the game a chance to reject this connection or modify the userinfo

	if (!(ge->ClientConnect (ent, userinfo)))
	{

		if (*Info_ValueForKey (userinfo, "rejmsg"))
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\n%s\nConnection refused.\n",
				Info_ValueForKey (userinfo, "rejmsg"));
		else
			Netchan_OutOfBandPrint (NS_SERVER, adr, "print\nConnection refused.\n" );
		SV_LogEvent( adr , "GRJ" , NULL );
		Com_DPrintf ("Game rejected a connection.\n");
		return;
	}

	// parse some info from the info strings
	Q_strncpyz2( newcl->userinfo, userinfo, sizeof(newcl->userinfo) );
	SV_UserinfoChanged (newcl);
	SV_LogEvent( adr , "UUS" , newcl->name );

	// send the connect packet to the client
	Netchan_OutOfBandPrint(NS_SERVER, adr, "client_connect %s", sv_downloadurl->string);

	Netchan_Setup (NS_SERVER, &newcl->netchan , adr, qport);

	newcl->state = cs_connected;

	SZ_Init (&newcl->datagram, newcl->datagram_buf, sizeof(newcl->datagram_buf) );
	SZ_SetName (&newcl->datagram, va("Datagram buffer %s", NET_AdrToString(adr)), true);

	newcl->datagram.allowoverflow = true;
	newcl->lastmessage = svs.realtime;	// don't timeout
	newcl->lastconnect = svs.realtime;
	
	ge->ForceExitIntermission ();
	Cbuf_Execute ();
}

int Rcon_Validate (void)
{
	if (!strlen (rcon_password->string))
		return 0;

	if (strcmp (Cmd_Argv(1), rcon_password->string) )
		return 0;

	return 1;
}

/*
===============
SVC_RemoteCommand

A client issued an rcon command.
Shift down the remaining args
Redirect all printfs
===============
*/
void SVC_RemoteCommand (void)
{
	int		i;
	char	remaining[1024];

	i = Rcon_Validate ();

	if (i == 0)
		Com_Printf ("Bad rcon from %s:\n%s\n", NET_AdrToString (net_from), net_message.data+4);
	else
		Com_Printf ("Rcon from %s:\n%s\n", NET_AdrToString (net_from), net_message.data+4);
	
	Cvar_Set ("this_rcon_address", NET_AdrToString (net_from)); 

	Com_BeginRedirect (RD_PACKET, sv_outputbuf, SV_OUTPUTBUF_LENGTH, SV_FlushRedirect);

	if (!Rcon_Validate ())
	{
		Com_Printf ("Bad rcon_password.\n");
	}
	else
	{
		remaining[0] = 0;

		for (i=2 ; i<Cmd_Argc() ; i++)
		{
			/* If spaces present in args, quote them in the remaining string */
			if(strchr(Cmd_Argv(i), ' '))
			{
				strcat (remaining, "\"");
				strcat (remaining, Cmd_Argv(i) );
				strcat (remaining, "\"");
			}
			else
			{
				strcat (remaining, Cmd_Argv(i) );
			}
			strcat (remaining, " ");
		}
		if ( strncmp( remaining, "killserver", 10 ) )
		{ // not too surprising, rcon killserver does bad things
			Cmd_ExecuteString (remaining);
		}
		else
		{
			Com_Printf("Cannot do that.\n");
		}
	}

	Com_EndRedirect ();
}

/*
=================
SV_ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
void SV_ConnectionlessPacket (void)
{
	char	*s;
	char	*c;

	//r1: make sure we never talk to ourselves
	if (NET_IsLocalAddress (net_from) && !NET_IsLocalHost(&net_from) && ShortSwap(net_from.port) == server_port)
	{
		Com_DPrintf ("dropped %d byte connectionless packet from self! (spoofing attack?)\n", net_message.cursize);
		return;
	}


	MSG_BeginReading (&net_message);
	MSG_ReadLong (&net_message);		// skip the -1 marker

	s = MSG_ReadStringLine (&net_message);

	Cmd_TokenizeString (s, false);

	c = Cmd_Argv(0);
	Com_DPrintf ("Packet %s : %s\n", NET_AdrToString(net_from), c);

	if (!strcmp(c, "ping"))
		SVC_Ping ();
	else if (!strcmp(c, "ack"))
		SVC_Ack ();
	else if (!strcmp(c,"status"))
		SVC_Status ();
	else if (!strcmp(c,"info"))
		SVC_Info ();
	else if (!strcmp(c,"getchallenge"))
		SVC_GetChallenge ();
	else if (!strcmp(c,"connect"))
		SVC_DirectConnect ();
	else if (!strcmp(c, "rcon"))
		SVC_RemoteCommand ();
	else if (!strcmp(c, "teamgame")) {
		Netchan_OutOfBandPrint (NS_SERVER, net_from, "teamgame %f", Cvar_VariableValue ("g_teamgame"));
	} 
	else if (!strcmp(c, "tickrate")) {
		Netchan_OutOfBandPrint (NS_SERVER, net_from, "tickrate %f", Cvar_VariableValue ("sv_tickrate"));
	} 
	else
		Com_Printf ("bad connectionless packet from %s:\n%s\n"
		, NET_AdrToString (net_from), s);
}


//============================================================================

/*
===================
SV_CalcPings

Updates the cl->ping variables
===================
*/
void SV_CalcPings (void)
{
	int			i, j;
	client_t	*cl;
	int			total, count;

	for (i=0 ; i<maxclients->integer ; i++)
	{
		cl = &svs.clients[i];
		if (cl->state != cs_spawned )
			continue;

#if 0
		if (cl->lastframe > 0)
			cl->frame_latency[sv.framenum&(LATENCY_COUNTS-1)] = sv.framenum - cl->lastframe + 1;
		else
			cl->frame_latency[sv.framenum&(LATENCY_COUNTS-1)] = 0;
#endif

		total = 0;
		count = 0;
		for (j=0 ; j<LATENCY_COUNTS ; j++)
		{
			if (cl->frame_latency[j] > 0)
			{
				count++;
				total += cl->frame_latency[j];
			}
		}
		if (!count)
			cl->ping = 0;
		else
#if 0
			cl->ping = total*100/count - 100;
#else
			cl->ping = total / count;
#endif

		// let the game dll know about the ping
		cl->edict->client->ping = cl->ping;
	}
}


/*
===================
SV_GiveMsec

Every few frames, gives all clients an allotment of milliseconds
for their command moves.  If they exceed it, assume cheating.
===================
*/
void SV_GiveMsec (void)
{
	int			i;
	client_t	*cl;

	if (sv.framenum & 15)
		return;

	for (i=0 ; i<maxclients->integer ; i++)
	{
		cl = &svs.clients[i];
		if (cl->state == cs_free )
			continue;

		cl->commandMsec = 1800;		// 1600 + some slop
	}
}


/*
=================
SV_ReadPackets
=================
*/
extern int sys_msec_as_of_packet_read;
void SV_ReadPackets (void)
{
	int			i;
	client_t	*cl;
	int			qport;
	sys_msec_as_of_packet_read = Sys_Milliseconds ();

	while (NET_GetPacket (NS_SERVER, &net_from, &net_message))
	{
		// check for connectionless packet (0xffffffff) first
		if (*(int *)net_message.data == -1)
		{
			SV_ConnectionlessPacket ();
			continue;
		}

		// read the qport out of the message so we can fix up
		// stupid address translating routers
		MSG_BeginReading (&net_message);
		MSG_ReadLong (&net_message);		// sequence number
		MSG_ReadLong (&net_message);		// sequence number
		qport = MSG_ReadShort (&net_message) & 0xffff;

		// check for packets from connected clients
		for (i=0, cl=svs.clients ; i<maxclients->integer ; i++,cl++)
		{
			if (cl->state == cs_free)
				continue;

			if (!NET_CompareBaseAdr (net_from, cl->netchan.remote_address))
				continue;
			if (cl->netchan.qport != qport)
				continue;
			if (cl->netchan.remote_address.port != net_from.port)
			{
				Com_Printf ("SV_ReadPackets: fixing up a translated port\n");
				cl->netchan.remote_address.port = net_from.port;
			}

			if (Netchan_Process(&cl->netchan, &net_message))
			{	// this is a valid, sequenced packet, so process it
				if (cl->state != cs_zombie)
				{
					cl->lastmessage = svs.realtime;	// don't timeout
					SV_ExecuteClientMessage (cl);
				}
			}
			break;
		}

		if (i != maxclients->integer)
			continue;
	}
}

/*
==================
SV_CheckTimeouts

If a packet has not been received from a client for timeout->value
seconds, drop the conneciton.  Server frames are used instead of
realtime to avoid dropping the local client while debugging.

When a client is normally dropped, the client_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
void SV_CheckTimeouts (void)
{
	int		i;
	client_t	*cl;
	int			droppoint;
	int			zombiepoint;

	droppoint = svs.realtime - 1000*timeout->value;
	zombiepoint = svs.realtime - 1000*zombietime->value;

	for (i=0,cl=svs.clients ; i<maxclients->integer ; i++,cl++)
	{


		// message times may be wrong across a changelevel
		if (cl->lastmessage > svs.realtime)
			cl->lastmessage = svs.realtime;

		if (cl->state == cs_zombie
		&& cl->lastmessage < zombiepoint)
		{
			cl->state = cs_free;	// can now be reused
			continue;
		}
		if ( (cl->state == cs_connected || cl->state == cs_spawned)
			&& cl->lastmessage < droppoint)
		{
			SV_BroadcastPrintf (PRINT_HIGH, "%s timed out\n", cl->name);
			SV_DropClient (cl);
			cl->state = cs_free;	// don't bother with zombie state
		}
	}
}

/*
================
SV_PrepWorldFrame

This has to be done before the world logic, because
player processing happens outside RunWorldFrame
================
*/
void SV_PrepWorldFrame (void)
{
	edict_t	*ent;
	int		i;

	for (i=0 ; i<ge->num_edicts ; i++, ent++)
	{
		ent = EDICT_NUM(i);
		// events only last for a single message
		ent->s.event = 0;
	}

}


/*
=================
SV_RunGameFrame
=================
*/

void SV_RunGameFrame (void)
{
	float FRAMETIME = 1.0/(float)sv_tickrate->integer;

	if (host_speeds->integer)
		time_before_game = Sys_Milliseconds ();

	// we always need to bump framenum, even if we
	// don't run the world, otherwise the delta
	// compression can get confused when a client
	// has the "current" frame
	sv.framenum++;
	sv.time = sv.framenum*FRAMETIME*1000;

	// don't run if paused
	if (!sv_paused->integer || maxclients->integer > 1)
	{
		ge->RunFrame ();

		// never get more than one tic behind
		if (sv.time < svs.realtime)
		{
			if (sv_showclamp->integer)
				Com_Printf ("sv highclamp\n");
			svs.realtime = sv.time;
		}
	}

	if (host_speeds->integer)
		time_after_game = Sys_Milliseconds ();

}

/*
==================
SV_Frame

==================
*/
extern int sys_lasthang;
void SV_Frame (int msec)
{
	int tmp_systime, tmp_hangtime;
	static int old_systime = 0;
	float FRAMETIME = 1.0/(float)sv_tickrate->integer;

	if (!old_systime)
		old_systime = Sys_Milliseconds ();
	time_before_game = time_after_game = 0;

	// if server is not active, do nothing
	if (!svs.initialized)
		return;

    svs.realtime += msec;

	// keep the random time dependent
	rand ();

	// check timeouts
	SV_CheckTimeouts ();

	// get packets from clients
	SV_ReadPackets ();

	// move autonomous things around if enough time has passed
	if (!sv_timedemo->integer && svs.realtime < sv.time)
	{
		// never let the time get too far off
		if (sv.time - svs.realtime > FRAMETIME*1000)
		{
			if (sv_showclamp->integer)
				Com_Printf ("sv lowclamp\n");
			svs.realtime = sv.time - FRAMETIME*1000;
		}
		NET_Sleep(sv.time - svs.realtime);
		return;
	}

	// update ping based on the last known frame from all clients
	SV_CalcPings ();

	// give the clients some timeslices
	SV_GiveMsec ();

	// let everything in the world think and move
	SV_RunGameFrame ();

	// send messages back to the clients that had packets read this frame
	SV_SendClientMessages ();

	// save the entire world state if recording a serverdemo
	SV_RecordDemoMessage ();

	// send a heartbeat to the master if needed
	Master_Heartbeat ();

	// clear teleport flags, etc for next frame
	SV_PrepWorldFrame ();
	
	tmp_systime = Sys_Milliseconds ();
	tmp_hangtime = tmp_systime-old_systime;
	old_systime = tmp_systime;
	if (tmp_hangtime > FRAMETIME * 1500) {
		sys_lasthang = tmp_systime;
	}

}

//============================================================================

/*
================
Master_Heartbeat

Send a message to the master every few minutes to
let it know we are alive, and log information
================
*/
#define	HEARTBEAT_SECONDS	300
void Master_Heartbeat (void)
{
	char		string[MAX_MSGLEN];

	// pgm post3.19 change, cvar pointer not validated before dereferencing

	if(!public_server || !public_server->integer)
		return;

	// check for time wraparound
	if (svs.last_heartbeat > svs.realtime)
		svs.last_heartbeat = svs.realtime;

	if (svs.realtime - svs.last_heartbeat < HEARTBEAT_SECONDS*1000)
		return;		// not time to send yet

	svs.last_heartbeat = svs.realtime;

	// send the same string that we would give for a status OOB command
	Com_sprintf (string, MAX_MSGLEN, "heartbeat\n%s", SV_StatusString ());
	SV_HandleMasters (string, "heartbeat");
}

/*
=================
Master_Shutdown

Informs all masters that this server is going down
=================
*/
void Master_Shutdown (void)
{

	// pgm post3.19 change, cvar pointer not validated before dereferencing
	if (!public_server || !public_server->integer)
		return;		// a private dedicated game

	SV_HandleMasters ("shutdown", "shutdown");
}


/*
=================
SV_HandleMasters

Sends a message to all master servers, looking up the
master servers' addresses if appropriate.
=================
*/
void SV_HandleMasters (const char *message, const char *console_message)
{
	int		i;
	qboolean	updated_master;

	// if the server is not dedicated, we need to check cl_master
	if ( !( dedicated && dedicated->integer ) )
	{
		if ( !sv_master )
		{
			sv_master = Cvar_Get ("cl_master", DEFAULT_MASTER_1, CVAR_ARCHIVE);
			updated_master = true;
		}
		else if ( sv_master->modified )
		{
			sv_master->modified = false;
			updated_master = true;
		}
		else
		{
			updated_master = false;
		}

		if ( updated_master )
		{
			memset (&master_status[0], 0, sizeof(master_sv_t));
			strncpy (master_status[0].name, sv_master->string, MAX_MASTER_LEN);
		}
	}

	// first we need to loop through the master servers
	// in order to find the ones that need (re-)resolving
	for ( i = 0; i < MAX_MASTERS ; i ++ )
	{
		if ( master_status[i].name[0] == 0 )
			break;

		// if we already sent a ping and didn't get
		// any acknowledgement packet, we need to try
		// re-resolving
		if ( master_status[i].resolved && master_status[i].last_ping_sent > master_status[i].last_ping_ack )
		{
			Com_Printf ("No acknowledgement from %s - re-resolving\n", master_status[i].name);
			master_status[i].resolved = false;
		}

		if ( master_status[i].resolved )
			continue;

		if (!NET_StringToAdr (master_status[i].name, &master_status[i].addr))
		{
			// resolution failed, did we say so already?
			if ( !master_status[i].failed )
			{
				Com_Printf ("Bad master address: %s\n", master_status[i].name);
				master_status[i].failed = true;
			}
			master_status[i].addr.port = 0;
		}
		else
		{
			master_status[i].failed = false;
			master_status[i].resolved = true;

			if (master_status[i].addr.port == 0)
				master_status[i].addr.port = BigShort (PORT_MASTER);

			Com_Printf ("Master server at %s\n", NET_AdrToString (master_status[i].addr));
		}
	}

	// send the message we needed to send
	for ( i = 0 ; i < MAX_MASTERS ; i ++ )
	{
		if ( master_status[i].name[0] == 0 )
			break;

		if ( master_status[i].addr.port == 0 )
			continue;

		Com_Printf ("Sending %s to %s\n", console_message, NET_AdrToString (master_status[i].addr));
		Netchan_OutOfBandPrint (NS_SERVER, master_status[i].addr, "%s", message);
		master_status[i].last_ping_sent = 1;
		master_status[i].last_ping_ack = 0;
	}
}


//============================================================================


/*
=================
SV_UserinfoChanged

Pull specific info from a newly changed userinfo string
into a more C freindly form.
=================
*/
void SV_UserinfoChanged (client_t *cl)
{
	char	*val;
	int		i;

	// call prog code to allow overrides
	ge->ClientUserinfoChanged (cl->edict, cl->userinfo, 0);

	// name for C code
	Q_strncpyz2( cl->name, Info_ValueForKey (cl->userinfo, "name"), sizeof(cl->name) );
	// mask off high bit
	for (i=0 ; i<sizeof(cl->name) ; i++)
		cl->name[i] &= 127;

	// rate command
	val = Info_ValueForKey (cl->userinfo, "rate");
	if (strlen(val))
	{
		i = atoi(val);
		cl->rate = i;
		if (cl->rate < 100)
			cl->rate = 100;
		if (cl->rate > 15000)
			cl->rate = 15000;
	}
	else
		cl->rate = 5000;

	// msg command
	val = Info_ValueForKey (cl->userinfo, "msg");
	if (strlen(val))
	{
		cl->messagelevel = atoi(val);
	}

}


//============================================================================

/*
===============
SV_Init

Only called at quake2.exe startup, not for each game
===============
*/
void SV_Init (void)
{
	SV_InitOperatorCommands	();

	rcon_password = Cvar_Get ("rcon_password", "", 0);
	Cvar_Get ("skill", "1", 0);
	Cvar_Get ("deathmatch", "1", CVAR_LATCH); //Alien Arena is *always* deathmatch
	Cvar_Get ("ctf", "0", CVAR_LATCH);
	Cvar_Get ("dmflags", va("%i", DF_INSTANT_ITEMS+DF_BOT_LEVELAD), CVAR_SERVERINFO);
	Cvar_Get ("fraglimit", "0", CVAR_SERVERINFO);
	Cvar_Get ("timelimit", "0", CVAR_SERVERINFO);
	Cvar_Get ("cheats", "0", CVAR_SERVERINFO|CVAR_LATCH);
	Cvar_Get ("protocol", va("%i", PROTOCOL_VERSION), CVAR_SERVERINFO|CVAR_NOSET);
	maxclients = Cvar_Get ("maxclients", "1", CVAR_SERVERINFO | CVAR_LATCH);
	hostname = Cvar_Get ("hostname", "noname", CVAR_SERVERINFO | CVAR_ARCHIVE);
	timeout = Cvar_Get ("timeout", "125", 0);
	zombietime = Cvar_Get ("zombietime", "2", 0);
	sv_showclamp = Cvar_Get ("showclamp", "0", 0);
	sv_paused = Cvar_Get ("paused", "0", 0);
	sv_timedemo = Cvar_Get ("timedemo", "0", 0);
	sv_enforcetime = Cvar_Get ("sv_enforcetime", "0", 0);
	allow_download = Cvar_Get ("allow_download", "1", CVAR_ARCHIVE);
	allow_download_players  = Cvar_Get ("allow_download_players", "0", CVAR_ARCHIVE);
	allow_download_models = Cvar_Get ("allow_download_models", "1", CVAR_ARCHIVE);
	allow_download_sounds = Cvar_Get ("allow_download_sounds", "1", CVAR_ARCHIVE);
	allow_download_maps	  = Cvar_Get ("allow_download_maps", "1", CVAR_ARCHIVE);
	allow_overwrite_maps  = Cvar_Get ("allow_overwrite_maps", "0", CVAR_ARCHIVE);
	Cvar_Describe (allow_overwrite_maps, "Used for command installmap. If set to 1, existing downloaded map packs will be overwritten and all files in it will be extracted, overwriting any existing files.");
	sv_downloadurl = Cvar_Get("sv_downloadurl", DEFAULT_DOWNLOAD_URL_1, CVAR_SERVERINFO);
	sv_tickrate = Cvar_Get("sv_tickrate", "10", CVAR_SERVERINFO | CVAR_ARCHIVE);

	sv_iplogfile = Cvar_Get("sv_iplogfile" , "" , CVAR_ARCHIVE);

	sv_noreload = Cvar_Get ("sv_noreload", "0", 0);

	sv_airaccelerate = Cvar_Get("sv_airaccelerate", "0", CVAR_LATCH);

	sv_joustmode = Cvar_Get("sv_joustmode", "0", CVAR_SERVERINFO);
	sv_tactical = Cvar_Get("g_tactical", "0", CVAR_LATCH | CVAR_GAMEINFO);
	sv_excessive = Cvar_Get("excessive", "0", CVAR_LATCH | CVAR_GAMEINFO);

	public_server = Cvar_Get ("sv_public", "1", 0);

	sv_reconnect_limit = Cvar_Get ("sv_reconnect_limit", "3", CVAR_ARCHIVE);

	sv_ratelimit_status = Cvar_Get ("sv_ratelimit_status", "15", 0);

	sv_iplimit = Cvar_Get ("sv_iplimit", "3", 0);

	SZ_Init (&net_message, net_message_buffer, sizeof(net_message_buffer));
	SZ_SetName (&net_message, "Net message buffer", true);

	remoteserver_runspeed = 300; //default
}

/*
==================
SV_FinalMessage

Used by SV_Shutdown to send a final message to all
connected clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
void SV_FinalMessage (char *message, qboolean reconnect)
{
	int			i;
	client_t	*cl;

	SZ_Clear (&net_message);
	MSG_WriteByte (&net_message, svc_print);
	MSG_WriteByte (&net_message, PRINT_HIGH);
	MSG_WriteString (&net_message, message);

	if (reconnect)
		MSG_WriteByte (&net_message, svc_reconnect);
	else
		MSG_WriteByte (&net_message, svc_disconnect);

	// send it twice
	// stagger the packets to crutch operating system limited buffers

	for (i=0, cl = svs.clients ; i<maxclients->integer ; i++, cl++)

		if (cl->state >= cs_connected)
			Netchan_Transmit (&cl->netchan, net_message.cursize
			, net_message.data);

	for (i=0, cl = svs.clients ; i<maxclients->integer ; i++, cl++)

		if (cl->state >= cs_connected)
			Netchan_Transmit (&cl->netchan, net_message.cursize
			, net_message.data);
}



/*
================
SV_Shutdown

Called when each game quits,
before Sys_Quit or Sys_Error
================
*/
void SV_Shutdown (char *finalmsg, qboolean reconnect)
{
	extern void Con_Clear_f (void);

	if (svs.clients)
		SV_FinalMessage (finalmsg, reconnect);

	Master_Shutdown ();
	SV_ShutdownGameProgs ();

	// free current level
	if (sv.demofile)
		fclose (sv.demofile);
	memset (&sv, 0, sizeof(sv));
	Com_SetServerState (sv.state);

	// free server static data
	if (svs.clients)
		Z_Free (svs.clients);
	if (svs.client_entities)
		Z_Free (svs.client_entities);
	if (svs.demofile)
		fclose (svs.demofile);
	memset (&svs, 0, sizeof(svs));
}

qboolean IsVisible(vec3_t org1,vec3_t org2)
{
	trace_t	trace;

	trace = SV_Trace2 (org1, NULL, NULL, org2, NULL, MASK_VISIBILILITY);

	if (trace.fraction != 1)
		return false;
	return true;
}
