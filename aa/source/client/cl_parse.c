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
// cl_parse.c  -- parse a message received from the server

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "client.h"
#include "qcommon/qcommon.h"
#include "minizip/unzip.h"

char *svc_strings[256] =
{
	"svc_bad",

	"svc_muzzleflash",
	"svc_muzzlflash2",
	"svc_temp_entity",
	"svc_layout",
	"svc_inventory",

	"svc_nop",
	"svc_disconnect",
	"svc_reconnect",
	"svc_sound",
	"svc_print",
	"svc_stufftext",
	"svc_serverdata",
	"svc_configstring",
	"svc_spawnbaseline",
	"svc_centerprint",
	"svc_download",
	"svc_playerinfo",
	"svc_packetentities",
	"svc_deltapacketentities",
	"svc_frame"
};

static size_t szr; // just for unused result warnings

extern	cvar_t *allow_overwrite_maps;

qboolean CL_ExtractFiles(char *zipFilename);

/*
==============
Q_strncpyz
==============
*/
void Q_strncpyz( char *dest, const char *src, size_t size )
{
#if defined HAVE_STRLCPY
	strlcpy( dest, src, size );
#else
	if( size )
	{
		while( --size && (*dest++ = *src++) );
		*dest = '\0';
	}
#endif
}

//=============================================================================

void CL_DownloadFileName (char *dest, int destlen, char *fn)
{
	memset(dest, 0, destlen);
	Com_sprintf (dest, destlen-1, "%s/%s", FS_Gamedir(), fn);
}

// Current download has been completed, rename temp file to main file
void CL_DownloadComplete (void)
{
	int		r;
	char	oldn[MAX_OSPATH];
	char	newn[MAX_OSPATH];
		
	// rename the temp file to it's final name
	CL_DownloadFileName(oldn, sizeof(oldn), cls.downloadtempname);
	CL_DownloadFileName(newn, sizeof(newn), cls.downloadname);
	
	r = rename (oldn, newn);
	if (r) {
		// File already exists?
		Com_Printf("failed to rename %s to %s.\n", cls.downloadtempname, cls.downloadname);
	}
	else {
		Com_Printf("Download complete: %s\n", cls.downloadname);
	}

	if (cls.downloadmappack) {
		if (CL_ExtractFiles(cls.downloadname)) {
			Com_Printf("Installed map pack %s.\n", cls.downloadname);
		}
		else {
			Com_Printf("Couldn't install map pack %s.\n", cls.downloadname);
		}

		cls.downloadmappack = false;
	}
}

/*
===============
CL_CheckOrDownloadFile

Returns true if the file exists, otherwise it attempts
to start a download from the server.
===============
*/
qboolean	CL_CheckOrDownloadFile (char *filename)
{
	FILE *fp;
	char	name[MAX_OSPATH];
	char	shortname[MAX_OSPATH];
	char    shortname2[MAX_OSPATH];
	qboolean	jpg = false;
	qboolean	md2 = false;

	if (strstr (filename, ".."))
	{
		Com_Printf ("Refusing to download a path with ..\n");
		return true;
	}

    //if pcx, strip extension and change to .jpg, we do not use .pcx anymore
    if(filename[strlen(filename)-1] == 'x')
	{
		//Filter out any potentially screwed up texture paths(meshes only reside in these folders)
		if (strncmp(filename, "models", 6) && strncmp(filename, "vehicles", 8)
			&& strncmp(filename, "maps", 4))
			return true;

		COM_StripExtension ( filename, shortname );
		sprintf(filename, "%s.jpg", shortname);
	}

	//if jpg, be sure to also try tga, if md2, also try iqm
	/* 2015-12-14 For textures, jpg extension is now default, so jpg is
	 * checked first then tga. See cl_main.c::CL_RequestNextDownload
	 */
    if(filename[strlen(filename)-2] == 'p' && filename[strlen(filename)-1] == 'g')
		jpg = true;

	if(filename[strlen(filename)-2] == 'd' && filename[strlen(filename)-1] == '2')
		md2 = true;

	if (FS_LoadFile (filename, NULL) != -1)
	{
		// it exists, no need to download
		return true;
	}

	if(jpg)
	{
		//didn't find .jpg skin, try for .tga skin
		//check for presence of a local .tga(but leave filename as original extension)
		//if we find a .tga, don't try to download anything
		COM_StripExtension ( filename, shortname );
		sprintf(shortname2, "%s.tga", shortname);
		if (FS_LoadFile (shortname2, NULL) != -1)
		{
			// it exists, no need to download
			return true;
		}
	}

	if(md2)
	{
		//didn't find .md2, try for .iqm
		//check for presence of a local .iqm(but leave filename as original extension)
		//if we find a .iqm, don't try to download anything
		COM_StripExtension ( filename, shortname );
		sprintf(shortname2, "%s.iqm", shortname);
		if (FS_LoadFile (shortname2, NULL) != -1)
		{
			// it exists, no need to download
			return true;
		}
	}

	strcpy (cls.downloadname, filename);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	COM_StripExtension (cls.downloadname, cls.downloadtempname);
	strcat (cls.downloadtempname, ".tmp");

	// attempt an http download if available(never try to dl game model skins here)
	if(cls.downloadurl[0] && CL_HttpDownload())
	{
		Com_Printf("HTTP download started: %s\n", cls.downloadname);
		return false;
	}

//ZOID
	// check to see if we already have a tmp for this file, if so, try to resume
	// open the file if not opened yet
	CL_DownloadFileName(name, sizeof(name), cls.downloadtempname);

//	FS_CreatePath (name);

	fp = fopen (name, "r+b");
	if (fp)
	{ // it exists
		int len;

		len = FS_filelength( fp );
		cls.download = fp;

		// give the server an offset to start the download
		Com_Printf ("Resuming %s\n", cls.downloadname);
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message,
			va("download %s %i", cls.downloadname, len));
	} else
	{
		Com_Printf ("Downloading %s\n", cls.downloadname);
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message,
			va("download %s", cls.downloadname));
	}

	cls.downloadnumber++;

	send_packet_now = true;
	return false;
}

/*
===============
CL_Download_f

Request a download from the server
===============
*/
void	CL_Download_f (void)
{
	char filename[MAX_OSPATH];
	qboolean udp, not_connected, not_enough_args;

	not_enough_args = Cmd_Argc () != 2;
	udp = !Q_strcasecmp (Cmd_Argv (0), "download");
	not_connected = udp && cls.state < ca_connected;

	if (not_connected)
		Com_Printf ("download command cannot be used unless connected to a server! Try downloadhttp instead.\n");

	if (cls.download)
		Com_Printf ("Already downloading something.\n");

	if (not_enough_args || not_connected || cls.download)
	{
		Com_Printf ("Usage: download <filename> - download from connected game server via UDP\n");
		Com_Printf ("       downloadhttp <filename> - download from central HTTP repository\n");
		return;
	}

	if (udp)
	{
		Com_Printf ("The \"download\" command is kept only for compatibility\n"
					"and will not be able to download files unless they're on\n"
					"the current connected game server. Consider using the\n"
					"\"downloadhttp\" command instead.\n");
	}

	Com_sprintf(filename, sizeof(filename), "%s", Cmd_Argv(1));

	if (strstr (filename, ".."))
	{
		Com_Printf ("Refusing to download a path with ..\n");
		return;
	}

	if (FS_LoadFile (filename, NULL) != -1)
	{	// it exists, no need to download
		Com_Printf("File already exists.\n");
		return;
	}

	strcpy (cls.downloadname, filename);
	Com_Printf ("Downloading %s\n", cls.downloadname);

	// download to a temp name, and only rename
	// to the real name when done, so if interrupted
	// a runt file wont be left
	COM_StripExtension (cls.downloadname, cls.downloadtempname);
	strcat (cls.downloadtempname, ".tmp");

	if (udp)
	{
		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		MSG_WriteString (&cls.netchan.message,
			va("download %s", cls.downloadname));

		cls.downloadnumber++;
	}
	else
	{
		cls.downloadfromcommand = true;
		if (!CL_HttpDownload ())
			Com_Printf ("Error initializing http download.\n");
	}
}

/*
** If the path in filename starts with "arena/", then it returns a copy of the original string without the "arena/" part.
*/
char *RemoveArenaFolderFromPath(char *filename) {
	// Equal means that it starts with "arena/"
	if (strstr(filename, "arena/") == filename) {
		static char newFilename[MAX_OSPATH];
		strcpy(newFilename, filename + 6);
		return newFilename;
	}
	return filename;
}

/*
===============
CL_ExtractFiles

Extracts downloaded zip containing one or more maps using minizip.
The zip will be placed in the arena folder, but it also contains an arena folder.
All files will be directly extracted from the read zip file to the place where they belong and the zip file remains in the arena folder.
===============
*/

#define dir_delimiter '/'
#define READ_SIZE 8192

qboolean CL_ExtractFiles(char *zipFilename)
{
	char fullPath[MAX_OSPATH];
	
	CL_DownloadFileName(fullPath, sizeof(fullPath), zipFilename);

    // Open the zip file
    unzFile *zipfile = unzOpen(fullPath);
    if ( zipfile == NULL )
    {
        Com_Printf("Not found: %s\n", fullPath);
        return false;
    }

    // Get info about the zip file
    unz_global_info global_info;
    if (unzGetGlobalInfo(zipfile, &global_info) != UNZ_OK)
    {
        Com_Printf("Could not read file global info.\n");
        unzClose( zipfile );
        return false;
    }

    // Buffer to hold data read from the zip file.
    char read_buffer[READ_SIZE];

    // Loop to extract all files
    uLong i;
    for (i = 0; i < global_info.number_entry; ++i)
    {
        // Get info about current file.
        unz_file_info file_info;
        char originalFilename[MAX_OSPATH];
        char filename[MAX_OSPATH];
        if (unzGetCurrentFileInfo(
            zipfile,
            &file_info,
            originalFilename,
            MAX_OSPATH,
            NULL, 0, NULL, 0) != UNZ_OK)
        {
            Com_Printf("Could not read file info.\n");
            unzClose(zipfile);
            return false;
        }

		// The zip has the "arena/" folder within the root, so correct for that
		strcpy(filename, RemoveArenaFolderFromPath(originalFilename));

		CL_DownloadFileName(fullPath, sizeof(fullPath), filename);

        // Check if this entry is a directory or file.
        const size_t filename_length = strlen(filename);
        if (filename[filename_length - 1] == dir_delimiter)
        {
            // Entry is a directory, so create it.
			Sys_Mkdir(fullPath);
        }
        else if (filename[0] != 0)
        {
            // Entry is a file, so extract it.
            if (unzOpenCurrentFile( zipfile ) != UNZ_OK)
            {
                Com_Printf("Could not open file.\n");
                unzClose( zipfile );
                return false;
            }	

            if (!FS_CheckFile(fullPath) || allow_overwrite_maps->integer)
            {
				FILE *out = fopen(fullPath, "wb"); // write / binary

				int readResult = UNZ_OK;
				do    
				{
					readResult = unzReadCurrentFile(zipfile, read_buffer, READ_SIZE);
					if (readResult < 0) // Error
					{
						Com_Printf("Error: %d\n", readResult);
						unzCloseCurrentFile( zipfile );
						unzClose( zipfile );
						return false;
					}

					// Write data to file.
					if (readResult > 0) // No error
					{
						size_t writeResult;
						writeResult = fwrite(read_buffer, readResult, 1, out);
					}
				} while (readResult > 0);
				
				fclose(out);
				
				Com_Printf("Extracted %s.\n", originalFilename);
			}
        }

        unzCloseCurrentFile(zipfile);

        // Go to the next entry listed in the zip file.
        if ((i + 1) < global_info.number_entry)
        {
            if (unzGoToNextFile(zipfile) != UNZ_OK)
            {
                Com_Printf("Could not read next file.\n");
                unzClose(zipfile);
                return false;
            }
        }
    }

    unzClose(zipfile);

	return true;
}

/*
===============
CL_InstallMap

From command "installmap". Request a map pack download from alienarena.org in zip format and extract the files.
===============
*/
void CL_InstallMap (void)
{
	char filename[MAX_OSPATH];
	char fullPath[MAX_OSPATH];
	qboolean not_enough_args;

	not_enough_args = Cmd_Argc () != 2;

	if (cls.download) {
		Com_Printf ("Already downloading something.\n");
	}

	if (not_enough_args || cls.download)
	{
		Com_Printf ("Usage: installmap <filename> - Download and extract a map pack from alienarena.org.\nSee also cvar allow_overwrite_maps to force a reinstall.\n");
		return;
	}

	Com_sprintf(filename, sizeof(filename), "%s", Cmd_Argv(1));

	if (strstr(filename, ".."))
	{
		Com_Printf("Refusing to download a path with ..\n");
		return;
	}
	
	COM_StripExtension(COM_SkipPath(filename), filename);
	Com_sprintf(cls.downloadname, sizeof(cls.downloadname), "%s.zip", filename);

	CL_DownloadFileName(fullPath, sizeof(fullPath), cls.downloadname);
	
	if (FS_CheckFile(fullPath)) {
		if (!allow_overwrite_maps->integer) {
			Com_Printf("Map pack already exists. Set allow_overwrite_maps to 1 to redownload and reinstall.\n");
			cls.downloadname[0] = 0;
			return;
		}
		else {
			// Delete existing zip file
			remove(fullPath);
		}
	}

	Com_sprintf(cls.downloadtempname, sizeof(cls.downloadtempname), "%s.tmp", filename);
	cls.downloadfromcommand = true;
	cls.downloadmappack = true;

	CL_HttpDownloadMapPack(MAPPACK_URL);
	
	Com_Printf("Map pack download started: %s\n", cls.downloadname);
	
	// The extracting part will be done in CL_ExtractFiles(), called from CL_DownloadComplete()
}

/*
======================
CL_RegisterSounds
======================
*/
void CL_RegisterSounds (void)
{
	int		i;

	S_BeginRegistration ();
	CL_RegisterTEntSounds ();
	for (i=1 ; i<MAX_SOUNDS ; i++)
	{
		if (!cl.configstrings[CS_SOUNDS+i][0])
			break;
		cl.sound_precache[i] = S_RegisterSound (cl.configstrings[CS_SOUNDS+i]);
		Sys_SendKeyEvents ();	// pump message loop
	}
	S_EndRegistration ();
}


/* Helper functions for CL_ParseDownload
 */
static void jpg_to_tga()
{
		
	// substitute tga. downloadtmpname should be ok as is.
	size_t exti = strlen(cls.downloadname) - 4;
	Q_strncpyz( &cls.downloadname[exti], ".tga", 5 );

}

static qboolean is_jpg()
{
	size_t exti = strlen(cls.downloadname) - 4;
	return  !Q_strncasecmp( ".jpg", &cls.downloadname[exti], 4 );
}

/*
=====================
CL_ParseDownload

A download message has been received from the server
=====================
*/
void CL_ParseDownload (void)
{
	int		size, percent;
	char	name[MAX_OSPATH];

	// read the data
	size = MSG_ReadShort (&net_message);
	percent = MSG_ReadByte (&net_message);
	if (size < 0) //fix issues with bad data being dl'd
	{
		// if partial file resume failed, close and clear the .tmp
		if (cls.download)
		{
			fclose (cls.download);
			cls.download = NULL;
		}
		
		if ( !is_jpg() )
		{
			// Failed download was not a .jpg.
			Com_Printf ("Server does not have file %s.\n", cls.downloadname);

			//nuke the temp filename, we don't want that getting left around.
			cls.downloadtempname[0] = 0;
			cls.downloadname[0] = 0;

			CL_RequestNextDownload ();
			return;
		}
		else
		{
			/* 2016-02-22 If a jpg download failed here, start over
			 * and look for a tga.
			 */
			Com_Printf("Server does not have jpg file %s. Trying tga.\n", cls.downloadname);
			jpg_to_tga();
			if ( cls.downloadurl[0] && CL_HttpDownload() )
			{
				Com_Printf("HTTP download started: %s\n", cls.downloadname);
			}
			else
			{ // No HTTP, try UDP
				Com_Printf("Downloading %s\n", cls.downloadname);
				MSG_WriteByte(
					&cls.netchan.message, clc_stringcmd);
				MSG_WriteString(
					&cls.netchan.message, va("download %s", cls.downloadname));
			}
			return;
		}
	}

	// open the file if not opened yet
	if (!cls.download)
	{
		CL_DownloadFileName(name, sizeof(name), cls.downloadtempname);

		FS_CreatePath (name);

		cls.download = fopen (name, "wb");
		if (!cls.download)
		{
			net_message.readcount += size;
			Com_Printf ("Failed to open %s\n", cls.downloadtempname);
			CL_RequestNextDownload ();
			return;
		}
	}

	szr = fwrite (net_message.data + net_message.readcount, 1, size, cls.download);
	net_message.readcount += size;

	if (percent != 100)
	{
		// request next block
		cls.downloadpercent = percent;

		MSG_WriteByte (&cls.netchan.message, clc_stringcmd);
		SZ_Print (&cls.netchan.message, "nextdl");
		send_packet_now = true;
	}
	else
	{
		fclose (cls.download);

		cls.download = NULL;
		cls.downloadpercent = 0;
		
		CL_DownloadComplete ();

		// get another file if needed
		CL_RequestNextDownload ();
	}
}


/*
=====================================================================

  SERVER CONNECTING MESSAGES

=====================================================================
*/

/*
==================
CL_ParseServerData
==================
*/
void CL_ParseServerData (void)
{
	extern cvar_t *fs_gamedirvar;
	char *str;
	int  i;

	Com_DPrintf ("Serverdata packet received.\n");
//
// wipe the client_state_t struct
//
	CL_ClearState ();
	cls.state = ca_connected;

// parse protocol version number
	i = MSG_ReadLong (&net_message);
	cls.serverProtocol = i;

	// BIG HACK to let demos from release work with the 3.0x patch!!!
	if (Com_ServerState() && (PROTOCOL_VERSION == 34 || PROTOCOL_VERSION == 35 || PROTOCOL_VERSION == 36))
	{
	}
	else if (i != PROTOCOL_VERSION)
		Com_Error (ERR_DROP,"Server returned version %i, not %i", i, PROTOCOL_VERSION);

	cl.servercount = MSG_ReadLong (&net_message);
	cl.attractloop = MSG_ReadByte (&net_message);
	
	// Hide console for demo playback. It interferes with timedemo results and
	// is annoying when you just want to watch.
	if (cl.attractloop)
		M_ForceMenuOff ();

	// game directory
	str = MSG_ReadString (&net_message);
	strncpy (cl.gamedir, str, sizeof(cl.gamedir)-1);

	// set gamedir
	/* Pending sorting out multiple game and game plugin module issues,
	 * changing the game and gamedir cvars is not supported.
	 * NOTE: seems like servers do not always send a gamedir (default to "arena"?)
	 * 2015-12-14 ARGH...fix stupid '==' not '=' bug.
	 */
	Com_DPrintf("[game: server: %s, client: %s]\n", cl.gamedir, fs_gamedirvar->string );
	if ( cl.gamedir[0] == '\0' )
		strcpy( cl.gamedir, "arena" );

	// parse player entity number
	cl.playernum = MSG_ReadShort (&net_message);

	// get the full level name
	str = MSG_ReadString (&net_message);

	// seperate the printfs so the server message can have a color
	Com_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
	Com_Printf ("%c%s\n", 2, str);

	// need to prep refresh at next oportunity
	cl.refresh_prepped = false;
}

/*
==================
CL_ParseBaseline
==================
*/
void CL_ParseBaseline (void)
{
	entity_state_t	*es;
	int				bits;
	int				newnum;
	entity_state_t	nullstate;

	memset (&nullstate, 0, sizeof(nullstate));

	newnum = CL_ParseEntityBits ( (unsigned *)&bits );
	es = &cl_entities[newnum].baseline;
	CL_ParseDelta (&nullstate, es, newnum, bits);
}


/*
================
CL_LoadClientinfo
================
*/
void CL_LoadClientinfo (clientinfo_t *ci, char *s)
{
	int i;
	char		*t;
	char		model_name[MAX_QPATH];
	char		skin_name[MAX_QPATH];
	char		model_filename[MAX_QPATH];
	char		skin_filename[MAX_QPATH];
	char		weapon_filename[MAX_QPATH];

	strncpy(ci->cinfo, s, sizeof(ci->cinfo));
	ci->cinfo[sizeof(ci->cinfo)-1] = 0;

	// isolate the player's name
	strncpy(ci->name, s, sizeof(ci->name));
	ci->name[sizeof(ci->name)-1] = 0;
	t = strstr (s, "\\");
	if (t)
	{
		ci->name[t-s] = 0;
		s = t+1;
	}

	// isolate the model name
	strcpy (model_name, s);
	t = strstr(model_name, "/");
	if (!t)
		t = strstr(model_name, "\\");
	if (!t)
		t = model_name;
	*t = 0;

	ci->helmet = NULL; //we only worry about this in these cases of missing textures or models
	ci->lod1 = NULL;
	ci->lod2 = NULL;

	if (cl_noskins->value || *s == 0)
	{
		Com_sprintf (model_filename, sizeof(model_filename), "players/martianenforcer/tris.iqm");
		Com_sprintf (weapon_filename, sizeof(weapon_filename), "players/martianenforcer/weapon.iqm");
		Com_sprintf (skin_filename, sizeof(skin_filename), "players/martianenforcer/default.jpg");
		Com_sprintf (ci->iconname, sizeof(ci->iconname), "/players/martianenforcer/default_i.jpg");
		ci->model = R_RegisterModel (model_filename);
		ci->helmet = R_RegisterModel("players/martianenforcer/helmet.md2");
		// weapon file
		for (i = 0; i < num_cl_weaponmodels; i++)
		{
			Com_sprintf (weapon_filename, sizeof(weapon_filename), "players/martianenforcer/%s", cl_weaponmodels[i]);
			ci->weaponmodel[i] = R_RegisterModel(weapon_filename);
			if (!cl_vwep->value)
				break; // only one when vwep is off
		}
		ci->skin = R_RegisterSkin (skin_filename);
		ci->icon = R_RegisterPic (ci->iconname);
	}
	else
	{
		// isolate the skin name
		strcpy (skin_name, s + strlen(model_name) + 1);

		// model file
		Com_sprintf (model_filename, sizeof(model_filename), "players/%s/tris.iqm", model_name);
		ci->model = R_RegisterModel (model_filename);
		if (!ci->model)
		{
			Com_sprintf (model_filename, sizeof(model_filename), "players/martianenforcer/tris.iqm");
			ci->model = R_RegisterModel (model_filename);
			ci->helmet = R_RegisterModel("players/martianenforcer/helmet.iqm");
			strcpy(model_name, "martianenforcer");
		}

		// skin file
		Com_sprintf (skin_filename, sizeof(skin_filename), "players/%s/%s.jpg", model_name, skin_name);
		ci->skin = R_RegisterSkin (skin_filename);

		// if don't have a skin, it means that the model didn't have
		// it, so default
		if (!ci->skin)
		{
			strcpy(skin_name, "default");
			Com_sprintf (skin_filename, sizeof(skin_filename), "players/%s/default.jpg", model_name);
			ci->skin = R_RegisterSkin (skin_filename);
		}

		// weapon file
		for (i = 0; i < num_cl_weaponmodels; i++)
		{
			Com_sprintf (weapon_filename, sizeof(weapon_filename), "players/%s/%s", model_name, cl_weaponmodels[i]);
			ci->weaponmodel[i] = R_RegisterModel(weapon_filename);
			if (!ci->weaponmodel[i] == 0) {
				Com_sprintf (weapon_filename, sizeof(weapon_filename), "players/martianenforcer/%s", cl_weaponmodels[i]);
				ci->weaponmodel[i] = R_RegisterModel(weapon_filename);
			}
			if (!cl_vwep->value)
				break; // only one when vwep is off
		}

		// icon file
		Com_sprintf (ci->iconname, sizeof(ci->iconname), "/players/%s/%s_i.jpg", model_name, skin_name);
		ci->icon = R_RegisterPic (ci->iconname);
	}

	//check for level of detail models
	if (cl_noskins->value || *s == 0)
		strcpy(model_name, "martianenforcer");

	Com_sprintf(model_filename, sizeof(model_filename), "players/%s/lod1.iqm", model_name);
	i = 0;
	do
		model_filename[i] = tolower(model_filename[i]);
	while (model_filename[i++]);

	ci->lod1 = R_RegisterModel(model_filename);

	Com_sprintf(model_filename, sizeof(model_filename), "players/%s/lod2.iqm", model_name);
	i = 0;
	do
		model_filename[i] = tolower(model_filename[i]);
	while (model_filename[i++]);

	ci->lod2 = R_RegisterModel(model_filename);

	// must have loaded all data types to be valid
	if (!ci->skin || !ci->icon || !ci->model || !ci->weaponmodel[0])
	{
		ci->skin = NULL;
		ci->icon = NULL;
		ci->model = NULL;
		ci->weaponmodel[0] = NULL;
		return;
	}

}

/*
================
CL_ParseClientinfo

Load the skin, icon, and model for a client
================
*/
void CL_ParseClientinfo (int player)
{
	char			*s;
	clientinfo_t	*ci;

	s = cl.configstrings[player+CS_PLAYERSKINS];

	ci = &cl.clientinfo[player];

	CL_LoadClientinfo (ci, s);
}

static void CL_ParseTaunt (const char *s)
{
	int l, j;
	char tauntsound[MAX_OSPATH];

	//parse
	strcpy( scr_playericon, COM_Parse( &s ) );
	l = strlen(scr_playericon);

	for (j=0 ; j<l ; j++)
		scr_playericon[j] = tolower(scr_playericon[j]);

	Com_sprintf(scr_playericon, sizeof(scr_playericon), "%s_i", scr_playericon);

	strcpy( tauntsound, COM_Parse( &s ) );

	Q_strncpyz2( scr_playername, COM_Parse( &s ), sizeof(scr_playername) );

	if(cl_playtaunts->value)
	{
		S_StartSound (NULL, 0, 0, S_RegisterSound (tauntsound), 1, ATTN_NONE, 0);
		scr_playericonalpha = 2.0;
	}

}

/*
================
CL_ParseConfigString
================
*/
void CL_ParseConfigString (void)
{
	int		i;
	char	*s;
	char	olds[MAX_QPATH];
	size_t	length;

	i = MSG_ReadShort (&net_message);
	if (i < 0 || i >= MAX_CONFIGSTRINGS)
		Com_Error (ERR_DROP, "configstring > MAX_CONFIGSTRINGS");
	s = MSG_ReadString(&net_message);

	strncpy (olds, cl.configstrings[i], sizeof(olds));
	olds[sizeof(olds) - 1] = 0;

	//r1: overflow may be desired by some mods in stats programs for example. who knows.
	length = strlen(s);

	if (length >= (sizeof(cl.configstrings[0]) * (MAX_CONFIGSTRINGS-i)) - 1)
		Com_Error (ERR_DROP, "CL_ParseConfigString: configstring %d exceeds available space", i);

	//r1: don't allow basic things to overflow
	if (i != CS_NAME && i < CS_GENERAL)
	{
		if (i >= CS_STATUSBAR && i < CS_AIRACCEL)
		{
			strncpy (cl.configstrings[i], s, (sizeof(cl.configstrings[i]) * (CS_AIRACCEL - i))-1);
		}
		else
		{
			// Alien Arena client/server protocol depends on MAX_QPATH being 64
			if (length >= MAX_QPATH)
				Com_Printf ("WARNING: Configstring %d of length %d exceeds MAX_QPATH.\n", i, length);
			Q_strncpyz (cl.configstrings[i], s, sizeof(cl.configstrings[i])-1);
		}
	}
	else
	{
		strcpy (cl.configstrings[i], s);
	}

	// do something apropriate

	if (i >= CS_LIGHTS && i < CS_LIGHTS+MAX_LIGHTSTYLES)
		CL_SetLightstyle (i - CS_LIGHTS);
	else if (i >= CS_MODELS && i < CS_MODELS+MAX_MODELS)
	{
		if (cl.refresh_prepped)
		{
			cl.model_draw[i-CS_MODELS] = R_RegisterModel (cl.configstrings[i]);
			if (cl.configstrings[i][0] == '*')
				cl.model_clip[i-CS_MODELS] = CM_InlineModel (cl.configstrings[i]);
			else
				cl.model_clip[i-CS_MODELS] = NULL;
		}
	}
	else if (i >= CS_SOUNDS && i < CS_SOUNDS+MAX_MODELS)
	{
		if (cl.refresh_prepped)
			cl.sound_precache[i-CS_SOUNDS] = S_RegisterSound (cl.configstrings[i]);
	}
	else if (i >= CS_IMAGES && i < CS_IMAGES+MAX_MODELS)
	{
		if (cl.refresh_prepped)
			cl.image_precache[i-CS_IMAGES] = R_RegisterPic (cl.configstrings[i]);
	}
	else if (i >= CS_PLAYERSKINS && i < CS_PLAYERSKINS+MAX_CLIENTS)
	{
		if (cl.refresh_prepped && strcmp(olds, s))
			CL_ParseClientinfo (i-CS_PLAYERSKINS);
	}
	else if ( i == CS_GENERAL)
		CL_ParseTaunt(s);
}


/*
=====================================================================

ACTION MESSAGES

=====================================================================
*/

/*
==================
CL_ParseStartSoundPacket
==================
*/
void CL_ParseStartSoundPacket(void)
{
    vec3_t  pos_v;
	float	*pos;
    int 	channel, ent;
    int 	sound_num;
    float 	volume;
    float 	attenuation;
	int		flags;
	float	ofs;

	flags = MSG_ReadByte (&net_message);
	sound_num = MSG_ReadByte (&net_message);

    if (flags & SND_VOLUME)
		volume = MSG_ReadByte (&net_message) / 255.0;
	else
		volume = DEFAULT_SOUND_PACKET_VOLUME;

    if (flags & SND_ATTENUATION)
		attenuation = MSG_ReadByte (&net_message) / 64.0;
	else
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;

    if (flags & SND_OFFSET)
		ofs = MSG_ReadByte (&net_message) / 1000.0;
	else
		ofs = 0;

	if (flags & SND_ENT)
	{	// entity reletive
		channel = MSG_ReadShort(&net_message);
		ent = channel>>3;
		if (ent > MAX_EDICTS)
			Com_Error (ERR_DROP,"CL_ParseStartSoundPacket: ent = %i", ent);

		channel &= 7;
	}
	else
	{
		ent = 0;
		channel = 0;
	}

	if (flags & SND_POS)
	{	// positioned in space
		MSG_ReadPos (&net_message, pos_v);

		pos = pos_v;
	}
	else	// use entity number
		pos = NULL;

	if (!cl.sound_precache[sound_num])
		return;

	S_StartSound (pos, ent, channel, cl.sound_precache[sound_num], volume, attenuation, ofs);
}


void SHOWNET(char *s)
{
	if (cl_shownet->value>=2)
		Com_Printf ("%3i:%s\n", net_message.readcount-1, s);
}

/*
=====================
CL_ParseServerMessage
=====================
*/
void CL_ParseServerMessage (void)
{
	int			cmd;
	char		*s;
	int			i;

//
// if recording demos, copy the message out
//
	if (cl_shownet->value == 1)
		Com_Printf ("%i ",net_message.cursize);
	else if (cl_shownet->value >= 2)
		Com_Printf ("------------------\n");

//
// parse the message
//
	while (1)
	{
		if (net_message.readcount > net_message.cursize)
		{
			Com_Error (ERR_DROP,"CL_ParseServerMessage: Bad server message");
			break;
		}

		cmd = MSG_ReadByte (&net_message);

		if (cmd == -1)
		{
			SHOWNET("END OF MESSAGE");
			break;
		}

		if (cl_shownet->value>=2)
		{
			if (!svc_strings[cmd])
				Com_Printf ("%3i:BAD CMD %i\n", net_message.readcount-1,cmd);
			else
				SHOWNET(svc_strings[cmd]);
		}

	// other commands
		switch (cmd)
		{
		default:
			Com_Error (ERR_DROP,"CL_ParseServerMessage: Illegible server message\n");
			break;

		case svc_nop:
//			Com_Printf ("svc_nop\n");
			break;

		case svc_disconnect:
			Com_Error (ERR_DISCONNECT,"Server disconnected\n");
			break;

		case svc_reconnect:
			Com_Printf ("Server disconnected, reconnecting\n");
			// stop download
				if(cls.download)
				{
					if(cls.downloadhttp)  // clean up http downloads
						CL_HttpDownloadCleanup();
					else  // or just stop legacy ones
						fclose(cls.download);
					cls.download = NULL;
				}
			cls.state = ca_connecting;
			cls.connect_time = -99999;	// CL_CheckForResend() will fire immediately
			break;

		case svc_print:
			i = MSG_ReadByte (&net_message);
			if (i == PRINT_CHAT)
			{
				S_StartLocalSound ("misc/talk.wav");
			}
			Com_Printf ("%s", MSG_ReadString (&net_message));
			break;

		case svc_centerprint:
			SCR_CenterPrint (MSG_ReadString (&net_message));
			break;

		case svc_stufftext:
			s = MSG_ReadString (&net_message);
			Com_DPrintf ("stufftext: %s\n", s);
			// HACK to unbreak demo playback (since we can't wait until next
			// frame for the cbuf to get executed.)
			if (!Q_strcasecmp (s, "precache\n"))
			    CL_Precache_f ();
			else
			    Cbuf_AddText (s);
			break;

		case svc_serverdata:
			Cbuf_Execute ();		// make sure any stuffed commands are done
			CL_ParseServerData ();
			break;

		case svc_configstring:
			CL_ParseConfigString ();
			break;

		case svc_sound:
			CL_ParseStartSoundPacket();
			break;

		case svc_spawnbaseline:
			CL_ParseBaseline ();
			break;

		case svc_temp_entity:
			CL_ParseTEnt ();
			break;

		case svc_muzzleflash:
			CL_ParseMuzzleFlash ();
			break;

		case svc_download:
			CL_ParseDownload ();
			break;

		case svc_frame:
			CL_ParseFrame ();
			break;

		case svc_inventory:
			CL_ParseInventory ();
			break;

		case svc_layout:
			s = MSG_ReadString (&net_message);
			strncpy (cl.layout, s, sizeof(cl.layout)-1);
			break;

		case svc_playerinfo:
		case svc_packetentities:
		case svc_deltapacketentities:
			Com_Error (ERR_DROP, "Out of place frame data");
			break;
		}
	}

	CL_AddNetgraph ();

	//
	// we don't know if it is ok to save a demo message until
	// after we have parsed the frame
	//
	if (cls.demorecording && !cls.demowaiting)
		CL_WriteDemoMessage ();

}


