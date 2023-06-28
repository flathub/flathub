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

JKD - 032207 - removed some very heinous things like bumping up the view
during jumps, and using the old "look at player" angles upon dying.

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "g_local.h"

void UpdateChaseCam(edict_t *ent)
{
	// is our chase target gone?
	if (!ent->client->chase_target->inuse
		|| !player_participating (ent->client->chase_target)) {
		edict_t *old = ent->client->chase_target;
		ChaseNext(ent);
		if (ent->client->chase_target == old) {
			ent->client->chase_target = NULL;
			ent->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
			return;
		}
	}
	
	gi.linkentity(ent);
}

void ChaseNext(edict_t *ent)
{
	int i;
	char clean_name[PLAYERNAME_SIZE];
	edict_t *e;

	if (!ent->client->chase_target)
		return;

	i = ent->client->chase_target - g_edicts;
	do {
		i++;
		if (i > g_maxclients->value)
			i = 1;
		e = g_edicts + i;
		if (!e->inuse)
			continue;
		if (player_participating (e))
			break;
	} while (e != ent->client->chase_target);

	ent->client->chase_target = e;
	ent->client->update_chase = true;
	G_CleanPlayerName( e->client->pers.netname, clean_name );
	safe_centerprintf(ent, "Following %s", clean_name);
}

void ChasePrev(edict_t *ent)
{
	int i;
	char clean_name[PLAYERNAME_SIZE];
	edict_t *e;

	if (!ent->client->chase_target)
		return;

	i = ent->client->chase_target - g_edicts;
	do {
		i--;
		if (i < 1)
			i = g_maxclients->value;
		e = g_edicts + i;
		if (!e->inuse)
			continue;
		if (player_participating (e))
			break;
	} while (e != ent->client->chase_target);

	ent->client->chase_target = e;
	ent->client->update_chase = true;
	G_CleanPlayerName( e->client->pers.netname, clean_name );
	safe_centerprintf(ent, "Following %s", clean_name);
}

void GetChaseTarget(edict_t *ent)
{
	int i;
	char clean_name[PLAYERNAME_SIZE];
	edict_t *other;

	for (i = 1; i <= g_maxclients->value; i++) {
		other = g_edicts + i;
		if (other->inuse && player_participating (other)) {
			ent->client->chase_target = other;
			ent->client->update_chase = true;
			G_CleanPlayerName( other->client->pers.netname, clean_name );
			safe_centerprintf(ent, "Following %s", clean_name);
			UpdateChaseCam(ent);
			return;
		}
	}
	safe_centerprintf(ent, "No other players to chase.");
}

