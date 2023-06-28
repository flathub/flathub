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

#define Function(f) {#f, f}

int red_team_score;
int blue_team_score;
int reddiff;
int bluediff;
int redwinning;
int print1, print2, print3;
int printT5, printT2, printT1;

mmove_t mmove_reloc;

field_t fields[] = {
	{"classname", FOFS(classname), F_LSTRING},
	{"model", FOFS(model), F_LSTRING},
	{"spawnflags", FOFS(spawnflags), F_INT},
	{"speed", FOFS(speed), F_FLOAT},
	{"accel", FOFS(accel), F_FLOAT},
	{"decel", FOFS(decel), F_FLOAT},
	{"target", FOFS(target), F_LSTRING},
	{"targetname", FOFS(targetname), F_LSTRING},
	{"pathtarget", FOFS(pathtarget), F_LSTRING},
	{"deathtarget", FOFS(deathtarget), F_LSTRING},
	{"killtarget", FOFS(killtarget), F_LSTRING},
	{"combattarget", FOFS(combattarget), F_LSTRING},
	{"message", FOFS(message), F_LSTRING},
	{"team", FOFS(team), F_LSTRING},
	{"wait", FOFS(wait), F_FLOAT},
	{"delay", FOFS(delay), F_FLOAT},
	{"random", FOFS(random), F_FLOAT},
	{"move_origin", FOFS(move_origin), F_VECTOR},
	{"move_angles", FOFS(move_angles), F_VECTOR},
	{"style", FOFS(style), F_INT},
	{"count", FOFS(count), F_INT},
	{"health", FOFS(health), F_INT},
	{"sounds", FOFS(sounds), F_INT},
	{"light", 0, F_IGNORE},
	{"dmg", FOFS(dmg), F_INT},
	{"mass", FOFS(mass), F_INT},
	{"volume", FOFS(volume), F_FLOAT},
	{"attenuation", FOFS(attenuation), F_FLOAT},
	{"map", FOFS(map), F_LSTRING},
	{"origin", FOFS(s.origin), F_VECTOR},
	{"angles", FOFS(s.angles), F_VECTOR},
	{"angle", FOFS(s.angles), F_ANGLEHACK},

	{"goalentity", FOFS(goalentity), F_EDICT, FFL_NOSPAWN},
	{"movetarget", FOFS(movetarget), F_EDICT, FFL_NOSPAWN},
	{"enemy", FOFS(enemy), F_EDICT, FFL_NOSPAWN},
	{"oldenemy", FOFS(oldenemy), F_EDICT, FFL_NOSPAWN},
	{"activator", FOFS(activator), F_EDICT, FFL_NOSPAWN},
	{"groundentity", FOFS(groundentity), F_EDICT, FFL_NOSPAWN},
	{"teamchain", FOFS(teamchain), F_EDICT, FFL_NOSPAWN},
	{"teammaster", FOFS(teammaster), F_EDICT, FFL_NOSPAWN},
	{"owner", FOFS(owner), F_EDICT, FFL_NOSPAWN},
	{"mynoise", FOFS(mynoise), F_EDICT, FFL_NOSPAWN},
	{"mynoise2", FOFS(mynoise2), F_EDICT, FFL_NOSPAWN},
	{"target_ent", FOFS(target_ent), F_EDICT, FFL_NOSPAWN},
	{"chain", FOFS(chain), F_EDICT, FFL_NOSPAWN},

	{"prethink", FOFS(prethink), F_FUNCTION, FFL_NOSPAWN},
	{"think", FOFS(think), F_FUNCTION, FFL_NOSPAWN},
	{"blocked", FOFS(blocked), F_FUNCTION, FFL_NOSPAWN},
	{"touch", FOFS(touch), F_FUNCTION, FFL_NOSPAWN},
	{"use", FOFS(use), F_FUNCTION, FFL_NOSPAWN},
	{"pain", FOFS(pain), F_FUNCTION, FFL_NOSPAWN},
	{"die", FOFS(die), F_FUNCTION, FFL_NOSPAWN},

	{"stand", FOFS(monsterinfo.stand), F_FUNCTION, FFL_NOSPAWN},
	{"idle", FOFS(monsterinfo.idle), F_FUNCTION, FFL_NOSPAWN},
	{"search", FOFS(monsterinfo.search), F_FUNCTION, FFL_NOSPAWN},
	{"walk", FOFS(monsterinfo.walk), F_FUNCTION, FFL_NOSPAWN},
	{"run", FOFS(monsterinfo.run), F_FUNCTION, FFL_NOSPAWN},
	{"dodge", FOFS(monsterinfo.dodge), F_FUNCTION, FFL_NOSPAWN},
	{"attack", FOFS(monsterinfo.attack), F_FUNCTION, FFL_NOSPAWN},
	{"melee", FOFS(monsterinfo.melee), F_FUNCTION, FFL_NOSPAWN},
	{"sight", FOFS(monsterinfo.sight), F_FUNCTION, FFL_NOSPAWN},
	{"checkattack", FOFS(monsterinfo.checkattack), F_FUNCTION, FFL_NOSPAWN},
	{"currentmove", FOFS(monsterinfo.currentmove), F_MMOVE, FFL_NOSPAWN},

	{"endfunc", FOFS(moveinfo.endfunc), F_FUNCTION, FFL_NOSPAWN},

	// temp spawn vars -- only valid when the spawn function is called
	{"lip", STOFS(lip), F_INT, FFL_SPAWNTEMP},
	{"distance", STOFS(distance), F_INT, FFL_SPAWNTEMP},
	{"height", STOFS(height), F_INT, FFL_SPAWNTEMP},
	{"noise", STOFS(noise), F_LSTRING, FFL_SPAWNTEMP},
	{"pausetime", STOFS(pausetime), F_FLOAT, FFL_SPAWNTEMP},
	{"item", STOFS(item), F_LSTRING, FFL_SPAWNTEMP},

//need for item field in edict struct, FFL_SPAWNTEMP item will be skipped on saves
	{"item", FOFS(item), F_ITEM},

	{"gravity", STOFS(gravity), F_LSTRING, FFL_SPAWNTEMP},
	{"sky", STOFS(sky), F_LSTRING, FFL_SPAWNTEMP},
	{"skyrotate", STOFS(skyrotate), F_FLOAT, FFL_SPAWNTEMP},
	{"skyaxis", STOFS(skyaxis), F_VECTOR, FFL_SPAWNTEMP},
	{"minyaw", STOFS(minyaw), F_FLOAT, FFL_SPAWNTEMP},
	{"maxyaw", STOFS(maxyaw), F_FLOAT, FFL_SPAWNTEMP},
	{"minpitch", STOFS(minpitch), F_FLOAT, FFL_SPAWNTEMP},
	{"maxpitch", STOFS(maxpitch), F_FLOAT, FFL_SPAWNTEMP},
	{"nextmap", STOFS(nextmap), F_LSTRING, FFL_SPAWNTEMP},

	{0, 0, 0, 0}

};

field_t		levelfields[] =
{
	{"changemap", LLOFS(changemap), F_LSTRING},

	{"sight_client", LLOFS(sight_client), F_EDICT},
	{"sight_entity", LLOFS(sight_entity), F_EDICT},
	{"sound_entity", LLOFS(sound_entity), F_EDICT},
	{"sound2_entity", LLOFS(sound2_entity), F_EDICT},

	{NULL, 0, F_INT}
};

field_t		clientfields[] =
{
	{"pers.weapon", CLOFS(pers.weapon), F_ITEM},
	{"pers.lastweapon", CLOFS(pers.lastweapon), F_ITEM},
	{"newweapon", CLOFS(newweapon), F_ITEM},

	{NULL, 0, F_INT}
};

/*
============
InitGame

This will be called when the dll is first loaded, which
only happens when a new game is started or a save game
is loaded.
============
*/
void InitGame (void)
{
	gi.dprintf ("==== InitGame ====\n");

	gun_x = gi.cvar ("gun_x", "0", 0);
	gun_y = gi.cvar ("gun_y", "0", 0);
	gun_z = gi.cvar ("gun_z", "0", 0);

	//FIXME: sv_ prefix is wrong for these
	sv_rollspeed = gi.cvar ("sv_rollspeed", "200", 0);
	sv_rollangle = gi.cvar ("sv_rollangle", "2", 0);
	sv_maxvelocity = gi.cvar ("sv_maxvelocity", "2000", 0);
	sv_gravity = gi.cvar ("sv_gravity", "800", 0);

	// noset vars
	g_dedicated = gi.cvar ("dedicated", "0", CVAR_NOSET);

	// latched vars
	sv_cheats = gi.cvar ("cheats", "0", CVAR_SERVERINFO | CVAR_LATCH);
	// gi.cvar ("gamename", GAMEVERSION , CVAR_SERVERINFO | CVAR_LATCH);
	gi.cvar ("gamedate", __DATE__ , CVAR_SERVERINFO | CVAR_LATCH);

	g_maxclients = gi.cvar ("maxclients", "1", CVAR_SERVERINFO | CVAR_LATCH | CVARDOC_INT);
	maxspectators = gi.cvar ("maxspectators", "4", CVAR_SERVERINFO | CVARDOC_INT);

	deathmatch = gi.cvar ("deathmatch", "0", CVAR_LATCH | CVARDOC_BOOL);
	ctf = gi.cvar ("ctf", "0", CVAR_LATCH | CVAR_GAMEINFO | CVARDOC_BOOL);
	g_duel = gi.cvar ("g_duel", "0", CVAR_LATCH | CVAR_GAMEINFO | CVARDOC_BOOL);
	g_tactical = gi.cvar ("g_tactical", "0", CVAR_LATCH | CVAR_GAMEINFO | CVARDOC_BOOL);

	skill = gi.cvar ("skill", "1", CVAR_LATCH | CVARDOC_INT);
	maxentities = gi.cvar ("maxentities", "1024", CVAR_LATCH | CVARDOC_INT);
	sv_botkickthreshold = gi.cvar("sv_botkickthreshold", "0", CVAR_LATCH | CVARDOC_INT);
	sv_custombots = gi.cvar("sv_custombots", "0", CVAR_LATCH | CVARDOC_INT);
	gi.cvar_describe (sv_custombots, "0 uses default botfile. Any other value selects a botfile of the form botinfo/custom<value>.tmp.");
	sv_tickrate = gi.cvar("sv_tickrate", "10", CVAR_SERVERINFO | CVAR_LATCH | CVARDOC_INT);

	// To output a game report after a match (only on dedicated servers)
	sv_gamereport = gi.cvar("sv_gamereport", "1", CVAR_SERVERINFO | CVARDOC_BOOL);

	//mutator
	instagib = gi.cvar ("instagib", "0", CVAR_LATCH | CVAR_GAMEINFO | CVARDOC_BOOL);
	rocket_arena = gi.cvar ("rocket_arena", "0", CVAR_LATCH | CVAR_GAMEINFO | CVARDOC_BOOL);
	insta_rockets = gi.cvar ("insta_rockets", "0", CVAR_LATCH | CVAR_GAMEINFO | CVARDOC_BOOL);
	all_out_assault = gi.cvar ("all_out_assault", "0", CVAR_LATCH | CVAR_GAMEINFO | CVARDOC_BOOL);
	low_grav = gi.cvar ("low_grav", "0", CVAR_LATCH | CVAR_GAMEINFO | CVARDOC_BOOL);
	regeneration = gi.cvar ("regeneration", "0", CVAR_LATCH | CVAR_GAMEINFO | CVARDOC_BOOL);
	vampire = gi.cvar ("vampire", "0", CVAR_LATCH | CVAR_GAMEINFO | CVARDOC_BOOL);
	excessive = gi.cvar ("excessive", "0", CVAR_LATCH | CVAR_GAMEINFO | CVARDOC_BOOL);
	grapple = gi.cvar ("grapple", "0", CVAR_LATCH | CVAR_GAMEINFO | CVARDOC_BOOL);
	classbased = gi.cvar ("classbased", "0", CVAR_LATCH | CVAR_GAMEINFO | CVARDOC_BOOL);

	g_losehealth = gi.cvar ("g_losehealth", "1", CVAR_LATCH | CVARDOC_BOOL);
	g_losehealth_num = gi.cvar ("g_losehealth_num", "100", CVAR_LATCH | CVARDOC_INT);

	//weapons
	wep_selfdmgmulti = gi.cvar("wep_selfdmgmulti", "1.0", CVARDOC_FLOAT);

	//health/max health/max ammo
	g_spawnhealth = gi.cvar("g_spawnhealth", "125", CVARDOC_INT);
	g_maxhealth = gi.cvar("g_maxhealth", "100", CVARDOC_INT);
	
#define X(name,itname,base,max,excessivemult)			\
	g_max##name = gi.cvar ("g_max" #name, #max, CVARDOC_INT);

	AMMO_TYPES

	#undef X

	//quick weapon change
	quickweap = gi.cvar ("quickweap", "0", CVAR_LATCH | CVAR_GAMEINFO | CVARDOC_INT);
	gi.cvar_describe (quickweap, "fast weapon switching.");

	//anti-camp
	anticamp = gi.cvar("anticamp", "0", CVAR_LATCH | CVAR_GAMEINFO | CVARDOC_BOOL);
	camptime = gi.cvar("camptime", "10", CVAR_LATCH | CVARDOC_INT);
	ac_frames = gi.cvar("ac_frames", "0", CVAR_LATCH | CVARDOC_INT);
	ac_threshold = gi.cvar("ac_threshold", "0", CVAR_LATCH | CVARDOC_INT);

	//random quad
	g_randomquad = gi.cvar("g_randomquad", "0", CVAR_LATCH | CVARDOC_BOOL);

	//warmup time
	if (g_maxclients->integer > 1)
		warmuptime = gi.cvar("warmuptime", "30", CVAR_LATCH | CVARDOC_INT);
	else
		// You can override this default if you want to, but a default of 0 is
		// nice if you're just loading a map to test code or to test the map 
		// itself, over and over again.
		warmuptime = gi.cvar("warmuptime", "0", CVAR_LATCH | CVARDOC_INT);

	//spawn protection
	g_spawnprotect = gi.cvar("g_spawnprotect", "2", CVAR_SERVERINFO | CVARDOC_INT);
	gi.cvar_describe (g_spawnprotect, "number of seconds of spawn protection.");

	//joust mode
	joustmode = gi.cvar("sv_joustmode", "0", CVAR_SERVERINFO | CVAR_GAMEINFO | CVARDOC_BOOL);
	gi.cvar_describe (joustmode, "players can jump in midair.");

	//map voting
	g_mapvote = gi.cvar("g_mapvote", "0", CVAR_SERVERINFO);
	g_voterand = gi.cvar("g_voterand", "0", 0);
	g_votemode = gi.cvar("g_votemode", "0", 0);
	g_votesame = gi.cvar("g_votesame", "1", 0);

	//call voting
	g_callvote = gi.cvar("g_callvote", "1", CVAR_SERVERINFO | CVARDOC_BOOL); //change after testing

	//forced autobalanced teams
	g_autobalance = gi.cvar("g_autobalance", "0", 0);

	//reward points threshold
	g_reward = gi.cvar("g_reward", "20", CVAR_SERVERINFO);

	//antilag
	g_antilagprojectiles = gi.cvar ("g_antilagprojectiles", "1", CVAR_GAMEINFO | CVARDOC_BOOL);
	g_antilagdebug = gi.cvar("g_antilagdebug", "0", 0 /*CVAR_SERVERINFO*/);

	// change anytime vars
	dmflags = gi.cvar ("dmflags", "0", CVAR_SERVERINFO);
	fraglimit = gi.cvar ("fraglimit", "0", CVAR_SERVERINFO);
	timelimit = gi.cvar ("timelimit", "0", CVAR_SERVERINFO);
	password = gi.cvar ("password", "", CVAR_USERINFO);
	spectator_password = gi.cvar ("spectator_password", "", CVAR_USERINFO);
	needpass = gi.cvar ("needpass", "0", CVAR_SERVERINFO);
	filterban = gi.cvar ("filterban", "1", 0);
	motdfile = gi.cvar ("motdfile", "motd.txt", 0);
	motdforce = gi.cvar ("motdforce", "0", 0);

	g_select_empty = gi.cvar ("g_select_empty", "0", CVAR_ARCHIVE);
	
	g_dmlights = gi.cvar ("g_dm_lights", "1", CVAR_ARCHIVE | CVAR_GAMEINFO);

	run_pitch = gi.cvar ("run_pitch", "0.002", 0);
	run_roll = gi.cvar ("run_roll", "0.005", 0);
	bob_up  = gi.cvar ("bob_up", "0.005", 0);
	bob_pitch = gi.cvar ("bob_pitch", "0.002", 0);
	bob_roll = gi.cvar ("bob_roll", "0.002", 0);

	g_background_music = gi.cvar ("background_music", "1", CVAR_ARCHIVE);

	// flood control
	flood_msgs = gi.cvar ("flood_msgs", "4", 0);
	flood_persecond = gi.cvar ("flood_persecond", "4", 0);
	flood_waitdelay = gi.cvar ("flood_waitdelay", "10", 0);

	// dm map list
	sv_maplist = gi.cvar ("sv_maplist", "", 0);
	
	//throwaway cvars
	gi.cvar ("g_teamgame", va("%d", TEAM_GAME), CVAR_SERVERINFO);

	// items
	InitItems ();
	
	//set this if you're running experimental code on your server
	gi.cvar("testcode", "0", CVAR_GAMEINFO | CVAR_ROM); 
	//set this in your config if you're testing an unfinished map
	gi.cvar("testmap", "0", CVAR_GAMEINFO);

	Com_sprintf (game.helpmessage1, sizeof(game.helpmessage1), "");

	Com_sprintf (game.helpmessage2, sizeof(game.helpmessage2), "");

	// initialize all entities for this game
	game.maxentities = maxentities->integer;
	g_edicts =  gi.TagMalloc (game.maxentities * sizeof(g_edicts[0]), TAG_GAME);
	globals.edicts = g_edicts;
	globals.max_edicts = game.maxentities;

	// initialize all clients for this game
	game.maxclients = g_maxclients->integer;
	game.clients = gi.TagMalloc (game.maxclients * sizeof(game.clients[0]), TAG_GAME);
	globals.num_edicts = game.maxclients+1;

	red_team_score = 0;
	blue_team_score = 0;

	if(sv_tickrate->integer > 120)
	{
		safe_bprintf(PRINT_HIGH, "Server rate too high, clamping at 120fps\n");
		FRAMETIME = 1.0/120.0;
		gi.cvar_set("sv_tickrate", "120");
	}
	else if(sv_tickrate->integer < 10)
	{
		safe_bprintf(PRINT_HIGH, "Server rate too low, clamping at 10fps\n");
		FRAMETIME = 1.0/10.0;
		gi.cvar_set("sv_tickrate", "10");
	}
	else
		FRAMETIME = 1.0/(float)sv_tickrate->integer;	
	
	FRAMETIME_MS = (int) (1000 * FRAMETIME);
	NUM_CLIENT_HISTORY_FOR_CURRENT_TICKRATE = (int) ((NUM_CLIENT_HISTORY - 1) / (120.0 * FRAMETIME));


	if(g_tactical->integer)
	{
		tacticalScore.alienAmmoDepot = 
			tacticalScore.alienComputer = 
			tacticalScore.alienPowerSource = 
			tacticalScore.alienBackupGen = 
			tacticalScore.humanAmmoDepot = 
			tacticalScore.humanComputer = 
			tacticalScore.humanPowerSource =
			tacticalScore.humanBackupGen = 
			true;

		tacticalScore.alienAmmoDepotHealth = 
			tacticalScore.alienComputerHealth = 
			tacticalScore.alienPowerSourceHealth = 
			tacticalScore.humanAmmoDepotHealth = 
			tacticalScore.humanComputerHealth = 
			tacticalScore.humanPowerSourceHealth =
			100;
	}

	//reset minderaser and jetpacks
	mindEraserTime = jetpackTime = level.time;	

	print1 = print2 = print3 = printT5 = printT2 = printT1= false;

}

