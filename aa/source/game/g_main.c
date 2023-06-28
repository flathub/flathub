/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2010 COR Entertainment, LLC.

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

#if defined HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#if !defined HAVE__STRDUP
#define _strdup strdup
#endif

#include "g_local.h"

game_locals_t	game;
level_locals_t	level;
game_import_t	gi;
game_export_t	globals;
spawn_temp_t	st;
g_vote_t		playervote;
tactical_t		tacticalScore;

int	sm_meat_index;
int meansOfDeath;
int bot_won;

char *winningmap;

edict_t		*g_edicts;

//game modes
cvar_t	*deathmatch;
cvar_t  *ctf;
cvar_t	*g_tactical;
cvar_t	*g_duel;

//mutators
cvar_t  *instagib;
cvar_t  *rocket_arena;
cvar_t  *insta_rockets;
cvar_t  *all_out_assault;
cvar_t  *low_grav;
cvar_t  *regeneration;
cvar_t  *vampire;
cvar_t  *excessive;
cvar_t  *grapple;
cvar_t  *classbased;

cvar_t	*g_losehealth;
cvar_t	*g_losehealth_num;

//weapons
cvar_t	*wep_selfdmgmulti;

//health/max health
cvar_t	*g_spawnhealth;
cvar_t	*g_maxhealth;

//quick weapon change
cvar_t  *quickweap;

//anti-camp
cvar_t  *anticamp;
cvar_t  *camptime;

//show player lights for visibility even in non-team games
cvar_t	*g_dmlights;

/** @brief Anti-camp frames
 *
 * This CVar controls the amount of frames velocity is accumulated for when
 * trying to determine if a player is camping. Its valid range is 1-100,
 * anything outside this range will default it to the value of the
 * G_ANTICAMP_FRAMES constant.
 *
 * @note If the value is 1, the anti-camp will work exactly as it used to.
 *
 * @note This CVar's value should be high enough for the anti-camp to work
 *	correctly (the higher it is, the more the system "remembers" about
 *	players' previous velocities), but low enough and for it not to
 *	punish players for camping long after they've resumed moving.
 */
cvar_t	*ac_frames;

/** @brief Anti-camp threshold
 *
 * This CVar contains the speed below which the timeout to "suicide" is no
 * longer updated. It will default to G_ANTICAMP_THRESHOLD if its value is
 * not in the ]0;500] range.
 *
 * @note In excessive mode, this value is multiplied by 1.5
 */
cvar_t	*ac_threshold;

//random quad
cvar_t  *g_randomquad;

//warmup
cvar_t  *warmuptime;

//spawn protection
cvar_t  *g_spawnprotect;

//joust mode
cvar_t  *joustmode;

//map voting
cvar_t	*g_mapvote;
cvar_t	*g_voterand;
cvar_t	*g_votemode;
cvar_t	*g_votesame;

//call voting
cvar_t	*g_callvote;

//reward point threshold
cvar_t	*g_reward;

//force autobalanced teams
cvar_t	*g_autobalance;

cvar_t	*dmflags;
cvar_t	*skill;
cvar_t	*fraglimit;
cvar_t	*timelimit;
cvar_t	*password;
cvar_t	*spectator_password;
cvar_t	*needpass;
cvar_t	*g_maxclients;
cvar_t	*maxspectators;
cvar_t	*maxentities;
cvar_t	*g_select_empty;
cvar_t	*g_dedicated;
cvar_t	*motdfile;

/** @brief CVar that forces MOTD display
 *
 * This CVar should contain an integer, which will indicate how client frames
 * the MOTD should be forced on for. If it is 0, then the MOTD will not be
 * forced on.
 */
cvar_t	*motdforce;

cvar_t	*filterban;

cvar_t	*sv_maxvelocity;
cvar_t	*sv_gravity;

cvar_t	*sv_rollspeed;
cvar_t	*sv_rollangle;
cvar_t	*gun_x;
cvar_t	*gun_y;
cvar_t	*gun_z;

cvar_t	*run_pitch;
cvar_t	*run_roll;
cvar_t	*bob_up;
cvar_t	*bob_pitch;
cvar_t	*bob_roll;

cvar_t	*sv_cheats;

cvar_t	*flood_msgs;
cvar_t	*flood_persecond;
cvar_t	*flood_waitdelay;

cvar_t	*sv_maplist;

cvar_t  *g_background_music;

cvar_t  *sv_botkickthreshold;
cvar_t  *sv_custombots;

cvar_t  *sv_tickrate;
float	FRAMETIME;		// FRAMETIME in seconds, SPF
int		FRAMETIME_MS; 	// FRAMETIME in milliseconds, as int
int		NUM_CLIENT_HISTORY_FOR_CURRENT_TICKRATE;
cvar_t  *sv_gamereport;

//unlagged
cvar_t	*g_antilagdebug;
cvar_t	*g_antilagprojectiles;

void SpawnEntities (char *mapname, const char *entities, char *spawnpoint);
void ClientThink (edict_t *ent, usercmd_t *cmd);
qboolean ClientConnect (edict_t *ent, char *userinfo);
void ClientUserinfoChanged (edict_t *ent, char *userinfo, int whereFrom);
void ClientDisconnect (edict_t *ent);
void ClientBegin (edict_t *ent);
void ClientCommand (edict_t *ent);
void RunEntity (edict_t *ent);
void InitGame (void);
void G_RunFrame (void);
int ACESP_FindBotNum(void);
extern long filelength(int);
extern void ED_CallSpawn (edict_t *ent);
extern void G_Ban( char *ip );

static size_t szr;

static int g_filelength( FILE *f )
{
	int length = -1;

#if defined HAVE_FILELENGTH

	length = filelength( fileno( f ) );

#elif defined HAVE_FSTAT

	struct stat statbfr;
	int result;

	result = fstat( fileno(f), &statbfr );
	if ( result != -1 )
	{
		length = (int)statbfr.st_size;
	}

#else

	long int pos;

	pos = ftell( f );
	fseek( f, 0L, SEEK_END );
	length = ftell( f );
	fseek( f, pos, SEEK_SET );

#endif

	return length;
}


//===================================================================


void ShutdownGame (void)
{
	gi.dprintf ("==== ShutdownGame ====\n");

	gi.FreeTags (TAG_LEVEL);
	gi.FreeTags (TAG_GAME);
}

/*
=============
G_ForceExitIntermission

To prevent those annoying occurrences when you join a server which has been 
idling at intermission for a long time, then it instantly switches maps as 
soon as the old one is done loading.

Now new player joins will force intermission to end after twenty seconds, even
if there are still players in the server.
=============
*/
void ExitLevel (void);
void G_ForceExitIntermission (void)
{
	if (level.time < level.intermissiontime + 20.0 || !level.intermissiontime)
		return;
	ExitLevel();
}

/*
=================
GetGameAPI

Returns a pointer to the structure with all entry points
and global variables
=================
*/
game_export_t *GetGameAPI (game_import_t *import)
{
	gi = *import;

	globals.apiversion = GAME_API_VERSION;
	globals.Init = InitGame;
	globals.Shutdown = ShutdownGame;
	globals.SpawnEntities = SpawnEntities;

	globals.ClientThink = ClientThink;
	globals.ClientConnect = ClientConnect;
	globals.ClientUserinfoChanged = ClientUserinfoChanged;
	globals.ClientDisconnect = ClientDisconnect;
	globals.ClientBegin = ClientBegin;
	globals.ClientCommand = ClientCommand;
	globals.ACESP_FindBotNum = ACESP_FindBotNum;

	globals.RunFrame = G_RunFrame;
	globals.ForceExitIntermission = G_ForceExitIntermission;

	globals.ServerCommand = ServerCommand;

	globals.edict_size = sizeof(edict_t);

	return &globals;
}


#define Z_MAGIC		0x1d1d

typedef struct zhead_s
{
	struct	zhead_s  *prev, *next;
	short   magic;
	short   tag;
	int	size;
} zhead_t;

zhead_t		z_chain;
int		z_count, z_bytes;


/*
=================
ClientEndServerFrames
=================
*/
void ClientEndServerFrames (void)
{
	int		i;
	edict_t	*ent;

	/* make sure bot info in first client is up to date */
	ACESP_UpdateBots();

	// calc the player views now that all pushing
	// and damage has been added
	for (i=0 ; i<g_maxclients->value ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse || !ent->client)
			continue;
		ClientEndServerFrame (ent);
	}

}

/*
=================
CreateTargetChangeLevel

Returns the created target changelevel
=================
*/
edict_t *CreateTargetChangeLevel(char *map)
{
	edict_t *ent;

	ent = G_Spawn ();
	ent->classname = "target_changelevel";
	Com_sprintf(level.nextmap, sizeof(level.nextmap), "%s", map);
	ent->map = level.nextmap;
	return ent;
}


// Note: This function returns a pointer to a substring of the original string.
// If the given string was allocated dynamically, the caller must not overwrite
// that pointer with the returned value, since the original pointer must be
// deallocated using the same allocator with which it was allocated.  The return
// value must NOT be deallocated using free() etc.
char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

/*
** Returns a copy of the original string with all color codes removed.
*/
char *StripColorCodes(char *str)
{
	static char copy[MAX_OSPATH];
	int count = 0;

	while (*str)
	{
		if ( *str == '^' && str[1] )
		{
			str += 2;
			continue;
		}
		copy[count] = *str;
		count++;
		str++;
	}
	copy[count] = '\0';

	return copy;
}

/*
** Returns a copy of the original string with invalid path/file characters 
** and other unwanted characters replaced with underscores.
*/
char *ReplaceIllegalChars(char *str)
{
	static const char allowedChars[62] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	int i;
	static char copy[MAX_OSPATH];

	strcpy(copy, str);

	for (i = 0; i < strlen(copy); i++)
	{
		if (strchr(allowedChars, copy[i]) == NULL)
		{
			copy[i] = '_';
		}
	}
	return copy;
}

extern char *Cvar_VariableString (const char *var_name);

/*
 * Log scores and stats into a game report file on the server in json format.
 * Bots will be skipped.
 *
 *--- weapon_hit[] incremented ---
 * 0 : blasterball_touch  : Weapon_Beamgun, Weapon_Blaster
 * 0 : fire_blaster_beam  : Weapon_Beamgun, Weapon_Blaster, Weapon_Alienblaster
 * 1 : fire_disruptor     : Weapon_Disruptor
 * 2 : smartgrenade_think : Weapon_Smartgun
 * 3 : fire_lead          : Weapon_Chain
 * 4 : fireball_touch     : Weapon_Flame
 * 4 : flame_touch        : Weapon_Flame
 * 5 : rocket_touch       : Weapon_RocketLauncher
 * 6 : fire_blaster       : Weapon_Beamgun, Weapon_Blaster
 * 7 : bomb_touch         : Weapon_Bomber, Weapon_Vaporizer
 * 7 : fire_vaporizer     : Weapon_Vaporizer
 * 8 : fire_violator      : Weapon_Violator
 */
static void game_report( void )
{
	static const char* weapname[] =
	{
		"blaster",        // 0 
		"disruptor",      // 1
		"smartgun",       // 2
		"chaingun",       // 3
		"flamethrower",   // 4
		"rocketlauncher", // 5
		"beamgun",        // 6 
		"vaporizer",      // 7
		"violator"        // 8
	};
	static const int MAX_HOSTNAME = 27;

	edict_t *pclient;
	int i;
	char *client_name;
	int	count;
	int	index[256];
	
	char jsonfilename[MAX_OSPATH];
	char fullpath[MAX_OSPATH];
	FILE *jsonfile;

	time_t t = time(NULL);
	struct tm tm = *gmtime(&t);

	char *hostname = Cvar_VariableString("hostname");
	char *sanitized_hostname = ReplaceIllegalChars(StripColorCodes(hostname));
	char title[49];

	if (sanitized_hostname != NULL && *sanitized_hostname != '\0' && strlen(sanitized_hostname) > 0)
	{
		// The file name may not be longer than 63 characters, gi.FullWritePath() will fail.
		// So truncate the host name part in case it's longer than 27.
		if (strlen(sanitized_hostname) > MAX_HOSTNAME)
		{
			sanitized_hostname[MAX_HOSTNAME] = '\0';
		}
		sprintf(title, "_%s", sanitized_hostname);
	}
	else
	{
		strcpy(title, "");
	}

	Com_sprintf(jsonfilename, sizeof(jsonfilename), "gamereport_%04d-%02d-%02d_%02d.%02d.%02d%s.json",
				tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
				title);

	gi.FullWritePath(fullpath, sizeof(fullpath), jsonfilename);

	jsonfile = fopen(fullpath, "w");
	if (!jsonfile) {
		Com_Printf("ERROR: couldn't open %s.\n", fullpath);
		return;
	}

	count = 0;
	for ( i = 0; i < game.maxclients; i++ )
	{
		pclient = g_edicts + 1 + i;
		if (pclient->inuse && player_participating (pclient) && !pclient->is_bot)
		{
			index[count] = i;
			count++;
		}
	}
	
	// sort by frags descending
	qsort (index, count, sizeof(index[0]), G_PlayerSortDescending);

	fprintf(jsonfile, "{\n");
	fprintf(jsonfile, "\t\"map\": \"%s\",\n", level.mapname);
	fprintf(jsonfile, "\t\"players\": [\n");

	for ( i = 0; i < count; i++ )
	{ 	
		int weap;
		qboolean weaponSkillPrinted = false;

		pclient = g_edicts + 1 + index[i];
		if (pclient->inuse && player_participating (pclient) && !pclient->is_bot)
		{						
			int ratio = 0;
			if (game.clients[index[i]].resp.deaths != 0)
			{
				ratio = 100 * game.clients[index[i]].resp.score / game.clients[index[i]].resp.deaths;
			}

			/* scoring */
			client_name = Info_ValueForKey( pclient->client->pers.userinfo, "name" );

			fprintf(jsonfile, "\t\t{\n");
			fprintf(jsonfile, "\t\t\t\"name\": \"%s\",\n", trimwhitespace(client_name));
			fprintf(jsonfile, "\t\t\t\"score\": %i,\n", game.clients[index[i]].resp.score);
			fprintf(jsonfile, "\t\t\t\"deaths\": %i,\n", game.clients[index[i]].resp.deaths);
			fprintf(jsonfile, "\t\t\t\"ratio\": %i,\n", ratio);
			
			/* weapon skill */
			fprintf(jsonfile, "\t\t\t\"weapon_skill\": [\n");
			for ( weap = 0; weap < 9; weap++ )
			{
				if ( pclient->client->resp.weapon_shots[weap] != 0 )
				{
					int hits  = pclient->client->resp.weapon_hits[weap];
					int shots = pclient->client->resp.weapon_shots[weap];
					int percent = 100 * hits / shots;
				
					if (weaponSkillPrinted)
					{
						fprintf(jsonfile, ",\n");
					}
					fprintf(jsonfile, "\t\t\t\t{\"weapon\": \"%s\", \"hits\": %i, \"shots\": %i, \"accuracy\": %i}",
							weapname[weap], hits, shots, percent);				
					weaponSkillPrinted = true;
				}
			}
			fprintf(jsonfile, "\n\t\t\t]\n");

			if (i < count - 1)
			{
				fprintf(jsonfile, "\t\t},\n");
			}
			else
			{
				fprintf(jsonfile, "\t\t}\n");
			}
		}
	}
	fprintf(jsonfile, "\t]\n");
	fprintf(jsonfile, "}\n");
	fclose(jsonfile);
}


/*
=================
EndDMLevel

The timelimit or fraglimit has been exceeded
=================
*/
void EndDMLevel (void)
{
	edict_t		*ent;
	char *s, *t, *f;
	const char *buf_cursor;
	static const char *seps = " ,\n\r";
	char *buffer;
	char  mapsname[MAX_OSPATH];
	int length;
	static char **mapnames;
	static int	  nummaps;
	int i;
	FILE *fp;

	// Output game scoring and statistics into a game report in json format.
	if ( g_dedicated && g_dedicated->integer != 0 && sv_gamereport->value )
	{
		game_report();
	}

	/* Search for dead players in order to remove DeathCam and free mem */
	for (i=0 ; i<g_maxclients->value ; i++)
	{
		ent = g_edicts + i + 1;
		if (!ent->inuse || !player_participating (ent))
			continue;
		if(!ent->is_bot && ent->deadflag)
			DeathcamRemove (ent, "off");
	}

	//call voting
	if(g_callvote->value) 
	{
		playervote.called = false;
		playervote.yay = 0;
		playervote.nay = 0;
		playervote.command[0] = 0;
	}

	//map voting
	if(g_mapvote->value) 
	{
		level.changemap = level.mapname;

		// initialise map list using the current map's name
		// this is done in all modes just in case
		for ( i = 0 ; i < 4 ; i ++ )
		{
			strcpy(votedmap[i].mapname, level.mapname);
			votedmap[i].tally = 0;
		}

		// if there is a map list, time to choose
		if ( sv_maplist && sv_maplist->string && *(sv_maplist->string) )
		{
			char * names[200]; // 200 map names should be fine
			int n_maps = 0;
			int same_index = -1;
			int mode = ( g_votemode ? g_votemode->value : 0 );
			qboolean same = ( g_votesame ? (g_votesame->value == 1) : 1 );

			memset( names, 0 , sizeof(names) );
			s = _strdup( sv_maplist->string );
			t = strtok( s, seps );
			do {
				// if using the same map is disallowed, skip it
				if ( !Q_strcasecmp(t, level.mapname) )
				{
					if ( same_index == -1 )
						same_index = n_maps;
					if ( !same )
						continue;
				}

				// avoid duplicates
				for ( i = 0 ; i < n_maps ; i ++ )
				{
					if ( ! Q_strcasecmp( t , names[i] ) )
						break;
				}
				if ( i < n_maps )
					continue;

				names[n_maps ++] = t;
			} while ( (t = strtok( NULL, seps)) != NULL );

			if ( n_maps > 0 )
			{
				// if the map list has been changed and the
				// current map is no longer in it, prevent
				// screw-ups
				if ( same_index == -1 )
					same_index = 0;

				if ( mode == 0 )
				{
					// standard vote mode, take the next
					// 4 maps from the list
					for ( i = 0 ; i < 4 ; i ++ )
						strcpy( votedmap[i].mapname, names[(same_index + i) % n_maps] );
				}
				else
				{
					// random selection
					for ( i = 0 ; i < 4 && n_maps > 0 ; i ++ )
					{
						int map = random() * (n_maps - 1);
						strcpy( votedmap[i].mapname, names[map] );
						names[map] = names[n_maps - 1];
						n_maps --;
					}
				}
			}

			free(s);
		}
	}

	// stay on same level flag
	if (dmflags->integer & DF_SAME_LEVEL)
	{
		BeginIntermission (CreateTargetChangeLevel (level.mapname) );
		return;
	}

	if (bot_won && !(dmflags->integer & DF_BOT_LEVELAD) && !ctf->value)
	{
		BeginIntermission (CreateTargetChangeLevel (level.mapname) );
		return;
	}

	if(ctf->integer && !g_dedicated->integer)
	{ //ctf will just stay on same level unless specified by dedicated list
		BeginIntermission (CreateTargetChangeLevel (level.mapname));
		return;
	}
	// see if it's in the map list
	if (*sv_maplist->string) {
		s = _strdup(sv_maplist->string);
		f = NULL;
		t = strtok(s, seps);
		while (t != NULL) {
			if (Q_strcasecmp(t, level.mapname) == 0) {
				// it's in the list, go to the next one
				t = strtok(NULL, seps);
				if (t == NULL) { // end of list, go to first one
					if (f == NULL) // there isn't a first one, same level
						BeginIntermission (CreateTargetChangeLevel (level.mapname) );
					else
						BeginIntermission (CreateTargetChangeLevel (f) );
				} else
					BeginIntermission (CreateTargetChangeLevel (t) );
				free(s);
				return;
			}
			if (!f)
				f = t;
			t = strtok(NULL, seps);
		}
		free(s);
	}

	if(ctf->integer) { //wasn't in the dedicated list
		BeginIntermission (CreateTargetChangeLevel (level.mapname));
		return;
	}

    //check the maps.lst file and read in those, which will overide anything in the level
	//write this code here

	/*
	** load the list of map names
	*/
	if ( !gi.FullPath( mapsname, sizeof(mapsname), "maps.lst" ) )
	{ // no maps.lst.
		// note: originally this was looked for in "./data1/" only
		//   hope it is ok to use FullPath() search path.
		BeginIntermission (CreateTargetChangeLevel (level.mapname) );
		return;
	}
	if ( ( fp = fopen( mapsname, "rb" ) ) == 0 )
	{
		BeginIntermission (CreateTargetChangeLevel (level.mapname) );
			return;
	}

	length = g_filelength( fp );
	buffer = malloc( length + 1 );
	szr = fread( buffer, length, 1, fp );
	buffer[length] = 0;

	buf_cursor = buffer;

	i = 0;
	while ( i < length )
	{
		if (buf_cursor[i] == '\r')
			nummaps++;
		i++;
	}

	mapnames = malloc( sizeof( char * ) * ( nummaps + 1 ) );
	memset( mapnames, 0, sizeof( char * ) * ( nummaps + 1 ) );

	buf_cursor = buffer;

	for ( i = 0; i < nummaps; i++ )
	{
		char  shortname[MAX_TOKEN_CHARS];
		char  longname[MAX_TOKEN_CHARS];
		char  scratch[200];
#if defined WIN32_VARIANT
		int  j;
#endif
		int l;

		strcpy (shortname, COM_Parse (&buf_cursor));
		l = strlen(shortname);
#if defined WIN32_VARIANT
		for (j=0 ; j<l ; j++)
			shortname[j] = toupper(shortname[j]);
#endif
		strcpy (longname, COM_Parse (&buf_cursor));
		Com_sprintf( scratch, sizeof( scratch ), "%s", shortname );

		mapnames[i] = malloc( strlen( scratch ) + 1 );
		strcpy( mapnames[i], scratch );
	}
	mapnames[nummaps] = 0;

	fclose(fp);
	free( buffer );

	//find map, goto next map - if one doesn't exist, repeat list
	//stick something in here to filter out CTF, and just make it loop back
	for (i = 0; i < nummaps; i++) {
		if (Q_strcasecmp(mapnames[i], level.mapname) == 0) {
			if(mapnames[i+1])
			{
				if(mapnames[i+1][0])
					BeginIntermission (CreateTargetChangeLevel (mapnames[i+1]) );
				else if(mapnames[0][0]) //no more maps, repeat list
					BeginIntermission (CreateTargetChangeLevel (mapnames[0]) );
			}
			else
				BeginIntermission (CreateTargetChangeLevel (mapnames[0]) );
		}
	}

	if (level.nextmap[0]) // go to a specific map
		BeginIntermission (CreateTargetChangeLevel (level.nextmap) );
	else {	// search for a changelevel
		ent = G_Find (NULL, FOFS(classname), "target_changelevel");
		if (!ent)
		{	// the map designer didn't include a changelevel,
			// so create a fake ent that goes back to the same level
			BeginIntermission (CreateTargetChangeLevel (level.mapname) );
			return;
		}
		BeginIntermission (ent);
	}
}


/*
=================
CheckNeedPass
=================
*/
void CheckNeedPass (void)
{
	int need;

	// if password or spectator_password has changed, update needpass
	// as needed
	if (password->modified || spectator_password->modified)
	{
		password->modified = spectator_password->modified = false;

		need = 0;

		if (*password->string && Q_strcasecmp(password->string, "none"))
			need |= 1;
		if (*spectator_password->string && Q_strcasecmp(spectator_password->string, "none"))
			need |= 2;

		gi.cvar_set("needpass", va("%d", need));
	}
}
/*
=================
ResetLevel
=================
*/
void ResetLevel (qboolean keepscores) //for resetting players and items after warmup
{
	int i, backup_score = 0; //initialize to shut gcc up
	edict_t	*ent;
	gitem_t *item;

	for (i=0 ; i<g_maxclients->value ; i++)
	{
		ent = g_edicts + i + 1;
		if (!ent->inuse || !player_participating (ent))
			continue;
		if (keepscores)
			backup_score = ent->client->resp.score;
		// locate ent at a spawn point and reset everything
		InitClientResp (ent->client);
		if (!ent->is_bot && ent->deadflag)
			DeathcamRemove (ent, "off");
		PutClientInServer (ent);
		ent->client->homing_shots = 0;
		if (keepscores)
			ent->client->resp.score = backup_score;
	}

	ACESP_SaveBots(); // update bots.tmp and client bot information

	blue_team_score = 0;
	red_team_score = 0;

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

		tacticalScore.hPTime = 
			tacticalScore.hCTime = 
			tacticalScore.hATime = 
			tacticalScore.aPTime = 
			tacticalScore.aCTime = 
			tacticalScore.aATime = 
			0;
	}

	mindEraserTime = jetpackTime = level.time;	

	//reset level items
	for (i=1, ent=g_edicts+i ; i < globals.num_edicts ; i++,ent++)
	{
		int j;
	
		if (!ent->inuse)
			continue;
		if(ent->client) //not players
			continue;

		//only items, not triggers, trains, etc
		for (j=0,item=itemlist ; j<game.num_items ; j++,item++)
		{
			if (!item->classname)
				continue;

			if (!strcmp(item->classname, ent->classname))
			{	// found it
				DoRespawn(ent);
				break;
			}
		}
	}

	if(g_callvote->integer)
		safe_bprintf(PRINT_HIGH, "Call voting is ^2ENABLED\n");
	else
		safe_bprintf(PRINT_HIGH, "Call voting is ^1DISABLED\n");

	if (g_antilagprojectiles->integer)
		safe_bprintf (PRINT_HIGH, "Antilagged projectiles are ^2ENABLED\n");
	else
		safe_bprintf (PRINT_HIGH, "Antilagged projectiles are ^1DISABLED\n");

	return;
}

/*
=================
CheckDMRules
=================
*/
void CheckDMRules (void)
{
	int        i, top_score;
	gclient_t  *cl;
	edict_t    *cl_ent;
	float      countdown_time;
	static int warmup_state = 0;
	static int teamgame_cvar_state = -1;
	
	/* note: g_teamgame cvar  code replaces setting the cvar repetitively */

	if ( TEAM_GAME )
	{
		if ( teamgame_cvar_state == -1 || teamgame_cvar_state == 0 )
		{
			gi.cvar_set ("g_teamgame", "1");
			teamgame_cvar_state = 1;
		}
		/* programming challenge: implement warmup for team games */
	}
	else 
	{
		if ( teamgame_cvar_state == -1 || teamgame_cvar_state == 1 )
		{
			gi.cvar_set ("g_teamgame", "0");
			teamgame_cvar_state = 0;
		}

		/*--- non-team game warmup ---*/
		/*
		 * note: broadcast sounds require first client to have sound info
		 * whether or not first client is inuse (bot or not, also not important)
		 */
		if(!g_tactical->integer)
		{
			if ( (warmup_state == 0) && (level.time <= warmuptime->value) )
			{ /* start warmup countdown */
				countdown_time = warmuptime->value - level.time;
				warmup_state = (int)(ceil( countdown_time ));

			}

			if ( warmup_state > 0 )
			{ /* warmup in progress */
				countdown_time = warmuptime->value - level.time;
				if ( (float)warmup_state > ceil( countdown_time ) )
				{ /* next state of countdown */
					--warmup_state;
					switch ( warmup_state )
					{
						case 3:
							gi.sound( &g_edicts[1], CHAN_AUTO, gi.soundindex( "misc/three.wav" ), 1, ATTN_NONE, 0 );
							break;
						case 2:
							gi.sound( &g_edicts[1], CHAN_AUTO, gi.soundindex( "misc/two.wav" ), 1, ATTN_NONE, 0 );
							break;
						case 1:
							gi.sound( &g_edicts[1], CHAN_AUTO, gi.soundindex( "misc/one.wav" ), 1, ATTN_NONE, 0 );
							break;
						case 0:
							gi.sound( &g_edicts[1], CHAN_AUTO, gi.soundindex( "misc/fight.wav" ), 1, ATTN_NONE, 0 );
							break;
						default:
							break;
					}
					if ( warmup_state > 0 )
					{
						for ( i = 1; i <= g_maxclients->integer; i++ )
						{
							safe_centerprintf( &g_edicts[i], "%i...\n", warmup_state );
						}
					}
					else
					{ /* end of warmup */
						for ( i = 1; i <= g_maxclients->integer; i++ )
						{
							safe_centerprintf( &g_edicts[i], "FIGHT!\n" );
						}
						ResetLevel(false);
					}
				}
			}
		}
	}

	if (level.intermissiontime)
		return;

	if (!deathmatch->integer)
		return;

	if (timelimit->value)
	{		
		if (timelimit->value*60 - level.time < 120)
		{
			if(!printT2)
			{
				for (i=0 ; i<g_maxclients->integer ; i++)
				{
					cl_ent = g_edicts + 1 + i;
					if (!cl_ent->inuse || cl_ent->is_bot)
						continue;
						safe_centerprintf(cl_ent, "2 minutes remain!\n");
				}
				printT2 = true;
			}
		}
		if (timelimit->value*60 - level.time < 60)
		{
			if(!printT1)
			{
				for (i=0 ; i<g_maxclients->integer ; i++)
				{
					cl_ent = g_edicts + 1 + i;
					if (!cl_ent->inuse || cl_ent->is_bot)
						continue;
						safe_centerprintf(cl_ent, "1 minute remains!\n");
				}
				printT1 = true;
			}
		}
		if (timelimit->value*60 - level.time < 30)
		{
			if(!printT5)
			{
				for (i=0 ; i<g_maxclients->integer ; i++)
				{
					cl_ent = g_edicts + 1 + i;
					if (!cl_ent->inuse || cl_ent->is_bot)
						continue;
						safe_centerprintf(cl_ent, "30 seconds remain!\n");
				}
				printT5 = true;
			}
		}
		
		if (level.time >= timelimit->value*60.0 && ((ctf->integer || (dmflags->integer & DF_SKINTEAMS)) || level.time > warmuptime->value))
		{
			safe_bprintf (PRINT_HIGH, "Timelimit hit.\n");
			EndDMLevel ();
			return;
		}
	}

	if (fraglimit->integer && ((ctf->integer || (dmflags->integer & DF_SKINTEAMS)) || level.time > warmuptime->value))
	{
		//team scores
		if ((dmflags->integer & DF_SKINTEAMS) || ctf->integer) //it's all about the team!
		{
			if(blue_team_score >= fraglimit->integer)
			{
				safe_bprintf(PRINT_HIGH, "Blue Team wins!\n");
				bot_won = 0; //we don't care if it's a bot that wins
				EndDMLevel();
				return;
			}
			if(red_team_score >= fraglimit->integer)
			{
				safe_bprintf(PRINT_HIGH, "Red Team wins!\n");
				bot_won = 0; //we don't care if it's a bot that wins
				EndDMLevel();
				return;
			}
		}
		else 
		{
			top_score = 0;
			for (i=0 ; i<g_maxclients->integer ; i++)
			{
				cl = game.clients + i;
				if (!g_edicts[i+1].inuse)
					continue;

				if(cl->resp.score > top_score)
					top_score = cl->resp.score; //grab the top score

				if (cl->resp.score >= fraglimit->integer)
				{
					if(g_edicts[i+1].is_bot)
					{
						bot_won = 1; //a bot has won the match
						safe_bprintf (PRINT_HIGH, "Fraglimit hit by bot.\n");
					}
					else
					{
						bot_won = 0;
						safe_bprintf (PRINT_HIGH, "Fraglimit hit.\n");
					}

					EndDMLevel ();
					return;
				}
			}
			if(!ctf->integer) 
			{
				i = fraglimit->integer - top_score;
				switch(i) 
				{
					case 3:
						if(!print3)
						{
							for (i=0 ; i<g_maxclients->integer ; i++)
							{
								cl_ent = g_edicts + 1 + i;
								if (!cl_ent->inuse || cl_ent->is_bot)
									continue;
								if(i == 0)
									gi.sound (cl_ent, CHAN_AUTO, gi.soundindex("misc/3frags.wav"), 1, ATTN_NONE, 0);
								safe_centerprintf(cl_ent, "3 frags remain!\n");
							}
							print3 = true;
						}
						break;
					case 2:
						if(!print2) 
						{
							for (i=0 ; i<g_maxclients->integer ; i++)
							{
								cl_ent = g_edicts + 1 + i;
								if (!cl_ent->inuse || cl_ent->is_bot)
									continue;
								if(i == 0)
									gi.sound (cl_ent, CHAN_AUTO, gi.soundindex("misc/2frags.wav"), 1, ATTN_NONE, 0);
								safe_centerprintf(cl_ent, "2 frags remain!\n");
							}
							print2 = true;
						}
						break;
					case 1:
						if(!print1) 
						{
							for (i=0 ; i<g_maxclients->integer ; i++)
							{
								cl_ent = g_edicts + 1 + i;
								if (!cl_ent->inuse || cl_ent->is_bot)
									continue;
								if(i == 0)
									gi.sound (cl_ent, CHAN_AUTO, gi.soundindex("misc/1frags.wav"), 1, ATTN_NONE, 0);
								safe_centerprintf(cl_ent, "1 frag remains!\n");
							}
							print1 = true;
						}
						break;
					default:
						break;
				}
			}
		}
	}

	if(g_tactical->integer)
	{
		if(!tacticalScore.alienAmmoDepot && !tacticalScore.alienComputer && !tacticalScore.alienPowerSource)
		{
			safe_bprintf(PRINT_HIGH, "The Humans have defeated the Aliens!\n");
			bot_won = 0; //we don't care if it's a bot that wins
			EndDMLevel();
			return;
		}
		if(!tacticalScore.humanAmmoDepot && !tacticalScore.humanComputer && !tacticalScore.humanPowerSource)
		{
			safe_bprintf(PRINT_HIGH, "The Aliens have defeated the Humans!\n");
			bot_won = 0; //we don't care if it's a bot that wins
			EndDMLevel();
			return;
		}

		//Warning Klaxons 
		//human
		if(tacticalScore.humanPowerSource && tacticalScore.humanPowerSourceHealth < 25)
		{
			if(level.time - tacticalScore.hPTime > 10)
			{
				tacticalScore.hPTime = level.time;
				for (i=0 ; i<g_maxclients->integer ; i++)
				{
					cl_ent = g_edicts + 1 + i;
					if (!cl_ent->inuse || cl_ent->is_bot || cl_ent->ctype == 0)
						continue;
					if(i == 0)
						gi.sound (cl_ent, CHAN_AUTO, gi.soundindex("misc/minderaser.wav"), 1, ATTN_NONE, 0);
					safe_centerprintf(cl_ent, "Human power source condition is ^2CRITICAL!\n");
				}
			}
		}
		if(tacticalScore.humanComputer && tacticalScore.humanComputerHealth < 25)
		{
			if(level.time - tacticalScore.hCTime > 10)
			{
				tacticalScore.hCTime = level.time;
				for (i=0 ; i<g_maxclients->integer ; i++)
				{
					cl_ent = g_edicts + 1 + i;
					if (!cl_ent->inuse || cl_ent->is_bot || cl_ent->ctype == 0)
						continue;
					if(i == 0)
						gi.sound (cl_ent, CHAN_AUTO, gi.soundindex("misc/minderaser.wav"), 1, ATTN_NONE, 0);
					safe_centerprintf(cl_ent, "Human computer condition is ^2CRITICAL!\n");
				}
			}
		}
		if(tacticalScore.humanAmmoDepot && tacticalScore.humanAmmoDepotHealth < 25)
		{
			if(level.time - tacticalScore.hATime > 10)
			{
				tacticalScore.hATime = level.time;
				for (i=0 ; i<g_maxclients->integer ; i++)
				{
					cl_ent = g_edicts + 1 + i;
					if (!cl_ent->inuse || cl_ent->is_bot || cl_ent->ctype == 0)
						continue;
					if(i == 0)
						gi.sound (cl_ent, CHAN_AUTO, gi.soundindex("misc/minderaser.wav"), 1, ATTN_NONE, 0);
					safe_centerprintf(cl_ent, "Human ammo depot condition is ^2CRITICAL!\n");
				}
			}
		}
		//alien
		if(tacticalScore.alienPowerSource && tacticalScore.alienPowerSourceHealth < 25)
		{
			if(level.time - tacticalScore.aPTime > 10)
			{
				tacticalScore.aPTime = level.time;
				for (i=0 ; i<g_maxclients->integer ; i++)
				{
					cl_ent = g_edicts + 1 + i;
					if (!cl_ent->inuse || cl_ent->is_bot || cl_ent->ctype == 1)
						continue;
					if(i == 0)
						gi.sound (cl_ent, CHAN_AUTO, gi.soundindex("misc/minderaser.wav"), 1, ATTN_NONE, 0);
					safe_centerprintf(cl_ent, "Alien power source condition is ^2CRITICAL!\n");
				}
			}
		}
		if(tacticalScore.alienComputer && tacticalScore.alienComputerHealth < 25)
		{
			if(level.time - tacticalScore.aCTime > 10)
			{
				tacticalScore.aCTime = level.time;
				for (i=0 ; i<g_maxclients->integer ; i++)
				{
					cl_ent = g_edicts + 1 + i;
					if (!cl_ent->inuse || cl_ent->is_bot || cl_ent->ctype == 1)
						continue;
					if(i == 0)
						gi.sound (cl_ent, CHAN_AUTO, gi.soundindex("misc/minderaser.wav"), 1, ATTN_NONE, 0);
					safe_centerprintf(cl_ent, "Alien computer condition is ^2CRITICAL!\n");
				}
			}
		}
		if(tacticalScore.alienAmmoDepot && tacticalScore.alienAmmoDepotHealth < 25)
		{
			if(level.time - tacticalScore.aATime > 10)
			{
				tacticalScore.aATime = level.time;
				for (i=0 ; i<g_maxclients->integer ; i++)
				{
					cl_ent = g_edicts + 1 + i;
					if (!cl_ent->inuse || cl_ent->is_bot || cl_ent->ctype == 1)
						continue;
					if(i == 0)
						gi.sound (cl_ent, CHAN_AUTO, gi.soundindex("misc/minderaser.wav"), 1, ATTN_NONE, 0);
					safe_centerprintf(cl_ent, "Alien ammo depot condition is ^2CRITICAL!\n");
				}
			}
		}
	}
}

/*
=============
ExitLevel
=============
*/
void ExitLevel (void)
{
	int			i;
	edict_t		*ent;
	char		command [256];

	if(strcmp(level.mapname, level.changemap) || timelimit->value || g_tactical->value) 
	{
		Com_sprintf( command, sizeof(command), "map \"%s\"\n", level.changemap );
		gi.AddCommandString( command );
	}

    //Note-- whenever the map command fails (for instance, misspelled bsp name
    //in the server cfg,) it will just play another game on the same map. As
    //of right here in the code, there is no way to detect if that is going to
    //happen, so we initialize another game on this map just in case. This
    //fixes many longstanding bugs where misspelled map names in the cfg would
    //cause servers to glitch out.
    // -Max

	level.changemap = NULL;
	level.exitintermission = 0;
	level.intermissiontime = 0;
	ClientEndServerFrames ();
	EndIntermission();

	// clear some things before going to next level
	for (i=0 ; i<g_maxclients->integer ; i++)
	{
		ent = g_edicts + 1 + i;
		if (!ent->inuse)
			continue;
		if (ent->health > ent->client->pers.max_health)
			ent->health = ent->client->pers.max_health;

		ent->client->resp.score = 0;
		ent->client->resp.deaths = 0;
		ent->client->resp.reward_pts = 0;
		ent->client->homing_shots = 0;
		// FIXME: kinda hacky
		if (ent->is_bot)
		{
			ent->takedamage = DAMAGE_AIM;
			ent->solid = SOLID_BBOX;
			ent->deadflag = DEAD_NO;
			PutClientInServer (ent);
		}
		else
		{
			spectator_respawn (ent);
		}
	}
	for (i=1, ent=g_edicts+i ; i < globals.num_edicts ; i++,ent++) 
	{

		if (!ent->inuse || ent->client)
			continue;
		//remove podiums
		if(!strcmp(ent->classname, "pad"))
			G_FreeEdict(ent);
	}

	red_team_score = 0;
	blue_team_score = 0;

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

		tacticalScore.hPTime = 
			tacticalScore.hCTime = 
			tacticalScore.hATime = 
			tacticalScore.aPTime = 
			tacticalScore.aCTime = 
			tacticalScore.aATime = 
			0;
	}

	print1 = print2 = print3 = false;

}

/*
================
G_NameMatch

 A name entered in the console may (unlikely) or may not have color codes.
 This is a case-sensitive match, because a case-insensitive match would
   complicate matters.

 Used by sv removebot <name> and callvote kicks

================
*/
qboolean G_NameMatch( const char* netname, const char *kickname )
{
	const char* p_netname = netname;
	const char* p_kickname = kickname;
	int char_count = 16;
	int len_count = 49; // maximum length of 16 char color name (16 * 3 + nul)
	qboolean matched = false;

	while ( len_count > 0 && char_count > 0)
	{
		if ( *p_netname != *p_kickname )
		{
			if ( Q_IsColorString( p_netname ) )
			{ // netname has color code, kickname does not, match still possible
				p_netname += 2;
				len_count -= 2;
			}
			else if ( Q_IsColorString( p_kickname ))
			{ // kickname has color code, netname does not, match still possible
				p_kickname += 2;
				len_count -= 2;
			}
			else
			{ // no match. mismatched chars, or end of one or the other
				break;
			}
		}
		else if ( !*p_netname && !*p_kickname )
		{ // end of both strings,
			matched = true;
			break;
		}
		else
		{ // chars matched
			if ( Q_IsColorString( p_netname ) && Q_IsColorString( p_kickname ) )
			{ // color does not have to match
				p_netname += 2;
				p_kickname += 2;
				len_count -= 2;
			}
			else
			{
				++p_netname;
				++p_kickname;
				--len_count;
				--char_count;
			}
		}
	}

	return matched;
}

/*
================
G_ParseVoteCommand

Parse and execute command
================
*/
// helper function
static edict_t* find_kick_target( const char *name )
{
	edict_t* kick_target = NULL;
	edict_t* ent;
	int i;

	for ( i = 1 ; i <= g_maxclients->integer ; i++ )
	{
		ent = &g_edicts[i];
		if ( !ent->inuse )
			continue;
		if ( ent->client )
		{
			if ( G_NameMatch( ent->client->pers.netname, name ) )
			{
				kick_target = ent;
				break;
			}
		}
	}

	return kick_target;
}

void G_ParseVoteCommand (void)
{
	int i, j;
	char command[128];
	char args[128];
	edict_t *ent;
	qboolean donearg = false;
	char	*value;

	//separate command from args
	i = j = 0;
	while(i < 128) {

		if(playervote.command[i] == ' ')
			donearg = true;

		if(!donearg)
			command[i] = playervote.command[i];
		else
			command[i] = 0;

		if(donearg && i < 127) { //skip the space between command and arg
			args[j] = playervote.command[i+1];
			j++;
		}
		i++;
	}

	if ( !strcmp(command, "kick") )
	{ // kick player or bot
		ent = find_kick_target( args );
		if ( ent )
		{
			if ( ent->is_bot )
			{
				if ( sv_botkickthreshold && sv_botkickthreshold->integer )
				{
					safe_bprintf(PRINT_HIGH,
						"Auto bot kick enabled, callvote kick <bot> disabled.\n%s not kicked.\n",
						 ent->client->pers.netname);
				}
				else
				{
					ACESP_KickBot( ent );
				}
			}
			else
			{
				safe_bprintf(PRINT_HIGH, "%s was kicked\n", args);
				ClientDisconnect (ent);
			}
		}
		else
		{
			safe_bprintf( PRINT_HIGH, "Did not find %s here.\n", args );
		}
	}
	else if ( !strcmp( command, "kickban") )
	{ // kickban player
		ent = find_kick_target( args );
		if ( ent )
		{
			if ( ent->is_bot )
			{ //
				safe_bprintf(PRINT_HIGH,
					"%s is a bot, use \"kick\", not \"kickban.\"\n", ent->client->pers.netname);
			}
			else
			{
				safe_bprintf(PRINT_HIGH, "%s was kickbanned\n", args);
				ClientDisconnect (ent);
				value = Info_ValueForKey (ent->client->pers.userinfo, "ip");
				G_Ban(value);
			}
		}
		else
		{
			safe_bprintf( PRINT_HIGH, "Did not find %s here.\n", args );
		}
	}
	else if(!strcmp(command, "fraglimit")) { //change fraglimit
		gi.cvar_set("fraglimit", args);
		safe_bprintf(PRINT_HIGH, "Fraglimit changed to %s\n", args);
	}
	else if(!strcmp(command, "timelimit")) { //change timelimit
		gi.cvar_set("timelimit", args);
		safe_bprintf(PRINT_HIGH, "Timelimit changed to %s\n", args);
	}
	else if(!strcmp(command, "map")) { //change map
		Com_sprintf (command, sizeof(command), "map \"%s\"\n", args);
		gi.AddCommandString (command);
	}
	else
		safe_bprintf(PRINT_HIGH, "Invalid command!");

}


/*
================
G_RunFrame

Advances the world by 0.1 seconds
================
*/
extern void ACEND_DrawPath(void);
void G_RunFrame (void)
{
	int		i, numActiveClients = 0;
	edict_t	*ent;

	level.framenum++;
	level.time = level.framenum*FRAMETIME;

	// Used in antilag code
	level.leveltime = gi.Sys_Milliseconds();

	/*
	 * update bot info in first client always, and in other active clients
	 */
	ACESP_UpdateBots();

	// choose a client for monsters to target this frame
	AI_SetSightClient ();

	// exit intermissions

	if (level.exitintermission)
	{
		ExitLevel ();
		return;
	}

	//
	// treat each object in turn
	// even the world gets a chance to think
	//

	ent = &g_edicts[0];
	for (i=0 ; i<globals.num_edicts ; i++, ent++)
	{
		if (!ent->inuse)
			continue;

		level.current_entity = ent;

		VectorCopy (ent->s.origin, ent->s.old_origin);

		// if the ground entity moved, make sure we are still on it
		if ((ent->groundentity) && (ent->groundentity->linkcount != ent->groundentity_linkcount))
		{
			ent->groundentity = NULL;
			if ( !(ent->flags & (FL_SWIM|FL_FLY)) && (ent->svflags & SVF_MONSTER) )
			{
				M_CheckGround (ent);
			}
		}

		if (i > 0 && i <= g_maxclients->integer)
		{
			ClientBeginServerFrame (ent);
		}

		if(ent->inuse && ent->client && !ent->is_bot)
		{
			if ( ent->s.number <= g_maxclients->integer )
			{ // count actual players, not deathcam entities
				numActiveClients++;
			}
		}

		G_RunEntity (ent, FRAMETIME);
	}

	// see if it is time to end a deathmatch
	CheckDMRules ();

	// see if needpass needs updated
	CheckNeedPass ();

	// build the playerstate_t structures for all players
	ClientEndServerFrames ();

	// For bot debugging
	ACEND_DrawPath();

	//call voting
	if(g_callvote->integer && playervote.called) {

		playervote.time = level.time;
		if(playervote.time-playervote.starttime > 15 ){ //15 seconds
			// Execute command if votes are sufficient. Unanimous votes always
			// pass, even if it's just a single person on the server.
			// Otherwise 3-1 minimum to pass.
			if (	(numActiveClients > 0 && playervote.yay == numActiveClients) ||
					(playervote.yay > 2 && playervote.yay > playervote.nay+1))
			{ 
				safe_bprintf(PRINT_HIGH, "Vote ^2Passed\n");
				//parse command(we will allow kick, map, fraglimit, timelimit).
				G_ParseVoteCommand();

			}
			else
			{
				safe_bprintf(PRINT_HIGH, "Vote ^1Failed\n");
			}

			//clear
			playervote.called = false;
			playervote.yay = playervote.nay = 0;
			playervote.command[0] = 0;

			//do each ent
			for (i=0 ; i<g_maxclients->integer ; i++)
			{
				ent = g_edicts + 1 + i;
				if (!ent->inuse || ent->is_bot)
					continue;
				ent->client->resp.voted = false;
			}
		}
	}
}

