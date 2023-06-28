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
// cl_ents.c -- entity parsing and management

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "client.h"

clentity_t *active_clentities, *free_clentities;
clentity_t clentities[MAX_CLENTITIES];
extern cvar_t *r_shaders;
extern void R_ApplyForceToRagdolls(vec3_t origin, float force);

/*
=========================================================================

FRAME PARSING

=========================================================================
*/

/*
=================
CL_ParseEntityBits

Returns the entity number and the header bits
=================
*/
int	bitcounts[32];	/// just for protocol profiling
int CL_ParseEntityBits (unsigned *bits)
{
	unsigned	b, total;
	int			i;
	int			number;

	total = MSG_ReadByte (&net_message);
	if (total & U_MOREBITS1)
	{
		b = MSG_ReadByte (&net_message);
		total |= b<<8;
	}
	if (total & U_MOREBITS2)
	{
		b = MSG_ReadByte (&net_message);
		total |= b<<16;
	}
	if (total & U_MOREBITS3)
	{
		b = MSG_ReadByte (&net_message);
		total |= b<<24;
	}

	// count the bits for net profiling
	for (i=0 ; i<32 ; i++)
		if (total&(1<<i))
			bitcounts[i]++;

	if (total & U_NUMBER16)
		number = MSG_ReadShort (&net_message);
	else
		number = MSG_ReadByte (&net_message);

	*bits = total;

	return number;
}

/*
==================
CL_ParseDelta

Can go from either a baseline or a previous packet_entity
==================
*/
void CL_ParseDelta (entity_state_t *from, entity_state_t *to, int number, int bits)
{
	// set everything to the state we are delta'ing from
	*to = *from;

	VectorCopy (from->origin, to->old_origin);
	to->number = number;

	if (bits & U_MODEL)
		to->modelindex = MSG_ReadByte (&net_message);
	if (bits & U_MODEL2)
		to->modelindex2 = MSG_ReadByte (&net_message);
	if (bits & U_MODEL3)
		to->modelindex3 = MSG_ReadByte (&net_message);
	if (bits & U_MODEL4)
		to->modelindex4 = MSG_ReadByte (&net_message);

	if (bits & U_FRAME8)
		to->frame = MSG_ReadByte (&net_message);
	if (bits & U_FRAME16)
		to->frame = MSG_ReadShort (&net_message);

	if ((bits & U_SKIN8) && (bits & U_SKIN16))		//used for laser colors
		to->skinnum = MSG_ReadLong(&net_message);
	else if (bits & U_SKIN8)
		to->skinnum = MSG_ReadByte(&net_message);
	else if (bits & U_SKIN16)
		to->skinnum = MSG_ReadShort(&net_message);

	if ( (bits & (U_EFFECTS8|U_EFFECTS16)) == (U_EFFECTS8|U_EFFECTS16) )
		to->effects = MSG_ReadLong(&net_message);
	else if (bits & U_EFFECTS8)
		to->effects = MSG_ReadByte(&net_message);
	else if (bits & U_EFFECTS16)
		to->effects = MSG_ReadShort(&net_message);

	if ( (bits & (U_RENDERFX8|U_RENDERFX16)) == (U_RENDERFX8|U_RENDERFX16) )
		to->renderfx = MSG_ReadLong(&net_message);
	else if (bits & U_RENDERFX8)
		to->renderfx = MSG_ReadByte(&net_message);
	else if (bits & U_RENDERFX16)
		to->renderfx = MSG_ReadShort(&net_message);

	if (bits & U_ORIGIN1)
		to->origin[0] = MSG_ReadCoord (&net_message);
	if (bits & U_ORIGIN2)
		to->origin[1] = MSG_ReadCoord (&net_message);
	if (bits & U_ORIGIN3)
		to->origin[2] = MSG_ReadCoord (&net_message);

	if (bits & U_ANGLE1)
		to->angles[0] = MSG_ReadAngle(&net_message);
	if (bits & U_ANGLE2)
		to->angles[1] = MSG_ReadAngle(&net_message);
	if (bits & U_ANGLE3)
		to->angles[2] = MSG_ReadAngle(&net_message);

	if (bits & U_OLDORIGIN)
		MSG_ReadPos (&net_message, to->old_origin);

	if (bits & U_SOUND)
		to->sound = MSG_ReadByte (&net_message);

	if (bits & U_EVENT)
		to->event = MSG_ReadByte (&net_message);
	else
		to->event = 0;

	if (bits & U_SOLID)
		to->solid = MSG_ReadShort (&net_message);
}

/*
==================
CL_DeltaEntity

Parses deltas from the given base and adds the resulting entity
to the current frame
==================
*/
void CL_DeltaEntity (frame_t *frame, int newnum, entity_state_t *old, int bits)
{
	centity_t	*ent;
	entity_state_t	*state;

	ent = &cl_entities[newnum];

	state = &cl_parse_entities[cl.parse_entities & (MAX_PARSE_ENTITIES-1)];
	cl.parse_entities++;
	frame->num_entities++;

	CL_ParseDelta (old, state, newnum, bits);

	// some data changes will force no lerping
	if (state->modelindex != ent->current.modelindex
		|| state->modelindex2 != ent->current.modelindex2
		|| state->modelindex3 != ent->current.modelindex3
		|| state->modelindex4 != ent->current.modelindex4
		|| abs(state->origin[0] - ent->current.origin[0]) > 512
		|| abs(state->origin[1] - ent->current.origin[1]) > 512
		|| abs(state->origin[2] - ent->current.origin[2]) > 512
		|| state->event == EV_PLAYER_TELEPORT
		|| state->event == EV_OTHER_TELEPORT
		)
	{
		ent->serverframe = -99;
	}

	if (ent->serverframe != cl.frame.serverframe - 1)
	{	// wasn't in last update, so initialize some things
		ent->trailcount = 1024;		// for diminishing rocket / grenade trails
		ent->pr = NULL;             // for chained particle trails
		// duplicate the current state so lerping doesn't hurt anything
		ent->prev = *state;
		if (state->event == EV_OTHER_TELEPORT)
		{
			VectorCopy (state->origin, ent->prev.origin);
			VectorCopy (state->origin, ent->lerp_origin);
		}
		else
		{
			VectorCopy (state->old_origin, ent->prev.origin);
			VectorCopy (state->old_origin, ent->lerp_origin);
		}
	}
	else
	{	// shuffle the last state to previous
		ent->prev = ent->current;
	}

	ent->serverframe = cl.frame.serverframe;
	ent->current = *state;
}

/*
==================
CL_ParsePacketEntities

An svc_packetentities has just been parsed, deal with the
rest of the data stream.
==================
*/
void CL_ParsePacketEntities (frame_t *oldframe, frame_t *newframe)
{
	int			newnum;
	int			bits;
	entity_state_t	*oldstate = NULL;
	int			oldindex, oldnum;

	newframe->parse_entities = cl.parse_entities;
	newframe->num_entities = 0;

	// delta from the entities present in oldframe
	oldindex = 0;
	if (!oldframe)
		oldnum = 99999;
	else
	{
		if (oldindex >= oldframe->num_entities)
			oldnum = 99999;
		else
		{
			oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}
	}

	while (1)
	{
		newnum = CL_ParseEntityBits ( (unsigned *)&bits );
		if (newnum >= MAX_EDICTS)
			Com_Error (ERR_DROP,"CL_ParsePacketEntities: bad number:%i", newnum);

		if (net_message.readcount > net_message.cursize)
			Com_Error (ERR_DROP,"CL_ParsePacketEntities: end of message");

		if (!newnum)
			break;

		while (oldnum < newnum)
		{	// one or more entities from the old packet are unchanged
			if (cl_shownet->value == 3)
				Com_Printf ("   unchanged: %i\n", oldnum);
			CL_DeltaEntity (newframe, oldnum, oldstate, 0);

			oldindex++;

			if (oldindex >= oldframe->num_entities)
				oldnum = 99999;
			else
			{
				oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
		}

		if (bits & U_REMOVE)
		{	// the entity present in oldframe is not in the current frame
			if (cl_shownet->value == 3)
				Com_Printf ("   remove: %i\n", newnum);
			if (oldnum != newnum)
			{
				if (cl_shownet->value == 3)
					Com_Printf ("U_REMOVE: oldnum != newnum\n");
			}

			oldindex++;

			if (oldindex >= oldframe->num_entities)
				oldnum = 99999;
			else
			{
				oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum == newnum)
		{	// delta from previous state
			if (cl_shownet->value == 3)
				Com_Printf ("   delta: %i\n", newnum);
			CL_DeltaEntity (newframe, newnum, oldstate, bits);

			oldindex++;

			if (oldindex >= oldframe->num_entities)
				oldnum = 99999;
			else
			{
				oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
				oldnum = oldstate->number;
			}
			continue;
		}

		if (oldnum > newnum)
		{	// delta from baseline
			if (cl_shownet->value == 3)
				Com_Printf ("   baseline: %i\n", newnum);
			CL_DeltaEntity (newframe, newnum, &cl_entities[newnum].baseline, bits);
			continue;
		}

	}

	// any remaining entities in the old frame are copied over
	while (oldnum != 99999)
	{	// one or more entities from the old packet are unchanged
		if (cl_shownet->value == 3)
			Com_Printf ("   unchanged: %i\n", oldnum);
		CL_DeltaEntity (newframe, oldnum, oldstate, 0);

		oldindex++;

		if (oldindex >= oldframe->num_entities)
			oldnum = 99999;
		else
		{
			oldstate = &cl_parse_entities[(oldframe->parse_entities+oldindex) & (MAX_PARSE_ENTITIES-1)];
			oldnum = oldstate->number;
		}
	}
}



/*
===================
CL_ParsePlayerstate
===================
*/
void CL_ParsePlayerstate (frame_t *oldframe, frame_t *newframe)
{
	int			flags;
	player_state_t	*state;
	int			i;
	int			statbits;

	state = &newframe->playerstate;

	// clear to old value before delta parsing
	if (oldframe)
		*state = oldframe->playerstate;
	else
		memset (state, 0, sizeof(*state));

	flags = MSG_ReadShort (&net_message);

	//
	// parse the pmove_state_t
	//
	if (flags & PS_M_TYPE)
		state->pmove.pm_type = MSG_ReadByte (&net_message);

	if (flags & PS_M_ORIGIN)
	{
		state->pmove.origin[0] = MSG_ReadSizeInt (&net_message, coord_bytes);
		state->pmove.origin[1] = MSG_ReadSizeInt (&net_message, coord_bytes);
		state->pmove.origin[2] = MSG_ReadSizeInt (&net_message, coord_bytes);
	}

	if (flags & PS_M_VELOCITY)
	{
		state->pmove.velocity[0] = MSG_ReadShort (&net_message);
		state->pmove.velocity[1] = MSG_ReadShort (&net_message);
		state->pmove.velocity[2] = MSG_ReadShort (&net_message);
	}

	if (flags & PS_M_TIME)
		state->pmove.pm_time = MSG_ReadByte (&net_message);

	if (flags & PS_M_FLAGS)
		state->pmove.pm_flags = MSG_ReadByte (&net_message);

	if (flags & PS_M_GRAVITY)
		state->pmove.gravity = MSG_ReadShort (&net_message);

	if (flags & PS_M_DELTA_ANGLES)
	{
		state->pmove.delta_angles[0] = MSG_ReadShort (&net_message);
		state->pmove.delta_angles[1] = MSG_ReadShort (&net_message);
		state->pmove.delta_angles[2] = MSG_ReadShort (&net_message);
	}

	if (cl.attractloop)
		state->pmove.pm_type = PM_FREEZE;		// demo playback

	//
	// parse the rest of the player_state_t
	//
	if (flags & PS_VIEWOFFSET)
	{
		state->viewoffset[0] = MSG_ReadChar (&net_message) * 0.25;
		state->viewoffset[1] = MSG_ReadChar (&net_message) * 0.25;
		state->viewoffset[2] = MSG_ReadChar (&net_message) * 0.25;
	}

	if (flags & PS_VIEWANGLES)
	{
		state->viewangles[0] = MSG_ReadAngle16 (&net_message);
		state->viewangles[1] = MSG_ReadAngle16 (&net_message);
		state->viewangles[2] = MSG_ReadAngle16 (&net_message);
	}

	if (flags & PS_KICKANGLES)
	{
		state->kick_angles[0] = MSG_ReadChar (&net_message) * 0.25;
		state->kick_angles[1] = MSG_ReadChar (&net_message) * 0.25;
		state->kick_angles[2] = MSG_ReadChar (&net_message) * 0.25;
	}

	if (flags & PS_WEAPONINDEX)
	{
		state->gunindex = MSG_ReadByte (&net_message);
	}

	if (flags & PS_WEAPONFRAME)
	{
		state->gunframe = MSG_ReadByte (&net_message);
		state->gunoffset[0] = MSG_ReadChar (&net_message)*0.25;
		state->gunoffset[1] = MSG_ReadChar (&net_message)*0.25;
		state->gunoffset[2] = MSG_ReadChar (&net_message)*0.25;
		state->gunangles[0] = MSG_ReadChar (&net_message)*0.25;
		state->gunangles[1] = MSG_ReadChar (&net_message)*0.25;
		state->gunangles[2] = MSG_ReadChar (&net_message)*0.25;
	}

	if (flags & PS_BLEND)
	{
		state->blend[0] = MSG_ReadByte (&net_message)/255.0;
		state->blend[1] = MSG_ReadByte (&net_message)/255.0;
		state->blend[2] = MSG_ReadByte (&net_message)/255.0;
		state->blend[3] = MSG_ReadByte (&net_message)/255.0;
	}

	if (flags & PS_FOV)
		state->fov = MSG_ReadByte (&net_message);

	if (flags & PS_RDFLAGS)
		state->rdflags = MSG_ReadByte (&net_message);

	// parse stats
	statbits = MSG_ReadLong (&net_message);
	for (i=0 ; i<MAX_STATS ; i++)
		if (statbits & (1<<i) )
			state->stats[i] = MSG_ReadShort(&net_message);
}


/*
==================
CL_FireEntityEvents

==================
*/
void CL_FireEntityEvents (frame_t *frame)
{
	entity_state_t		*s1;
	int					pnum, num;

	for (pnum = 0 ; pnum<frame->num_entities ; pnum++)
	{
		num = (frame->parse_entities + pnum)&(MAX_PARSE_ENTITIES-1);
		s1 = &cl_parse_entities[num];
		if (s1->event)
			CL_EntityEvent (s1);

		// EF_TELEPORTER acts like an event, but is not cleared each frame
		if (s1->effects & EF_TELEPORTER)
			CL_BigTeleportParticles(s1->origin);
	}
}


/*
================
CL_ParseFrame
================
*/
void CL_ParseFrame (void)
{
	int			cmd;
	int			len;
	frame_t		*old;
	byte		new_areabits[MAX_MAP_AREAS/8];
	float FRAMETIME = 1.0/(float)server_tickrate;

	//HACK: reduce the number of areabit changes detected
	memcpy (new_areabits, cl.frame.areabits, MAX_MAP_AREAS/8);
	memset (&cl.frame, 0, sizeof(cl.frame));
	memcpy (cl.frame.areabits, new_areabits, MAX_MAP_AREAS/8);

	cl.frame.serverframe = MSG_ReadLong (&net_message);
	cl.frame.deltaframe = MSG_ReadLong (&net_message);
	cl.frame.servertime = cl.frame.serverframe*1000*FRAMETIME;

	// BIG HACK to let old demos continue to work
	if (cls.serverProtocol != 26)
		cl.surpressCount = MSG_ReadByte (&net_message);

	if (cl_shownet->value == 3)
		Com_Printf ("   frame:%i  delta:%i\n", cl.frame.serverframe,
		cl.frame.deltaframe);

	// If the frame is delta compressed from data that we
	// no longer have available, we must suck up the rest of
	// the frame, but not use it, then ask for a non-compressed
	// message
	if (cl.frame.deltaframe <= 0)
	{
		cl.frame.valid = true;		// uncompressed frame
		old = NULL;
		cls.demowaiting = false;	// we can start recording now
	}
	else
	{
		old = &cl.frames[cl.frame.deltaframe & UPDATE_MASK];
		if (!old->valid)
		{	// should never happen
			Com_Printf ("Delta from invalid frame (not supposed to happen!).\n");
		}
		if (old->serverframe != cl.frame.deltaframe)
		{	// The frame that the server did the delta from
			// is too old, so we can't reconstruct it properly.
			Com_Printf ("Delta frame too old.\n");
		}
		else if (cl.parse_entities - old->parse_entities > MAX_PARSE_ENTITIES-128)
		{
			if (cl_shownet->value == 3)
			Com_Printf ("Delta parse_entities too old.\n");
		}
		else
			cl.frame.valid = true;	// valid delta parse
	}

	// clamp time
	if (cl.time > cl.frame.servertime)
		cl.time = cl.frame.servertime;
	else if (cl.time < cl.frame.servertime - 1000*FRAMETIME)
		cl.time = cl.frame.servertime - 1000*FRAMETIME;

	// read areabits
	len = MSG_ReadByte (&net_message);
	MSG_ReadData (&net_message, &new_areabits, len);
	if (memcmp (cl.frame.areabits, new_areabits, len))
	{
		memcpy (cl.frame.areabits, new_areabits, len);
		cl.refdef.areabits_changed = true;
	}

	// read playerinfo
	cmd = MSG_ReadByte (&net_message);
	SHOWNET(svc_strings[cmd]);
	if (cmd != svc_playerinfo)
		Com_Error (ERR_DROP, "CL_ParseFrame: not playerinfo");
	CL_ParsePlayerstate (old, &cl.frame);

	// read packet entities
	cmd = MSG_ReadByte (&net_message);
	SHOWNET(svc_strings[cmd]);
	if (cmd != svc_packetentities)
		Com_Error (ERR_DROP, "CL_ParseFrame: not packetentities");
	CL_ParsePacketEntities (old, &cl.frame);

	// save the frame off in the backup array for later delta comparisons
	cl.frames[cl.frame.serverframe & UPDATE_MASK] = cl.frame;

	if (cl.frame.valid)
	{
		// getting a valid frame message ends the connection process
		if (cls.state != ca_active)
		{
			cls.state = ca_active;
			cl.force_refdef = true;
			cl.predicted_origin[0] = cl.frame.playerstate.pmove.origin[0]*0.125;
			cl.predicted_origin[1] = cl.frame.playerstate.pmove.origin[1]*0.125;
			cl.predicted_origin[2] = cl.frame.playerstate.pmove.origin[2]*0.125;
			VectorCopy (cl.frame.playerstate.viewangles, cl.predicted_angles);
			if (cls.disable_servercount != cl.servercount
				&& cl.refresh_prepped)
				SCR_EndLoadingPlaque ();	// get rid of loading plaque
		}
		cl.sound_prepped = true;	// can start mixing ambient sounds

		// fire entity events
		CL_FireEntityEvents (&cl.frame);
		CL_CheckPredictionError ();
	}
}

/*
==========================================================================

INTERPOLATE BETWEEN FRAMES TO GET RENDERING PARMS

==========================================================================
*/

/*
===============
CL_AddPacketEntities

===============
*/
struct model_s
{
	char		name[MAX_QPATH];
};

static int lastEntFireFrameTime = 0;
static qboolean runFire = false;
void CL_AddPacketEntities (frame_t *frame)
{
	entity_t			ent;
	entity_state_t		*s1;
	float				autorotate;
	float				bob, bob_scale;
	int					i;
	int					pnum;
	centity_t			*cent;
	int					autoanim;
	clientinfo_t		*ci;
	unsigned int		effects, renderfx;
	qboolean			playermodel;
	char	shortname[MAX_OSPATH];

	// bonus items rotate at a fixed rate
	autorotate = anglemod(cl.time/10.0); 

	// brush models can auto animate their frames
	autoanim = 2*cl.time/1000;

	memset (&ent, 0, sizeof(ent));

	// run fire effects only at a roughly 30 fps rate
	if(Sys_Milliseconds() - lastEntFireFrameTime > 30)
	{
		lastEntFireFrameTime = Sys_Milliseconds();
		runFire = true;
	}
	else
		runFire = false;

	for (pnum = 0 ; pnum<frame->num_entities ; pnum++)
	{
		s1 = &cl_parse_entities[(frame->parse_entities+pnum)&(MAX_PARSE_ENTITIES-1)];

		cent = &cl_entities[s1->number];
		
		ent.number = s1->number;

		playermodel = false;
		
		effects = s1->effects;
		renderfx = s1->renderfx;

			// set frame
		if (effects & EF_ANIM01)
			ent.frame = autoanim & 1;
		else if (effects & EF_ANIM23)
			ent.frame = 2 + (autoanim & 1);
		else if (effects & EF_ANIM_ALL)
			ent.frame = autoanim;
		else if (effects & EF_ANIM_ALLFAST)
			ent.frame = cl.time / 100; //To Do - check this at different tickrates, looks like another 10fps hardcoded in
		else
			ent.frame = s1->frame;		

		ent.oldframe = cent->prev.frame;
		ent.backlerp = 1.0 - cl.lerpfrac;

		//animation timestamps are set here
		if(cent->prevframe != s1->frame) {
			cent->prevframe = s1->frame;
			cent->frametime = Sys_Milliseconds();
		}
		ent.prevframe = cent->prevframe;
		ent.frametime = cent->frametime;

		// create a new entity

		ent.lod1 = NULL;
		ent.lod2 = NULL;
		ent.team = 0;
		ent.flag = 0;
		ent.nodraw = 0;

		// set skin
		if (s1->modelindex == 255)
		{	// use custom player skin
			ent.skinnum = 0;
			ci = &cl.clientinfo[s1->skinnum & 0xff];
			ent.skin = ci->skin;
			ent.model = ci->model;

			ent.lod1 = ci->lod1;
			
			ent.lod2 = ci->lod2;
			
			if (!ent.skin || !ent.model)
			{
				ent.skin = cl.baseclientinfo.skin;
				ent.model = cl.baseclientinfo.model;
			}
			playermodel = true;

			strcpy(ent.name, ci->name);

			//set team of this player model
			if (effects & EF_TEAM1)
				ent.team = 1;
			else if (effects & EF_TEAM2)
			{
				if(server_is_team)
					ent.team = 2;
				else
					ent.team = 3;
			}
		}
		else
		{
			ent.skinnum = s1->skinnum;
			ent.skin = NULL;
			ent.model = cl.model_draw[s1->modelindex];
		}

		// quad and pent can do different things on client
		if (effects & EF_PENT)
		{
			//effects &= ~EF_PENT;
			effects |= EF_COLOR_SHELL;
			renderfx |= RF_SHELL_RED;
		}

		if (effects & EF_QUAD)
		{
			//effects &= ~EF_QUAD;
			effects |= EF_COLOR_SHELL;
			renderfx |= RF_SHELL_BLUE;
		}
		
		if (renderfx & (RF_FRAMELERP))
		{	// step origin discretely, because the frames
			// do the animation properly
			VectorCopy (cent->current.origin, ent.origin);
			VectorCopy (cent->current.old_origin, ent.oldorigin);
		}
		else if (s1->number == cl.playernum+1)
		{
			for (i=0; i<3; i++) {
				ent.origin[i] = ent.oldorigin[i] = cent->current.origin[i] + cl.lerpfrac *
					(cent->current.origin[i] - cent->prev.origin[i]);
			}
		}
		else
		{	// interpolate origin
			for (i=0 ; i<3 ; i++)
			{
				ent.origin[i] = ent.oldorigin[i] = cent->prev.origin[i] + cl.lerpfrac *
					(cent->current.origin[i] - cent->prev.origin[i]);
			}
		}

		// calculate angles
		if (effects & EF_ROTATE)
		{	// some bonus items auto-rotate
			ent.angles[0] = 0;
			ent.angles[1] = autorotate;
			ent.angles[2] = 0;
			// bobbing items
			bob_scale = (0.005f + s1->number * 0.00001f) * 0.5;
			bob = 5 + cos( (cl.time + (1000)) * bob_scale ) * 5;
			ent.oldorigin[2] += bob;
			ent.origin[2] += bob;

			ent.bob = bob;

			renderfx |= RF_BOBBING;
		}
		else
		{	// interpolate angles
			float	a1, a2;

			for (i=0 ; i<3 ; i++)
			{
				a1 = cent->current.angles[i];
				a2 = cent->prev.angles[i];
				ent.angles[i] = LerpAngle (a2, a1, cl.lerpfrac);
			}
		}

		// render effects (fullbright, translucent, etc)
		if ((effects & EF_COLOR_SHELL))
			ent.flags = 0;	// renderfx go on color shell entity
		else
			ent.flags = renderfx;

		if (s1->number == cl.playernum+1)
		{
			ent.flags |= RF_VIEWERMODEL;	// only draw from mirrors
			// fixed player origin - fixes the "jittery" player shadows too
			if ((cl_predict->value) && !(cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION))
			{
				VectorCopy (cl.predicted_origin, ent.origin);
				VectorCopy (cl.predicted_origin, ent.oldorigin);
			}

		}

		// Sigh, this is the reason for the RF_NODRAW workaround. 
#if 1 // TODO: replace this with an IFDEF for non Alien Arena CRX games
		// if set to invisible, skip
		if (!s1->modelindex)
			continue;
#endif

		if (effects & EF_PLASMA)
		{
			ent.flags |= RF_TRANSLUCENT;
			ent.alpha = 0.6;
		}

		if (effects & EF_BUBBLES)
		{
			CL_PoweredEffects (ent.origin, EF_BUBBLES);
		}

		//Various muzzleflash effects
		if(!(ent.flags & RF_VIEWERMODEL))
		{
			if (effects & EF_CHAINGUN) 
			{
				CL_MuzzleFlashParticle(ent.origin, ent.angles, false);
			}
			else if (effects & EF_GREENMZF) 
			{
				CL_BlasterMuzzleParticles(ent.origin, ent.angles, 0xd4, 0.7, false);
			}
			else if (effects & EF_BLUEMZF) 
			{
				CL_BlasterMuzzleParticles(ent.origin, ent.angles, 0x74, 0.7, false);
			}
			else if (effects & EF_SHOCKBALL)
			{
				CL_LightningBall (ent.origin, ent.angles, 0x74, false);
			}
			else if (effects & EF_SMARTMZF)
			{
				CL_SmartMuzzle (ent.origin, ent.angles, false);
			}
			else if (effects & EF_PLASMAMZF)
			{
				CL_PlasmaFlashParticle(ent.origin, ent.angles, false);
			}
			else if (effects & EF_ROCKETMZF)
			{
				CL_RocketMuzzle(ent.origin, ent.angles, false);
			}
			else if (effects & EF_MEMZF)
			{
				CL_MEMuzzle(ent.origin, ent.angles, false);
			}
			else if (effects & EF_FLAMETHROWER)
			{
				if(runFire)
					CL_FlameThrower (ent.origin, ent.angles, false);
			}
			else if (effects & EF_FIRE)
			{
				if(runFire)
					CL_FireParticles(ent.origin);
			}
		}

		//Ctf flag particle effects
		COM_StripExtension ( cl.configstrings[CS_MODELS+(s1->modelindex)], shortname );
		if (!Q_strcasecmp (shortname, "models/items/flags/flag1")) 
		{
			CL_FlagEffects(ent.origin, 0);
			ent.flag = 1;
			ent.nodraw = 1;
		}
		else if (!Q_strcasecmp (shortname, "models/items/flags/flag2")) 
		{
			CL_FlagEffects(ent.origin, 1);
			ent.flag = 2;
			ent.nodraw = 1;
		}
		
		if (s1->modelindex != 0 && !(renderfx & RF_NODRAW))
		{
			// add to refresh list
			V_AddEntity (&ent);

			// color shells generate a seperate entity for the main model
			if ((effects & EF_COLOR_SHELL) && !(s1->number == cl.playernum+1))
			{
				//replace player color shells for powerups with floating gfx effect
				if((effects & EF_QUAD) && playermodel)
				{
					CL_PoweredEffects (ent.origin, EF_QUAD);					
				}
				else if((effects & EF_PENT) && playermodel)
				{
					CL_PoweredEffects (ent.origin, EF_PENT);
				}
				else
				{
					ent.flags = renderfx | RF_TRANSLUCENT;
					ent.alpha = 0.30;
					V_AddEntity (&ent);		
				}
			}
		}

		ent.skin = NULL;		// never use a custom skin on others
		ent.skinnum = 0;
		ent.flags = 0;
		ent.alpha = 0;
		ent.lod1 = NULL;		// only player models get lods
		ent.lod2 = NULL;
		ent.team = 0;
		ent.nodraw = 0;

		ci = &cl.clientinfo[s1->skinnum & 0xff];

		if (s1->modelindex != 0 && !(renderfx & RF_NODRAW))
		{
			if (!Q_strcasecmp (shortname, "models/weapons/g_rocket/tris")) 
			{
				//add clear cover
				if (!cl_simpleitems->integer)
				{
					ent.model = R_RegisterModel("models/weapons/g_rocket/cover.iqm");
					ent.flags |= RF_TRANSLUCENT;
					ent.alpha = 0.30;
					V_AddEntity (&ent);
				}
			}	
			if (!Q_strcasecmp (shortname, "models/weapons/g_disruptor/tris")) 
			{
				//add clear cover
				if (!cl_simpleitems->integer)
				{
					ent.model = R_RegisterModel("models/weapons/g_disruptor/cover.iqm");
					ent.flags |= RF_TRANSLUCENT;
					ent.alpha = 0.30;
					V_AddEntity (&ent);
				}
			}
		}

		if (s1->modelindex2)
		{
			if (s1->modelindex2 == 255 || (ci->helmet && playermodel))
			{	// custom weapon

				i = (s1->skinnum >> 8); // 0 is default weapon model
				if (!cl_vwep->value || i > MAX_CLIENTWEAPONMODELS - 1)
					i = 0;
				ent.model = ci->weaponmodel[i];
				if (!ent.model) 
				{
					if (i != 0)
						ent.model = ci->weaponmodel[0];
					if (!ent.model)
						ent.model = cl.baseclientinfo.weaponmodel[0];
				}
				
				//set team of this weapon model
				if (effects & EF_TEAM1)
					ent.team = 1;
				else if (effects & EF_TEAM2)
				{
					if(server_is_team)
						ent.team = 2;
					else
						ent.team = 3;
				}
			}
			else 
			{
				ent.model = cl.model_draw[s1->modelindex2];
				if(playermodel)
				{
					if (effects & EF_TEAM1)
						ent.team = 1;
					else if (effects & EF_TEAM2)
					{
						if(server_is_team)
							ent.team = 2;
						else
							ent.team = 3;
					}
				}
			}
						
			//here is where we will set the alpha for certain model parts - would like to eventually
			//do something a little less uh, hardcoded.
			COM_StripExtension ( cl.configstrings[CS_MODELS+(s1->modelindex2)], shortname );
			if (!Q_strcasecmp (shortname, "models/items/healing/globe/tris"))
			{
				if(cl_simpleitems->value)
					continue;
				ent.alpha = 0.4;
				ent.flags = RF_TRANSLUCENT;
			}
			else if (!Q_strcasecmp (shortname, "models/items/quaddama/unit"))
			{
				if(cl_simpleitems->value)
					continue;
				ent.alpha = 0.4;
				ent.flags = RF_TRANSLUCENT;
			}
			else if (!Q_strcasecmp (shortname, "models/items/adrenaline/glass"))
			{
				if(cl_simpleitems->value)
					continue;
				ent.alpha = 0.4;
				ent.flags = RF_TRANSLUCENT;
			}

			if (s1->number == cl.playernum+1) 
				ent.flags |= RF_VIEWERMODEL;

			V_AddEntity (&ent);
			
			if (s1->modelindex != 0 && !(renderfx & RF_NODRAW))
			{				
				if(ent.frame < 198)
				{
					//if frame is not death, set the model effect
					if((effects & EF_QUAD) && playermodel)
					{
						ent.model = R_RegisterModel("models/items/activated/double/tris.iqm");
						V_AddEntity (&ent);		
					
						ent.model = R_RegisterModel("models/items/activated/double/glass.iqm");
						ent.flags |= RF_TRANSLUCENT;
						ent.alpha = 0.30;
						V_AddEntity (&ent);		
					}
					else if((effects & EF_PENT) && playermodel)
					{
						ent.model = R_RegisterModel("models/items/activated/force/tris.iqm");
						V_AddEntity (&ent);		
					}
					else if((effects & EF_SPAWNPROTECTED) && playermodel)
					{
						ent.model = R_RegisterModel("models/objects/powerdome/tris.iqm");
						ent.flags |= RF_NOSHADOWS;
						ent.team = 0;
						if (!r_shaders->integer)
						{
							ent.flags |= RF_TRANSLUCENT;
							ent.flags |= RF_SHELL_BLUE;
							ent.alpha = 0.30;
						}
						V_AddEntity (&ent);		
					}
				}
			}		

			//PGM - make sure these get reset.
			ent.flags = 0;
			ent.alpha = 0;
			//PGM

		}

		if (s1->modelindex3 || (ci->helmet && playermodel)) //index 3 is only used for clear entities now, no need for all that checking
		{
			if(ci->helmet && playermodel)
				ent.model = ci->helmet;
			else
				ent.model = cl.model_draw[s1->modelindex3];

			ent.alpha = 0.4;
			ent.flags = RF_TRANSLUCENT;
			
			if (s1->number == cl.playernum+1) {
				ent.flags |= RF_VIEWERMODEL;
			}

			if (ent.model) // Special hack for legacy servers and nonexistant helmets
				V_AddEntity (&ent);
		}

		if (s1->number == cl.playernum+1)
			goto end;

		if (s1->modelindex4)
		{
			ent.flags = 0;
			ent.alpha = 1.0;

			//Ctf flag effects
			COM_StripExtension ( cl.configstrings[CS_MODELS+(s1->modelindex4)], shortname );
			if (!Q_strcasecmp (shortname, "models/items/flags/flag1")) 
			{
				CL_FlagEffects(ent.origin, 0);
				ent.model = 0;
			}
			else if (!Q_strcasecmp (shortname, "models/items/flags/flag2")) 
			{
				CL_FlagEffects(ent.origin, 1);
				ent.model = 0;
			}
			else 
			{
				ent.model = cl.model_draw[s1->modelindex4];
				V_AddEntity (&ent);
			}
		}

		// add automatic particle trails
		if ( (effects&~EF_ROTATE) )
		{
			if (effects & EF_ROCKET)
			{
				CL_RocketTrail (cent->lerp_origin, ent.origin, cent);
				V_AddLight (ent.origin, 200, .4, .4, .1);
			}
			if (effects & EF_SHIPEXHAUST)
			{
				CL_ShipExhaust (cent->lerp_origin, ent.origin, cent);
			}
			if (effects & EF_ROCKETEXHAUST)
			{
				CL_RocketExhaust (cent->lerp_origin, ent.origin, cent);
			}
			else if (effects & EF_HYPERBLASTER)
			{
				V_AddLight (ent.origin, 400*crand(), 1, 0, 1);
			}
			else if (effects & EF_GIB)
			{
				CL_DiminishingTrail (cent->lerp_origin, ent.origin, cent, effects);
			}
			//we can leave these effects as an additional option for those who really have eye issues
			else if ((effects & EF_TEAM1) && cl_dmlights->integer)
			{
				vec3_t right;
				vec3_t start;
				vec3_t up;

				AngleVectors (ent.angles, NULL, right, up);
				VectorMA (ent.origin, 16, right, start);
				VectorMA (start, 32, up, start);
				CL_RedTeamLight(start);

				VectorMA (ent.origin, -16, right, start);
				VectorMA (start, 32, up, start);
				CL_RedTeamLight(start);
			}
			else if ((effects & EF_TEAM2) && cl_dmlights->integer)
			{
				vec3_t right;
				vec3_t start;
				vec3_t up;

				AngleVectors (ent.angles, NULL, right, up);
				VectorMA (ent.origin, 16, right, start);
				VectorMA (start, 32, up, start);
				CL_BlueTeamLight(start);

				VectorMA (ent.origin, -16, right, start);
				VectorMA (start, 32, up, start);
				CL_BlueTeamLight(start);
			}

			else if (effects & EF_GREENGIB)
			{
				CL_DiminishingTrail (cent->lerp_origin, ent.origin, cent, effects);
			}
			else if (effects & EF_PLASMA)
			{
				CL_BlasterTrail (cent->lerp_origin, ent.origin, 0x72);
				V_AddLight (ent.origin, 200, 0, .3, .5);
			}
			else if (effects & EF_BLASTER)
			{
				CL_BlasterTrail (cent->lerp_origin, ent.origin, 0xd0);
				V_AddLight (ent.origin, 200, 0, .5, .1);
			}
		}
end:
		VectorCopy (ent.origin, cent->lerp_origin);
	}
}

/*
=======================
Berserker@quake2  shell
brass effect. No change
=======================
*/

void CL_BrassShells(vec3_t org, vec3_t dir, int count)
{
	int i, j;
	clentity_t *le;
	float d;

	if (!cl_brass->value || !count)
		return;

	for (i = 0; i < count; i++) {

		if (!free_clentities)
			return;

		le = free_clentities;
		free_clentities = le->next;
		le->next = active_clentities;
		active_clentities = le;
		le->time = cl.time;
		d = (192 + rand()) & 63;
		VectorClear(le->accel);
		VectorClear(le->vel);
		le->accel[0] = le->accel[1] = 0;
		le->accel[2] = -6 * PARTICLE_GRAVITY;
		le->alpha = 1.0;
		le->alphavel = -0.1;
		le->flags = CLM_BOUNCE | CLM_FRICTION | CLM_ROTATE | CLM_NOSHADOW | CLM_BRASS;
		le->model = R_RegisterModel("models/objects/brass/tris.iqm");
		le->ang = crand() * 360;
		le->avel = crand() * 500;

		for (j = 0; j < 3; j++) {
			le->lastOrg[j] = le->org[j] = org[j];
			le->vel[j] = crand() * 24 + d * dir[j];
		}
	}
}

void CL_GlassShards(vec3_t org, vec3_t dir, int count)
{
	int i, j;
	clentity_t *le;
	float d;

	if (!count)
		return;

	for (i = 0; i < count; i++) {

		if (!free_clentities)
			return;

		le = free_clentities;
		free_clentities = le->next;
		le->next = active_clentities;
		active_clentities = le;
		le->time = cl.time;
		d = (192 + rand()) & 13;
		VectorClear(le->accel);
		VectorClear(le->vel);
		le->accel[0] = le->accel[1] = 0;
		le->accel[2] = -3 * PARTICLE_GRAVITY;
		le->alpha = 1.0;
		le->alphavel = -0.01;
		le->flags = CLM_BOUNCE | CLM_FRICTION | CLM_ROTATE | CLM_NOSHADOW | CLM_GLASS;
		le->model = R_RegisterModel("models/objects/debris1/tris.iqm");
		le->ang = crand() * 360;
		le->avel = crand() * 500;

		for (j = 0; j < 3; j++) {
			le->lastOrg[j] = le->org[j] = org[j];
			le->vel[j] = crand() * 12 + d * dir[j];
		}
	}
}

/*
==============
CL_AddViewWeapon
==============
*/

static int gunPrevFrame = 0;
static int gunFrameTime = 0;
static int lastFireFrameTime = 0;
static qboolean fire = false;
static particle_t *last_blue_flame = NULL;
static particle_t *last_flame = NULL;
void CL_AddViewWeapon (player_state_t *ps, player_state_t *ops)
{
	entity_t	gun;		// view model
	int			i;
	qboolean	useFX = false;
	vec3_t		offset_down;
	vec3_t		offset_right;

	if (cl.frame.playerstate.stats[STAT_ZOOMED])
		return;

	memset (&gun, 0, sizeof(gun));

	gun.model = cl.model_draw[ps->gunindex];

	gun.lod1 = NULL;
	gun.lod2 = NULL;

	if (!gun.model)
		return;

	// nudge gun down and right if in wide angle view
	if(cl.refdef.fov_x > 90)
	{
		VectorScale(cl.v_up, 0.2 * (cl.refdef.fov_x - 90), offset_down);
		VectorScale(cl.v_right, -0.15 * (cl.refdef.fov_x - 90), offset_right);
	}
	else
		offset_down[0] = offset_down[1] = offset_down[2] = offset_right[0] = offset_right[1] = offset_right[2] = 0;

	// set up gun position
	for (i=0 ; i<3 ; i++)
	{
		gun.origin[i] = cl.refdef.vieworg[i] + ops->gunoffset[i]
			+ cl.lerpfrac * (ps->gunoffset[i] - ops->gunoffset[i]);
		gun.angles[i] = cl.refdef.viewangles[i] + LerpAngle (ops->gunangles[i],
			ps->gunangles[i], cl.lerpfrac);
	}

	gun.frame = ps->gunframe;
	if (gun.frame == 0)
		gun.oldframe = 0;	// just changed weapons, don't lerp from old
	else
		gun.oldframe = ops->gunframe;

	// animation timestamps are set here
	if(gunPrevFrame != ps->gunframe) 
	{
		gunPrevFrame = ps->gunframe;
		gun.frametime = gunFrameTime = Sys_Milliseconds();
	}
	else 
	{
		gun.frametime = gunFrameTime;		
	}

	if(Sys_Milliseconds() - lastFireFrameTime > 30)
	{
		lastFireFrameTime = Sys_Milliseconds();
		fire = true;
	}
	else
		fire = false;

	VectorSubtract(gun.origin, offset_down, gun.origin);
	VectorSubtract(gun.origin, offset_right, gun.origin);
	
	gun.flags = RF_MINLIGHT | RF_DEPTHHACK | RF_WEAPONMODEL;
	gun.backlerp = 1.0 - cl.lerpfrac;
	VectorCopy (gun.origin, gun.oldorigin);	// don't lerp at all

	// add an attached muzzleflash for chaingun
	if(!(strcmp("models/weapons/v_chaingun/tris.iqm", gun.model->name))) 
	{
		if(gun.frame > 4 && gun.frame < 14)
			CL_MuzzleFlashParticle(gun.origin, gun.angles, true);
	}	
	else if(!(strcmp("models/weapons/v_disruptor/tris.iqm", gun.model->name))) 
	{
		if(gun.frame == 6) 
		{
			CL_PlasmaFlashParticle(gun.origin, gun.angles, true);
			useFX = true;
		}
	}
	else if(!(strcmp("models/weapons/v_beamgun/tris.iqm", gun.model->name))) 
	{
		if(gun.frame > 6 && gun.frame < 25) 
		{
			CL_BlasterMuzzleParticles (gun.origin, gun.angles, 0xd4, 0.4, true);
		}
	}
	else if(!(strcmp("models/weapons/v_alienblast/tris.iqm", gun.model->name))) 
	{
		if(gun.frame == 6) 
		{
			CL_BlasterMuzzleParticles (gun.origin, gun.angles, 0xd4, 0.7, true);
			useFX = true;
		}
	}
	else if(!(strcmp("models/weapons/v_blast/tris.iqm", gun.model->name))) 
	{
		if(gun.frame == 6) 
		{
			CL_BlasterMuzzleParticles (gun.origin, gun.angles, 0x74, 0.7, true);
			useFX = true;
		}
	}
	else if(!(strcmp("models/weapons/v_violator/tris.iqm", gun.model->name))) 
	{
		if(gun.frame == 6 || gun.frame == 8 || gun.frame == 10 || gun.frame == 12) 
		{
			CL_LightningBall (gun.origin, gun.angles, 0x74, true);
			R_ApplyForceToRagdolls(gun.origin, -50);
		}
	}
	else if(!(strcmp("models/weapons/v_smartgun/tris.iqm", gun.model->name))) 
	{
		if(gun.frame == 6) 
		{
			CL_SmartMuzzle (gun.origin, gun.angles, true);
		}
	}
	else if(!(strcmp("models/weapons/v_rocket/tris.iqm", gun.model->name))) 
	{
		if(gun.frame == 7) 
		{
			CL_RocketMuzzle (gun.origin, gun.angles, true);
		}
	}
	else if(!(strcmp("models/weapons/v_minderaser/tris.iqm", gun.model->name))) 
	{
		if(gun.frame == 7) 
		{
			CL_MEMuzzle (gun.origin, gun.angles, true);
		}
	}
	else if (!strcmp("models/weapons/v_flamethrower/tris.iqm", gun.model->name) && gun.frame > 5 && gun.frame < 19 && fire)
	{
		CL_FlameThrower (gun.origin, gun.angles, true);
	}
	
	// both of these two effects look best at 30fps, so limit them to roughly that.
	if (!strcmp("models/weapons/v_flamethrower/tris.iqm", gun.model->name) && gun.frame > 18 && gun.frame < 50 && fire)
		last_blue_flame = CL_BlueFlameParticle (gun.origin, gun.angles, last_blue_flame);
	else if(fire)
		last_blue_flame = NULL;

	if (!strcmp("models/weapons/v_flamethrower/tris.iqm", gun.model->name) && gun.frame > 5 && gun.frame < 19 && fire)
		last_flame = CL_FlameThrowerParticle (gun.origin, gun.angles, last_flame);
	else if(fire)
		last_flame = NULL;

	V_AddViewEntity (&gun);

	// add shells
	{
		int oldeffects = gun.flags, pnum;
		entity_state_t	*s1;

		for (pnum = 0 ; pnum<cl.frame.num_entities ; pnum++)
			if ((s1=&cl_parse_entities[(cl.frame.parse_entities+pnum)&(MAX_PARSE_ENTITIES-1)])->number == cl.playernum+1)
			{
				int effects = s1->renderfx;

				if (effects & (RF_SHELL_RED|RF_SHELL_BLUE|RF_SHELL_GREEN) || s1->effects&(EF_PENT|EF_QUAD) || useFX)
				{
					if (effects & RF_SHELL_RED)
						gun.flags |= RF_SHELL_RED;
					if (effects & RF_SHELL_BLUE)
						gun.flags |= RF_SHELL_BLUE;
					if (effects & RF_SHELL_GREEN)
						gun.flags |= RF_SHELL_GREEN;

					if (useFX) 
						gun.flags |= RF_SHELL_GREEN;

					gun.flags |= RF_TRANSLUCENT;
					gun.alpha = 0.30;

					if ( ( s1->effects & EF_COLOR_SHELL && gun.flags & (RF_SHELL_RED|RF_SHELL_BLUE|RF_SHELL_GREEN) ) || useFX)
					{
						V_AddViewEntity (&gun);
					}
					if (s1->effects & EF_PENT)
					{
						gun.flags = oldeffects | RF_TRANSLUCENT | RF_SHELL_RED;


						V_AddViewEntity (&gun);
					}
					if (s1->effects & EF_QUAD && cl_gun->value)
					{
						gun.flags = oldeffects | RF_TRANSLUCENT | RF_SHELL_BLUE;

						V_AddViewEntity (&gun);
					}
				}
			}

		gun.flags = oldeffects;
	}

	// add glass and effect pieces
	
	if(!(strcmp("models/weapons/v_rocket/tris.iqm", gun.model->name))) 
	{
		gun.model = R_RegisterModel("models/weapons/v_rocket/cover.iqm");
		gun.flags |= RF_TRANSLUCENT;
		gun.alpha = 0.30;
		V_AddViewEntity (&gun);
	}
	else if(!(strcmp("models/weapons/v_disruptor/tris.iqm", gun.model->name))) 
	{
		int oldeffects = gun.flags;

		gun.model = R_RegisterModel("models/weapons/v_disruptor/cover.iqm");
		gun.flags |= RF_TRANSLUCENT;
		gun.alpha = 0.30;
		V_AddViewEntity (&gun);
		gun.flags = oldeffects;
		gun.alpha = 1.0;
		gun.model = R_RegisterModel("models/weapons/v_disruptor/effects.iqm");
		V_AddViewEntity (&gun);
	}
	else if(!(strcmp("models/weapons/v_smartgun/tris.iqm", gun.model->name))) 
	{
		gun.model = R_RegisterModel("models/weapons/v_smartgun/effects.iqm");
		V_AddViewEntity (&gun);
	}
	else if(!(strcmp("models/weapons/v_beamgun/tris.iqm", gun.model->name))) 
	{
		int oldeffects = gun.flags;
		gun.model = R_RegisterModel("models/weapons/v_beamgun/effects.iqm");
		gun.flags = oldeffects | RF_TRANSLUCENT | RF_SHELL_GREEN;
		gun.alpha = 0.50;
		V_AddViewEntity (&gun);
	}
	else if(!(strcmp("models/weapons/v_blast/tris.iqm", gun.model->name))) 
	{
		gun.model = R_RegisterModel("models/weapons/v_blast/cover.iqm");
		gun.flags |= RF_TRANSLUCENT;
		gun.alpha = 0.30;
		V_AddViewEntity (&gun);
	}
	else if(!(strcmp("models/weapons/v_alienblast/tris.iqm", gun.model->name))) 
	{
		gun.model = R_RegisterModel("models/weapons/v_alienblast/cover.iqm");
		gun.flags |= RF_TRANSLUCENT;
		gun.alpha = 0.30;
		V_AddViewEntity (&gun);
	}
}


/*
===============
CL_CalcViewValues

Sets cl.refdef view values
===============
*/
void CL_CalcViewValues (void)
{
	int			i;
	float		lerp, backlerp;
	centity_t	*ent;
	frame_t		*oldframe;
	player_state_t	*ps, *ops;

	// find the previous frame to interpolate from
	ps = &cl.frame.playerstate;
	i = (cl.frame.serverframe - 1) & UPDATE_MASK;
	oldframe = &cl.frames[i];
	if (oldframe->serverframe != cl.frame.serverframe-1 || !oldframe->valid)
		oldframe = &cl.frame;		// previous frame was dropped or involid
	ops = &oldframe->playerstate;

	// see if the player entity was teleported this frame
	if ( fabs(ops->pmove.origin[0] - ps->pmove.origin[0]) > 256*8
		|| abs(ops->pmove.origin[1] - ps->pmove.origin[1]) > 256*8
		|| abs(ops->pmove.origin[2] - ps->pmove.origin[2]) > 256*8)
		ops = ps;		// don't interpolate

	ent = &cl_entities[cl.playernum+1];
	lerp = cl.lerpfrac;

	// calculate the origin
	if ( !cl.attractloop && !ps->stats[STAT_CHASE] && cl_predict->value && !(cl.frame.playerstate.pmove.pm_flags & PMF_NO_PREDICTION))  //not for demo/chasecam
	{	// use predicted values
		unsigned	delta;

		backlerp = 1.0 - lerp;
		for (i=0 ; i<3 ; i++)
		{
			cl.refdef.vieworg[i] = cl.predicted_origin[i] + ops->viewoffset[i]
				+ cl.lerpfrac * (ps->viewoffset[i] - ops->viewoffset[i])
				- backlerp * cl.prediction_error[i];
		}

		// smooth out stair climbing
		delta = cls.realtime - cl.predicted_step_time;
		if (delta < 100)
			cl.refdef.vieworg[2] -= cl.predicted_step * (100 - delta) * 0.01;
	}
	else
	{	// just use interpolated values
		for (i=0 ; i<3 ; i++)
			cl.refdef.vieworg[i] = ops->pmove.origin[i]*0.125 + ops->viewoffset[i]
				+ lerp * (ps->pmove.origin[i]*0.125 + ps->viewoffset[i]
				- (ops->pmove.origin[i]*0.125 + ops->viewoffset[i]) );
	}

	// if not running a demo or on a locked frame, add the local angle movement
	if ( cl.frame.playerstate.pmove.pm_type < PM_DEAD )
	{	// use predicted values
		for (i=0 ; i<3 ; i++)
			cl.refdef.viewangles[i] = cl.predicted_angles[i];
	}
	else
	{	// just use interpolated values
		for (i=0 ; i<3 ; i++)
			cl.refdef.viewangles[i] = LerpAngle (ops->viewangles[i], ps->viewangles[i], lerp);
	}

	for (i=0 ; i<3 ; i++)
		cl.refdef.viewangles[i] += LerpAngle (ops->kick_angles[i], ps->kick_angles[i], lerp);

	AngleVectors (cl.refdef.viewangles, cl.v_forward, cl.v_right, cl.v_up);

	// interpolate field of view
	cl.refdef.fov_x = ops->fov + lerp * (ps->fov - ops->fov);

	// don't interpolate blend color
	for (i=0 ; i<4 ; i++)
		cl.refdef.blend[i] = ps->blend[i];

	// add the weapon
	CL_AddViewWeapon (ps, ops);
}

/*
===============
CL_AddEntities

Emits all entities, particles, and lights to the refresh
===============
*/
void CL_AddEntities (void)
{
	float FRAMETIME = 1.0f/(float)server_tickrate;

	if (cls.state != ca_active)
		return;

	if (cl.time > cl.frame.servertime)
	{
		if (cl_showclamp->value)
			Com_Printf ("high clamp %i\n", cl.time - cl.frame.servertime);
		cl.time = cl.frame.servertime;
		cl.lerpfrac = 1.0;
	}
	else if (cl.time < cl.frame.servertime - (100.0f * 0.1f/FRAMETIME))
	{
		if (cl_showclamp->value)
			Com_Printf ("low clamp %i\n", cl.frame.servertime-(100.0f * 0.1f/FRAMETIME) - cl.time);
		cl.time = cl.frame.servertime - (100.0f * FRAMETIME/0.1f);
		cl.lerpfrac = 0;
	}
	else
		cl.lerpfrac = 1.0 - ((cl.frame.servertime - cl.time) * 0.01f * 0.1f/FRAMETIME);

	if (cl_timedemo->value)
		cl.lerpfrac = 1.0;

	CL_CalcViewValues ();

	// PMM - moved this here so the heat beam has the right values for the vieworg, and can lock the beam to the gun
	CL_AddPacketEntities (&cl.frame);
	CL_AddTEnts ();
	CL_AddParticles ();
	CL_AddDLights ();
	CL_AddClEntities();
	CL_AddLightStyles ();
}

/*
===============
CL_GetEntitySoundOrigin

Called to get the sound spatialization origin
===============
*/
void CL_GetEntitySoundOrigin (int ent, vec3_t org)
{
	centity_t	*old;

	if (ent < 0 || ent >= MAX_EDICTS)
		Com_Error (ERR_DROP, "CL_GetEntitySoundOrigin: bad ent");
	old = &cl_entities[ent];
	VectorCopy (old->lerp_origin, org);

	// FIXME: bmodel issues...
}

/*
================
Client side entities(ejecting brass, etc
================
*/

/*
-----------------------------
ClipMoveEntitiesWorld
-----------------------------
*/
void CL_ClipMoveToEntitiesWorld(vec3_t start, vec3_t mins, vec3_t maxs,
								vec3_t end, trace_t * tr, int mask)
{
	int i;
	trace_t trace;
	int headnode;
	float *angles;
	entity_state_t *ent;
	int num;
	cmodel_t *cmodel;

	for (i = 0; i < cl.frame.num_entities; i++) {
		num = (cl.frame.parse_entities + i) & (MAX_PARSE_ENTITIES - 1);
		ent = &cl_parse_entities[num];

		if (!ent->solid)
			continue;

		if (ent->solid != 31)	// special value for bmodel
			continue;

		cmodel = cl.model_clip[ent->modelindex];
		if (!cmodel)
			continue;
		headnode = cmodel->headnode;
		angles = ent->angles;

		if (tr->allsolid)
			return;

		trace =
			CM_TransformedBoxTrace(start, end, mins, maxs, headnode, mask,
								   ent->origin, angles);

		if (trace.allsolid || trace.startsolid
			|| trace.fraction < tr->fraction) {
			trace.ent = (struct edict_s *) ent;
			if (tr->startsolid) {
				*tr = trace;
				tr->startsolid = true;
			} else
				*tr = trace;
		} else if (trace.startsolid)
			tr->startsolid = true;
	}
}


trace_t CL_PMTraceWorld(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end,
						int mask)
{
	trace_t t;

	// check against world
	t = CM_BoxTrace(start, end, mins, maxs, 0, mask);
	if (t.fraction < 1.0)
		t.ent = (struct edict_s *) 1;

	// check all other solid models
	CL_ClipMoveToEntitiesWorld(start, mins, maxs, end, &t, mask);

	return t;
}

void VectorReflect(const vec3_t v, const vec3_t normal, vec3_t out)
{
	float d;

	d = 2.0 * (v[0] * normal[0] + v[1] * normal[1] + v[2] * normal[2]);

	out[0] = v[0] - normal[0] * d;
	out[1] = v[1] - normal[1] * d;
	out[2] = v[2] - normal[2] * d;
}

extern int CL_PMpointcontents (vec3_t point);
void CL_AddClEntities()
{
	entity_t ent;
	clentity_t *le, *next;
	clentity_t *active, *tail;
	vec3_t org, dir;
	float alpha, bak;
	int contents;
	qboolean onground;
	char soundname[64];
	float time, time2, grav = Cvar_VariableValue("sv_gravity");
	static vec3_t mins = { -2, -2, -2 }; 
    static vec3_t maxs = { 2, 2, 2 }; 
	
	if (!grav)
		grav = 1;
	else
		grav /= 800;

	active = NULL;
	tail = NULL;

	memset(&ent, 0, sizeof(ent));

	for (le = active_clentities; le; le = next) {
		next = le->next;

		time = (cl.time - le->time) * 0.001; 
		alpha = le->alpha + time * le->alphavel;

		if (alpha <= 0) {		// faded out
			le->next = free_clentities;
			free_clentities = le;
			continue;
		}

		time2 = time * time;
		org[0] = le->org[0] + le->vel[0] * time + le->accel[0] * time2;
		org[1] = le->org[1] + le->vel[1] * time + le->accel[1] * time2;
		bak = le->org[2] + le->vel[2] * time + le->accel[2] * time2 * grav;

		org[2] = bak - 1;
		contents = CL_PMpointcontents(org);
		org[2] = bak;
		onground = contents & MASK_SOLID;
		if (onground) {
			le->flags &= ~CLM_ROTATE;
			le->avel = 0;
		} else if (le->flags & CLM_STOPPED) {
			le->flags &= ~CLM_STOPPED;
			le->flags |= CLM_BOUNCE;
			le->accel[2] = -15 * PARTICLE_GRAVITY;
			// Reset
			le->alpha = 1;
			le->time = cl.time;
			VectorCopy(org, le->org);
		}

		le->next = NULL;
		if (!tail)
			active = tail = le;
		else {
			tail->next = le;
			tail = le;
		}

		if (alpha > 1.0)
			alpha = 1;
		
		if(le->flags & CLM_NOSHADOW)
			ent.flags |= RF_NOSHADOWS;

		if (le->flags & CLM_FRICTION) {
			// Water friction affected cl model
			if (contents & MASK_WATER) {
				if (contents & CONTENTS_LAVA) {	// kill entity in lava
					VectorSet(dir, 0, 0, 1);
					le->alpha = 0;
					continue;
				} else {
					// Add friction
					VectorScale(le->vel, 0.25, le->vel);
					VectorScale(le->accel, 0.25, le->accel);

					// Don't add friction again
					le->flags &= ~(CLM_FRICTION | CLM_ROTATE);

					// Reset
					le->time = cl.time;
					VectorCopy(org, le->org);
				}
			}
		}
		
		if (le->flags & CLM_BOUNCE)
		{
			trace_t trace = CL_PMTraceWorld (le->lastOrg, mins, maxs, org, MASK_SOLID);

			if (trace.fraction > 0.0001f && trace.fraction < 0.9999f )
			{
				vec3_t	vel;
				// Reflect velocity
				float time =
					cl.time - (cls.frametime +
							   cls.frametime * trace.fraction) * 1000;
				time = (time - le->time) * 0.001;

				VectorSet(vel, le->vel[0], le->vel[1], le->vel[2] + le->accel[2] * time * grav);
				VectorReflect(vel, trace.plane.normal, le->vel);
				
				// Check for stop or slide along the plane
				if (trace.plane.normal[2] > 0 && le->vel[2] < 1)
				{
					if (trace.plane.normal[2] > 0.9)
					{
						VectorClear(le->vel);
						VectorClear(le->accel);
						le->avel = 0;
						le->flags &= ~CLM_BOUNCE;
						le->flags |= CLM_STOPPED;
					}
					else
					{
						// FIXME: check for new plane or free fall
						float dot = DotProduct(le->vel, trace.plane.normal);
						VectorMA(le->vel, -dot, trace.plane.normal, le->vel);

						dot = DotProduct(le->accel, trace.plane.normal);
						VectorMA(le->accel, -dot, trace.plane.normal, le->accel);
					}
				}

				VectorCopy(trace.endpos, org);

				// Reset
				le->time = cl.time;
				VectorCopy(org, le->org);

				//play a sound if brass
				if ( (le->flags & (CLM_BRASS))  &&  (le->flags & (CLM_BOUNCE)) ) {
					Com_sprintf(soundname, sizeof(soundname), "weapons/clink0%i.wav", (rand() % 2) + 1);
					S_StartSound (le->org, 0, CHAN_WEAPON, S_RegisterSound(soundname), 1.0, ATTN_NORM, 0);
				}

				//play a sound if glass
				if ( (le->flags & (CLM_GLASS))  &&  (le->flags & (CLM_BOUNCE)) ) {
					Com_sprintf(soundname, sizeof(soundname), "weapons/clink0%i.wav", (rand() % 2) + 1); //to do - get glass sound
					S_StartSound (le->org, 0, CHAN_WEAPON, S_RegisterSound(soundname), 1.0, ATTN_NORM, 0);
				}
			}
		}
		// Save current origin if needed
		if (le->flags & (CLM_BOUNCE)) {
			VectorCopy(le->lastOrg, ent.origin);
			VectorCopy(org, le->lastOrg);	// FIXME: pause
		} else
			VectorCopy(org, ent.origin);

		if (CL_PMpointcontents(ent.origin) & MASK_SOLID) {	// kill entity
															// in solid
			le->alpha = 0;
			continue;
		}
		
		ent.model = le->model;
		if (!ent.model)
			continue;

		ent.lod1 = NULL;
		ent.lod2 = NULL;

		ent.angles[0] = le->ang +
			((le->flags & CLM_ROTATE) ? (time * le->avel) : 0);;
		ent.angles[2] = le->ang +
			((le->flags & CLM_ROTATE) ? (time * le->avel) : 0);;
		ent.angles[1] =
			le->ang +
			((le->flags & CLM_ROTATE) ? (time * le->avel) : 0);

		ent.frame = ent.oldframe = 0;

		if(le->flags & CLM_GLASS)
			ent.flags |= RF_TRANSLUCENT;
			
		V_AddEntity(&ent);
		
	}

	active_clentities = active;
}

void CL_ClearClEntities()
{
	int i;

	free_clentities = &clentities[0];
	active_clentities = NULL;

	for (i = 0; i < MAX_CLENTITIES-1; i++)
		clentities[i].next = &clentities[i + 1];
	clentities[MAX_CLENTITIES - 1].next = NULL;
}
