/*
* Copyright (C) 1997-2001 Id Software, Inc.
* Copyright (C) 2002 The Quakeforge Project.
* Copyright (C) 2006 Quake2World.
* Copyright (C) 2010 COR Entertainment, LLC.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or(at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*
* See the GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "client.h"

#if defined HAVE_UNISTD_H
#include <unistd.h>
#endif

#if defined HAVE_UNLINK && !defined HAVE__UNLINK
#define _unlink unlink
#endif

#include "curl/curl.h"
static CURLM *curlm;
static CURL *curl;

// generic encapsulation for common http response codes
typedef struct response_s {
	int code;
	char *text;
} response_t;

response_t responses[] = {
	{301, "Moved permanently"},
	{400, "Bad request"}, 
	{401, "Unauthorized"}, 
	{403, "Forbidden"},
	{404, "Not found"}, 
	{500, "Internal server error"},
	{0, NULL}
};

char curlerr[MAX_STRING_CHARS];  // curl's error buffer

char url[MAX_OSPATH];  // remote url to fetch from
char dnld_file[MAX_OSPATH];  // local path to save to

long status, length;  // for current transfer
qboolean success;

typedef enum
{
	default_1,
	default_2,
	server_custom
} downloadhost_t;

downloadhost_t currentHost;

/*
CL_HttpDownloadRecv
*/
size_t CL_HttpDownloadRecv(void *buffer, size_t size, size_t nmemb, void *p){
   return fwrite(buffer, size, nmemb, cls.download);
}

/*
CL_HttpDownload

Queue up an http download-- get a specified file from a specified HTTP server.
We use cURL's multi interface, even tho we only ever perform one download at a
time, because it is non-blocking.
*/
static qboolean CL_HttpDownloadFromHost (downloadhost_t host, const char *filename, qboolean useGameFolder)
{
	const char *hostname;
	char game[64];

	if(!curlm)
		return false;

	if(!curl)
		return false;

	// cls.downloadtempname created by CL_CheckOrDownloadFile in cl_parse.c
	CL_DownloadFileName (dnld_file, sizeof(dnld_file), cls.downloadtempname);

	FS_CreatePath(dnld_file);  // create the directory

	if(!(cls.download = fopen(dnld_file, "wb"))){
		Com_Printf("Failed to open %s.\n", dnld_file);
		return false;  // and open the file
	}

	cls.downloadhttp = true;
	currentHost = host;

	memset(game, 0, sizeof(game));  // resolve gamedir
	strncpy(game, Cvar_VariableString("game"), sizeof(game) - 1);

	if(!strlen(game))  // use default if not set
		strcpy(game, "arena");

	switch (host)
	{
		case default_1:
			hostname = DEFAULT_DOWNLOAD_URL_1;
			break;
		case default_2:
			hostname = DEFAULT_DOWNLOAD_URL_2;
			break;
		case server_custom:
			hostname = cls.downloadurl;
			break;
	}
	
	memset(url, 0, sizeof(url));  // construct url
	if (useGameFolder) {
		Com_sprintf(url, sizeof(url) - 1, "%s/%s/%s", hostname, game, filename);
	} else {
		Com_sprintf(url, sizeof(url) - 1, "%s/%s", hostname, filename);
	}

	// set handle to default state
	curl_easy_reset(curl);
	
	// Set Http version to 1.1, somehow this seems to be needed for the multi-download
	if (curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, (long) CURL_HTTP_VERSION_1_1) != CURLE_OK) return false;

	// set url from which to retrieve the file
	if (curl_easy_setopt(curl, CURLOPT_URL, url) != CURLE_OK) return false;

	// Follow redirects to https - but this doesn't seem to be working (the two fixed urls are now both https, but the custom url might be http)
	if (curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L) != CURLE_OK) return false;
	if (curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3L) != CURLE_OK) return false;
	
	// Don't verify that the host matches the certificate
	if (curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L) != CURLE_OK) return false;

	// time out in 5s
	if (curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5) != CURLE_OK) return false;

	// set error buffer so we may print meaningful messages
	memset(curlerr, 0, sizeof(curlerr));
	if (curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlerr) != CURLE_OK) return false;

	// set the callback for when data is received
	if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CL_HttpDownloadRecv) != CURLE_OK) return false;

	if (curl_multi_add_handle(curlm, curl) != CURLM_OK) return false;

	return true;
}

/*
CL_HttpDownload

Queue up an http download.  The url is resolved from cls.downloadurl and
the current gamedir.  We use cURL's multi interface, even tho we only ever
perform one download at a time, because it is non-blocking.
*/
qboolean CL_HttpDownload(void)
{
	return CL_HttpDownloadFromHost (default_1, cls.downloadname, true);
}

/*
Download map pack from specific location. Don't use the game folder in the url.
*/
qboolean CL_HttpDownloadMapPack(char *host)
{
	strcpy(cls.downloadurl, host);
	return CL_HttpDownloadFromHost (server_custom, cls.downloadname, false);
}


/*
CL_HttpResponseCode
*/
char *CL_HttpResponseCode(long code){
	int i = 0;
	static char codeAsString[15];

	while(responses[i].code){
		if(responses[i].code == code)
			return responses[i].text;
		i++;
	}
	memset(codeAsString, 0, sizeof(codeAsString));
	Com_sprintf(codeAsString, sizeof(codeAsString) - 1, "Unknown - %ld", code);
	return codeAsString;
}


/*
CL_HttpDownloadCleanup

If a download is currently taking place, clean it up.  This is called
both to finalize completed downloads as well as abort incomplete ones.
*/
void CL_HttpDownloadCleanup(){
	char *errorMessage;
	char *statusDescription;

	if(!cls.download || !cls.downloadhttp)
		return;

	(void)curl_multi_remove_handle(curlm, curl);  // cleanup curl

	fclose(cls.download);  // always close the file
	cls.download = NULL;
	
	cls.downloadpercent = 0;
	cls.downloadhttp = false;

	statusDescription = CL_HttpResponseCode(status);
	status = length = 0;

	if(success)
	{
		CL_DownloadComplete ();
		cls.downloadname[0] = 0;
		success = false;
	}
	else
	{  // retry via legacy udp download

		errorMessage = strlen(curlerr) ? curlerr : statusDescription;
	
		if (cls.downloadfromcommand)
			Com_Printf ("Failed to download %s from %s via HTTP: %s.\n", cls.downloadname, url, errorMessage);
		else
			Com_DPrintf ("Failed to download %s from %s via HTTP: %s.\n", cls.downloadname, url, errorMessage);
		
		_unlink(dnld_file);  // delete partial or empty file
		
		// determine the next download source to try. (currentHost is the
		// source that just failed.)
		switch (currentHost)
		{
			case default_1:
				Com_DPrintf ("Trying next hardcoded HTTP.\n");
				CL_HttpDownloadFromHost (currentHost+1, cls.downloadname, true);
				break;
			case default_2:
				if (	cls.downloadurl[0] &&
						strcmp (cls.downloadurl, DEFAULT_DOWNLOAD_URL_1) &&
						strcmp (cls.downloadurl, DEFAULT_DOWNLOAD_URL_2))
				{
					Com_DPrintf ("Trying server custom HTTP.\n");
					CL_HttpDownloadFromHost (currentHost+1, cls.downloadname, true);
					break;
				}
			case server_custom:
				if (cls.state >= ca_connected)
				{
					Com_DPrintf ("Trying UDP.\n");
					MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
					MSG_WriteString(&cls.netchan.message, va("download %s", cls.downloadname));
				}
				cls.downloadfromcommand = false;
				break;
		}
	}
}



/*
CL_HttpDownloadThink

Process the pending download by giving cURL some time to think.
Poll it for feedback on the transfer to determine what action to take.
If a transfer fails, stuff a stringcmd to download it via UDP.  Since
we leave cls.download.tempname in tact during our cleanup, when the
download is parsed back in the client, it will fopen and begin.
*/
void CL_HttpDownloadThink(void){
	CURLMsg *msg;
	int i;
	int runt;

	if (!cls.download)
		return;  // nothing to do

	// process the download as long as data is avaialble
	while(curl_multi_perform(curlm, &i) == CURLM_CALL_MULTI_PERFORM){}

	// fail fast on any curl error
	if(strlen(curlerr)){
		CL_HttpDownloadCleanup();
		return;
	}

	// check for http status code
	if(!status){
		if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status) != CURLE_OK || 
		(status > 0 && status != 200)){  // 404, 403, etc..
			CL_HttpDownloadCleanup();
			return;
		}
	}

	// check for completion
	while((msg = curl_multi_info_read(curlm, &i)))
	{
		if(msg->msg == CURLMSG_DONE)
		{
			//not so fast, curl gives false positives sometimes
			runt = FS_LoadFile (cls.downloadtempname, NULL);
			if(runt > 2048) //the curl bug produces a 2kb chunk of data
			{ 
				success = true;
				CL_HttpDownloadCleanup();
				CL_RequestNextDownload();
			}
			else
			{
				success = false;
				CL_HttpDownloadCleanup();
			}
			return;
		}
	}
}


/*
CL_InitHttpDownload
*/
void CL_InitHttpDownload(void)
{
	
	if(!(curlm = curl_multi_init()))
		return;

	if(!(curl = curl_easy_init()))
		return;
}


/*
CL_ShutdownHttpDownload
*/
void CL_ShutdownHttpDownload(void){
	CL_HttpDownloadCleanup();

	curl_easy_cleanup(curl);
	curl = NULL;

	(void)curl_multi_cleanup(curlm);
	curlm = NULL;
}
