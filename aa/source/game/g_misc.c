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
// g_misc.c

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "g_local.h"


/*QUAKED func_group (0 0 0) ?
Used to group brushes together just for editor convenience.
*/

//=====================================================

void Use_Areaportal (edict_t *ent, edict_t *other, edict_t *activator)
{
	ent->count ^= 1;		// toggle state
//	gi.dprintf ("portalstate: %i = %i\n", ent->style, ent->count);
	gi.SetAreaPortalState (ent->style, ent->count);
}

/*QUAKED func_areaportal (0 0 0) ?

This is a non-visible object that divides the world into
areas that are seperated when this portal is not activated.
Usually enclosed in the middle of a door.
*/
void SP_func_areaportal (edict_t *ent)
{
	ent->use = Use_Areaportal;
	ent->count = 0;		// always start closed;
}

//=====================================================


/*
=================
Misc functions
=================
*/
void VelocityForDamage (int damage, vec3_t v)
{
	v[0] = 100.0 * crandom();
	v[1] = 100.0 * crandom();
	v[2] = 200.0 + 100.0 * random();

	if (damage < 50)
		VectorScale (v, 0.7, v);
	else
		VectorScale (v, 1.2, v);
}

void ClipGibVelocity (edict_t *ent)
{
	if (ent->velocity[0] < -300)
		ent->velocity[0] = -300;
	else if (ent->velocity[0] > 300)
		ent->velocity[0] = 300;
	if (ent->velocity[1] < -300)
		ent->velocity[1] = -300;
	else if (ent->velocity[1] > 300)
		ent->velocity[1] = 300;
	if (ent->velocity[2] < 200)
		ent->velocity[2] = 200;	// always some upwards
	else if (ent->velocity[2] > 500)
		ent->velocity[2] = 500;
}


/*
=================
gibs
=================
*/
void gib_think (edict_t *self)
{
	self->s.frame++;
	self->nextthink = level.time + TENFPS; 

	if (self->s.frame > 9)
	{
		self->think = G_FreeEdict;
		self->nextthink = level.time + 2 + random()*2;
	}
}

void gib_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t	normal_angles, right;

	if (!self->groundentity)
		return;

	self->touch = NULL;

	if (plane)
	{
		gi.sound (self, CHAN_VOICE, gi.soundindex ("misc/fhit3.wav"), 1, ATTN_NORM, 0);

		vectoangles (plane->normal, normal_angles);
		AngleVectors (normal_angles, NULL, right, NULL);
		vectoangles (right, self->s.angles);

		if (self->s.modelindex == sm_meat_index)
		{
			self->s.frame = 0;//++;
			self->think = gib_think;
			self->nextthink = level.time + TENFPS;
		}
	}
}

void gib_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	G_FreeEdict (self);
}

void ThrowGib (edict_t *self, char *gibname, int damage, int type, int effects)
{
	edict_t *gib;
	vec3_t	vd;
	vec3_t	origin;
	vec3_t	size;
	float	vscale;

	gib = G_Spawn();

	gib->classname = "g_gib";

	VectorScale (self->size, 1, size);
	VectorAdd (self->absmin, size, origin);
	gib->s.origin[0] = origin[0] + crandom() * size[0];
	gib->s.origin[1] = origin[1] + crandom() * size[1];
	gib->s.origin[2] = origin[2] + crandom() * size[2];

	gi.setmodel (gib, gibname);
	gib->solid = SOLID_NOT;
	gib->s.effects = effects;
	gib->flags |= FL_NO_KNOCKBACK;
	gib->takedamage = DAMAGE_YES;
	gib->die = gib_die;

	if (type == GIB_ORGANIC)
	{
		gib->movetype = MOVETYPE_BOUNCE;
		gib->touch = gib_touch;
		vscale = 0.2;
	}
	else
	{
		gib->movetype = MOVETYPE_BOUNCE;
		vscale = 1.0;
	}

	VelocityForDamage (damage, vd);
	VectorMA (self->velocity, vscale, vd, gib->velocity);
	ClipGibVelocity (gib);
	gib->avelocity[0] = random()*600;
	gib->avelocity[1] = random()*600;
	gib->avelocity[2] = random()*600;

	gib->think = G_FreeEdict;
	gib->nextthink = level.time + 2 + random()*2;

	gi.linkentity (gib);
}

void ThrowClientHead (edict_t *self, int damage)
{
	vec3_t	vd;
	char	*gibname;

	gibname = "models/objects/gibs/sm_meat/tris.iqm";
	self->s.skinnum = 0;

	self->s.origin[2] += 32;
	self->s.frame = 0;
	gi.setmodel (self, gibname);
	VectorSet (self->mins, -16, -16, 0);
	VectorSet (self->maxs, 16, 16, 16);

	self->takedamage = DAMAGE_NO;
	self->solid = SOLID_NOT;
	self->s.effects = EF_GIB;
	self->s.sound = 0;
	self->flags |= FL_NO_KNOCKBACK;

	self->movetype = MOVETYPE_TOSS;
	VelocityForDamage (damage, vd);
	VectorAdd (self->velocity, vd, self->velocity);

	if (self->client)	// bodies in the queue don't have a client anymore
	{
		self->client->anim_priority = ANIM_DEATH;
		self->client->anim_end = self->s.frame;
	}
	else
	{
		self->think = NULL;
		self->nextthink = 0;
	}

	gi.linkentity (self);
}


/*
=================
debris
=================
*/
void debris_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	G_FreeEdict (self);
}

void ThrowDebris (edict_t *self, char *modelname, float speed, vec3_t origin)
{
	edict_t	*chunk;
	vec3_t	v;

	chunk = G_Spawn();
	VectorCopy (origin, chunk->s.origin);
	gi.setmodel (chunk, modelname);
	v[0] = 100 * crandom();
	v[1] = 100 * crandom();
	v[2] = 100 + 100 * crandom();
	VectorMA (self->velocity, speed, v, chunk->velocity);
	chunk->movetype = MOVETYPE_BOUNCE;
	chunk->solid = SOLID_NOT;
	chunk->avelocity[0] = random()*600;
	chunk->avelocity[1] = random()*600;
	chunk->avelocity[2] = random()*600;
	chunk->s.effects |= EF_ROCKET;
	chunk->think = G_FreeEdict;
	chunk->nextthink = level.time + 5 + random()*5;
	chunk->s.frame = 0;
	chunk->flags = 0;
	chunk->classname = "debris";
	chunk->takedamage = DAMAGE_YES;
	chunk->die = debris_die;
	gi.linkentity (chunk);
}

void ThrowDebris2 (edict_t *self, char *modelname, float speed, vec3_t origin)
{
	edict_t	*chunk;
	vec3_t	v;

	chunk = G_Spawn();
	VectorCopy (origin, chunk->s.origin);
	gi.setmodel (chunk, modelname);
	v[0] = 100 * crandom();
	v[1] = 100 * crandom();
	v[2] = 100 + 100 * crandom();
	VectorMA (self->velocity, speed, v, chunk->velocity);
	chunk->movetype = MOVETYPE_BOUNCE;
	chunk->solid = SOLID_NOT;
	chunk->avelocity[0] = random()*600;
	chunk->avelocity[1] = random()*600;
	chunk->avelocity[2] = random()*600;
	chunk->think = G_FreeEdict;
	chunk->nextthink = level.time + 5 + random()*5;
	chunk->s.frame = 0;
	chunk->flags = 0;
	chunk->classname = "debris";
	chunk->takedamage = DAMAGE_YES;
	chunk->die = debris_die;
	gi.linkentity (chunk);
}

void BecomeExplosion1 (edict_t *self)
{
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION1);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	G_FreeEdict (self);
}


void BecomeExplosion2 (edict_t *self)
{
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_EXPLOSION2);
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	G_FreeEdict (self);
}


/*QUAKED path_corner (.5 .3 0) (-8 -8 -8) (8 8 8) TELEPORT
Target: next path corner
Pathtarget: gets used when an entity that has
	this path_corner targeted touches it
*/

void path_corner_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	vec3_t		v;
	edict_t		*next;

	if (other->movetarget != self)
		return;

	if (other->enemy)
		return;

	if (self->pathtarget)
	{
		char *savetarget;

		savetarget = self->target;
		self->target = self->pathtarget;
		G_UseTargets (self, other);
		self->target = savetarget;
	}

	if (self->target)
		next = G_PickTarget(self->target);
	else
		next = NULL;

	if ((next) && (next->spawnflags & 1))
	{
		VectorCopy (next->s.origin, v);
		v[2] += next->mins[2];
		v[2] -= other->mins[2];
		VectorCopy (v, other->s.origin);
		next = G_PickTarget(next->target);
		other->s.event = EV_OTHER_TELEPORT;
	}

	other->goalentity = other->movetarget = next;

	if (self->wait)
	{
		other->monsterinfo.pausetime = level.time + self->wait;
		other->monsterinfo.stand (other);
		return;
	}

	if (!other->movetarget)
	{
		other->monsterinfo.pausetime = level.time + 100000000;
		other->monsterinfo.stand (other);
	}
	else
	{
		VectorSubtract (other->goalentity->s.origin, other->s.origin, v);
		other->ideal_yaw = vectoyaw (v);
	}
}

void SP_path_corner (edict_t *self)
{
	if (!self->targetname)
	{
		gi.dprintf ("path_corner with no targetname at %s\n", vtos(self->s.origin));
		G_FreeEdict (self);
		return;
	}

	self->solid = SOLID_TRIGGER;
	self->touch = path_corner_touch;
	VectorSet (self->mins, -8, -8, -8);
	VectorSet (self->maxs, 8, 8, 8);
	self->svflags |= SVF_NOCLIENT;
	gi.linkentity (self);
}


/*QUAKED point_combat (0.5 0.3 0) (-8 -8 -8) (8 8 8) Hold
Makes this the target of a monster and it will head here
when first activated before going after the activator.  If
hold is selected, it will stay here.
*/
void point_combat_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	edict_t	*activator;

	if (other->movetarget != self)
		return;

	if (self->target)
	{
		other->target = self->target;
		other->goalentity = other->movetarget = G_PickTarget(other->target);
		if (!other->goalentity)
		{
			gi.dprintf("%s at %s target %s does not exist\n", self->classname, vtos(self->s.origin), self->target);
			other->movetarget = self;
		}
		self->target = NULL;
	}
	else if ((self->spawnflags & 1) && !(other->flags & (FL_SWIM|FL_FLY)))
	{
		other->monsterinfo.pausetime = level.time + 100000000;
		other->monsterinfo.aiflags |= AI_STAND_GROUND;
		other->monsterinfo.stand (other);
	}

	if (other->movetarget == self)
	{
		other->target = NULL;
		other->movetarget = NULL;
		other->goalentity = other->enemy;
		other->monsterinfo.aiflags &= ~AI_COMBAT_POINT;
	}

	if (self->pathtarget)
	{
		char *savetarget;

		savetarget = self->target;
		self->target = self->pathtarget;
		if (other->enemy && other->enemy->client)
			activator = other->enemy;
		else if (other->oldenemy && other->oldenemy->client)
			activator = other->oldenemy;
		else if (other->activator && other->activator->client)
			activator = other->activator;
		else
			activator = other;
		G_UseTargets (self, activator);
		self->target = savetarget;
	}
}

void SP_point_combat (edict_t *self)
{
	if (deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}
	self->solid = SOLID_TRIGGER;
	self->touch = point_combat_touch;
	VectorSet (self->mins, -8, -8, -16);
	VectorSet (self->maxs, 8, 8, 16);
	self->svflags = SVF_NOCLIENT;
	gi.linkentity (self);
};

/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for spotlights, etc.
*/
void SP_info_null (edict_t *self)
{
	G_FreeEdict (self);
};


/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for lightning.
*/
void SP_info_notnull (edict_t *self)
{
	VectorCopy (self->s.origin, self->absmin);
	VectorCopy (self->s.origin, self->absmax);
};


/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) START_OFF
Non-displayed light.
Default light value is 300.
Default style is 0.
If targeted, will toggle between on and off.
Default _cone value is 10 (used to set size of light for spotlights)
*/

#define START_OFF	1

static void light_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->spawnflags & START_OFF)
	{
		gi.configstring (CS_LIGHTS+self->style, "m");
		self->spawnflags &= ~START_OFF;
	}
	else
	{
		gi.configstring (CS_LIGHTS+self->style, "a");
		self->spawnflags |= START_OFF;
	}
}

void SP_light (edict_t *self)
{
	// no targeted lights in deathmatch, because they cause global messages
	if (!self->targetname || deathmatch->value)
	{
		G_FreeEdict (self);
		return;
	}

	if (self->style >= 32)
	{
		self->use = light_use;
		if (self->spawnflags & START_OFF)
			gi.configstring (CS_LIGHTS+self->style, "a");
		else
			gi.configstring (CS_LIGHTS+self->style, "m");
	}
}


/*QUAKED func_wall (0 .5 .8) ? TRIGGER_SPAWN TOGGLE START_ON ANIMATED ANIMATED_FAST
This is just a solid wall if not inhibited

TRIGGER_SPAWN	the wall will not be present until triggered
				it will then blink in to existance; it will
				kill anything that was in it's way

TOGGLE			only valid for TRIGGER_SPAWN walls
				this allows the wall to be turned on and off

START_ON		only valid for TRIGGER_SPAWN walls
				the wall will initially be present
*/

void func_wall_use (edict_t *self, edict_t *other, edict_t *activator)
{
	if (self->solid == SOLID_NOT)
	{
		self->solid = SOLID_BSP;
		self->svflags &= ~SVF_NOCLIENT;
		KillBox (self);
	}
	else
	{
		self->solid = SOLID_NOT;
		self->svflags |= SVF_NOCLIENT;
	}
	gi.linkentity (self);

	if (!(self->spawnflags & 2))
		self->use = NULL;
}

void SP_func_wall (edict_t *self)
{
	self->movetype = MOVETYPE_PUSH;
	gi.setmodel (self, self->model);

	if (self->spawnflags & 8)
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags & 16)
		self->s.effects |= EF_ANIM_ALLFAST;

	// just a wall
	if ((self->spawnflags & 7) == 0)
	{
		self->solid = SOLID_BSP;
		gi.linkentity (self);
		return;
	}

	// it must be TRIGGER_SPAWN
	if (!(self->spawnflags & 1))
	{
//		gi.dprintf("func_wall missing TRIGGER_SPAWN\n");
		self->spawnflags |= 1;
	}

	// yell if the spawnflags are odd
	if (self->spawnflags & 4)
	{
		if (!(self->spawnflags & 2))
		{
			gi.dprintf("func_wall START_ON without TOGGLE\n");
			self->spawnflags |= 2;
		}
	}

	self->use = func_wall_use;
	if (self->spawnflags & 4)
	{
		self->solid = SOLID_BSP;
	}
	else
	{
		self->solid = SOLID_NOT;
		self->svflags |= SVF_NOCLIENT;
	}
	gi.linkentity (self);
}


/*QUAKED func_object (0 .5 .8) ? TRIGGER_SPAWN ANIMATED ANIMATED_FAST
This is solid bmodel that will fall if it's support it removed.
*/

void func_object_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	// only squash thing we fall on top of
	if (!plane)
		return;
	if (plane->normal[2] < 1.0)
		return;
	if (other->takedamage == DAMAGE_NO)
		return;
	T_Damage (other, self, self, vec3_origin, self->s.origin, vec3_origin, self->dmg, 1, 0, MOD_CRUSH);
}

void func_object_release (edict_t *self)
{
	self->movetype = MOVETYPE_TOSS;
	self->touch = func_object_touch;
}

void func_object_use (edict_t *self, edict_t *other, edict_t *activator)
{
	self->solid = SOLID_BSP;
	self->svflags &= ~SVF_NOCLIENT;
	self->use = NULL;
	KillBox (self);
	func_object_release (self);
}

void SP_func_object (edict_t *self)
{
	gi.setmodel (self, self->model);

	self->mins[0] += 1;
	self->mins[1] += 1;
	self->mins[2] += 1;
	self->maxs[0] -= 1;
	self->maxs[1] -= 1;
	self->maxs[2] -= 1;

	if (!self->dmg)
		self->dmg = 100;

	if (self->spawnflags == 0)
	{
		self->solid = SOLID_BSP;
		self->movetype = MOVETYPE_PUSH;
		self->think = func_object_release;
		self->nextthink = level.time + 2 * TENFPS;
	}
	else
	{
		self->solid = SOLID_NOT;
		self->movetype = MOVETYPE_PUSH;
		self->use = func_object_use;
		self->svflags |= SVF_NOCLIENT;
	}

	if (self->spawnflags & 2)
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags & 4)
		self->s.effects |= EF_ANIM_ALLFAST;

	self->clipmask = MASK_MONSTERSOLID;

	gi.linkentity (self);
}


/*QUAKED func_explosive (0 .5 .8) ? Trigger_Spawn ANIMATED ANIMATED_FAST
Any brush that you want to explode or break apart.  If you want an
ex0plosion, set dmg and it will do a radius explosion of that amount
at the center of the bursh.

If targeted it will not be shootable.

health defaults to 100.

mass defaults to 75.  This determines how much debris is emitted when
it explodes.  You get one large chunk per 100 of mass (up to 8) and
one small chunk per 25 of mass (up to 16).  So 800 gives the most.
*/
void func_explosive_explode (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	vec3_t	origin;
	vec3_t	chunkorigin;
	vec3_t	size;
	int		count;
	int		mass;

	// bmodel origins are (0 0 0), we need to adjust that here
	VectorScale (self->size, 0.5, size);
	VectorAdd (self->absmin, size, origin);
	VectorCopy (origin, self->s.origin);

	self->takedamage = DAMAGE_NO;

	if (self->dmg)
		T_RadiusDamage (self, attacker, self->dmg, NULL, self->dmg+40, MOD_EXPLOSIVE, -1);

	VectorSubtract (self->s.origin, inflictor->s.origin, self->velocity);
	VectorNormalize (self->velocity);
	VectorScale (self->velocity, 150, self->velocity);

	// start chunks towards the center
	VectorScale (size, 0.5, size);

	mass = self->mass;
	if (!mass)
		mass = 75;

	// big chunks
	if (mass >= 100)
	{
		count = mass / 100;
		if (count > 8)
			count = 8;
		while(count--)
		{
			chunkorigin[0] = origin[0] + crandom() * size[0];
			chunkorigin[1] = origin[1] + crandom() * size[1];
			chunkorigin[2] = origin[2] + crandom() * size[2];
			ThrowDebris (self, "models/objects/debris1/tris.iqm", 1, chunkorigin);
		}
	}

	// small chunks
	count = mass / 25;
	if (count > 16)
		count = 16;
	while(count--)
	{
		chunkorigin[0] = origin[0] + crandom() * size[0];
		chunkorigin[1] = origin[1] + crandom() * size[1];
		chunkorigin[2] = origin[2] + crandom() * size[2];
		ThrowDebris (self, "models/objects/debris2/tris.iqm", 2, chunkorigin);
	}

	G_UseTargets (self, attacker);

	if (self->dmg)
		BecomeExplosion1 (self);
	else
		G_FreeEdict (self);
}

void func_explosive_use(edict_t *self, edict_t *other, edict_t *activator)
{
	func_explosive_explode (self, self, other, self->health, vec3_origin);
}

void func_explosive_spawn (edict_t *self, edict_t *other, edict_t *activator)
{
	self->solid = SOLID_BSP;
	self->svflags &= ~SVF_NOCLIENT;
	self->use = NULL;
	KillBox (self);
	gi.linkentity (self);
}

void SP_func_explosive (edict_t *self)
{
	self->movetype = MOVETYPE_PUSH;

	gi.modelindex ("models/objects/debris1/tris.iqm");
	gi.modelindex ("models/objects/debris2/tris.iqm");

	gi.setmodel (self, self->model);

	if (self->spawnflags & 1)
	{
		self->svflags |= SVF_NOCLIENT;
		self->solid = SOLID_NOT;
		self->use = func_explosive_spawn;
	}
	else
	{
		self->solid = SOLID_BSP;
		if (self->targetname)
			self->use = func_explosive_use;
	}

	if (self->spawnflags & 2)
		self->s.effects |= EF_ANIM_ALL;
	if (self->spawnflags & 4)
		self->s.effects |= EF_ANIM_ALLFAST;

	if (self->use != func_explosive_use)
	{
		if (!self->health)
			self->health = 100;
		self->die = func_explosive_explode;
		self->takedamage = DAMAGE_YES;
	}

	gi.linkentity (self);
}


/*QUAKED misc_explobox (0 .5 .8) (-16 -16 0) (16 16 40)
Large exploding box.  You can override its mass (100),
health (80), and dmg (150).
*/

void barrel_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)

{
	float	ratio;
	vec3_t	v;

	if ((!other->groundentity) || (other->groundentity == self))
		return;

	ratio = (float)other->mass / (float)self->mass;
	VectorSubtract (self->s.origin, other->s.origin, v);
	M_walkmove (self, vectoyaw(v), 20 * ratio * TENFPS);
}

void barrel_explode (edict_t *self)
{
	vec3_t	org;
	float	spd;
	vec3_t	save, size;

	T_RadiusDamage (self, self->activator, self->dmg, NULL, self->dmg+40, MOD_BARREL, -1);

	VectorCopy (self->s.origin, save);
	VectorMA (self->absmin, 0.5, self->size, self->s.origin);
	VectorScale (self->size, 0.5, size);

	// a few big chunks
	spd = 1.5 * (float)self->dmg / 200.0;
	org[0] = self->s.origin[0] + crandom() * size[0];
	org[1] = self->s.origin[1] + crandom() * size[1];
	org[2] = self->s.origin[2] + crandom() * size[2];
	ThrowDebris (self, "models/objects/debris1/tris.iqm", spd, org);
	org[0] = self->s.origin[0] + crandom() * size[0];
	org[1] = self->s.origin[1] + crandom() * size[1];
	org[2] = self->s.origin[2] + crandom() * size[2];
	ThrowDebris (self, "models/objects/debris1/tris.iqm", spd, org);

	// bottom corners
	spd = 1.75 * (float)self->dmg / 200.0;
	VectorCopy (self->absmin, org);
	ThrowDebris (self, "models/objects/debris3/tris.iqm", spd, org);
	VectorCopy (self->absmin, org);
	org[0] += self->size[0];
	ThrowDebris (self, "models/objects/debris3/tris.iqm", spd, org);
	VectorCopy (self->absmin, org);
	org[1] += self->size[1];
	ThrowDebris (self, "models/objects/debris3/tris.iqm", spd, org);
	VectorCopy (self->absmin, org);
	org[0] += self->size[0];
	org[1] += self->size[1];
	ThrowDebris (self, "models/objects/debris3/tris.iqm", spd, org);

	// a bunch of little chunks
	spd = 2 * self->dmg / 200;
	org[0] = self->s.origin[0] + crandom() * size[0];
	org[1] = self->s.origin[1] + crandom() * size[1];
	org[2] = self->s.origin[2] + crandom() * size[2];
	ThrowDebris (self, "models/objects/debris2/tris.iqm", spd, org);
	org[0] = self->s.origin[0] + crandom() * size[0];
	org[1] = self->s.origin[1] + crandom() * size[1];
	org[2] = self->s.origin[2] + crandom() * size[2];
	ThrowDebris (self, "models/objects/debris2/tris.iqm", spd, org);
	org[0] = self->s.origin[0] + crandom() * size[0];
	org[1] = self->s.origin[1] + crandom() * size[1];
	org[2] = self->s.origin[2] + crandom() * size[2];
	ThrowDebris (self, "models/objects/debris2/tris.iqm", spd, org);
	org[0] = self->s.origin[0] + crandom() * size[0];
	org[1] = self->s.origin[1] + crandom() * size[1];
	org[2] = self->s.origin[2] + crandom() * size[2];
	ThrowDebris (self, "models/objects/debris2/tris.iqm", spd, org);
	org[0] = self->s.origin[0] + crandom() * size[0];
	org[1] = self->s.origin[1] + crandom() * size[1];
	org[2] = self->s.origin[2] + crandom() * size[2];
	ThrowDebris (self, "models/objects/debris2/tris.iqm", spd, org);
	org[0] = self->s.origin[0] + crandom() * size[0];
	org[1] = self->s.origin[1] + crandom() * size[1];
	org[2] = self->s.origin[2] + crandom() * size[2];
	ThrowDebris (self, "models/objects/debris2/tris.iqm", spd, org);
	org[0] = self->s.origin[0] + crandom() * size[0];
	org[1] = self->s.origin[1] + crandom() * size[1];
	org[2] = self->s.origin[2] + crandom() * size[2];
	ThrowDebris (self, "models/objects/debris2/tris.iqm", spd, org);
	org[0] = self->s.origin[0] + crandom() * size[0];
	org[1] = self->s.origin[1] + crandom() * size[1];
	org[2] = self->s.origin[2] + crandom() * size[2];
	ThrowDebris (self, "models/objects/debris2/tris.iqm", spd, org);

	VectorCopy (save, self->s.origin);
	if (self->groundentity)
		BecomeExplosion2 (self);
	else
		BecomeExplosion1 (self);
}

void barrel_delay (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->takedamage = DAMAGE_NO;
	self->nextthink = level.time + 2 * TENFPS;
	self->think = barrel_explode;
	self->activator = attacker;
}

//=================================================================================

void teleporter_touch (edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	edict_t		*dest;
	int			i;

	if (!other->client)
		return;
	dest = G_Find (NULL, FOFS(targetname), self->target);
	if (!dest)
	{
		gi.dprintf ("Couldn't find destination\n");
		return;
	}

	//ZOID
	CTFPlayerResetGrapple(other);
	//ZOID

	// unlink to make sure it can't possibly interfere with KillBox
	gi.unlinkentity (other);

	VectorCopy (dest->s.origin, other->s.origin);
	VectorCopy (dest->s.origin, other->s.old_origin);
	other->s.origin[2] += 10;

	// clear the velocity and hold them in place briefly
	VectorClear (other->velocity);
	other->client->ps.pmove.pm_time = 160>>3;		// hold time
	other->client->ps.pmove.pm_flags |= PMF_TIME_TELEPORT;

	// draw the teleport splash at source and on the player
	self->owner->s.event = EV_PLAYER_TELEPORT;
	other->s.event = EV_PLAYER_TELEPORT;

	// set angles
	for (i=0 ; i<3 ; i++)
	{
		other->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(dest->s.angles[i] - other->client->resp.cmd_angles[i]);
	}

	VectorClear (other->s.angles);
	VectorClear (other->client->ps.viewangles);
	VectorClear (other->client->v_angle);

	// kill anything at the destination
	KillBox (other);

	gi.linkentity (other);
}

/*QUAKED misc_teleporter (1 0 0) (-32 -32 -24) (32 32 -16)
Stepping onto this disc will teleport players to the targeted misc_teleporter_dest object.
*/
void SP_misc_teleporter (edict_t *ent)
{
	edict_t		*trig;

	if (!ent->target)
	{
		gi.dprintf ("teleporter without a target.\n");
		G_FreeEdict (ent);
		return;
	}

	// The actual misc_teleporter is converted into a cosmetic object for the
	// glowing effect, while a new entity is spawned to act as the functioning
	// teleporter. 
	gi.setmodel (ent, "models/objects/blank/tris.iqm");
	ent->s.effects = EF_TELEPORTER;	
	ent->s.renderfx |= RF_NODRAW;
	ent->solid     = SOLID_NOT;

	VectorSet (ent->mins, -32, -32, 0);
	VectorSet (ent->maxs, 32, 32, 16);
	gi.linkentity (ent);

	trig = G_Spawn ();
	trig->touch = teleporter_touch;
	trig->solid = SOLID_TRIGGER;
	trig->target = ent->target;
	trig->owner = ent;
	VectorCopy (ent->s.origin, trig->s.origin);
	VectorSet (trig->mins, -8, -8, 8);
	VectorSet (trig->maxs, 8, 8, 24);
	gi.linkentity (trig);
}

/*QUAKED misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16)
Point teleporters at these.
*/
void SP_misc_teleporter_dest (edict_t *ent)
{
	ent->s.modelindex = 0;
	ent->s.skinnum = 0;
	ent->solid = SOLID_NOT;
	VectorSet (ent->mins, -32, -32, -24);
	VectorSet (ent->maxs, 32, 32, -16);
	gi.linkentity (ent);
}

void misc_mapmodel_think (edict_t *ent)
{
	if(ent->spawnflags & 2)
		ent->s.frame = (ent->s.frame + 1) % 39;
	else
		ent->s.frame = (ent->s.frame + 1) % 23;
	ent->nextthink = level.time + TENFPS;
}
void SP_misc_mapmodel (edict_t *ent) //random .md2 map models
{
	gi.setmodel (ent, ent->model);
	
	// we will need clipping brushes around it.
	ent->solid = SOLID_NOT; 

	//disable shadows(also disables per-pixel dynamic lighting unless minlight set
	if(ent->spawnflags & 1)
		ent->s.renderfx = RF_NOSHADOWS;

	if(ent->spawnflags & 16)
		ent->s.renderfx = RF_TRANSLUCENT;

	if(ent->spawnflags & 32) { //animated mesh
		if(ent->spawnflags & 128)
			ent->s.frame = 0;
		else
			ent->s.frame = rand()%24;
		ent->think = misc_mapmodel_think;
		ent->nextthink = level.time + TENFPS;
	}
	else //static mesh
		ent->s.frame = 0;

	//allow dynamic per-pixel lighting
	if(ent->spawnflags & 64)
		ent->s.renderfx |= RF_MINLIGHT;

	gi.linkentity (ent);
}

void watersplash_think (edict_t *ent)
{
	vec3_t up;

	up[0] = 0;
	up[1] = 0;
	up[2] = 1;

	//write effect
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_SPLASH);
	gi.WriteByte (8);
	gi.WritePosition (ent->s.origin);
	gi.WriteDir (up);
	if(ent->spawnflags & 1)
		gi.WriteByte (SPLASH_LAVA);
	else if(ent->spawnflags & 2)
		gi.WriteByte (SPLASH_BLOOD);
	else
		gi.WriteByte (SPLASH_BLUE_WATER); 
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->nextthink = level.time + 1.0;
}

void SP_misc_watersplash (edict_t *ent)
{
	gi.setmodel(ent, NULL);
	ent->solid = SOLID_NOT;
	ent->movetype = MOVETYPE_NONE;
	ent->takedamage = DAMAGE_NO;

	ent->think = watersplash_think;
	ent->nextthink = level.time + 0.5 + random();

	gi.linkentity (ent);
}

void misc_electroflash_think (edict_t *ent)
{
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (ent-g_edicts);
	gi.WriteByte (MZ_RAILGUN);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	ent->nextthink = level.time + 0.05 + random();
}

void SP_misc_electroflash (edict_t *ent) //random electrical pulses
{
	gi.setmodel (ent, NULL);

	ent->solid = SOLID_NOT; 
	ent->movetype = MOVETYPE_NONE;
	ent->takedamage = DAMAGE_NO;
	ent->s.sound = gi.soundindex("world/electricity.wav");
	ent->think = misc_electroflash_think;
	ent->nextthink = level.time + 0.05 + random();

	gi.linkentity (ent);
}

//Tactical base items
//Rules:  When a computer is destroyed, that base's turrents/deathrays will behave eratically, even firing on it's own team on occasion.  Laser barriers shut off.  
//When a powersource is destroyed, the backup generators(if not already destroyed) for the computer and ammo generator turn on(generator models will animate, emit sound).  Ammo will generate from depot at half speed.  
//Laser barriers shut off, turrets and deathrays are weak.
//When an ammo depot is destroyed, ammo stops being produced.
//When all three are disabled, the other team wins.

//lasers
void misc_laser_think (edict_t *self)
{
	edict_t *ent;
	edict_t	*ignore;
	vec3_t	start;
	vec3_t	end;
	trace_t	tr;
	vec3_t	point;
	vec3_t	last_movedir;
	int		count;
	
	if(self->spawnflags & 1)
	{
		if(!tacticalScore.humanComputer || !tacticalScore.humanPowerSource)
		{
			self->nextthink = 0;
			return;
		}
	}
	else if(!tacticalScore.alienComputer || !tacticalScore.alienPowerSource)
	{
		self->nextthink = 0;
		return;
	}

	count = 8;

	if (!self->enemy)
	{
		if (self->target)
		{
			ent = G_Find (NULL, FOFS(targetname), self->target);
			if (!ent)
				gi.dprintf ("%s at %s: %s is a bad target\n", self->classname, vtos(self->s.origin), self->target);
			self->enemy = ent;
		}
		else
		{
			G_SetMovedir (self->s.angles, self->movedir);
		}
	}
	else 
	{
		VectorCopy (self->movedir, last_movedir);
		VectorMA (self->enemy->absmin, 0.5, self->enemy->size, point);
		VectorSubtract (point, self->s.origin, self->movedir);
		VectorNormalize (self->movedir);
	}

	ignore = self;
	VectorCopy (self->s.origin, start);
	VectorCopy (self->enemy->s.origin, end);
	while(1)
	{
		tr = gi.trace (start, NULL, NULL, end, ignore, CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_DEADMONSTER);
		gi.WriteByte (svc_temp_entity);
		if (self->spawnflags & 1)
			gi.WriteByte (TE_REDLASER);
		else
			gi.WriteByte (TE_LASERBEAM);
		gi.WritePosition (start);
		gi.WritePosition (end);
		gi.multicast (start, MULTICAST_PHS);

		if (!tr.ent)
			break;

		//don't hurt anyone on the same team as the laser - spawnflag 1 = human
		if(self->spawnflags & 1) 
		{
			if(tr.ent->ctype == 1)
				break;
		}
		else if(!tr.ent->ctype)
			break;	

		// hurt it if we can
		if (tr.ent->takedamage)
			T_Damage (tr.ent, self, self, self->movedir, tr.endpos, vec3_origin, self->dmg, 1, DAMAGE_ENERGY, MOD_TARGET_LASER);

		// if we hit something that's not a player, we're done
		if (!tr.ent->client)
		{
			break;
		}

		ignore = tr.ent;
		VectorCopy (tr.endpos, start);
	}

	VectorCopy (tr.endpos, self->s.old_origin);

	self->nextthink = level.time + TENFPS;
}

void misc_laser_start (edict_t *self)
{
	
	self->think = misc_laser_think;		
}

void laser_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	self->takedamage = DAMAGE_NO;
	self->activator = attacker;

	gi.WriteByte (svc_temp_entity);
	if(self->spawnflags & 1)
	{
		gi.WriteByte (TE_ROCKET_EXPLOSION); 		
	}
	else
	{
		gi.WriteByte (TE_BFG_BIGEXPLOSION);		
	}
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PHS);
	
	gi.sound( &g_edicts[1], CHAN_AUTO, gi.soundindex( "world/explosion1.wav" ), 1, ATTN_NONE, 0 );

	G_FreeEdict (self);
}

void SP_misc_laser (edict_t *ent)
{
	if (!g_tactical->integer)
	{
		G_FreeEdict (ent);
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->takedamage = DAMAGE_YES; 
	ent->health = 1000;  
	ent->die = laser_die;
	
	ent->dmg = 500; //cause severe damage, especially if multiples(most cases)

	if(ent->spawnflags & 1)
	{
		ent->s.modelindex = gi.modelindex ("models/tactical/human_laser.iqm");
		ent->ctype = 1;
	}
	else
	{
		ent->s.modelindex = gi.modelindex ("models/tactical/alien_laser.iqm");
		ent->ctype = 0;
	}

	VectorSet (ent->mins, -16, -16, -16);
	VectorSet (ent->maxs, 16, 16, 16);

	// let everything else get spawned before we start firing
	ent->think = misc_laser_think;
	ent->nextthink = level.time + 1;

	gi.linkentity (ent);
}


//computers
void computer_think (edict_t *ent)
{
	if(ent->classname == "alien computer")
		tacticalScore.alienComputerHealth = ent->health/15;
	else
		tacticalScore.humanComputerHealth = ent->health/15;

	ent->s.frame = (ent->s.frame + 1) % 39;
	ent->nextthink = level.time + TENFPS;
}

void computer_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	edict_t *cl_ent;
	int i;

	self->takedamage = DAMAGE_NO;
	self->activator = attacker;

	gi.WriteByte (svc_temp_entity);
	if(self->classname == "alien computer")
	{
		tacticalScore.alienComputer = false;
		tacticalScore.alienComputerHealth = 0;
		gi.WriteByte (TE_BFG_BIGEXPLOSION); 
	}
	else
	{
		tacticalScore.humanComputer = false;
		tacticalScore.humanComputerHealth = 0;
		gi.WriteByte (TE_ROCKET_EXPLOSION);
	}
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PHS);

	if(self->classname == "alien computer")
	{
		for (i=0 ; i<g_maxclients->value ; i++)
		{
			cl_ent = g_edicts + 1 + i;
			if (!cl_ent->inuse || cl_ent->is_bot)
				continue;
			safe_centerprintf(cl_ent, "Alien Central Computer has been destroyed!");
		}
	}
	else
	{
		for (i=0 ; i<g_maxclients->value ; i++)
		{
			cl_ent = g_edicts + 1 + i;
			if (!cl_ent->inuse || cl_ent->is_bot)
				continue;
			safe_centerprintf(cl_ent, "Human Central Computer has been destroyed!");
		}
	}
	
	gi.sound( &g_edicts[1], CHAN_AUTO, gi.soundindex( "world/explosion1.wav" ), 1, ATTN_NONE, 0 );

	if(!attacker->is_bot) 
	{
		// Send Steam stats
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte(TE_BASEKILL);
		gi.unicast (attacker, false);
	}

	G_FreeEdict (self);
}

void SP_misc_aliencomputer (edict_t *ent)
{
	if (!g_tactical->integer)
	{
		G_FreeEdict (ent);
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->takedamage = DAMAGE_YES;

	ent->s.modelindex = gi.modelindex ("models/tactical/alien_computer.iqm");

	VectorSet (ent->mins, -64, -64, 0);
	VectorSet (ent->maxs, 64, 64, 64);
	ent->health = 1500; 
	ent->die = computer_die;
	ent->think = computer_think;
	ent->nextthink = level.time + TENFPS;
	ent->classname = "alien computer";
	ent->ctype = 0;

	gi.linkentity (ent);
	M_droptofloor (ent);
}

void SP_misc_humancomputer (edict_t *ent)
{
	if (!g_tactical->integer)
	{
		G_FreeEdict (ent);
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->takedamage = DAMAGE_YES;

	ent->s.modelindex = gi.modelindex ("models/tactical/human_computer.iqm");

	VectorSet (ent->mins, -64, -64, 0);
	VectorSet (ent->maxs, 64, 64, 64);
	ent->health = 1500; 
	ent->die = computer_die;
	ent->think = computer_think;
	ent->nextthink = level.time + TENFPS;
	ent->classname = "human computer";
	ent->ctype = 1;

	gi.linkentity (ent);
	M_droptofloor (ent);
}

//power sources
void powersrc_think (edict_t *ent)
{
	if(ent->classname == "alien powersrc")
		tacticalScore.alienPowerSourceHealth = ent->health/15;
	else
		tacticalScore.humanPowerSourceHealth = ent->health/15;
	ent->s.frame = (ent->s.frame + 1) % 39;
	ent->nextthink = level.time + TENFPS;
}

void powersrc_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	edict_t *cl_ent;
	int i;

	self->takedamage = DAMAGE_NO;
	self->activator = attacker;

	gi.WriteByte (svc_temp_entity);
	if(self->classname == "alien powersrc")
	{
		tacticalScore.alienPowerSource = false;
		tacticalScore.alienPowerSourceHealth = 0;
		gi.WriteByte (TE_BFG_BIGEXPLOSION); 
	}
	else
	{
		tacticalScore.humanPowerSource = false;
		tacticalScore.humanPowerSourceHealth = 0;
		gi.WriteByte (TE_ROCKET_EXPLOSION);		
	}
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PHS);

	if(self->classname == "alien powersrc")
	{
		for (i=0 ; i<g_maxclients->value ; i++)
		{
			cl_ent = g_edicts + 1 + i;
			if (!cl_ent->inuse || cl_ent->is_bot)
				continue;
			safe_centerprintf(cl_ent, "Alien Power Source has been destroyed!");
		}
	}
	else
	{
		for (i=0 ; i<g_maxclients->value ; i++)
		{
			cl_ent = g_edicts + 1 + i;
			if (!cl_ent->inuse || cl_ent->is_bot)
				continue;
			safe_centerprintf(cl_ent, "Human Power Source has been destroyed!");
		}
	}
	
	gi.sound( &g_edicts[1], CHAN_AUTO, gi.soundindex( "world/explosion1.wav" ), 1, ATTN_NONE, 0 );

	if(!attacker->is_bot) 
	{
		// Send Steam stats
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte(TE_BASEKILL);
		gi.unicast (attacker, false);
	}

	G_FreeEdict (self);
}

void SP_misc_alienpowersrc (edict_t *ent)
{
	if (!g_tactical->integer)
	{
		G_FreeEdict (ent);
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->takedamage = DAMAGE_YES;

	ent->s.modelindex = gi.modelindex ("models/tactical/alien_powersrc.iqm");

	VectorSet (ent->mins, -64, -64, 0);
	VectorSet (ent->maxs, 64, 64, 72);
	ent->health = 1500; 
	ent->die = powersrc_die;
	ent->think = powersrc_think;
	ent->nextthink = level.time + TENFPS;
	ent->classname = "alien powersrc";
	ent->ctype = 0;

	gi.linkentity (ent);
	M_droptofloor (ent);
}

void SP_misc_humanpowersrc (edict_t *ent)
{
	if (!g_tactical->integer)
	{
		G_FreeEdict (ent);
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->takedamage = DAMAGE_YES;

	ent->s.modelindex = gi.modelindex ("models/tactical/human_powersrc.iqm");

	VectorSet (ent->mins, -32, -32, 0);
	VectorSet (ent->maxs, 32, 32, 72);
	ent->health = 1500; 
	ent->die = powersrc_die;
	ent->think = powersrc_think;
	ent->nextthink = level.time + TENFPS;
	ent->classname = "human powersrc";
	ent->ctype = 1;

	gi.linkentity (ent);
	M_droptofloor (ent);
}

//ammo depots
void ammodepot_think (edict_t *ent)
{
	if(ent->classname == "alien ammodepot")
		tacticalScore.alienAmmoDepotHealth = ent->health/15;
	else
		tacticalScore.humanAmmoDepotHealth = ent->health/15;
	ent->nextthink = level.time + TENFPS;
}

void ammodepot_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	edict_t	*cl_ent;
	int i;

	self->takedamage = DAMAGE_NO;
	self->activator = attacker;

	gi.WriteByte (svc_temp_entity);
	if(self->classname == "alien ammodepot")
	{
		tacticalScore.alienAmmoDepot = false;
		tacticalScore.alienAmmoDepotHealth = 0;
		gi.WriteByte (TE_BFG_BIGEXPLOSION); 
	}
	else
	{
		tacticalScore.humanAmmoDepot = false;
		tacticalScore.humanAmmoDepotHealth = 0;
		gi.WriteByte (TE_ROCKET_EXPLOSION);
	}
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PHS);

	if(self->classname == "alien ammodepot")
	{
		for (i=0 ; i<g_maxclients->value ; i++)
		{
			cl_ent = g_edicts + 1 + i;
			if (!cl_ent->inuse || cl_ent->is_bot)
				continue;
			safe_centerprintf(cl_ent, "Alien Ammo Depot has been destroyed!");
		}
	}
	else
	{
		for (i=0 ; i<g_maxclients->value ; i++)
		{
			cl_ent = g_edicts + 1 + i;
			if (!cl_ent->inuse || cl_ent->is_bot)
				continue;
			safe_centerprintf(cl_ent, "Human Ammo Depot has been destroyed!");
		}
	}
	
	gi.sound( &g_edicts[1], CHAN_AUTO, gi.soundindex( "world/explosion1.wav" ), 1, ATTN_NONE, 0 );

	if(!attacker->is_bot) 
	{
		// Send Steam stats
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte(TE_BASEKILL);
		gi.unicast (attacker, false);
	}

	G_FreeEdict (self);
}

void SP_misc_alienammodepot (edict_t *ent)
{
	if (!g_tactical->integer)
	{
		G_FreeEdict (ent);
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->takedamage = DAMAGE_YES;

	ent->s.modelindex = gi.modelindex ("maps/meshes/flagpad.md2");

	VectorSet (ent->mins, -32, -32, 0);
	VectorSet (ent->maxs, 32, 32, 16);
	ent->health = 1500; 
	ent->die = ammodepot_die;
	ent->think = ammodepot_think;
	ent->nextthink = level.time + TENFPS;
	ent->classname = "alien ammodepot";
	ent->ctype = 0;

	gi.linkentity (ent);
	M_droptofloor (ent);
}

void SP_misc_humanammodepot (edict_t *ent)
{
	if (!g_tactical->integer)
	{
		G_FreeEdict (ent);
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->takedamage = DAMAGE_YES;

	ent->s.modelindex = gi.modelindex ("maps/meshes/flagpad.md2");

	VectorSet (ent->mins, -32, -32, 0);
	VectorSet (ent->maxs, 32, 32, 16);
	ent->health = 1500; 
	ent->die = ammodepot_die;
	ent->think = ammodepot_think;
	ent->nextthink = level.time + TENFPS;
	ent->classname = "human ammodepot";
	ent->ctype = 1;

	gi.linkentity (ent);
	M_droptofloor (ent);
}

//Backup generators
void backupgen_think (edict_t *ent)
{
	if(ent->classname == "alien backupgen")
	{
		if(!tacticalScore.alienPowerSource)
		{
			//animate
			ent->s.frame = (ent->s.frame + 1) % 24;
			gi.sound (ent, CHAN_AUTO, gi.soundindex("misc/reject.wav"), 1, ATTN_STATIC, 0);
		}
	}
	else
	{
		if(!tacticalScore.humanPowerSource)
		{
			ent->s.frame = (ent->s.frame + 1) % 24;
			gi.sound (ent, CHAN_AUTO, gi.soundindex("misc/reject.wav"), 1, ATTN_STATIC, 0);
		}
	}	
	ent->nextthink = level.time + TENFPS;
}

void backupgen_die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	edict_t *cl_ent;
	int i;

	self->takedamage = DAMAGE_NO;
	self->activator = attacker;

	gi.WriteByte (svc_temp_entity);
	if(self->classname == "alien backupgen")
	{
		tacticalScore.alienBackupGen = false;
		gi.WriteByte (TE_BFG_BIGEXPLOSION); 
	}
	else
	{
		tacticalScore.humanBackupGen = false;
		gi.WriteByte (TE_ROCKET_EXPLOSION);		
	}
	gi.WritePosition (self->s.origin);
	gi.multicast (self->s.origin, MULTICAST_PHS);

	if(self->classname == "alien backupgen")
	{
		for (i=0 ; i<g_maxclients->value ; i++)
		{
			cl_ent = g_edicts + 1 + i;
			if (!cl_ent->inuse || cl_ent->is_bot)
				continue;
			safe_centerprintf(cl_ent, "Alien Backup Generator has been destroyed!");
		}
	}
	else
	{
		for (i=0 ; i<g_maxclients->value ; i++)
		{
			cl_ent = g_edicts + 1 + i;
			if (!cl_ent->inuse || cl_ent->is_bot)
				continue;
			safe_centerprintf(cl_ent, "Human Backup Generator has been destroyed!");
		}
	}
	
	gi.sound( &g_edicts[1], CHAN_AUTO, gi.soundindex( "world/explosion1.wav" ), 1, ATTN_NONE, 0 );

	if(!attacker->is_bot) 
	{
		// Send Steam stats
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte(TE_BASEKILL);
		gi.unicast (attacker, false);
	}

	G_FreeEdict (self);
}

void SP_misc_alienbackupgen (edict_t *ent)
{
	if (!g_tactical->integer)
	{
		G_FreeEdict (ent);
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->takedamage = DAMAGE_YES;

	ent->s.modelindex = gi.modelindex ("models/tactical/alien_backupgen.iqm");

	VectorSet (ent->mins, -24, -24, 0);
	VectorSet (ent->maxs, 24, 24, 48);
	ent->health = 300; 
	ent->die = backupgen_die;
	ent->think = backupgen_think;
	ent->nextthink = level.time + TENFPS;
	ent->classname = "alien backupgen";
	ent->ctype = 0;

	gi.linkentity (ent);
	M_droptofloor (ent);
}

void SP_misc_humanbackupgen (edict_t *ent)
{
	if (!g_tactical->integer)
	{
		G_FreeEdict (ent);
		return;
	}

	ent->movetype = MOVETYPE_NONE;
	ent->solid = SOLID_BBOX;
	ent->takedamage = DAMAGE_YES;

	ent->s.modelindex = gi.modelindex ("models/tactical/human_backupgen.iqm");

	VectorSet (ent->mins, -24, -24, 0);
	VectorSet (ent->maxs, 24, 24, 48);
	ent->health = 300; 
	ent->die = backupgen_die;
	ent->think = backupgen_think;
	ent->nextthink = level.time + TENFPS;
	ent->classname = "human backupgen";
	ent->ctype = 1;

	gi.linkentity (ent);
	M_droptofloor (ent);
}

void Check_tactical_items(edict_t *self)
{
	int i;
	edict_t *ent;

	for (i=1, ent=g_edicts+i ; i < globals.num_edicts ; i++,ent++)
	{
		vec3_t dist;
	
		if (!ent->inuse)
			continue;
		if(ent->client) //not players
			continue;
		
		if(!(infront(self, ent)))
			continue;

		VectorSubtract(self->s.origin, ent->s.origin, dist);

		if(VectorLength(dist) > 256)
			continue;

		if(self->ctype == 0)
		{
			if (self->client->pers.weapon == FindItem("Alien Bomb")) 
				safe_centerprintf(self, "Press <fire> to plant a bomb!");

			if(ent->classname == "human powersrc")
			{
				if(self->has_bomb)
					safe_centerprintf(self, "Plant a bomb here or fire weapon on power source.");
				else
					safe_centerprintf(self, "Fire weapon on power source to destroy it!");
			}
			else if(ent->classname == "human ammodepot")
			{
				if(self->has_bomb)
					safe_centerprintf(self, "Plant a bomb here or fire weapon on ammo depot.");
				else
					safe_centerprintf(self, "Fire weapon on ammo depot to destroy it!");
			}
			else if(ent->classname == "human computer")
			{
				if(self->has_bomb)
					safe_centerprintf(self, "Plant a bomb here or fire weapon on computer.");
				else
					safe_centerprintf(self, "Fire weapon on computer to destroy it!");
			}
			else if(ent->classname == "human backupgen")
			{
				if(self->has_bomb)
					safe_centerprintf(self, "Plant a bomb here or fire weapon on backup generator.");
				else
					safe_centerprintf(self, "Fire weapon on backup generator to destroy it!");
			}
			else if(ent->classname == "abomb")
			{
				if(self->has_detonator)
					safe_centerprintf(self, "Touch this bomb to activate it!");
			}
		}
		else if(self->ctype == 1)
		{
			if (self->client->pers.weapon == FindItem("Human Bomb")) 
				safe_centerprintf(self, "Press <fire> to plant a bomb!");

			if(ent->classname == "alien powersrc")
			{
				if(self->has_bomb)
					safe_centerprintf(self, "Plant a bomb here or fire weapon on power source.");
				else
					safe_centerprintf(self, "Fire weapon on power source to destroy it!");
			}
			else if(ent->classname == "alien ammodepot")
			{
				if(self->has_bomb)
					safe_centerprintf(self, "Plant a bomb here or fire weapon on ammo depot.");
				else
					safe_centerprintf(self, "Fire weapon on ammo depot to destroy it!");
			}
			else if(ent->classname == "alien computer")
			{
				if(self->has_bomb)
					safe_centerprintf(self, "Plant a bomb here or fire weapon on computer.");
				else
					safe_centerprintf(self, "Fire weapon on computer to destroy it!");
			}
			else if(ent->classname == "alien backupgen")
			{
				if(self->has_bomb)
					safe_centerprintf(self, "Plant a bomb here or fire weapon on backup generator.");
				else
					safe_centerprintf(self, "Fire weapon on backup generator to destroy it!");
			}
			else if(ent->classname == "hbomb")
			{
				if(self->has_detonator)
					safe_centerprintf(self, "Touch this bomb to activate it!");
			}
		}
	}
}

// Provide tips for players to learn the game, etc
void Tactical_tutorial_think(edict_t *ent)
{
	if(ent->is_bot)
		return;

	if(!ent->inuse)
		return;

	if(!ent->client)
		return;

	// Dont' spam 
	if(level.time - 2.0 > ent->lastTmsg)
	{
		Check_tactical_items(ent);
		ent->lastTmsg = level.time;
	}

}