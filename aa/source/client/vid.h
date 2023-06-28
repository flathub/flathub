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
// vid.h -- video driver defs

typedef struct vidmode_s
{
	const char *description;
	int         width, height;
	int         mode;
} vidmode_t;

extern vidmode_t vid_modes[];
extern cvar_t* gl_mode;

#define VID_MIN_WIDTH (vid_modes[0].width)
#define VID_MIN_HEIGHT (vid_modes[0].height)
#define VID_MAX_WIDTH (gl_mode->integer == -1 ? vid_width->integer : vid_modes[gl_mode->integer].width)
#define VID_MAX_HEIGHT (gl_mode->integer == -1 ? vid_height->integer : vid_modes[gl_mode->integer].height)

typedef struct vrect_s
{
	int				x,y,width,height;
} vrect_t;

typedef struct
{
	int		width, height;			// coordinates from main game
} viddef_t;

extern	viddef_t	viddef;				// global video state

typedef enum 
{
	windowmode_windowed,
	windowmode_borderless_windowed,
	windowmode_exclusive_fullscreen,
} windowmode_t;

// Video module initialisation etc
void	VID_Init (void);
void	VID_Shutdown (void);
void	VID_CheckChanges (void);

qboolean	VID_GetModeInfo (int *max_width, int *max_height, int *current_width, int *current_height, int mode, windowmode_t windowmode);
void		R_Register (void); //defined in ref_gl
void		VID_MenuInit( void );
void		VID_NewWindow( int width, int height );
void		VID_NewPosition( int x, int y );
