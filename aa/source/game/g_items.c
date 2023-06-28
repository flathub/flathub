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

qboolean	Pickup_Weapon (edict_t *ent, edict_t *other);
void		Use_Weapon (edict_t *ent, gitem_t *inv);
void		Drop_Weapon (edict_t *ent, gitem_t *inv);

#ifdef ALTERIA
void Weapon_Punch ( edict_t *ent);
#else
void Weapon_Blaster (edict_t *ent);
void Weapon_AlienBlaster (edict_t *ent);
void Weapon_Violator (edict_t *ent);
void Weapon_Smartgun (edict_t *ent);
void Weapon_Chain (edict_t *ent);
void Weapon_Flame (edict_t *ent);
void Weapon_Disruptor (edict_t *ent);
void Weapon_RocketLauncher (edict_t *ent);
void Weapon_Beamgun (edict_t *ent);
void Weapon_Vaporizer (edict_t *ent);
void Weapon_Minderaser (edict_t *ent);
void Weapon_TacticalBomb ( edict_t *ent);
#endif

gitem_armor_t jacketarmor_info	= { 25,  50, .30, .00, ARMOR_JACKET};
gitem_armor_t combatarmor_info	= { 50, 100, .60, .30, ARMOR_COMBAT};
gitem_armor_t bodyarmor_info	= {100, 200, .80, .60, ARMOR_BODY};

static int	jacket_armor_index;
static int	combat_armor_index;
static int	body_armor_index;

#define HEALTH_IGNORE_MAX	1
#define HEALTH_TIMED		2

static void Use_Doubledamage (edict_t *ent, gitem_t *item);
static float doubledamage_drop_timeout_hack;

//======================================================================

/*
===============
GetItemByIndex
===============
*/
gitem_t	*GetItemByIndex (int index)
{
	if (index == 0 || index >= game.num_items)
		return NULL;

	return &itemlist[index];
}


/*
===============
FindItemByClassname

===============
*/
gitem_t	*FindItemByClassname (const char *classname)
{
	int		i;
	gitem_t	*it;

	it = itemlist;
	for (i=0 ; i<game.num_items ; i++, it++)
	{
		if (!it->classname)
			continue;
		if (!Q_strcasecmp(it->classname, classname))
			return it;
	}

	return NULL;
}

/*
===============
FindItem

===============
*/
gitem_t	*FindItem (const char *pickup_name)
{
	int		i;
	gitem_t	*it;

	it = itemlist;
	for (i=0 ; i<game.num_items ; i++, it++)
	{
		if (!it->pickup_name)
			continue;
		if (!Q_strcasecmp(it->pickup_name, pickup_name))
			return it;
	}

	return NULL;
}

//======================================================================

static void drop_temp_touch (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	if (other == ent->owner)
		return;

	Touch_Item (ent, other, plane, surf);
}

static void drop_make_touchable (edict_t *ent)
{
	ent->touch = Touch_Item;
	if (deathmatch->integer)
	{
		if(!g_tactical->integer) //in tactical mode, we don't remove dropped items
		{
			ent->nextthink = level.time + 29;
			ent->think = G_FreeEdict;
		}
	}
}

float mindEraserTime;
void SpawnMinderaser(edict_t *ent)
{
	edict_t *minderaser, *cl_ent;
	int i;

	for (i = 0; i < g_maxclients->integer; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || cl_ent->is_bot)
			continue;
		safe_centerprintf(cl_ent, "A Mind Eraser has spawned!\n");
	}

	//to do - play level wide klaxxon 
	gi.sound( &g_edicts[1], CHAN_AUTO, gi.soundindex( "misc/minderaser.wav" ), 1, ATTN_NONE, 0 );

	minderaser = G_Spawn();
	VectorCopy(ent->s.origin, minderaser->s.origin);
	minderaser->spawnflags = DROPPED_PLAYER_ITEM;
	minderaser->model = "models/weapons/g_minderaser/tris.iqm";
	minderaser->classname = "weapon_minderaser";
	minderaser->item = FindItem ("Minderaser");
	minderaser->s.effects = minderaser->item->world_model_flags;
	minderaser->s.renderfx = RF_GLOW;
	VectorSet (minderaser->mins, -15, -15, -15);
	VectorSet (minderaser->maxs, 15, 15, 15);
	gi.setmodel (minderaser, minderaser->item->world_model);
	minderaser->solid = SOLID_TRIGGER;
	minderaser->health = 100;
	minderaser->movetype = MOVETYPE_TOSS;
	minderaser->touch = drop_temp_touch;
	minderaser->owner = NULL;

	SetRespawn (ent, 1000000); //huge delay until ME is picked up from pad.			
	minderaser->replaced_weapon = ent; //remember this entity

	mindEraserTime = level.time;
}

void DoRespawn (edict_t *ent)
{
	char szTmp[64];
	//Add mind eraser spawn here, if it's been two minutes since last respawn of it,
	//go ahead and set the next weapon spawned to be the mind eraser
	//need to check if first part of name is weapon
	if(level.time > mindEraserTime + 120.0)
	{
		strcpy(szTmp, ent->classname);
		szTmp[6] = 0;
		if(!Q_strcasecmp(szTmp, "weapon"))
		{
			SpawnMinderaser(ent);
			return;
		}
	}

	//All Out Assault - replace weapons with jetpacks every minute, up to 3 max
	if(all_out_assault->integer)
	{
		if(level.time > jetpackTime + 30.0)
		{
			int i;
			int numJetPacks = 0;
			edict_t *gEnt;

			//make sure there are no more than 3 jetpacks around, as it can cause too many weapons to be "missing" at any given time in smaller matches.
			for (i=1, gEnt=g_edicts+i ; i < globals.num_edicts ; i++,gEnt++)
			{
				if (!gEnt->inuse)
					continue;
				if(gEnt->client) //not players
					continue;

				if(!Q_strcasecmp(gEnt->classname, "item_jetpack") && !(gEnt->spawnflags & DROPPED_ITEM))
					numJetPacks++;
			}
			if(numJetPacks < 3)
			{
				strcpy(szTmp, ent->classname);
				szTmp[6] = 0;
				if(!Q_strcasecmp(szTmp, "weapon"))
				{
					SpawnJetpack(ent);
					return;
				}
			}
		}
	}

	if (ent->team)
	{
		edict_t	*master;
		int	count;
		int choice;

		master = ent->teammaster;

		for (count = 0, ent = master; ent; ent = ent->chain, count++)
			;

		choice = rand() % count;

		for (count = 0, ent = master; count < choice; ent = ent->chain, count++)
			;
	}

	ent->svflags &= ~SVF_NOCLIENT;
	ent->solid = SOLID_TRIGGER;
	gi.linkentity (ent);

	// send an effect
	ent->s.event = EV_ITEM_RESPAWN;
}

void SetRespawn (edict_t *ent, float delay)
{

    if (	ent->item && g_duel->integer && 
    		ent->item->classnum != weapon_minderaser		)
	{
		switch (ent->item->flags)
		{
			//TODO: playtest this and adjust the scaling factors.
			case IT_WEAPON:
				if(all_out_assault->integer)
					delay *= 1.5;
				else
					delay *= 3.0;
				break;
			case IT_POWERUP:
			case IT_AMMO: //intentional fallthrough
				delay *= 2.0;
				break;
			case IT_ARMOR:
			case IT_HEALTH: //intentional fallthrough
				delay *= 1.5;
				break;
			default:
				break;
		}
	}
	ent->flags |= FL_RESPAWN;
	ent->svflags |= SVF_NOCLIENT;
	ent->solid = SOLID_NOT;
	ent->nextthink = level.time + delay;
	ent->think = DoRespawn;
	gi.linkentity (ent);
}


//======================================================================

qboolean Pickup_Powerup (edict_t *ent, edict_t *other)
{
	int		quantity;

	quantity = other->client->pers.inventory[ITEM_INDEX(ent->item)];
	if ((skill->value == 1 && quantity >= 2) || (skill->value >= 2 && quantity >= 1))
		return false;

	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;

	if (deathmatch->integer)
	{
		int randomSpawn;
		//Phenax - Add random time to quad spawn
		if (ent->item->use == Use_Doubledamage && g_randomquad->integer)
			randomSpawn = 10 + rand() % (30 - 10); //10 to 30 seconds randomness
		else
			randomSpawn = 0;

		if (!(ent->spawnflags & DROPPED_ITEM) )
			SetRespawn (ent, ent->item->quantity + randomSpawn);
		if ((dmflags->integer & DF_INSTANT_ITEMS) || (ent->item->use == Use_Doubledamage && (ent->spawnflags & DROPPED_PLAYER_ITEM)))
		{
			if ((ent->item->use == Use_Doubledamage) && (ent->spawnflags & DROPPED_PLAYER_ITEM))
				doubledamage_drop_timeout_hack = ent->nextthink - level.time;
			ent->item->use (other, ent->item);
		}
	}

	return true;
}

void Drop_General (edict_t *ent, gitem_t *item)
{
	Drop_Item (ent, item);
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);
}


//======================================================================

qboolean Pickup_Adrenaline (edict_t *ent, edict_t *other)
{
	if (!deathmatch->integer)
		other->max_health += 1;

	if (other->health < other->max_health)
		other->health = other->max_health;

	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->integer))
		SetRespawn (ent, ent->item->quantity);

	return true;
}

//======================================================================

static void Use_Doubledamage (edict_t *ent, gitem_t *item)
{
	float timeout;

	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	if (doubledamage_drop_timeout_hack != 0.0f)
	{
		timeout = doubledamage_drop_timeout_hack;
		doubledamage_drop_timeout_hack = 0.0f;
	}
	else
	{
		timeout = 30.0f;
	}

	if (ent->client->doubledamage_expiretime > level.time)
		ent->client->doubledamage_expiretime += timeout;
	else
		ent->client->doubledamage_expiretime = level.time + timeout;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

static void Use_Alienforce (edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	if (ent->client->alienforce_expiretime > level.time)
		ent->client->alienforce_expiretime += 30.0f;
	else
		ent->client->alienforce_expiretime = level.time + 30.0f;

	//add full armor
	ent->client->pers.inventory[combat_armor_index] = 200;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/protect.wav"), 1, ATTN_NORM, 0);
}

void Use_Haste (edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	if (ent->client->haste_expiretime > level.time)
		ent->client->haste_expiretime += 30.0f;
	else
		ent->client->haste_expiretime = level.time + 30.0f;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/powerup.wav"), 1, ATTN_NORM, 0);
}
//======================================================================

void Use_Sproing (edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	if (ent->client->sproing_expiretime > level.time)
		ent->client->sproing_expiretime += 30.0f;
	else
		ent->client->sproing_expiretime = level.time + 30.0f;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/powerup.wav"), 1, ATTN_NORM, 0);
}

void Use_Invisibility (edict_t *ent, gitem_t *item)
{
	ent->client->pers.inventory[ITEM_INDEX(item)]--;
	ValidateSelectedItem (ent);

	if (ent->client->invis_expiretime > level.time)
		ent->client->invis_expiretime += 30.0f;
	else
		ent->client->invis_expiretime = level.time + 30.0f;

	gi.sound(ent, CHAN_ITEM, gi.soundindex("items/powerup.wav"), 1, ATTN_NORM, 0);
}

//======================================================================

qboolean Pickup_Key (edict_t *ent, edict_t *other)
{
	other->client->pers.inventory[ITEM_INDEX(ent->item)]++;
	return true;
}

//======================================================================

qboolean Add_Ammo (edict_t *ent, gitem_t *item, int count, qboolean weapon, qboolean dropped)
{
	int			index;
	int			max, base;
	gitem_t		*failedswitch;

	if (!ent->client)
		return false;

#define X(name,itname,defbase,defmax,excessivemult)	\
	if (item->classnum == ammo_##name) \
	{ \
		max = g_max##name->integer; \
		if (excessive->value) \
			max *= excessivemult; \
		base = defbase; \
	} \
	else // leave a trailing else for the return false

	AMMO_TYPES

	#undef X
	
		return false;

	index = ITEM_INDEX(item);

	if (ent->client->pers.inventory[index] == max)
		return false;

	if (weapon && !dropped && (ent->client->pers.inventory[index] > 0))
		count = 1; //already has weapon -- not dropped. Give him 1 ammo.

	//if less than base ammo, restock ammo fully
	if(ent->client->pers.inventory[index] < base) //less than base ammount
		ent->client->pers.inventory[index] = base;
	else
		ent->client->pers.inventory[index] += count;

	if (ent->client->pers.inventory[index] > max)
		ent->client->pers.inventory[index] = max;
	
	failedswitch = ent->client->pers.lastfailedswitch;
	if (failedswitch && failedswitch->ammo && 
		(FindItem(failedswitch->ammo) == item) && 
		(level.framenum - ent->client->pers.failedswitch_framenum) < 5)
		ent->client->newweapon = failedswitch;

	return true;
}

qboolean Pickup_Ammo (edict_t *ent, edict_t *other)
{
	int			oldcount;
	int			count;
	qboolean		weapon;

	weapon = (ent->item->flags & IT_WEAPON);
	if ( weapon && ( dmflags->integer & DF_INFINITE_AMMO ) )
		count = 1000;
	else if (ent->count)
		count = ent->count;
	else
		count = ent->item->quantity;

	oldcount = other->client->pers.inventory[ITEM_INDEX(ent->item)];

	if (!Add_Ammo (other, ent->item, count, false, true)) //Not 'dropped' but give full ammo even if ammo > 0
		return false;

	if (weapon && !oldcount)
	{
		if (other->client->pers.weapon != ent->item && ( !deathmatch->integer || other->client->pers.weapon == FindItem("Blaster") || other->client->pers.weapon == FindItem("Alien Blaster")) )
			other->client->newweapon = ent->item;
	}

	if (!(ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)) && (deathmatch->integer))
	{
		if(g_tactical->integer)
		{
			if(!strcmp(ent->classname, "ammo_cells") || !strcmp(ent->classname, "ammo_shells"))
			{
				if(!tacticalScore.alienPowerSource)
				{
					if(tacticalScore.alienBackupGen)
						SetRespawn (ent, 20); //on backup power, generate ammo much slower
					else
						SetRespawn (ent, 40); //for the most part, dead
				}
				else
					SetRespawn (ent, 5);
			}
			else if(!strcmp(ent->classname, "ammo_rockets") || !strcmp(ent->classname, "ammo_bullets") || !strcmp(ent->classname, "ammo_grenades"))
			{
				if(!tacticalScore.humanPowerSource)
				{
					if(tacticalScore.humanBackupGen)
						SetRespawn (ent, 20);
					else
						SetRespawn (ent, 40);
				}
				else
					SetRespawn (ent, 5);
			}
		}
		else
			SetRespawn (ent, 30);
	}

	return true;
}

void Drop_Ammo (edict_t *ent, gitem_t *item)
{
	edict_t	*dropped;
	int		index;

	index = ITEM_INDEX(item);
	dropped = Drop_Item (ent, item);
	if (ent->client->pers.inventory[index] >= item->quantity)
		dropped->count = item->quantity;
	else
		dropped->count = ent->client->pers.inventory[index];

	if (ent->client->pers.weapon &&
		ent->client->pers.weapon->classnum == ammo_grenades &&
		item->classnum == ammo_grenades &&
		ent->client->pers.inventory[index] - dropped->count <= 0) {
		safe_cprintf (ent, PRINT_HIGH, "Can't drop current weapon\n");
		G_FreeEdict(dropped);
		return;
	}

	ent->client->pers.inventory[index] -= dropped->count;
	ValidateSelectedItem (ent);
}


//======================================================================

void MegaHealth_think (edict_t *self)
{
	if (self->owner->health > self->owner->max_health)
	{
		self->nextthink = level.time + 1;
		self->owner->health -= 1;
		return;
	}

	if (!(self->spawnflags & DROPPED_ITEM) && (deathmatch->integer))
		SetRespawn (self, 20);
	else
		G_FreeEdict (self);
}
void Healthbox_think (edict_t *self)
{

	self->nextthink = level.time + 7;
	self->s.effects = EF_ROTATE;
	self->s.renderfx = RF_GLOW;
	return;
}
qboolean Pickup_Health (edict_t *ent, edict_t *other)
{
	if (!(ent->style & HEALTH_IGNORE_MAX))
		if (other->health >= other->max_health)
		{
			ent->s.effects |= EF_ROTATE;
			ent->think = Healthbox_think;
			ent->nextthink = level.time + 7;
			return false;
		}

	other->health += ent->count;

	if (!(ent->style & HEALTH_IGNORE_MAX))
	{
		if (other->health > other->max_health)
			other->health = other->max_health;
	}

	if (ent->style & HEALTH_TIMED)
	{
		ent->think = MegaHealth_think;
		ent->nextthink = level.time + 5;
		ent->owner = other;
		ent->flags |= FL_RESPAWN;
		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
	}
	else
	{
		if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->integer))
			SetRespawn (ent, 30);
	}

	return true;
}

//======================================================================

int ArmorIndex (edict_t *ent)
{
	if (!ent->client)
		return 0;

	if (ent->client->pers.inventory[jacket_armor_index] > 0)
		return jacket_armor_index;

	if (ent->client->pers.inventory[combat_armor_index] > 0)
		return combat_armor_index;

	if (ent->client->pers.inventory[body_armor_index] > 0)
		return body_armor_index;

	return 0;
}

qboolean Pickup_Armor (edict_t *ent, edict_t *other)
{
	int				old_armor_index;
	gitem_armor_t	*oldinfo;
	gitem_armor_t	*newinfo;
	int				newcount;
	float			salvage;
	int				salvagecount;

	// get info on new armor
	newinfo = (gitem_armor_t *)ent->item->info;

	old_armor_index = ArmorIndex (other);

	// handle armor shards specially
	if (ent->item->tag == ARMOR_SHARD)
	{
		if (!old_armor_index)
			other->client->pers.inventory[jacket_armor_index] = 5;
		else
			other->client->pers.inventory[old_armor_index] += 5;
	}

	// if player has no armor, just use it
	else if (!old_armor_index)
	{
		other->client->pers.inventory[ITEM_INDEX(ent->item)] = newinfo->base_count;
	}

	// use the better armor
	else
	{
		// get info on old armor
		if (old_armor_index == jacket_armor_index)
			oldinfo = &jacketarmor_info;
		else if (old_armor_index == combat_armor_index)
			oldinfo = &combatarmor_info;
		else // (old_armor_index == body_armor_index)
			oldinfo = &bodyarmor_info;

		if (newinfo->normal_protection > oldinfo->normal_protection)
		{
			// calc new armor values
			salvage = oldinfo->normal_protection / newinfo->normal_protection;
			salvagecount = salvage * other->client->pers.inventory[old_armor_index];
			newcount = newinfo->base_count + salvagecount;
			if (newcount > newinfo->max_count)
				newcount = newinfo->max_count;

			// zero count of old armor so it goes away
			other->client->pers.inventory[old_armor_index] = 0;

			// change armor to new item with computed value
			other->client->pers.inventory[ITEM_INDEX(ent->item)] = newcount;
		}
		else
		{
			// calc new armor values
			salvage = newinfo->normal_protection / oldinfo->normal_protection;
			salvagecount = salvage * newinfo->base_count;
			newcount = other->client->pers.inventory[old_armor_index] + salvagecount;
			if (newcount > oldinfo->max_count)
				newcount = oldinfo->max_count;

			// if we're already maxed out then we don't need the new armor
			if (other->client->pers.inventory[old_armor_index] >= newcount)
				return false;

			// update current armor value
			other->client->pers.inventory[old_armor_index] = newcount;
		}
	}

	if (!(ent->spawnflags & DROPPED_ITEM) && (deathmatch->integer))
		SetRespawn (ent, 20);
	
	return true;
}

//======================================================================

/*
===============
Touch_Item
===============
*/
void Touch_Item (edict_t *ent, edict_t *other, cplane_t *plane, csurface_t *surf)
{
	qboolean	taken;

	if (!other->client)
		return;
	if (other->health < 1)
		return;		// dead people can't pickup
	if (!ent->item->pickup)
		return;		// not a grabbable item?

	taken = ent->item->pickup(ent, other);

	if (taken)
	{
		// flash the screen
		other->client->bonus_alpha = 0.25;

		// show icon and name on status bar
		other->client->pickup_msg_time = level.time + 3.0;

		// change selected item
		if (ent->item->use)
			other->client->pers.selected_item = other->client->ps.stats[STAT_SELECTED_ITEM] = ITEM_INDEX(ent->item);

		if (ent->item->pickup == Pickup_Health)
		{
			if (ent->count == 5)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/s_health.wav"), 1, ATTN_NORM, 0);
			else if (ent->count == 10)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/n_health.wav"), 1, ATTN_NORM, 0);
			else if (ent->count == 25)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/l_health.wav"), 1, ATTN_NORM, 0);
			else // (ent->count == 100)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/m_health.wav"), 1, ATTN_NORM, 0);
		}
		else if (ent->item->pickup_sound)
		{
			gi.sound(other, CHAN_ITEM, gi.soundindex(ent->item->pickup_sound), 1, ATTN_NORM, 0);
		}

		//if AOA, make sure original weapon gets respawned
		if(all_out_assault->integer)
		{
			if (ent->item->classnum == item_jetpack)
			{
				if(ent->replaced_weapon != NULL)
					SetRespawn(ent->replaced_weapon, 5);
			}
		}
	}

	if (!(ent->spawnflags & ITEM_TARGETS_USED))
	{
		G_UseTargets (ent, other);
		ent->spawnflags |= ITEM_TARGETS_USED;
	}

	if (!taken)
		return;

	if(g_tactical->integer) //items do not respawn in tactical mode(except ammo when ammo depots are running)
	{
		if((!strcmp(ent->classname, "ammo_cells") || !strcmp(ent->classname, "ammo_shells")) && tacticalScore.alienAmmoDepot)
			return;
		else if((!strcmp(ent->classname, "ammo_rockets") || !strcmp(ent->classname, "ammo_bullets") || !strcmp(ent->classname, "ammo_grenades")) && tacticalScore.humanAmmoDepot)
			return;

		G_FreeEdict (ent); 
		return;
	}

	if (ent->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM))
	{
		if (ent->flags & FL_RESPAWN)
			ent->flags &= ~FL_RESPAWN;
		else
			G_FreeEdict (ent);
	}
}

//======================================================================

edict_t *Drop_Item (edict_t *ent, gitem_t *item)
{
	edict_t	*dropped;
	vec3_t	forward, right;
	vec3_t	offset;

	dropped = G_Spawn();

	dropped->classname = item->classname;
	dropped->item = item;
	dropped->spawnflags = DROPPED_ITEM;
	dropped->s.effects = item->world_model_flags;
	dropped->s.renderfx = RF_GLOW;
	VectorSet (dropped->mins, -15, -15, -15);
	VectorSet (dropped->maxs, 15, 15, 15);
	gi.setmodel (dropped, dropped->item->world_model);
	dropped->solid = SOLID_TRIGGER;
	dropped->movetype = MOVETYPE_TOSS;
	dropped->touch = drop_temp_touch;
	dropped->owner = ent;

	if (ent->client)
	{
		trace_t	trace;

		AngleVectors (ent->client->v_angle, forward, right, NULL);
		VectorSet(offset, 24, 0, -16);
		G_ProjectSource (ent->s.origin, offset, forward, right, dropped->s.origin);
		trace = gi.trace (ent->s.origin, dropped->mins, dropped->maxs,
			dropped->s.origin, ent, CONTENTS_SOLID);
		VectorCopy (trace.endpos, dropped->s.origin);
	}
	else
	{
		AngleVectors (ent->s.angles, forward, right, NULL);
		VectorCopy (ent->s.origin, dropped->s.origin);
	}

	VectorScale (forward, 100, dropped->velocity);
	dropped->velocity[2] = 300;

	dropped->think = drop_make_touchable;
	dropped->nextthink = level.time + 1;

	gi.linkentity (dropped);

	return dropped;
}

void Use_Item (edict_t *ent, edict_t *other, edict_t *activator)
{
	ent->svflags &= ~SVF_NOCLIENT;
	ent->use = NULL;

	if (ent->spawnflags & ITEM_NO_TOUCH)
	{
		ent->solid = SOLID_BBOX;
		ent->touch = NULL;
	}
	else
	{
		ent->solid = SOLID_TRIGGER;
		ent->touch = Touch_Item;
	}

	gi.linkentity (ent);
}

edict_t *Throw_Item (edict_t *ent, gitem_t *item)
{
	edict_t	*dropped;
	vec3_t	forward, right;
	vec3_t	offset;

	dropped = G_Spawn();

	dropped->classname = item->classname;
	dropped->item = item;
	dropped->spawnflags = DROPPED_ITEM;
	dropped->s.renderfx = RF_GLOW;
	dropped->s.effects = EF_ROTATE;
	VectorSet (dropped->mins, -15, -15, -15);
	VectorSet (dropped->maxs, 15, 15, 15);
	gi.setmodel (dropped, dropped->item->world_model);
	dropped->solid = SOLID_TRIGGER;
	dropped->movetype = MOVETYPE_TOSS;
	dropped->touch = drop_temp_touch;
	dropped->owner = ent;

	if (ent->client)
	{
		trace_t	trace;

		AngleVectors (ent->client->v_angle, forward, right, NULL);
		VectorSet(offset, 24, 0, -16);
		G_ProjectSource (ent->s.origin, offset, forward, right, dropped->s.origin);
		trace = gi.trace (ent->s.origin, dropped->mins, dropped->maxs,
			dropped->s.origin, ent, CONTENTS_SOLID);
		VectorCopy (trace.endpos, dropped->s.origin);
	}
	else
	{
		AngleVectors (ent->s.angles, forward, right, NULL);
		VectorCopy (ent->s.origin, dropped->s.origin);
	}

	dropped->velocity[0] = 200.0 * crandom();
	dropped->velocity[1] = 200.0 * crandom();
	dropped->velocity[2] = 300.0 + 200.0 * random();

	dropped->think = drop_make_touchable;
	dropped->nextthink = level.time + 1;

	gi.linkentity (dropped);

	return dropped;
}
//======================================================================

/*
================
droptofloor
================
*/
void droptofloor (edict_t *ent)
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
		gi.dprintf ("droptofloor: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
		G_FreeEdict (ent);
		return;
	}

	VectorCopy (tr.endpos, ent->s.origin);

	if (ent->team)
	{
		ent->flags &= ~FL_TEAMSLAVE;
		ent->chain = ent->teamchain;
		ent->teamchain = NULL;

		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;

		if (ent == ent->teammaster)
		{
			ent->nextthink = level.time + FRAMETIME;
			ent->think = DoRespawn;
		}
	}

	if (ent->spawnflags & ITEM_NO_TOUCH)
	{
		ent->solid = SOLID_BBOX;
		ent->touch = NULL;
		ent->s.effects &= ~EF_ROTATE;
		ent->s.renderfx &= ~RF_GLOW;
	}

	if (ent->spawnflags & ITEM_TRIGGER_SPAWN)
	{
		ent->svflags |= SVF_NOCLIENT;
		ent->solid = SOLID_NOT;
		ent->use = Use_Item;
	}

	gi.linkentity (ent);
}


/*
===============
PrecacheItem

Precaches all data needed for a given item.
This will be called for each item spawned in a level,
and for each item in each client's inventory.
===============
*/
void PrecacheItem (gitem_t *it)
{
	char	*s, *start;
	char	data[MAX_QPATH];
	int		len;
	gitem_t	*ammo;

	if (!it)
		return;

	if (it->pickup_sound)
		gi.soundindex (it->pickup_sound);
	if (it->world_model)
		gi.modelindex (it->world_model);
	if (it->view_model)
		gi.modelindex (it->view_model);
	if (it->icon)
		gi.imageindex (it->icon);

	// parse everything for its ammo
	if (it->ammo && it->ammo[0])
	{
		ammo = FindItem (it->ammo);
		if (ammo != it)
			PrecacheItem (ammo);
	}

	// parse the space seperated precache string for other items
	s = it->precaches;
	if (!s || !s[0])
		return;

	while (*s)
	{
		start = s;
		while (*s && *s != ' ')
			s++;

		len = s-start;
		if (len >= MAX_QPATH || len < 5)
			gi.error ("PrecacheItem: %s has bad precache string", it->classname);
		memcpy (data, start, len);
		data[len] = 0;
		if (*s)
			s++;

		// determine type based on extension
		if (!strcmp(data+len-3, "md2"))
			gi.modelindex (data);
		else if (!strcmp(data+len-3, "iqm"))
			gi.modelindex (data);
		else if (!strcmp(data+len-3, "wav"))
			gi.soundindex (data);
		if (!strcmp(data+len-3, "pcx"))
			gi.imageindex (data);
	}
}

/*
============
SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void SpawnItem (edict_t *ent, gitem_t *item)
{
	PrecacheItem (item);

	if (ent->spawnflags)
	{
		if (strcmp(ent->classname, "key_power_cube") != 0)
		{
			ent->spawnflags = 0;
			gi.dprintf("%s at %s has invalid spawnflags set\n", ent->classname, vtos(ent->s.origin));
		}
	}

	// some items will be prevented in deathmatch
	if (deathmatch->integer)
	{
		if ( dmflags->integer & DF_NO_ARMOR )
		{
			if (item->pickup == Pickup_Armor)
			{
				G_FreeEdict (ent);
				return;
			}
		}
		if ( dmflags->integer & DF_NO_ITEMS )
		{
			if (item->pickup == Pickup_Powerup)
			{
				G_FreeEdict (ent);
				return;
			}
		}
		if ( dmflags->integer & DF_NO_HEALTH )
		{
			if (item->pickup == Pickup_Health || item->pickup == Pickup_Adrenaline)
			{
				G_FreeEdict (ent);
				return;
			}
		}
		if ( dmflags->integer & DF_INFINITE_AMMO )
		{
			if ( (item->flags == IT_AMMO) || (strcmp(ent->classname, "weapon_vaporizer") == 0) )
			{
				G_FreeEdict (ent);
				return;
			}
		}
		if(excessive->integer || instagib->integer || rocket_arena->integer || insta_rockets->integer )
		{
			if (item->flags == IT_AMMO || (strcmp(ent->classname, "weapon_vaporizer") == 0) ||
				(strcmp(ent->classname, "weapon_disruptor") == 0) ||
				(strcmp(ent->classname, "weapon_beamgun") == 0) ||
				(strcmp(ent->classname, "weapon_rocketlauncher") == 0) ||
				(strcmp(ent->classname, "weapon_chaingun") == 0) ||
				(strcmp(ent->classname, "weapon_flamethrower") == 0) ||
				(strcmp(ent->classname, "weapon_smartgun") == 0))
			{
				G_FreeEdict (ent);
				return;
			}
		}
	}

	//CTF
	//Don't spawn the flags unless enabled
	if (!ctf->integer &&
		(strcmp(ent->classname, "item_flag_red") == 0 ||
		strcmp(ent->classname, "item_flag_blue") == 0)) {
		G_FreeEdict(ent);
		return;
	}

	ent->item = item;
	ent->nextthink = level.time + 2 * FRAMETIME;    // items start after other solids
	ent->think = droptofloor;
	if (strcmp(ent->classname, "item_flag_red") && //flags are special and don't get this
		strcmp(ent->classname, "item_flag_blue")) {
		ent->s.effects = EF_ROTATE;
	}
	ent->s.renderfx = RF_GLOW;
	if((strcmp(ent->classname, "Health") == 0)) {
		ent->s.modelindex2 = gi.modelindex("models/items/healing/globe/tris.iqm");
	}
	else if((strcmp(ent->classname, "item_quad") == 0)) {
		ent->s.modelindex2 = gi.modelindex("models/items/quaddama/unit.iqm");
	}
	else if((strcmp(ent->classname, "item_adrenaline") == 0)) {
		ent->s.modelindex2 = gi.modelindex("models/items/adrenaline/glass.iqm");
	}

	if (ent->model)
		gi.modelindex (ent->model);

	//flags are server animated and have special handling
	if (strcmp(ent->classname, "item_flag_red") == 0 ||
		strcmp(ent->classname, "item_flag_blue") == 0) {
		ent->think = CTFFlagSetup;
	}

	//vehicles have special handling
	if((strcmp(ent->classname, "item_jetpack") == 0))
		ent->think = VehicleSetup;

	//give ammo boxes and armor shards a pulsing shell(combined with RF_GLOW the renderer will know to phase it
	if(strcmp(ent->classname, "ammo_rockets") == 0 || strcmp(ent->classname, "ammo_cells") == 0 || strcmp(ent->classname, "ammo_shells") == 0 ||
		strcmp(ent->classname, "ammo_grenades") == 0 || strcmp(ent->classname, "ammo_bullets") == 0 || strcmp(ent->classname, "item_armor_shard") == 0)
	{
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= RF_SHELL_GREEN;
	}

	if(strcmp(ent->classname, "item_invulnerability") == 0)
	{
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= RF_SHELL_RED;
	}
}

//======================================================================

gitem_t	itemlist[] =
{
	{
		-1, NULL
	},	// leave index 0 alone

	//
	// ARMOR
	//

/*QUAKED item_armor_body (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		GITEM_INIT_IDENTIFY (item_armor_body, IT_ARMOR),
		GITEM_INIT_CALLBACKS (Pickup_Armor, NULL, NULL, NULL),
		GITEM_INIT_WORLDMODEL ("models/items/armor/body/tris.iqm", EF_ROTATE),
		GITEM_INIT_CLIENTSIDE ("i_bodyarmor", "Body Armor", "misc/ar1_pkup.wav"),
		GITEM_INIT_ARMOR (&bodyarmor_info, ARMOR_BODY),
/* precache */ ""
	},

/*QUAKED item_armor_combat (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		GITEM_INIT_IDENTIFY (item_armor_combat, IT_ARMOR),
		GITEM_INIT_CALLBACKS (Pickup_Armor, NULL, NULL, NULL),
		GITEM_INIT_WORLDMODEL ("models/items/armor/combat/tris.iqm", EF_ROTATE),
		GITEM_INIT_CLIENTSIDE ("i_combatarmor", "Combat Armor", "misc/ar1_pkup.wav"),
		GITEM_INIT_ARMOR (&combatarmor_info, ARMOR_COMBAT),
/* precache */ ""
	},

/*QUAKED item_armor_jacket (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		GITEM_INIT_IDENTIFY (item_armor_jacket, IT_ARMOR),
		GITEM_INIT_CALLBACKS (Pickup_Armor, NULL, NULL, NULL),
		GITEM_INIT_WORLDMODEL ("models/items/armor/jacket/tris.iqm", EF_ROTATE),
		GITEM_INIT_CLIENTSIDE ("i_jacketarmor", "Jacket Armor", "misc/ar1_pkup.wav"),
		GITEM_INIT_ARMOR (&jacketarmor_info, ARMOR_JACKET),
/* precache */ ""
	},

/*QUAKED item_armor_shard (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		GITEM_INIT_IDENTIFY (item_armor_shard, IT_ARMOR),
		GITEM_INIT_CALLBACKS (Pickup_Armor, NULL, NULL, NULL),
		GITEM_INIT_WORLDMODEL ("models/items/armor/shard/tris.iqm", EF_ROTATE),
		GITEM_INIT_CLIENTSIDE ("i_jacketarmor", "Armor Shard", "misc/ar2_pkup.wav"),
		GITEM_INIT_ARMOR (NULL, ARMOR_SHARD),
/* precache */ ""
	},

//CTF
/*QUAKED item_flag_team1 (1 0.2 0) (-16 -16 -24) (16 16 32)
*/
	{
		GITEM_INIT_IDENTIFY (item_flag_red, 0),
		// should we supply a drop callback if we don't want players to drop
		// it manually?
		GITEM_INIT_CALLBACKS (CTFPickup_Flag, NULL, CTFDrop_Flag, NULL),
		GITEM_INIT_WORLDMODEL ("models/items/flags/flag1.iqm", EF_TEAM1),
		GITEM_INIT_CLIENTSIDE ("i_flag1", "Red Flag", NULL),
		GITEM_INIT_OTHER (),
/* precache */ "misc/red_scores.wav misc/red_takes.wav misc/red_increases.wav misc/red_wins.wav misc/scores_tied.wav misc/red_picked.wav"
	},

/*QUAKED item_flag_team2 (1 0.2 0) (-16 -16 -24) (16 16 32)
*/
	{
		GITEM_INIT_IDENTIFY (item_flag_blue, 0),
		// should we supply a drop callback if we don't want players to drop
		// it manually?
		GITEM_INIT_CALLBACKS (CTFPickup_Flag, NULL, CTFDrop_Flag, NULL),
		GITEM_INIT_WORLDMODEL ("models/items/flags/flag2.iqm", EF_TEAM2),
		GITEM_INIT_CLIENTSIDE ("i_flag2", "Blue Flag", NULL),
		GITEM_INIT_OTHER (),
/* precache */ "misc/blue_scores.wav misc/blue_takes.wav misc/blue_increases.wav misc/blue_wins.wav misc/blue_picked.wav"
	},

#ifndef ALTERIA

//Tactical bombs
	{
		GITEM_INIT_IDENTIFY (item_alien_bomb, IT_WEAPON),
		// perhaps eventually write a Pickup_alienBomb? only owner will be
		// able to pick this item back up
		GITEM_INIT_CALLBACKS (NULL, Use_Weapon, NULL, Weapon_TacticalBomb),
		GITEM_INIT_WORLDMODEL ("models/tactical/alien_bomb.iqm", 0),
		// TODO: make an icon for the alien bomb
		GITEM_INIT_CLIENTSIDE (NULL, "Alien Bomb", NULL),
		// will use db's vweap for bombs and detonators
		GITEM_INIT_WEAP (1, 1, "Bombs", "models/objects/blank/tris.iqm", "weapon.iqm"),
		NULL
	},

	{
		GITEM_INIT_IDENTIFY (item_human_bomb, IT_WEAPON),
		// perhaps eventually write a Pickup_humanBomb? only owner will be
		// able to pick this item back up
		GITEM_INIT_CALLBACKS (NULL, Use_Weapon, NULL, Weapon_TacticalBomb),
		GITEM_INIT_WORLDMODEL ("models/tactical/human_bomb.iqm", 0),
		// TODO: make an icon for the human bomb
		GITEM_INIT_CLIENTSIDE (NULL, "Human Bomb", NULL),
		// will use db's vweap for bombs and detonators
		GITEM_INIT_WEAP (1, 1, "Bombs", "models/objects/blank/tris.iqm", "weapon.iqm"),
		NULL
	},
#endif
#ifdef ALTERIA
	//note some of this is clearly temporary placeholder
	{
		GITEM_INIT_IDENTIFY (weapon_warrior_punch, IT_WEAPON),
		GITEM_INIT_CALLBACKS (NULL, Use_Weapon, NULL, Weapon_Punch),
		GITEM_INIT_WORLDMODEL (NULL, 0),
		GITEM_INIT_CLIENTSIDE ("warriorpunch", "Warriorpunch", "misc/w_pkup.wav"),
		GITEM_INIT_WEAP (0, 0, NULL, "models/weapons/v_warriorhands/tris.iqm", "w_violator.iqm"),
/* precache */ "weapons/viofire1.wav weapons/viofire2.wav"
	},

	{
		GITEM_INIT_IDENTIFY (weapon_wizard_punch, IT_WEAPON),
		GITEM_INIT_CALLBACKS (NULL, Use_Weapon, NULL, Weapon_Punch),
		GITEM_INIT_WORLDMODEL (NULL, 0),
		"models/weapons/v_wizardhands/tris.iqm",
		GITEM_INIT_CLIENTSIDE ("wizardpunch", "Wizardpunch", "misc/w_pkup.wav"),
		GITEM_INIT_WEAP (0, 0, NULL, "models/weapons/v_wizardhands/tris.iqm", "w_violator.iqm"),
/* precache */ "weapons/viofire1.wav weapons/viofire2.wav"
	},
#else
/* weapon_blaster (.3 .3 1) (-16 -16 -16) (16 16 16)
always owned, never in the world
*/
	{
		GITEM_INIT_IDENTIFY (weapon_blaster, IT_WEAPON),
		GITEM_INIT_CALLBACKS (NULL, Use_Weapon, NULL, Weapon_Blaster),
		GITEM_INIT_WORLDMODEL (NULL, 0),
		GITEM_INIT_CLIENTSIDE ("i_hud_blaster", "Blaster", "misc/w_pkup.wav"),
		GITEM_INIT_WEAP (0, 0, NULL, "models/weapons/v_blast/tris.iqm", "w_blaster.iqm"),
/* precache */ "weapons/blastf1a.wav misc/lasfly.wav weapons/blasterreload.wav"
	},

	/* weapon_blaster (.3 .3 1) (-16 -16 -16) (16 16 16)
always owned, never in the world
*/
	{
		GITEM_INIT_IDENTIFY (weapon_alienblaster, IT_WEAPON),
		GITEM_INIT_CALLBACKS (NULL, Use_Weapon, NULL, Weapon_AlienBlaster),
		GITEM_INIT_WORLDMODEL (NULL, 0),
		GITEM_INIT_CLIENTSIDE ("i_hud_alienblaster", "Alien Blaster", "misc/w_pkup.wav"),
		GITEM_INIT_WEAP (0, 0, NULL, "models/weapons/v_alienblast/tris.iqm", "w_blaster.iqm"),
/* precache */ "weapons/blastf1a.wav misc/lasfly.wav" //to do tactical - change sound
	},

	{
		GITEM_INIT_IDENTIFY (weapon_violator, IT_WEAPON),
		GITEM_INIT_CALLBACKS (NULL, Use_Weapon, NULL, Weapon_Violator),
		GITEM_INIT_WORLDMODEL (NULL, 0),
		GITEM_INIT_CLIENTSIDE ("i_hud_violator", "Violator", "misc/w_pkup.wav"),
		GITEM_INIT_WEAP (0, 0, NULL, "models/weapons/v_violator/tris.iqm", "w_violator.iqm"),
/* precache */ "weapons/viofire1.wav weapons/viofire2.wav weapons/violatorreload.wav"
	},

/*QUAKED weapon_smartgun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		GITEM_INIT_IDENTIFY (weapon_smartgun, IT_WEAPON),
		GITEM_INIT_CALLBACKS (Pickup_Weapon, Use_Weapon, Drop_Weapon, Weapon_Smartgun),
		GITEM_INIT_WORLDMODEL ("models/weapons/g_smartgun/tris.iqm", EF_ROTATE),
		GITEM_INIT_CLIENTSIDE ("i_hud_smartgun", "Alien Smartgun", "misc/w_pkup.wav"),
		GITEM_INIT_WEAP (1, 1, "Alien Smart Grenade", "models/weapons/v_smartgun/tris.iqm", "w_smartgun.iqm"),
/* precache */ "weapons/clank.wav weapons/shotgf1b.wav weapons/smartgun_hum.wav"
	},

/*QUAKED weapon_chaingun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		GITEM_INIT_IDENTIFY (weapon_chaingun, IT_WEAPON),
		GITEM_INIT_CALLBACKS (Pickup_Weapon, Use_Weapon, Drop_Weapon, Weapon_Chain),
		GITEM_INIT_WORLDMODEL ("models/weapons/g_chaingun/tris.iqm", EF_ROTATE),
		GITEM_INIT_CLIENTSIDE ("i_hud_chaingun", "Chaingun", "misc/w_pkup.wav"), 
		GITEM_INIT_WEAP (1, 1, "Bullets", "models/weapons/v_chaingun/tris.iqm", "w_chaingun.iqm"),
/* precache */ "weapons/machgf1b.wav weapons/machgf2b.wav weapons/machgf3b.wav weapons/machgf4b.wav weapons/shotgf1b.wav weapons/chaingunreload.wav"
	},

/*QUAKED weapon_flamethrower (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		GITEM_INIT_IDENTIFY (weapon_flamethrower, IT_WEAPON),
		GITEM_INIT_CALLBACKS (Pickup_Weapon, Use_Weapon, Drop_Weapon, Weapon_Flame),
		GITEM_INIT_WORLDMODEL ("models/weapons/g_flamethrower/tris.iqm", EF_ROTATE),
		GITEM_INIT_CLIENTSIDE ("i_hud_flamethrower", "Flame Thrower", "misc/w_pkup.wav"),
		GITEM_INIT_WEAP (1, 10, "Napalm", "models/weapons/v_flamethrower/tris.iqm", "w_flamethrower.iqm"),
/* precache */ "weapons/grenlb1b.wav weapons/grenlf1a.wav weapons/flamereload.wav"
	},

/*QUAKED weapon_rocketlauncher (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		GITEM_INIT_IDENTIFY (weapon_rocketlauncher, IT_WEAPON),
		GITEM_INIT_CALLBACKS (Pickup_Weapon, Use_Weapon, Drop_Weapon, Weapon_RocketLauncher),
		GITEM_INIT_WORLDMODEL ("models/weapons/g_rocket/tris.iqm", EF_ROTATE),
		GITEM_INIT_CLIENTSIDE ("i_hud_rocketlauncher", "Rocket Launcher", "misc/w_pkup.wav"),
		GITEM_INIT_WEAP (1, 1, "Rockets", "models/weapons/v_rocket/tris.iqm", "w_rlauncher.iqm"),
/* precache */ "models/objects/rocket/tris.iqm weapons/rockfly.wav weapons/rocklf1a.wav weapons/rlauncherreload.wav models/objects/debris2/tris.iqm"
	},

/*QUAKED weapon_disruptor (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		GITEM_INIT_IDENTIFY (weapon_disruptor, IT_WEAPON),
		GITEM_INIT_CALLBACKS (Pickup_Weapon, Use_Weapon, Drop_Weapon, Weapon_Disruptor),
		GITEM_INIT_WORLDMODEL ("models/weapons/g_disruptor/tris.iqm", EF_ROTATE),
		GITEM_INIT_CLIENTSIDE ("i_hud_disruptor", "Alien Disruptor", "misc/w_pkup.wav"),
		GITEM_INIT_WEAP (5, 10, "Cells", "models/weapons/v_disruptor/tris.iqm", "w_disruptor.iqm"),
/* precache */ "weapons/railgf1a.wav weapons/disruptorreload.wav"
	},

/*QUAKED weapon_railgun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		GITEM_INIT_IDENTIFY (weapon_beamgun, IT_WEAPON),
		GITEM_INIT_CALLBACKS (Pickup_Weapon, Use_Weapon, Drop_Weapon, Weapon_Beamgun),
		GITEM_INIT_WORLDMODEL ("models/weapons/g_beamgun/tris.iqm", EF_ROTATE),
		GITEM_INIT_CLIENTSIDE ("i_hud_beamgun", "Disruptor", "misc/w_pkup.wav"),
		GITEM_INIT_WEAP (1, 1, "Cells", "models/weapons/v_beamgun/tris.iqm", "w_beamgun.iqm"),
/* precache */ "weapons/hyprbf1a.wav weapons/beamgunreload.wav"
	},

/*QUAKED weapon_vaporizer (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		GITEM_INIT_IDENTIFY (weapon_vaporizer, IT_WEAPON),
		GITEM_INIT_CALLBACKS (Pickup_Weapon, Use_Weapon, Drop_Weapon, Weapon_Vaporizer),
		GITEM_INIT_WORLDMODEL ("models/weapons/g_vaporizer/tris.iqm", EF_ROTATE),
		GITEM_INIT_CLIENTSIDE ("i_hud_vaporizer", "Alien Vaporizer", "misc/w_pkup.wav"),
		GITEM_INIT_WEAP (2, 1, "Slugs", "models/weapons/v_vaporizer/tris.iqm", "w_vaporizer.iqm"),
/* precache */ "weapons/energyfield.wav smallmech/sight.wav weapons/vaporizer_hum.wav weapons/vaporizerreload.wav"
	},

	{
		GITEM_INIT_IDENTIFY (weapon_minderaser, IT_WEAPON),
		// not droppable, so drop callback not supplied
		GITEM_INIT_CALLBACKS (Pickup_Weapon, Use_Weapon, NULL, Weapon_Minderaser),
		GITEM_INIT_WORLDMODEL ("models/weapons/g_minderaser/tris.iqm", EF_ROTATE),
		// TODO: create an icon for the mind eraser!
		GITEM_INIT_CLIENTSIDE (NULL, "Minderaser", "misc/w_pkup.wav"),
		GITEM_INIT_WEAP (1, 1, "Seekers", "models/weapons/v_minderaser/tris.iqm", "w_minderaser.iqm"),
/* precache */ "weapons/clank.wav weapons/minderaserfire.wav weapons/shotgf1b.wav weapons/smartgun_hum.wav weapons/minderaserreload.wav misc/minderaser.wav"
	},

	{
		GITEM_INIT_IDENTIFY (weapon_grapple, IT_WEAPON),
		// not droppable, so drop callback not supplied
		GITEM_INIT_CALLBACKS (NULL, Use_Weapon, NULL, CTFWeapon_Grapple),
		GITEM_INIT_WORLDMODEL (NULL, 0),
		// TODO: create an icon for the mind eraser!
		GITEM_INIT_CLIENTSIDE (NULL, "grapple", "misc/w_pkup.wav"),
		GITEM_INIT_WEAP (1, 1, NULL, "models/weapons/v_grapple/tris.iqm", "w_grapple.iqm"),
/* precache */ "weapons/electroball.wav"
	},
		
#endif
	//
	// AMMO ITEMS
	//
/*QUAKED ammo_shells (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		GITEM_INIT_IDENTIFY (ammo_shells, IT_AMMO),
		GITEM_INIT_CALLBACKS (Pickup_Ammo, NULL, Drop_Ammo, NULL),
		GITEM_INIT_WORLDMODEL ("models/items/ammo/shells/medium/tris.iqm", 0),
		GITEM_INIT_CLIENTSIDE ("w_smartgun", "Alien Smart Grenade", "misc/am_pkup.wav"),
		GITEM_INIT_AMMO (10),
/* precache */ ""
	},
/*QUAKED ammo_grenades (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		GITEM_INIT_IDENTIFY (ammo_grenades, IT_AMMO),
		GITEM_INIT_CALLBACKS (Pickup_Ammo, NULL, Drop_Ammo, NULL),
		GITEM_INIT_WORLDMODEL ("models/items/ammo/grenades/medium/tris.iqm", 0),
		GITEM_INIT_CLIENTSIDE ("w_flamethrower", "Napalm", "misc/am_pkup.wav"),
		GITEM_INIT_AMMO (50),
/* precache */ ""
	},

/*QUAKED ammo_bullets (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		GITEM_INIT_IDENTIFY (ammo_bullets, IT_AMMO),
		GITEM_INIT_CALLBACKS (Pickup_Ammo, NULL, Drop_Ammo, NULL),
		GITEM_INIT_WORLDMODEL ("models/items/ammo/bullets/medium/tris.iqm", 0),
		GITEM_INIT_CLIENTSIDE ("w_chaingun", "Bullets", "misc/am_pkup.wav"),
		GITEM_INIT_AMMO (50),
/* precache */ ""
	},

/*QUAKED ammo_cells (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		GITEM_INIT_IDENTIFY (ammo_cells, IT_AMMO),
		GITEM_INIT_CALLBACKS (Pickup_Ammo, NULL, Drop_Ammo, NULL),
		GITEM_INIT_WORLDMODEL ("models/items/ammo/cells/medium/tris.iqm", 0),
		GITEM_INIT_CLIENTSIDE ("w_beamgun", "Cells", "misc/am_pkup.wav"),
		GITEM_INIT_AMMO (50),
/* precache */ ""
	},

/*QUAKED ammo_rockets (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		GITEM_INIT_IDENTIFY (ammo_rockets, IT_AMMO),
		GITEM_INIT_CALLBACKS (Pickup_Ammo, NULL, Drop_Ammo, NULL),
		GITEM_INIT_WORLDMODEL ("models/items/ammo/rockets/medium/tris.iqm", 0),
		GITEM_INIT_CLIENTSIDE ("w_rlauncher", "Rockets", "misc/am_pkup.wav"),
		GITEM_INIT_AMMO (5),
/* precache */ ""
	},

	/*QUAKED ammo_slugs (.3 .3 1) (-16 -16 -16) (16 16 16) - To Do - not sure we use this, check on that.
*/
	{
		GITEM_INIT_IDENTIFY (ammo_slugs, IT_AMMO),
		GITEM_INIT_CALLBACKS (Pickup_Ammo, NULL, Drop_Ammo, NULL),
		GITEM_INIT_WORLDMODEL (NULL, 0),
		GITEM_INIT_CLIENTSIDE ("w_vaporizer", "Slugs", "misc/am_pkup.wav"),
		GITEM_INIT_AMMO (10),
/* precache */ ""
	},


	{
		GITEM_INIT_IDENTIFY (ammo_seekers, IT_AMMO),
		// maybe all callbacks should be NULL here?
		GITEM_INIT_CALLBACKS (Pickup_Ammo, NULL, Drop_Ammo, NULL),
		GITEM_INIT_WORLDMODEL (NULL, 0),
		GITEM_INIT_CLIENTSIDE (NULL, "Seekers", "misc/am_pkup.wav"),
		GITEM_INIT_AMMO (1),
/* precache */ ""
	},

	{
		GITEM_INIT_IDENTIFY (ammo_bombs, IT_AMMO),
		// maybe all callbacks should be NULL here?
		GITEM_INIT_CALLBACKS (Pickup_Ammo, NULL, Drop_Ammo, NULL),
		GITEM_INIT_WORLDMODEL (NULL, 0),
		GITEM_INIT_CLIENTSIDE (NULL, "Bombs", "misc/am_pkup.wav"),
		GITEM_INIT_AMMO (1),
/* precache */ ""
	},

	//
	// POWERUP ITEMS
	//
/*QUAKED item_quad (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		GITEM_INIT_IDENTIFY (item_quad, IT_POWERUP),
		GITEM_INIT_CALLBACKS (Pickup_Powerup, Use_Doubledamage, Drop_General, NULL),
		GITEM_INIT_WORLDMODEL ("models/items/quaddama/tris.iqm", EF_ROTATE),
		GITEM_INIT_CLIENTSIDE ("p_quad", "Double Damage", "items/powerup.wav"),
		GITEM_INIT_POWERUP (150),
/* precache */ "items/damage.wav items/damage2.wav items/damage3.wav"
	},

/*QUAKED item_invulnerability (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		GITEM_INIT_IDENTIFY (item_invulnerability, IT_POWERUP),
		GITEM_INIT_CALLBACKS (Pickup_Powerup, Use_Alienforce, Drop_General, NULL),
		GITEM_INIT_WORLDMODEL ("models/items/invulner/tris.iqm", EF_ROTATE),
		// now "Alien Force" - incoming amage reduced to 1/3, max armor added
		GITEM_INIT_CLIENTSIDE ("p_invulnerability", "Alien Force", "items/powerup.wav"),
		GITEM_INIT_POWERUP (300),
/* precache */ "items/protect.wav items/protect2.wav items/protect4.wav"
	},

/*QUAKED item_adrenaline (.3 .3 1) (-16 -16 -16) (16 16 16)
gives +1 to maximum health
*/
	{
		GITEM_INIT_IDENTIFY (item_adrenaline, IT_HEALTH),
		GITEM_INIT_CALLBACKS (Pickup_Adrenaline, NULL, NULL, NULL),
		GITEM_INIT_WORLDMODEL ("models/items/adrenaline/tris.iqm", EF_ROTATE),
		GITEM_INIT_CLIENTSIDE ("p_adrenaline", "Adrenaline", "items/n_health.wav"),
		GITEM_INIT_POWERUP (60),
/* precache */ ""
	},

	{
		-1,
		NULL,
		IT_HEALTH,
		GITEM_INIT_CALLBACKS (Pickup_Health, NULL, NULL, NULL),
		GITEM_INIT_WORLDMODEL (NULL, 0),
		GITEM_INIT_CLIENTSIDE ("i_health", "Health", "items/l_health.wav"),
		GITEM_INIT_OTHER (),
/* precache */ "items/s_health.wav items/n_health.wav items/l_health.wav items/m_health.wav"
	},

	{
		GITEM_INIT_IDENTIFY (item_haste, IT_POWERUP|IT_BUYABLE),
		GITEM_INIT_CALLBACKS (Pickup_Powerup, Use_Haste, Drop_General, NULL),
		GITEM_INIT_WORLDMODEL ("models/items/haste/tris.iqm", EF_ROTATE),
		GITEM_INIT_CLIENTSIDE ("p_haste", "Haste", "items/powerup.wav"),
		GITEM_INIT_POWERUP (60),
/* precache */ "items/hasteout.wav"
	},
	{
		GITEM_INIT_IDENTIFY (item_sproing, IT_POWERUP|IT_BUYABLE),
		GITEM_INIT_CALLBACKS (Pickup_Powerup, Use_Sproing, Drop_General, NULL),
		GITEM_INIT_WORLDMODEL ("models/items/sproing/tris.iqm", EF_ROTATE),
		GITEM_INIT_CLIENTSIDE ("p_sproing", "Sproing", "items/powerup.wav"),
		GITEM_INIT_POWERUP (60),
/* precache */ "items/sproingout.wav"
	},

	//this item is handled uniquely 
	{
		GITEM_INIT_IDENTIFY (item_jetpack, IT_POWERUP),
		GITEM_INIT_CALLBACKS (Get_in_vehicle, NULL, Leave_vehicle, NULL),
		GITEM_INIT_WORLDMODEL ("vehicles/jetpack/tris.iqm", EF_ROTATE),
		GITEM_INIT_CLIENTSIDE ("jetpack", "Jetpack", NULL),
		GITEM_INIT_POWERUP (0),
		NULL
	},

	//these next two powerups are never placed in maps
	{
		GITEM_INIT_IDENTIFY (item_invisibility, IT_POWERUP|IT_BUYABLE),
		// we don't want people dropping this one, so drop callback not given
		GITEM_INIT_CALLBACKS (Pickup_Powerup, Use_Invisibility, NULL, NULL),
		GITEM_INIT_WORLDMODEL (NULL, EF_ROTATE),
		GITEM_INIT_CLIENTSIDE (NULL, "Invisibility", "items/powerup.wav"),
		GITEM_INIT_POWERUP (60),
/* precache */ "items/protect2.wav"
	},

	// end of list marker
	{-1, NULL}
};


/*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
void SP_item_health (edict_t *self)
{
	if ( deathmatch->integer && (dmflags->integer & DF_NO_HEALTH) )
	{
		G_FreeEdict (self);
		return;
	}
	self->model = "models/items/healing/medium/tris.iqm";
	self->classname = "Health";
	self->count = 10;
	SpawnItem (self, FindItem ("Health"));
	gi.soundindex ("items/n_health.wav");
}

/*QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
void SP_item_health_small (edict_t *self)
{
	if ( deathmatch->integer && (dmflags->integer & DF_NO_HEALTH) )
	{
		G_FreeEdict (self);
		return;
	}

	self->model = "models/items/healing/small/tris.iqm";
	self->count = 5;
	self->classname = "Health";
	SpawnItem (self, FindItem ("Health"));
	self->style = HEALTH_IGNORE_MAX;
	gi.soundindex ("items/s_health.wav");
}

/*QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
void SP_item_health_large (edict_t *self)
{
	if ( deathmatch->integer && (dmflags->integer & DF_NO_HEALTH) )
	{
		G_FreeEdict (self);
		return;
	}

	self->model = "models/items/healing/large/tris.iqm";
	self->classname = "Health";
	self->count = 25;
	SpawnItem (self, FindItem ("Health"));
	gi.soundindex ("items/l_health.wav");
}

/*QUAKED item_health_mega (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
void SP_item_health_mega (edict_t *self)
{
	if ( deathmatch->integer && (dmflags->integer & DF_NO_HEALTH) )
	{
		G_FreeEdict (self);
		return;
	}

	self->model = "models/items/mega_h/tris.iqm";
	self->count = 100;
	SpawnItem (self, FindItem ("Health"));
	gi.soundindex ("items/m_health.wav");
	self->style = HEALTH_IGNORE_MAX|HEALTH_TIMED;
}


void InitItems (void)
{
	int i, j, curr_weapmodel_idx = 0;
	
	game.num_items = sizeof(itemlist)/sizeof(itemlist[0]) - 1;
	
	for (i = 1; i < game.num_items; i++)
	{
		if (itemlist[i].weapmodel != NULL)
		{
			// insure uniqueness
			for (j = 1; j < i; j++)
			{
				if (itemlist[j].weapmodel != NULL &&
				    !Q_strcasecmp (itemlist[j].weapmodel, itemlist[i].weapmodel))
					break;
			}
			if (j == i)
				itemlist[i].weapmodel_idx = ++curr_weapmodel_idx; // must start at 1!
			else
				itemlist[i].weapmodel_idx = itemlist[j].weapmodel_idx;
		}
	}
}



/*
===============
SetItemNames

Called by worldspawn
===============
*/
void SetItemNames (void)
{
	int		i;
	gitem_t	*it;

	for (i=0 ; i<game.num_items ; i++)
	{
		it = &itemlist[i];
		gi.configstring (CS_ITEMS+i, it->pickup_name);
	}

	jacket_armor_index = ITEM_INDEX(FindItem("Jacket Armor"));
	combat_armor_index = ITEM_INDEX(FindItem("Combat Armor"));
	body_armor_index   = ITEM_INDEX(FindItem("Body Armor"));
}
