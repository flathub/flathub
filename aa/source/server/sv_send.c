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
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

*/
// sv_send.c

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "server.h"
#include "stdint.h"

/*
=============================================================================

Com_Printf redirection

=============================================================================
*/

char sv_outputbuf[SV_OUTPUTBUF_LENGTH];

void SV_FlushRedirect (int sv_redirected, char *outputbuf)
{
	if (sv_redirected == RD_PACKET)
	{
		Netchan_OutOfBandPrint (NS_SERVER, net_from, "print\n%s", outputbuf);
	}
	else if (sv_redirected == RD_CLIENT)
	{
		MSG_WriteByte (&sv_client->netchan.message, svc_print);
		MSG_WriteByte (&sv_client->netchan.message, PRINT_HIGH);
		MSG_WriteString (&sv_client->netchan.message, outputbuf);
	}
}


/*
=============================================================================

EVENT MESSAGES

=============================================================================
*/


/*
=================
SV_ClientPrintf

Sends text across to be displayed if the level passes
=================
*/
void SV_ClientPrintf (client_t *cl, int level, char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];

	if (level < cl->messagelevel)
		return;

	va_start (argptr,fmt);
	vsnprintf(string, sizeof(string), fmt, argptr);
	va_end (argptr);

	MSG_WriteByte (&cl->netchan.message, svc_print);
	MSG_WriteByte (&cl->netchan.message, level);
	MSG_WriteString (&cl->netchan.message, string);
}

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void SV_BroadcastPrintf (int level, char *fmt, ...)
{
	va_list		argptr;
	char		string[2048];
	client_t	*cl;
	int			i;

	va_start (argptr,fmt);
	vsnprintf(string, sizeof(string), fmt, argptr);
	va_end (argptr);

	// echo to console
	if (dedicated->value)
	{
		char	copy[1024];
		int		i;

		// mask off high bits
		for (i=0 ; i<1023 && string[i] ; i++)
			copy[i] = string[i]&127;
		copy[i] = 0;
		Com_Printf ("%s", copy);
	}

	for (i=0, cl = svs.clients ; i<maxclients->value; i++, cl++)
	{
		if (level < cl->messagelevel)
			continue;
		if (cl->state != cs_spawned)
			continue;
		MSG_WriteByte (&cl->netchan.message, svc_print);
		MSG_WriteByte (&cl->netchan.message, level);
		MSG_WriteString (&cl->netchan.message, string);
	}
}

/*
=================
SV_BroadcastCommand

Sends text to all active clients
=================
*/
void SV_BroadcastCommand (char *fmt, ...)
{
	va_list		argptr;
	char		string[1024];

	if (!sv.state)
		return;
	va_start (argptr,fmt);
	vsnprintf(string, sizeof(string), fmt, argptr);
	va_end (argptr);

	MSG_WriteByte (&sv.multicast, svc_stufftext);
	MSG_WriteString (&sv.multicast, string);
	SV_Multicast (NULL, MULTICAST_ALL_R);
}


/*
=================
SV_Multicast

Sends the contents of sv.multicast to a subset of the clients,
then clears sv.multicast.

MULTICAST_ALL	same as broadcast (origin can be NULL)
MULTICAST_PVS	send to clients potentially visible from org
MULTICAST_PHS	send to clients potentially hearable from org
=================
*/
void SV_Multicast (vec3_t origin, multicast_t to)
{
	client_t	*client;
	byte		*mask;
	int			leafnum, cluster;
	int			j;
	qboolean	reliable;
	int			area1, area2;

	/* 2011-11 Remove redundancy, was calling CM_PointLeafnum(origin) twice
	 * for every PVS/PHS message. Was doing unnecessary check on client
	 * location for PVS/PHS. Both minor performance hits.
	 */

	// if doing a serverrecord, store everything
	if (svs.demofile)
		SZ_Write (&svs.demo_multicast, sv.multicast.data, sv.multicast.cursize);

	reliable = false;
	mask     = NULL;
	area1    = 0;
	switch (to)
	{
	case MULTICAST_ALL_R:
		reliable = true;	// intentional fallthrough
	case MULTICAST_ALL:
		break;

	case MULTICAST_PHS_R:
		reliable = true;	// intentional fallthrough
	case MULTICAST_PHS:
		// location for source of the sound that may be hearable
		leafnum = CM_PointLeafnum (origin);
		area1   = CM_LeafArea (leafnum);
		cluster = CM_LeafCluster (leafnum);
		mask = CM_ClusterPHS (cluster);
		break;

	case MULTICAST_PVS_R:
		reliable = true;	// intentional fallthrough
	case MULTICAST_PVS:
		// location for entity or effect that may be visible
		leafnum = CM_PointLeafnum (origin);
		area1   = CM_LeafArea (leafnum);
		cluster = CM_LeafCluster (leafnum);
		mask = CM_ClusterPVS (cluster);
		break;

	default:
		Com_Error (ERR_FATAL, "SV_Multicast: bad to:%i", to);
	}

	// send the data to all relevent clients
	for ( j=maxclients->integer, client=svs.clients ; j-- ; client++ )
	{
		if (client->state == cs_free || client->state == cs_zombie)
			continue;
		if (client->state != cs_spawned && !reliable)
			continue;

		if (mask)
		{ // check for "hearable" or "visible"
			// find cluster player is in
			leafnum = CM_PointLeafnum (client->edict->s.origin);
			cluster = CM_LeafCluster (leafnum);
			if ( !(mask[cluster>>3] & (1<<(cluster & 7))) )
				continue; // by cluster mask, player cannot see/hear this
			area2 = CM_LeafArea (leafnum);
			if (!CM_AreasConnected (area1, area2))
				continue; // by leaf, player cannot see/hear this
		}

		if (reliable)
			SZ_Write (&client->netchan.message, sv.multicast.data, sv.multicast.cursize);
		else
			SZ_Write (&client->datagram, sv.multicast.data, sv.multicast.cursize);
	}

	SZ_Clear (&sv.multicast);
}


/*
==================
SV_StartSound

Each entity can have eight independant sound sources, like voice,
weapon, feet, etc.

If cahnnel & 8, the sound will be sent to everyone, not just
things in the PHS.

FIXME: if entity isn't in PHS, they must be forced to be sent or
have the origin explicitly sent.

Channel 0 is an auto-allocate channel, the others override anything
already running on that entity/channel pair.

An attenuation of 0 will play full volume everywhere in the level.
Larger attenuations will drop off.  (max 4 attenuation)

Timeofs can range from 0.0 to 0.1 to cause sounds to be started
later in the frame than they normally would.

If origin is NULL, the origin is determined from the entity origin
or the midpoint of the entity box for bmodels.
==================
*/
void SV_StartSound (vec3_t origin, edict_t *entity, int channel,
					int soundindex, float volume,
					float attenuation, float timeofs)
{
	int			sendchan;
    int			flags;
    int			i;
	int			ent;
	vec3_t		origin_v;
	qboolean	use_phs;

	if (volume < 0 || volume > 1.0)
		Com_Error (ERR_FATAL, "SV_StartSound: volume = %f", volume);

	if (attenuation < 0 || attenuation > 4)
		Com_Error (ERR_FATAL, "SV_StartSound: attenuation = %f", attenuation);

//	if (channel < 0 || channel > 15)
//		Com_Error (ERR_FATAL, "SV_StartSound: channel = %i", channel);

	if (timeofs < 0 || timeofs > 0.255)
		Com_Error (ERR_FATAL, "SV_StartSound: timeofs = %f", timeofs);

	ent = NUM_FOR_EDICT(entity);

	if (channel & 8)	// no PHS flag
	{
		use_phs = false;
		channel &= 7;
	}
	else
		use_phs = true;

	sendchan = (ent<<3) | (channel&7);

	flags = 0;
	if (volume != DEFAULT_SOUND_PACKET_VOLUME)
		flags |= SND_VOLUME;
	if (attenuation != DEFAULT_SOUND_PACKET_ATTENUATION)
		flags |= SND_ATTENUATION;

	// the client doesn't know that bmodels have weird origins
	// the origin can also be explicitly set
	if ( (entity->svflags & SVF_NOCLIENT)
		|| (entity->solid == SOLID_BSP)
		|| origin )
		flags |= SND_POS;

	// always send the entity number for channel overrides
	flags |= SND_ENT;

	if (timeofs)
		flags |= SND_OFFSET;

	// use the entity origin unless it is a bmodel or explicitly specified
	if (!origin)
	{
		origin = origin_v;
		if (entity->solid == SOLID_BSP)
		{
			for (i=0 ; i<3 ; i++)
				origin_v[i] = entity->s.origin[i]+0.5*(entity->mins[i]+entity->maxs[i]);
		}
		else
		{
			VectorCopy (entity->s.origin, origin_v);
		}
	}

	MSG_WriteByte (&sv.multicast, svc_sound);
	MSG_WriteByte (&sv.multicast, flags);
	MSG_WriteByte (&sv.multicast, soundindex);

	if (flags & SND_VOLUME)
		MSG_WriteByte (&sv.multicast, volume*255);
	if (flags & SND_ATTENUATION)
		MSG_WriteByte (&sv.multicast, attenuation*64);
	if (flags & SND_OFFSET)
		MSG_WriteByte (&sv.multicast, timeofs*1000);

	if (flags & SND_ENT)
		MSG_WriteShort (&sv.multicast, sendchan);

	if (flags & SND_POS)
		MSG_WritePos (&sv.multicast, origin);

	// if the sound doesn't attenuate,send it to everyone
	// (global radio chatter, voiceovers, etc)
	if (attenuation == ATTN_NONE)
		use_phs = false;

	if (channel & CHAN_RELIABLE)
	{
		if (use_phs)
			SV_Multicast (origin, MULTICAST_PHS_R);
		else
			SV_Multicast (origin, MULTICAST_ALL_R);
	}
	else
	{
		if (use_phs)
			SV_Multicast (origin, MULTICAST_PHS);
		else
			SV_Multicast (origin, MULTICAST_ALL);
	}
}


/*
===============================================================================

FRAME UPDATES

===============================================================================
*/



/*
=======================
SV_SendClientDatagram
=======================
*/
qboolean SV_SendClientDatagram (client_t *client)
{
	byte		msg_buf[MAX_MSGLEN];
	sizebuf_t	msg;

	SV_BuildClientFrame (client);

	SZ_Init (&msg, msg_buf, sizeof(msg_buf));
	SZ_SetName (&msg, va("Client msg out buffer (%s)", NET_AdrToString(client->netchan.remote_address)), false);
	msg.allowoverflow = true;

	// send over all the relevant entity_state_t
	// and the player_state_t
	SV_WriteFrameToClient (client, &msg);

	// copy the accumulated multicast datagram
	// for this client out to the message
	// it is necessary for this to be after the WriteEntities
	// so that entity references will be current
	if (client->datagram.overflowed)
		Com_Printf ("WARNING: datagram overflowed for %s\n", client->name);
	else
		SZ_Write (&msg, client->datagram.data, client->datagram.cursize);
	SZ_Clear (&client->datagram);

	if (msg.overflowed)
	{	// must have room left for the packet header
		Com_Printf ("WARNING: msg overflowed for %s\n", client->name);
		SZ_Clear (&msg);
	}

	// send the datagram
	Netchan_Transmit (&client->netchan, msg.cursize, msg.data);

	// record the size for rate estimation
	client->message_size[sv.framenum % RATE_MESSAGES] = msg.cursize;

	return true;
}


/*
==================
SV_DemoCompleted
==================
*/
void SV_DemoCompleted (void)
{
	if (sv.demofile)
	{
		fclose (sv.demofile);
		sv.demofile = NULL;
		FS_FreeFile (sv.demobuf);
		sv.demosize = sv.demo_ofs = 0;
	}
	SV_Nextserver ();
}


/*
=======================
SV_RateDrop

Returns true if the client is over its current
bandwidth estimation and should not be sent another packet
=======================
*/
qboolean SV_RateDrop (client_t *c)
{
	int		total;
	int		i;

	// never drop over the loopback
	if (c->netchan.remote_address.type == NA_LOOPBACK)
		return false;

	total = 0;

	for (i = 0 ; i < RATE_MESSAGES ; i++)
	{
		total += c->message_size[i];
	}

	if (total > c->rate)
	{
		c->surpressCount++;
		c->message_size[sv.framenum % RATE_MESSAGES] = 0;
		return true;
	}

	return false;
}

/*
=======================
SV_SendClientMessages
=======================
*/
void SV_SendClientMessages (void)
{
	int			i;
	client_t	*c;
	int			msglen;
	byte		msgbuf[MAX_MSGLEN];

	msglen = 0;

	// read the next demo message if needed
	if (sv.state == ss_demo && sv.demofile)
	{
		if (sv_paused->value)
			msglen = 0;
		else
		{
			// get the next message
			if (sv.demo_ofs+4 >= sv.demosize)
			{
				SV_DemoCompleted ();
				return;
			}
			msglen = *(int32_t *)(sv.demobuf+sv.demo_ofs);
			msglen = LittleLong (msglen);
			sv.demo_ofs += sizeof (int32_t);
			if (msglen == -1)
			{
				SV_DemoCompleted ();
				return;
			}
			if (msglen > MAX_MSGLEN)
				Com_Error (ERR_DROP, "SV_SendClientMessages: msglen > MAX_MSGLEN");
			
			if (sv.demo_ofs+msglen >= sv.demosize)
			{
				SV_DemoCompleted ();
				return;
			}
			memcpy (msgbuf, sv.demobuf+sv.demo_ofs, msglen);
			sv.demo_ofs += msglen;
		}
	}

	// send a message to each connected client
	for (i=0, c = svs.clients ; i<maxclients->value; i++, c++)
	{
		if (!c->state)
			continue;
		// if the reliable message overflowed,
		// drop the client
		if (c->netchan.message.overflowed)
		{
			SZ_Clear (&c->netchan.message);
			SZ_Clear (&c->datagram);
			SV_BroadcastPrintf (PRINT_HIGH, "%s overflowed\n", c->name);
			SV_DropClient (c);
		}

		if (sv.state == ss_cinematic
			|| sv.state == ss_demo
			|| sv.state == ss_pic
			)
			Netchan_Transmit (&c->netchan, msglen, msgbuf);
		else if (c->state == cs_spawned)
		{
			// don't overrun bandwidth
			if (SV_RateDrop (c))
				continue;

			SV_SendClientDatagram (c);
		}
		else
		{ // not spawned
			/* send accumulated reliable messages when buffer has enough data
			 * or when a time limit is reached whichever comes first.
			 * time limit is 1 sec, buffer size trigger is 2800/4 = 700
			 * keeps UDP packets mostly below 1000 bytes. some capture data
			 * collected 2012-11 indicated packets occasionally could exceed
			 * 1500 bytes which is not good. Possible cause of some overflow
			 * disconnects and other net comm problems.
			 */
			int timeout = curtime - c->netchan.last_sent ;
			int cursize = c->netchan.message.cursize ;
			if ( timeout > 1000 || cursize >= (MAX_MSGLEN)/4 )
			{
				Netchan_Transmit( &c->netchan, 0, NULL );
			}
		}
	}
}

