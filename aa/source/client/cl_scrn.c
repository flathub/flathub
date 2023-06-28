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
// cl_scrn.c -- master for refresh, status bar, console, chat, notify, etc

/*

  full screen console
  put up loading plaque
  blanked background with loading plaque
  blanked background with menu
  full screen image for quit and victory

  end of unit intermissions

  */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "client.h"
#include "qmenu.h"

float		scr_con_current;	// aproaches scr_conlines at scr_conspeed
float		scr_conlines;		// 0.0 to 1.0 lines of console to display

qboolean	scr_initialized;		// ready to draw

int			scr_draw_loading;

vrect_t		scr_vrect;		// position of render window on screen

cvar_t		*scr_conspeed;
cvar_t		*scr_centertime;
cvar_t		*scr_showturtle;
cvar_t		*scr_showpause;
cvar_t		*scr_printspeed;

cvar_t		*scr_netgraph;
cvar_t		*scr_timegraph;
cvar_t		*scr_debuggraph;
cvar_t		*scr_graphheight;
cvar_t		*scr_graphscale;
cvar_t		*scr_graphshift;
cvar_t      *scr_timerefcount; // for timerefresh command

cvar_t		*scr_consize;

cvar_t		*cl_drawfps;
cvar_t		*cl_drawtimer;
cvar_t		*cl_drawbandwidth;
cvar_t		*cl_drawframesover;

cvar_t		*cl_drawtime;

char		crosshair_pic[MAX_QPATH];
int		crosshair_width, crosshair_height;

void SCR_TimeRefresh_f (void);
void SCR_Loading_f (void);

extern cvar_t *rs_hasflag;
extern cvar_t *rs_team;

/*
 * map performance counters
 *
 * Used to measure the complexity of map areas.
 * Modified for display in performance counter array
 * Replace general "r_speed" cvar.
 *
 * Note: c_visible_textures seems to be always 0 and
 *   c_visible_lightmaps is not used
 */
// extern cvar_t *r_speeds;
cvar_t *rspeed_wpolys;
cvar_t *rspeed_epolys;
cvar_t *rspeed_flares;
cvar_t *rspeed_grasses;
cvar_t *rspeed_beams;
cvar_t *rspeed_vbobatches;
cvar_t *rspeed_particles;

extern int c_brush_polys;  /* "wpoly"   polygons from brushes */
extern int c_alias_polys;  /* "epoly"   polygons from .md2 meshes */
extern int c_flares;       /* "flares"  lens flares */
extern int c_grasses;      /* "grasses" vegetation instances */
extern int c_beams;        /* "beams"   light beams (?) */
extern int c_vbo_batches;  /* "vbobatches" batched draw commands */

/*
===============================================================================

BAR GRAPHS

===============================================================================
*/

/*
==============
CL_AddNetgraph

A new packet was just parsed
==============
*/
void CL_AddNetgraph (void)
{
	int		i;
	int		in;
	int		ping;

	// if using the debuggraph for something else, don't
	// add the net lines
	if (scr_debuggraph->value || scr_timegraph->value)
		return;

	for (i=0 ; i<cls.netchan.dropped ; i++)
		SCR_DebugGraph (30, RGBA8(167,59,49,255));

	for (i=0 ; i<cl.surpressCount ; i++)
		SCR_DebugGraph (30, RGBA8(255,191,15,255));

	// see what the latency was on this packet
	in = cls.netchan.incoming_acknowledged & (CMD_BACKUP-1);
	ping = cls.realtime - cl.cmd_time[in];
	ping /= 30;
	if (ping > 30)
		ping = 30;
	SCR_DebugGraph (ping, RGBA(0,1,0,1));
}


typedef struct
{
	float	value;
	float	color[4];
} graphsamp_t;

#define	DEBUGGRAPH_VALUES	4096
static	int			current;
static	graphsamp_t	values[DEBUGGRAPH_VALUES];

/*
==============
SCR_DebugGraph
==============
*/
void SCR_DebugGraph (float value, const float color[])
{
	int i;
	int limit;
	limit = scr_vrect.width;
	if (limit > DEBUGGRAPH_VALUES)
		limit = DEBUGGRAPH_VALUES;
	values[current%limit].value = value;
	for (i = 0; i < 4; i++)
		values[current%limit].color[i] = color[i];
	current++;
}

/*
==============
SCR_DrawDebugGraph
==============
*/
void SCR_DrawDebugGraph (void)
{
	int			a, x, y, w, i, h;
	float		v;
	const float	*color;

	//
	// draw the graph
	//
	
	w = scr_vrect.width;
	if (w > DEBUGGRAPH_VALUES)
		w = DEBUGGRAPH_VALUES;

	x = scr_vrect.x;
	y = scr_vrect.y+scr_vrect.height;
	Draw_Fill (x, y-scr_graphheight->value,
		w, scr_graphheight->value, RGBA8(123,123,123,255));

	for (a=0 ; a<w ; a++)
	{
		i = (current-1-a+w) % w;
		v = values[i].value;
		color = values[i].color;
		v = v*scr_graphscale->value + scr_graphshift->value;

		if (v < 0)
			v += scr_graphheight->value * (1+(int)(-v/scr_graphheight->value));
		h = (int)v % scr_graphheight->integer;
		Draw_Fill (x+w-1-a, y - h, 1, h, color);
	}
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

char		scr_centerstring[1024];
float		scr_centertime_start;	// for slow victory printing
float		scr_centertime_off;
int			scr_center_lines;

/*
==============
SCR_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void SCR_CenterPrint (char *str)
{
	char	*s;

	if(!cl_centerprint->value)
		return;

	strncpy (scr_centerstring, str, sizeof(scr_centerstring)-1);
	scr_centertime_off = scr_centertime->value;
	scr_centertime_start = cl.time;

	// count the number of lines for centering
	scr_center_lines = 1;
	s = str;
	while (*s)
	{
		if (*s == '\n')
			scr_center_lines++;
		s++;
	}
}


void SCR_DrawCenterString (void)
{
	FNT_font_t		font;
	struct FNT_window_s	box;
	float			colors[ 8 ] = { 1, 1, 1, 1, 0.25, 1, 0.25, 1 };

	if ( scr_centertime_off <= scr_centertime->value / 5.0 ) {
		colors[ 3 ] = colors[ 7 ] = scr_centertime_off * 5.0 / scr_centertime->value;
	}

	box.x = 0;
	if ( scr_center_lines <= 4 ) {
		box.y = viddef.height * 0.35;
	} else {
		box.y = 48;
	}
	box.height = viddef.height - box.y;
	box.width = viddef.width;

	font = FNT_AutoGet( CL_centerFont );
	FNT_BoundedPrint( font , scr_centerstring , FNT_CMODE_TWO , FNT_ALIGN_CENTER , &box , colors );
}

void SCR_CheckDrawCenterString (void)
{
	scr_centertime_off -= cls.frametime;

	if (scr_centertime_off <= 0)
		return;

	SCR_DrawCenterString ();
}

/*
===============================================================================

IRC PRINTING

  we'll have 5 lines stored.
  once full, start bumping lines, as in copying line 1 into line 0, and so forth

===============================================================================
*/

extern vec4_t		Color_Table[8];

#define SCR_IRC_LINES	5
#define SCR_IRC_LENGTH	1024
#define SCR_IRC_INDENT	8

#define SCR_IRC_DISPLAY_HIDE 0		// During line bumping
#define SCR_IRC_DISPLAY_PARTIAL 1	// When a new line is being added
#define SCR_IRC_DISPLAY_FULL 2		// No problemo

char			scr_IRCstring_contents[ SCR_IRC_LINES ][ SCR_IRC_LENGTH ];
char *			scr_IRCstring[ SCR_IRC_LINES ];
float			scr_IRCtime_start;	// for slow victory printing
float			scr_IRCtime_off;
int			scr_IRC_lines = -1;
int			scr_IRC_display = SCR_IRC_DISPLAY_FULL;

/*
==============
SCR_IRCPrint

Called for IRC messages
==============
*/


void IRC_SetNextLine( int len , const char *buffer )
{
	char * to_set;
	int i;

	// Access the correct line buffer
	scr_IRC_display = SCR_IRC_DISPLAY_HIDE;
	if ( scr_IRC_lines < SCR_IRC_LINES - 1 ) {
		scr_IRC_lines ++;
		to_set = scr_IRCstring_contents[ scr_IRC_lines ];
	} else {
		to_set = scr_IRCstring[ 0 ];
		for ( i = 0 ; i < SCR_IRC_LINES - 1 ; i ++ )
			scr_IRCstring[ i ] = scr_IRCstring[ i + 1 ];
	}
	scr_IRCstring[ scr_IRC_lines ] = to_set;
	scr_IRC_display = SCR_IRC_DISPLAY_PARTIAL;

	// Copy string
	Q_strncpyz2( to_set , buffer , SCR_IRC_LENGTH );

	// Restore IRC display and update time-related variables
	scr_IRCtime_off = 15;
	scr_IRCtime_start = cl.time;
	scr_IRC_display = SCR_IRC_DISPLAY_FULL;
}


void SCR_IRCPrintf (char *fmt, ...)
{
	va_list		argptr;
	char		msg[1024];
	int		len;

	va_start (argptr,fmt);
	len = vsnprintf(msg, sizeof(msg), fmt, argptr);
	va_end (argptr);

	if ( len >= sizeof(msg) )
		len = sizeof( msg ) - 1;

	IRC_SetNextLine( len , msg );
}


void SCR_CheckDrawIRCString (void)
{
	FNT_font_t		font;
	struct FNT_window_s	box;
	int			i , last_line;
	static float		color[ 4 ] = { 1 , 1 , 1 , 1 };

	scr_IRCtime_off -= cls.frametime;
	if (scr_IRCtime_off <= 0 || scr_IRC_display == SCR_IRC_DISPLAY_HIDE)
		return;

	last_line = ( scr_IRC_display == SCR_IRC_DISPLAY_FULL ) ? scr_IRC_lines : ( scr_IRC_lines - 1 );
	if ( last_line == -1 )
		return;

	font = FNT_AutoGet( CL_gameFont );

	box.x = 0;
	box.y = font->size * 5;
	for ( i = 0 ; i <= last_line ; i++ ) {
		box.width = viddef.width;
		box.height = font->size * 15 - box.y;

		FNT_WrappedPrint( font , scr_IRCstring[ i ] , FNT_CMODE_QUAKE , FNT_ALIGN_LEFT ,
			SCR_IRC_INDENT , &box , color );

		box.y += box.height;
		if ( box.y >= font->size * 15 ) {
			break;
		}
	}
}

//=============================================================================

/*
=================
SCR_CalcVrect

Sets scr_vrect, the coordinates of the rendered window
=================
*/
static void SCR_CalcVrect (void)
{

	scr_vrect.width = viddef.width;
	scr_vrect.height = viddef.height;

	// calculate left and top margins-- for now always 0
	scr_vrect.x = (viddef.width - scr_vrect.width)/2;
	scr_vrect.y = (viddef.height - scr_vrect.height)/2;
}

/*
=================
SCR_Sky_f

Set a specific sky and rotation speed
=================
*/
void SCR_Sky_f (void)
{
	float	rotate;
	vec3_t	axis;

	if (Cmd_Argc() < 2)
	{
		Com_Printf ("Usage: sky <basename> <rotate> <axis x y z>\n");
		return;
	}
	if (Cmd_Argc() > 2)
		rotate = atof(Cmd_Argv(2));
	else
		rotate = 0;
	if (Cmd_Argc() == 6)
	{
		axis[0] = atof(Cmd_Argv(3));
		axis[1] = atof(Cmd_Argv(4));
		axis[2] = atof(Cmd_Argv(5));
	}
	else
	{
		axis[0] = 0;
		axis[1] = 0;
		axis[2] = 1;
	}

	R_SetSky (Cmd_Argv(1), rotate, axis);
}

//============================================================================

/*
==================
SCR_Init
==================
*/
void SCR_Init (void)
{
	scr_conspeed = Cvar_Get ("scr_conspeed", "3", CVAR_ARCHIVE);
	scr_consize = Cvar_Get ("scr_consize", "0.5", CVAR_ARCHIVE);
	scr_showturtle = Cvar_Get ("scr_showturtle", "0", 0);
	// default back to 1 if we ever actually make a pause icon:
	scr_showpause = Cvar_Get ("scr_showpause", "0", 0); 
	scr_centertime = Cvar_Get ("scr_centertime", "2.5", CVAR_ARCHIVE);
	scr_printspeed = Cvar_Get ("scr_printspeed", "8", CVAR_ARCHIVE);
	scr_netgraph = Cvar_Get ("netgraph", "0", 0);
	scr_timegraph = Cvar_Get ("timegraph", "0", 0);
	scr_debuggraph = Cvar_Get ("debuggraph", "0", 0);
	scr_graphheight = Cvar_Get ("graphheight", "32", 0);
	scr_graphscale = Cvar_Get ("graphscale", "1", 0);
	scr_graphshift = Cvar_Get ("graphshift", "0", 0);
	// configurable number of frames for timerefresh command
	scr_timerefcount = Cvar_Get("scr_timerefcount", "128", 0 );

	cl_drawfps = Cvar_Get ("cl_drawfps", "0", CVAR_ARCHIVE);
	cl_drawtimer = Cvar_Get("cl_drawtimer", "0", CVAR_ARCHIVE);
	cl_drawbandwidth = Cvar_Get ("cl_drawbandwidth", "0", CVAR_ARCHIVE);
	cl_drawframesover = Cvar_Get ("cl_drawframesover", "0", CVAR_ARCHIVE);

	rspeed_wpolys   = Cvar_Get("rspeed_wpolys",  "0", 0);
	rspeed_epolys   = Cvar_Get("rspeed_epolys",  "0", 0);
	rspeed_flares   = Cvar_Get("rspeed_flares",  "0", 0);
	rspeed_grasses  = Cvar_Get("rspeed_grasses", "0", 0);
	rspeed_beams    = Cvar_Get("rspeed_beams",   "0", 0);
	rspeed_vbobatches = Cvar_Get("rspeed_vbobatches", "0", 0);
	rspeed_particles = Cvar_Get("rspeed_particles", "0", 0);
	
	memset (perftests, 0, sizeof(perftests));

//
// register our commands
//
	Cmd_AddCommand ("timerefresh",SCR_TimeRefresh_f);
	Cmd_AddCommand ("loading",SCR_Loading_f);
	Cmd_AddCommand ("sky",SCR_Sky_f);

	scr_initialized = true;
}


/*
==============
SCR_DrawNet
==============
*/
void SCR_DrawNet (void)
{
	if (cls.netchan.outgoing_sequence - cls.netchan.incoming_acknowledged
		< CMD_BACKUP-1)
		return;

	Draw_Pic (scr_vrect.x+64, scr_vrect.y, "net");
}

void SCR_DrawAlertMessagePicture (char *name, qboolean center)
{
	float ratio;
	int scale;
	int w, h;

	scale = viddef.width/MENU_STATIC_WIDTH;

	Draw_GetPicSize (&w, &h, name );
	if (w)
	{
		ratio = 35.0/(float)h;
		h = 35;
		w *= ratio;
	}
	else
		return;

	if(center)
		Draw_StretchPic (
			viddef.width / 2 - w*scale/2,
			viddef.height/ 2 - h*scale/2,
			scale*w, scale*h, name);
	else
		Draw_StretchPic (
			viddef.width / 2 - w*scale/2,
			scale * 100,
			scale*w, scale*h, name);
}

void SCR_DrawPause (void)
{

	if (!scr_showpause->value)		// turn off for screenshots
		return;

	if (!cl_paused->value)
		return;

	if (cls.key_dest == key_menu)
		return;

	SCR_DrawAlertMessagePicture("pause", true);
}

/*
==============
SCR_DrawLoading
==============
*/


void SCR_DrawRotatingIcon( void )
{
	extern float CalcFov( float fov_x, float w, float h );
	refdef_t refdef;
	static float yaw;
	entity_t entity;

	memset( &refdef, 0, sizeof( refdef ) );

	refdef.width = viddef.width;
	refdef.height = viddef.height;
	refdef.x = 0;
	refdef.y = 0;
	refdef.fov_x = 90;
	refdef.fov_y = CalcFov( refdef.fov_x, refdef.width, refdef.height );
	refdef.time = cls.realtime*0.001;

	memset( &entity, 0, sizeof( entity ) );

	yaw += cls.frametime*50;
	if (yaw > 360)
		yaw = 0;

	entity.model = R_RegisterModel( "models/objects/icon/tris.md2" );

	entity.flags = RF_FULLBRIGHT | RF_MENUMODEL;
	entity.origin[0] = 80;
	entity.origin[1] = 30;
	entity.origin[2] = -5;

	VectorCopy( entity.origin, entity.oldorigin );

	entity.frame = 0;
	entity.oldframe = 0;
	entity.backlerp = 0.0;
	entity.angles[1] = (int)yaw;

	refdef.areabits = 0;
	refdef.num_entities = 1;

	refdef.entities = &entity;
	refdef.lightstyles = 0;
	refdef.rdflags = RDF_NOWORLDMODEL;

	refdef.height += 4;

	R_RenderFrame( &refdef );

	free(entity.model);
}

void SCR_DrawLoadingBar (int percent, int scale)
{
	float hudscale;

	hudscale = (float)(viddef.height)/600;
	if(hudscale < 1)
		hudscale = 1;

	if (R_RegisterPic("bar_background") && R_RegisterPic("bar_loading"))
	{
		Draw_StretchPic (
			viddef.width/2-scale*15 + 1*hudscale,viddef.height/2 + scale*5.4,
			scale*30-2*hudscale, scale*10, "bar_background");
		Draw_StretchPic (
			viddef.width/2-scale*15 + 1*hudscale,viddef.height/2 + scale*6.2,
			(scale*30-2*hudscale)*percent/100, scale*2, "bar_loading");
	}
	else
	{
		Draw_Fill (
			viddef.width/2-scale*15 + 1,viddef.height/2 + scale*5+1,
			scale*30-2, scale*2-2, RGBA8(47,47,47,255));
		Draw_Fill (
			viddef.width/2-scale*15 + 1,viddef.height/2 + scale*5+1,
			(scale*30-2)*percent/100, scale*2-2, RGBA8(107,107,107,255));
	}
}



void SCR_DrawLoading (void)
{
	FNT_font_t		font;
	struct FNT_window_s	box;
	char			mapfile[32];
	qboolean		isMap = false;
	float			hudscale;

	if (!scr_draw_loading)
		return;
	scr_draw_loading = false;
	font = FNT_AutoGet( CL_gameFont );

	hudscale = (float)(viddef.height)/600;
	if(hudscale < 1)
		hudscale = 1;

	//loading a map...
	if (loadingMessage && cl.configstrings[CS_MODELS+1][0])
	{
		strcpy (mapfile, cl.configstrings[CS_MODELS+1] + 5);	// skip "maps/"
		mapfile[strlen(mapfile)-4] = 0;		// cut off ".bsp"

		if(map_pic_loaded)
			Draw_StretchPic (0, 0, viddef.width, viddef.height, va("/levelshots/%s.pcx", mapfile));
		else
			Draw_Fill (0, 0, viddef.width, viddef.height, RGBA(0,0,0,1));

		isMap = true;

	}
	else
		Draw_Fill (0, 0, viddef.width, viddef.height, RGBA(0,0,0,1));

#if 0
	// no m_background pic, but a pic here over-writes the levelshot
	Draw_StretchPic (0, 0, viddef.width, viddef.height, "m_background");
#endif

	//loading message stuff...
	if (isMap)
	{
		char loadMessage[ MAX_QPATH * 6 ];
		int i;

		Com_sprintf( loadMessage , sizeof( loadMessage ) , "Loading Map [%s]\n^3%s" , mapfile , cl.configstrings[CS_NAME] );

		box.x = 0;
		box.y = viddef.height / 2 - font->size * 5;
		box.width = viddef.width;
		box.height = 0;
		FNT_BoundedPrint( font , loadMessage , FNT_CMODE_QUAKE , FNT_ALIGN_CENTER , &box , FNT_colors[ 7 ] );

		box.y += font->size * 4;
		for ( i = 0 ; i < 5 ; i ++ ) {
			box.x = viddef.width / 2 - font->size * 15;
			box.width = box.height = 0;
			FNT_BoundedPrint( font , loadingMessages[i][0] , FNT_CMODE_QUAKE , FNT_ALIGN_LEFT , &box , FNT_colors[ 7 ] );

			box.x += box.width + font->size;
			box.width = viddef.width / 2 + font->size * 15 - box.x;
			box.height = 0;
			FNT_BoundedPrint( font , loadingMessages[i][1] , FNT_CMODE_QUAKE , FNT_ALIGN_RIGHT , &box , FNT_colors[ 7 ] );

			box.y += font->size;
		}

		//check for instance of icons we would like to show in loading process, ala q3
		if(rocketlauncher) {
			Draw_ScaledPic((int)(viddef.width/3), (int)(viddef.height/3.2), hudscale/7.1, "w_rlauncher");
			if(!rocketlauncher_drawn){
				rocketlauncher_drawn = 40*hudscale;
			}
		}
		if(chaingun) {
			Draw_ScaledPic((int)(viddef.width/3) + rocketlauncher_drawn, (int)(viddef.height/3.2), hudscale/7.1, "w_chaingun");
			if(!chaingun_drawn) {
				chaingun_drawn = 40*hudscale;
			}
		}
		if(smartgun) {
			Draw_ScaledPic((int)(viddef.width/3) + rocketlauncher_drawn+chaingun_drawn, (int)(viddef.height/3.2), hudscale/7.1, "w_smartgun");
			if(!smartgun_drawn) {
				smartgun_drawn = 40*hudscale;
			}
		}
		if(beamgun) {
			Draw_ScaledPic((int)(viddef.width/3) + rocketlauncher_drawn+chaingun_drawn+smartgun_drawn, (int)(viddef.height/3.2), hudscale/7.1, "w_beamgun");
			if(!beamgun_drawn) {
				beamgun_drawn = 40*hudscale;
			}
		}
		if(flamethrower) {
			Draw_ScaledPic((int)(viddef.width/3) + rocketlauncher_drawn+chaingun_drawn+smartgun_drawn+beamgun_drawn
				, (int)(viddef.height/3.2), hudscale/7.1, "w_flamethrower");
			if(!flamethrower_drawn) {
				flamethrower_drawn = 40*hudscale;
			}
		}
		if(disruptor) {
			Draw_ScaledPic((int)(viddef.width/3) + rocketlauncher_drawn+chaingun_drawn+smartgun_drawn+beamgun_drawn+flamethrower_drawn
				, (int)(viddef.height/3.2), hudscale/7.1, "w_disruptor");
			if(!disruptor_drawn) {
				disruptor_drawn = 40*hudscale;
			}
		}
		if(vaporizer) {
			Draw_ScaledPic((int)(viddef.width/3) + rocketlauncher_drawn+chaingun_drawn+smartgun_drawn+beamgun_drawn+flamethrower_drawn+disruptor_drawn
				, (int)(viddef.height/3.2), hudscale/7.1, "w_vaporizer");
			if(!vaporizer_drawn) {
				vaporizer_drawn = 40*hudscale;
			}
		}
		if(quad) {
			Draw_ScaledPic((int)(viddef.width/3) + rocketlauncher_drawn+chaingun_drawn+smartgun_drawn+beamgun_drawn+flamethrower_drawn+disruptor_drawn+vaporizer_drawn
				, (int)(viddef.height/3.2), hudscale/7.1, "p_quad");
			if(!quad_drawn) {
				quad_drawn = 40*hudscale;
			}
		}
		if(haste) {
			Draw_ScaledPic((int)(viddef.width/3) + rocketlauncher_drawn+chaingun_drawn+smartgun_drawn+beamgun_drawn+flamethrower_drawn+disruptor_drawn+vaporizer_drawn+quad_drawn
				, (int)(viddef.height/3.2), hudscale/7.1, "p_haste");
			if(!haste_drawn) {
				haste_drawn = 40*hudscale;
			}
		}
		if(sproing) {
			Draw_ScaledPic((int)(viddef.width/3) + rocketlauncher_drawn+chaingun_drawn+smartgun_drawn+beamgun_drawn+flamethrower_drawn+disruptor_drawn+vaporizer_drawn+quad_drawn+haste_drawn
				, (int)(viddef.height/3.2), hudscale/7.1, "p_sproing");
			if(!sproing_drawn) {
				sproing_drawn = 40*hudscale;
			}
		}
		if(inv) {
			Draw_ScaledPic((int)(viddef.width/3) + rocketlauncher_drawn+chaingun_drawn+smartgun_drawn+beamgun_drawn+flamethrower_drawn+disruptor_drawn+vaporizer_drawn+quad_drawn+haste_drawn+sproing_drawn
				, (int)(viddef.height/3.2), hudscale/7.1, "p_invulnerability");
			if(!inv_drawn) {
				inv_drawn = 40*hudscale;
			}
		}
		if(adren) {
			Draw_ScaledPic((int)(viddef.width/3) + rocketlauncher_drawn+chaingun_drawn+smartgun_drawn+beamgun_drawn+flamethrower_drawn+disruptor_drawn+vaporizer_drawn+quad_drawn+haste_drawn+sproing_drawn+inv_drawn
				, (int)(viddef.height/3.2), hudscale/7.1, "p_adrenaline");
		}
	}
	else
	{
		box.x = 0;
		box.y = ( viddef.height - font->size ) / 2;
		box.width = viddef.width;
		box.height = 0;

		FNT_BoundedPrint( font , "Awaiting Connection ..." , FNT_CMODE_NONE , FNT_ALIGN_CENTER , &box , FNT_colors[ 7 ] );
	}

	// Add Download info stuff...
	if (cls.download && ( (int)ftell( cls.download ) / 1024 ) != 0 )
	{
		if ( cls.key_dest != key_console ) //drop the console because of CURL's readout
		{
			CON_ToggleConsole();
		}
	}
	else if (isMap) //loading bar...
	{
		//to do - fix
		//SCR_DrawRotatingIcon();

		SCR_DrawLoadingBar(loadingPercent, font->size);

		box.x = 0;
		box.y = 2 + viddef.height / 2 + font->size * 6.3;
		box.width = viddef.width;
		box.height = 0;

		FNT_BoundedPrint( font , va("%3d%%", (int)loadingPercent) , FNT_CMODE_NONE , FNT_ALIGN_CENTER , &box , FNT_colors[ 7 ] );
	}
}

//=============================================================================

/*
==================
SCR_RunConsole

Scroll it up or down
==================
*/
void SCR_RunConsole (void)
{
// decide on the height of the console
	if (cls.key_dest == key_console)
		scr_conlines = scr_consize->value;
	else
		scr_conlines = 0;				// none visible

	if (scr_conlines < scr_con_current)
	{
		scr_con_current -= scr_conspeed->value*cls.frametime;
		if (scr_conlines > scr_con_current)
			scr_con_current = scr_conlines;

	}
	else if (scr_conlines > scr_con_current)
	{
		scr_con_current += scr_conspeed->value*cls.frametime;
		if (scr_conlines < scr_con_current)
			scr_con_current = scr_conlines;
	}

}

/*
==================
SCR_DrawConsole
==================
*/
float sendBubbleNow;
void SCR_DrawConsole (void)
{
	if (cls.key_dest == key_menu)
		return;

	if (cls.state == ca_disconnected || cls.state == ca_connecting)
	{	// forced full screen console
		CON_DrawConsole( scr_consize->value );
		return;
	}

	if (cls.state != ca_active || !cl.refresh_prepped)
	{	// connected, but can't render
		Draw_Fill(0, 0, viddef.width, viddef.height, RGBA(0, 0, 0, 1));
		CON_DrawConsole( scr_consize->value );
		return;
	}

	if (scr_con_current)
	{
		CON_DrawConsole( scr_con_current );
	}
	else
	{
		if (cls.key_dest == key_game || cls.key_dest == key_message)
			CON_DrawNotify ();	// only draw notify in game
	}

	//draw chat bubble
	if(cls.key_dest == key_message || scr_con_current) {
		//only send this every couple of seconds no need to flood
		sendBubbleNow += cls.frametime;
		if(sendBubbleNow >= 1) {
			Cbuf_AddText("chatbubble\n");
			Cbuf_Execute ();
			sendBubbleNow = 0;
		}
	}
}

//=============================================================================

/*
================
SCR_BeginLoadingPlaque
================
*/
qboolean needLoadingPlaque (void)
{
	if (!cls.disable_screen || !scr_draw_loading)
		return true;
	return false;
}
void SCR_BeginLoadingPlaque (void)
{
	S_StopAllSounds ();
	cl.sound_prepped = false;		// don't play ambients

	scr_draw_loading = 1;

	SCR_UpdateScreen ();
	cls.disable_screen = Sys_Milliseconds ();
	cls.disable_servercount = cl.servercount;

}

/*
================
SCR_EndLoadingPlaque
================
*/
void SCR_EndLoadingPlaque (void)
{
	cls.disable_screen = 0;
	scr_draw_loading = 0;
	CON_ClearNotify( );

	Cvar_Set ("paused", "0");
}

/*
================
SCR_Loading_f
================
*/
void SCR_Loading_f (void)
{
	SCR_BeginLoadingPlaque ();
}

int entitycmpfnc( const entity_t *a, const entity_t *b )
{
	/*
	** all other models are sorted by model then skin
	*/
	if ( a->model == b->model )
	{
		return ( ( ptrdiff_t ) a->skin - ( ptrdiff_t ) b->skin );
	}
	else
	{
		return ( ( ptrdiff_t ) a->model - ( ptrdiff_t ) b->model );
	}
}

/**
 * @brief  Rendering performance test.
 *
 * Target of 'timerefresh' command. When invoked with an argument, end of
 * frame flush and page flipping are not done. Modified 2011-02 with
 * additional info in result. Use 'viewpos' (also modified) command
 * for repeatable, consistent positioning.
 *
 * Modified 2012-12
 */
void SCR_TimeRefresh_f( void )
{
	int   t_start, t_stop;
	float d_time;
	float frames_per_sec;
	float msec_per_frame;
	float save_yaw;
	int   frame_count;
	float num_frames;
	float yaw_increment;
	float frame_yaw;
	char  *cmd_arg;

	if ( cls.state != ca_active )
		return;

	save_yaw    = cl.refdef.viewangles[YAW];
	num_frames  = scr_timerefcount->value;
	frame_count = scr_timerefcount->integer;
	if ( frame_count < 1 || frame_count > 4320 )
	{ // silently protect against unreasonable setting
		num_frames = 128.0f;
		frame_count = 128;
	}
	cmd_arg = Cmd_Argv(1);

	switch ( *cmd_arg )
	{
	case '1': // rotate
		yaw_increment = 360.0f / num_frames;
		break;

	case '2': // no rotate
		yaw_increment = 0.0f;
		break;

	default:
		Com_Printf("usage: timerefresh <arg>, 1=rotate,2=fixed\n");
		return;
	}

	// show position
	Com_Printf ("x:%#1.0f y:%#1.0f z:%#1.0f yaw:%#1.0f pitch:%#1.0f\n",
		cl.refdef.vieworg[0], cl.refdef.vieworg[1], cl.refdef.vieworg[2],
		cl.refdef.viewangles[YAW], cl.refdef.viewangles[PITCH] );

	// test loop
	frame_yaw = save_yaw;
	t_start = Sys_Milliseconds();
	while ( frame_count-- )
	{
		cl.refdef.viewangles[YAW] = frame_yaw;
		R_BeginFrame( 0 );
		R_RenderFrame( &cl.refdef );
		R_EndFrame();
		frame_yaw += yaw_increment;
	}
	t_stop = Sys_Milliseconds();

	// restore original yaw (not included in test frame count)
	cl.refdef.viewangles[YAW] = save_yaw;
	R_BeginFrame( 0 );
	R_RenderFrame( &cl.refdef );
	R_EndFrame();

	// report
	d_time = (float)(t_stop - t_start);
	d_time += 0.5f;
	msec_per_frame = d_time / num_frames;
	d_time /= 1000.0f;
	frames_per_sec = num_frames / d_time;

	Com_Printf( "%1.0f frames, %1.3f sec, %1.3f msec/frame, %1.1f FPS\n",
			num_frames, d_time, msec_per_frame, frames_per_sec );

}

//===============================================================

#define STAT_MINUS		10	// num frame for '-' stats digit
char		*sb_nums[2][11] =
{
	{"num_0", "num_1", "num_2", "num_3", "num_4", "num_5",
	"num_6", "num_7", "num_8", "num_9", "num_minus"},
	{"anum_0", "anum_1", "anum_2", "anum_3", "anum_4", "anum_5",
	"anum_6", "anum_7", "anum_8", "anum_9", "anum_minus"}
};

#define	ICON_WIDTH	24
#define	ICON_HEIGHT	24
#define	CHAR_WIDTH	16
#define	ICON_SPACE	8



/*
================
SizeHUDString

Allow embedded \n in the string
================
*/
void SizeHUDString (char *string, int *w, int *h)
{
	int		lines, width, current;
	int		charscale;

	charscale = (float)(viddef.height)*8/600;
	if(charscale < 8)
		charscale = 8;

	lines = 1;
	width = 0;

	current = 0;
	while (*string)
	{
		if (*string == '\n')
		{
			lines++;
			current = 0;
		}
		else
		{
			current++;
			if (current > width)
				width = current;
		}
		string++;
	}

	*w = width * charscale;
	*h = lines * charscale;
}

/*
==============
SCR_DrawField
==============
*/
void SCR_DrawField (int x, int y, int color, int width, int value, float scale)
{
	char	num[16], *ptr;
	int		l;
	int		frame;

	if (width < 1)
		return;

	// draw number string
	if (width > 5)
		width = 5;

	Com_sprintf (num, sizeof(num), "%i", value);
	l = strlen(num);
	if (l > width)
		l = width;
	x += 2 + CHAR_WIDTH*(width - l)*scale;

	ptr = num;
	while (*ptr && l)
	{
		if (*ptr == '-')
			frame = STAT_MINUS;
		else
			frame = *ptr -'0';

		Draw_AngledScaledPic (x,y,scale/7.2,sb_nums[color][frame], true);
		x += CHAR_WIDTH*scale;
		ptr++;
		l--;
	}
}


/*
===============
SCR_TouchPics

Allows rendering code to cache all needed sbar graphics
===============
*/
void SCR_TouchPics (void)
{
	int		i, j;

	for (i=0 ; i<2 ; i++)
		for (j=0 ; j<11 ; j++)
			R_RegisterPic (sb_nums[i][j]);

	if (strcmp(crosshair->string, "none"))
	{

		Com_sprintf (crosshair_pic, sizeof(crosshair_pic), "%s", crosshair->string);
		Draw_GetPicSize (&crosshair_width, &crosshair_height, crosshair_pic);
		if (!crosshair_width)
			crosshair_pic[0] = 0;
	}
}

/*
================
SCR_ExecuteLayoutString

================
*/
static void SCR_ExecuteLayoutString (const char *s)
{
	FNT_font_t		font;
	struct FNT_window_s	box;
	int			x, y, ny=0; //ny is coordinates used for new sb layout client tags only
	int			value;
	char *			token;
	int			numwidth = 3; //amount of digits of number being drawn
	int			index;
	clientinfo_t *		ci;
	int			charscale;
	float			scale;
	qboolean		newSBlayout = false;
	int				left, top, right, bottom, width, height, midx, midy;

	if (cls.state != ca_active || !cl.refresh_prepped)
		return;

	if (!s[0])
		return;

	font = FNT_AutoGet( CL_gameFont );
	charscale = font->size;
	scale = charscale * 0.125;
	
	width = cl.refdef.width;
	height = cl.refdef.height;
	
	x = left = cl.refdef.x;
	y = ny = top = cl.refdef.y;
	right = left+width;
	bottom = top+height;
	midx = left+width/2;
	midy = top+height/2;
	
	if (width < 1024 || height < 768)
	{
		// Below 1024x768, start scaling things down to make sure everything
		// fits. We scale x and y by the same amount to keep things square,
		// but we figure out the ratios separately and use the ratio which
		// results in the smallest scale.
		float xscale, yscale;
		xscale = (float)width/1024.0f;
		yscale = (float)height/768.0f;
		if (xscale < yscale)
			scale *= xscale;
		else
			scale *= yscale;
	}

	while (s)
	{
		token = COM_Parse (&s);
		if (!strcmp(token, "xl"))
		{
			token = COM_Parse (&s);
			x = left + atoi(token)*scale;
			continue;
		}
		if (!strcmp(token, "xr"))
		{
			token = COM_Parse (&s);
			x = right + atoi(token)*scale;
			continue;
		}
		if (!strcmp(token, "xv"))
		{
			token = COM_Parse (&s);
			x = midx + (atoi(token) - 160)*scale;
			continue;
		}

		if (!strcmp(token, "yt"))
		{
			token = COM_Parse (&s);
			y = top + atoi(token)*scale;
			continue;
		}
		if (!strcmp(token, "yb"))
		{
			token = COM_Parse (&s);
			y = bottom + atoi(token)*scale;
			continue;
		}
		if (!strcmp(token, "yv"))
		{
			token = COM_Parse (&s);
			y = midy + (atoi(token) - 100)*scale;
			ny = midy + (atoi(token)/2 - 100)*scale;
			continue;
		}

		if (!strcmp(token, "pic"))
		{	// draw a pic from a stat number
			if(strcmp(cl_hudimage1->string, "none")) {
				token = COM_Parse (&s);
				value = cl.frame.playerstate.stats[atoi(token)];
				if (value >= MAX_IMAGES)
					Com_Error (ERR_DROP, "Pic >= MAX_IMAGES");
				if( cl.configstrings[CS_IMAGES+value][0] )
				{
					if(!strncmp(cl.configstrings[CS_IMAGES+value], "p_", 2))
						Draw_ScaledPic (x, y, scale/8.0, cl.configstrings[CS_IMAGES+value]);
					else if(!strncmp(cl.configstrings[CS_IMAGES+value], "i_flag", 6) || !strncmp(cl.configstrings[CS_IMAGES+value], "i_team", 6))
						Draw_ScaledPic (x, y, scale, cl.configstrings[CS_IMAGES+value]);
					else if(!strncmp(cl.configstrings[CS_IMAGES+value], "i_", 2))
						Draw_AngledScaledPic (x, y, scale/4.0, cl.configstrings[CS_IMAGES+value], false);					
					else
						Draw_ScaledPic (x, y, scale, cl.configstrings[CS_IMAGES+value]);

					if(!strcmp(cl.configstrings[CS_IMAGES+value], "i_team1") || 
						!strcmp(cl.configstrings[CS_IMAGES+value], "i_team2"))
					{
						r_gotFlag = false;
						r_lostFlag = true;
						Cvar_Set("rs_hasflag", "0");
						if(!strcmp(cl.configstrings[CS_IMAGES+value], "i_team1"))
							Cvar_Set("rs_team", "blue");
						else
							Cvar_Set("rs_team", "red");
					}
					else if(!strcmp(cl.configstrings[CS_IMAGES+value], "i_flag1") || 
						!strcmp(cl.configstrings[CS_IMAGES+value], "i_flag2"))
					{
						r_gotFlag = true;
						r_lostFlag = false;
						Cvar_Set("rs_hasflag", "1");
						if(!strcmp(cl.configstrings[CS_IMAGES+value], "i_flag1"))
							Cvar_Set("rs_team", "blue");
						else
							Cvar_Set("rs_team", "red");
					}
				}
			}

			continue;
		}

		if (!strcmp(token, "newsb"))
		{
			//print header here
			x = midx - 160*scale;
			y = midy - 64*scale;

			box.x = x + 4 * scale , box.y = y;
			FNT_RawPrint( font , "Player" , 6 , false , box.x , box.y , FNT_colors[ 7 ] );
			box.x += 160 * scale , box.width = 56 * scale , box.height = 0;
			FNT_BoundedPrint( font , "Score" , FNT_CMODE_NONE , FNT_ALIGN_RIGHT , &box , FNT_colors[ 7 ] );
			box.x += 56 * scale , box.width = 48 * scale , box.height = 0;
			FNT_BoundedPrint( font , "Ping" , FNT_CMODE_NONE , FNT_ALIGN_RIGHT , &box , FNT_colors[ 7 ] );
			box.x += 48 * scale , box.width = 48 * scale , box.height = 0;
			FNT_BoundedPrint( font , "Time" , FNT_CMODE_NONE , FNT_ALIGN_RIGHT , &box , FNT_colors[ 7 ] );

			newSBlayout = true;
			continue;
		}

		if (!strcmp(token, "newctfsb"))
		{
			//print header here
			x = midx - 256*scale;
			y = midy - 72*scale;

			while ( x <= midx ) {
				box.x = x + 14 * scale , box.y = y;
				FNT_RawPrint( font , "Player" , 6 , false , box.x , box.y , FNT_colors[ 7 ] );
				box.x += 118 * scale , box.width = 56 * scale , box.height = 0;
				FNT_BoundedPrint( font , "Score" , FNT_CMODE_NONE , FNT_ALIGN_RIGHT , &box , FNT_colors[ 7 ] );
				box.x += 56 * scale , box.width = 48 * scale , box.height = 0;
				FNT_BoundedPrint( font , "Ping" , FNT_CMODE_NONE , FNT_ALIGN_RIGHT , &box , FNT_colors[ 7 ] );
				x += 256 * scale;
			}

			newSBlayout = true;
			continue;
		}

		if (!strcmp(token, "tacsb"))
		{
			//print header here
			x = midx - 256*scale;
			y = midy - 72*scale;

			while ( x <= midx ) {
				box.x = x + 14 * scale , box.y = y;
				FNT_RawPrint( font , "Player" , 6 , false , box.x , box.y , FNT_colors[ 7 ] );
				box.x += 118 * scale , box.width = 56 * scale , box.height = 0;
				FNT_BoundedPrint( font , "Kills" , FNT_CMODE_NONE , FNT_ALIGN_RIGHT , &box , FNT_colors[ 7 ] );
				box.x += 56 * scale , box.width = 48 * scale , box.height = 0;
				FNT_BoundedPrint( font , "Ping" , FNT_CMODE_NONE , FNT_ALIGN_RIGHT , &box , FNT_colors[ 7 ] );
				x += 256 * scale;
			}

			newSBlayout = true;
			continue;
		}

		if (!strcmp(token, "client") || !strcmp(token, "queued"))
		{	// draw a deathmatch client block
			qboolean	isQueued = ( token[ 0 ] == 'q' );
			int		score, ping, time;
			const char *	timeMessage;
			int		timeColor;

			token = COM_Parse (&s);
			x = midx + (atoi(token) - 160)*scale;
			token = COM_Parse (&s);
			if(newSBlayout)
				y = midy + (atoi(token)/2 - 100)*scale;
			else
				y = midy + (atoi(token) - 100)*scale;

			token = COM_Parse (&s);
			value = atoi(token);
			if (value >= MAX_CLIENTS || value < 0)
				Com_Error (ERR_DROP, "client >= MAX_CLIENTS");
			ci = &cl.clientinfo[value];

			token = COM_Parse (&s);
			score = atoi(token);

			token = COM_Parse (&s);
			ping = atoi(token);

			token = COM_Parse (&s);
			time = atoi(token);

			if ( isQueued ) {
				COM_Parse (&s);
				timeMessage = "Queue:";
				timeColor = 1;
			} else {
				timeMessage = "Time:";
				timeColor = 7;
			}

			if(newSBlayout) { //new scoreboard layout

				box.x = x + 4 * scale , box.y = y + 34 * scale;
				box.width = 160 * scale , box.height = 0;
				FNT_BoundedPrint( font , ci->name , FNT_CMODE_QUAKE_SRS , FNT_ALIGN_LEFT , &box , FNT_colors[ 2 ] );
				box.x += 160 * scale , box.width = 56 * scale , box.height = 0;
				FNT_BoundedPrint( font , va( "%i" , score ) , FNT_CMODE_NONE , FNT_ALIGN_RIGHT , &box , FNT_colors[ 2 ] );
				box.x += 56 * scale , box.width = 48 * scale , box.height = 0;
				FNT_BoundedPrint( font , va( "%i" , ping ) , FNT_CMODE_NONE , FNT_ALIGN_RIGHT , &box , FNT_colors[ 7 ] );
				box.x += 48 * scale , box.width = 48 * scale , box.height = 0;
				FNT_BoundedPrint( font , va( "%i" , time ) , FNT_CMODE_QUAKE , FNT_ALIGN_RIGHT , &box , FNT_colors[ timeColor ] );
			}
			else
			{
				box.x = x + 32 * scale , box.y = y;
				box.width = 288 * scale , box.height = 0;
				FNT_BoundedPrint( font , ci->name , FNT_CMODE_QUAKE_SRS , FNT_ALIGN_LEFT , &box , FNT_colors[ 2 ] );

				box.y += 8 * scale;
				box.width = 100 * scale , box.height = 0;
				FNT_BoundedPrint( font , "Score:" , FNT_CMODE_NONE , FNT_ALIGN_LEFT , &box , FNT_colors[ 7 ] );
				box.x += box.width;
				box.width = 100 * scale - box.width , box.height = 0;
				FNT_BoundedPrint( font , va( "%i" , score ) , FNT_CMODE_NONE , FNT_ALIGN_RIGHT , &box , FNT_colors[ 2 ] );

				box.y += 8 * scale , box.x = x + 32 * scale;
				box.width = 100 * scale , box.height = 0;
				FNT_BoundedPrint( font , "Ping:" , FNT_CMODE_NONE , FNT_ALIGN_LEFT , &box , FNT_colors[ 7 ] );
				box.x += box.width;
				box.width = 100 * scale - box.width , box.height = 0;
				FNT_BoundedPrint( font , va( "%i" , ping ) , FNT_CMODE_NONE , FNT_ALIGN_RIGHT , &box , FNT_colors[ 7 ] );

				box.y += 8 * scale , box.x = x + 32 * scale;
				box.width = 100 * scale , box.height = 0;
				FNT_BoundedPrint( font , timeMessage , FNT_CMODE_NONE , FNT_ALIGN_LEFT , &box , FNT_colors[ timeColor ] );
				box.x += box.width;
				box.width = 100 * scale - box.width , box.height = 0;
				FNT_BoundedPrint( font , va( "%i" , time ) , FNT_CMODE_NONE , FNT_ALIGN_RIGHT , &box , FNT_colors[ timeColor ] );

				if (!ci->icon)
					ci = &cl.baseclientinfo;
				Draw_ScaledPic (x, y, scale/2, ci->iconname);
			}

			continue;
		}
		if (!strcmp(token, "ctf") || !strcmp(token, "tac"))
		{	// draw a team client block
			int	score, ping;
			char	block[80];
			int		team;

			token = COM_Parse (&s);
			x = midx + (atoi(token) - 160)*scale;
			if(atoi(token) < 0)
				team = 0;
			else
				team = 1;
			token = COM_Parse (&s);
			y = midy + (atoi(token) - 100)*scale;

			token = COM_Parse (&s);
			value = atoi(token);
			if (value >= MAX_CLIENTS || value < 0)
				Com_Error (ERR_DROP, "client >= MAX_CLIENTS");
			ci = &cl.clientinfo[value];

			token = COM_Parse (&s);
			score = atoi(token);

			token = COM_Parse (&s);
			ping = atoi(token);
			if (ping > 999)
				ping = 999;

			if(newSBlayout) {
				box.x = x + 14 * scale , box.y = y + 2 * scale;
				box.width = 118 * scale , box.height = 0;
				FNT_BoundedPrint( font , ci->name , FNT_CMODE_QUAKE_SRS , FNT_ALIGN_LEFT , &box , FNT_colors[ 2 ] );
				box.x += 120 * scale , box.width = 56 * scale , box.height = 0;
				FNT_BoundedPrint( font , va( "%i" , score ) , FNT_CMODE_NONE , FNT_ALIGN_RIGHT , &box , FNT_colors[ 2 ] );
				box.x += 56 * scale , box.width = 48 * scale , box.height = 0;
				FNT_BoundedPrint( font , va( "%i" , ping ) , FNT_CMODE_NONE , FNT_ALIGN_RIGHT , &box , FNT_colors[ 7 ] );

				if(team) { //draw pic on left side(done client side to save packetsize
					Draw_ScaledPic (x, y-2*scale, scale/4.0, "/pics/blueplayerbox");
				}
				else
					Draw_ScaledPic(x, y-2*scale, scale/4.0, "/pics/redplayerbox");
			}
			else {
				sprintf(block, "%3d %3d %s", score, ping, ci->name);
				box.x = x , box.y = y , box.width = 256 , box.height = 0;
				FNT_BoundedPrint( font , block , FNT_CMODE_QUAKE_SRS , FNT_ALIGN_LEFT , &box , FNT_colors[ 2 ] );
			}

			continue;
		}

		if (!strcmp(token, "picn"))
		{	// draw a pic from a name
			token = COM_Parse (&s);
			if(newSBlayout && !strcmp(token, "playerbox")) { //cannot simply fill y = ny here
				Draw_ScaledPic (x, ny+32*scale, scale/4.0, token);
			}
			else {
				Draw_ScaledPic (x, y, scale/4.0, token);
			}
			continue;
		}

		if (!strcmp(token, "num"))
		{	// draw a number
			if(strcmp(cl_hudimage1->string, "none")) {
				token = COM_Parse (&s);
				numwidth = atoi(token);
				token = COM_Parse (&s);
				value = cl.frame.playerstate.stats[atoi(token)];
				SCR_DrawField (x, y, 0, numwidth, value, scale);
			}

			continue;
		}

		if (!strcmp(token, "hnum"))
		{	// health number
			if(strcmp(cl_hudimage1->string, "none")) {
				int		color;
				float FRAMETIME = 1.0/(float)server_tickrate;
				long nFrame;

				nFrame = 1 + ((0.1/FRAMETIME) + 1L)/2.0;

				numwidth = 3;
				value = cl.frame.playerstate.stats[STAT_HEALTH];
				if (value > 25)
					color = 0;	// green
				else if (value > 0)
					color = (cl.frame.serverframe>>nFrame) & 1;		// flash
				else
					color = 1;

				SCR_DrawField (x, y, color, numwidth, value, scale);
			}

			//draw the zoom scope pic if we are using the zoom
			if (cl.frame.playerstate.stats[STAT_ZOOMED])
			{
				char zoompic[32];
				sprintf(zoompic, "zoomscope%i", cl.frame.playerstate.stats[STAT_ZOOMED]);
				Draw_StretchPic (left, top, width, height, zoompic);
			}

			continue;
		}

		if (!strcmp(token, "anum"))
		{	// ammo number
			if(strcmp(cl_hudimage1->string, "none")) {
				int		color;
				float FRAMETIME = 1.0/(float)server_tickrate;
				long nFrame;

				nFrame = 1 + ((0.1/FRAMETIME) + 1L)/2.0;

				numwidth = 3;
				value = cl.frame.playerstate.stats[STAT_AMMO];
				if (value > 5)
					color = 0;	// green
				else if (value >= 0)
					color = (cl.frame.serverframe>>nFrame) & 1;		// flash
				else
					continue;	// negative number = don't show

				SCR_DrawField (x, y, color, numwidth, value, scale);
			}

			continue;
		}

		if (!strcmp(token, "rnum"))
		{	// armor number
			if(strcmp(cl_hudimage1->string, "none")) {
				int		color;

				numwidth = 3;
				value = cl.frame.playerstate.stats[STAT_ARMOR];
				if (value < 1)
					continue;

				color = 0;	// green

				SCR_DrawField (x, y, color, numwidth, value, scale);
			}

			continue;
		}


		if (!strcmp(token, "stat_string"))
		{
			token = COM_Parse (&s);
			index = atoi(token);
			if (index < 0 || index >= MAX_CONFIGSTRINGS)
				Com_Error (ERR_DROP, "Bad stat_string index");
			index = cl.frame.playerstate.stats[index];
			if (index < 0 || index >= MAX_CONFIGSTRINGS)
				Com_Error (ERR_DROP, "Bad stat_string index");

			FNT_RawPrint( font , cl.configstrings[index] , strlen( cl.configstrings[index] ) ,
				false , x , y , FNT_colors[ 7 ] );
			continue;
		}

		if (!strcmp(token, "cstring"))
		{
			token = COM_Parse (&s);
			box.width = 2*(x>midx?(right-x):x); 
			box.x = x-box.width/2;
			box.y = y;
			box.height = 0;
			FNT_BoundedPrint( font , token , FNT_CMODE_NONE , FNT_ALIGN_CENTER , &box , FNT_colors[ 7 ] );
			continue;
		}

		if (!strcmp(token, "string"))
		{
			token = COM_Parse (&s);
			//this line is an Alien Arena specific hack of sorts, remove if needed
			if(!strcmp(token, "Vote"))
				Draw_ScaledPic (midx - 85*scale, y-8*scale, scale, "votebox");
			FNT_RawPrint( font , token , strlen( token ) , false , x , y , FNT_colors[ 7 ] );
			continue;
		}

		if (!strcmp(token, "cstring2"))
		{
			token = COM_Parse (&s);
			box.width = 2*(x>midx?(right-x):x); 
			box.x = x-box.width/2;
			box.y = y;
			box.height = 0;
			FNT_BoundedPrint( font , token , FNT_CMODE_NONE , FNT_ALIGN_CENTER , &box , FNT_colors[ 3 ] );
			continue;
		}

		if (!strcmp(token, "string2"))
		{
			token = COM_Parse (&s);
			FNT_RawPrint( font , token , strlen( token ) , false , x , y , FNT_colors[ 7 ] );
			continue;
		}

		if (!strcmp(token, "if"))
		{	// draw a number
			token = COM_Parse (&s);
			value = cl.frame.playerstate.stats[atoi(token)];
			if (!value)
			{	// skip to endif
				while (s && strcmp(token, "endif") )
				{
					token = COM_Parse (&s);
				}
			}

			continue;
		}
	}
}


/*
================
SCR_DrawStats

The status bar is a small layout program that
is based on the stats array
================
*/
void SCR_DrawStats (void)
{
	SCR_ExecuteLayoutString (cl.configstrings[CS_STATUSBAR]);
}


/*
================
SCR_DrawLayout

================
*/
#define	STAT_LAYOUTS		13

void SCR_DrawLayout (void)
{
	if (!cl.frame.playerstate.stats[STAT_LAYOUTS])
		return;
	SCR_ExecuteLayoutString (cl.layout);
}

/*
=================
SCR_DrawPlayerIcon
=================
*/
char		scr_playericon[MAX_OSPATH];
char		scr_playername[PLAYERNAME_SIZE];
float		scr_playericonalpha;
void SCR_DrawPlayerIcon(void) {
	FNT_font_t		font;
	struct FNT_window_s	box;
	int			w, h;
	float			scale, iconPos;

	if (cls.key_dest == key_menu || cls.key_dest == key_console)
		return;

	if(scr_playericonalpha <= 0.0)
		return;

	scr_playericonalpha -= cls.frametime; //fade map pic in

	if(scr_playericonalpha < 1.0)
		iconPos = scr_playericonalpha;
	else
		iconPos = 1.0;

	scale = (float)(viddef.height)/600;
	if(scale < 1)
		scale = 1;

	w = h = 64; //icon size, will change to be larger

	w*=scale;
	h*=scale;

	Draw_AlphaStretchPlayerIcon( -w+(w*iconPos), viddef.height/2 + h/2, w, h, scr_playericon, scr_playericonalpha);

	font = FNT_AutoGet( CL_gameFont );
	box.x = -w+(w*iconPos);
	box.y = viddef.height/2 + h + 32*scale;
	box.width = viddef.width - box.x;
	box.height = 0;
	FNT_BoundedPrint( font , scr_playername , FNT_CMODE_QUAKE , FNT_ALIGN_LEFT , &box , FNT_colors[ 2 ] );
}

/*
================

SCR_showTimer

================
*/
int	seconds, minutes;
void SCR_showTimer(void)
{
	static int		timecounter;
    static perftest_t *test = NULL;
    if (!test) {
        test = get_perftest("timer");
        if (!test)
            return; //couldn't acquire test
        test->is_special = true;
        test->cvar = cl_drawtimer;
    }
    
	if (cls.key_dest == key_menu || cls.key_dest == key_console)
		return;

	if ((cls.realtime + 2000) < timecounter)
		timecounter = cl.time + 1000;

	if (cls.realtime > timecounter)
	{
		seconds += 1;
		if(seconds >= 60) {
			minutes++;
			seconds = 0;
		}
		Com_sprintf(test->text, sizeof(test->text),"time %i:%2.2i", minutes, seconds);
		timecounter = cls.realtime + 1000;
	}
}

/*
================

SCR_showPerfTest

================
*/

perftest_t *get_perftest(char *name) {
    int         i; 
    perftest_t  *test = NULL;
    
    for (i = 0; i < MAX_PERFTESTS; i++) {
        if (!perftests[i].in_use) {
            test = &perftests[i];
            break;
        }
        if (!strncmp(perftests[i].name, name, sizeof(perftests[i].name)))
            return &perftests[i];
    }
    
    if (!test) 
        return NULL; //remember to handle this in your code!
    
    memset(test, 0, sizeof(perftest_t));
    
    test->in_use = true;
    strncpy (test->name, name, sizeof(test->name));
    
    return test;
}    

//slotnum is used for the vertical offset on the screen
void SCR_showPerfTest (perftest_t *test, int slotnum) {
	FNT_font_t		font;
	int			    end_msec;
	float			time_sec;
	float			rate;
	int				height;
	
	if (!test->in_use)
	    return;
	
	if (cls.key_dest == key_menu || cls.key_dest == key_console || !test->cvar->integer) {
		test->restart = true;
		return;
	}
	
	if (test->restart && !test->is_special) {
	    test->start_msec = Sys_Milliseconds();
	    test->counter = 0.0f;
	    test->framecounter = 0.0f;
	    test->update_trigger = 10.0f; //initial delay to update
	    test->text[0] = 0; //blank the text buffer
	    test->restart = false;
	    return;
	}
	
	test->framecounter += 1.0f;
	if (test->framecounter >= test->update_trigger && !test->is_special) {
	    end_msec = Sys_Milliseconds();
        time_sec = ((float)(end_msec - test->start_msec)) / 1000.0f;
	    if (test->is_timerate) {
	        //calculate rate to display
	        rate = test->scale * test->counter / time_sec;
	    } else {
	        rate = test->scale * test->counter;
	    }
	    
	    //update text buffer for display
	    Com_sprintf( test->text, sizeof(test->text), test->format, rate );
	    
	    test->start_msec = end_msec;
	    if (test->is_timerate)
	        //setup for next update - 0.25 sec update interval
	        test->update_trigger = test->framecounter / (4.0 * time_sec); 
	    else
	        //0.05 sec update interval
	        test->update_trigger = test->framecounter / (20.0 * time_sec); 
	    test->framecounter = 0.0f;
	    test->counter = 0.0f;
	} 
	
	font = FNT_AutoGet( CL_gameFont );

	height = viddef.height - font->size * (2.0 * (slotnum + 1) + 8.0);
	FNT_RawPrint( font , test->text , strlen( test->text ) , false ,
		viddef.width - 8 * font->size , height, FNT_colors[ 2 ] );
}

void SCR_showAllPerfTests (void) {
    int i, testsDrawn;
    for (i = 0, testsDrawn = 0; i < MAX_PERFTESTS; i++) {
        if (perftests[i].in_use && perftests[i].cvar->integer) {
            SCR_showPerfTest (&perftests[i], testsDrawn);
            testsDrawn++;
        }
    }
}

void SCR_showFPS(void)
{
    static perftest_t *test = NULL;
    if (!test) {
        test = get_perftest("fps");
        if (!test)
            return; //couldn't acquire test
        test->cvar = cl_drawfps;
        test->is_timerate = true;
        strcpy (test->format, "%3.0ffps");
        test->scale = 1.0f;
    }
    
    test->counter += 1.0f;
    
}

// Shows kilobytes per second from the server
extern int c_incoming_bytes;
void SCR_showBandwidth (void) {
    static perftest_t *test = NULL;
    if (!test) {
        test = get_perftest("bandwidth");
        if (!test)
            return; //couldn't acquire test
        test->is_timerate = true;
        test->cvar = cl_drawbandwidth;
        strcpy (test->format, "%2.2fkibps");
        test->scale = 1.0f/1024.0f; //the SI is wrong, it's 1024
    }
    
    test->counter += c_incoming_bytes;
    c_incoming_bytes = 0;
}

// Shows number of frames per second which took longer than a configurable
// amount of milliseconds. This is a useful measure of jitter, which isn't
// usually reflected in average FPS rates but which can still be visually
// annoying. 
void SCR_showFramesOver (void) {
    static perftest_t *test = NULL;
    static int old_milliseconds = 0;
    int new_milliseconds;
    if (!test) {
        test = get_perftest("framesover");
        if (!test)
            return; //couldn't acquire test
        test->is_timerate = true;
        test->cvar = cl_drawframesover;
        strcpy (test->format, "%3.0fjfps"); //jfps = jitter frames per second
        test->scale = 1.0f;
        old_milliseconds = Sys_Milliseconds();
    }
    
    new_milliseconds = Sys_Milliseconds();
    if ((new_milliseconds - old_milliseconds) > cl_drawframesover->integer)
    	test->counter++;
    old_milliseconds = new_milliseconds;
}

/**
 *
 * @brief Construct map-maker's "r_speed" performance counter displays
 *
 * the traditional r_speeds cvar is replaced by individual counter cvars,
 *  named "rspeed_*"
 */
static void show_rspeed_helper(
	cvar_t*     test_cvar,
	perftest_t* test_obj,
	char*       test_name,
	char*       test_format,
	int         test_counter
	)
{
	if ( test_cvar && test_cvar->integer )
	{
		if ( test_obj == NULL )
		{ /* constructor */
			test_obj = get_perftest( test_name );
			if ( test_obj != NULL )
			{ /* slot allocated, construct */
				test_obj->is_timerate = false;
				test_obj->cvar = test_cvar;
				strcpy( test_obj->format, test_format );
				test_obj->scale = 1.0f;
				test_obj->counter = test_counter;
			}
			else
			{ /* no slot, force disable */
				(void)Cvar_ForceSet( test_cvar->name, "0" );
			}
		}
		else
		{ /* running, update counter from refresh counter */
			test_obj->counter = test_counter;
		}
	}
}

void SCR_showRSpeeds( void )
{
	static perftest_t *test_wpoly    = NULL;
	static perftest_t *test_epoly    = NULL;
	static perftest_t *test_flares   = NULL;
	static perftest_t *test_grasses  = NULL;
	static perftest_t *test_beams    = NULL;
	static perftest_t *test_vbobatches = NULL;
	static perftest_t *test_particles = NULL;

	show_rspeed_helper( rspeed_wpolys, test_wpoly,
			"wpolys", "%5.0f wpolys", c_brush_polys );

	show_rspeed_helper( rspeed_epolys, test_epoly,
			"epolys", "%5.0f epolys", c_alias_polys );

	show_rspeed_helper( rspeed_flares, test_flares,
			"flares", "%5.0f flares", c_flares );

	show_rspeed_helper( rspeed_grasses, test_grasses,
			"grasses", "%5.0f grasses", c_grasses );

	show_rspeed_helper( rspeed_beams, test_beams,
			"beams", "%5.0f beams", c_beams );
	
	show_rspeed_helper( rspeed_vbobatches, test_vbobatches,
			"vbobatches", "%5.0f vbobatches", c_vbo_batches );
	
	show_rspeed_helper( rspeed_particles, test_particles,
			"particles", "%5.0f particles", cl.refdef.num_particles );

}

/*
==================
SCR_UpdateScreen

This is called every frame, and can also be called explicitly to flush
text to the screen.
==================
*/
void SCR_UpdateScreen (void)
{
	int numframes;
	int i;
	static int last_loading_percent = -1;
	float separation[2] = { 0, 0 };

	// if the screen is disabled (loading plaque is up, or vid mode changing)
	// do nothing at all
	if (cls.disable_screen)
	{
		if (Sys_Milliseconds() - cls.disable_screen > 120000)
		{
			cls.disable_screen = 0;
			Com_Printf ("Loading plaque timed out.\n");
			return;
		}
		scr_draw_loading = 2;
	}


	if (!scr_initialized)
		return;				// not initialized yet

	/*
	** range check cl_camera_separation so we don't inadvertently fry someone's
	** brain
	*/
	if ( cl_stereo_separation->value > 1.0 )
		Cvar_SetValue( "cl_stereo_separation", 1.0 );
	else if ( cl_stereo_separation->value < 0 )
		Cvar_SetValue( "cl_stereo_separation", 0.0 );

	if ( cl_stereo->value )
	{
		numframes = 2;
		separation[0] = -cl_stereo_separation->value / 2;
		separation[1] =  cl_stereo_separation->value / 2;
	}
	else
	{
		separation[0] = 0;
		separation[1] = 0;
		numframes = 1;
	}
	
	if (scr_draw_loading)
	{
		if (loadingPercent == last_loading_percent)
			return;
		last_loading_percent = loadingPercent;
	}
	else
	{
		last_loading_percent = -1;
	}

	for ( i = 0; i < numframes; i++ )
	{
		R_BeginFrame( separation[i] );

		if (scr_draw_loading == 2)
		{	//  loading plaque over black screen
			SCR_DrawLoading ();

			if (cls.disable_screen)
				scr_draw_loading = 2;

			//NO CONSOLE!!!
			continue;
		}
		else
		{
			// do 3D refresh drawing, and then update the screen
			SCR_CalcVrect ();

			V_RenderView ( separation[i] );

			SCR_DrawStats ();
			if (cl.frame.playerstate.stats[STAT_LAYOUTS] & 1)
				SCR_DrawLayout ();
			if (cl.frame.playerstate.stats[STAT_LAYOUTS] & 2)
				CL_DrawInventory ();

			SCR_DrawNet ();
			SCR_CheckDrawCenterString ();
			SCR_CheckDrawIRCString();

			if (scr_timegraph->value)
				SCR_DebugGraph (cls.frametime*600, RGBA(0,0,0,1));

			if (scr_debuggraph->value || scr_timegraph->value || scr_netgraph->value)
				SCR_DrawDebugGraph ();

			SCR_DrawPause ();

			M_Draw ();

			SCR_DrawConsole ();

			SCR_DrawLoading ();

			if(cl_drawfps->integer)
			{
				SCR_showFPS();
			}
			
			if(cl_drawbandwidth->integer)
			{
				SCR_showBandwidth();
			}
			
			if(cl_drawframesover->integer)
			{
				SCR_showFramesOver();
			}

			SCR_showRSpeeds(); /* map-maker's performance counters */

			if(cl_drawtimer->integer)
			{
				SCR_showTimer();
			}
			
			SCR_showAllPerfTests();

			SCR_DrawPlayerIcon();
		}
	}
	R_EndFrame();
}
