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

#include <stdio.h>
#include <errno.h>

#include "g_local.h"


void	Svcmd_Test_f (void)
{
	safe_cprintf (NULL, PRINT_HIGH, "Svcmd_Test_f()\n");
}

/*
==============================================================================

PACKET FILTERING

You can add or remove addresses from the filter list with:

sv addip <ip>
sv removeip <ip>

The ip address is specified in dot format, and any unspecified digits
will match any value, so you can specify an entire class C network
with "addip 192.246.40".

Removeip will only remove an address specified exactly the same way.
You cannot addip a subnet, then removeip a single host.

sv listip
Prints the current list of filters.

sv writeip

Dumps "addip <ip>" commands to listip.cfg so it can be execed at a
later date.  The filter lists are not saved and restored by default,
because I beleive it would cause too much confusion.

filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will
be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This
lets you easily set up a private game, or a game that only allows
players from your local network.

------ 2014-01-13 NOTE -------

Corrected file system support. It was still using the assumption
that the executable's current working directory was the parent
directory for the arena subdir.

The sv commands are passed to the game module when they are not
recogized by the server. It seem obvious that these were originally
in the server code, and were moved to the game for some reason
(maybe to give the capability to mods?).

==============================================================================
*/

/*
=================
StringToFilter
=================
*/
qboolean StringToFilter (char *s, ipfilter_t *f)
{
	char	num[128];
	int		i, j;
	byte	b[4];
	byte	m[4];

	for (i=0 ; i<4 ; i++)
	{
		b[i] = 0;
		m[i] = 0;
	}

	for (i=0 ; i<4 ; i++)
	{
		if (*s < '0' || *s > '9')
		{
			safe_cprintf(NULL, PRINT_HIGH, "Bad filter address: %s\n", s);
			return false;
		}

		j = 0;
		while (*s >= '0' && *s <= '9')
		{
			num[j++] = *s++;
		}
		num[j] = 0;
		b[i] = atoi(num);
		if (b[i] != 0)
			m[i] = 255;

		if (!*s)
			break;
		s++;
	}

	f->mask = *(unsigned *)m;
	f->compare = *(unsigned *)b;

	return true;
}

/*
=================
SV_FilterPacket
=================
*/
qboolean SV_FilterPacket (char *from)
{
	int		i;
	unsigned	in;
	byte m[4];
	char *p;

	i = 0;
	p = from;
	while (*p && i < 4) {
		m[i] = 0;
		while (*p >= '0' && *p <= '9') {
			m[i] = m[i]*10 + (*p - '0');
			p++;
		}
		if (!*p || *p == ':')
			break;
		i++, p++;
	}

	in = *(unsigned *)m;

	for (i=0 ; i<numipfilters ; i++)
		if ( (in & ipfilters[i].mask) == ipfilters[i].compare)
			return filterban->integer;

	return !filterban->integer;
}

/**
 * For kickban, add player ip to ipfilter banned list.
 * Does not write to iplist.cfg. Do 'sv writeip', if
 * that is wanted.
 *
 * @params ip - string containing dotted IP addr.
 */
void G_Ban( char *ip )
{
	ipfilter_t banfilter;

	if ( filterban->integer && StringToFilter( ip, &banfilter ))
	{
		int i;
		for ( i = 0 ; i < numipfilters ; ++i )
			if ( ipfilters[i].compare == 0xffffffff )
				break;		// free spot
		if ( i == numipfilters && numipfilters < MAX_IPFILTERS )
		{
			if ( !StringToFilter( ip, &ipfilters[i] ))
				ipfilters[i].compare = 0xffffffff;
			else
				++numipfilters;
		}
	}
}

/*
=================
SV_AddIP_f
=================
*/
void SVCmd_AddIP_f (void)
{
	int		i;

	if (gi.argc() < 3) {
		safe_cprintf(NULL, PRINT_HIGH, "Usage:  addip <ip-mask>\n");
		return;
	}

	for (i=0 ; i<numipfilters ; i++)
		if (ipfilters[i].compare == 0xffffffff)
			break;		// free spot
	if (i == numipfilters)
	{
		if (numipfilters == MAX_IPFILTERS)
		{
			safe_cprintf (NULL, PRINT_HIGH, "IP filter list is full\n");
			return;
		}
		numipfilters++;
	}

	if (!StringToFilter (gi.argv(2), &ipfilters[i]))
		ipfilters[i].compare = 0xffffffff;
}

/*
=================
SV_RemoveIP_f
=================
*/
void SVCmd_RemoveIP_f (void)
{
	ipfilter_t	f;
	int			i, j;

	if (gi.argc() < 3) {
		safe_cprintf(NULL, PRINT_HIGH, "Usage:  sv removeip <ip-mask>\n");
		return;
	}

	if (!StringToFilter (gi.argv(2), &f))
		return;

	for (i=0 ; i<numipfilters ; i++)
		if (ipfilters[i].mask == f.mask
		&& ipfilters[i].compare == f.compare)
		{
			for (j=i+1 ; j<numipfilters ; j++)
				ipfilters[j-1] = ipfilters[j];
			numipfilters--;
			safe_cprintf (NULL, PRINT_HIGH, "Removed.\n");
			return;
		}
	safe_cprintf (NULL, PRINT_HIGH, "Didn't find %s.\n", gi.argv(2));
}

/*
=================
SV_ListIP_f
=================
*/
void SVCmd_ListIP_f (void)
{
	int		i;
	byte	b[4];

	safe_cprintf (NULL, PRINT_HIGH, "Filter list:\n");
	for (i=0 ; i<numipfilters ; i++)
	{
		*(unsigned *)b = ipfilters[i].compare;
		safe_cprintf (NULL, PRINT_HIGH, "%3i.%3i.%3i.%3i\n", b[0], b[1], b[2], b[3]);
	}
}

/*
=================
SV_WriteIP_f
=================
*/
/**
 * Create or replace the listip.cfg with current filter list
 *
 * @param - void
 * @return - void
 */
void SVCmd_WriteIP_f (void)
{
	char  listip_name[MAX_OSPATH];
	FILE* listip_fd;
	unsigned char b[4];

	errno = 0;
	gi.FullWritePath( listip_name, sizeof(listip_name), "listip.cfg" );
	listip_fd = fopen( listip_name, "w" );
	if ( listip_fd != NULL )
	{
		int i;
		int wrcount = 
			fprintf( listip_fd, "set filterban %d\n", filterban->integer );
		for ( i = 0 ; i < numipfilters && wrcount > 0 ; ++i )
		{
			*(unsigned *)b = ipfilters[i].compare;
			wrcount = fprintf( listip_fd, "sv addip %i.%i.%i.%i\n",
							   b[0], b[1], b[2], b[3] );
		}
		if ( fclose( listip_fd ) == 0 )
		{
			safe_cprintf(NULL, PRINT_HIGH, "writeip: %s written.\n", listip_name);
			return;
		}
	}
	safe_cprintf(NULL, PRINT_HIGH, "writeip: listip.cfg file error (%i)\n", errno);

}

/*
=================
ServerCommand

ServerCommand will be called when an "sv" command is issued.
The game can issue gi.argc() / gi.argv() commands to get the rest
of the parameters
=================
*/
void	ServerCommand (void)
{
	char	*cmd;
	char botname[PLAYERNAME_SIZE];

	cmd = gi.argv(1);
	if (Q_strcasecmp (cmd, "test") == 0)
		Svcmd_Test_f ();
	else if (Q_strcasecmp (cmd, "addip") == 0)
		SVCmd_AddIP_f ();
	else if (Q_strcasecmp (cmd, "removeip") == 0)
		SVCmd_RemoveIP_f ();
	else if (Q_strcasecmp (cmd, "listip") == 0)
		SVCmd_ListIP_f ();
	else if (Q_strcasecmp (cmd, "writeip") == 0)
		SVCmd_WriteIP_f ();

// ACEBOT_ADD
	else if(Q_strcasecmp (cmd, "acedebug") == 0)
 		if (strcmp(gi.argv(2),"on")==0)
		{
			safe_bprintf (PRINT_MEDIUM, "ACE: Debug Mode On\n");
			debug_mode = true;
		}
		else
		{
			safe_bprintf (PRINT_MEDIUM, "ACE: Debug Mode Off\n");
			debug_mode = false;
		}

	else if (Q_strcasecmp (cmd, "addbot") == 0)
	{
		if ( dmflags->integer & DF_BOTS )
		{
			safe_cprintf( NULL, PRINT_HIGH,
					"Bots disabled in dmflags, sv addbot disabled.\n");
		}
		else if ( (sv_botkickthreshold && sv_botkickthreshold->integer)
				|| (g_duel && g_duel-> integer) )
		{ // duel mode forces bot kick threshold
			safe_cprintf( NULL, PRINT_HIGH,
					"Auto bot kick enabled, sv addbot disabled.\n");
		}
		else
		{ // can become a team bot with assigned skin color
			Q_strncpyz2( botname, gi.argv(2), sizeof(botname) );
			ValidatePlayerName( botname, sizeof(botname) );
			ACESP_SpawnBot( botname, gi.argv(3), NULL );
		}
	}

	// removebot
    else if(Q_strcasecmp (cmd, "removebot") == 0)
    {
		if ( dmflags->integer & DF_BOTS )
		{
			safe_cprintf( NULL, PRINT_HIGH,
					"Bots disabled in dmflags, sv removebot disabled.\n");
		}
		else if ( (sv_botkickthreshold && sv_botkickthreshold->integer)
				|| ( g_duel && g_duel->integer ) )
		{ // duel mode forces bot kick threshold
			safe_cprintf( NULL, PRINT_HIGH,
					"Auto bot kick enabled, sv removebot disabled.\n");
		}
		else
		{
			ACESP_RemoveBot(gi.argv(2));
		}
    }

	// Node saving
	else if(Q_strcasecmp (cmd, "savenodes") == 0)
    	ACEND_SaveNodes();

// ACEBOT_END
	else
		safe_cprintf (NULL, PRINT_HIGH, "Unknown server command \"%s\"\n", cmd);
}

