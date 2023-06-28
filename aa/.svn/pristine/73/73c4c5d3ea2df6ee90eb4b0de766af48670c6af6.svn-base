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
// cl_inv.c -- client inventory screen

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "client.h"

/*
================
CL_ParseInventory
================
*/
void CL_ParseInventory (void)
{
	int		i;

	for (i=0 ; i<MAX_ITEMS ; i++)
		cl.inventory[i] = MSG_ReadShort (&net_message);
}


/*
================
CL_DrawInventory
================
*/
#define	DISPLAY_ITEMS	17

void CL_DrawInventory (void)
{
	FNT_font_t		font;
	struct FNT_window_s	box;
	int			i, j;
	int			num;
	int			selected_num;
	int			selected;
	int			top;
	int			index[ MAX_ITEMS ];
	float			scale;
	int			colWidth[ 3 ];
	int			colPos[ 3 ];

	// Find selected item
	num = 0;
	selected = cl.frame.playerstate.stats[ STAT_SELECTED_ITEM ];
	selected_num = 0;
	for ( i = 0 ; i < MAX_ITEMS ; i++ ) {
		if ( i == selected) {
			selected_num = num;
		}
		if (cl.inventory[i]) {
			index[num] = i;
			num++;
		}
	}

	// Load font and compute scaled size
	font = FNT_AutoGet( CL_gameFont );
	scale = font->size / 8.0;

	// determine scroll point
	top = selected_num - DISPLAY_ITEMS/2;
	if (num - top < DISPLAY_ITEMS)
		top = num - DISPLAY_ITEMS;
	if (top < 0)
		top = 0;

	// Draw frame and headers
	box.x = (int)( ( viddef.width - 416 * scale ) / 2 );
	box.y = (int)( ( viddef.height - 256 * scale ) / 2 );
	Draw_StretchPic( box.x , box.y , 416 * scale , 256 * scale , "inventory" );

	colPos[ 0 ] = ( box.x += 56 * scale ) , box.y += 32 * scale;
	colWidth[ 0 ] = box.width = 55 * scale , box.height = 0;
	FNT_BoundedPrint( font , "Hotkey" , FNT_CMODE_NONE , FNT_ALIGN_RIGHT , &box , FNT_colors[ 7 ] );

	colPos[ 1 ] = ( box.x += 60 * scale );
	colWidth[ 1 ] = box.width = 43 * scale , box.height = 0;
	FNT_BoundedPrint( font , "###" , FNT_CMODE_NONE , FNT_ALIGN_CENTER , &box , FNT_colors[ 7 ] );

	colPos[ 2 ] = ( box.x += 48 * scale );
	colWidth[ 2 ] = box.width = 100 * scale , box.height = 0;
	FNT_BoundedPrint( font , "Item" , FNT_CMODE_NONE , FNT_ALIGN_LEFT , &box , FNT_colors[ 7 ] );

	Draw_Fill( colPos[ 0 ] + scale , box.y + 11 * scale , 302 * scale , 2 * scale , RGBA8(235,235,235,255));

	// Draw inventory contents
	box.y += 16 * scale;
	for ( i = top ; i < num && i < top + DISPLAY_ITEMS ; i++ ) {
		int		item = index[ i ];
		char		binding[ MAX_QPATH + 5 ];
		const char *	bind;
		const float *	color = FNT_colors[ index[ i ] == selected ? 7 : 2 ];
		char		count[ 5 ];

		// search for a binding
		Com_sprintf (binding, sizeof(binding), "use %s", cl.configstrings[CS_ITEMS+item]);
		bind = "";
		for (j=0 ; j<256 ; j++) {
			if (keybindings[j] && !Q_strcasecmp (keybindings[j], binding))
			{
				bind = Key_KeynumToString(j);
				break;
			}
		}

		// Draw inventory line
		Com_sprintf( count , sizeof( count ) , "%i" , cl.inventory[item] );
		box.x = colPos[ 0 ] , box.width = colWidth[ 0 ] , box.height = 0;
		FNT_BoundedPrint( font , bind , FNT_CMODE_NONE , FNT_ALIGN_RIGHT , &box , color );
		box.x = colPos[ 1 ] , box.width = colWidth[ 1 ] , box.height = 0;
		FNT_BoundedPrint( font , count , FNT_CMODE_NONE , FNT_ALIGN_CENTER , &box , color );
		box.x = colPos[ 2 ] , box.width = colWidth[ 2 ] , box.height = 0;
		FNT_BoundedPrint( font , cl.configstrings[ CS_ITEMS + item ] , FNT_CMODE_NONE , FNT_ALIGN_LEFT , &box , color );

		box.y += 8 * scale;
	}
}
