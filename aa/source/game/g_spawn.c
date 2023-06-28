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

extern void SV_LinkEdict (edict_t *ent);

typedef struct
{
	char	*name;
	void	(*spawn)(edict_t *ent);
} spawn_t;

spawn_t	spawns[] = {
	{"item_health", SP_item_health},
	{"item_health_small", SP_item_health_small},
	{"item_health_large", SP_item_health_large},
	{"item_health_mega", SP_item_health_mega},

	{"info_player_start", SP_info_player_start},
	{"info_player_deathmatch", SP_info_player_deathmatch},
	{"info_player_intermission", SP_info_player_intermission},
	{"info_player_red", SP_info_player_red},
	{"info_player_blue", SP_info_player_blue},

	{"func_plat", SP_func_plat},
	{"func_button", SP_func_button},
	{"func_door", SP_func_door},
	{"func_door_secret", SP_func_door_secret},
	{"func_door_rotating", SP_func_door_rotating},
	{"func_rotating", SP_func_rotating},
	{"func_train", SP_func_train},
	{"func_water", SP_func_water},
	{"func_conveyor", SP_func_conveyor},
	{"func_areaportal", SP_func_areaportal},
	{"func_wall", SP_func_wall},
	{"func_object", SP_func_object},
	{"func_timer", SP_func_timer},
	{"func_explosive", SP_func_explosive},
	{"func_killbox", SP_func_killbox},

	{"trigger_always", SP_trigger_always},
	{"trigger_once", SP_trigger_once},
	{"trigger_multiple", SP_trigger_multiple},
	{"trigger_relay", SP_trigger_relay},
	{"trigger_push", SP_trigger_push},
	{"trigger_hurt", SP_trigger_hurt},
	{"trigger_key", SP_trigger_key},
	{"trigger_counter", SP_trigger_counter},
	{"trigger_elevator", SP_trigger_elevator},
	{"trigger_gravity", SP_trigger_gravity},
	{"trigger_monsterjump", SP_trigger_monsterjump},
	
	{"target_temp_entity", SP_target_temp_entity},
	{"target_speaker", SP_target_speaker},
	{"target_explosion", SP_target_explosion},
	{"target_secret", SP_target_secret},
	{"target_splash", SP_target_splash},
	{"target_steam", SP_target_steam},
	{"target_spawner", SP_target_spawner},
#ifndef ALTERIA
	{"target_blaster", SP_target_blaster},
	{"target_laser", SP_target_laser},
#endif
	{"target_lightramp", SP_target_lightramp},
	{"target_earthquake", SP_target_earthquake},
	{"target_fire", SP_target_fire},
	{"target_dust", SP_target_dust},
	{"target_changelevel", SP_target_changelevel},

	{"worldspawn", SP_worldspawn},

	{"light", SP_light},
	{"info_null", SP_info_null},
	{"func_group", SP_info_null},
	{"info_notnull", SP_info_notnull},
	{"path_corner", SP_path_corner},
	{"point_combat", SP_point_combat},

	{"misc_teleporter", SP_misc_teleporter},
	{"misc_teleporter_dest", SP_misc_teleporter_dest},

	//Tactical
	{"misc_aliencomputer", SP_misc_aliencomputer},
	{"misc_humancomputer", SP_misc_humancomputer},
	{"misc_alienpowersrc", SP_misc_alienpowersrc},
	{"misc_humanpowersrc", SP_misc_humanpowersrc},
	{"misc_alienammodepot", SP_misc_alienammodepot},
	{"misc_humanammodepot", SP_misc_humanammodepot},
	{"misc_alienbackupgen", SP_misc_alienbackupgen},
	{"misc_humanbackupgen", SP_misc_humanbackupgen},
	{"misc_deathray", SP_misc_deathray}, //note - spawnflags determine team for this item(1 for human)
	{"misc_laser", SP_misc_laser}, //spawnflag 1 for human

	//Monsters and hazards
	{"npc_piranha", SP_monster_piranha},	
	{"npc_deathray", SP_npc_deathray},

	{"misc_mapmodel", SP_misc_mapmodel},
	{"misc_watersplash", SP_misc_watersplash},
	{"misc_electroflash", SP_misc_electroflash},
	
	//Game code ignores these
	{"misc_terrainmodel", SP_info_null},
	{"misc_decal", SP_info_null},
	
	{NULL, NULL}
};

static void ED_PostSpawn_SanityCheck (edict_t *ent)
{
	if (ent->solid == SOLID_BSP && (ent->model == NULL || ent->model[0] != '*'))
	{
		gi.dprintf ("Changing %s from SOLID_BSP to SOLID_NOT "
					"(entity has no brushes!)\n", ent->classname);
		ent->solid = SOLID_NOT;
	}
	
	// HACK: since solidness may have changed, if the entity is
	// already linked, it must be relinked.
	if (ent->linkcount != 0)
		SV_LinkEdict (ent);
}

/*
===============
ED_CallSpawn

Finds the spawn function for the entity and calls it
===============
*/
void ED_CallSpawn (edict_t *ent)
{
	spawn_t	*s;
	gitem_t	*item;
	int		i;

	if (!ent->classname)
	{
		gi.dprintf ("ED_CallSpawn: NULL classname\n");
		return;
	}

	// check item spawn functions
	for (i=0,item=itemlist ; i<game.num_items ; i++,item++)
	{
		if (!item->classname)
			continue;

		if(!Q_strcasecmp(ent->classname, "weapon_minderaser"))
			continue; //never place one of these traditionally

		if (!strcmp(item->classname, ent->classname))
		{	// found it
			SpawnItem (ent, item);
			return;
		}
	}

	gi.WriteByte (svc_temp_entity);
	gi.WriteByte (TE_TELEPORT_EFFECT);
	gi.WritePosition (ent->s.origin);
	gi.multicast (ent->s.origin, MULTICAST_PVS);

	// check normal spawn functions
	for (s=spawns ; s->name ; s++)
	{
		if (!strcmp(s->name, ent->classname))
		{	// found it
			s->spawn (ent);
			if (ent != g_edicts && ent->inuse)
				ED_PostSpawn_SanityCheck (ent);
			return;
		}
	}

	gi.dprintf ("%s doesn't have a spawn function\n", ent->classname);
}

/*
=============
ED_NewString
=============
*/
char *ED_NewString (char *string)
{
	char	*newb, *new_p;
	int		i,l;

	l = strlen(string) + 1;

	newb = gi.TagMalloc (l, TAG_LEVEL);

	new_p = newb;

	for (i=0 ; i< l ; i++)
	{
		if (string[i] == '\\' && i < l-1)
		{
			i++;
			if (string[i] == 'n')
				*new_p++ = '\n';
			else
				*new_p++ = '\\';
		}
		else
			*new_p++ = string[i];
	}

	return newb;
}




/*
===============
ED_ParseField

Takes a key/value pair and sets the binary values
in an edict
===============
*/
void ED_ParseField (char *key, char *value, edict_t *ent)
{
	field_t	*f;
	byte	*b;
	float	v;
	vec3_t	vec;

	for (f=fields ; f->name ; f++)
	{
		if (!Q_strcasecmp(f->name, key))
		{	// found it
			if (f->flags & FFL_SPAWNTEMP)
				b = (byte *)&st;
			else
				b = (byte *)ent;

			switch (f->type)
			{
			case F_LSTRING:
				*(char **)(b+f->ofs) = ED_NewString (value);
				break;
			case F_VECTOR:
				sscanf (value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
				((float *)(b+f->ofs))[0] = vec[0];
				((float *)(b+f->ofs))[1] = vec[1];
				((float *)(b+f->ofs))[2] = vec[2];
				break;
			case F_INT:
				*(int *)(b+f->ofs) = atoi(value);
				break;
			case F_FLOAT:
				*(float *)(b+f->ofs) = atof(value);
				break;
			case F_ANGLEHACK:
				v = atof(value);
				((float *)(b+f->ofs))[0] = 0;
				((float *)(b+f->ofs))[1] = v;
				((float *)(b+f->ofs))[2] = 0;
				break;
			case F_IGNORE:
			default:
				break;
			}
			return;
		}
	}
	gi.dprintf ("%s is not a field\n", key);
}

/*
====================
ED_ParseEdict

Parses an edict out of the given string, returning the new position
ed should be a properly initialized empty edict.
====================
*/
static const char *ED_ParseEdict (const char *data, edict_t *ent)
{
	qboolean	init;
	char		keyname[256];
	char		*com_token;

	init = false;
	memset (&st, 0, sizeof(st));

// go through all the dictionary pairs
	while (1)
	{
	// parse key
		com_token = COM_Parse (&data);
		if (com_token[0] == '}')
			break;
		if (!data)
			gi.error ("ED_ParseEntity: EOF without closing brace");

		strncpy (keyname, com_token, sizeof(keyname)-1);

	// parse value
		com_token = COM_Parse (&data);
		if (!data)
			gi.error ("ED_ParseEntity: EOF without closing brace");

		if (com_token[0] == '}')
			gi.error ("ED_ParseEntity: closing brace without data");

		init = true;

	// keynames with a leading underscore are used for utility comments,
	// and are immediately discarded by quake
		if (keyname[0] == '_')
			continue;

		ED_ParseField (keyname, com_token, ent);
	}

	if (!init)
		memset (ent, 0, sizeof(*ent));

	return data;
}


/*
================
G_FindTeams

Chain together all entities with a matching team field.

All but the first will have the FL_TEAMSLAVE flag set.
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams (void)
{
	edict_t	*e, *e2, *chain;
	int		i, j;
	int		c, c2;

	c = 0;
	c2 = 0;
	for (i=1, e=g_edicts+i ; i < globals.num_edicts ; i++,e++)
	{
		if (!e->inuse)
			continue;
		if (!e->team)
			continue;
		if (e->flags & FL_TEAMSLAVE)
			continue;
		chain = e;
		e->teammaster = e;
		c++;
		c2++;
		for (j=i+1, e2=e+1 ; j < globals.num_edicts ; j++,e2++)
		{
			if (!e2->inuse)
				continue;
			if (!e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;
			if (!strcmp(e->team, e2->team))
			{
				c2++;
				chain->teamchain = e2;
				e2->teammaster = e;
				chain = e2;
				e2->flags |= FL_TEAMSLAVE;
			}
		}
	}

	gi.dprintf ("%i teams with %i entities\n", c, c2);
}

/*
==============
SpawnEntities

Creates a server's entity / program execution context by
parsing textual entity definitions out of an ent file.
==============
*/
int levelnum;
void SpawnEntities (char *mapname, const char *entities, char *spawnpoint)
{
	edict_t		*ent;
	int			inhibit;
	char		*com_token;
	int			i;
	float		skill_level;

	skill_level = floor (skill->value);
	if (skill_level < -1)
		skill_level = -1;
	if (skill_level > 3)
		skill_level = 3;
	if (skill->value != skill_level)
		gi.cvar_forceset("skill", va("%f", skill_level));

	SaveClientData ();

	gi.FreeTags (TAG_LEVEL);
	
	red_team_score = 0;
	blue_team_score = 0;
	
	memset (&level, 0, sizeof(level));
	memset (g_edicts, 0, game.maxentities * sizeof (g_edicts[0]));

	strncpy (level.mapname, mapname, sizeof(level.mapname)-1);

	// set client fields on player ents
	for (i = 0; i < game.maxclients; i++)
	{
		g_edicts[i+1].client = game.clients + i;
		game.clients[i].pers.queue = 0; // FIXME: kinda hacky
	}

	ent = NULL;
	inhibit = 0;
	levelnum++;

// parse ents
	while (1)
	{
		// parse the opening brace
		com_token = COM_Parse (&entities);
		if (!entities)
			break;
		if (com_token[0] != '{')
			gi.error ("ED_LoadFromFile: found %s when expecting {",com_token);

		if (!ent)
			ent = g_edicts;
		else
			ent = G_Spawn ();
		entities = ED_ParseEdict (entities, ent);

		// yet another map hack
		if (!Q_strcasecmp(level.mapname, "command") && !Q_strcasecmp(ent->classname, "trigger_once") && !Q_strcasecmp(ent->model, "*27"))
			ent->spawnflags &= ~SPAWNFLAG_NOT_HARD;

		// remove things (except the world) from different skill levels or deathmatch
		if (ent != g_edicts)
		{
			if (deathmatch->value)
			{
				if ( ent->spawnflags & SPAWNFLAG_NOT_DEATHMATCH )
				{
					G_FreeEdict (ent);
					inhibit++;
					continue;
				}
			}
			else
			{
				if (
					((skill->value == 0) && (ent->spawnflags & SPAWNFLAG_NOT_EASY)) ||
					((skill->value == 1) && (ent->spawnflags & SPAWNFLAG_NOT_MEDIUM)) ||
					(((skill->value == 2) || (skill->value == 3)) && (ent->spawnflags & SPAWNFLAG_NOT_HARD))
					)
					{
						G_FreeEdict (ent);
						inhibit++;
						continue;
					}
			}

			ent->spawnflags &= ~(SPAWNFLAG_NOT_EASY|SPAWNFLAG_NOT_MEDIUM|SPAWNFLAG_NOT_HARD|SPAWNFLAG_NOT_DEATHMATCH);
		}

		ED_CallSpawn (ent);
	}

	gi.dprintf ("%i entities inhibited\n", inhibit);

	//reset minderaser and jetpacks
	mindEraserTime = jetpackTime = level.time;	

	ACEND_InitNodes();
	ACEND_LoadNodes();

	G_FindTeams ();

	PlayerTrail_Init ();
}


//===================================================================

#if 0
	// cursor positioning
	xl <value>
	xr <value>
	yb <value>
	yt <value>
	xv <value>
	yv <value>

	// drawing
	statpic <name>
	pic <stat>
	num <fieldwidth> <stat>
	string <stat>

	// control
	if <stat>
	ifeq <stat> <value>
	ifbit <stat> <value>
	endif

#endif

char *dm_statusbar =
//background
"yb -256 "
"xl	 0 "
"pic 0 "
"xr  -130 "
"yt  2 "
"pic 18 "
"yb -256 "
"xr -256 "
"pic 2 "

// health
"yb	-29 "
"xl	11 "
"hnum "

// ammo
"if 2 "
"	yb	-38 "
"	xr	-140 "
"	anum "
"endif "

// armor
"   yb -29 "
"	xl	76 "
"	rnum "

// timer
"if 9 "
"	xv	262 "
"   yb  -24 "
"	num	2	10 "
"	xv	296 "
"   yb  -32 "
"	pic	9 "
"endif "

// weapon icon
"if 11 "
"	xr	-80 "
"   yb  -43 "
"	pic	11 "
"endif "

//  frags
"xr	-67 "
"yt 16 "
"num 3 14"

//  deaths
"xr	-67 "
"yt 48 "
"num 3 19 "

//  high scorer
"yt 80 "
"num 3 20 "
;

char *team_statusbar =
// background
"yb -256 "
"xl	 0 "
"pic 0 "
"xr  -130 "
"yt  2 "
"pic 18 "
"yb -256 "
"xr -256 "
"pic 2 "

// health
"yb	-29 "
"xl	11 "
"hnum "

// ammo
"if 2 "
"	yb	-38 "
"	xr	-140 "
"	anum "
"endif "

// armor
"   yb -29 "
"	xl	76 "
"	rnum "

// timer
"if 9 "
"	xv	262 "
"   yb  -24 "
"	num	2	10 "
"	xv	296 "
"   yb  -32 "
"	pic	9 "
"endif "

// weapon icon
"if 11 "
"	xr	-80 "
"   yb  -43 "
"	pic	11 "
"endif "

//  frags
"xr	-67 "
"yt 16 "
"num 3 14"

//  deaths
"xr	-67 "
"yt 48 "
"num 3 19 "

//  high scorer
"yt 80 "
"num 3 20 "

//  red team
"yt 132 "
"num 3 21 "
//  blue team
"yt 166 "
"num 3 22 "

//  flag
"   xv 100 "
"   yb -120 "
"   pic 23 "
;

char *tactical_statusbar =
// background
"yb -256 "
"xl	 0 "
"pic 0 "
"xr  -130 "
"yt  2 "
"pic 8 "
"yb -256 "
"xr -256 "
"pic 2 "

// health
"yb	-29 "
"xl	11 "
"hnum "

// ammo
"if 2 "
"	yb	-38 "
"	xr	-140 "
"	anum "
"endif "

// armor
"   yb -29 "
"	xl	76 "
"	rnum "

//  Human Computer
"xr	-67 "
"yt 16 "
"num 3 25 "

//  Human Power
"yt 48 "
"num 3 26 "

//  Human Ammo Depot
"yt 80 "
"num 3 27 "

//  Alien Computer
"yt 122 "
"num 3 28 "

//  Alien Power
"yt 154 "
"num 3 29 "

//  Alien Ammo Depot
"yt 186 "
"num 3 30 "

// weapon icon
"if 11 "
"	xr	-80 "
"   yb  -43 "
"	pic	11 "
"endif "
;
/*QUAKED worldspawn (0 0 0) ?

Only used for the world.
"sky"	environment map name
"skyaxis"	vector axis for rotating sky
"skyrotate"	speed of rotation in degrees/second
"sounds"	music cd track number
"gravity"	800 is default gravity
"message"	text to print at user logon
*/
void SP_worldspawn (edict_t *ent)
{
	ent->movetype = MOVETYPE_PUSH;
	ent->solid = SOLID_BSP;
	ent->inuse = true;			// since the world doesn't use G_Spawn()
	ent->s.modelindex = 1;		// world model is always index 1

	//---------------

	// reserve some spots for dead player bodies for coop / deathmatch
	InitBodyQue ();

	// set configstrings for items
	SetItemNames ();

	if (st.nextmap)
		strcpy (level.nextmap, st.nextmap);

	// make some data visible to the server

	if (ent->message && ent->message[0])
	{
		gi.configstring (CS_NAME, ent->message);
		strncpy (level.level_name, ent->message, sizeof(level.level_name));
	}
	else
		strncpy (level.level_name, level.mapname, sizeof(level.level_name));

	if (st.sky && st.sky[0])
		gi.configstring (CS_SKY, st.sky);
	else
		gi.configstring (CS_SKY, "hspace");

	gi.configstring (CS_SKYROTATE, va("%f", st.skyrotate) );

	gi.configstring (CS_SKYAXIS, va("%f %f %f",
		st.skyaxis[0], st.skyaxis[1], st.skyaxis[2]) );

	gi.configstring (CS_MAXCLIENTS, va("%i", g_maxclients->integer ) );

	// status bar program
	if ((dmflags->integer & DF_SKINTEAMS) || ctf->integer)
	{
		gi.configstring (CS_STATUSBAR, team_statusbar);
		if(ctf->value)
			CTFPrecache();
	}
	else if (g_tactical->integer)
		gi.configstring (CS_STATUSBAR, tactical_statusbar);
	else
		gi.configstring (CS_STATUSBAR, dm_statusbar);

	//---------------


	// help icon for statusbar
	level.pic_health = gi.imageindex ("i_health");
	gi.imageindex ("help");

	if (!st.gravity) {
		if(low_grav->value)
			gi.cvar_set("sv_gravity", "300");
		else
			gi.cvar_set("sv_gravity", "800");
	}
	else
		gi.cvar_set("sv_gravity", st.gravity);

	//prechaches

	PrecacheItem (FindItem ("Blaster"));
	PrecacheItem (FindItem ("Violator"));

	gi.soundindex ("player/lava1.wav");
	gi.soundindex ("player/lava2.wav");

	gi.soundindex ("misc/pc_up.wav");
	gi.soundindex ("misc/talk1.wav");

	gi.soundindex ("items/respawn1.wav");

	// sexed sounds
	gi.soundindex ("*death1.wav");
	gi.soundindex ("*death2.wav");
	gi.soundindex ("*death3.wav");
	gi.soundindex ("*death4.wav");
	gi.soundindex ("*fall1.wav");
	gi.soundindex ("*fall2.wav");
	gi.soundindex ("*gurp1.wav");		// drowning damage
	gi.soundindex ("*gurp2.wav");
	gi.soundindex ("*jump1.wav");		// player jump
	gi.soundindex ("*pain25_1.wav");
	gi.soundindex ("*pain25_2.wav");
	gi.soundindex ("*pain50_1.wav");
	gi.soundindex ("*pain50_2.wav");
	gi.soundindex ("*pain75_1.wav");
	gi.soundindex ("*pain75_2.wav");
	gi.soundindex ("*pain100_1.wav");
	gi.soundindex ("*pain100_2.wav");

	//-------------------

	gi.soundindex ("player/gasp1.wav");		// gasping for air
	gi.soundindex ("player/gasp2.wav");		// head breaking surface, not gasping

	gi.soundindex ("player/watr_in.wav");	// feet hitting water
	gi.soundindex ("player/watr_out.wav");	// feet leaving water

	gi.soundindex ("player/watr_un.wav");	// head going underwater

	gi.soundindex ("items/damage.wav");
	gi.soundindex ("items/protect.wav");
	gi.soundindex ("items/protect4.wav");
	gi.soundindex ("weapons/noammo.wav");

	gi.soundindex ("weapons/whoosh.wav");

	gi.soundindex ("misc/1frags.wav");
	gi.soundindex ("misc/2frags.wav");
	gi.soundindex ("misc/3frags.wav");
	gi.soundindex ("misc/one.wav");
	gi.soundindex ("misc/two.wav");
	gi.soundindex ("misc/three.wav");
	gi.soundindex ("misc/godlike.wav");
	gi.soundindex ("misc/rampage.wav");
	gi.soundindex ("misc/fight.wav");
	gi.soundindex ("misc/minderaser.wav");
	
    //if maxclients is 0 or 1, it means someone is probably just testing a map
    //or testing some code, so we really don't need to precache all this crap
    if (game.maxclients > 1)
    {
    	static const char *player_names[] = {
			"martianenforcer",
			"martiancyborg",
			"commander",
			"enforcer",
			"rustbot",
			"slashbot",
			"femborg",
			"martiancyborg",
			"martianoverlord",
			"martianwarrior"
		};
		int num_players = sizeof (player_names) / sizeof (player_names[0]);
		
		int player_num, item_num;
		char w_weap_prefix[MAX_QPATH];
		
		// loop through all the weapons and players, and index all the w_weap
		// models and taunts.
		for (player_num = 0; player_num < num_players + 1; player_num++)
		{
			// real players only:
			if (player_num < num_players)
			{
				// precache all base player taunts
				int taunt_num;
				for (taunt_num = 1; taunt_num <= 5; taunt_num++)
					gi.soundindex (va ("taunts/%s/taunt%d.wav", player_names[player_num], taunt_num));
				
				// Precache the base player model itself. This eliminates
				// "stutter" when a player joins the game. This should be here
				// for the cases of 7.45 clients and older, and for game modes
				// where unarmed combat is done (fists as a weapon.)
				gi.modelindex (va ("players/%s/tris.iqm", player_names[player_num]));
				
				Com_sprintf (w_weap_prefix, sizeof (w_weap_prefix), "players/%s/", player_names[player_num]);
				gi.modelindex (va ("%sweapon.iqm", w_weap_prefix));
			}
			else
			{
				// these are needed to insure that all clients have the same
				// ordering for their weap model indexes.
				Com_sprintf (w_weap_prefix, sizeof (w_weap_prefix), "#");
			}
			
			for (item_num = 1; item_num < game.num_items; item_num++)
			{
				if (itemlist[item_num].weapmodel != NULL)
					gi.modelindex (va ("%s%s", w_weap_prefix, itemlist[item_num].weapmodel));
			}
		}
		
	    //precache any gibs
	    sm_meat_index = gi.modelindex ("models/objects/gibs/sm_meat/tris.iqm");
	    gi.modelindex ("models/objects/gibs/mart_gut/tris.iqm");
	    gi.modelindex ("models/objects/debris1/tris.iqm");
		gi.modelindex ("models/objects/debris2/tris.iqm");
	    gi.modelindex ("models/objects/debris3/tris.iqm");

		//misc game models
		gi.modelindex ("models/weapons/v_minderaser/tris.iqm");
		gi.modelindex ("models/weapons/g_minderaser/tris.iqm");
		gi.modelindex ("models/objects/spud/tris.iqm");
		gi.modelindex ("models/objects/spider/tris.iqm");
		gi.modelindex ("models/objects/spider/helmet.iqm");
		gi.modelindex ("models/objects/blank/tris.iqm");
		gi.modelindex ("models/objects/electroball/tris.iqm");
		gi.modelindex ("models/items/activated/double/tris.iqm");
		gi.modelindex ("models/items/activated/double/glass.iqm");
		gi.modelindex ("models/items/activated/force/tris.iqm");
		gi.modelindex ("models/objects/dmspot/tris.iqm");
	}

//
// Setup light animation tables. 'a' is total darkness, 'z' is doublebright.
//

	// 0 normal
	gi.configstring(CS_LIGHTS+0, "m");

	// 1 FLICKER (first variety)
	gi.configstring(CS_LIGHTS+1, "mmnmmommommnonmmonqnmmo");

	// 2 SLOW STRONG PULSE
	gi.configstring(CS_LIGHTS+2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");

	// 3 CANDLE (first variety)
	gi.configstring(CS_LIGHTS+3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");

	// 4 FAST STROBE
	gi.configstring(CS_LIGHTS+4, "mamamamamama");

	// 5 GENTLE PULSE 1
	gi.configstring(CS_LIGHTS+5,"jklmnopqrstuvwxyzyxwvutsrqponmlkj");

	// 6 FLICKER (second variety)
	gi.configstring(CS_LIGHTS+6, "nmonqnmomnmomomno");

	// 7 CANDLE (second variety)
	gi.configstring(CS_LIGHTS+7, "mmmaaaabcdefgmmmmaaaammmaamm");

	// 8 CANDLE (third variety)
	gi.configstring(CS_LIGHTS+8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");

	// 9 SLOW STROBE (fourth variety)
	gi.configstring(CS_LIGHTS+9, "aaaaaaaazzzzzzzz");

	// 10 FLUORESCENT FLICKER
	gi.configstring(CS_LIGHTS+10, "mmamammmmammamamaaamammma");

	// 11 SLOW PULSE NOT FADE TO BLACK
	gi.configstring(CS_LIGHTS+11, "abcdefghijklmnopqrrqponmlkjihgfedcba");

	// styles 32-62 are assigned by the light program for switchable lights

	// 63 testing
	gi.configstring(CS_LIGHTS+63, "a");

	//call voting(we've started a new level, clear it)
	if(g_callvote->value) {
		playervote.called = false;
		playervote.yay = 0;
		playervote.nay = 0;
		playervote.command[0] = 0;
	}
}

