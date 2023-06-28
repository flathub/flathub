/*
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

#include "client.h"
#include "qcommon/md5.h"

#if defined HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "curl/curl.h"
CURLM *curlm;
CURL *curl;

#define STAT_PROTOCOL 1

extern cvar_t *cl_master;
extern cvar_t *cl_stats_server;

extern cvar_t *name;
extern cvar_t *stats_password;
extern cvar_t *pw_hashed;

static char szVerificationString[64];
static char *cpr; // mostly for unused result warnings

steamstats_t stStats;

static FILE* statsdb_open( const char* mode )
{
	FILE* file;
	char pathbfr[MAX_OSPATH];

#if defined WIN32_VARIANT
	char *appData = getenv("AppData");
	Com_sprintf (pathbfr, sizeof(pathbfr)-1, "%s/AAWoM/%s", appData, "stats.db");
#else
	Com_sprintf (pathbfr, sizeof(pathbfr)-1, "%s/%s", FS_Gamedir(), "stats.db");
#endif

	file = fopen( pathbfr, mode );

	return file;
}

static size_t write_data(const void *buffer, size_t size, size_t nmemb, void *userp)
{
	FILE* file;
	size_t bytecount = 0;

	file = statsdb_open( "a" ); //append, don't rewrite

	if(file) {
		//write buffer to file
		bytecount = fwrite( buffer, size, nmemb, file );
		fclose(file);
	}
	return bytecount;
}

//get the stats database
void STATS_getStatsDB( void )
{
	FILE* file;
	char statserver[128];

	CURL* easyhandle = curl_easy_init() ;

	file = statsdb_open( "w" ); //create new, blank file for writing
	if(file)
		fclose(file);

	Com_sprintf(statserver, sizeof(statserver), "%s%s", cl_stats_server->string, "/playerrank.db");

	curl_easy_setopt(easyhandle, CURLOPT_URL, statserver);

	// Set Http version to 1.1, somehow this seems to be needed for the multi-download
	curl_easy_setopt(easyhandle, CURLOPT_HTTP_VERSION, (long) CURL_HTTP_VERSION_1_1);

	// Follow redirects to https - but this doesn't seem to be working
	curl_easy_setopt(easyhandle, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(easyhandle, CURLOPT_MAXREDIRS, 3L);
	
	// Don't verify that the host matches the certificate
	curl_easy_setopt(easyhandle, CURLOPT_SSL_VERIFYHOST, 0L);

	// time out in 5s
	curl_easy_setopt(easyhandle, CURLOPT_CONNECTTIMEOUT, 5);

	curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, write_data);

	curl_easy_perform(easyhandle);

	curl_easy_cleanup(easyhandle);
}

//parse the stats database, looking for player match
PLAYERSTATS getPlayerRanking ( PLAYERSTATS player )
{
	FILE* file;
	char name[34], points[32], frags[32], totalfrags[32], time[16], totaltime[16], ip[32], poll[16], remote_address[21];
	int foundplayer = false;

	//open file,
	file = statsdb_open( "r" ) ;

	if(file != NULL) {

		//parse it, and compare to player name
		while(player.ranking < 1000) {

			//name. upto 31 chars total, 15 are printable.
			//  fgets needs at least 33 chars. 31 + newline + terminating nul
			cpr = fgets(name, sizeof(name), file);
			if ( cpr == NULL )
			{ // end-of-file
				break;
			}
			name[strlen(name) - 2] = 0; //truncate line feed
			//remote address
			cpr = fgets(remote_address, sizeof(remote_address), file);
			//points
			cpr = fgets(points, sizeof(points), file);
			//frags
			cpr = fgets(frags, sizeof(frags), file);
			//total frags
			cpr = fgets(totalfrags, sizeof(totalfrags), file);
			if(!strcmp(player.playername, name))
				player.totalfrags = atoi(totalfrags);
			//current time in poll
			cpr = fgets(time, sizeof(time), file);
			//total time
			cpr = fgets(totaltime, sizeof(totaltime), file);
			if(!strcmp(player.playername, name))
				player.totaltime = atof(totaltime);
			//last server.ip
			cpr = fgets(ip, sizeof(ip), file);
			//what poll
			cpr = fgets(poll, sizeof(poll), file);

			player.ranking++;

			if(!Q_strcasecmp(player.playername, name)) {
				foundplayer = true;
				break; //get out we are done
			}
		}
		fclose(file);
	}

	if(!foundplayer) {
		player.ranking = 1000;
		player.totalfrags = 0;
		player.totaltime = 1;
	}

	return player;
}

//get player info by rank
PLAYERSTATS getPlayerByRank ( int rank, PLAYERSTATS player )
{
	FILE* file;
	char name[32], points[32], frags[32], totalfrags[32], time[16], totaltime[16], ip[32], poll[16], remote_address[21];
	int foundplayer = false;

	//open file,
	file = statsdb_open( "r" ) ;

	if(file != NULL) {

		//parse it, and compare to player name
		while(player.ranking < 1000) {

			//name
			cpr = fgets(name, sizeof(name), file);
			strcpy(player.playername, name);
			player.playername[strlen(player.playername)-2] = 0; //remove line feed
			//remote address
			cpr = fgets(remote_address, sizeof(remote_address), file);
			//points
			cpr = fgets(points, sizeof(points), file);
			//frags
			cpr = fgets(frags, sizeof(frags), file);
			//total frags
			cpr = fgets(totalfrags, sizeof(totalfrags), file);
			player.totalfrags = atoi(totalfrags);
			//current time in poll
			cpr = fgets(time, sizeof(time), file);
			//total time
			cpr = fgets(totaltime, sizeof(totaltime), file);
			player.totaltime = atof(totaltime);
			//last server.ip
			cpr = fgets(ip, sizeof(ip), file);
			//what poll
			cpr = fgets(poll, sizeof(poll), file);

			player.ranking++;

			if(player.ranking == rank) {
				foundplayer = true;
				break; //get out we are done
			}
		}
		fclose(file);
	}

	if(!foundplayer) {
		player.totalfrags = 0;
		player.totaltime = 1;
		strcpy(player.playername, "unknown");
	}

	return player;
}

//This next section deals with account verification
//Passwords are hashed before storing or sending, raw strings are never exposed.

//Send request for login(vstrings are random, unique strings that the server generates for each player - provides an extra layer of encryption)
void STATS_RequestVerification (void)
{
	char *requeststring;
	netadr_t adr;

	if(currLoginState.validated)
		return; //already validated

	NET_Config (true);

	currLoginState.requestType = STATSLOGIN;

	requeststring = va("requestvstring\\\\%i\\\\%s", STAT_PROTOCOL, name->string);

	if( NET_StringToAdr( cl_master->string, &adr ) ) {
		if( !adr.port )
			adr.port = BigShort( PORT_STATS );
		Netchan_OutOfBandPrint( NS_CLIENT, adr, requeststring );
	}
	else
	{
		Com_Printf( "Bad address: %s\n", cl_master->string);
	}
}

//Send request for password change
void STATS_RequestPwChange (void)
{
	char *requeststring;
	netadr_t adr;

	currLoginState.validated = false; //force new login with new password

	NET_Config (true);

	currLoginState.requestType = STATSPWCHANGE;

	requeststring = va("requestvstring\\\\%i\\\\%s", STAT_PROTOCOL, name->string);

	if( NET_StringToAdr( cl_master->string, &adr ) ) {
		if( !adr.port )
			adr.port = BigShort( PORT_STATS );
		Netchan_OutOfBandPrint( NS_CLIENT, adr, requeststring );
	}
	else
	{
		Com_Printf( "Bad address: %s\n", cl_master->string);
	}
}

void STATS_EncryptPassword(void)
{
	char szPassword[256];
	char szPassHash[256];
	char szPassHash2[256];

	//salt
	Com_sprintf(szPassword, sizeof(szPassword), "%s%s", stats_password->string, szVerificationString);
	Com_MD5HashString (szPassword, strlen(szPassword), szPassHash, sizeof(szPassHash));

	//salt
	Com_sprintf(szPassword, sizeof(szPassword), "%s%s", szPassHash, szVerificationString);

	Com_MD5HashString (szPassword, strlen(szPassword), szPassHash, sizeof(szPassHash));
	Com_HMACMD5String(szPassHash, strlen(szPassHash), szVerificationString, strlen(szVerificationString), szPassHash2, sizeof(szPassHash2));

	Cvar_FullSet("stats_pw_hashed", "1", CVAR_PROFILE);
	Cvar_FullSet("stats_password", szPassHash2, CVAR_PROFILE);

	currLoginState.hashed = true;

	//get new hashed password
	stats_password = Cvar_Get("stats_password", "password", CVAR_PROFILE);
}

//Send stats server authentication pw
void STATS_AuthenticateStats (char *vstring)
{
	char *requeststring;
	netadr_t adr;

	Q_strncpyz2(szVerificationString, vstring, sizeof(szVerificationString));

	NET_Config (true);

	//use md5 encryption on password, never send or store a raw password!
	if(!pw_hashed->integer)
	{
		STATS_EncryptPassword();
	}

	requeststring = va("login\\\\%i\\\\%s\\\\%s\\\\%s\\\\", STAT_PROTOCOL, name->string, stats_password->string, szVerificationString );

	if( NET_StringToAdr( cl_master->string, &adr ) ) {
		if( !adr.port )
			adr.port = BigShort( PORT_STATS );
		Netchan_OutOfBandPrint( NS_CLIENT, adr, requeststring );
	}
	else
	{
		Com_Printf( "Bad address: %s\n", cl_master->string);
	}
}

//Send password change request to server(this is to be called from the menu when a password is edited)
void STATS_ChangePassword (char *vstring)
{
	char *requeststring;
	netadr_t adr;

	Q_strncpyz2(szVerificationString, vstring, sizeof(szVerificationString));

	NET_Config (true);

	STATS_EncryptPassword();

	requeststring = va("changepw\\\\%i\\\\%s\\\\%s\\\\%s\\\\%s\\\\", STAT_PROTOCOL, name->string, currLoginState.old_password, stats_password->string, szVerificationString);

	if( NET_StringToAdr( cl_master->string, &adr ) ) {
		if( !adr.port )
			adr.port = BigShort( PORT_STATS );
		Netchan_OutOfBandPrint( NS_CLIENT, adr, requeststring );
	}
	else
	{
		Com_Printf( "Bad address: %s\n", cl_master->string);
	}
}

//Logout of stats server
void STATS_Logout (void)
{
	char *requeststring;
	netadr_t adr;

	if(!currLoginState.validated)
		return; //no point in logging out, we were never validated!

	NET_Config (true);

	//use md5 encryption on password, never send or store a raw password!
	if(!pw_hashed->integer)
	{
		STATS_EncryptPassword();
	}

	requeststring = va("logout\\\\%i\\\\%s\\\\%s\\\\%s\\\\", STAT_PROTOCOL, name->string, stats_password->string, szVerificationString );

	if( NET_StringToAdr( cl_master->string, &adr ) ) {
		if( !adr.port )
			adr.port = BigShort( PORT_STATS );
		Netchan_OutOfBandPrint( NS_CLIENT, adr, requeststring );
	}
	else
	{
		Com_Printf( "Bad address: %s\n", cl_master->string);
	}
}

//Steam stats section
void STATS_ST_Init (void)
{
	// Set everything to 0 - we will only be tracking this session's numbers.
	stStats.g_NumBaseKills = 0;
	stStats.g_NumFlagCaptures = 0;
	stStats.g_NumFlagReturns = 0;
	stStats.g_NumGames = 0;
	stStats.g_NumKillStreaks = 0;
	stStats.g_NumRampages = 0;
	stStats.g_NumUnstoppables = 0;
	stStats.g_NumGodLikes = 0;
	stStats.g_NumHeadShots = 0;
	stStats.g_NumMindErases = 0;
	stStats.g_NumDisintegrates = 0;
	stStats.g_NumViolations = 0;
	stStats.g_NumMidAirShots = 0;
	stStats.g_NumKills = 0;
	stStats.g_NumWins = 0;
}

void STATS_ST_Write (void)
{
	// Write out file that our steam client will read in
	FILE* file;
	char statsFile[MAX_QPATH];
	size_t sz;

#if defined WIN32_VARIANT
	char *appData = getenv("AppData");
	sprintf(statsFile, "%s/AAWoM/swdata.db", appData);
#else
	sprintf(statsFile, "%s/swdata.db", GAME_GAMEDATA);
#endif

    file = fopen(statsFile, "wb");
    if (file != NULL) 
	{		
		sz = fwrite(&stStats.g_NumBaseKills, sizeof(int), 1, file); 
		sz = fwrite(&stStats.g_NumFlagCaptures, sizeof(int), 1, file); 
		sz = fwrite(&stStats.g_NumFlagReturns, sizeof(int), 1, file);
		sz = fwrite(&stStats.g_NumGames, sizeof(int), 1, file); 
		sz = fwrite(&stStats.g_NumKillStreaks, sizeof(int), 1, file);
		sz = fwrite(&stStats.g_NumRampages, sizeof(int), 1, file);
		sz = fwrite(&stStats.g_NumUnstoppables, sizeof(int), 1, file);
		sz = fwrite(&stStats.g_NumGodLikes, sizeof(int), 1, file); 
		sz = fwrite(&stStats.g_NumHeadShots, sizeof(int), 1, file); 
		sz = fwrite(&stStats.g_NumMindErases, sizeof(int), 1, file);
		sz = fwrite(&stStats.g_NumDisintegrates, sizeof(int), 1, file);
		sz = fwrite(&stStats.g_NumViolations, sizeof(int), 1, file);
		sz = fwrite(&stStats.g_NumMidAirShots, sizeof(int), 1, file);
		sz = fwrite(&stStats.g_NumKills, sizeof(int), 1, file); 
		sz = fwrite(&stStats.g_NumWins, sizeof(int), 1, file); 

		fclose(file);
	}
}
