/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 20?? COR Entertainment, LLC.

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

void AttackFinished (edict_t *self, float time)
{
	self->monsterinfo.attack_finished = level.time + time;
}


void M_CheckGround (edict_t *ent)
{
	vec3_t		point;
	trace_t		trace;

	if (ent->flags & (FL_SWIM|FL_FLY))
		return;

	if (ent->velocity[2] > 100)
	{
		ent->groundentity = NULL;
		return;
	}

// if the hull point one-quarter unit down is solid the entity is on ground
	point[0] = ent->s.origin[0];
	point[1] = ent->s.origin[1];
	point[2] = ent->s.origin[2] - 0.25;

	trace = gi.trace (ent->s.origin, ent->mins, ent->maxs, point, ent, MASK_MONSTERSOLID);

	// check steepness
	if ( trace.plane.normal[2] < 0.7 && !trace.startsolid)
	{
		ent->groundentity = NULL;
		return;
	}

//	ent->groundentity = trace.ent;
//	ent->groundentity_linkcount = trace.ent->linkcount;
//	if (!trace.startsolid && !trace.allsolid)
//		VectorCopy (trace.endpos, ent->s.origin);
	if (!trace.startsolid && !trace.allsolid)
	{
		VectorCopy (trace.endpos, ent->s.origin);
		ent->groundentity = trace.ent;
		ent->groundentity_linkcount = trace.ent->linkcount;
		ent->velocity[2] = 0;
	}
}


void M_CatagorizePosition (edict_t *ent)
{
	vec3_t		point;
	int			cont;

//
// get waterlevel
//
	point[0] = ent->s.origin[0];
	point[1] = ent->s.origin[1];
	point[2] = ent->s.origin[2] + ent->mins[2] + 1;
	cont = gi.pointcontents (point);

	if (!(cont & MASK_WATER))
	{
		ent->waterlevel = 0;
		ent->watertype = 0;
		return;
	}

	ent->watertype = cont;
	ent->waterlevel = 1;
	point[2] += 26;
	cont = gi.pointcontents (point);
	if (!(cont & MASK_WATER))
		return;

	ent->waterlevel = 2;
	point[2] += 22;
	cont = gi.pointcontents (point);
	if (cont & MASK_WATER)
		ent->waterlevel = 3;
}


void M_WorldEffects (edict_t *ent)
{
	int		dmg;

	if (ent->health > 0)
	{
		if (!(ent->flags & FL_SWIM))
		{
			if (ent->waterlevel < 3)
			{
				ent->air_finished = level.time + 12;
			}
			else if (ent->air_finished < level.time)
			{	// drown!
				if (ent->pain_debounce_time < level.time)
				{
					dmg = 2 + 2 * floor(level.time - ent->air_finished);
					if (dmg > 15)
						dmg = 15;
					T_Damage (ent, world, world, vec3_origin, ent->s.origin, vec3_origin, dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
					ent->pain_debounce_time = level.time + 1;
				}
			}
		}
		else
		{
			if (ent->waterlevel > 0)
			{
				ent->air_finished = level.time + 9;
			}
			else if (ent->air_finished < level.time)
			{	// suffocate!
				if (ent->pain_debounce_time < level.time)
				{
					dmg = 2 + 2 * floor(level.time - ent->air_finished);
					if (dmg > 15)
						dmg = 15;
					T_Damage (ent, world, world, vec3_origin, ent->s.origin, vec3_origin, dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
					ent->pain_debounce_time = level.time + 1;
				}
			}
		}
	}

	if (ent->waterlevel == 0)
	{
		if (ent->flags & FL_INWATER)
		{
			gi.sound (ent, CHAN_BODY, gi.soundindex("player/watr_out.wav"), 1, ATTN_NORM, 0);
			ent->flags &= ~FL_INWATER;
		}
		return;
	}

	if ((ent->watertype & CONTENTS_LAVA) && !(ent->flags & FL_IMMUNE_LAVA))
	{
		if (ent->damage_debounce_time < level.time)
		{
			ent->damage_debounce_time = level.time + 0.2;
			T_Damage (ent, world, world, vec3_origin, ent->s.origin, vec3_origin, 10*ent->waterlevel, 0, 0, MOD_LAVA);
		}
	}
	if ((ent->watertype & CONTENTS_SLIME) && !(ent->flags & FL_IMMUNE_SLIME))
	{
		if (ent->damage_debounce_time < level.time)
		{
			ent->damage_debounce_time = level.time + 1;
			T_Damage (ent, world, world, vec3_origin, ent->s.origin, vec3_origin, 4*ent->waterlevel, 0, 0, MOD_SLIME);
		}
	}

	if ( !(ent->flags & FL_INWATER) )
	{
		if (!(ent->svflags & SVF_DEADMONSTER))
		{
			if (ent->watertype & CONTENTS_LAVA)
				if (random() <= 0.5)
					gi.sound (ent, CHAN_BODY, gi.soundindex("player/lava1.wav"), 1, ATTN_NORM, 0);
				else
					gi.sound (ent, CHAN_BODY, gi.soundindex("player/lava2.wav"), 1, ATTN_NORM, 0);
			else if (ent->watertype & CONTENTS_SLIME)
				gi.sound (ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
			else if (ent->watertype & CONTENTS_WATER)
				gi.sound (ent, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1, ATTN_NORM, 0);
		}

		ent->flags |= FL_INWATER;
		ent->damage_debounce_time = 0;
	}
}


void M_droptofloor (edict_t *ent)
{
	vec3_t		end;
	trace_t		trace;

	ent->s.origin[2] += 1;
	VectorCopy (ent->s.origin, end);
	end[2] -= 256;

	trace = gi.trace (ent->s.origin, ent->mins, ent->maxs, end, ent, MASK_MONSTERSOLID);

	if (trace.fraction == 1 || trace.allsolid)
		return;

	VectorCopy (trace.endpos, ent->s.origin);

	gi.linkentity (ent);
	M_CheckGround (ent);
	M_CatagorizePosition (ent);
}


void M_SetEffects (edict_t *ent)
{
	ent->s.effects &= ~(EF_COLOR_SHELL);
	ent->s.renderfx &= ~(RF_SHELL_RED|RF_SHELL_GREEN|RF_SHELL_BLUE);

	if (ent->monsterinfo.aiflags & AI_RESURRECTING)
	{
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= RF_SHELL_BLUE;
	}

	if (ent->health <= 0)
		return;
}


void M_MoveFrame (edict_t *self)
{
	mmove_t	*move;
	int		index;

	move = self->monsterinfo.currentmove;
	self->nextthink = level.time + FRAMETIME;

	if ((self->monsterinfo.nextframe) && (self->monsterinfo.nextframe >= move->firstframe) && (self->monsterinfo.nextframe <= move->lastframe))
	{
		self->s.frame = self->monsterinfo.nextframe;
		self->monsterinfo.nextframe = 0;
	}
	else
	{
		if (self->s.frame == move->lastframe)
		{
			if (move->endfunc)
			{
				move->endfunc (self);

				// regrab move, endfunc is very likely to change it
				move = self->monsterinfo.currentmove;

				// check for death
				if (self->svflags & SVF_DEADMONSTER)
					return;
			}
		}

		if (self->s.frame < move->firstframe || self->s.frame > move->lastframe)
		{
			self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
			self->s.frame = move->firstframe;
		}
		else
		{
			if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
			{
				if(level.framenum - self->last_anim_frame >= round(TENFPS/FRAMETIME))
				{
					self->s.frame++;
					if (self->s.frame > move->lastframe)
						self->s.frame = move->firstframe;

					self->last_anim_frame = level.framenum;
				}
			}
		}
	}

	index = self->s.frame - move->firstframe;
	if (move->frame[index].aifunc)
	{
		if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
			move->frame[index].aifunc (self, move->frame[index].dist * self->monsterinfo.scale);
		else
			move->frame[index].aifunc (self, 0);
	}

	if (move->frame[index].thinkfunc)
		move->frame[index].thinkfunc (self);
}


void monster_think (edict_t *self)
{
	M_MoveFrame (self);
	if (self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround (self);
	}
	M_CatagorizePosition (self);
	M_WorldEffects (self);
	M_SetEffects (self);
}


/*
================
monster_use

Using a monster makes it angry at the current activator
================
*/
void monster_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->enemy)
		return;
	if (self->health <= 0)
		return;
	if (activator->flags & FL_NOTARGET)
		return;
	if (!(activator->client) && !(activator->monsterinfo.aiflags & AI_GOOD_GUY))
		return;

// delay reaction so if the monster is teleported, its sound is still heard
	self->enemy = activator;
	FoundTarget (self);
}


void monster_start_go (edict_t *self);


void monster_triggered_spawn (edict_t *self)
{
	self->s.origin[2] += 1;
	KillBox (self);

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->svflags &= ~SVF_NOCLIENT;
	self->air_finished = level.time + 12;
	gi.linkentity (self);

	monster_start_go (self);

	if (self->enemy && !(self->spawnflags & 1) && !(self->enemy->flags & FL_NOTARGET))
	{
		FoundTarget (self);
	}
	else
	{
		self->enemy = NULL;
	}
}

void monster_triggered_spawn_use (edict_t *self, edict_t *other, edict_t *activator)
{
	// we have a one frame delay here so we don't telefrag the guy who activated us
	self->think = monster_triggered_spawn;
	self->nextthink = level.time + FRAMETIME;
	if (activator->client)
		self->enemy = activator;
	self->use = monster_use;
}

void monster_triggered_start (edict_t *self)
{
	self->solid = SOLID_NOT;
	self->movetype = MOVETYPE_NONE;
	self->svflags |= SVF_NOCLIENT;
	self->nextthink = 0;
	self->use = monster_triggered_spawn_use;
}


/*
================
monster_death_use

When a monster dies, it fires all of its targets with the current
enemy as activator.
================
*/
void monster_death_use (edict_t *self)
{
	self->flags &= ~(FL_FLY|FL_SWIM);
	self->monsterinfo.aiflags &= AI_GOOD_GUY;

	if (self->item)
	{
		Drop_Item (self, self->item);
		self->item = NULL;
	}

	if (self->deathtarget)
		self->target = self->deathtarget;

	if (!self->target)
		return;

	G_UseTargets (self, self->enemy);
}


//============================================================================

qboolean monster_start (edict_t *self)
{

	if ((self->spawnflags & 4) && !(self->monsterinfo.aiflags & AI_GOOD_GUY))
	{
		self->spawnflags &= ~4;
		self->spawnflags |= 1;
//		gi.dprintf("fixed spawnflags on %s at %s\n", self->classname, vtos(self->s.origin));
	}

	if (!(self->monsterinfo.aiflags & AI_GOOD_GUY))
		level.total_monsters++;

	self->nextthink = level.time + FRAMETIME;
	self->svflags |= SVF_MONSTER;
	self->takedamage = DAMAGE_AIM;
	self->air_finished = level.time + 12;
	self->use = monster_use;
	self->max_health = self->health;
	self->clipmask = MASK_MONSTERSOLID;

	self->s.skinnum = 0;
	self->deadflag = DEAD_NO;
	self->svflags &= ~SVF_DEADMONSTER;

	if (!self->monsterinfo.checkattack)
		self->monsterinfo.checkattack = M_CheckAttack;
	VectorCopy (self->s.origin, self->s.old_origin);

	if (st.item)
	{
		self->item = FindItemByClassname (st.item);
		if (!self->item)
			gi.dprintf("%s at %s has bad item: %s\n", self->classname, vtos(self->s.origin), st.item);
	}

	// randomize what frame they start on
	if (self->monsterinfo.currentmove)
		self->s.frame = self->monsterinfo.currentmove->firstframe + (rand() % (self->monsterinfo.currentmove->lastframe - self->monsterinfo.currentmove->firstframe + 1));

	return true;
}

void monster_start_go (edict_t *self)
{
	vec3_t	v;

	if (self->health <= 0)
		return;

	// check for target to combat_point and change to combattarget
	if (self->target)
	{
		qboolean	notcombat;
		qboolean	fixup;
		edict_t		*target;

		target = NULL;
		notcombat = false;
		fixup = false;
		while ((target = G_Find (target, FOFS(targetname), self->target)) != NULL)
		{
			if (strcmp(target->classname, "point_combat") == 0)
			{
				self->combattarget = self->target;
				fixup = true;
			}
			else
			{
				notcombat = true;
			}
		}
		if (notcombat && self->combattarget)
			gi.dprintf("%s at %s has target with mixed types\n", self->classname, vtos(self->s.origin));
		if (fixup)
			self->target = NULL;
	}

	// validate combattarget
	if (self->combattarget)
	{
		edict_t		*target;

		target = NULL;

		while ((target = G_Find (target, FOFS(targetname), self->combattarget)) != NULL)
		{
			if (strcmp(target->classname, "point_combat") != 0)
			{
				gi.dprintf("%s at (%i %i %i) has a bad combattarget %s : %s at (%i %i %i)\n",
					self->classname, (int)self->s.origin[0], (int)self->s.origin[1], (int)self->s.origin[2],
					self->combattarget, target->classname, (int)target->s.origin[0], (int)target->s.origin[1],
					(int)target->s.origin[2]);
			}
		}
	}

	if (self->target)
	{
		self->goalentity = self->movetarget = G_PickTarget(self->target);
		if (!self->movetarget)
		{
			gi.dprintf ("%s can't find target %s at %s\n", self->classname, self->target, vtos(self->s.origin));
			self->target = NULL;
			self->monsterinfo.pausetime = 100000000;
			self->monsterinfo.stand (self);
		}
		else if (strcmp (self->movetarget->classname, "path_corner") == 0)
		{
			VectorSubtract (self->goalentity->s.origin, self->s.origin, v);
			self->ideal_yaw = self->s.angles[YAW] = vectoyaw(v);
			self->monsterinfo.walk (self);
			self->target = NULL;
		}
		else
		{
			self->goalentity = self->movetarget = NULL;
			self->monsterinfo.pausetime = 100000000;
			self->monsterinfo.stand (self);
		}
	}
	else
	{
		self->monsterinfo.pausetime = 100000000;
		self->monsterinfo.stand (self);
	}

	self->think = monster_think;
	self->nextthink = level.time + FRAMETIME;
}


void walkmonster_start_go (edict_t *self)
{
	if (!(self->spawnflags & 2) && level.time < 1)
	{
		M_droptofloor (self);

		if (self->groundentity)
			if (!M_walkmove (self, 0, 0))
				gi.dprintf ("%s in solid at %s\n", self->classname, vtos(self->s.origin));
	}

	if (!self->yaw_speed)
		self->yaw_speed = 20;
	self->viewheight = 25;

	monster_start_go (self);

	if (self->spawnflags & 2)
		monster_triggered_start (self);
}

void walkmonster_start (edict_t *self)
{
	self->s.angles[PITCH] = 0.0;
	self->s.angles[ROLL] = 0.0;
	self->think = walkmonster_start_go;
	monster_start (self);
}


void flymonster_start_go (edict_t *self)
{
	if (!M_walkmove (self, 0, 0))
		gi.dprintf ("%s in solid at %s\n", self->classname, vtos(self->s.origin));

	if (!self->yaw_speed)
		self->yaw_speed = 10;
	self->viewheight = 25;

	monster_start_go (self);

	if (self->spawnflags & 2)
		monster_triggered_start (self);
}


void flymonster_start (edict_t *self)
{
	self->flags |= FL_FLY;
	self->think = flymonster_start_go;
	monster_start (self);
}


void swimmonster_start_go (edict_t *self)
{
	if (!self->yaw_speed)
		self->yaw_speed = 10;
	self->viewheight = 10;

	monster_start_go (self);

	if (self->spawnflags & 2)
		monster_triggered_start (self);
}

void swimmonster_start (edict_t *self)
{
	self->flags |= FL_SWIM;
	self->think = swimmonster_start_go;
	monster_start (self);
}

//Since Alien Arena has no real "monsters" other than these, just place them all here in one locale.

static int	sound_chomp;

void fish_stand (edict_t *self);

qboolean bite (edict_t *self, vec3_t aim, int damage, int kick)
{
	trace_t		tr;
	vec3_t		forward, right, up;
	vec3_t		v;
	vec3_t		point;
	float		range;
	vec3_t		dir;

	//see if enemy is in range
	VectorSubtract (self->enemy->s.origin, self->s.origin, dir);
	range = VectorLength(dir);
	if (range > aim[0])
		return false;

	if (aim[1] > self->mins[0] && aim[1] < self->maxs[0])
	{
		// the hit is straight on so back the range up to the edge of their bbox
		range -= self->enemy->maxs[0];
	}
	else
	{
		// this is a side hit so adjust the "right" value out to the edge of their bbox
		if (aim[1] < 0)
			aim[1] = self->enemy->mins[0];
		else
			aim[1] = self->enemy->maxs[0];
	}

	VectorMA (self->s.origin, range, dir, point);

	tr = gi.trace (self->s.origin, NULL, NULL, point, self, MASK_SHOT);
	if (tr.fraction < 1)
	{
		if (!tr.ent->takedamage)
			return false;
		// if it will hit any client/monster then hit the one we wanted to hit
		if ((tr.ent->svflags & SVF_MONSTER) || (tr.ent->client))
			tr.ent = self->enemy;
	}

	AngleVectors(self->s.angles, forward, right, up);
	VectorMA (self->s.origin, range, forward, point);
	VectorMA (point, aim[1], right, point);
	VectorMA (point, aim[2], up, point);
	VectorSubtract (point, self->enemy->s.origin, dir);

	// do the damage
	T_Damage (tr.ent, self, self, dir, point, vec3_origin, damage, kick/2, DAMAGE_NO_KNOCKBACK, MOD_VIOLATOR);

	gi.sound (self, CHAN_VOICE, sound_chomp, 1, ATTN_NORM, 0);

	if (!(tr.ent->svflags & SVF_MONSTER) && (!tr.ent->client))
		return false;

	// do our special form of knockback here
	VectorMA (self->enemy->absmin, 0.5, self->enemy->size, v);
	VectorSubtract (v, point, v);
	VectorNormalize (v);
	VectorMA (self->enemy->velocity, kick, v, self->enemy->velocity);
	if (self->enemy->velocity[2] > 0)
		self->enemy->groundentity = NULL;
	return true;
}

mframe_t fish_frames_stand [] =
{
	ai_stand, 0, NULL
};
	
mmove_t	fish_move_stand = {0, 0, fish_frames_stand, NULL};

void fish_stand (edict_t *self)
{
		self->monsterinfo.currentmove = &fish_move_stand;
}

#define FISH_SWIM_SPEED	12

mframe_t fish_frames_run [] =
{
	ai_run, FISH_SWIM_SPEED, NULL,	
	ai_run, FISH_SWIM_SPEED, NULL,
	ai_run, FISH_SWIM_SPEED, NULL,
	ai_run, FISH_SWIM_SPEED, NULL,
	ai_run, FISH_SWIM_SPEED, NULL,	
	ai_run, FISH_SWIM_SPEED, NULL,
	ai_run, FISH_SWIM_SPEED, NULL,
	ai_run, FISH_SWIM_SPEED, NULL,
	ai_run, FISH_SWIM_SPEED, NULL,
	ai_run, FISH_SWIM_SPEED, NULL,
	ai_run, FISH_SWIM_SPEED, NULL,
	ai_run, FISH_SWIM_SPEED, NULL,
	ai_run, FISH_SWIM_SPEED, NULL,
	ai_run, FISH_SWIM_SPEED, NULL,
	ai_run, FISH_SWIM_SPEED, NULL,	
	ai_run, FISH_SWIM_SPEED, NULL,
	ai_run, FISH_SWIM_SPEED, NULL,	
	ai_run, FISH_SWIM_SPEED, NULL		
};
mmove_t fish_move_run = {56, 73, fish_frames_run, NULL};

void fish_run (edict_t *self)
{
	self->monsterinfo.currentmove = &fish_move_run;
}

/* Standard Swimming */ 
mframe_t fish_frames_walk [] =
{
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL,
	ai_walk, 4, NULL
};
mmove_t fish_move_walk = {0, 54, fish_frames_walk, NULL};

void fish_walk (edict_t *self)
{
	self->monsterinfo.currentmove = &fish_move_walk;
}

void fish_bite (edict_t *self)
{
	vec3_t	aim;

	VectorSet (aim, MELEE_DISTANCE, 0, 0);
	bite (self, aim, 5, 0);
}


mframe_t fish_frames_attack [] =
{
	ai_charge, FISH_SWIM_SPEED,	NULL,
	ai_charge, FISH_SWIM_SPEED,	NULL,
	ai_charge, FISH_SWIM_SPEED,	NULL,
	ai_charge, FISH_SWIM_SPEED,	NULL,
	ai_charge, FISH_SWIM_SPEED,	NULL,
	ai_charge, FISH_SWIM_SPEED,	NULL,
	ai_charge, FISH_SWIM_SPEED,	NULL,
	ai_charge, FISH_SWIM_SPEED,	NULL,
	ai_charge, FISH_SWIM_SPEED,	NULL,
	ai_charge, FISH_SWIM_SPEED,	NULL,
	ai_charge, FISH_SWIM_SPEED,	NULL,
	ai_charge, FISH_SWIM_SPEED,	NULL,
	ai_charge, FISH_SWIM_SPEED,	NULL,
	ai_charge, FISH_SWIM_SPEED,	fish_bite,
	ai_charge, FISH_SWIM_SPEED,	NULL,
	ai_charge, FISH_SWIM_SPEED,	NULL,
	ai_charge, FISH_SWIM_SPEED,	NULL,
	ai_charge, FISH_SWIM_SPEED,	NULL,
	ai_charge, FISH_SWIM_SPEED,	NULL,
	ai_charge, FISH_SWIM_SPEED,	NULL,
	ai_charge, FISH_SWIM_SPEED,	NULL,
	ai_charge, FISH_SWIM_SPEED,	NULL,
	ai_charge, FISH_SWIM_SPEED,	fish_bite,
	ai_charge, FISH_SWIM_SPEED,	NULL
};
mmove_t fish_move_attack = {74, 97, fish_frames_attack, fish_run};

void fish_melee(edict_t *self)
{
	self->monsterinfo.currentmove = &fish_move_attack;
}

void fish_sight (edict_t *self, edict_t *other)
{
	self->monsterinfo.currentmove = &fish_move_run;
}

void fish_pain (edict_t *self, edict_t *other, float kick, int damage)
{

}

void fish_search (edict_t *self)
{

}

void fish_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;

	self->deadflag = DEAD_DEAD;

	self->think = G_FreeEdict;

	gi.linkentity (self);

	G_FreeEdict(self);
}

void SP_monster_piranha (edict_t *self)
{
	sound_chomp		= gi.soundindex ("misc/piranha.wav");

	self->movetype = MOVETYPE_STEP;
	self->solid = SOLID_BBOX;
	self->s.modelindex = gi.modelindex ("models/misc/piranha/tris.iqm");
	VectorSet (self->mins, -24, -24, 0);
	VectorSet (self->maxs, 24, 24, 24);

	self->takedamage = DAMAGE_YES; 
	self->health = 50;
	self->gib_health = -30;
	self->mass = 100;

	self->monsterinfo.sight = fish_sight;
	self->monsterinfo.stand = fish_stand;
	self->monsterinfo.walk = fish_walk;
	self->monsterinfo.run = fish_run;
	self->monsterinfo.melee = fish_melee;
	self->monsterinfo.attack = fish_run;
	self->monsterinfo.sight = fish_sight;
	self->monsterinfo.search = fish_search;
	self->monsterinfo.dodge = NULL;
	self->s.renderfx |= RF_MONSTER;

	self->pain = fish_pain;
	self->die = fish_die;
	self->enemy = NULL;
	self->monsterinfo.scale = 1.0;

	gi.linkentity (self);

	self->monsterinfo.currentmove = &fish_move_stand;	

	swimmonster_start (self);
}


