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
// g_combat.c

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "g_local.h"

/*
============
CanDamage

Returns true if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
qboolean CanDamage (edict_t *targ, edict_t *inflictor)
{
	vec3_t	dest;
	trace_t	trace;

// bmodels need special checking because their origin is 0,0,0
	if (targ->movetype == MOVETYPE_PUSH)
	{
		VectorAdd (targ->absmin, targ->absmax, dest);
		VectorScale (dest, 0.5, dest);
		trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
		if (trace.fraction == 1.0)
			return true;
		if (trace.ent == targ)
			return true;
		return false;
	}

	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, targ->s.origin, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] += 15.0;
	dest[1] += 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] += 15.0;
	dest[1] -= 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] -= 15.0;
	dest[1] += 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;

	VectorCopy (targ->s.origin, dest);
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	trace = gi.trace (inflictor->s.origin, vec3_origin, vec3_origin, dest, inflictor, MASK_SOLID);
	if (trace.fraction == 1.0)
		return true;


	return false;
}


/*
============
Killed
============
*/
void Killed (edict_t *targ, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point)
{
	if (targ->health < -999)
		targ->health = -999;

	if (targ->monsterinfo.aiflags & AI_NPC) { //npc's never really die, they just return to their
											  //original spawn points
		//send an effect
		gi.WriteByte (svc_temp_entity);
		gi.WriteByte (TE_BFG_BIGEXPLOSION);
		gi.WritePosition (targ->s.origin);
		gi.multicast (targ->s.origin, MULTICAST_PHS);

		targ->health = targ->max_health;
		targ->s.event = EV_PLAYER_TELEPORT;
		targ->enemy = NULL;
		VectorCopy(targ->s.spawn_pos, targ->s.origin);
		return;
	}

	targ->enemy = attacker;

	if ((targ->svflags & SVF_MONSTER) && (targ->deadflag != DEAD_DEAD))
	{
//		targ->svflags |= SVF_DEADMONSTER;	// now treat as a different content type
		if (!(targ->monsterinfo.aiflags & AI_GOOD_GUY))
		{
			level.killed_monsters++;
			// medics won't heal monsters that they kill themselves
			if (strcmp(attacker->classname, "monster_medic") == 0)
				targ->owner = attacker;
		}
	}

	if (targ->movetype == MOVETYPE_PUSH || targ->movetype == MOVETYPE_STOP || targ->movetype == MOVETYPE_NONE)
	{	// doors, triggers, etc
		targ->die (targ, inflictor, attacker, damage, point);
		return;
	}

	if ((targ->svflags & SVF_MONSTER) && (targ->deadflag != DEAD_DEAD))
	{
		targ->touch = NULL;
		monster_death_use (targ);
	}

	targ->die (targ, inflictor, attacker, damage, point);
}


/*
================
SpawnDamage
================
*/
void SpawnDamage (int type, vec3_t origin, vec3_t normal, int damage)
{
	if (damage > 255)
		damage = 255;
	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (type);
//	gi.WriteByte (damage);
	gi.WritePosition (origin);
	gi.WriteDir (normal);
	gi.multicast (origin, MULTICAST_PVS);
}


/*
============
T_Damage

targ		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir			direction of the attack
point		point at which the damage is being inflicted
normal		normal vector from that point
damage		amount of damage being inflicted
knockback	force to be applied against targ as a result of the damage

dflags		these flags are used to control how T_Damage works
	DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR			armor does not protect from this damage
	DAMAGE_ENERGY			damage is from an energy based weapon
	DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
	DAMAGE_BULLET			damage is from a bullet (used for ricochets)
	DAMAGE_NO_PROTECTION	kills godmode, armor, everything
============
*/
static int CheckArmor (edict_t *ent, vec3_t point, vec3_t normal, int damage, int te_sparks, int dflags)
{
	gclient_t	*client;
	int			save;
	int			index;
	gitem_t		*armor;

	if (!damage)
		return 0;

	client = ent->client;

	if (!client)
		return 0;

	if (dflags & DAMAGE_NO_ARMOR)
		return 0;

	index = ArmorIndex (ent);
	if (!index)
		return 0;

	armor = GetItemByIndex (index);

	if (dflags & DAMAGE_ENERGY)
		save = ceil(((gitem_armor_t *)armor->info)->energy_protection*damage);
	else
		save = ceil(((gitem_armor_t *)armor->info)->normal_protection*damage);
	if (save >= client->pers.inventory[index])
		save = client->pers.inventory[index];

	if (!save)
		return 0;

	client->pers.inventory[index] -= save;
	SpawnDamage (te_sparks, point, normal, save);

	return save;
}

qboolean CheckTeamDamage (edict_t *targ, edict_t *attacker)
{
		//FIXME make the next line real and uncomment this block
		// if ((ability to damage a teammate == OFF) && (targ's team == attacker's team))
	return false;
}

void VerifyHeadShot( vec3_t point, vec3_t dir, float height, vec3_t newpoint)
{
        vec3_t normdir;
        vec3_t normdir2;


        VectorNormalize2(dir, normdir);
        VectorScale( normdir, height, normdir2 );
        VectorAdd( point, normdir2, newpoint );
}

#define HEAD_HEIGHT 12.0
#define CROUCHING_MAXS2 16

void T_Damage (edict_t *targ, edict_t *inflictor, edict_t *attacker, vec3_t dir, vec3_t point, vec3_t normal, int damage, int knockback, int dflags, int mod)
{
	gclient_t	*client;
	int			take;
	int			save;
	int			asave;
	int			te_sparks;
	int         head_success = 0;
    float       z_rel;
    int         height;
    float		from_top;
    float       targ_maxs2;

	if (!targ->takedamage)
		return;

	if(mod != MOD_TELEFRAG)
		if(targ->inuse && targ->client)
			if(targ->client->spawnprotected)
				return;

	//headshots for disruptors
	targ_maxs2 = targ->maxs[2];
    if (targ_maxs2 == 4)
		targ_maxs2 = CROUCHING_MAXS2;

    height = abs(targ->mins[2]) + targ_maxs2;

    if (targ->client && mod == MOD_DISRUPTOR)
	{
		z_rel = point[2] - targ->s.origin[2];
        from_top = targ_maxs2 - z_rel;

		if (from_top < 0.0)
			from_top = 0.0;

        if ( from_top < 2*HEAD_HEIGHT )
		{
			vec3_t new_point;
            VerifyHeadShot( point, dir, HEAD_HEIGHT, new_point );
            VectorSubtract( new_point, targ->s.origin, new_point );

            if ( (targ_maxs2 - new_point[2]) < HEAD_HEIGHT
				&& (abs(new_point[1])) < HEAD_HEIGHT*.8
                && (abs(new_point[0])) < HEAD_HEIGHT*.8 ) {
				head_success = 1;
            }
        }

        if ( head_success )
		{
			damage = damage*1.8 + 1;
            if (attacker->client)
				mod = MOD_HEADSHOT;
        }
    }

	// friendly fire avoidance
	// if enabled you can't hurt teammates (but you can hurt yourself)
	// knockback still occurs
	if ((targ != attacker) && ((deathmatch->value && (dmflags->integer & (DF_SKINTEAMS))) || ctf->value || g_tactical->value))
	{
		if (OnSameTeam (targ, attacker) && mod != MOD_TELEFRAG) //telefrag kills no matter what
		{
			if (dmflags->integer & DF_NO_FRIENDLY_FIRE)
				damage = 0;
			else
				mod |= MOD_FRIENDLY_FIRE;

			//educate the noobs....
			safe_centerprintf(attacker, "Stop shooting your teammates!!!");
		}
	}

	if (targ == attacker) {
		damage *= wep_selfdmgmulti->value;
	}

	meansOfDeath = mod;

	// easy mode takes half damage
	if (skill->value == 0 && deathmatch->value == 0 && targ->client)
	{
		damage *= 0.5;
		if (!damage)
			damage = 1;
	}

	client = targ->client;

	if (dflags & DAMAGE_BULLET)
		te_sparks = TE_BULLET_SPARKS;
	else
		te_sparks = TE_SPARKS;

	VectorNormalize(dir);

	if (targ->flags & FL_NO_KNOCKBACK)
		knockback = 0;

	// figure momentum add
	if (!(dflags & DAMAGE_NO_KNOCKBACK))
	{
		if ((knockback) && (targ->movetype != MOVETYPE_NONE) && (targ->movetype != MOVETYPE_BOUNCE) && (targ->movetype != MOVETYPE_PUSH) && (targ->movetype != MOVETYPE_STOP))
		{
			vec3_t	kvel;
			float	mass;

			if (targ->mass < 50)
				mass = 50;
			else
				mass = targ->mass;

			if (targ->client  && attacker == targ)
				VectorScale (dir, 1600.0 * (float)knockback / mass, kvel);	// the rocket jump hack...
			else
				VectorScale (dir, 500.0 * (float)knockback / mass, kvel);

			VectorAdd (targ->velocity, kvel, targ->velocity);
		}
	}
	//diminish plasma splash, as we want this to be minimal, as it's more used for tricks
	if(mod == MOD_PLASMA_SPLASH)
	{
		// damage /= (1+ (int)(random() * 10.0));
		damage /= 6; // median of formerly random 1..11
	}

	take = damage;
	save = 0;

	// check for godmode
	if ( (targ->flags & FL_GODMODE) && !(dflags & DAMAGE_NO_PROTECTION) )
	{
		take = 0;
		save = damage;
		SpawnDamage (te_sparks, point, normal, save);
	}

	// check for invincibility
	if ((client && client->alienforce_expiretime > level.time) && !(dflags & DAMAGE_NO_PROTECTION))
	{
		if (targ->pain_debounce_time < level.time)
		{
			gi.sound(targ, CHAN_ITEM, gi.soundindex("items/protect4.wav"), 1, ATTN_NORM, 0);
			targ->pain_debounce_time = level.time + 2;
		}
		if(mod == MOD_TRIGGER_HURT)
		{
			take = 0;
			save = damage;
		}
		else
		{
			take = damage/3;
			save = 0;
		}
	}

	asave = CheckArmor (targ, point, normal, take, te_sparks, dflags);
	take -= asave;

	//treat cheat/powerup savings the same as armor
	asave += save;

	// team damage avoidance
	if (!(dflags & DAMAGE_NO_PROTECTION) && CheckTeamDamage (targ, attacker))
		return;

// do the damage
	if (take)
	{
		if (client)
		{
			if (targ->ctype == 0) //alien, robot, human
				SpawnDamage (TE_GREENBLOOD, point, normal, take);
			else if (targ->ctype == 2)
				SpawnDamage (TE_GUNSHOT, point, normal, take);
			else
				SpawnDamage (TE_BLOOD, point, normal, take);
		}
		else
		{
			if (targ->ctype == 0) //alien, robot, human
				SpawnDamage (TE_GREENBLOOD, point, normal, take);
			else if (targ->ctype == 2)
				SpawnDamage (TE_GUNSHOT, point, normal, take);
			else
				SpawnDamage (TE_BLOOD, point, normal, take);
			
			if(g_tactical->value)
			{
				if(!strcmp(targ->classname, "alien computer"))
					safe_centerprintf(attacker, "Alien Computer health at %i percent", 100*(targ->health-take)/1500);
				else if(!strcmp(targ->classname, "human computer"))
					safe_centerprintf(attacker, "Human Computer health at %i percent", 100*(targ->health-take)/1500);
				else if(!strcmp(targ->classname, "alien powersrc"))
					safe_centerprintf(attacker, "Alien Power Source health at %i percent", 100*(targ->health-take)/1500);
				else if(!strcmp(targ->classname, "human powersrc"))
					safe_centerprintf(attacker, "Human Power Source health at %i percent", 100*(targ->health-take)/1500);
				else if(!strcmp(targ->classname, "alien ammodepot"))
					safe_centerprintf(attacker, "Alien Ammo Depot health at %i percent", 100*(targ->health-take)/1500);
				else if(!strcmp(targ->classname, "human ammodepot"))
					safe_centerprintf(attacker, "Human Ammo Depot health at %i percent", 100*(targ->health-take)/1500);
			}
		}

		targ->health = targ->health - take;

		if (targ->health <= 0)
		{
			if (client)
				targ->flags |= FL_NO_KNOCKBACK;
			Killed (targ, inflictor, attacker, take, point);
			return;
		}
	}

	if (client)
	{
		if (!(targ->flags & FL_GODMODE) && (take) && (targ->pain))
			targ->pain (targ, attacker, knockback, take);
	}
	else if (take)
	{
		if (targ->pain)
			targ->pain (targ, attacker, knockback, take);
	}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if (client)
	{
		client->damage_armor += asave;
		client->damage_blood += take;
		client->damage_knockback += knockback;
		VectorCopy (point, client->damage_from);
	}
}


/*
============
T_RadiusDamage
============
*/
void T_RadiusDamage (edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, int mod, int weapon)
{
	float	points;
	edict_t	*ent = NULL;
	vec3_t	v;
	vec3_t	dir;

	while ((ent = findradius(ent, inflictor->s.origin, radius)) != NULL)
	{
		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;

		VectorAdd (ent->mins, ent->maxs, v);
		VectorMA (ent->s.origin, 0.5, v, v);
		VectorSubtract (inflictor->s.origin, v, v);
		points = damage - 0.5 * VectorLength (v);
		if (ent == attacker)
			points = points * 0.5;
		if (points > 0)
		{
			if (CanDamage (ent, inflictor))
			{
				VectorSubtract (ent->s.origin, inflictor->s.origin, dir);
				T_Damage (ent, inflictor, attacker, dir, inflictor->s.origin, vec3_origin, (int)points, (int)points, DAMAGE_RADIUS, mod);
				if (ent != attacker)
					gi.sound (attacker, CHAN_VOICE, gi.soundindex("misc/hit.wav"), 1, ATTN_STATIC, 0);
				if(weapon >=0)
					attacker->client->resp.weapon_hits[weapon]++;
			}
		}
	}
}
