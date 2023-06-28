///////////////////////////////////////////////////////////////////////
//
//  ACE - Quake II Bot Base Code
//
//  Version 1.0
//
//  This file is Copyright(c), Steve Yeager 1998, All Rights Reserved
//
//
//	All other files are Copyright(c) Id Software, Inc.
//
//	Please see liscense.txt in the source directory for the copyright
//	information regarding those files belonging to Id Software, Inc.
//
//	Should you decide to release a modified version of ACE, you MUST
//	include the following text (minus the BEGIN and END lines) in the
//	documentation for your modification.
//
//	--- BEGIN ---
//
//	The ACE Bot is a product of Steve Yeager, and is available from
//	the ACE Bot homepage, at http://www.axionfx.com/ace.
//
//	This program is a modification of the ACE Bot, and is therefore
//	in NO WAY supported by Steve Yeager.

//	This program MUST NOT be sold in ANY form. If you have paid for
//	this product, you should contact Steve Yeager immediately, via
//	the ACE Bot homepage.
//
//	--- END ---
//
//	I, Steve Yeager, hold no responsibility for any harm caused by the
//	use of this source code, especially to small children and animals.
//  It is provided as-is with no implied warranty or support.
//
//  I also wish to thank and acknowledge the great work of others
//  that has helped me to develop this code.
//
//  John Cricket    - For ideas and swapping code.
//  Ryan Feltrin    - For ideas and swapping code.
//  SABIN           - For showing how to do true client based movement.
//  BotEpidemic     - For keeping us up to date.
//  Telefragged.com - For giving ACE a home.
//  Microsoft       - For giving us such a wonderful crash free OS.
//  id              - Need I say more.
//
//  And to all the other testers, pathers, and players and people
//  who I can't remember who the heck they were, but helped out.
//
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//
//  acebot_items.c - This file contains all of the
//                   item handling routines for the
//                   ACE bot, including fact table support
//
///////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "game/g_local.h"
#include "acebot.h"

int num_items = 0;
item_table_t item_table[MAX_EDICTS];

static gitem_t *redflag;
static gitem_t *blueflag;


void ACEAI_InitKnownCTFItems(void)
{
	//int		i;

	//define gitem_t pointers commonly used for comparisons inside this archive.
	redflag = FindItemByClassname("item_flag_red");
	blueflag = FindItemByClassname("item_flag_blue");

}

///////////////////////////////////////////////////////////////////////
// Can we get there?
///////////////////////////////////////////////////////////////////////
qboolean ACEIT_IsReachable(edict_t *self, vec3_t goal)
{
	trace_t trace;
	vec3_t v;

	VectorCopy(self->mins,v);
	v[2] += 18; // Stepsize

	trace = gi.trace (self->s.origin, v, self->maxs, goal, self, BOTMASK_OPAQUE);

	// Yes we can see it
	if (trace.fraction == 1.0)
		return true;
	else
		return false;

}

///////////////////////////////////////////////////////////////////////
// Visiblilty check
///////////////////////////////////////////////////////////////////////
qboolean ACEIT_IsVisible(edict_t *self, vec3_t goal)
{
	trace_t trace;

	trace = gi.trace (self->s.origin, vec3_origin, vec3_origin, goal, self, BOTMASK_OPAQUE);

	// Yes we can see it
	if (trace.fraction == 1.0)
		return true;
	else
		return false;

}

///////////////////////////////////////////////////////////////////////
//  Weapon changing support
///////////////////////////////////////////////////////////////////////
qboolean ACEIT_ChangeWeapon (edict_t *ent, gitem_t *item)
{
	int			ammo_index;
	gitem_t		*ammo_item;
	
	// make sure it's a valid item
	if (item == NULL)
		return false;

	// see if we're already using it
	if (item == ent->client->pers.weapon)
		return true;

	// Has not picked up weapon yet
	if(!ent->client->pers.inventory[ITEM_INDEX(item)])
		return false;

	// Do we have ammo for it?
	if (item->ammo)
	{
		ammo_item = FindItem(item->ammo);
		ammo_index = ITEM_INDEX(ammo_item);
		if (!ent->client->pers.inventory[ammo_index] && !g_select_empty->value)
			return false;
	}

	// Change to this weapon
	ent->client->newweapon = item;

	// Make weapon swap visible.	
	ChangeWeapon( ent );

	return true;
}


extern gitem_armor_t jacketarmor_info;
extern gitem_armor_t combatarmor_info;
extern gitem_armor_t bodyarmor_info;

///////////////////////////////////////////////////////////////////////
// Check if we can use the armor
///////////////////////////////////////////////////////////////////////
qboolean ACEIT_CanUseArmor (gitem_t *item, edict_t *other)
{
	int				old_armor_index;
	gitem_armor_t	*oldinfo;
	gitem_armor_t	*newinfo;
	int				newcount;
	float			salvage;
	int				salvagecount;

	// get info on new armor
	newinfo = (gitem_armor_t *)item->info;

	old_armor_index = ArmorIndex (other);

	// handle armor shards specially
	if (item->tag == ARMOR_SHARD)
		return true;

	// get info on old armor
	if (old_armor_index == ITEM_INDEX(FindItem("Jacket Armor")))
		oldinfo = &jacketarmor_info;
	else if (old_armor_index == ITEM_INDEX(FindItem("Combat Armor")))
		oldinfo = &combatarmor_info;
	else // (old_armor_index == body_armor_index)
		oldinfo = &bodyarmor_info;

	if (newinfo->normal_protection <= oldinfo->normal_protection)
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

	}

	return true;
}
//==========================================
// find needed flag
//
//==========================================
gitem_t	*ACEIT_WantedFlag (edict_t *self)
{
	qboolean	hasflag;

	if (!ctf->value)
		return NULL;

	//find out if the player has a flag, and what flag is it
	if (redflag && self->client->pers.inventory[ITEM_INDEX(redflag)])
		hasflag = true;
	else if (blueflag && self->client->pers.inventory[ITEM_INDEX(blueflag)])
		hasflag = true;
	else
		hasflag = false;

	//jalToDo: see if our flag is at base

	if (!hasflag)//if we don't have a flag we want other's team flag
	{
		if (self->dmteam == RED_TEAM)
			return blueflag;
		else
			return redflag;
	}
	else	//we have a flag
	{
		if (self->dmteam == BLUE_TEAM)
			return redflag;
		else
			return blueflag;
	}

	return NULL;
}


///////////////////////////////////////////////////////////////////////
// Determins the NEED for an item
//
// This function can be modified to support new items to pick up
// Any other logic that needs to be added for custom decision making
// can be added here. For now it is very simple.
///////////////////////////////////////////////////////////////////////
float ACEIT_ItemNeed (edict_t *self, gitem_t *item)
{
	int idx;
	
	if (item == NULL)
		return 0.0;
	
	idx = ITEM_INDEX (item);
	
	// if the item is an ammo type, make sure we have room for it
#define X(name,itname,base,max,excessivemult)	\
	if (item->classnum == ammo_##name) \
	{ \
		if (excessive->integer) \
		{ \
			if (self->client->pers.inventory[idx] >= g_max##name->integer*excessivemult) \
				return 0.0; \
		} \
		else if (self->client->pers.inventory[idx] >= g_max##name->integer) \
		{ \
			return 0.0; \
		} \
	}
	
	AMMO_TYPES
	
#undef X
	
	if (item->flags & IT_HEALTH)
	{
		if (self->health < 100)
			return 1.0 - (float)self->health / 100.0f; // worse off, higher priority
		else
			return 0.0;
	}
	
	if (item->flags & IT_POWERUP)
		return 0.6;
	
	if (item->flags & IT_ARMOR)
	{
		if (ACEIT_CanUseArmor (item, self))
			return 0.6;
		else
			return 0.0;
	}

	switch (item->classnum)
	{
		// Weapons
		case weapon_rocketlauncher: case weapon_chaingun:
		case weapon_flamethrower: case weapon_vaporizer:
			if(g_tactical->integer && self->ctype == 0)
				return 0.0;
			else if(g_tactical->integer && self->ctype == 1)
			{
				if (!self->client->pers.inventory[idx])
					return 0.9;
				else
					return 0.0;
			}
		case weapon_smartgun: case weapon_disruptor: case weapon_beamgun:
		case weapon_minderaser:
			if(g_tactical->integer && self->ctype == 1)
				return 0.0;
			else if(g_tactical->integer && self->ctype == 0)
			{
				if (!self->client->pers.inventory[idx])
					return 0.9;
				else
					return 0.0;
			}

			//normal game mode
			if (!self->client->pers.inventory[idx])
				return 0.9; //was .7
			else
				return 0.0;

		// Ammo
		case ammo_slugs:
			return 0.4;

		case ammo_bullets: case ammo_shells: case ammo_cells:
		case ammo_grenades:
			return 0.3;

		case ammo_rockets:
			return 1.5;
		
		//flags
		case item_flag_red: case item_flag_blue:
			if (item != ACEIT_WantedFlag (self))
				return 0.0;
			else
				return 3.0;
		
		//vehicles
		case item_jetpack:
			if (!self->client->pers.inventory[idx])
				return 0.9;
			else
				return 0.0;

		default:
			return 0.0;

	}

}

///////////////////////////////////////////////////////////////////////
// Only called once per level, when saved will not be called again
//
// Downside of the routine is that items can not move about. If the level
// has been saved before and reloaded, it could cause a problem if there
// are items that spawn at random locations.
//
//#define DEBUG // uncomment to write out items to a file.
///////////////////////////////////////////////////////////////////////
void ACEIT_BuildItemNodeTable (qboolean rebuild)
{
	edict_t *items;
	int i;
	vec3_t v,v1,v2;

#ifdef DEBUG
	FILE *pOut; // for testing
	if((pOut = fopen("items.txt","wt"))==NULL)
		return;
#endif

	num_items = 0;

	// Add game items
	for(items = g_edicts; items < &g_edicts[globals.num_edicts]; items++)
	{
		// filter out crap
		if(items->solid == SOLID_NOT)
			continue;

		if(!items->classname)
			continue;

		if(strcmp(items->classname,"func_plat")==0)
		{
			// Special node dropping for platforms
			if(!rebuild)
				ACEND_AddNode(items,NODE_PLATFORM);
		}
		else if (strcmp (items->classname, "misc_teleporter_dest") == 0 || strcmp (items->classname, "misc_teleporter") == 0)
		{
			// Special node dropping for teleporters
			if(!rebuild)
				ACEND_AddNode(items,NODE_TELEPORTER);
		}
		else if (items->item == NULL)
		{
			// besides platforms and teleporters, all nodes must be items.
#ifdef DEBUG
			fprintf (pOut, "Rejected ent for not being an item: %s pos: %f %f %f\n", items->classname, items->s.origin[0], items->s.origin[1], items->s.origin[2]);
#endif
			continue;
		}

		// add a pointer to the item entity
		item_table[num_items].ent = items;

		// If new, add nodes for items
		if(!rebuild)
		{
			// Add a new node at the item's location.
			item_table[num_items].node = ACEND_AddNode(items,NODE_ITEM);
#ifdef DEBUG
			fprintf (pOut, "item: %s node: %d pos: %f %f %f\n", items->classname, item_table[num_items].node, items->s.origin[0], items->s.origin[1], items->s.origin[2]);
#endif
			num_items++;
		}
		else // Now if rebuilding, just relink ent structures
		{
			// Find stored location
			for(i=0;i<bot_numnodes;i++)
			{
				if(nodes[i].type == NODE_ITEM ||
				   nodes[i].type == NODE_PLATFORM ||
				   nodes[i].type == NODE_TELEPORTER) // valid types
				{
					VectorCopy(items->s.origin,v);

					// Add 16 to item type nodes
					if(nodes[i].type == NODE_ITEM)
						v[2] += 16;

					// Add 32 to teleporter
					if(nodes[i].type == NODE_TELEPORTER)
						v[2] += 32;

					if(nodes[i].type == NODE_PLATFORM)
					{
						VectorCopy(items->maxs,v1);
						VectorCopy(items->mins,v2);

						// To get the center
						v[0] = (v1[0] - v2[0]) / 2 + v2[0];
						v[1] = (v1[1] - v2[1]) / 2 + v2[1];
						v[2] = items->mins[2]+64;
					}

					if(v[0] == nodes[i].origin[0] &&
 					   v[1] == nodes[i].origin[1] &&
					   v[2] == nodes[i].origin[2])
					{
						// found a match now link to facts
						item_table[num_items].node = i;
#ifdef DEBUG
						fprintf(pOut,"Relink item: %s node: %d pos: %f %f %f\n",items->classname,item_table[num_items].node,items->s.origin[0],items->s.origin[1],items->s.origin[2]);
#endif
						num_items++;
					}
				}
			}
		}


	}

#ifdef DEBUG
	fclose(pOut);
#endif

}
