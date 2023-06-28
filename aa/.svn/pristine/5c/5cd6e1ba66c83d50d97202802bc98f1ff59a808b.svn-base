/*
Copyright (C) 2011 COR Entertainment, LLC.

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
#include <string.h>
#include "curl/curl.h"

#include "client.h"

extern cvar_t  *cl_latest_game_version_url;

static char versionstr[32];
static size_t versionstr_sz = 0;

struct dotted_triple
{
	unsigned long major;
	unsigned long minor;
	unsigned long point;
};
static struct dotted_triple this_version;
static struct dotted_triple latest_version;
static char update_notice[256];


/**
 *
 * valid dotted version numbers
 *   double  <0..99>.<0..99> (.point is 0)
 *   triple  <0..99>.<0..99>.<0..99> (point can be 0)
 *   with the exceptions: 0.0 and 0.0.d are not valid
 *
 * @param vstring      string from server
 * @param version_out  parsed dotted triple output
 * @return             true if valid, false otherwise
 */
static qboolean parse_version( const char* vstring, struct dotted_triple *version_out )
{
	char* pch_start;
	char* pch_end;
	qboolean valid_version = false;
	unsigned long major;
	unsigned long minor;
	unsigned long point;
	
	pch_start = (char*)vstring;
	if ( isdigit( *pch_start ) )
	{
		major = strtoul( vstring, &pch_end, 10 );
		if ( major <= 99UL && *pch_end == '.' )
		{
			pch_start = pch_end + 1;
			if ( isdigit( *pch_start ) )
			{
				minor = strtoul( pch_start, &pch_end, 10 );
				if ( (major >= 1UL || (major == 0UL && minor >= 1UL)) 
					&& minor <= 99UL )
				{
					if ( *pch_end == '.' )
					{
						pch_start = pch_end + 1;
						if ( isdigit( *pch_start ) )
						{
							point = strtoul( pch_start, &pch_end, 10 );
							if ( point <= 99UL && *pch_end == '\0')
							{ /* valid x.y.z */
								version_out->major = major;
								version_out->minor = minor;
								version_out->point = point;
								valid_version = true;
							}
						}
					}
					else if ( *pch_end == '\0' )
					{ /* valid x.y */
						version_out->major = major;
						version_out->minor = minor;
						version_out->point = 0UL;
						valid_version = true;
					}
				}
			}
		}
	}
	
	return valid_version;
}

/**
 *  Compare dotted triple structs
 * 
 * @param version1		Dotted triple struct
 * @param version2		Dotted triple struct
 * @return				True if version2 newer than version1, false otherwise
 *
 */ 
static qboolean compare_version (const struct dotted_triple *version1, const struct dotted_triple *version2)
{
	if ( version1->major < version2->major )
	{
		return true;
	}
	if ( version1->major == version2->major 
		&& version1->minor < version2->minor )
	{
		return true;
	}
	if ( version1->major == version2->major 
		&& version1->minor == version2->minor 
		&& version1->point < version2->point )
	{
		return true;
	}
	return false;
}

/**
 *  generate a version update notice, or nul the string
 *  see CL_VersionUpdateNotice() below
 * 
 * @param vstring  the latest version from the server
 *
 */ 
static void update_version( const char* vstring )
{
	qboolean valid_version;
	qboolean update_message = false;
	
	valid_version = parse_version( vstring, &latest_version );

	if ( valid_version )
	{ /* valid from server */
		valid_version = parse_version( VERSION, &this_version );
		if ( valid_version )
		{ /* local should always be valid */
			update_message = compare_version (&this_version, &latest_version);
		}
	}
	if ( update_message )
	{
		char this_string[16];
		char latest_string[16];

		if ( latest_version.point == 0UL )
		{ /* x.y */
			Com_sprintf( latest_string, sizeof(latest_string), "%d.%d", 
				latest_version.major, latest_version.minor );
		}
		else
		{ /* x.y.z */
			Com_sprintf( latest_string, sizeof(latest_string), "%d.%d.%d", 
				latest_version.major, latest_version.minor, latest_version.point );
		}
		if ( this_version.point == 0UL )
		{ /* x.y */
			Com_sprintf( this_string, sizeof(this_string), "%d.%d", 
				this_version.major, this_version.minor );
		}
		else
		{ /* x.y.z */
			Com_sprintf( this_string, sizeof(this_string), "%d.%d.%d", 
				this_version.major, this_version.minor, this_version.point );
		}
		Com_sprintf( update_notice, sizeof(update_notice),
			 	"Version %s available (%s currently installed)", 
			 	latest_string, this_string );
	}
	else
	{ /* not available */
		update_notice[0] = '\0';
	}
}

static size_t write_data(const void *buffer, size_t size, size_t nmemb, void *userp)
{
	size_t bytecount = size*nmemb;

	if ( (versionstr_sz + bytecount) < sizeof(versionstr) )
	{
		memcpy( &versionstr[versionstr_sz], buffer, bytecount );
		versionstr_sz += bytecount;
		versionstr[versionstr_sz] = 0;
	}
	else
	{
		bytecount = 0; // will cause curl to return an error
	}

	return bytecount;
}

// Note: This function returns a pointer to a substring of the original string.
// If the given string was allocated dynamically, the caller must not overwrite
// that pointer with the returned value, since the original pointer must be
// deallocated using the same allocator with which it was allocated.  The return
// value must NOT be deallocated using free() etc.
char *trimblanks(char *str)
{
  char *end;

  // Trim leading whitespace
  while(isblank((unsigned char)*str) || *str == '\n' || *str == '\r') str++;

  // All spaces?
  if(*str == 0) {
    return str;
  }

  // Trim trailing whitespace
  end = str + strlen(str) - 1;
  while(end > str && isblank((unsigned char)*end) || *end == '\n' || *end == '\r') end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

void CL_GetLatestGameVersion( void )
{
	char url[128];
	CURL* easyhandle;
	CURLcode result;

    memset( versionstr, 0, sizeof(versionstr) );
	versionstr_sz = 0;

	easyhandle = curl_easy_init();

	// Set Http version to 1.1, somehow this seems to be needed for the multi-download
	if (curl_easy_setopt(easyhandle, CURLOPT_HTTP_VERSION, (long) CURL_HTTP_VERSION_1_1) != CURLE_OK) return false;

	// Follow redirects to https - but this doesn't seem to be working
	if (curl_easy_setopt(easyhandle, CURLOPT_FOLLOWLOCATION, 1L) != CURLE_OK) return false;
	if (curl_easy_setopt(easyhandle, CURLOPT_MAXREDIRS, 3L) != CURLE_OK) return false;
	
	// Don't verify that the host matches the certificate
	if (curl_easy_setopt(easyhandle, CURLOPT_SSL_VERIFYHOST, 0L) != CURLE_OK) return false;

	Com_sprintf(url, sizeof(url), "%s", cl_latest_game_version_url->string);

	if (curl_easy_setopt(easyhandle, CURLOPT_URL, url) != CURLE_OK) return;

	// time out in 5s
	if (curl_easy_setopt(easyhandle, CURLOPT_CONNECTTIMEOUT, 5) != CURLE_OK) return;

	if (curl_easy_setopt(easyhandle, CURLOPT_WRITEFUNCTION, write_data) != CURLE_OK) return;

	result = curl_easy_perform(easyhandle);

	if (result != CURLE_OK)	{
		Com_Printf("Version check failed with error %ld.\n", result);
		return;
	}

	(void)curl_easy_cleanup(easyhandle);

	// Remove whitespace including linefeeds and carriage returns
	trimblanks(versionstr);

	update_version(versionstr);
}

/**
 *  
 * @returns NULL if program is latest version, pointer to update notice otherwise
 */
char* CL_VersionUpdateNotice( void )
{
	if ( update_notice[0] == '\0' )
		return NULL;
	else
		return update_notice;		
}

/**
 *
 * @param server_vstring	the version string from a remote game server
 * @returns True if the the server is out of date or if the string is malformed, false otherwise.
 *
 * NOTE: CL_GetLatestGameVersion has to have been called at least once before
 * calling this!
 */
qboolean CL_ServerIsOutdated (char *server_vstring){
	char *end;
	struct dotted_triple server_version;
	qboolean valid_version;
	
	// have to get rid of everything after the first space
	memset( versionstr, 0, sizeof(versionstr) );
	
	strncpy (versionstr, server_vstring, sizeof(versionstr));
	
	end = strchr (versionstr, ' ');
	if (end != NULL)
		*end = '\0';
	
	valid_version = parse_version( versionstr, &server_version );
	
	// assume it's out of date if the version string is malformed
	if (!valid_version)
		return true;
	
	return compare_version (&server_version, &latest_version);
}
