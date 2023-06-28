/*
Copyright (C) 2012 COR Entertainment, LLC.

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

/*
==============================================================================

Spider projectile bot

==============================================================================
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "g_local.h"
#include "g_spider.h"

static int sound_pain;
static int sound_die;
static int sound_walk;
static int sound_punch;
static int sound_sight;
static int sound_search;

void spider_search (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

mframe_t spider_frames_stand [] =
{
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL}

};
mmove_t spider_move_stand = {FRAME_stand01, FRAME_stand13, spider_frames_stand, NULL};

void spider_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &spider_move_stand;
}

void spider_step(edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_walk, 1, ATTN_NORM, 0);
}

mframe_t spider_frames_walk1 [] =
{
	{ai_run, 4, NULL},
	{ai_run, 8, spider_step},
	{ai_run, 12, NULL},
	{ai_run, 12, spider_step},
	{ai_run, 8, NULL},
	{ai_run, 12, spider_step}
};
mmove_t spider_move_walk1 = {FRAME_walk01, FRAME_walk06, spider_frames_walk1, NULL};

void spider_walk (edict_t *self)
{
	if (self->monsterinfo.aiflags & AI_STAND_GROUND)
		self->monsterinfo.currentmove = &spider_move_stand;
	else
		self->monsterinfo.currentmove = &spider_move_walk1;
}

void spiderShot (edict_t *self) //same as deathray shot
{
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	end;
	vec3_t	dir;
	vec3_t	from;
	vec3_t	offset;
	int		damage = 50;
    trace_t tr;

	//don't shoot any faster than .1 seconds apart
	if( (level.time - self->last_action) > TENFPS )
		self->last_action = level.time;
	else
		return;

	if(g_tactical->integer && (self->ctype == self->enemy->ctype))
		return;

	AngleVectors (self->s.angles, forward, right, NULL);
	VectorSet(offset, 16, 0, 16);
	G_ProjectSource (self->s.origin, offset, forward, right, start);
    VectorCopy (self->s.origin, start);
	VectorCopy (self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight;
	VectorSubtract (end, start, dir);
	right[0] = forward[0] * 32;
	right[1] = forward[1] * 32;
    VectorAdd(start, right, start);
    start[2] += 16;
    VectorCopy (start, from);
    tr = gi.trace (from, NULL, NULL, end, self, MASK_SHOT);
    VectorCopy (tr.endpos, from);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (self-g_edicts);
	gi.WriteByte (MZ_RAILGUN);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_VAPORBEAM);
	gi.WritePosition (start);
	gi.WritePosition (end);
	gi.multicast (self->s.origin, MULTICAST_PHS);

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_BFG_BIGEXPLOSION);
	gi.WritePosition (end);
	gi.multicast (end, MULTICAST_PVS);

	gi.sound (self, CHAN_VOICE, sound_punch, 1, ATTN_NORM, 0);

	if ((tr.ent != self) && (tr.ent->takedamage))
		T_Damage (tr.ent, self, self, dir, tr.endpos, tr.plane.normal, damage, 0, 0, MOD_SPIDER);
	else if (!((tr.surface) && (tr.surface->flags & SURF_SKY)))
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_SCREEN_SPARKS);
		gi.WritePosition (tr.endpos);
		gi.WriteDir (tr.plane.normal);
		gi.multicast (self->s.origin, MULTICAST_PVS);
	}
}

mframe_t spider_frames_attack_shoot [] =
{
	{ai_charge, 0, NULL},
	{ai_charge, 0, spiderShot},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL}
};
mmove_t spider_move_attack_shoot = {FRAME_shoot01, FRAME_shoot06, spider_frames_attack_shoot, spider_walk};

void spider_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &spider_move_walk1;
}

void spider_attack (edict_t *self)
{
	self->monsterinfo.currentmove = &spider_move_attack_shoot;
}

mframe_t spider_frames_pain1 [] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t spider_move_pain1 = {FRAME_pain01, FRAME_pain05, spider_frames_pain1, spider_walk};

void spider_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;
	gi.sound (self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &spider_move_pain1;
}

void spider_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	gi.sound (self, CHAN_VOICE, sound_die, 1, ATTN_NORM, 0);

	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	self->deadflag = DEAD_DEAD;

	self->think = G_FreeEdict;

	gi.linkentity (self);

	G_FreeEdict(self);
}

void spawn_spider (edict_t *owner, vec3_t origin, vec3_t angle)
{
	edict_t *self = G_Spawn();

	VectorCopy(origin, self->s.origin);
	VectorCopy(angle, self->s.angles);
	self->s.origin[2] += 4;

	sound_pain  = gi.soundindex ("misc/deathray/fizz.wav");
	sound_die   = gi.soundindex ("misc/deathray/fizz.wav");
	sound_walk  = gi.soundindex ("weapons/spidermov.wav");
	sound_punch = gi.soundindex ("misc/deathray/shoot.wav");
	sound_search = gi.soundindex ("weapons/seeker.wav");
	sound_sight = gi.soundindex ("misc/deathray/weird2.wav");

	self->s.modelindex = gi.modelindex("models/objects/spider/tris.iqm");
	self->s.modelindex3 = gi.modelindex("models/objects/spider/helmet.iqm");

	self->classname = "proj_spider";

	VectorSet (self->mins, -16, -16, 0);
	VectorSet (self->maxs, 16, 16, 24);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->takedamage = DAMAGE_YES; 
	self->max_health = 100;
	self->health = self->max_health;
	self->gib_health = 0;
	self->mass = 100;

	self->pain = spider_pain;
	self->die = spider_die;

	self->monsterinfo.stand = spider_stand;
	self->monsterinfo.walk = spider_walk;
	self->monsterinfo.run = spider_walk;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = spider_attack;
	self->monsterinfo.melee = spider_attack;
	self->monsterinfo.sight = spider_sight;
	self->monsterinfo.search = spider_search;

	self->monsterinfo.currentmove = &spider_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;
	self->enemy = NULL;

	self->owner = owner;

	if(g_tactical->integer)
		self->ctype = owner->ctype;

	self->last_action = level.time;

	gi.linkentity (self);

	walkmonster_start (self);
}

void spider_think (edict_t *self)
{	
	self->nextthink = level.time + FRAMETIME;
}

void spider_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent->owner)
		return;

	if (surf && (surf->flags & SURF_SKY))
	{
		G_FreeEdict (ent);
		return;
	}
	else if(surf)
	{
		spawn_spider(ent->owner, ent->s.origin, ent->s.angles);
		
		G_FreeEdict (ent);
		return;
	}

	//just bounce off of anything else
	gi.sound (ent, CHAN_VOICE, gi.soundindex ("weapons/clank.wav"), 1, ATTN_NORM, 0);
	return;

}

void fire_spider (edict_t *self, vec3_t start, vec3_t aimdir, int speed)
{
	edict_t	*spider;
    vec3_t	dir, forward, right, up;

	self->client->resp.weapon_shots[2]++;

	vectoangles(aimdir, dir);
	AngleVectors(dir, forward, right, up);

	spider = G_Spawn();
	VectorCopy (start, spider->s.origin);
	VectorScale (aimdir, speed, spider->velocity);
	VectorMA (spider->velocity, 200 + crandom() * 10.0, up, spider->velocity);
	VectorSet (spider->avelocity, 300, 300, 300);
	spider->movetype = MOVETYPE_BOUNCE;
	spider->clipmask = MASK_SHOT;
	spider->solid = SOLID_BBOX;
	VectorClear (spider->mins);
	VectorClear (spider->maxs);
	VectorSet (spider->mins, -32, -32, -32);
	VectorSet (spider->maxs, 32, 32, 32);
	spider->s.modelindex = gi.modelindex("models/objects/spider/tris.iqm");
	spider->s.modelindex3 = gi.modelindex("models/objects/spider/helmet.iqm");
	spider->s.frame = 31;
	spider->owner = self;
	spider->touch = spider_touch;
	spider->nextthink = level.time + FRAMETIME;
    spider->think = spider_think;
   
	spider->s.sound = gi.soundindex ("weapons/electroball.wav");
	spider->classname = "proj_spider";

	gi.linkentity (spider);
}
