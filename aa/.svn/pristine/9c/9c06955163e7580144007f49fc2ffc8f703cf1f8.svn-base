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

#include "g_local.h"

int imageindex_sbfctf1;
int imageindex_sbfctf2;

/*
==================
CTFScoreboardMessage
==================
*/
void CTFScoreboardMessage (edict_t *ent, edict_t *killer, int mapvote)
{
	char	entry[1024];
	char	string[1400];
	int		len;
	int		i, j, k, x, y;
	int		sorted[2][MAX_CLIENTS];
	int		sortedscores[2][MAX_CLIENTS];
	int		score, total[2], totalscore[2];

	gclient_t	*cl;
	edict_t		*cl_ent;
	int team;
	int maxsize = 1024;
	gitem_t *flag1_item, *flag2_item;

	flag1_item = FindItemByClassname("item_flag_red");
	flag2_item = FindItemByClassname("item_flag_blue");

	// sort the clients by team and score
	total[0] = total[1] = 0;
	totalscore[0] = totalscore[1] = 0;
	for (i=0 ; i<game.maxclients ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse)
			continue;
		if (cl_ent->dmteam == RED_TEAM)
			team = 0;
		else if (cl_ent->dmteam == BLUE_TEAM)
			team = 1;
		else
			continue; // unknown team?

		score = game.clients[i].resp.score;
		for (j=0 ; j<total[team] ; j++)
		{
			if (score > sortedscores[team][j])
				break;
		}
		for (k=total[team] ; k>j ; k--)
		{
			sorted[team][k] = sorted[team][k-1];
			sortedscores[team][k] = sortedscores[team][k-1];
		}
		sorted[team][j] = i;
		sortedscores[team][j] = score;
		totalscore[team] += score;
		total[team]++;
	}

	// print level name and exit rules
	// add the clients in sorted order
	*string = 0;
	len = 0;

	if(ctf->value) {
		//do ctf
		sprintf(string, "newctfsb xv -16 yv -8 picn ctf1 "
			"xv +12 yv 4 num 3 21 "
			"xv 238 yv -8 picn ctf2 "
			"xv 264 yv -16 num 3 22 ");
		len = strlen(string);
	}
	else {
		//regular team games
		sprintf(string, "newctfsb xv -16 yv -8 picn team1 "
			"xv +12 yv 4 num 3 21 "
			"xv 238 yv -8 picn team2 "
			"xv 264 yv -16 num 3 22 ");
		len = strlen(string);
	}

	for (i=0 ; i<16 ; i++)
	{
		if (i >= total[0] && i >= total[1])
			break; // we're done

		*entry = 0;

		// left side
		if (i < total[0]) {
			cl = &game.clients[sorted[0][i]];
			cl_ent = g_edicts + 1 + sorted[0][i];

			sprintf(entry+strlen(entry),
				"ctf -96 %d %d %d %d ",
				42 + i * 16,
				sorted[0][i],
				cl->resp.score,
				cl->ping > 999 ? 999 : cl->ping);

			if (cl_ent->client->pers.inventory[ITEM_INDEX(flag2_item)])
				sprintf(entry + strlen(entry), "xv -92 yv %d picn sbfctf2 ",
					43 + i * 16);

			if (maxsize - len > strlen(entry)) {
				strcat(string, entry);
				len = strlen(string);
			}
		}

		// right side
		if (i < total[1]) {
			cl = &game.clients[sorted[1][i]];
			cl_ent = g_edicts + 1 + sorted[1][i];

			sprintf(entry+strlen(entry),
				"ctf 160 %d %d %d %d ",
				42 + i * 16,
				sorted[1][i],
				cl->resp.score,
				cl->ping > 999 ? 999 : cl->ping);

			if (cl_ent->client->pers.inventory[ITEM_INDEX(flag1_item)])
				sprintf(entry + strlen(entry), "xv 164 yv %d picn sbfctf1 ",
					43 + i * 16);

			if (maxsize - len > strlen(entry)) {
				strcat(string, entry);
				len = strlen(string);
			}
		}
	}

	if(mapvote) {
		y = 64;
		x = 96;
		Com_sprintf(entry, sizeof(entry),
			"xv %i yt %i string Vote ", x, y);
		if(maxsize - len > strlen(entry)) {
			strcat(string, entry);
			len = strlen(string);
		}
		x = 136;
		Com_sprintf(entry, sizeof(entry),
			"xv %i yt %i string for ", x, y);
		if(maxsize - len > strlen(entry)) {
			strcat(string, entry);
			len = strlen(string);
		}
		x = 168;
		Com_sprintf(entry, sizeof(entry),
			"xv %i yt %i string next ", x, y);
		if(maxsize - len > strlen(entry)) {
			strcat(string, entry);
			len = strlen(string);
		}
		x = 208;
		Com_sprintf(entry, sizeof(entry),
			"xv %i yt %i string map: ", x, y);
		if(maxsize - len > strlen(entry)) {
			strcat(string, entry);
			len = strlen(string);
		}
		x = 96;
		for(i=0; i<4; i++) {

			Com_sprintf(entry, sizeof(entry),
			"xv %i yt %i string %i.%s ", x, y+((i+1)*9)+9, i+1, votedmap[i].mapname);
			if(maxsize - len > strlen(entry)) {
				strcat(string, entry);
				len = strlen(string);
			}
		}

	}

	gi.WriteByte (svc_layout);
	gi.WriteString (string);
}
/*
 * Precache CTF items
 */

void CTFPrecache(void)
{
	imageindex_sbfctf1 =  gi.imageindex("sbfctf1");
	imageindex_sbfctf2 =  gi.imageindex("sbfctf2");
}


/*------------------------------------------------------------------------*/
/* GRAPPLE																  */
/*------------------------------------------------------------------------*/

// ent is player
void CTFPlayerResetGrapple(edict_t *ent)
{
	if (ent->client && ent->client->ctf_grapple)
		CTFResetGrapple(ent->client->ctf_grapple);
}

// self is grapple, not player
void CTFResetGrapple(edict_t *self)
{
	if (self->owner->client->ctf_grapple) {
		float volume = 1.0;
		gclient_t *cl;

		cl = self->owner->client;
		cl->ctf_grapple = NULL;
		cl->ctf_grapplereleasetime = level.time;
		cl->ctf_grapplestate = CTF_GRAPPLE_STATE_FLY; // we're firing, not on hook
		cl->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
		G_FreeEdict(self);
	}
}

void CTFGrappleTouch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	// float volume = 1.0;

	if (other == self->owner)
		return;

	if (self->owner->client->ctf_grapplestate != CTF_GRAPPLE_STATE_FLY)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		CTFResetGrapple(self);
		return;
	}

	VectorCopy(vec3_origin, self->velocity);

	PlayerNoise(self->owner, self->s.origin, PNOISE_IMPACT);

	if (other->takedamage) {
		T_Damage (other, self, self->owner, self->velocity, self->s.origin, plane->normal, self->dmg, 1, 0, MOD_GRAPPLE);
	}

	self->owner->client->ctf_grapplestate = CTF_GRAPPLE_STATE_PULL; // we're on hook
	self->enemy = other;

	self->solid = SOLID_NOT;

}

// draw beam between grapple and self
void CTFGrappleDrawCable(edict_t *self)
{
	vec3_t	offset, start, end, f, r;
	vec3_t	dir;
	float	distance;

	AngleVectors (self->owner->client->v_angle, f, r, NULL);
	VectorSet(offset, 42, 6, self->owner->viewheight-5);
	P_ProjectSource (self->owner->client, self->owner->s.origin, offset, f, r, start);

	VectorSubtract(start, self->owner->s.origin, offset);

	VectorSubtract (start, self->s.origin, dir);
	distance = VectorLength(dir);
	// don't draw cable if close
	if (distance < 64)
		return;

#if 0
	if (distance > 256)
		return;

	// check for min/max pitch
	vectoangles (dir, angles);
	if (angles[0] < -180)
		angles[0] += 360;
	if (fabs(angles[0]) > 45)
		return;

	trace_t	tr; //!!

	tr = gi.trace (start, NULL, NULL, self->s.origin, self, MASK_SHOT);
	if (tr.ent != self) {
		CTFResetGrapple(self);
		return;
	}
#endif

	VectorCopy (self->s.origin, end);

	gi.WriteByte (svc_temp_entity);

	gi.WriteByte (TE_BLASTERBEAM);
	gi.WritePosition (start);
	gi.WritePosition (end);
	gi.multicast (self->s.origin, MULTICAST_PVS);

}

void SV_AddGravity (edict_t *ent, float timespan);

// pull the player toward the grapple
void CTFGrapplePull(edict_t *self)
{
	vec3_t hookdir, v;
	float vlen;

	if (strcmp(self->owner->client->pers.weapon->classname, "weapon_grapple") == 0 &&
		!self->owner->client->newweapon &&
		self->owner->client->weaponstate != WEAPON_FIRING &&
		self->owner->client->weaponstate != WEAPON_ACTIVATING) {
		CTFResetGrapple(self);
		return;
	}

	if (self->enemy) {
		if (self->enemy->solid == SOLID_NOT) {
			CTFResetGrapple(self);
			return;
		}
		if (self->enemy->solid == SOLID_BBOX) {
			VectorScale(self->enemy->size, 0.5, v);
			VectorAdd(v, self->enemy->s.origin, v);
			VectorAdd(v, self->enemy->mins, self->s.origin);
			gi.linkentity (self);
		} else
			VectorCopy(self->enemy->velocity, self->velocity);
		if (self->enemy->takedamage) {
			T_Damage (self->enemy, self, self->owner, self->velocity, self->s.origin, vec3_origin, 1, 1, 0, MOD_GRAPPLE);
		}
		if (self->enemy->deadflag) { // he died
			CTFResetGrapple(self);
			return;
		}
	}

	CTFGrappleDrawCable(self);

	if (self->owner->client->ctf_grapplestate > CTF_GRAPPLE_STATE_FLY) {
		// pull player toward grapple
		// this causes icky stuff with prediction, we need to extend
		// the prediction layer to include two new fields in the player
		// move stuff: a point and a velocity.  The client should add
		// that velociy in the direction of the point
		vec3_t forward, up;

		AngleVectors (self->owner->client->v_angle, forward, NULL, up);
		VectorCopy(self->owner->s.origin, v);
		v[2] += self->owner->viewheight;
		VectorSubtract (self->s.origin, v, hookdir);

		vlen = VectorLength(hookdir);

		if (self->owner->client->ctf_grapplestate == CTF_GRAPPLE_STATE_PULL &&
			vlen < 64) {
			float volume = 1.0;

			self->owner->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
			gi.sound (self->owner, CHAN_RELIABLE+CHAN_WEAPON, gi.soundindex("weapons/electroball.wav"), volume, ATTN_NORM, 0);
			self->owner->client->ctf_grapplestate = CTF_GRAPPLE_STATE_HANG;
		}

		VectorNormalize (hookdir);
		VectorScale(hookdir, CTF_GRAPPLE_PULL_SPEED, hookdir);
		VectorCopy(hookdir, self->owner->velocity);
		SV_AddGravity(self->owner, FRAMETIME);
	}
}

void CTFFireGrapple (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int effect)
{
	edict_t	*grapple;
	trace_t	tr;

	VectorNormalize (dir);

	grapple = G_Spawn();
	VectorCopy (start, grapple->s.origin);
	VectorCopy (start, grapple->s.old_origin);
	vectoangles (dir, grapple->s.angles);
	VectorScale (dir, speed, grapple->velocity);
	grapple->movetype = MOVETYPE_FLYMISSILE;
	grapple->clipmask = MASK_SHOT;
	grapple->solid = SOLID_BBOX;
	grapple->s.effects |= effect;
	VectorClear (grapple->mins);
	VectorClear (grapple->maxs);
	grapple->s.modelindex = 0;
	grapple->owner = self;
	grapple->touch = CTFGrappleTouch;
	grapple->dmg = damage;
	self->client->ctf_grapple = grapple;
	self->client->ctf_grapplestate = CTF_GRAPPLE_STATE_FLY; // we're firing, not on hook
	gi.linkentity (grapple);

	tr = gi.trace (self->s.origin, NULL, NULL, grapple->s.origin, grapple, MASK_SHOT);
	if (tr.fraction < 1.0)
	{
		VectorMA (grapple->s.origin, -10, dir, grapple->s.origin);
		grapple->touch (grapple, tr.ent, NULL, NULL);
	}
}

void CTFGrappleFire (edict_t *ent, vec3_t g_offset, int damage, int effect)
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset;
	// float volume = 1.0;

	if (ent->client->ctf_grapplestate > CTF_GRAPPLE_STATE_FLY)
		return; // it's already out

	AngleVectors (ent->client->v_angle, forward, right, NULL);
	VectorSet(offset, 24, 8, ent->viewheight-6);
	VectorAdd (offset, g_offset, offset);
	P_ProjectSource (ent->client, ent->s.origin, offset, forward, right, start);

	CTFFireGrapple (ent, start, forward, damage, CTF_GRAPPLE_SPEED, effect);

	PlayerNoise(ent, start, PNOISE_WEAPON);
}


void CTFWeapon_Grapple_Fire (edict_t *ent)
{
	int		damage;

	damage = 15;
	CTFGrappleFire (ent, vec3_origin, damage, 0);
	ent->client->ps.gunframe++;
}

void CTFWeapon_Grapple (edict_t *ent)
{
	static int	pause_frames[]	= {52, 0};
	static int	fire_frames[]	= {5,0};
	int prevstate;

	// if the the attack button is still down, stay in the firing frame
	if ((ent->client->buttons & BUTTON_ATTACK) &&
		ent->client->weaponstate == WEAPON_FIRING &&
		ent->client->ctf_grapple)
		ent->client->ps.gunframe = 8;

	if (!(ent->client->buttons & BUTTON_ATTACK) &&
		ent->client->ctf_grapple) {
		CTFResetGrapple(ent->client->ctf_grapple);
		if (ent->client->weaponstate == WEAPON_FIRING)
			ent->client->weaponstate = WEAPON_READY;
	}


	if (ent->client->newweapon &&
		ent->client->ctf_grapplestate > CTF_GRAPPLE_STATE_FLY &&
		ent->client->weaponstate == WEAPON_FIRING) {
		// he wants to change weapons while grappled
		ent->client->weaponstate = WEAPON_DROPPING;
		ent->client->ps.gunframe = 53;
	}

	prevstate = ent->client->weaponstate;

	Weapon_Generic (ent, 4, 8, 52, 55, pause_frames, fire_frames,
		CTFWeapon_Grapple_Fire);

	// if we just switched back to grapple, immediately go to fire frame
	if (prevstate == WEAPON_ACTIVATING &&
		ent->client->weaponstate == WEAPON_READY &&
		ent->client->ctf_grapplestate > CTF_GRAPPLE_STATE_FLY) {
		if (!(ent->client->buttons & BUTTON_ATTACK))
			ent->client->ps.gunframe = 8;
		else
			ent->client->ps.gunframe = 4;
		ent->client->weaponstate = WEAPON_FIRING;
	}
}

static void CTFFlagThink(edict_t *ent)
{
	if (ent->solid != SOLID_NOT)
		ent->s.frame = 173 + (((ent->s.frame - 173) + 1) % 16);
	ent->nextthink = level.time + FRAMETIME;
}
void CTFFlagSetup (edict_t *ent)
{
	trace_t		tr;
	vec3_t		dest;
	float		*v;

	v = tv(-15,-15,-15);
	VectorCopy (v, ent->mins);
	v = tv(15,15,15);
	VectorCopy (v, ent->maxs);

	if (ent->model)
		gi.setmodel (ent, ent->model);
	else
		gi.setmodel (ent, ent->item->world_model);
	ent->solid = SOLID_TRIGGER;
	ent->movetype = MOVETYPE_TOSS;
	ent->touch = Touch_Item;

	v = tv(0,0,-128);
	VectorAdd (ent->s.origin, v, dest);

	tr = gi.trace (ent->s.origin, ent->mins, ent->maxs, dest, ent, MASK_SOLID);
	if (tr.startsolid)
	{
		gi.dprintf ("CTFFlagSetup: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
		G_FreeEdict (ent);
		return;
	}

	VectorCopy (tr.endpos, ent->s.origin);

	gi.linkentity (ent);

	ent->nextthink = level.time + FRAMETIME;
	ent->think = CTFFlagThink;
}
void CTFEffects(edict_t *player)
{
	gitem_t *flag1_item, *flag2_item;

	flag1_item = FindItemByClassname("item_flag_red");
	flag2_item = FindItemByClassname("item_flag_blue");

	if (player->client->pers.inventory[ITEM_INDEX(flag1_item)])
		player->s.modelindex4 = gi.modelindex("models/items/flags/flag1.iqm");
	else if (player->client->pers.inventory[ITEM_INDEX(flag2_item)])
		player->s.modelindex4 = gi.modelindex("models/items/flags/flag2.iqm");
	else
		player->s.modelindex4 = 0;
}
void CTFDrop_Flag(edict_t *ent, gitem_t *item)
{
	if (rand() & 1)
		safe_cprintf(ent, PRINT_HIGH, "Only lusers drop flags.\n");
	else
		safe_cprintf(ent, PRINT_HIGH, "Winners don't drop flags.\n");
}
void CTFResetFlag(int ctf_team)
{
	char *c;
	edict_t *ent;

	switch (ctf_team) {
	case RED_TEAM:
		c = "item_flag_red";
		break;
	case BLUE_TEAM:
		c = "item_flag_blue";
		break;
	default:
		return;
	}

	ent = NULL;
	while ((ent = G_Find (ent, FOFS(classname), c)) != NULL) {
		if (ent->spawnflags & DROPPED_ITEM)
			G_FreeEdict(ent);
		else {
			ent->svflags &= ~SVF_NOCLIENT;
			ent->solid = SOLID_TRIGGER;
			gi.linkentity(ent);
			ent->s.event = EV_ITEM_RESPAWN;
		}
	}
}

void CTFResetFlags(void)
{
	CTFResetFlag(RED_TEAM);
	CTFResetFlag(BLUE_TEAM);
}
static void CTFDropFlagTouch(edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	//owner (who dropped us) can't touch for two secs
	if (other == ent->owner &&
		ent->nextthink - level.time > 28)
		return;

	Touch_Item (ent, other, plane, surf);
}
static void CTFDropFlagThink(edict_t *ent)
{
	// auto return the flag
	// reset flag will remove ourselves
	if (strcmp(ent->classname, "item_flag_red") == 0) {
		CTFResetFlag(RED_TEAM);
		safe_bprintf(PRINT_HIGH, "The %s flag has returned!\n",
			"Red");
	} else if (strcmp(ent->classname, "item_flag_blue") == 0) {
		CTFResetFlag(BLUE_TEAM);
		safe_bprintf(PRINT_HIGH, "The %s flag has returned!\n",
			"Blue");
	}

}
void CTFDeadDropFlag(edict_t *self, edict_t *other)
{
	edict_t *dropped = NULL;
	gitem_t *flag1_item, *flag2_item;

	dropped = NULL;
	flag1_item = flag2_item = NULL;

	flag1_item = FindItemByClassname("item_flag_red");
	flag2_item = FindItemByClassname("item_flag_blue");

	/* notes on assist bonus:
	 *  other can be NULL, if self is disconnecting, or self is a bot being kicked.
	 *  other can be self for suicides, but that is handled with team check.
	 */
	if (self->client->pers.inventory[ITEM_INDEX(flag1_item)]) {

		if ( other != NULL && other->client
			&& ( (other->dmteam == RED_TEAM && self->dmteam == BLUE_TEAM)
				|| (other->dmteam == BLUE_TEAM && self->dmteam == RED_TEAM) ))
		{//kill enemy w flag; assist bonus (1 for frag + 4 = 5 total)
			other->client->resp.score += 4;
		}

		dropped = Drop_Item(self, flag1_item);
		self->client->pers.inventory[ITEM_INDEX(flag1_item)] = 0;
		safe_bprintf(PRINT_HIGH, "%s lost the %s flag!\n",
			self->client->pers.netname, "Red");
	} else if (self->client->pers.inventory[ITEM_INDEX(flag2_item)]) {

		if ( other != NULL && other->client
			&& ( (other->dmteam == RED_TEAM && self->dmteam == BLUE_TEAM)
				|| (other->dmteam == BLUE_TEAM && self->dmteam == RED_TEAM ) ))
		{ //kill enemy w flag; assist bonus (1 for frag + 4 = 5 total)
			other->client->resp.score += 4;
		}

		dropped = Drop_Item(self, flag2_item);
		self->client->pers.inventory[ITEM_INDEX(flag2_item)] = 0;
		safe_bprintf(PRINT_HIGH, "%s lost the %s flag!\n",
			self->client->pers.netname, "Blue");
	}

	if (dropped) {
		dropped->think = CTFDropFlagThink;
		dropped->nextthink = level.time + 30;
		dropped->touch = CTFDropFlagTouch;
		dropped->s.frame = 175;
		dropped->s.effects = EF_ROTATE;
	}
}
qboolean CTFPickup_Flag (edict_t *ent, edict_t *other)
{
	int ctf_team;
	char team_name[16] = " ";
	char enemy_team_name[16] = " ";
	gitem_t *flag_item, *enemy_flag_item;

	// figure out what team this flag is
	if (strcmp(ent->classname, "item_flag_red") == 0)
		ctf_team = RED_TEAM;
	else if (strcmp(ent->classname, "item_flag_blue") == 0)
		ctf_team = BLUE_TEAM;
	else {
		safe_cprintf(ent, PRINT_HIGH, "Don't know what team the flag is on.\n");
		return false;
	}

// same team, if the flag at base, check to he has the enemy flag
	if (ctf_team == RED_TEAM) {
		flag_item = FindItemByClassname("item_flag_red");
		enemy_flag_item = FindItemByClassname("item_flag_blue");
		strcpy(team_name, "Red");
		strcpy(enemy_team_name, "Blue");
	} else {
		flag_item = FindItemByClassname("item_flag_blue");
		enemy_flag_item = FindItemByClassname("item_flag_red");
		strcpy(team_name, "Blue");
		strcpy(enemy_team_name, "Red");
	}

	if (ctf_team == other->dmteam) {

		if (!(ent->spawnflags & DROPPED_ITEM)) {
			// the flag is at home base.  if the player has the enemy
			// flag, he's just won!

			if (other->client->pers.inventory[ITEM_INDEX(enemy_flag_item)]) 
			{
				safe_bprintf(PRINT_HIGH, "%s captured the %s flag!\n",
						other->client->pers.netname, enemy_team_name);
				other->client->pers.inventory[ITEM_INDEX(enemy_flag_item)] = 0;

				if(!other->is_bot) 
				{
					// Send Steam stats
					gi.WriteByte (svc_temp_entity);
					gi.WriteByte(TE_FLAGCAPTURE);
					gi.unicast (other, true);
				}

				if (ctf_team == RED_TEAM)
				{
					red_team_score++;

					if(red_team_score > blue_team_score){
						reddiff = red_team_score - blue_team_score;
						if(reddiff == 1){
							gi.sound (ent, CHAN_AUTO, gi.soundindex("misc/red_takes.wav"), 1, ATTN_NONE, 0);
						}
						else
						{
							gi.sound (ent, CHAN_AUTO, gi.soundindex("misc/red_increases.wav"), 1, ATTN_NONE, 0);
						}

					}
					else
					{
						if(red_team_score == blue_team_score){
							gi.sound (ent, CHAN_AUTO, gi.soundindex("misc/scores_tied.wav"), 1, ATTN_NONE, 0);
						}
						else
						{
							gi.sound (ent, CHAN_AUTO, gi.soundindex("misc/red_scores.wav"), 1, ATTN_NONE, 0);
						}
					}

				}
				else
				{
					blue_team_score++;

					if(blue_team_score > red_team_score){
						bluediff = blue_team_score - red_team_score;
						if(bluediff == 1){
							gi.sound (ent, CHAN_AUTO, gi.soundindex("misc/blue_takes.wav"), 1, ATTN_NONE, 0);
						}
						else
						{
							gi.sound (ent, CHAN_AUTO, gi.soundindex("misc/blue_increases.wav"), 1, ATTN_NONE, 0);
						}

					}
					else
					{
						if(red_team_score == blue_team_score){
							gi.sound (ent, CHAN_AUTO, gi.soundindex("misc/scores_tied.wav"), 1, ATTN_NONE, 0);
						}
						else
						{
							gi.sound (ent, CHAN_AUTO, gi.soundindex("misc/blue_scores.wav"), 1, ATTN_NONE, 0);
						}
					}
				}

				// other gets another 10 frag bonus
				other->client->resp.score += 10;//CTF_CAPTURE_BONUS;
				//reward points bonus
				PlayerGrantRewardPoints (other, 5);

				CTFResetFlags();
				return false;
			}
			return false; // its at home base already
		}
		// hey, its not home.  return it by teleporting it back
		safe_bprintf(PRINT_HIGH, "%s returned the %s flag!\n",
			other->client->pers.netname, team_name);
		other->client->resp.score += 5;//CTF_RECOVERY_BONUS;
		if(!strcmp("Red", team_name))
			gi.sound (ent, CHAN_AUTO, gi.soundindex("misc/red_returned.wav"), 1, ATTN_NONE, 0);
		else
			gi.sound (ent, CHAN_AUTO, gi.soundindex("misc/blue_returned.wav"), 1, ATTN_NONE, 0);

		//reward points bonus
		PlayerGrantRewardPoints (other, 2);

		// Steam stats update
		if(!other->is_bot) 
		{
			// Send Steam stats
			gi.WriteByte (svc_temp_entity);
			gi.WriteByte(TE_FLAGRETURN);
			gi.unicast (other, false);
		}

		//CTFResetFlag will remove this entity!  We must return false
		CTFResetFlag(ctf_team);
		return false;
	}

	// hey, its not our flag, pick it up
	safe_bprintf(PRINT_HIGH, "%s got the %s flag!\n",
		other->client->pers.netname, team_name);
	other->client->resp.score += 10;//CTF_FLAG_BONUS;
	if(!strcmp("Red", team_name))
		gi.sound (ent, CHAN_AUTO, gi.soundindex("misc/red_picked.wav"), 1, ATTN_NONE, 0);
	else
		gi.sound (ent, CHAN_AUTO, gi.soundindex("misc/blue_picked.wav"), 1, ATTN_NONE, 0);

	//if a bot, break off of path, so we can find base path
	if(other->is_bot)
	{
		other->state = STATE_WANDER;
		other->wander_timeout = level.time + 1.0;
	}

	PlayerGrantRewardPoints (other, 2);

	other->client->pers.inventory[ITEM_INDEX(flag_item)] = 1;

	// pick up the flag
	// if it's not a dropped flag, we just make is disappear
	// if it's dropped, it will be removed by the pickup caller
	if (!(ent->spawnflags & DROPPED_ITEM)) 
	{
		ent->flags |= FL_RESPAWN;
		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
	}
	return true;
}
