/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 1998 Steve Yeager
Copyright (C) 2010 COR Entertainment, LLC.

See below for Steve Yeager's original copyright notice.
Modified to GPL in 2002.

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
//  acebot_spawn.c - This file contains all of the
//                   spawing support routines for the ACE bot.
//
///////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "game/g_local.h"
#include "game/m_player.h"
#include "acebot.h"

#define MAX_BOTTMPFILE_COUNT 64 // arbitrary limit for bot .tmp files
	// mostly just for checking validity of the count field in the file.

static size_t szr; // just for unused result warnings

/*
 * Bot File Functions
 *
 * for loading bots from custom<n>.cfg, team.cfg, and <mapname>.cfg
 */
static struct loadbots_file_s
{
	FILE *pfile;
	int record_count;
} loadbots_file;

static void loadbots_openfile( void )
{
	char bot_filename[MAX_OSPATH];
	char stem[MAX_QPATH];
	int tmpcount;
	FILE *tmppfile;
	size_t result;

	loadbots_file.pfile = NULL;
	loadbots_file.record_count = 0;

	// custom<n>.cfg has priority over team.cfg
	if ( sv_custombots && sv_custombots->integer )
	{
		sprintf( stem, "botinfo/custom%i.tmp", sv_custombots->integer );
	}
	else if ( TEAM_GAME )
	{
		strcpy(stem, "botinfo/team.tmp");
	}
	else
	{
		sprintf(stem, "botinfo/%s.tmp", level.mapname);
	}

	if ( !gi.FullPath( bot_filename, sizeof(bot_filename), stem ) )
	{
		gi.dprintf("ACESP_LoadBots: not found: %s\n", stem);
	}
	else if ( (tmppfile = fopen(bot_filename, "rb")) == NULL )
	{
		gi.dprintf("ACESP_LoadBots: failed fopen for read: %s\n", bot_filename);
	}
	else
	{ // read the count
		result = fread( &tmpcount, sizeof(int), 1, tmppfile );
		if (result != 1 || tmpcount < 0 || tmpcount > MAX_BOTTMPFILE_COUNT)
		{
			gi.dprintf("ACESP_LoadBots: failed fread or invalid count in %s\n", bot_filename);
			fclose( tmppfile );
		}
		else if ( tmpcount == 0 )
		{
			gi.dprintf("ACESP_LoadBots: %s is empty\n", bot_filename);
			fclose( tmppfile );
		}
		else
		{
			loadbots_file.pfile = tmppfile;
			loadbots_file.record_count = tmpcount;
		}
	}
}

static void loadbots_closefile( void )
{
	if ( loadbots_file.pfile != NULL )
	{
		fclose( loadbots_file.pfile );
		loadbots_file.record_count = 0;
	}
}

static size_t loadbots_readnext( char *p_userinfo_bfr )
{
	size_t result;
	char botname[PLAYERNAME_SIZE];

	if ( loadbots_file.pfile == NULL || loadbots_file.record_count == 0 )
	{
		result = 0 ;
	}
	else
	{
		result = fread( p_userinfo_bfr, sizeof(char) * MAX_INFO_STRING, 1, loadbots_file.pfile );
		if ( result )
		{ // make sure name from file is valid
			Q_strncpyz2( botname, Info_ValueForKey( p_userinfo_bfr, "name" ), sizeof(botname) );
			ValidatePlayerName( botname, sizeof(botname) );
			Info_SetValueForKey( p_userinfo_bfr, "name", botname );
		}
	}

	return result;  // 1 if ok, 0 if not
}


/*
 * client_botupdate
 *
 * count bots and update bot information in clients
 *
 * see:
 *  p_hud.c::G_UpdateStats()
 *  sv_main.c::SV_StatusString()
 *
 */
static int client_botupdate( void )
{
	int botnum;
	int botidx;
	char *botname;
	edict_t *pbot;
	int bots;
	edict_t *pclient;
	int clients;
	qboolean firstclient;

	// count bots
	botnum = 0;
	clients = game.maxclients;
	for ( pbot=&g_edicts[clients]; clients--; --pbot )
	{
		if ( pbot->inuse && pbot->is_bot )
		{
			++botnum;
		}
	}

	// update clients
	if ( botnum == 0 )
	{ // clear bot count
		for ( pclient = &g_edicts[1], clients=game.maxclients ; clients--; ++pclient )
		{ // in every active client
			if ( pclient->inuse )
			{
				pclient->client->ps.botnum = pclient->client->resp.botnum = 0;
			}
		}
		return 0;
	}

	botidx = 0;
	bots = botnum;
	for ( pbot = &g_edicts[game.maxclients]; bots ; --pbot )
	{ // process each bot
		if ( pbot->inuse && pbot->is_bot )
		{
			botname = Info_ValueForKey( pbot->client->pers.userinfo, "name" );
			firstclient = true;
			for ( pclient = &g_edicts[1], clients=game.maxclients ; clients--; ++pclient )
			{ // for every active client, plus always update first client for server
				if ( pclient->inuse || firstclient )
				{
					pclient->client->ps.botnum = pclient->client->resp.botnum = botnum;
					strcpy( pclient->client->resp.bots[botidx].name, botname );
					strcpy( pclient->client->ps.bots[botidx].name, botname );
					pclient->client->resp.bots[botidx].score  = pbot->client->resp.score;
					pclient->client->ps.bots[botidx].score    = pbot->client->resp.score;
					pclient->client->resp.bots[botidx].dmteam = pbot->dmteam;
					pclient->client->ps.bots[botidx].dmteam   = pbot->dmteam;
					firstclient = false;
				}
			}
			++botidx;
			--bots;
		}
	}

	return botnum;
}

/**
 * @brief Update bot info in client records
 *
 * @detail  Intended to be called once per frame, in RunFrame. First client
 *          record should always have current bot info, so that server status
 *          shows bots even when in intermission.
 *          In ACE debug mode, reports when bot count changes. ACE debug mode
 *          is controlled with "sv acedebug on","sv acedebug off"
 *
 */
void ACESP_UpdateBots( void )
{
	static int prev_count = 0;
	int count;

	/* count bots, and, in debug mode, output changes. */
	count = client_botupdate();
	if ( debug_mode )
	{
		if ( count != prev_count )
			debug_printf("Bot count %i to %i\n", prev_count, count );
	}
	prev_count = count;

}

/*
======
 ACESP_SaveBots

 Save current bots to bots.tmp file, which is used for
 creating custom<n>.tmp, team.tmp and <mapname>.tmp files

 also update bot information in client records

======
 */
void ACESP_SaveBots( void )
{
    edict_t *bot;
    FILE *pOut;
	int i,count;
	char full_path[MAX_OSPATH];

    count = client_botupdate(); // count bots and update clients

	gi.FullWritePath( full_path, sizeof(full_path), "botinfo/bots.tmp" );
	if ( ( pOut = fopen( full_path, "wb" )) == NULL )
	{
		gi.dprintf("ACESP_SaveBots: fopen for write failed: %s\n", full_path );
		return;
	}

	if ( count > MAX_BOTTMPFILE_COUNT )
	{ // record count limit mostly just for file error checking
		count = MAX_BOTTMPFILE_COUNT;
		gi.dprintf("ACESP_SaveBots: count limited to (%i)\n, ", count );
	}

	szr = fwrite(&count,sizeof (int),1,pOut); // Write number of bots

	for (i = game.maxclients; i > 0 && count ; i--)
	{ // write all current bots to bots.tmp
		bot = &g_edicts[i];
		if (bot->inuse && bot->is_bot)
		{
			szr = fwrite(bot->client->pers.userinfo,sizeof (char) * MAX_INFO_STRING,1,pOut);
			--count;
		}
	}

    fclose(pOut);

}

/*
======
 ACESP_FindBot

 find bot by name
======
*/
edict_t *ACESP_FindBot( const char *name )
{
	edict_t *pbot;
	edict_t *found_bot = NULL;
	int clients = game.maxclients;

	for ( pbot=&g_edicts[clients]; clients-- ; --pbot )
	{
		if ( pbot->inuse && pbot->is_bot
			&& !strcmp( pbot->client->pers.netname, name ) )
		{
			found_bot = pbot;
			break;
		}
	}

	return found_bot;
}

/*
 * game_census
 *
 * simple census, for non team.
 * team games use p_client.c::TeamCensus()
 */
typedef struct gamecensus_s
{
	int real;
	int bots;
} gamecensus_t;

static void game_census( gamecensus_t *gamecensus )
{
	int i;
	int real = 0;
	int bots = 0;

	for (i = 0 ; i < game.maxclients; i++)
	{
		edict_t *cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse)
			continue;
		
		if (cl_ent->is_bot)
			bots++;
		else
			real +=	game.clients[i].resp.participation == participation_playing ||
					game.clients[i].resp.participation == participation_duelwaiting;
	}
	gamecensus->real = real;
	gamecensus->bots = bots;
}

/*
======
 ACESP_LoadBots

 Called for a client on connect or disconnect
 or for every client on ResetLevel()

 Also "unloads" bots for auto bot kick

======
*/

/*
 * LoadBots sub-functions
 *
 * Team and Non-Team, With and Without Auto Bot Kick
 *
 * On entry:
 *  '*.tmp' file has been opened, caller will close file
 *  record count has been read and is known to be >0
 *  file read position is at first bot record
 */

static void loadbots_team_botkick( edict_t *ent )
{
	char userinfo[MAX_INFO_STRING];
	char *name;
	char *skin;
	int rec_count;
	teamcensus_t teamcensus;
	int spawnkicknum;
	int ingame_players;
	int ingame_bots;
	qboolean bot_spawned;
	qboolean replace_bot;
	int botreplace_team;
	edict_t *pbot;
	int result;

	// count the ingame players and bots
	TeamCensus( &teamcensus );

	ingame_players = teamcensus.real;
	ingame_bots = teamcensus.bots;

	spawnkicknum = sv_botkickthreshold->integer;
	if ( ingame_players >= spawnkicknum )
	{ // do not need any bots, kick any that are on server
		for ( pbot=&g_edicts[game.maxclients]; ingame_bots ; --pbot )
		{
			if ( pbot->inuse && pbot->is_bot )
			{
				ACESP_KickBot( pbot );
				--ingame_bots;
			}
		}
		return;
	}

	replace_bot = false;
	botreplace_team = NO_TEAM; // default, either team.
	if ( teamcensus.total > spawnkicknum && ingame_bots > 0 )
	{  // a bot will be replaced
		replace_bot = true;
		if ( teamcensus.bots_blue > 0 && teamcensus.bots_red > 0 )
		{ // bots on both teams,
			botreplace_team = ent->dmteam; // replace same team, unless...
			if ( ent->dmteam == RED_TEAM && teamcensus.red < teamcensus.blue )
			{
				botreplace_team = BLUE_TEAM;
			}
			else if ( ent->dmteam == BLUE_TEAM && teamcensus.blue < teamcensus.red )
			{
				botreplace_team = RED_TEAM;
			}
		}
	}

	// bot team assignment is done in ACESP_SetName(), called from ACESP_SpawnBot()

	rec_count = loadbots_file.record_count;
	while ( rec_count-- )
	{
		result = loadbots_readnext( userinfo );
		if ( result == 0 )
		{ // file error
			break;
		}

		name = Info_ValueForKey( userinfo, "name" );
		pbot = ACESP_FindBot( name );
		if ( pbot == NULL )
		{ // not on server
			if ( !replace_bot )
			{ // not benching a bot, so ok to spawn if limit allows
				if ( (ingame_players + ingame_bots) < spawnkicknum )
				{ // spawn a team bot
					skin = Info_ValueForKey( userinfo, "skin" );
					bot_spawned = ACESP_SpawnBot( name, skin, NULL);
					if ( bot_spawned )
					{
						++ingame_bots;
					}
				}
			}
		}
		else
		{ // already on server
			if ( replace_bot )
			{
				if ( botreplace_team == NO_TEAM || botreplace_team == pbot->dmteam )
				{ // on either team (NO_TEAM) or on specified team only
					ACESP_KickBot( pbot );
					--ingame_bots;
					replace_bot = false; // one time only
				}
			}
			else if ( (ingame_players + ingame_bots) > spawnkicknum )
			{ // apply bot kick threshold
				ACESP_KickBot( pbot );
				--ingame_bots;
			}
		}
	}
}

static void loadbots_team( void )
{
	char userinfo[MAX_INFO_STRING];
	char *name;
	char *skin;
	int rec_count;
	qboolean bot_spawned;
	edict_t *bot;
	int result;

	// bot team assignment is done in ACESP_SetName(), called from ACESP_SpawnBot()

	rec_count = loadbots_file.record_count;
	while ( rec_count-- )
	{ // load all bots in the file
		result = loadbots_readnext( userinfo );
		if (!result)
		{ // file read error
			break;
		}

		name = Info_ValueForKey(userinfo, "name");
		bot = ACESP_FindBot( name );
		if (bot == NULL)
		{ // not on server, spawn a team bot
			skin = Info_ValueForKey(userinfo, "skin");
			bot_spawned = ACESP_SpawnBot( name, skin, NULL );
		}
	}
}

static void loadbots_nonteam_botkick( void )
{
	char userinfo[MAX_INFO_STRING];
	char *name;
	int rec_count;
	int spawnkicknum;
	int ingame_players;
	int ingame_bots;
	qboolean bot_spawned;
	edict_t *pbot;
	int result;
	gamecensus_t gamecensus;

	game_census( &gamecensus );

	ingame_players = gamecensus.real;
	ingame_bots = gamecensus.bots;

	if ( g_duel->integer )
	{ // duel mode can have 1 and only 1 bot
		if ( ingame_players > 0 )
		{ // 1 or 0 bots and 1 or more real players
			spawnkicknum = 2;
		}
		else
		{ // 1 bot or 0 bots, 0 real players
			spawnkicknum = 1;
		}
	}
	else
	{
		spawnkicknum = sv_botkickthreshold->integer;
	}

	if ( ingame_players >= spawnkicknum )
	{ // do not need any bots, kick any that are on server
		for ( pbot=&g_edicts[game.maxclients]; ingame_bots ; pbot-- )
		{
			if ( pbot->inuse && pbot->is_bot )
			{
				ACESP_KickBot( pbot );
				--ingame_bots;
			}
		}
		return;
	}

	rec_count = loadbots_file.record_count;
	while ( rec_count-- )
	{
		result = loadbots_readnext( userinfo );
		if ( result == 0 )
		{ // file error
			break;
		}

		name = Info_ValueForKey( userinfo, "name" );
		pbot = ACESP_FindBot( name );
		if ( pbot == NULL )
		{ // not on server, spawn a bot if bot kick threshold allows
			if ( (ingame_players + ingame_bots) < spawnkicknum  )
			{ // spawn a non-team bot
				bot_spawned = ACESP_SpawnBot (NULL, NULL, userinfo);
				if ( bot_spawned )
				{
					++ingame_bots;
				}
			}
		}
		else
		{ // already on server
			if ( (ingame_players + ingame_bots) > spawnkicknum )
			{ // bot kick threshold
				ACESP_KickBot( pbot );
				--ingame_bots;
			}
		}
	}
}

static void loadbots_nonteam( void )
{
	char userinfo[MAX_INFO_STRING];
	char *name;
	int rec_count;
	qboolean bot_spawned;
	edict_t *pbot;
	int result;

	rec_count = loadbots_file.record_count;
	while ( rec_count-- )
	{
		result = loadbots_readnext( userinfo );
		if ( !result )
		{ // file read error
			break;
		}
		name = Info_ValueForKey( userinfo, "name" );
		pbot = ACESP_FindBot( name );
		if ( pbot == NULL )
		{ // not found on server, spawn a non-team bot
			bot_spawned = ACESP_SpawnBot (NULL, NULL, userinfo);
		}
	}
}

void ACESP_LoadBots( edict_t *ent )
{

	if ( dmflags->integer & DF_BOTS )
	{ // bots disabled.
		// result of setting the dmflag to disable bots when there are
		// bots in the game is undefined.
		return;
	}

	loadbots_openfile();
	if ( loadbots_file.record_count == 0 )
	{ // no bots to load
		loadbots_closefile();
		return;
	}

	if ( (sv_botkickthreshold && sv_botkickthreshold->integer) || g_duel->integer )
	{ // auto botkick, duel mode allows only 1 bot
		if ( TEAM_GAME )
		{
			loadbots_team_botkick( ent );
		}
		else
		{
			loadbots_nonteam_botkick();
		}
	}
	else
	{ // no auto botkick
		/*
		 * TODO: see if it is possible to disable bot loading here
		 *  if there have been any callvote bot kicks in a game.
		 *  Because, if a new player enters, kicked bots will be reloaded.
		 */
		if ( TEAM_GAME )
		{
			loadbots_team();
		}
		else
		{
			loadbots_nonteam();
		}
	}

	loadbots_closefile();

}

/*
======
 ACESP_FindBotNum

 called by server to determine bot kick threshold
======
 */
int ACESP_FindBotNum(void)
{
	int count;

	if ( dmflags->integer & DF_BOTS )
	{ // bots disabled by dmflag bit
		return 0;
	}

	loadbots_openfile();
	count = loadbots_file.record_count;
	loadbots_closefile();

	return count;
}

///////////////////////////////////////////////////////////////////////
// Called by PutClient in Server to actually release the bot into the game
// Keep from killin' each other when all spawned at once
///////////////////////////////////////////////////////////////////////
static void ACESP_HoldSpawn (edict_t *self)
{

	if (!self->inuse || !self->is_bot)
	{
		gi.dprintf("ACEAI_HoldSpawn: bad call program error\n");
		return;
	}
	
	PutClientInServer (self);

	// send effect
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (self-g_edicts);
	gi.WriteByte (MZ_LOGIN);
	gi.multicast (self->s.origin, MULTICAST_PVS);

	safe_bprintf (PRINT_MEDIUM, "%s entered the game\n", self->client->pers.netname);
}

/*===
  ACECO_ReadConfig()

  System-independent bot configuration file reader.
  2010-08: Replaces function in acebot_config.cpp for Windows

  To be called with relative path to the .cfg file.

===*/
void ACECO_ReadConfig( char *config_file )
{
	char full_path[ MAX_OSPATH];
	FILE *fp;
	int k;
	size_t length, result;
	char *buffer;
	char *s;
	const char *delim = "\r\n";
	float tmpf;
	int i;

	//set bot defaults(in case no bot config file is present for that bot)
	botvals.skill = 1; //medium
	strcpy(botvals.faveweap, "None");
	for ( k = 1; k < 10; k++ )
		botvals.weapacc[k] = 0.75;
	botvals.awareness = 0.7; // 0.7 is 145 degree FOV

	strcpy( botvals.chatmsg[0], "%s: You are a real jerk %s!"    );
	strcpy( botvals.chatmsg[1], "%s: Wait till next time %s."    );
	strcpy( botvals.chatmsg[2], "%s: Life was better alive, %s!" );
	strcpy( botvals.chatmsg[3], "%s: You will pay for this %s!"  );
	strcpy( botvals.chatmsg[4], "%s: You're using a bot %s!"     );
	strcpy( botvals.chatmsg[5], "%s: I will be hunting you %s!"  );
	strcpy( botvals.chatmsg[6], "%s: It hurts %s...it hurts..."  );
	strcpy( botvals.chatmsg[7], "%s: Just a lucky shot %s!"      );

	if ( !gi.FullPath( full_path, sizeof(full_path), config_file ) )
	{ // bot not configured, use defaults
		return;
	}
	if ( (fp = fopen( full_path, "rb" )) == NULL )
	{
		gi.dprintf("ACECO_ReadConfig: failed open for read: %s\n", full_path );
		return;
	}
	if ( fseek(fp, 0, SEEK_END) )
	{ // seek error
		fclose( fp );
		return;
	}
	if ( (length = ftell(fp)) == (size_t)-1L )
	{ // tell error
		fclose( fp );
		return;
	}
	if ( fseek(fp, 0, SEEK_SET) )
	{ // seek error
		fclose( fp );
		return;
	}
	buffer = malloc( length + 1);
	if ( buffer == NULL )
	{ // memory allocation error
		fclose( fp );
		return;
	}
	result = fread( buffer, 1, length, fp );
	fclose( fp );
	if ( result != length )
	{ // read error
		free( buffer );
		return;
	}
	buffer[length] = 0;

	// note: malloc'd buffer is modified by strtok
	if ( (s = strtok( buffer, delim )) != NULL )
		botvals.skill = atoi( s );
	if ( botvals.skill < 0 )
		botvals.skill = 0;

	if ( s &&  ((s = strtok( NULL, delim )) != NULL) )
		strncpy( botvals.faveweap, s, sizeof(botvals.faveweap)-1 );

	for(k = 1; k < 10; k++)
	{
		if ( s && ((s = strtok( NULL, delim )) != NULL ) )
		{
			tmpf = atof( s );
			if ( tmpf < 0.5f )
				tmpf = 0.5f;
			else if (tmpf > 1.0f )
				tmpf = 1.0f;
			botvals.weapacc[k] = tmpf;
		}
	}

	if ( s && ((s = strtok( NULL, delim)) != NULL) )
	{
		tmpf = atof( s );
		if ( tmpf < 0.1f )
			tmpf = 0.1f;
		else if ( tmpf > 1.0f )
			tmpf = 1.0f;
		botvals.awareness = tmpf;
	}

	i = 0;
	while ((s = strtok( NULL, delim)) != NULL && i < sizeof(botvals.chatmsg)/sizeof(botvals.chatmsg[0]))
	{
		strncpy( botvals.chatmsg[i], s, sizeof(botvals.chatmsg[i])-1 );
		i++;
	}
	
	free( buffer );

}

void ACESP_SpawnInitializeAI (edict_t *ent)
{
	ent->enemy = NULL;
	ent->movetarget = NULL;
	ent->targDeltaX = 0;
	ent->targDeltaY = 0;
	ent->targDirSwitchX = false;
	ent->targDirSwitchY = false;
	ent->yaw_speed = 37 * FRAMETIME/TENFPS; // bot turning speed. angle in degrees
	ent->state = STATE_MOVE;

	// Set the current node
	ent->current_node = ACEND_FindClosestReachableNode(ent,NODE_DENSITY, NODE_ALL);
	ent->goal_node = ent->current_node;
	ent->next_node = ent->current_node;
	ent->next_move_time = level.time;
	ent->suicide_timeout = level.time + 15.0;
	ent->nextthink = level.time + FRAMETIME;
	ent->think = ACEAI_Think;
}

///////////////////////////////////////////////////////////////////////
// Find a free client spot
///////////////////////////////////////////////////////////////////////
edict_t *ACESP_FindFreeClient (void)
{
	int clients;
	edict_t *pbot = NULL;
	edict_t *found_bot = NULL;
	int maxcount = 0;

	// find current maximum count field, used for bot naming
	clients = game.maxclients;
	for ( pbot=&g_edicts[clients]; clients--; --pbot )
	{
		if ( pbot->is_bot && (pbot->count > maxcount) )
		{
			maxcount = pbot->count;
		}
	}
	// find a free slot for new bot
	clients = game.maxclients;
	for ( pbot=&g_edicts[clients]; clients--; --pbot )
	{ // reverse search, bots are assigned from end of list
		if ( !pbot->inuse )
		{
			pbot->count = maxcount + 1; // for new bot's generic name
			found_bot = pbot;
			break;
		}
	}

	return found_bot;
}

/*
 * ACESP_SetName()
 *
 * determine bot name and skin
 * for team games, select the team
 */
void ACESP_SetName(edict_t *bot, char *name, char *skin, char *userinfo )
{
	char bot_skin[MAX_INFO_STRING];
	char bot_name[MAX_INFO_STRING];
	char playerskin[MAX_INFO_STRING];
	char playermodel[MAX_INFO_STRING];
	int i, j, k, copychar;
	teamcensus_t teamcensus;
	char *skin2;

	if(strlen(name) == 0)
	{ // generate generic name, set model/skin to default
		sprintf(bot_name,"ACEBot_%d",bot->count);
		sprintf(bot_skin,"martianenforcer/default");
		skin2 = bot_skin;
	}
	else
	{ // name and skin from args
		strcpy(bot_name,name);
		skin2 = skin;
	}

	bot->dmteam = NO_TEAM;
	bot->teamset = false;
	if ( TEAM_GAME )
	{
		// extract model from <model/skin> info record
		copychar = false;
		strcpy(playerskin, " ");
		strcpy(playermodel, " ");
		j = k = 0;
		for(i = 0; i <= (int)strlen(skin2) && i < MAX_INFO_STRING; i++)
		{
			if(copychar){
				playerskin[k] = skin2[i];
				k++;
			}
			else {
				playermodel[j] = skin2[i];
				j++;
			}
			if(skin2[i] == '/')
				copychar = true;
		}
		playermodel[j] = 0;

		// assign bot to a team and set skin
		TeamCensus( &teamcensus ); // apply team balancing rules
		bot->dmteam = teamcensus.team_for_bot;
		if ( bot->dmteam == BLUE_TEAM )
		{
			strcpy(playerskin, "blue");
			bot->teamset = true;
		}
		else if ( bot->dmteam == RED_TEAM )
		{
			strcpy(playerskin, "red");
			bot->teamset = true;
		}
		else
		{ // not expected. probably a program error.
			gi.dprintf("ACESP_SetName: bot team skin program error.\n");
			strcpy( playerskin, "default" );
		}
		// concatenate model and new skin
		strcpy(bot_skin, playermodel);
		strcat(bot_skin, playerskin);
	}
	else
	{ // non-team
		if ( strlen(skin2) == 0 )
		{ // if no skin yet, choose model/skin randomly
			if ( rand() & 1 )
				sprintf(bot_skin,"martianenforcer/red");
			else
				sprintf(bot_skin,"martianenforcer/blue");
		}
		else
			strcpy(bot_skin,skin2);
	}

	// initialise userinfo
	memset( userinfo, 0, MAX_INFO_STRING );

	// add bot's name\model/skin\hand to userinfo
	Info_SetValueForKey (userinfo, "name", bot_name);
	Info_SetValueForKey (userinfo, "skin", bot_skin);
	Info_SetValueForKey (userinfo, "hand", "2"); // bot is center handed for now!

}

/*
 * ACESP_ClientConnect
 *
 * bot version of p_client.c::ClientConnect()
 *
 */
qboolean ACESP_ClientConnect( edict_t *pbot )
{
	edict_t *pclient;
	int clients;
	char *name = pbot->client->pers.netname;

	// check for collison with a player name
	clients = game.maxclients;
	for ( pclient = &g_edicts[1]; clients--; pclient++ )
	{
		if ( pclient->inuse && pclient != pbot &&
				!strcmp( pclient->client->pers.netname, name  ) )
		{ //
			return false;
		}
	}


	return true;
}

/*
 * ACESP_SpawnBot
 *
 * initial spawn from LoadBots() or server command
 *
 */
qboolean ACESP_SpawnBot (char *name, char *skin, char *userinfo)
{
	char new_userinfo[MAX_INFO_STRING];
	char bot_configfilename[MAX_OSPATH];
	edict_t *pbot;
	qboolean connect_allowed = false;
	int i;


	pbot = ACESP_FindFreeClient ();
	if (!pbot)
	{ // no free client record
		gi.dprintf("Server is full, maxclients is %d\n", game.maxclients);
		return false;
	}
	// there is a free client record, use it
	pbot->inuse = true;
	pbot->is_bot = true;

	if ( userinfo == NULL )
	{ // team bot or "sv addbot", generate userinfo.
		// for team games, select team
		ACESP_SetName( pbot, name, skin, new_userinfo );
	}
	else
	{ // non-team bot , userinfo from *.tmp file
		// copy to local userinfo record
		memcpy( new_userinfo, userinfo, MAX_INFO_STRING );
	}
	// store userinfo in client
	memcpy( pbot->client->pers.userinfo, new_userinfo, MAX_INFO_STRING );
	// set name from userinfo
	memcpy( pbot->client->pers.netname, Info_ValueForKey( new_userinfo, "name"),
				sizeof( pbot->client->pers.netname ) );

	// attempt connection to server
	connect_allowed = ACESP_ClientConnect( pbot );
	if( !connect_allowed )
	{
		/* Tony: Sometimes bots are refused entry to servers - give up gracefully */
		// safe_bprintf (PRINT_MEDIUM, "Bot was refused entry to server.\n");
		gi.dprintf("%s (bot) failed connection to server\n", pbot->client->pers.netname );
		// release client record and exit
		pbot->inuse = false;
		pbot->is_bot = false;
		return false;
	}
	pbot->client->pers.connected = true;
	gi.dprintf("%s (bot) connected\n", pbot->client->pers.netname );

	// some basic initial value setting
	G_InitEdict( pbot );
	InitClientResp( pbot->client);

	// initial spawn
	/*
	 * on initial spawn, load bot configuration
	 * from botinfo/<botname>.cfg file
	 * ReadConfig sets defaults if there is no such file.
	 */
	sprintf (bot_configfilename, "botinfo/%s.cfg", pbot->client->pers.netname);
	ACECO_ReadConfig (bot_configfilename);

	//set config items
	pbot->skill = botvals.skill;
	strcpy (pbot->faveweap, botvals.faveweap);
	for (i = 1; i < 10; i++)
		pbot->weapacc[i] = botvals.weapacc[i];
	pbot->awareness = botvals.awareness;
	memcpy (pbot->chatmsg, botvals.chatmsg, sizeof (pbot->chatmsg));

	/*
	 * adjust skill according to cvar. Single Player menu selections
	 *  force the cvar.
	 *   0 : forces all to 0 skill (single player easy)
	 *   1 : skill is cfg setting  (single player medium)
	 *   2 : skill is cfg setting setting plus 1 (single player hard)
	 *   3 : forces all to skill 3 (single player ultra)
	 */
	if (skill->integer == 0 || skill->integer == -1)
	{
		pbot->skill = skill->integer; //dumb as a box of rocks
	}
	else if (skill->integer == 2)
	{
		pbot->skill += 1;
		if (pbot->skill > 3)
			pbot->skill = 3;
	}
	else if (skill->integer >= 3)
	{
		pbot->skill = 3;
	}

	/*
	 * clear the weapon accuracy statistics.
	 * for testing aim related settings.
	 */
	for (i = 0; i < 9; i++)
	{
		pbot->client->resp.weapon_shots[i] = 0;
		pbot->client->resp.weapon_hits[i] = 0;
	}
	
	// On initial spawn, hold off for up to three seconds before releasing into game
	pbot->think = ACESP_HoldSpawn;
	pbot->nextthink = level.time + random () * 3.0; // up to three seconds

	ACESP_SaveBots(); // update bots.tmp and clients bot information

	if (g_duel->integer)
		ClientPlaceInQueue (pbot);

	// make sure all view stuff is valid
	ClientEndServerFrame( pbot );

	ACEAI_PickLongRangeGoal( pbot ); // pick a new goal

	return true;
}

///////////////////////////////////////////////////////////////////////
// Remove/Kick Bots
///////////////////////////////////////////////////////////////////////

/*===
 remove_bot()

 common routine for removal or kick of a bot
 adapted from player_die(), and previous ACESP_RemoveBot(), ACESP_KickBot()

=== */
static void remove_bot( edict_t *bot )
{

	VectorClear( bot->avelocity );

	if ( bot->in_vehicle )
	{
		VehicleDeadDrop( bot );
	}
	if(ctf->value)
	{
		CTFDeadDropFlag(bot, NULL);
	}
	
	if ( g_duel->integer )
	{// duel mode, we need to bump people down the queue if its the player in game leaving
		MoveClientsDownQueue(bot);
		if (player_participating (bot))
		{ // bot was in duel
			int j;
			for ( j = 1; j <= game.maxclients; j++)
			{ // clear scores of other players
				if ( g_edicts[j].inuse && g_edicts[j].client )
					g_edicts[j].client->resp.score = 0;
			}
		}
	}

	bot->inuse = false;
	bot->solid = SOLID_NOT;
	bot->classname = "disconnected";

	bot->s.modelindex = 0;
	bot->s.modelindex2= 0;
	bot->s.modelindex3 = 0;
	bot->s.modelindex4 = 0;
	bot->s.angles[0] = 0;  // ?
	bot->s.angles[2] = 0;  // ?
	bot->s.sound = 0;
	bot->client->weapon_sound = 0;
	bot->s.effects = 0;

	Player_ResetPowerups (bot);

	// clear inventory
	memset( bot->client->pers.inventory, 0, sizeof(bot->client->pers.inventory));

	bot->client->pers.connected = false;

	// particle effect for exit from game
	gi.WriteByte (svc_muzzleflash);
	gi.WriteShort (bot-g_edicts);
	gi.WriteByte (MZ_LOGOUT);
	gi.multicast (bot->s.origin, MULTICAST_PVS);

	gi.configstring( CS_PLAYERSKINS + ( ((ptrdiff_t)(bot - g_edicts))-1 ), "");

	// unlink from world
	gi.unlinkentity (bot);

}

/*
======
 ACESP_RemoveBot

 remove by server command "sv removebot <name>|all"
======
*/
void ACESP_RemoveBot(char *name)
{
	edict_t *pbot;
	int clients;
	qboolean allbots;
	qboolean freed = false;

	allbots = strcmp( name, "all" ) == 0;

	clients = game.maxclients;
	for ( pbot = &g_edicts[1]; clients--; pbot++ )
	{
		if ( pbot->inuse && pbot->is_bot
				&& (allbots || G_NameMatch(	pbot->client->pers.netname, name )))
		{
			remove_bot( pbot );
			safe_cprintf( NULL, PRINT_HIGH, "%s removed\n", pbot->client->pers.netname );
			freed = true;
		}
	}
	if ( !allbots && !freed )
	{
		safe_cprintf( NULL, PRINT_HIGH, "%s not found, not removed\n", name );
	}

	// update bots.tmp and client bot information
	ACESP_SaveBots();
}

/*
======
 ACESP_KickBot

 remove by auto bot kick (cvar:sv_botkickthreshold)
 or by "callvote kick <botname>"

======
 */
void ACESP_KickBot( edict_t *bot )
{

	remove_bot( bot );
	safe_bprintf (PRINT_MEDIUM, "%s (bot) kicked\n", bot->client->pers.netname);

	// update bots.tmp and client bot information
	ACESP_SaveBots();
}

