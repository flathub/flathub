/*
Copyright (C) 2009 COR Entertainment, LLC.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "g_local.h"

/*
============
G_ResetHistory

Clear out the given client's history (should be called when the teleport bit is flipped)
============
*/
void G_ResetHistory( edict_t *ent ) 
{
	int		i, time;

	// fill up the history with data (assume the current position)
	ent->client->historyHead = NUM_CLIENT_HISTORY_FOR_CURRENT_TICKRATE;
	for ( i = ent->client->historyHead, time = level.leveltime; i >= 0; i--, time -= FRAMETIME_MS ) 
	{
		VectorCopy( ent->mins, ent->client->history[i].mins );
		VectorCopy( ent->maxs, ent->client->history[i].maxs );
		VectorCopy( ent->s.origin, ent->client->history[i].currentOrigin );
		ent->client->history[i].leveltime = time;
	}
}


/*
============
G_StoreHistory

Keep track of where the client's been
============
*/
void G_StoreHistory( edict_t *ent ) 
{
	int		head;

	ent->client->historyHead++;
	if ( ent->client->historyHead > NUM_CLIENT_HISTORY_FOR_CURRENT_TICKRATE )
	{
		ent->client->historyHead = 0;
	}
	
	head = ent->client->historyHead;

	// store all the collision-detection info and the time
	VectorCopy( ent->mins, ent->client->history[head].mins );
	VectorCopy( ent->maxs, ent->client->history[head].maxs );
	VectorCopy( ent->s.origin, ent->client->history[head].currentOrigin );
	SnapVector( ent->client->history[head].currentOrigin );
	ent->client->history[head].leveltime = level.leveltime;
}


/*
=============
TimeShiftLerp

Used below to interpolate between two previous vectors
Returns a vector "frac" times the distance between "start" and "end"
=============
*/
static void TimeShiftLerp( float frac, vec3_t start, vec3_t end, vec3_t result ) 
{
	result[0] = start[0] + frac * ( end[0] - start[0] );
	result[1] = start[1] + frac * ( end[1] - start[1] );
	result[2] = start[2] + frac * ( end[2] - start[2] );
}


/*
=================
G_TimeShiftClient

Move a client back to where he was at the specified time
=================
*/
void G_TimeShiftClient( edict_t *ent, int time, qboolean debug, edict_t *debugger ) 
{
	int		j, k;
	char	str[MAX_STRING_CHARS];	
	int 	failSafeCounter;

	// Fix for rocket funround crash/loop,
	// when time has value 0 it gets stuck in the do/while loop below.
	if (time <= 0) {
		safe_cprintf(debugger, PRINT_HIGH, "G_TimeShiftClient: time <= 0, %d, exit\n", time);
		return;
	}

	if (ent->client->historyHead > NUM_CLIENT_HISTORY_FOR_CURRENT_TICKRATE) {
		// historyHead should never be larger than this value, for example higher than 8 at tickrate 10 or 16 at tickrate 20
		safe_cprintf(debugger, PRINT_HIGH, "G_TimeShiftClient: historyHead larger than expected: historyHead %d, historyHead leveltime: %i, FRAMETIME: %f, tickrate %f, exit\n",
			ent->client->historyHead, ent->client->history[ent->client->historyHead].leveltime, FRAMETIME, 1.0 / FRAMETIME);

		return;
	}

	if(g_antilagdebug->integer > 1) 
	{ 
		//debug
		Com_sprintf(str, sizeof(str), "head: %i, %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i time: %i\n",
			ent->client->historyHead,
			ent->client->history[0].leveltime,
			ent->client->history[1].leveltime,
			ent->client->history[2].leveltime,
			ent->client->history[3].leveltime,
			ent->client->history[4].leveltime,
			ent->client->history[5].leveltime,
			ent->client->history[6].leveltime,
			ent->client->history[7].leveltime,
			ent->client->history[8].leveltime,
			ent->client->history[9].leveltime,
			ent->client->history[10].leveltime,
			ent->client->history[11].leveltime,
			ent->client->history[12].leveltime,
			ent->client->history[13].leveltime,
			ent->client->history[14].leveltime,
			ent->client->history[15].leveltime,
			ent->client->history[16].leveltime,
			time );
		safe_cprintf(debugger, PRINT_HIGH, "%s\n", str);
	}

	// find two entries in the history whose times sandwich "time"
	// assumes no two adjacent records have the same timestamp
	j = k = ent->client->historyHead;
		
	failSafeCounter = 0;
	do {

		if ( ent->client->history[j].leveltime <= time )
			break;
		
		// Exit the loop in case the number of iterations is larger than the history
		if (failSafeCounter > NUM_CLIENT_HISTORY_FOR_CURRENT_TICKRATE + 1) {
			safe_cprintf(debugger, PRINT_HIGH, "*** Counter reached %d, exit loop and exit G_TimeShiftClient, g_antilagprojectiles switched off. ***", NUM_CLIENT_HISTORY_FOR_CURRENT_TICKRATE + 1);
			
			if (g_antilagdebug->integer <= 1) {
				// If not printed above already
				Com_sprintf(str, sizeof(str), "head: %i, %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i time: %i\n",
					ent->client->historyHead,
					ent->client->history[0].leveltime,
					ent->client->history[1].leveltime,
					ent->client->history[2].leveltime,
					ent->client->history[3].leveltime,
					ent->client->history[4].leveltime,
					ent->client->history[5].leveltime,
					ent->client->history[6].leveltime,
					ent->client->history[7].leveltime,
					ent->client->history[8].leveltime,
					ent->client->history[9].leveltime,
					ent->client->history[10].leveltime,
					ent->client->history[11].leveltime,
					ent->client->history[12].leveltime,
					ent->client->history[13].leveltime,
					ent->client->history[14].leveltime,
					ent->client->history[15].leveltime,
					ent->client->history[16].leveltime,
					time );
				safe_cprintf(debugger, PRINT_HIGH, "%s\n", str);
			}

			g_antilagprojectiles->integer = 0;
			Cvar_SetValue( "g_antilagprojectiles", 0);
			return;
		}

		k = j;
		j--;
		if ( j < 0 ) 
		{
			j = NUM_CLIENT_HISTORY_FOR_CURRENT_TICKRATE;
		}
		failSafeCounter++;
	}
	while ( j != ent->client->historyHead );
	
	// if we got past the first iteration above, we've sandwiched (or wrapped)
	if ( j != k ) 
	{
		if(g_antilagdebug->value) {
			safe_cprintf(debugger, PRINT_HIGH, "Head: %i, reconciled time at: %i, attacker ping: %i, client ping: %i\n",
				ent->client->historyHead, j, debugger->client->ping, ent->client->ping);
		}

		// make sure it doesn't get re-saved
		if ( ent->client->saved.leveltime != level.leveltime ) {
			VectorCopy( ent->mins, ent->client->saved.mins );
			VectorCopy( ent->maxs, ent->client->saved.maxs );
			VectorCopy( ent->s.origin, ent->client->saved.currentOrigin );
			ent->client->saved.leveltime = level.leveltime;
		}

		// if we haven't wrapped back to the head, we've sandwiched, so
		// we shift the client's position back to where he was at "time"
		if ( j != ent->client->historyHead ) 
		{
			float	frac = (float)(time - ent->client->history[j].leveltime) /
				(float)(ent->client->history[k].leveltime - ent->client->history[j].leveltime);

			// interpolate between the two origins to give position at time index "time"
			TimeShiftLerp( frac,
				ent->client->history[j].currentOrigin, ent->client->history[k].currentOrigin,
				ent->s.origin );

			// lerp these too, just for fun (and ducking)
			TimeShiftLerp( frac,
				ent->client->history[j].mins, ent->client->history[k].mins,
				ent->mins );

			TimeShiftLerp( frac,
				ent->client->history[j].maxs, ent->client->history[k].maxs,
				ent->maxs );

			// this will recalculate absmin and absmax
			gi.linkentity( ent );

		} else 
		{
			// we wrapped, so grab the earliest
			VectorCopy( ent->client->history[k].currentOrigin, ent->s.origin );
			VectorCopy( ent->client->history[k].mins, ent->mins );
			VectorCopy( ent->client->history[k].maxs, ent->maxs );

			// this will recalculate absmin and absmax
			gi.linkentity( ent );
		}
	}
}

/*
=====================
G_TimeShiftAllClients

Move ALL clients back to where they were at the specified "time",
except for "skip"
=====================
*/
void G_TimeShiftAllClients( int time, edict_t *skip ) 
{
	int			i;
	edict_t	*ent;

	for (i=0 ; i<g_maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse || !ent->client)
			continue;
		if (player_participating (ent) && ent != skip)
			G_TimeShiftClient (ent, time, false, skip);
	}
}


/*
================
G_DoTimeShiftFor

Decide what time to shift everyone back to, and do it
================
*/
void G_DoTimeShiftFor( edict_t *ent ) {

	//check this, because this will be different for alien arena for sure.
//	int wpflags[10] = { 0, 0, 2, 4, 0, 0, 8, 16, 0, 0 };

//	int wpflag = wpflags[ent->client->ps.weapon];
	int time;

	// don't time shift for mistakes or bots
	if ( !ent->inuse || !ent->client || ent->is_bot ) 
	{
		return;
	}

	// do the full lag compensation
	time = ent->client->attackTime - ent->client->ping - FRAMETIME_MS;
	//100 ms is our "built-in" lag due to the 10fps server frame

	if (g_antilagdebug->integer > 0) {
		Com_Printf("leveltime: %i, ping: %i, attackTime: %i, compensation: %i, corrected time: %i\n",
			level.leveltime, ent->client->ping, ent->client->attackTime, ent->client->ping + FRAMETIME_MS, time);
	}

	G_TimeShiftAllClients( time, ent );
}


/*
===================
G_UnTimeShiftClient

Move a client back to where he was before the time shift
===================
*/
void G_UnTimeShiftClient( edict_t *ent ) 
{
	// if it was saved
	if ( ent->client->saved.leveltime == level.leveltime ) {
		// move it back
		VectorCopy( ent->client->saved.mins, ent->mins );
		VectorCopy( ent->client->saved.maxs, ent->maxs );
		VectorCopy( ent->client->saved.currentOrigin, ent->s.origin );
		ent->client->saved.leveltime = 0;

		// this will recalculate absmin and absmax
		gi.linkentity( ent );
	}
}


/*
=======================
G_UnTimeShiftAllClients

Move ALL the clients back to where they were before the time shift,
except for "skip"
=======================
*/
void G_UnTimeShiftAllClients( edict_t *skip ) 
{
	int			i;
	edict_t	*ent;

	for (i=0 ; i<g_maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse || !ent->client)
			continue;
		if (player_participating (ent) && ent != skip)
			G_UnTimeShiftClient (ent);
	}
}


/*
==================
G_UndoTimeShiftFor

Put everyone except for this client back where they were
==================
*/
void G_UndoTimeShiftFor( edict_t *ent ) {

	// don't un-time shift for mistakes or bots
	if ( !ent->inuse || !ent->client || ent->is_bot ) 
	{
		return;
	}

	G_UnTimeShiftAllClients( ent );
}


/*
==================
G_AntilagProjectile

Simulate any extra frames to get the projectile "caught up" on the current
state of the game. 
==================
*/
void G_AntilagProjectile( edict_t *ent )
{
	int ping, time;
	edict_t *owner;
	
	// Save a copy of the player who fired the shot. The reason not to refer
	// to ent->owner directly is because if the projectile hits something,
	// its contents will be cleared during the call to G_RunEntity.
	owner = ent->owner;
	
	// don't antilag mistakes or bots
	if ( !ent || !ent->inuse || !owner || !owner->inuse || !owner->client || owner->is_bot ) 
	{
		return;
	}
	
	for (ping = owner->client->ping; ping > FRAMETIME_MS; ping -= FRAMETIME_MS)
	{
		// do the full lag compensation, without the "built in" lag
		time = owner->client->attackTime - ping; 
		if (g_antilagdebug->integer > 0)
		{
			Com_Printf("Full lag compensation, ping %d, time %d\n", ping, time);
		}		
		G_TimeShiftAllClients( time, owner );
		G_RunEntity (ent, FRAMETIME);
		G_UnTimeShiftAllClients( owner );
		if ( !ent->inuse )
			return;
	}

	time = owner->client->attackTime - ping; 
	if (g_antilagdebug->integer > 0)
	{
		Com_Printf("Default lag compensation, ping %d, time %d\n", ping, time);
	}		
	G_TimeShiftAllClients( time, owner );
	G_RunEntity (ent, (float)ping/1000.0f);
	G_UnTimeShiftAllClients( owner );
}
