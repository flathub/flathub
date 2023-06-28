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

#include "client/client.h"


cvar_t	*in_mouse;
cvar_t	*in_dgamouse;
cvar_t	*in_joystick;
cvar_t	*m_accel;

void IN_Init(void)
{

	// mouse variables
	in_mouse = Cvar_Get ("in_mouse", "1", CVAR_ARCHIVE);
	in_dgamouse = Cvar_Get ("in_dgamouse", "1", CVAR_ARCHIVE);
	in_joystick     = Cvar_Get ("in_joystick", "0", CVAR_ARCHIVE);

	Cmd_AddCommand ("+mlook", IN_MLookDown);
	Cmd_AddCommand ("-mlook", IN_MLookUp);

	mouse_available = true;

	refreshCursorLink();
}

void IN_Shutdown(void)
{
	if (!mouse_available)
		return;

	IN_Activate(false);

	mouse_available = false;

	Cmd_RemoveCommand ("+mlook");
	Cmd_RemoveCommand ("-mlook");
}


/*
===========
IN_Commands
===========
*/
void IN_Commands (void)
{
}


void IN_Frame (void)
{
	if (!mouse_available)
		return;

	if ( !cl.refresh_prepped || cls.key_dest == key_console || cls.key_dest == key_menu || cl.attractloop )
		IN_Activate(false);
	else
		IN_Activate(true);

}


void IN_JoyMove (usercmd_t *cmd)
{
	// No joystick support for Linux
}
