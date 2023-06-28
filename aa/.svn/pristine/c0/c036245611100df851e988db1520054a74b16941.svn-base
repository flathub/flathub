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

/*
==============================================================================

Death Ray Gun

==============================================================================
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif


#include "g_local.h"
#include "g_deathray.h"


static int sound_pain;
static int sound_die;
static int sound_idle;
static int sound_punch;
static int sound_sight;
static int sound_search;


void deathray_search (edict_t *self)
{
	gi.sound (self, CHAN_VOICE, sound_search, 1, ATTN_NORM, 0);
}

mframe_t deathray_frames_stand [] =
{
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL},
	{ai_stand, 0, NULL}

};
mmove_t deathray_move_stand = {FRAME_stand01, FRAME_stand06, deathray_frames_stand, NULL};

void deathray_stand (edict_t *self)
{
	self->monsterinfo.currentmove = &deathray_move_stand;
}

mframe_t deathray_frames_run1 [] =
{
	{ai_run, 0, NULL},
	{ai_run, 0, NULL},
	{ai_run, 0, NULL},
	{ai_run, 0, NULL},
	{ai_run, 0, NULL},
	{ai_run, 0, NULL}
};
mmove_t deathray_move_run1 = {FRAME_stand01, FRAME_stand06, deathray_frames_run1, NULL};

void deathray_run (edict_t *self)
{
	self->monsterinfo.currentmove = &deathray_move_run1;
}

void deathrayShot (edict_t *self)
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

	//tactical
	if(g_tactical->value)
	{
		if(self->spawnflags & 1)
		{
			//if power source is down, and backup gen is on, get erratic
			if(!tacticalScore.humanPowerSource)
			{
				if(!tacticalScore.humanBackupGen)
					return;

				if(random() < 0.5) 
				{
					if(self->enemy->ctype == 1)
					{
						if(random() < 0.5)
							return;
					}
					else
						return;
				}
			}
			else if(!tacticalScore.humanComputer) //we have power, but no computer, so behave erratically
			{
				if(random() < 0.5) 
				{
					if(self->enemy->ctype == 1)
					{
						if(random() < 0.5)
							return;
					}
					else
						return;
				}
			}

			//do not fire on humans
			if(self->enemy->ctype == 1)
				return;
		}
		else
		{
			if(!tacticalScore.alienPowerSource)
			{
				if(!tacticalScore.alienBackupGen)
					return;

				if(random() < 0.5) 
				{
					if(self->enemy->ctype == 0)
					{
						if(random() < 0.5)
							return;
					}
					else
						return;
				}
			}
			else if(!tacticalScore.alienComputer)
			{
				if(random() < 0.5) 
				{
					if(self->enemy->ctype == 0)
					{
						if(random() < 0.5)
							return;
					}
					else
						return;
				}
			}

			//do not fire on aliens
			if(self->enemy->ctype == 0)
				return; 
		}
	}

	AngleVectors (self->s.angles, forward, right, NULL);
	VectorSet(offset, 32, 0, 48);
	G_ProjectSource (self->s.origin, offset, forward, right, start);
    VectorCopy (self->s.origin, start);
	VectorCopy (self->enemy->s.origin, end);
	end[2] += self->enemy->viewheight;
	VectorSubtract (end, start, dir);
	right[0] = forward[0] * 64;
	right[1] = forward[1] * 64;
    VectorAdd(start, right, start);
	end[2] -= 32;
    start[2] += 64;
    VectorCopy (start, from);
    tr = gi.trace (from, NULL, NULL, end, self, MASK_SHOT);
    VectorCopy (tr.endpos, from);

	// send muzzle flash
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (self-g_edicts);
	gi.WriteByte (MZ_RAILGUN);
	gi.multicast (self->s.origin, MULTICAST_PVS);
	
	gi.WriteByte (svc_temp_entity);
	//tactical
	if(self->spawnflags & 1)
		gi.WriteByte (TE_REDLASER);
	else
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
		T_Damage (tr.ent, self, self, dir, tr.endpos, tr.plane.normal, damage, 0, 0, MOD_DEATHRAY);
	else if (!((tr.surface) && (tr.surface->flags & SURF_SKY)))
	{
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_SCREEN_SPARKS);
		gi.WritePosition (tr.endpos);
		gi.WriteDir (tr.plane.normal);
		gi.multicast (self->s.origin, MULTICAST_PVS);
	}
}

mframe_t deathray_frames_attack_shoot [] =
{
	{ai_charge, 0, NULL},
	{ai_charge, 0, deathrayShot},
	{ai_charge, 0, NULL},
	{ai_charge, 0, NULL},
	{ai_charge, 0, deathrayShot},
	{ai_charge, 0, NULL},
	{ai_charge, 0, deathrayShot}
};
mmove_t deathray_move_attack_shoot = {FRAME_stand01, FRAME_stand06, deathray_frames_attack_shoot, deathray_run};


void deathray_sight (edict_t *self, edict_t *other)
{
	gi.sound (self, CHAN_VOICE, sound_sight, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &deathray_move_run1;

}

void deathray_attack (edict_t *self)
{
	self->monsterinfo.currentmove = &deathray_move_attack_shoot;
}

mframe_t deathray_frames_pain1 [] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}
};
mmove_t deathray_move_pain1 = {FRAME_stand01, FRAME_stand06, deathray_frames_pain1, deathray_run};

void deathray_pain (edict_t *self, edict_t *other, float kick, int damage)
{
	if (self->health < (self->max_health / 2))
		self->s.skinnum = 1;

	if (level.time < self->pain_debounce_time)
		return;

	self->pain_debounce_time = level.time + 3;
	gi.sound (self, CHAN_VOICE, sound_pain, 1, ATTN_NORM, 0);
	self->monsterinfo.currentmove = &deathray_move_pain1;
}


void deathray_dead (edict_t *self)
{
	VectorSet (self->mins, -16, -16, -42);
	VectorSet (self->maxs, 16, 16, -8);
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity (self);
}


mframe_t deathray_frames_death1 [] =
{
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL},
	{ai_move, 0, NULL}


};
mmove_t deathray_move_death1 = {FRAME_stand01, FRAME_stand05, deathray_frames_death1, deathray_dead};

void deathray_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	self->deadflag = DEAD_DEAD;

	gi.sound( &g_edicts[1], CHAN_AUTO, gi.soundindex( "world/explosion1.wav" ), 1, ATTN_NONE, 0 );

	G_FreeEdict (self);
}

void SP_npc_deathray (edict_t *self)
{

	// pre-caches
	sound_pain  = gi.soundindex ("misc/deathray/fizz.wav");
	sound_die   = gi.soundindex ("misc/deathray/fizz.wav");
	sound_idle  = gi.soundindex ("misc/deathray/weird2.wav");
	sound_punch = gi.soundindex ("misc/deathray/shoot.wav");
	sound_search = gi.soundindex ("misc/deathray/weird2.wav");
	sound_sight = gi.soundindex ("misc/deathray/weird2.wav");

	self->s.modelindex = gi.modelindex("models/misc/deathray/deathray.iqm");

	VectorSet (self->mins, -16, -16, 0);
	VectorSet (self->maxs, 16, 16, 48);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->takedamage = DAMAGE_NO; //invincible
	self->max_health = 5000;
	self->health = self->max_health;
	self->gib_health = 0;
	self->mass = 5000;

	self->pain = deathray_pain;
	self->die = deathray_die;

	self->monsterinfo.stand = deathray_stand;
	self->monsterinfo.walk = deathray_run;
	self->monsterinfo.run = deathray_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = deathray_attack;
	self->monsterinfo.melee = deathray_attack;
	self->monsterinfo.sight = deathray_sight;
	self->monsterinfo.search = deathray_search;
	self->s.renderfx |= RF_MONSTER;

	self->monsterinfo.currentmove = &deathray_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;

	self->last_action = level.time;

	gi.linkentity (self);

	walkmonster_start (self);
}

void SP_misc_deathray (edict_t *self)
{

	// pre-caches
	sound_pain  = gi.soundindex ("misc/deathray/fizz.wav");
	sound_die   = gi.soundindex ("misc/deathray/fizz.wav");
	sound_idle  = gi.soundindex ("misc/deathray/weird2.wav");
	sound_punch = gi.soundindex ("misc/deathray/shoot.wav");
	sound_search = gi.soundindex ("misc/deathray/weird2.wav");
	sound_sight = gi.soundindex ("misc/deathray/weird2.wav");

	//to do - make different mesh for human weapon
	if(self->spawnflags & 1)
	{
		self->ctype = 1;
		self->s.modelindex = gi.modelindex("models/tactical/human_turret.iqm");
	}
	else
	{
		self->ctype = 0;
		self->s.modelindex = gi.modelindex("models/misc/deathray/deathray.iqm");
	}

	VectorSet (self->mins, -16, -16, 0);
	VectorSet (self->maxs, 16, 16, 48);
	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->takedamage = DAMAGE_YES; 
	self->max_health = 300;
	self->health = self->max_health;
	self->gib_health = 0;
	self->mass = 5000;

	self->pain = deathray_pain;
	self->die = deathray_die;

	self->monsterinfo.stand = deathray_stand;
	self->monsterinfo.walk = deathray_run;
	self->monsterinfo.run = deathray_run;
	self->monsterinfo.dodge = NULL;
	self->monsterinfo.attack = deathray_attack;
	self->monsterinfo.melee = deathray_attack;
	self->monsterinfo.sight = deathray_sight;
	self->monsterinfo.search = deathray_search;
	self->s.renderfx |= RF_MONSTER;

	self->monsterinfo.currentmove = &deathray_move_stand;
	self->monsterinfo.scale = MODEL_SCALE;

	self->last_action = level.time;

	gi.linkentity (self);

	walkmonster_start (self);
}
