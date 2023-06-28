/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2005-2013 COR Entertainment, LLC.

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

/* To do - clean up these warnings

10>C:\alienarena_w32\source\client\menu.c(1808): warning C4090: '=' : different 'const' qualifiers
10>C:\alienarena_w32\source\client\menu.c(1820): warning C4090: '=' : different 'const' qualifiers

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <ctype.h>
#if defined WIN32_VARIANT
#include <winsock.h>
#endif

#if defined UNIX_VARIANT
#include <sys/time.h>
#if defined HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif

#if defined WIN32_VARIANT
#include <io.h>
#endif

#include "curl/curl.h"

#include "client.h"
#include "client/qmenu.h"

#if !defined HAVE__STRDUP
#if defined HAVE_STRDUP
#define _strdup strdup
#endif
#endif

#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

// After a float has been sprintf'd to a buffer, strip trailing zeroes and
// possibly trailing decimal point.
static void cleanup_float_string (char *str)
{
	char *cursor = strchr (str, '\0') - 1;
	
	while (cursor > str && *cursor == '0')
		*cursor-- = '\0';
	
	if (*cursor == '.')
	{
		if (cursor > str)
			*cursor = '\0';
		else
			*cursor = '0';
	}
}

/* ---- GAME MODES ---- */

enum Game_mode
{
	mode_dm   = 0, 
	mode_ctf  = 1, // team
	mode_tac  = 2, // team
	mode_duel = 3
};

static const char *game_mode_names[] =
{
	"deathmatch",        // 0 
	"ctf",               // 1
	"tactical",          // 2
	"duel",              // 3
	NULL
};
#define num_game_modes (static_array_size(game_mode_names)-1)

//same order as game_mode_names
static const char *map_prefixes[num_game_modes][3] =
{
	{"dm",  "tourney", NULL}, // 0
	{"ctf", NULL,      NULL}, // 1
	{"tac", NULL,      NULL}, // 2
	{"dm",  "tourney", NULL}  // 3
};

static void SetGameModeCvars (enum Game_mode mode)
{
	int cvflags = CVAR_LATCH | CVAR_GAMEINFO | CVARDOC_BOOL;

	// The deathmatch cvar doesn't specifically indicate a pure frag-to-win
	// game mode. It's actually the "enable multiplayer" cvar.
	// It protects from running vestigial Quake non-multiplayer code.
	Cvar_SetValue ("deathmatch", 1);

	Cvar_FullSet ("ctf",    "0", cvflags);
	Cvar_FullSet ("g_duel", "0", cvflags);
	Cvar_FullSet ("g_tactical", "0", cvflags);
	Cvar_SetValue ("gamerules", mode);
	
	switch (mode)
	{
		case mode_ctf:
			Cvar_ForceSet ("ctf", "1");
			break;
		case mode_tac:
			Cvar_ForceSet ("g_tactical", "1");
			break;
		case mode_duel:
			Cvar_ForceSet ("g_duel", "1");
			break;
		default:
			break;
	}
}

/*--------*/

static int	m_main_cursor;
static int news_start_time;

extern int CL_GetPingStartTime(netadr_t adr);

extern void RS_LoadScript(char *script);
extern void RS_LoadSpecialScripts(void);
extern void RS_ScanPathForScripts(void);
extern void RS_FreeUnmarked (void);
extern void SCR_DrawCenterString (void);
extern cvar_t *scriptsloaded;
#if defined WIN32_VARIANT
extern char map_music[MAX_PATH];
#else
extern char map_music[MAX_OSPATH];
#endif
extern cvar_t *background_music;
extern cvar_t *background_music_vol;
extern cvar_t *fov;
extern cvar_t *stats_password;
extern CURL *curl;

static char *menu_in_sound		= "misc/menu1.wav";
static char *menu_move_sound	= "misc/menu2.wav";
static char *menu_out_sound		= "misc/menu3.wav";

#define PLAYER_NAME_UNIQUE (strcmp (Cvar_VariableString ("name"), "Player") != 0)

void SetCrosshairNames (char **list);
void SetHudNames (char **list);
void SetFontNames (char **list);

void M_Menu_Main_f (void);
	static void M_Menu_PlayerConfig_f (void);
	static void M_Menu_Game_prototype_f (void);
	static void M_Menu_Game_f (void);
	static void M_Menu_JoinServer_f (void);
			static void M_Menu_AddressBook_f( void );
			static void M_Menu_PlayerRanking_f( void );
			static void M_Menu_Tactical_f( void );
	static void M_Menu_StartServer_f (void);
			static void M_Menu_BotOptions_f (void);
	static void M_Menu_IRC_f (void);
	static void M_Menu_Options_f (void);
		static void M_Menu_Video_f (void);
		static void M_Menu_Keys_f (void);
	static void M_Menu_Quit_f (void);

	static void M_Menu_Credits_f (void);

qboolean	m_entersound;		// play after drawing a frame, so caching
								// won't disrupt the sound

static size_t szr; // just for unused result warnings

static inline qboolean is_team_game (float rule_value)
{
	int rv = (int)rule_value;
	Com_DPrintf("[is_team_game: rule_value:  %i]\n", rule_value );

	return ( rv == mode_ctf );
}

// common callbacks

static void StrFieldCallback( void *_self )
{
	menufield_s *self = (menufield_s *)_self;
	Cvar_Set( self->generic.localstrings[0], self->buffer);
}

static void IntFieldCallback( void *_self )
{
	menufield_s *self = (menufield_s *)_self;
	Cvar_SetValue( self->generic.localstrings[0], atoi(self->buffer));
}

static menuvec2_t PicSizeFunc (void *_self, FNT_font_t font)
{
	menuvec2_t	ret;
	menuitem_s	*self = (menuitem_s *)_self;
	
	ret.x = ret.y = 0;
	
	// Determine if pic exists, if not return 0 size. 
	// However, we give the benefit of the doubt if the name isn't there, and
	// assume it will be.
	if (self->generic.localstrings[0] == NULL || Draw_PicExists (self->generic.localstrings[0]))
	{
		ret.x = self->generic.localints[0]*font->size;
		ret.y = self->generic.localints[1]*font->size;
		ret.x += self->generic.localints[2];
	}
	
	return ret;
}

// most useful if this element will always draw the same pic
static void PicDrawFunc (void *_self, FNT_font_t font)
{
	int x, y;
	menuitem_s *self = (menuitem_s *)_self;
	
	x = Item_GetX (*self) + self->generic.localints[2];
	y = Item_GetY (*self);
	
	Draw_StretchPic (x, y, font->size*self->generic.localints[0], font->size*self->generic.localints[1], self->generic.localstrings[0]);
}

// for spin controls where each item is a texture path
static menuvec2_t PicSpinSizeFunc (void *_self, FNT_font_t font)
{
	menuvec2_t	ret;
	menulist_s	*self = (menulist_s *)_self;
	
	ret.x = self->generic.localints[0]*font->size;
	ret.y = self->generic.localints[1]*font->size;
	ret.x += self->generic.localints[2];
	
	return ret;
}

static void PicSpinDrawFunc (void *_self, FNT_font_t font)
{
	int x, y;
	menulist_s *self = (menulist_s *)_self;
	
	x = Item_GetX (*self);
	y = Item_GetY (*self);
	x += self->generic.localints[2];
	
	if (strlen(self->itemnames[self->curvalue]) > 0)
		Draw_StretchPic (x, y,font->size*self->generic.localints[0], font->size*self->generic.localints[1], self->itemnames[self->curvalue]);
}


// name lists: list of strings terminated by 0

static const char *onoff_names[] =
{
	"",
	"menu/on",
	0
};

// when you want 0 to be on
static const char *offon_names[] =
{
	"menu/on",
	"",
	0
};

static const char *windowmode_names[] =
{
	"windowed",
	"fullscreen",
	0
};

static const char *windowmode_names_legacy[] =
{
	"windowed",
	"fullscreen",
	"fullscreen (legacy)", // previously called "fullscreen (exclusive)"
	0
};

static menuvec2_t IconSpinSizeFunc (void *_self, FNT_font_t font)
{
	menuvec2_t ret;
	menulist_s *self = (menulist_s *)_self;
	
	ret.x = ret.y = font->size;
	ret.x += RCOLUMN_OFFSET;
	if ((self->generic.flags & QMF_RIGHT_COLUMN)) 
		ret.x += Menu_PredictSize (self->generic.name);
	return ret;
}

static void IconSpinDrawFunc (void *_self, FNT_font_t font)
{
	int x, y;
	menulist_s *self = (menulist_s *)_self;
	
	x = Item_GetX (*self)+RCOLUMN_OFFSET;
	y = Item_GetY (*self)+MenuText_UpperMargin (self, font->size);
	if ((self->generic.flags & QMF_RIGHT_COLUMN))
		x += Menu_PredictSize (self->generic.name);
	Draw_AlphaStretchPic (
		x, y, font->size, font->size, "menu/icon_border", 
		self->generic.highlight_alpha*self->generic.highlight_alpha
	);
	if (strlen(self->itemnames[self->curvalue]) > 0)
		Draw_AlphaStretchPic (x, y, font->size, font->size, self->itemnames[self->curvalue], self->generic.highlight_alpha);
}

#define setup_tickbox(spinctrl) \
{ \
	(spinctrl).generic.type = MTYPE_SPINCONTROL; \
	(spinctrl).generic.itemsizecallback = IconSpinSizeFunc; \
	(spinctrl).generic.itemdraw = IconSpinDrawFunc; \
	(spinctrl).itemnames = onoff_names; \
	(spinctrl).generic.flags |= QMF_ALLOW_WRAP; \
	(spinctrl).curvalue = 0; \
}

static void RadioSpinDrawFunc (void *_self, FNT_font_t font)
{
	int x, y;
	menulist_s *self = (menulist_s *)_self;
	
	x = Item_GetX (*self)+RCOLUMN_OFFSET;
	y = Item_GetY (*self)+MenuText_UpperMargin (self, font->size);
	if ((self->generic.flags & QMF_RIGHT_COLUMN))
		x += Menu_PredictSize (self->generic.name);
	Draw_AlphaStretchPic (
		x, y, font->size, font->size, "menu/radio_border", 
		self->generic.highlight_alpha*self->generic.highlight_alpha
	);
	if (strlen(self->itemnames[self->curvalue]) > 0)
		Draw_AlphaStretchPic (x, y, font->size, font->size, self->itemnames[self->curvalue], self->generic.highlight_alpha);
}

#define setup_radiobutton(spinctrl) \
{ \
	(spinctrl).generic.type = MTYPE_SPINCONTROL; \
	(spinctrl).generic.itemsizecallback = IconSpinSizeFunc; \
	(spinctrl).generic.itemdraw = RadioSpinDrawFunc; \
	(spinctrl).itemnames = onoff_names; \
	(spinctrl).generic.flags |= QMF_ALLOW_WRAP; \
	(spinctrl).curvalue = 0; \
}

#define setup_nth_window(parent,n,window,title) \
{ \
	(parent).nitems = n; \
	(parent).num_apply_pending = 0; \
	\
	(window).generic.type = MTYPE_SUBMENU; \
	(window).navagable = true; \
	(window).nitems = 0; \
	(window).bordertitle = title; \
	(window).bordertexture = "menu/m_"; \
	\
	Menu_AddItem (&(parent), &(window)); \
}

#define setup_window(parent,window,title) setup_nth_window(parent,0,window,title)

#define setup_panel(parent,panel) \
{ \
	(panel).generic.type = MTYPE_SUBMENU; \
	(panel).generic.flags = QMF_SNUG_LEFT; \
	(panel).navagable = true; \
	(panel).nitems = 0; \
	(panel).bordertexture = "menu/sm_"; \
	Menu_AddItem (&(parent), &(panel)); \
}

// if you just want to add some text to a menu and never need to refer to it
// again (don't use inside a loop!)
#define add_text(menu,text,itflags) \
{\
	static menutxt_s it; \
	it.generic.type = MTYPE_TEXT; \
	it.generic.flags = (itflags); \
	it.generic.name = (text); \
	Menu_AddItem(&(menu), &(it)); \
}

// if you just want to add an action to a menu and never need to refer to it
// again (don't use inside a loop!)
#define add_action(menu,itname,itcallback,itflags) \
{\
	static menuaction_s it; \
	it.generic.type = MTYPE_ACTION; \
	it.generic.flags = (itflags)|QMF_BUTTON; \
	it.generic.name = (itname); \
	it.generic.callback = (itcallback); \
	Menu_AddItem (&(menu), &(it)); \
}


// Should be useful for most menus
#define M_PushMenu_Defaults(struct) \
	M_PushMenu (Screen_Draw, Default_MenuKey, &(struct))

static inline void refreshCursorButton (int button)
{
	cursor.buttonused[button] = true;
	cursor.buttonclicks[button] = 0;
}

static void refreshAllCursorButtons (void)
{
	int i;
	for (i = 0; i < MENU_CURSOR_BUTTON_MAX; i++)
		refreshCursorButton (i);
}

//=============================================================================
/*	Screen layout routines -- responsible for tiling all the levels of menus
	on the screen and animating transitions between them. This uses a finite
	state machine to track which windows are active, "incoming" (will become
	active after the current animation is complete,) and "outgoing" (will no
	longer be active after the current animation is complete. If there are
	incoming or outgoing windows, user input is disabled until the transition
	animation is complete.
	
	Each window is a menu tree (a menuframework_s struct with submenus.) The 
	purpose of the animation code is to determine what x-axis offset each 
	window should be drawn at (that is, what number of pixels should be added
	to the x-axis of the window when it is drawn.) The x offset for each
	window is recalculated each frame so that the windows tile neatly
	alongside each other, slide across the screen, etc. 
	
	The main menu is a special case, in that it will shrink into a sidebar 
	instead of appearing partially off screen.
	
	Architecturally, this is done with a simple finite-state machine.
*/

// M_Interp - responsible for animating the transitions as menus are added and
// removed from the screen.  Actually, this function just performs an
// interpolation between 0 and target, returning a number that should be used
// next time for progress. you determine roughly how many pixels a menu is
// going to slide, and in which direction.  Your target is that number of
// pixels, positive or negative depending on the direction.  Call this
// function repeatedly with your target, and it will return a series of pixel
// offsets that can be used in your animation.
static int M_Interp (int progress, int target)
{
	int increment = 0; // always positive
	
	// Determine the movement amount this frame. Make it nice and fast,
	// because while slow might look cool, it's also an inconvenience.
	if (target != progress)
	{
		static float frametime_accum = 0;

		// The animation speeds up as it gets further from the starting point
		// and slows down twice as fast as it approaches the ending point.
		increment = min(	abs((11*target)/10-progress)/2,
							abs(progress) ) * 40;
		
		// Clamp the animation speed at a minimum so it won't freeze due to
		// rounding errors or take too long at either end.
		increment = max (increment, abs(target)/10);

		// Scale the animation by frame time so its speed is independent of 
		// framerate. At very high framerates, each individual frame might be
		// too small a time to result in an integer amount of movement. So we
		// just add frames together until we do get some movement.
		frametime_accum += cls.frametime;
		increment *= frametime_accum;

		if (increment > 0)
			frametime_accum = 0;
		else
			return progress; // no movement, better luck next time.
	}

	if (target > 0)
	{
		// increasing
		progress += increment;
		progress = min (progress, target); // make sure we don't overshoot
	}
	else if (target < 0)
	{
		// decreasing
		progress -= increment;
		progress = max (progress, target); // make sure we don't overshoot
	}

	return progress;
}

// linear interpolation
#define lerp(start,end,progress) ((start) + (double)((end)-(start))*(progress))

#define	MAX_MENU_DEPTH	8

#define sidebar_width ((float)(150*viddef.width)/1024.0)

// Window wrapper
// (TODO: rename all mention of "layer" to "screen" or "window," haven't
// decided which yet.)
typedef struct
{
	void	(*draw) (menuframework_s *screen, menuvec2_t offset);
	const char *(*key) (menuframework_s *screen, int k);
	menuframework_s *screen;
} menulayer_t;

// An "inelastic" row of windows. They always tile side by side, and the total
// width is always the sum of the contained windows. This struct is for
// convenience; it's easier to animate such a group of windows as a single 
// unit.
typedef struct
{
	int			offset; // starting x-axis pixel offset for the leftmost window
	int			num_layers;
	menulayer_t	layers[MAX_MENU_DEPTH];
} layergroup_t;

#define layergroup_last(g) ((g).layers[(g).num_layers-1])

// add up all the widths of each window in the group
static inline int layergroup_width (layergroup_t *g)
{
	int i, ret;
	
	ret = 0;
	for (i = 0; i < g->num_layers; i++)
		ret += Menu_TrueWidth (*g->layers[i].screen);
	return ret;
}

// Add up the widths of each window in the group that cannot fit on screen,
// starting with the leftmost. If the final (deepest) window is itself too 
// wide, it still won't be included.
static inline int layergroup_excesswidth (layergroup_t *g)
{
	int i, ret, w;
	
	ret = w = layergroup_width (g);
	for (i = 0; i < g->num_layers-1; i++)
	{
		if (ret < viddef.width)
			break;
		ret -= Menu_TrueWidth (*g->layers[i].screen);
	}
	return w-ret;
}

// Like layergroup_excesswidth, but as if the windows from the two groups were
// hypothetically in the same group.
static inline int layergroup_pair_excesswidth (layergroup_t *g1, layergroup_t *g2)
{
	int i, ret, w;
	
	
	if (g2->num_layers == 0)
		return layergroup_excesswidth (g1);
	if (g1->num_layers == 0)
		return layergroup_excesswidth (g2);
	
	ret = w = layergroup_width (g1) + layergroup_width (g2);
	for (i = 0; i < g1->num_layers; i++)
	{
		if (ret < viddef.width)
			break;
		ret -= Menu_TrueWidth (*g1->layers[i].screen);
	}
	for (i = 0; i < g2->num_layers-1; i++)
	{
		if (ret < viddef.width)
			break;
		ret -= Menu_TrueWidth (*g2->layers[i].screen);
	}
	return w-ret;
}
	

static void layergroup_draw (layergroup_t *g)
{
	int i;
	menuvec2_t offs;
	offs.y = 0;
	offs.x = g->offset;
	for (i = 0; i < g->num_layers; i++)
	{
		g->layers[i].draw (g->layers[i].screen, offs);
		offs.x += Menu_TrueWidth (*g->layers[i].screen);
	}
}

// this holds the state machine state
static struct
{
	enum
	{
		mstate_steady,	// no animation, incoming & outgoing empty
		mstate_insert,	// menus being added, possibly some outgoing menus
		mstate_remove	// outgoing menus
	} state;
	layergroup_t active;
	layergroup_t outgoing;
	layergroup_t incoming;
	int animation; // current animation pixel offset
} mstate;

static inline void mstate_reset (void)
{
	mstate.active.num_layers = mstate.incoming.num_layers = mstate.outgoing.num_layers = 0;
	mstate.state = mstate_steady;
	refreshCursorLink ();
}

#define activelayer(idx) (mstate.active.layers[(idx)])

int Cursor_GetLayer (void)
{
	int i;
	menuframework_s *screen;
	
	if (cursor.menuitem == NULL)
		Com_Error (ERR_FATAL, "Cursor_GetLayer: unset cursor.menuitem!");
	
	screen = Menu_GetItemTree (cursor.menuitem);
	
	for (i = 0; i < mstate.active.num_layers; i++)
	{
		if (activelayer(i).screen == screen)
			return i;
	}

	// We only get here if, after changing resolutions, the mouse is no longer
	// on screen.
	Com_Printf ("WARN: fake cursor.menulayer!\n");	
	return -1;
}

static int activelayer_coordidx (int xcoord)
{
	int i;
	xcoord -= mstate.active.offset;
	if (xcoord < 0 || mstate.active.num_layers == 0)
		return -1;
	for (i = 0; i < mstate.active.num_layers; i++)
	{
		xcoord -= Menu_TrueWidth (*activelayer(i).screen);
		if (xcoord < 0)
			break;
	}
	return i;
}

// Figure out the starting offset for the leftmost window of the "active"
// (neither incoming nor outgoing) window group. Usually just equal to the 
// maximum width of the sidebar, unless there are so many large windows that
// they can't all fit on screen at once, in which case it may be a negative
// number.
static inline int Menuscreens_Animate_Active (void)
{
	int shove_offset, ret, excess;
	excess = layergroup_excesswidth (&mstate.active);
	if (excess != 0)
		return -excess;
	ret = sidebar_width;
	shove_offset = viddef.width - layergroup_width (&mstate.active);
	if (shove_offset < ret)
		ret = shove_offset;
	return ret;
}

// Figure out the starting offset for the leftmost window of the active window
// group, *if* the "incoming" windows were hypothetically added to the end of
// the active window group. Will be used as the "target" for the incoming-
// window animation. This is because when the animation is done, the incoming
// windows will be added to the end of the active window group, and we want 
// the transition to be smooth.
static inline int MenuScreens_Animate_Incoming_Target (void)
{
	int shove_offset, ret, excess;
	excess = layergroup_pair_excesswidth (&mstate.active, &mstate.incoming);
	if (excess != 0)
		return -excess;
	ret = sidebar_width;
	shove_offset = viddef.width - layergroup_width (&mstate.active) - layergroup_width (&mstate.incoming);
	if (shove_offset < ret)
		ret = shove_offset;
	return ret;
}

// Figure out the starting offset for the leftmost window of the "active" 
// window group, *if* the outgoing windows were hypothetically added to the
// end of the active window group. Will be used as the "start" for the
// outgoing- window animation. This is because before the animation started,
// the outgoing windows were at the end of the active window group, and we 
// want the transition to be smooth.
static inline int MenuScreens_Animate_Outgoing_Start (void)
{
	int shove_offset, ret, excess;
	excess = layergroup_pair_excesswidth (&mstate.active, &mstate.outgoing);
	if (excess != 0)
		return -excess;
	ret = sidebar_width;
	shove_offset = viddef.width - layergroup_width (&mstate.active) - layergroup_width (&mstate.outgoing);
	if (shove_offset < ret)
		ret = shove_offset;
	return ret;
}

static void Menuscreens_Animate (void);

// state machine state transitions
static void Menuscreens_Animate_Insert_To_Steady (void)
{
	int i;
	for (i = 0; i < mstate.incoming.num_layers; i++)
		activelayer(mstate.active.num_layers++) = mstate.incoming.layers[i];
	Cursor_SelectMenu (layergroup_last(mstate.active).screen);
	mstate.incoming.num_layers = 0;
	mstate.outgoing.num_layers = 0;
	mstate.state = mstate_steady;
	mstate.animation = 0;
	Menuscreens_Animate ();
}
static void Menuscreens_Animate_Remove_To_Steady (void)
{
	mstate.outgoing.num_layers = 0;
	mstate.state = mstate_steady;
	if (mstate.active.num_layers == 0)
		refreshCursorLink ();
	else
		Cursor_SelectMenu (layergroup_last(mstate.active).screen);
	mstate.animation = 0;
	Menuscreens_Animate ();
}

static void M_Main_Draw (menuvec2_t offset);
void CheckMainMenuMouse (void);

// This is where the magic happens. (TODO: maybe separate the actual rendering
// out into a different function?)
static void Menuscreens_Animate (void)
{
	int shove_offset, anim_start, anim_end;
	menuvec2_t main_offs;
	
	main_offs.x = main_offs.y = 0;
	
	switch (mstate.state)
	{
	case mstate_steady:
	{
		if (mstate.active.num_layers != 0)
		{
			mstate.active.offset = Menuscreens_Animate_Active ();
			main_offs.x = mstate.active.offset-viddef.width;
		}
		
		M_Main_Draw (main_offs);
		layergroup_draw (&mstate.active);
	}
	break;
	case mstate_insert:
	{
		if (mstate.active.num_layers == 0)
			mstate.active.offset = 0;
		else
			mstate.active.offset = Menuscreens_Animate_Active ();
		
		anim_start = mstate.active.offset+viddef.width;
		anim_end = MenuScreens_Animate_Incoming_Target ();  
		
		mstate.animation = M_Interp (mstate.animation, anim_end-anim_start);
		if (mstate.animation <= anim_end-anim_start)
		{
			Menuscreens_Animate_Insert_To_Steady ();
			return;
		}
		shove_offset = anim_start+mstate.animation;
		
		if (shove_offset < mstate.active.offset || mstate.active.num_layers == 0)
			mstate.active.offset = shove_offset;
		
		// If there are outgoing windows, the incoming ones "push" them back
		// behind the active windows and the sidebar.
		if (mstate.outgoing.num_layers > 0)
		{
			int outgoing_shove, outgoing_start, outgoing_end;
			double outgoing_fade;
			
			mstate.outgoing.offset = outgoing_start = MenuScreens_Animate_Outgoing_Start () + layergroup_width (&mstate.active);
			
			outgoing_end = Menuscreens_Animate_Active () - layergroup_width (&mstate.outgoing);
			
			outgoing_shove = shove_offset + layergroup_width (&mstate.active) - layergroup_width (&mstate.outgoing);
			if (outgoing_shove < mstate.outgoing.offset)
				mstate.outgoing.offset = outgoing_shove;
			
			layergroup_draw (&mstate.outgoing);
			
			outgoing_fade = (double)(mstate.outgoing.offset-outgoing_start)/(double)(outgoing_end-outgoing_start);
			
			// Fade out the outgoing windows
			Draw_Fill (
				mstate.outgoing.offset, 0,
				layergroup_width (&mstate.outgoing), viddef.height,
				RGBA (0, 0, 0, sqrt(outgoing_fade))
			);
			
			// Interpolate the sidebar as well.
			mstate.active.offset = lerp (
				MenuScreens_Animate_Outgoing_Start (),
				MenuScreens_Animate_Incoming_Target (),
				outgoing_fade
			);
		}
			
		main_offs.x = mstate.active.offset-viddef.width;
		mstate.incoming.offset = shove_offset + layergroup_width (&mstate.active);
		
		M_Main_Draw (main_offs);
		layergroup_draw (&mstate.active);
		layergroup_draw (&mstate.incoming);
	}
	break;
	case mstate_remove:
	{
		if (mstate.active.num_layers == 0)
			mstate.active.offset = 0;
		else
			mstate.active.offset = Menuscreens_Animate_Active ();
		
		anim_start = MenuScreens_Animate_Outgoing_Start ();
		anim_end = mstate.active.offset + viddef.width;
		
		mstate.animation = M_Interp (mstate.animation, anim_end-anim_start);
		if (mstate.animation >= anim_end-anim_start)
		{
			Menuscreens_Animate_Remove_To_Steady ();
			return;
		}
		shove_offset = anim_start+mstate.animation;
		
		if (shove_offset < mstate.active.offset || mstate.active.num_layers == 0)
			mstate.active.offset = shove_offset;
		
		main_offs.x = mstate.active.offset-viddef.width;
		mstate.outgoing.offset = shove_offset + layergroup_width (&mstate.active);

		M_Main_Draw (main_offs);
		layergroup_draw (&mstate.active);
		layergroup_draw (&mstate.outgoing);
	}
	break;
	}
}

// These functions (Push, Force Off, and Pop) are used by the outside world to
// control the state machine.

static void M_PushMenu (void (*draw) (menuframework_s *screen, menuvec2_t offset), const char *(*key) (menuframework_s *screen, int k), menuframework_s *screen)
{
	int			i, insertion_point;
	qboolean	found = false;
	
	if (Cvar_VariableValue ("maxclients") == 1
		&& Com_ServerState ())
		Cvar_Set ("paused", "1");
	
	screen->navagable = true;
	Menu_AutoArrange (screen);
	
	for (i = 0; i < mstate.active.num_layers; i++)
	{
		if (activelayer(i).screen == screen)
		{
			found = true;
			break;
		}
	}
	
	if (found)
	{
		insertion_point = i;
		mstate.state = mstate_remove;
	}
	else
	{
		mstate.incoming.num_layers++;
		layergroup_last(mstate.incoming).draw = draw;
		layergroup_last(mstate.incoming).key = key;
		layergroup_last(mstate.incoming).screen = screen;
		mstate.state = mstate_insert;
		insertion_point = cursor.menulayer;
		S_StartLocalSound ("doors/dr1_end.wav");
	}
	
	for (i = insertion_point+1; i < mstate.active.num_layers; i++)
		mstate.outgoing.layers[mstate.outgoing.num_layers++] = activelayer(i);
	mstate.active.num_layers = insertion_point+1;
	
	cls.key_dest = key_menu;
	
	m_entersound = true;
}

void M_ForceMenuOff (void)
{
	if (cls.state == ca_active && cls.key_dest == key_menu)
	{
		//-JD kill the music when leaving the menu of course
		S_StopAllSounds ();
		background_music = Cvar_Get ("background_music", "1", CVAR_ARCHIVE);
		S_StartMapMusic ();
	}

	cls.key_dest = key_game;
	Key_ClearStates ();
	Cvar_Set ("paused", "0");

	if (cls.state == ca_active)
		mstate_reset ();
}

static void M_PopMenu (void)
{
	S_StartLocalSound( menu_out_sound );
	if (mstate.active.num_layers == 0)
	{
		M_ForceMenuOff ();
		return;
	}
			
	mstate.outgoing.layers[mstate.outgoing.num_layers++] = 
		activelayer(--mstate.active.num_layers);
	mstate.state = mstate_remove;
}


static const char *Default_MenuKey (UNUSED menuframework_s *m, int key)
{
	const char *sound = NULL;
	
	// this should work no matter what
	if (key == K_ESCAPE)
	{
		M_PopMenu();
		return menu_out_sound;
	}

	// the rest of these won't work unless there's a selected menu item
	if (cursor.menuitem == NULL)
		return NULL;
	
	// offer the keypress to the field key parser, see if it wants it
	if (Field_Key (key))
	{
		Menu_ActivateItem (cursor.menuitem);
		return NULL;
	}
	
	switch ( key )
	{
	case K_MWHEELUP:
	case K_KP_UPARROW:
	case K_UPARROW:
		Menu_AdvanceCursor (-1, false);
		break;
	case K_TAB:
		Menu_AdvanceCursor (1, true);
		break;
	case K_MWHEELDOWN:
	case K_KP_DOWNARROW:
	case K_DOWNARROW:
		Menu_AdvanceCursor (1, false);
		break;
	case K_KP_LEFTARROW:
	case K_LEFTARROW:
		Menu_SlideItem (-1);
		sound = menu_move_sound;
		break;
	case K_KP_RIGHTARROW:
	case K_RIGHTARROW:
		Menu_SlideItem (1);
		sound = menu_move_sound;
		break;
	case K_KP_ENTER:
	case K_ENTER:
		Menu_ActivateItem (cursor.menuitem);
		sound = menu_move_sound;
		break;
	}

	return sound;
}

/*
=======================================================================

MAIN MENU

=======================================================================
*/

//#define NEW_SINGLEPLAYER

char *main_names[] =
{
	"m_main_game",
#ifdef NEW_SINGLEPLAYER
	"m_main_game",
#endif
	"m_main_join",
	"m_main_host",
	"m_main_options",
	"m_main_quit",
	"m_main_credits",
};
#define MAIN_ITEMS static_array_size(main_names)

void (*main_open_funcs[MAIN_ITEMS])(void) = 
{
	&M_Menu_Game_f,
#ifdef NEW_SINGLEPLAYER
	&M_Menu_Game_prototype_f,
#endif
	&M_Menu_JoinServer_f,
	&M_Menu_StartServer_f,
	&M_Menu_Options_f,
	&M_Menu_Quit_f,
	&M_Menu_Credits_f
};

static void findMenuCoords (int *xoffset, int *ystart, int *totalheight, int *widest)
{
	int w, h;
	unsigned int i;
	float scale;

	scale = (float)(viddef.height)/600;

	*totalheight = 0;
	*widest = -1;

	for (i = 0; i < MAIN_ITEMS; i++)
	{
		Draw_GetPicSize( &w, &h, main_names[i] );

		if ( w*scale > *widest )
			*widest = w*scale;
		*totalheight += ( h*scale + 24*scale);
	}

	*ystart = ( viddef.height / 2 - 20*scale );
	*xoffset = ( viddef.width - *widest + 350*scale) / 2;
}

static const char *news[] =
{
	"+Alien Arena News Feed",
	"",
	"+LATEST AA NEWS!",
	//"This is a test.",
	//"Real news coming soon!",
	0
};

static char newsFeed[256][256];

static FILE* newsfile_open( const char* mode )
{
	FILE* file;
	char pathbfr[MAX_OSPATH];

#if defined WIN32_VARIANT
	char *appData = getenv("AppData");
	Com_sprintf (pathbfr, sizeof(pathbfr)-1, "%s/AAWoM/%s", appData, "newsfeed.db");
#else
	Com_sprintf (pathbfr, sizeof(pathbfr)-1, "%s/%s", FS_Gamedir(), "newsfeed.db");
#endif

	file = fopen( pathbfr, mode );

	return file;
}

static size_t write_data(const void *buffer, size_t size, size_t nmemb, void *userp)
{
	FILE* file;
	size_t bytecount = 0;

	file = newsfile_open( "a" ); //append, don't rewrite

	if(file) {
		//write buffer to file
		bytecount = fwrite( buffer, size, nmemb, file );
		fclose(file);
	}
	return bytecount;
}

void GetNews()
{
	FILE* file;
	char newsserver[128];
	char line[256];
	int i = 0;

	CURL* easyhandle = curl_easy_init();

	file = newsfile_open("w"); //create new, blank file for writing
	if(file)
		fclose(file);

	Com_sprintf(newsserver, sizeof(newsserver), NEWSFEED_URL);

	curl_easy_setopt(easyhandle, CURLOPT_URL, newsserver);

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

	// parse the file and build string array
	file = newsfile_open("r") ;

	if(file != NULL) 
	{
		while(fgets(line, sizeof(line), file) != NULL)
		{
			strcpy(newsFeed[i], line);
			i++;
		}
		fclose(file);
	}
}

static void M_Main_Draw (menuvec2_t offset)
{
	unsigned int i;
	int ystart, xstart, xend;
	int	xoffset;
	int widest = -1;
	int totalheight = 0;
	char litname[80];
	float scale, hscale, hscaleoffs;
	float widscale;
	int w, h;
	int y;
	char montagepicname[16];
	char backgroundpic[16];
	char *version_warning;	
	FNT_font_t		font;
	struct FNT_window_s	box;
	
	static float mainalpha;
	static int montagepic = 1;

	scale = ((float)(viddef.height))/600.0;

	widscale = ((float)(viddef.width))/1024.0;

	findMenuCoords(&xoffset, &ystart, &totalheight, &widest);

	ystart = ( viddef.height / 2 - 20*scale ) + offset.y;
	xoffset = ( viddef.width - widest - 35*widscale) / 2 + offset.x;
	
	// When animating a transition away from the main menu, the background 
	// slides away at double speed, disappearing and leaving just the menu 
	// items themselves. Hence some things use offset.x*1.25.

	Draw_StretchPic(offset.x*1.25, offset.y, viddef.width, viddef.height, "m_main");

	//draw the montage pics
	mainalpha += cls.frametime; //fade image in
	if(mainalpha > 4) 
	{		
		//switch pics at this point
		mainalpha = 0.1;
		montagepic++;
		if(montagepic > 5) 
		{
			montagepic = 1;
		}
	}
	sprintf(backgroundpic, "m_main_mont%i", (montagepic==1)?5:montagepic-1);
	sprintf(montagepicname, "m_main_mont%i", montagepic);
	Draw_StretchPic (offset.x*1.25, offset.y, viddef.width, viddef.height, backgroundpic);
	Draw_AlphaStretchPic (offset.x*1.25, offset.y, viddef.width, viddef.height, montagepicname, mainalpha);


	/* check for more recent program version */
	version_warning = CL_VersionUpdateNotice();
	if ( version_warning != NULL )
	{
		const float warning_color[4] = {1, 1, 0, 1};
		Menu_DrawString (
			viddef.width - offset.x, offset.y + 5*scale,
			version_warning, FNT_CMODE_QUAKE_SRS, FNT_ALIGN_RIGHT, warning_color
		);
	}
	
	//draw the main menu buttons
	for (i = 0; i < MAIN_ITEMS; i++)
	{
		strcpy( litname, main_names[i] );
		if (i == m_main_cursor && cursor.menulayer == -1)
			strcat( litname, "_sel");
		Draw_GetPicSize( &w, &h, litname );
		xstart = xoffset + 100*widscale + (20*i*widscale);
		if (xstart < 0)
			xstart += min(-xstart, (8-i)*20*widscale);
		xend = xstart+w*widscale;
		hscale = 1;
		if (xstart < 0)
		{
			if (xend < 150*widscale)
				xend = min (viddef.width+offset.x, 150*widscale);
			xstart = 0;
			if (xend < 50*widscale)
				return;
		}
		hscale = (float)(xend-xstart)/(float)(w*widscale);
		hscaleoffs = (float)h*scale-(float)h*hscale*scale;
		Draw_StretchPic( xstart, (int)(ystart + i * 32.5*scale + 13*scale + hscaleoffs), xend-xstart, h*hscale*scale, litname );
	}

	//draw the news feed - this is temp, test code.  Actual news will be dynamically read in from http	
	font = FNT_AutoGet( CL_menuFont );
	scale = font->size / 8.0;

	for ( i = 0, y = viddef.height/8.0 - ( ( cls.realtime - news_start_time ) / 40.0F ); news[i]; y += 12*scale, i++ )
	{
		if ( y <= 12*scale || y > 48*scale)
			continue;
		
		box.y = offset.y + y;
		box.x = offset.x;
		box.height = 0;
		box.width = viddef.width/4.0;

		if ( news[i][0] == '+' )
		{
			FNT_BoundedPrint (font, news[i]+1, FNT_CMODE_NONE, FNT_ALIGN_CENTER, &box, FNT_colors[3]);
		}
		else
		{
			FNT_BoundedPrint (font, news[i], FNT_CMODE_NONE, FNT_ALIGN_CENTER, &box, FNT_colors[7]);
		}
	}

	//add in dynamicly read news portions here.
	for ( i = 0; strlen(newsFeed[i]) > 4; y += 12*scale, i++ )
	{
		if ( y <= 12*scale || y > 48*scale)
			continue;

		box.y = offset.y + y;
		box.x = offset.x;
		box.height = 0;
		box.width = viddef.width/4.0;

		if ( newsFeed[i][0] == '+' )
		{
			FNT_BoundedPrint (font, newsFeed[i]+1, FNT_CMODE_NONE, FNT_ALIGN_CENTER, &box, FNT_colors[3]);
		}
		else
		{
			FNT_BoundedPrint (font, newsFeed[i], FNT_CMODE_NONE, FNT_ALIGN_CENTER, &box, FNT_colors[7]);
		}
	}

	if ( y <= 12*scale )
		news_start_time = cls.realtime;
}

void CheckMainMenuMouse (void)
{
	int ystart;
	int	xoffset;
	int widest;
	int totalheight;
	int i, oldhover;
	float scale;
	static int MainMenuMouseHover;

	scale = (float)(viddef.height)/600;

	oldhover = MainMenuMouseHover;
	MainMenuMouseHover = 0;

	findMenuCoords(&xoffset, &ystart, &totalheight, &widest);

	i = (cursor.y - ystart - 24*scale)/(32*scale);
	if (i < 0 || (unsigned)i >= MAIN_ITEMS)
	{
		if (cursor.buttonclicks[MOUSEBUTTON1]==1)
		{
			cursor.buttonused[MOUSEBUTTON1] = true;
			cursor.buttonclicks[MOUSEBUTTON1] = 0;
		}
		return;
	}

	if (cursor.mouseaction)
	{
		refreshCursorLink ();
		m_main_cursor = i;
		cursor.mouseaction = false;
	}

	MainMenuMouseHover = 1 + i;

	if (oldhover == MainMenuMouseHover && MainMenuMouseHover-1 == m_main_cursor &&
		!cursor.buttonused[MOUSEBUTTON1] && cursor.buttonclicks[MOUSEBUTTON1]==1)
	{
		main_open_funcs[m_main_cursor]();
		S_StartLocalSound( menu_move_sound );
		cursor.buttonused[MOUSEBUTTON1] = true;
		cursor.buttonclicks[MOUSEBUTTON1] = 0;
	}
}

static const char *M_Main_Key (int key)
{
	switch (key)
	{
	case K_ESCAPE:
		m_entersound = true;
		M_PopMenu ();
		break;

	case K_KP_DOWNARROW:
	case K_DOWNARROW:
	case K_TAB:
	case K_MWHEELDOWN:
		if ((unsigned)(++m_main_cursor) >= MAIN_ITEMS)
			m_main_cursor = 0;
		break;

	case K_KP_UPARROW:
	case K_UPARROW:
	case K_MWHEELUP:
		if (--m_main_cursor < 0)
			m_main_cursor = MAIN_ITEMS - 1;
		break;

	case K_KP_ENTER:
	case K_ENTER:
		m_entersound = true;
		main_open_funcs[m_main_cursor]();
		break;
	}

	return NULL;
}


void M_Menu_Main_f (void)
{
	static qboolean latestGameVersionRetrieved = false;

	S_StartMenuMusic();
	GetNews();
	if (!latestGameVersionRetrieved) {
		CL_GetLatestGameVersion();
		latestGameVersionRetrieved = true;
	}
	cls.key_dest = key_menu;
	if (cls.state == ca_active)
		mstate_reset ();
}

/*
=======================================================================

OPTIONS MENUS - INPUT MENU

=======================================================================
*/
char *bindnames[][2] =
{
{"+attack", 		"primary attack"},
{"+attack2",		"secondary attack"},
{"weapnext", 		"next weapon"},
{"weapprev", 		"previous weapon"},

{"+forward", 		"walk forward"},
{"+back", 			"walk back"},
{"+moveleft", 		"step left"},
{"+moveright", 		"step right"},
{"+moveup",			"up / jump"},
{"+movedown",		"down / crouch"},
{"+speed", 			"running speed select"},
{"+left",			"turn left (keyboard)"},
{"+right",			"turn right (keyboard)"},

{"+leanright",		"lean right"},
{"+leanleft",		"lean left"},
{"+sneak",			"sneak"},
{"+zoom",			"zoom"},

{"inven",			"inventory"},
{"invuse",			"use item"},
{"invdrop",			"drop item"},
{"invprev",			"prev item"},
{"invnext",			"next item"},

{"use Blaster",			"switch to Blaster"},
{"use Alien Disruptor",	"switch to Alien Disruptor"},
{"use Chaingun",		"switch to Chaingun"},
{"use Flame Thrower",	"switch to Flame Thrower"},
{"use Rocket Launcher",	"switch to Rocket Launcher"},
{"use Alien Smartgun",	"switch to Alien Smartgun"},
{"use Disruptor",		"switch to Alien Beamgun"},
{"use Alien Vaporizer", "switch to Alien Vaporizer"},
{"use Violator",		"switch to Violator"},
{"use Minderaser",		"switch to Mind Eraser"},
{"use grapple",			"switch to Grapple Hook"},

{"use sproing",			"buy Sproing"},
{"use haste",			"buy Haste"},
{"use invisibility",	"buy Invisibility"},

{"score",				"show scores"},
{"vtaunt 1",			"voice taunt #1"},
{"vtaunt 2",			"voice taunt #2"},
{"vtaunt 3",			"voice taunt #3"},
{"vtaunt 4",			"voice taunt #4"},
{"vtaunt 5",			"voice taunt #5"},
{"vtaunt 0",			"voice taunt auto"}
};

#define num_bindable_actions static_array_size(bindnames)

static int		bind_grab;

static menuframework_s	s_keys_screen;
static menuframework_s	s_keys_menu;
static menuaction_s		s_keys_actions[num_bindable_actions];

static void M_UnbindCommand (const char *command)
{
	int		j;
	int		l;
	char	*b;

	l = strlen(command);

	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j];
		if (!b)

			continue;
		if (!strncmp (b, command, l) )
			Key_SetBinding (j, "");
	}
}

static void M_FindKeysForCommand (const char *command, int *twokeys)
{
	int		count;
	int		j;
	char	*b;

	twokeys[0] = twokeys[1] = -1;
	count = 0;
	
	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!Q_strcasecmp (b, command) )
		{
			twokeys[count] = j;
			count++;
			if (count == 2)
				break;
		}
	}
}

static void M_KeyBindingDisplayStr (const char *command, size_t bufsize, char *buf)
{
	int keys[2];
	
	M_FindKeysForCommand (command, keys);
	
	if (keys[0] == -1)
	{
		Com_sprintf (buf, bufsize, "???");
	}
	else
	{
		// Key_KeynumToString reuses the same buffer for output sometimes
		int len;
		Com_sprintf (buf, bufsize, "%s", Key_KeynumToString (keys[0]));
		len = strlen(buf);
		if (keys[1] != -1)
			Com_sprintf (buf+len, bufsize-len, "  or  %s", Key_KeynumToString (keys[1]));
	}
}

static menuvec2_t KeySizeFunc (void *_self, FNT_font_t font)
{
	menuvec2_t ret;
	char buf[1024];
	menuaction_s *self = ( menuaction_s * ) _self;
	
	M_KeyBindingDisplayStr (self->generic.localstrings[0], sizeof(buf), buf);
	
	ret.y = font->height;
	ret.x = RCOLUMN_OFFSET + Menu_PredictSize (buf);
	
	return ret;
}

static void DrawKeyBindingFunc( void *_self, FNT_font_t font )
{
	extern const float light_color[4];
	char buf[1024];
	menuaction_s *self = ( menuaction_s * ) _self;

	M_KeyBindingDisplayStr (self->generic.localstrings[0], sizeof(buf), buf);
	
	Menu_DrawString (
		Item_GetX (*self) + RCOLUMN_OFFSET,
		Item_GetY (*self) + MenuText_UpperMargin (self, font->size),
		buf, FNT_CMODE_QUAKE_SRS, FNT_ALIGN_LEFT, light_color
	);
}

static void KeyBindingFunc( void *_self )
{
	menuaction_s *self = ( menuaction_s * ) _self;
	int keys[2];

	M_FindKeysForCommand( self->generic.localstrings[0], keys );

	if (keys[1] != -1)
		M_UnbindCommand( self->generic.localstrings[0]);

	bind_grab = true;

	Menu_SetStatusBar( &s_keys_menu, "press a key or button for this action" );
}

static void Keys_MenuInit( void )
{
	unsigned int i;

	setup_window (s_keys_screen, s_keys_menu, "CUSTOMIZE CONTROLS");
	
	for (i = 0; i < num_bindable_actions; i++)
	{
		s_keys_actions[i].generic.type				= MTYPE_ACTION;
		s_keys_actions[i].generic.callback			= KeyBindingFunc;
		s_keys_actions[i].generic.itemdraw			= DrawKeyBindingFunc;
		s_keys_actions[i].generic.localstrings[0]	= bindnames[i][0];
		s_keys_actions[i].generic.name				= bindnames[i][1];
		s_keys_actions[i].generic.itemsizecallback	= KeySizeFunc;
		Menu_AddItem( &s_keys_menu, &s_keys_actions[i]);
	}

	Menu_SetStatusBar( &s_keys_menu, "enter to change, backspace to clear" );
	
	s_keys_menu.maxlines = 30;
}

static const char *Keys_MenuKey (menuframework_s *screen, int key)
{
	menuaction_s *item = ( menuaction_s * ) cursor.menuitem;

	if ( bind_grab && !(cursor.buttonused[MOUSEBUTTON1]&&key==K_MOUSE1))
	{
		if ( key != K_ESCAPE && key != '`' )
		{
			char cmd[1024];

			Com_sprintf (cmd, sizeof(cmd), "bind \"%s\" \"%s\"\n", Key_KeynumToString(key), item->generic.localstrings[0]);
			Cbuf_InsertText (cmd);
		}

		//dont let selecting with mouse buttons screw everything up
		refreshAllCursorButtons();
		if (key==K_MOUSE1)
			cursor.buttonclicks[MOUSEBUTTON1] = -1;

		Menu_SetStatusBar (&s_keys_menu, "enter to change, backspace to clear");
		bind_grab = false;
		
		Menu_AutoArrange (screen);
		
		return menu_out_sound;
	}

	switch ( key )
	{
	case K_BACKSPACE:		// delete bindings
	case K_DEL:				// delete bindings
	case K_KP_DEL:
		M_UnbindCommand( item->generic.localstrings[0] );
		return menu_out_sound;
	default:
		return Default_MenuKey (screen, key);
	}
}

void M_Menu_Keys_f (void)
{
	Keys_MenuInit();
	M_PushMenu (Screen_Draw, Keys_MenuKey, &s_keys_screen);
}

/*
=======================================================================

OPTIONS MENUS - GENERIC CODE FOR OPTIONS WIDGETS

=======================================================================
*/

typedef struct
{
	int maxchars;
	int	min_value, max_value; // for numerical fields, otherwise ignore
} fieldsize_t;

typedef struct
{
	int slider_min, slider_max;
	float cvar_min, cvar_max;
} sliderlimit_t;

typedef struct 
{
	enum 
	{
		option_slider,
		option_textcvarslider,
		option_spincontrol,
		option_textcvarspincontrol,
		option_textcvarpicspincontrol,
		option_hudspincontrol,
		option_minimapspincontrol,
		option_numberfield
	} type;
	const char *cvarname;
	const char *displayname;
	const char *tooltip;
	
	// extra data - set the appropriate one
	const char 			**names;
	const sliderlimit_t *limits;
	const fieldsize_t	*fieldsize;
	#define setnames(x)		(const char **)(x), NULL, NULL
	#define setlimits(x)	NULL, &(x), NULL
	#define setfieldsize(x)	NULL, NULL, &(x)
	
	// flags - optional, defaults to no flags
	int flags;
	
} option_name_t;

static void SpinOptionFunc (void *_self)
{
	menulist_s *self;
	const char *cvarname;
	
	self = (menulist_s *)_self;
	cvarname = self->generic.localstrings[0];
	
	Cvar_SetValue( cvarname, self->curvalue );

	if (cvarname == "vid_fullscreen" && 
		(self->curvalue == windowmode_borderless_windowed || self->curvalue == windowmode_exclusive_fullscreen))
	{
		// Store preferred full screen mode for toggling between windowed and fullscreen with alt-enter
		Cvar_SetValue( "vid_preferred_fullscreen", self->curvalue );
	}
}

static menuvec2_t FontSelectorSizeFunc (void *_self, UNUSED FNT_font_t unused)
{
	menuvec2_t ret;
	menulist_s *self;
	FNT_font_t font;
	
	self = (menulist_s *)_self;
	font = FNT_AutoGet (*(FNT_auto_t *)self->generic.localptrs[0]);
	
	ret.y = font->height;
	ret.x = RCOLUMN_OFFSET + FNT_PredictSize (font, self->itemnames[self->curvalue], false);
	
	return ret;
}

static void FontSelectorDrawFunc (void *_self, UNUSED FNT_font_t unused)
{
	extern const float light_color[4];
	menulist_s *self;
	FNT_font_t font;
	
	self = (menulist_s *)_self;
	font = FNT_AutoGet (*(FNT_auto_t *)self->generic.localptrs[0]);
	
	menu_box.x = Item_GetX (*self)+RCOLUMN_OFFSET;
	menu_box.y = Item_GetY (*self) + MenuText_UpperMargin (self, font->size);
	menu_box.height = menu_box.width = 0;
	
	FNT_BoundedPrint (font, self->itemnames[self->curvalue], FNT_CMODE_QUAKE_SRS, FNT_ALIGN_LEFT, &menu_box, light_color);
}

static void TextVarSpinOptionUpdateLabelFunc (void *_self)
{
	menulist_s *self = (menulist_s *)_self;
	
	Com_sprintf (self->buffer, sizeof (self->buffer), "%s", self->itemnames[self->curvalue]);
}

static void TextVarSpinOptionFunc (void *_self)
{
	menulist_s *self;
	const char *cvarname;
	char *cvarval;
	
	self = (menulist_s *)_self;
	cvarname = self->generic.localstrings[0];
	
	cvarval = strchr(self->itemnames[self->curvalue], '\0')+1;
	Cvar_Set (cvarname, cvarval);
	
	TextVarSpinOptionUpdateLabelFunc (_self);
}

static void UpdateDopplerEffectFunc( void *self )
{
	TextVarSpinOptionFunc (self);
	R_EndFrame(); // buffer swap needed to show text box
	S_UpdateDopplerFactor();
}

static float TextSliderValueSizeFunc (void *_self)
{
	menulist_s *self;
	int i;
	float ret = 0.0f;
	
	self = (menulist_s *)_self;
	
	for (i = self->minvalue; i <= self->maxvalue; i++)
	{
		float cursize;
		cursize = Menu_PredictSize (self->itemnames[i]);
		if (cursize > ret)
			ret = cursize;
	}
	
	return ret;
}

static float SliderOptionGetVal (const menuslider_s *self)
{
	float sliderval, valscale;
	const sliderlimit_t *limit;
	
	limit = (const sliderlimit_t *) self->generic.localptrs[0];
	
	sliderval = self->curvalue;
	
	valscale = 	(limit->cvar_max-limit->cvar_min)/
				(float)(limit->slider_max-limit->slider_min);

	return limit->cvar_min + valscale*(sliderval-limit->slider_min);
}

static void SliderOptionUpdateLabelFunc (void *_self)
{
	menuslider_s *self;
	float cvarval;
	
	self = (menuslider_s *)_self;
	
	cvarval = SliderOptionGetVal (self);
	Com_sprintf (self->buffer, sizeof (self->buffer), "%f", cvarval);
	cleanup_float_string (self->buffer);
}

static void SliderOptionFunc (void *_self)
{
	menuslider_s *self;
	const char *cvarname;
	float cvarval;
	
	self = (menuslider_s *)_self;
	cvarname = self->generic.localstrings[0];
	
	cvarval = SliderOptionGetVal (self);
	Cvar_SetValue (cvarname, cvarval);
	
	SliderOptionUpdateLabelFunc (_self);
}

static float NumericalSliderValueSizeFunc (void *_self)
{
	menuslider_s *self;
	float valscale, cvarval;
	const sliderlimit_t *limit;
	int i;
	char buffer[80];
	float ret = 0.0f;
	
	self = (menuslider_s *)_self;
	
	limit = (const sliderlimit_t *) self->generic.localptrs[0];
	
	valscale = 	(limit->cvar_max-limit->cvar_min)/
				(float)(limit->slider_max-limit->slider_min);
	
	for (i = self->minvalue; i <= self->maxvalue; i++)
	{
		float cursize;
		cvarval = (float)(i - self->minvalue) * valscale;
		Com_sprintf (buffer, sizeof(buffer), "%f", cvarval);
		cleanup_float_string (buffer);
		cursize = Menu_PredictSize (buffer);
		if (cursize > ret)
			ret = cursize;
	}
	
	return ret;
}

static void NumberFieldOptionFunc (void *_self)
{
	menufield_s *self;
	int num, clamped_num;
	const fieldsize_t *fieldsize;
	const char *cvarname;
	int resolution_custom = -1;
	
	self = (menufield_s *)_self;
	cvarname = self->generic.localstrings[0];
	
	fieldsize = (const fieldsize_t *)self->generic.localptrs[0];
	
	num = atoi (self->buffer);
	clamped_num = clamp (num, fieldsize->min_value, fieldsize->max_value);
	
	Com_sprintf (self->buffer, sizeof(self->buffer), "%d", num);
	self->cursor = clamp (self->cursor, 0, strlen (self->buffer));
	if (num != clamped_num)
		Com_sprintf (self->buffer, sizeof(self->buffer), "%d  ^1(%d)", num, clamped_num);
	
	if ((cvarname == "vid_width" || cvarname == "vid_height") &&
		clamped_num != Cvar_Get(cvarname, "0", CVAR_ARCHIVE)->integer)
	{
		Cvar_SetValue ("gl_mode", resolution_custom);
	}
	
	Cvar_SetValue (cvarname, clamped_num);
}

// HACKS for specific menus
extern cvar_t *crosshair;
#define MAX_CROSSHAIRS 256
char *crosshair_names[MAX_CROSSHAIRS];
int	numcrosshairs = 0;

static void HudFunc( void *item );
static void MinimapFunc( void *item );
static void UpdateBGMusicFunc( void *_self );

static float ClampCvar( float min, float max, float value );

extern cvar_t *r_minimap;
extern cvar_t *r_minimap_style;

#define MAX_FONTS 32
char *font_names[MAX_FONTS];
int	numfonts = 0;

// name and value lists: for spin-controls where the cvar value isn't simply
// the integer index of whatever text is displaying in the control. We use
// NULL terminators to separate display names and variable values, so that way
// we can use the existing menu code.

static const char *doppler_effect_items[] =
{
	"off\0000",
	"normal\0001",
	"high\0003",
	"very high\0005",
	0
};

int	numhuds = 0;
extern cvar_t *cl_hudimage1;
extern cvar_t *cl_hudimage2;
extern cvar_t *cl_hudimage3;
#define MAX_HUDS 256
char *hud_names[MAX_HUDS];

// initialize a menu item as an "option."
static void Option_Setup (menumultival_s *item, option_name_t *optionname)
{
	int val, maxval;
	char *vartextval;
	int i;
	float cvarval, sliderval, valscale;
	const sliderlimit_t *limit;
	const fieldsize_t *fieldsize;
	
	// Do not re-allocate font/crosshair/HUD names each time the menu is
	// displayed - BlackIce
	if ( numfonts == 0 )
		SetFontNames (font_names);
	
	if ( numhuds == 0 )
		SetHudNames (hud_names);
	
	if ( numcrosshairs == 0 )
		SetCrosshairNames (crosshair_names);
	
	// initialize item
	
	item->generic.name = optionname->displayname;
	item->generic.tooltip = optionname->tooltip;
	item->generic.localstrings[0] = optionname->cvarname;
	item->generic.flags = optionname->flags;
	item->generic.apply_pending = false;
	item->generic.waitcallback = NULL;
	
	switch (optionname->type)
	{
		case option_spincontrol:
			item->generic.type = MTYPE_SPINCONTROL;
			
			if (optionname->cvarname == "vid_fullscreen" &&
				Cvar_Get( "vid_fullscreen", "1", CVAR_ARCHIVE|CVARDOC_INT)->integer == windowmode_exclusive_fullscreen)
			{
				// Show fullscreen (legacy) as well
				// previously called fullscreen (exclusive)
				optionname->names = windowmode_names_legacy;
			}

			item->itemnames = optionname->names;

			if (!strcmp (optionname->cvarname, "background_music"))
				// FIXME HACK
				item->generic.callback = UpdateBGMusicFunc;
			else
				item->generic.callback = SpinOptionFunc;
			if (item->itemnames == onoff_names)
				setup_tickbox (*item);
			if (item->itemnames == offon_names)
			{
				setup_tickbox (*item); // because setup_tickbox overwrites itemnames
				item->itemnames = offon_names;
			}
			break;
		
		case option_textcvarslider:
			item->generic.type = MTYPE_SLIDER;
			item->itemnames = optionname->names; 
			item->minvalue = 0;
			for (item->maxvalue = 0; item->itemnames[item->maxvalue+1]; item->maxvalue++)
				continue;
			if (item->itemnames == doppler_effect_items)
				// FIXME HACK
				item->generic.callback = UpdateDopplerEffectFunc;
			else
				item->generic.callback = TextVarSpinOptionFunc;
			item->generic.waitcallback = TextVarSpinOptionUpdateLabelFunc;
			item->slidervaluesizecallback = TextSliderValueSizeFunc;
			break;
		
		case option_textcvarspincontrol:
		case option_textcvarpicspincontrol:
			item->generic.type = MTYPE_SPINCONTROL;
			item->itemnames = optionname->names;
			item->generic.callback = TextVarSpinOptionFunc;
			// FIXME HACK
			if (item->itemnames == font_names)
			{
				item->generic.itemsizecallback = FontSelectorSizeFunc;
				item->generic.itemdraw = FontSelectorDrawFunc;
				if (!strcmp (optionname->cvarname, "fnt_game"))
					item->generic.localptrs[0] = &CL_gameFont;
				else if (!strcmp (optionname->cvarname, "fnt_console"))
					item->generic.localptrs[0] = &CL_consoleFont;
				else if (!strcmp (optionname->cvarname, "fnt_menu"))
					item->generic.localptrs[0] = &CL_menuFont;
			}
			else if (optionname->type == option_textcvarpicspincontrol)
			{
				item->generic.itemsizecallback = PicSpinSizeFunc;
				item->generic.itemdraw = PicSpinDrawFunc;
				VectorSet (item->generic.localints, 5, 5, RCOLUMN_OFFSET);
			}
			break;
		
		case option_hudspincontrol:
			item->generic.type = MTYPE_SPINCONTROL;
			item->itemnames = optionname->names;
			item->generic.callback = HudFunc;
			break;
		
		case option_minimapspincontrol:
			item->generic.type = MTYPE_SPINCONTROL;
			item->itemnames = optionname->names;
			item->generic.callback = MinimapFunc;
			break;
		
		case option_slider:
			limit = optionname->limits;
			item->generic.type = MTYPE_SLIDER;
			item->minvalue = limit->slider_min;
			item->maxvalue = limit->slider_max;
			item->generic.callback = SliderOptionFunc;
			item->generic.waitcallback = SliderOptionUpdateLabelFunc;
			item->generic.localptrs[0] = limit;
			item->slidervaluesizecallback = NumericalSliderValueSizeFunc;
			break;
		
		case option_numberfield:
			fieldsize = optionname->fieldsize;
			item->generic.type = MTYPE_FIELD;
			item->generic.flags |= QMF_NUMBERSONLY;
			item->generic.visible_length = 2*fieldsize->maxchars;
			item->cursor = 0;
			memset (item->buffer, 0, sizeof(item->buffer));
			item->generic.callback = NumberFieldOptionFunc;
			item->generic.localptrs[0] = fieldsize;
			break;
	}
	
	// initialize value
	
	switch (optionname->type)
	{
		case option_spincontrol:
			for (maxval = 0; item->itemnames[maxval]; maxval++) 
				continue;
			maxval--;
		
			val = ClampCvar (0, maxval, Cvar_VariableValue (optionname->cvarname));
		
			item->curvalue = val;
			Cvar_SetValue (optionname->cvarname, val);
			break;
		
		case option_hudspincontrol:
		case option_textcvarspincontrol:
		case option_textcvarpicspincontrol:
		case option_textcvarslider:
			item->curvalue = 0;
			vartextval = Cvar_VariableString (optionname->cvarname);
			
			for (i=0; item->itemnames[i]; i++)
			{
				char *corresponding_cvar_val = strchr(item->itemnames[i], '\0')+1;
				if (!Q_strcasecmp(vartextval, corresponding_cvar_val))
				{
					item->curvalue = i;
					break;
				}
			}
			Com_sprintf (item->buffer, sizeof(item->buffer), "%s", item->itemnames[item->curvalue]);
			break;
		
		case option_minimapspincontrol:
			Cvar_SetValue("r_minimap_style", ClampCvar(0, 1, r_minimap_style->value));
			Cvar_SetValue("r_minimap", ClampCvar(0, 1, r_minimap->value));
			if(r_minimap_style->value == 0)
				item->curvalue = 2;
			else
				item->curvalue = r_minimap->value;
			break;
		
		case option_slider:
			limit = optionname->limits;
		
			cvarval = ClampCvar (	limit->cvar_min, limit->cvar_max,
									Cvar_VariableValue (optionname->cvarname));
			Cvar_SetValue (optionname->cvarname, cvarval);
		
			valscale = 	(float)(limit->slider_max-limit->slider_min)/
						(limit->cvar_max-limit->cvar_min);
			sliderval = limit->slider_min + valscale*(cvarval-limit->cvar_min);
			item->curvalue = sliderval;
			Com_sprintf (item->buffer, sizeof(item->buffer), "%f", cvarval);
			cleanup_float_string (item->buffer);
			break;
		
		case option_numberfield:
			fieldsize = optionname->fieldsize;
			Com_sprintf (item->buffer, sizeof(item->buffer), "%d", (int)Cvar_VariableValue (optionname->cvarname));
			item->cursor = strlen (item->buffer);
			break;
	}
}

// all "options" menus have roughly the same layout, so we can automate some
// of the grunt work

typedef struct
{
	menuframework_s	screen;
	menuframework_s	window;
	menuframework_s	panel;
	menumultival_s	widgets[];
} options_menu_t;

static options_menu_t	*last_options_menu;
static option_name_t	*last_options_menu_namelist;
static int				last_options_menu_nitems;

// Use this anywhere to reinitialize whatever options menu is currently 
// showing so it reflects current cvar values.
static void Options_Menu_Reinitialize (void)
{
	int i;
	for (i = 0; i < last_options_menu_nitems; i++)
		Option_Setup (&last_options_menu->widgets[i], &last_options_menu_namelist[i]);
}

#define options_menu(menu,title) \
	static struct \
	{ \
		menuframework_s	screen; \
		menuframework_s	window; \
		menuframework_s	panel; \
		menumultival_s	widgets[static_array_size(menu ## _option_names)]; \
	} menu; \
	\
	setup_window (menu.screen, menu.window, title); \
	setup_panel (menu.window, menu.panel); \
	\
	{ \
	\
		int i; \
		for (i = 0; i < static_array_size(menu ## _option_names); i++) \
		{ \
			Option_Setup (&menu.widgets[i], &(menu ## _option_names)[i]); \
			Menu_AddItem( &menu.panel, &menu.widgets[i]); \
		} \
	} \
	last_options_menu_nitems = static_array_size(menu ## _option_names); \
	last_options_menu_namelist = &(menu ## _option_names)[0]; \
	last_options_menu = (options_menu_t*)&menu;
	


/*
=======================================================================

OPTIONS MENUS - DISPLAY OPTIONS MENU

=======================================================================
*/

static void MinimapFunc( void *item )
{
	menulist_s *self = (menulist_s *)item;
	Cvar_SetValue("r_minimap", self->curvalue != 0);
	if(r_minimap->integer) {
		Cvar_SetValue("r_minimap_style", self->curvalue % 2);
	}
}

static float ClampCvar( float min, float max, float value )
{
	if ( value < min ) return min;
	if ( value > max ) return max;
	return value;
}

static qboolean fontInList (char *check, int num, char **list)
{
	int i;
	for (i=0;i<num;i++)
		if (!Q_strcasecmp(check, list[i]))
			return true;
	return false;
}

// One string after another, both in the same memory block, but each with its
// own null terminator. 
static char *str_combine (char *in1, char *in2)
{
	size_t outsize;
	char *out;
	
	outsize = strlen(in1)+1+strlen(in2)+1;
	out = malloc (outsize);
	memset (out, 0, outsize);
	strcpy (out, in1);
	strcpy (strchr(out, '\0')+1, in2);
	
	return out;
}

static void insertFile (char ** list, char *insert1, char *insert2, int ndefaults, int len)
{
	int i, j;
	char *tmp;
	
	tmp = str_combine (insert1, insert2);

	for (i=ndefaults;i<len; i++)
	{
		if (!list[i])
			break;

		if (strcmp( list[i], insert1 ))
		{
			for (j=len; j>i ;j--)
				list[j] = list[j-1];

			list[i] = tmp;

			return;
		}
	}

	list[len] = tmp;
}

static void AddFontNames( char * path , int * nfontnames , char ** list )
{
	char ** fontfiles;
	int nfonts = 0;
	int i;

	fontfiles = FS_ListFilesInFS( path , &nfonts, 0,
		SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

	for (i=0;i<nfonts && *nfontnames<MAX_FONTS;i++)
	{
		int num;
		char * p;

		p = strstr(fontfiles[i], "fonts/"); p++;
		p = strstr(p, "/"); p++;

		num = strlen(p)-4;
		p[num] = 0;

		if (!fontInList(p, i, list))
		{
			insertFile (list, p, p, 1, i);
			(*nfontnames)++;
		}
	}

	if (fontfiles)
		FS_FreeFileList(fontfiles, nfonts);
}

void SetFontNames (char **list)
{
	int nfontnames;

	memset( list, 0, sizeof( char * ) * MAX_FONTS );

	nfontnames = 0;
	AddFontNames( "fonts/*.ttf" , &nfontnames , list );

	numfonts = nfontnames;
}

void SetCrosshairNames (char **list)
{
	char *curCrosshairFile;
	char *p;
	int ncrosshairs = 0, ncrosshairnames;
	char **crosshairfiles;
	int i;

	ncrosshairnames = 4;

	list[0] = str_combine("none", "none"); //the old crosshairs
	list[1] = str_combine("ch1", "ch1");
	list[2] = str_combine("ch2", "ch2");
	list[3] = str_combine("ch3", "ch3");

	crosshairfiles = FS_ListFilesInFS( "pics/crosshairs/*.tga",
		&ncrosshairs, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

	for (i=0;i<ncrosshairs && ncrosshairnames<MAX_CROSSHAIRS;i++)
	{
		int num;

		p = strstr(crosshairfiles[i], "/crosshairs/"); p++;
		curCrosshairFile = p;
		
		p = strstr(p, "/"); p++;

		num = strlen(p)-4;
		p[num] = 0;

		if (!fontInList(curCrosshairFile, ncrosshairnames, list))
		{
			// HACK
			insertFile (list,curCrosshairFile,curCrosshairFile,4,ncrosshairnames);
			ncrosshairnames++;
		}
	}

	if (crosshairfiles)
		FS_FreeFileList(crosshairfiles, ncrosshairs);

	numcrosshairs = ncrosshairnames;
}

static void HudFunc( void *item )
{
	menulist_s *self;
	char hud1[MAX_OSPATH];
	char hud2[MAX_OSPATH];
	char hud3[MAX_OSPATH];
	
	self = (menulist_s *)item;

	if(self->curvalue == 0) { //none
		sprintf(hud1, "none");
		sprintf(hud2, "none");
		sprintf(hud3, "none");
	}

	if(self->curvalue == 1) {
		sprintf(hud1, "pics/i_health.tga");
		sprintf(hud2, "pics/i_score.tga");
		sprintf(hud3, "pics/i_ammo.tga");
	}

	if(self->curvalue > 1) {
		sprintf(hud1, "pics/huds/%s1", hud_names[self->curvalue]);
		sprintf(hud2, "pics/huds/%s2", hud_names[self->curvalue]);
		sprintf(hud3, "pics/huds/%s3", hud_names[self->curvalue]);
	}

	//set the cvars, all of them
	Cvar_Set( "cl_hudimage1", hud1 );
	Cvar_Set( "cl_hudimage2", hud2 );
	Cvar_Set( "cl_hudimage3", hud3 );
}

void SetHudNames (char **list)
{
	char *curHud, *curHudFile;
	char *p;
	int nhuds = 0, nhudnames;
	char **hudfiles;
	int i;

	memset( list, 0, sizeof( char * ) * MAX_HUDS );

	nhudnames = 2;

	list[0] = str_combine("none", "none");
	list[1] = str_combine("default", "pics/i_health.tga"); //the default hud

	hudfiles = FS_ListFilesInFS( "pics/huds/*1.tga", &nhuds, 0,
		SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

	for (i=0;i<nhuds && nhudnames<MAX_HUDS;i++)
	{
		int num, file_ext;

		p = strstr(hudfiles[i], "/huds/"); p++;
		p = strstr(p, "/"); p++;

		num = strlen(p)-5;
		file_ext = num+1;
		p[file_ext] = 0;
		
		// we only need this because the second part of a text cvar value list
		// will be compared against cl_hudimage1 so it needs the 1 suffix.
		curHudFile = _strdup (hudfiles[i]);
		
		p[num] = 0;

		curHud = p;

		if (!fontInList(curHud, nhudnames, list))
		{
			insertFile (list,curHud,curHudFile,2,nhudnames);
			nhudnames++;
		}

		free (curHudFile);
	}

	if (hudfiles)
		FS_FreeFileList(hudfiles, nhuds);

	numhuds = nhudnames;
}

static void UpdateBGMusicFunc( void *_self )
{
	menulist_s *self = (menulist_s *)_self;
	Cvar_SetValue( "background_music", self->curvalue );
	if ( background_music->value > 0.99f && background_music_vol->value >= 0.1f )
	{
		S_StartMenuMusic();
	}
}

// name lists: list of strings terminated by 0

static const char *minimap_names[] =
{
	"off",
	"static",
	"rotating",
	0
};
static const char *playerid_names[] =
{
	"off",
	"centered",
	"over player",
	0
};

static const char *color_names[] =
{
	"^2green",
	"^4blue",
	"^1red",
	"^3yellow",
	"^6purple",
	0
};

static const char *console_color_names[] =
{
	"^7none",
	"^1red",
	"^2green",
	"^4blue",
	"^5cyan",
	"^6purple",
	0
};

static const char *handedness_names[] =
{
	"right",
	"left",
	"center",
	0
};

sliderlimit_t mousespeed_limits = 
{
	1, 111, 0.1f, 11.1f
};

fieldsize_t fov_limits = 
{
	3, 10, 130
};

option_name_t disp_option_names[] = 
{
	{
		option_spincontrol,
		"cl_precachecustom",
		"precache custom models",
		"Enabling this can result in slow map loading times",
		setnames (onoff_names)
	},
	{
		option_spincontrol,
		"cl_showplayernames",
		"identify target",
		NULL, 
		setnames (playerid_names)
	},
	{
		option_spincontrol,
		"r_ragdolls",
		"ragdolls",
		NULL,
		setnames (onoff_names)
	},
	{
		option_spincontrol,
		"cl_noblood",
		"no blood",
		NULL,
		setnames (onoff_names)
	},
	{
		option_spincontrol,
		"cl_noskins",
		"force martian models",
		NULL,
		setnames (onoff_names)
	},
	{
		option_textcvarspincontrol,
		"fnt_console",
		"console font",
		"select the font used to display the console",
		setnames (font_names)
	},
	{
		option_spincontrol,
		"con_color",
		"console color",
		"select console background color accent",
		setnames (console_color_names)
	},
	{
		option_textcvarspincontrol,
		"fnt_game",
		"game font",
		"select the font used in the game",
		setnames (font_names)
	},
	{
		option_textcvarspincontrol,
		"fnt_menu",
		"menu font",
		"select the font used in the menu",
		setnames (font_names)
	},
	{
		option_textcvarpicspincontrol,
		"crosshair",
		"crosshair",
		"select your crosshair",
		setnames (crosshair_names)
	},
	{
		option_hudspincontrol,
		"cl_hudimage1", //multiple cvars controlled-- see HudFunc
		"HUD",
		"select your HUD style",
		setnames (hud_names)
	},
	{
		option_spincontrol,
		"cl_disbeamclr",
		"disruptor color",
		"select disruptor beam color",
		setnames (color_names)
	},
	{
		option_minimapspincontrol,
		NULL, //multiple cvars controlled-- see MinimapFunc
		"minimap",
		"select your minimap style",
		setnames (minimap_names)
	},
	{
		option_spincontrol,
		"cl_drawfps",
		"display framerate",
		NULL,
		setnames (onoff_names)
	},
	{
		option_spincontrol,
		"cl_drawtimer",
		"display timer",
		NULL,
		setnames (onoff_names)
	},
	{
		option_spincontrol,
		"cl_simpleitems",
		"simple items",
		"Draw floating icons instead of 3D models for ingame items",
		setnames (onoff_names)
	},
	{
		option_spincontrol,
		"hand",
		"weapon handedness",
		"NOTE: This does effect aim!",
		setnames (handedness_names)
	},
	{
		option_numberfield,
		"fov",
		"FOV",
		"Horizontal field of view in degrees",
		setfieldsize (fov_limits)
	}
};

#define num_options static_array_size(disp_option_names)

menumultival_s options[num_options];

static void OptionsResetDefaultsFunc (UNUSED void *unused)
{
	Cbuf_AddText ("exec default.cfg\n");
	Cbuf_Execute();

	Options_Menu_Reinitialize ();

	CL_Snd_Restart_f();
	S_StartMenuMusic();

}

static void OptionsResetSavedFunc (UNUSED void *unused)
{
	Cbuf_AddText ("exec config.cfg\n");
	Cbuf_Execute();

	Options_Menu_Reinitialize ();

	CL_Snd_Restart_f();
	S_StartMenuMusic();

}

static void M_Menu_Display_f (void)
{
	options_menu (disp, "DISPLAY");

	M_PushMenu_Defaults (disp.screen);
}

/*
=======================================================================

OPTIONS MENUS - VIDEO OPTIONS MENU

=======================================================================
*/

sliderlimit_t brightnesscontrast_limits = 
{
	1, 20, 0.1f, 2.0f
};

// FIXME: is this really supposed to be separate?
sliderlimit_t bloom_limits = 
{
	0, 20, 0.0f, 2.0f
};

sliderlimit_t modulate_limits = 
{
	1, 5, 1.0f, 5.0f
};

sliderlimit_t dlight_limits = 
{
	0, 8, 0.0f, 8.0f
};

fieldsize_t resolution_width_limits = 
{
	4, 640, 7680
};

fieldsize_t resolution_height_limits = 
{
	4, 480, 4320
};

static const char *resolution_items[] =
{
	"[640 480  ]\0000",
	"[800 600  ]\0001",
	"[960 720  ]\0002",
	"[1024 768 ]\0003",
	"[1152 864 ]\0004",
	"[1280 960 ]\0005",
	"[1280 1024]\0006",
	"[1360 768 ]\0007",
	"[1366 768 ]\0008",
	"[1600 1200]\0009",
	"[1680 1050]\00010",
	"[1920 1080]\00011", // FHD/1080P
	"[2560 1080]\00013", // UWFHD/1080P ultrawide (21:9)
	"[2048 1536]\00012", // out of order mode name to preserve cvar compat
	"[2560 1440]\00014", // QHD/1440P
	"[3440 1440]\00015", // UWQHD/1440P ultrawide (21:9)
	"[3840 2160]\00016", // 4K
	"[7680 4320]\00017", // 8K
	"[custom   ]\000-1",
	0
};

static const char *overbright_items[] = 
{
	"low\0001",
	"medium\0002",
	"high\0003",
	0
};

static const char *texquality_items[] = 
{
	"very low\0003",
	"low\0002",
	"medium\0001",
	"high\0000",
	0
};

static const char *antialiasing_items[] = 
{
	"off\0000",
	"2x\0002",
	"4x\0004",
	"8x\0008",
	"16x\00016",
	0
};

option_name_t video_option_names[] = 
{
	{
		option_textcvarspincontrol,
		"gl_mode",
		"video mode",
		NULL,
		setnames (resolution_items),
		QMF_ACTION_WAIT
	},
	{
		option_numberfield,
		"vid_width",
		"custom width",
		"set custom horizontal screen resolution",
		setfieldsize (resolution_width_limits),
		QMF_ACTION_WAIT
	},
	{
		option_numberfield,
		"vid_height",
		"custom height",
		"set custom vertical screen resolution",
		setfieldsize (resolution_height_limits),
		QMF_ACTION_WAIT
	},
	{
		option_spincontrol,
		"vid_fullscreen",
		"window mode",
		NULL,
		setnames (windowmode_names),
		QMF_ACTION_WAIT
	},
	{
		option_slider,
		"vid_gamma",
		"texture brightness",
		NULL,
		setlimits (brightnesscontrast_limits),
		QMF_ACTION_WAIT
	},
	{
		option_slider,
		"vid_contrast",
		"texture contrast",
		NULL,
		setlimits (brightnesscontrast_limits),
		QMF_ACTION_WAIT
	},
	{
		option_slider,
		"gl_modulate",
		"lightmap brightness",
		NULL,
		setlimits (modulate_limits),
		QMF_ACTION_WAIT
	},
	{
		option_spincontrol,
		"r_bloom",
		"light bloom",
		NULL,
		setnames (onoff_names)
	},
	{
		option_slider,
		"r_bloom_intensity",
		"bloom intensity",
		NULL,
		setlimits (bloom_limits)
	},
	{
		option_textcvarslider,
		"r_overbrightbits",
		"overbright bits",
		NULL,
		setnames (overbright_items)
	},
	{
		option_textcvarslider,
		"gl_picmip",
		"texture quality",
		NULL,
		setnames (texquality_items),
		QMF_ACTION_WAIT
	},
	{
		option_slider,
		"gl_dynamic",
		"dynamic lights",
		"Maximum number of dynamic lights on screen at once",
		setlimits (dlight_limits)
	},
	{
		option_spincontrol,
		"cl_paindist",
		"pain distortion fx",
		"Warp the screen when you take damage",
		setnames (onoff_names)
	},
	{
		option_spincontrol,
		"cl_explosiondist",
		"explosion distortion fx",
		"Explosions warp the screen",
		setnames (onoff_names)
	},
	{
		option_spincontrol,
		"cl_raindist",
		"rain droplet fx",
		"Rain dripping down your visor",
		setnames (onoff_names)
	},
	{
		option_spincontrol,
		"gl_finish",
		"triple buffering",
		"improved framerates, but displayed frame is more out of date",
		setnames (offon_names),
		QMF_ACTION_WAIT
	},
	{
		option_spincontrol,
		"gl_swapinterval",
		"vertical sync",
		"should be on unless framerates dip below your monitor's refresh rate",
		setnames (onoff_names),
		QMF_ACTION_WAIT
	},
	{
		option_textcvarslider,
		"r_antialiasing",
		"multisample anti-aliasing",
		NULL,
		setnames (antialiasing_items),
		QMF_ACTION_WAIT
	}
};

const char *graphical_preset_names[][3] = 
{
	// display name, cfg name, tooltip
	{
		"High Compatibility",	"compatibility",
		"use when all other modes fail or run slowly"
	},
	{
		"High Performance",		"maxperformance",
		"fast rendering, many effects disabled"
	},
	{
		"Performance",			"performance",
		"dynamic lighting and postprocess"
	},
	{
		"Quality",				"quality",
		"per-pixel effects on all surfaces"
	},
	{
		"High Quality",			"maxquality",
		"shadows, light shafts from sun"
	}
};

#define num_graphical_presets static_array_size(graphical_preset_names)

static menuaction_s		s_graphical_presets[num_graphical_presets];

static menuframework_s *Video_MenuInit (void);

static void PresetCallback (void *_self)
{
	char cmd[MAX_STRING_CHARS];
	menuaction_s *self = (menuaction_s *)_self;
	
	Com_sprintf (cmd, sizeof(cmd), "exec graphical_presets/%s.cfg", self->generic.localstrings[0]);
	Cmd_ExecuteString (cmd);
	Cbuf_Execute ();
	Video_MenuInit (); //TODO: alert user of the need to apply here
}

static void VidApplyFunc (void *self)
{
#if defined UNIX_VARIANT
	extern qboolean vid_restart;
#endif
	extern cvar_t *vid_ref;
	
	Menu_ApplyMenu (Menu_GetItemTree ((menuitem_s *)self));
	
	RS_FreeUnmarked();
	Cvar_SetValue("scriptsloaded", 0); //scripts get flushed

	vid_ref->modified = true;
#if defined UNIX_VARIANT
	vid_restart = true;
#endif

	M_ForceMenuOff();
}

static menuframework_s *Video_MenuInit (void)
{
	int i;
	
	options_menu (video, "VIDEO OPTIONS");
	
	add_text (video.window, NULL, 0); // spacer
	
	for (i = 0; i < num_graphical_presets; i++)
	{
		s_graphical_presets[i].generic.type = MTYPE_ACTION;
		s_graphical_presets[i].generic.flags = QMF_BUTTON|QMF_RIGHT_COLUMN;
		s_graphical_presets[i].generic.callback = PresetCallback;
		s_graphical_presets[i].generic.name = graphical_preset_names[i][0];
		s_graphical_presets[i].generic.localstrings[0] = graphical_preset_names[i][1];
		s_graphical_presets[i].generic.tooltip = graphical_preset_names[i][2];
		Menu_AddItem (&video.window, &s_graphical_presets[i]);
	}
	
	add_text (video.window, NULL, 0); // spacer
	
	add_action (video.window, "Apply", VidApplyFunc, 0);
	
	return &video.screen;
}

static void M_Menu_Video_f (void)
{
	menuframework_s *screen = Video_MenuInit ();
	M_PushMenu_Defaults (*screen);
}


/*
=======================================================================

OPTIONS MENUS - AUDIO OPTIONS MENU

=======================================================================
*/

sliderlimit_t volume_limits = 
{
	1, 50, 0.0f, 1.0f
};

option_name_t audio_option_names[] = 
{
	{
		option_spincontrol,
		"cl_playtaunts",
		"player taunts",
		NULL,
		setnames (onoff_names)
	},
	{
		option_slider,
		"s_volume",
		"global volume", 
		NULL,
		setlimits (volume_limits)
	},
	{
		option_slider,
		"background_music_vol",
		"music volume", 
		NULL,
		setlimits (volume_limits)
	},
	{
		option_spincontrol,
		"background_music",
		"music",
		NULL, 
		setnames (onoff_names)
	},
	{
		option_textcvarslider,
		"s_doppler",
		"doppler effect",
		NULL,
		setnames (doppler_effect_items)
	},
};

static void M_Menu_Audio_f (void)
{
	options_menu (audio, "AUDIO OPTIONS");
	M_PushMenu_Defaults (audio.screen);
}


/*
=======================================================================

OPTIONS MENUS - INPUT OPTIONS MENU

=======================================================================
*/

static const char *slowfast_names[] =
{
	"slow",
	"fast",
	0
};

option_name_t input_option_names[] = 
{
	{
		option_slider,
		"sensitivity",
		"mouse speed",
		NULL,
		setlimits (mousespeed_limits)
	},
	{
		option_slider,
		"menu_sensitivity",
		"menu mouse speed",
		NULL,
		setlimits (mousespeed_limits)
	},
	{
		option_spincontrol,
		"m_accel", 
		"mouse acceleration",
		NULL,
		setnames (onoff_names)
	},
	{
		option_spincontrol,
		"in_joystick",
		"use joystick",
		NULL,
		setnames (onoff_names)
	},
	{
		option_spincontrol,
		"cl_run",
		"primary running speed",
		"You will run at this speed unless you hold down the \"running speed select\" button",
		setnames (slowfast_names)
	},
};

static void InvertMouseFunc( void *_self )
{
	menulist_s *self = (menulist_s *)_self;
	if(self->curvalue && m_pitch->value > 0)
		Cvar_SetValue( "m_pitch", -m_pitch->value );
	else if(m_pitch->value < 0)
		Cvar_SetValue( "m_pitch", -m_pitch->value );
}

static void CustomizeControlsFunc (UNUSED void *unused)
{
	M_Menu_Keys_f ();
}

static void M_Menu_Input_f (void)
{
	options_menu (input, "INPUT OPTIONS");

	{
		static menulist_s s_options_invertmouse_box;
		s_options_invertmouse_box.generic.name	= "invert mouse";
		s_options_invertmouse_box.generic.callback = InvertMouseFunc;
		s_options_invertmouse_box.curvalue		= m_pitch->value < 0;
		setup_tickbox (s_options_invertmouse_box);
		Menu_AddItem (&input.panel, &s_options_invertmouse_box);
	}
	
	add_text (input.window, NULL, 0); //spacer
	
	add_action (input.window, "Key Bindings", CustomizeControlsFunc, 0);
	
	M_PushMenu_Defaults (input.screen);
}

/*
=======================================================================

OPTIONS MENUS - NETWORK OPTIONS MENU

=======================================================================
*/

option_name_t net_option_names[] = 
{
	{
		option_spincontrol,
		"allow_download",
		"download missing files",
		NULL,
		setnames (onoff_names)
	},
	{
		option_spincontrol,
		"allow_download_maps",
		"maps",
		NULL,
		setnames (onoff_names)
	},
	{
		option_spincontrol,
		"allow_download_players",
		"player models/skins",
		NULL,
		setnames (onoff_names)
	},
	{
		option_spincontrol,
		"allow_download_models",
		"models",
		NULL,
		setnames (onoff_names)
	},
	{
		option_spincontrol,
		"allow_download_sounds",
		"sounds",
		NULL,
		setnames (onoff_names)
	},
	{
		option_spincontrol,
		"allow_overwrite_maps",
		"installmap overwrites map files",
		NULL,
		setnames (onoff_names)
	},
};

static void M_Menu_Net_f (void)
{
	options_menu (net, "NETWORK OPTIONS");
	M_PushMenu_Defaults (net.screen);
}


/*
=============================================================================

OPTIONS MENUS - IRC OPTIONS MENU

=============================================================================
*/

char			IRC_key[64];

static menuframework_s	s_irc_screen;

static menuframework_s	s_irc_menu;
static menuaction_s		s_irc_join;
static menulist_s		s_irc_joinatstartup;

static menufield_s		s_irc_server;
static menufield_s		s_irc_channel;
static menufield_s		s_irc_port;
static menulist_s		s_irc_ovnickname;
static menufield_s		s_irc_nickname;
static menufield_s		s_irc_kickrejoin;
static menufield_s		s_irc_reconnectdelay;

static void JoinIRCFunc (UNUSED void *unused)
{
	if(PLAYER_NAME_UNIQUE)
		CL_InitIRC();
}

static void QuitIRCFunc (UNUSED void *unused)
{
	CL_IRCInitiateShutdown();
}

static void ApplyIRCSettings (UNUSED void *self)
{
	qboolean running = CL_IRCIsRunning( );
	if ( running ) {
		CL_IRCInitiateShutdown( );
		CL_IRCWaitShutdown( );
	}

	Cvar_Set(	"cl_IRC_server" ,		s_irc_server.buffer);
	Cvar_Set(	"cl_IRC_channel" ,		s_irc_channel.buffer);
	Cvar_SetValue(	"cl_IRC_port" , 		atoi( s_irc_port.buffer ) );
	Cvar_SetValue(	"cl_IRC_override_nickname" ,	s_irc_ovnickname.curvalue );
	Cvar_Set(	"cl_IRC_nickname" ,		s_irc_nickname.buffer );
	Cvar_SetValue(	"cl_IRC_kick_rejoin" ,		atoi( s_irc_kickrejoin.buffer ) );
	Cvar_SetValue(	"cl_IRC_reconnect_delay" ,	atoi( s_irc_reconnectdelay.buffer ) );

	if ( running )
		CL_InitIRC( );

	M_PopMenu( );
}

// TODO: use the options menu macros like everywhere else.
static void IRC_Settings_SubMenuInit( )
{
	setup_tickbox (s_irc_joinatstartup);
	s_irc_joinatstartup.generic.name	= "join at startup";
	s_irc_joinatstartup.generic.localstrings[0] = "cl_IRC_connect_at_startup";
	s_irc_joinatstartup.curvalue = cl_IRC_connect_at_startup->integer != 0;
	s_irc_joinatstartup.generic.callback = SpinOptionFunc;
	Menu_AddItem( &s_irc_menu, &s_irc_joinatstartup );

	s_irc_server.generic.type		= MTYPE_FIELD;
	s_irc_server.generic.name		= "server";
	s_irc_server.generic.tooltip	= "Address or name of the IRC server";
	s_irc_server.generic.visible_length		= LONGINPUT_SIZE;
	s_irc_server.cursor			= strlen( cl_IRC_server->string );
	strcpy( s_irc_server.buffer, Cvar_VariableString("cl_IRC_server") );
	Menu_AddItem( &s_irc_menu, &s_irc_server );

	s_irc_channel.generic.type		= MTYPE_FIELD;
	s_irc_channel.generic.name		= "channel";
	s_irc_channel.generic.tooltip	= "Name of the channel to join";
	s_irc_channel.generic.visible_length		= LONGINPUT_SIZE;
	s_irc_channel.cursor			= strlen( cl_IRC_channel->string );
	strcpy( s_irc_channel.buffer, Cvar_VariableString("cl_IRC_channel") );
	Menu_AddItem( &s_irc_menu, &s_irc_channel );

	s_irc_port.generic.type			= MTYPE_FIELD;
	s_irc_port.generic.name			= "port";
	s_irc_port.generic.tooltip		= "Port to connect to on the server";
	s_irc_port.generic.visible_length		= 4;
	s_irc_port.cursor			= strlen( cl_IRC_port->string );
	strcpy( s_irc_port.buffer, Cvar_VariableString("cl_IRC_port") );
	Menu_AddItem( &s_irc_menu, &s_irc_port );

	s_irc_ovnickname.generic.name		= "override nick";
	s_irc_ovnickname.generic.tooltip	= "Enable this to override the default, player-based nick";
	setup_tickbox (s_irc_ovnickname);
	s_irc_ovnickname.curvalue		= cl_IRC_override_nickname->value ? 1 : 0;
	Menu_AddItem( &s_irc_menu, &s_irc_ovnickname );

	s_irc_nickname.generic.type		= MTYPE_FIELD;
	s_irc_nickname.generic.name		= "nick";
	s_irc_nickname.generic.tooltip	= "Nickname override to use";
	s_irc_nickname.generic.visible_length		= LONGINPUT_SIZE;
	s_irc_nickname.cursor			= strlen( cl_IRC_nickname->string );
	strcpy( s_irc_nickname.buffer, Cvar_VariableString("cl_IRC_nickname") );
	Menu_AddItem( &s_irc_menu, &s_irc_nickname );

	s_irc_kickrejoin.generic.type		= MTYPE_FIELD;
	s_irc_kickrejoin.generic.name		= "autorejoin";
	s_irc_kickrejoin.generic.tooltip	= "Delay before automatic rejoin after kick (0 to disable)";
	s_irc_kickrejoin.generic.visible_length		= 4;
	s_irc_kickrejoin.cursor			= strlen( cl_IRC_kick_rejoin->string );
	strcpy( s_irc_kickrejoin.buffer, Cvar_VariableString("cl_IRC_kick_rejoin") );
	Menu_AddItem( &s_irc_menu, &s_irc_kickrejoin );

	s_irc_reconnectdelay.generic.type	= MTYPE_FIELD;
	s_irc_reconnectdelay.generic.name	= "reconnect";
	s_irc_reconnectdelay.generic.tooltip= "Delay between reconnection attempts (minimum 5)";
	s_irc_reconnectdelay.generic.visible_length	= 4;
	s_irc_reconnectdelay.cursor		= strlen( cl_IRC_reconnect_delay->string );
	strcpy( s_irc_reconnectdelay.buffer, Cvar_VariableString("cl_IRC_reconnect_delay") );
	Menu_AddItem( &s_irc_menu, &s_irc_reconnectdelay );

	add_action (s_irc_menu, "Apply", ApplyIRCSettings, 0);

}


static void M_FindIRCKey ( void )
{
	int		count;
	int		j;
	int		l;
	char	*b;
	int twokeys[2];

	twokeys[0] = twokeys[1] = -1;
	l = strlen("messagemode3");
	count = 0;

	for (j=0 ; j<256 ; j++)
	{
		b = keybindings[j];
		if (!b)
			continue;
		if (!strncmp (b, "messagemode3", l) )
		{
			twokeys[count] = j;
			count++;
			if (count == 2)
				break;
		}
	}
	//got our key
	Com_sprintf(IRC_key, sizeof(IRC_key), "(IRC Chat Key is %s)", Key_KeynumToString(twokeys[0]));
}

static void IRC_MenuInit (void)
{
	if(!cl_IRC_connect_at_startup)
		cl_IRC_connect_at_startup = Cvar_Get("cl_IRC_connect_at_startup", "0", CVAR_ARCHIVE);

	M_FindIRCKey();
	
	setup_window (s_irc_screen, s_irc_menu, "IRC CHAT OPTIONS");

	s_irc_join.generic.type	= MTYPE_ACTION;
	s_irc_join.generic.flags = QMF_BUTTON;
	s_irc_join.generic.name	= "Connect Now";
	s_irc_join.generic.callback = JoinIRCFunc;
	Menu_AddItem( &s_irc_menu, &s_irc_join );

	IRC_Settings_SubMenuInit ();

	add_text (s_irc_menu, IRC_key, 0);
	Menu_AutoArrange (&s_irc_screen);
}


static void IRC_MenuDraw (UNUSED menuframework_s *dummy, menuvec2_t offset)
{
	//warn user that they cannot join until changing default player name
	if(!PLAYER_NAME_UNIQUE)
		s_irc_menu.statusbar = "You must create your player name before joining a server!";
	else if(CL_IRCIsConnected())
		s_irc_menu.statusbar = "Connected to IRC server.";
	else if(CL_IRCIsRunning())
		s_irc_menu.statusbar = "Connecting to IRC server...";
	else
		s_irc_menu.statusbar = "Not connected to IRC server.";

	// Update join/quit menu entry
	if ( CL_IRCIsRunning( ) ) {
		s_irc_join.generic.name	= "Disconnect Now";
		s_irc_join.generic.callback = QuitIRCFunc;
	} else {
		s_irc_join.generic.name	= "Connect Now";
		s_irc_join.generic.callback = JoinIRCFunc;
	}

	Screen_Draw (&s_irc_screen, offset);
}

static void M_Menu_IRC_f (void)
{
	IRC_MenuInit();
	M_PushMenu (IRC_MenuDraw, Default_MenuKey, &s_irc_screen);
}


/*
=======================================================================

OPTIONS MENUS - TOP-LEVEL OPTIONS MENU

=======================================================================
*/

static menuframework_s	s_options_screen;
static menuframework_s	s_options_menu;

char *option_screen_names[] =
{
	"Player", // whatever's first will be the default
	"Display",
	"Video",
	"Audio",
	"Input",
	"Network", 
	"IRC Chat",
};
#define OPTION_SCREENS static_array_size(option_screen_names)

void (*option_open_funcs[OPTION_SCREENS])(void) = 
{
	&M_Menu_PlayerConfig_f,
	&M_Menu_Display_f,
	&M_Menu_Video_f,
	&M_Menu_Audio_f,
	&M_Menu_Input_f,
	&M_Menu_Net_f,
	&M_Menu_IRC_f,
};

static menuframework_s	s_player_config_screen;

static menuaction_s		s_option_screen_actions[OPTION_SCREENS];

static void OptionScreenFunc (void *_self)
{
	menuframework_s *self = (menuframework_s *)_self;
	
	option_open_funcs[self->generic.localints[0]]();
}

static void M_Menu_Options_f (void)
{
	int i;
	
	setup_window (s_options_screen, s_options_menu, "OPTIONS");
	
	for (i = 0; i < OPTION_SCREENS; i++)
	{
		s_option_screen_actions[i].generic.type = MTYPE_ACTION;
		s_option_screen_actions[i].generic.flags = QMF_BUTTON;
		s_option_screen_actions[i].generic.name = option_screen_names[i];
		s_option_screen_actions[i].generic.localints[0] = i;
		s_option_screen_actions[i].generic.callback = OptionScreenFunc;
		Menu_AddItem (&s_options_menu, &s_option_screen_actions[i]);
	}
	
	add_text (s_options_menu, NULL, 0); //spacer
	
	add_action (s_options_menu, "Reset to Defaults", OptionsResetDefaultsFunc, 0);
	add_action (s_options_menu, "Restore from Saved", OptionsResetSavedFunc, 0);
	
	M_PushMenu_Defaults (s_options_screen);
	
	// select the default options screen
	OptionScreenFunc (&s_option_screen_actions[0]);
}

/*
=============================================================================

END GAME MENU

=============================================================================
*/
static int credits_start_time;
static const char **credits;
//static char *creditsIndex[256];
static char *creditsBuffer;
static const char *idcredits[] =
{
	"+Alien Arena by COR Entertainment",
	"",
	"+PROGRAMMING",
	"John Diamond",
	"Jim Bower",
	"Emmanuel Benoit",
	"Max Eliaser",
	"Charles Hudson",
	"Lee Salzman",
	"Dave Carter",
	"Victor Luchits",
	"Jan Rafaj",
	"Shane Bayer",
	"Tony Jackson",
	"Stephan Stahl",
	"Kyle Hunter",
	"Andres Mejia",
	"",
	"+ART",
	"John Diamond",
	"Dennis -xEMPx- Zedlach",
	"Franc Cassar",
	"Shawn Keeth",
	"Enki",
	"",
	"+FONTS",
	"John Diamond",
	"The League of Moveable Type",
	"Brian Kent",
	"",
	"+LOGO",
	"Adam -servercleaner- Szalai",
	"Paul -GimpAlien-",
	"",
	"+LEVEL DESIGN",
	"John Diamond",
	"Dennis -xEMPx- Zedlach",
	"Charles Hudson",
	"Torben Fahrnbach",
	"",
	"+SOUND EFFECTS AND MUSIC",
	"Music/FX Composed and Produced by",
	"Paul Joyce, Whitelipper, Divinity",
	"Arteria Games, Wooden Productions",
	"and Soundrangers.com",
	"",
	"+CROSSHAIRS AND HUDS",
	"Astralsin",
	"Dangeresque",
	"Phenax",
	"Roadrage",
	"Forsaken",
	"Capt Crazy",
	"Torok -Intimidator- Ivan",
	"Stratocaster",
	"ChexGuy",
	"Crayon",
	"",
	"+LINUX PORT",
	"Shane Bayer",
	"",
	"+FREEBSD PORT",
	"Ale",
	"",
	"+GENTOO PORTAGE",
	"Paul Bredbury",
	"",
	"+DEBIAN PACKAGE",
	"Andres Mejia",
	"",
	"+LANGUAGE TRANSLATIONS",
	"Ken Deguisse",
	"",
	"+STORYLINE",
	"Sinnocent",
	"",
	"+SPECIAL THANKS",
	"The Alien Arena Community",
	"and everyone else who",
	"has been loyal to the",
	"game.",
	"",
	"",
	"+ALIEN ARENA - THE STORY",
	"",
	"Alien Arena : Many are called, only one will reign supreme",
	"",
	"Eternal war ravaged the vastness of infinite space.",
	"For as far back into the ages as the memories of the",
	"galaxies oldest races could reach, it had been this way.",
	"Planet at war with planet, conflicts ending with a",
	"burned cinder orbiting a dead sun and countless billions",
	"sent screaming into oblivion. War, endless and ",
	"eternal, embracing all the peoples of the cosmos.",
	"Scientific triumphs heralded the creation of ever more",
	"deadly weapons until the very fabric of the universe itself was threatened.",
	"",
	"Then came the call.",
	"",
	"Some said it was sent by an elder race, legendary beings",
	"of terrifying power who had existed since the",
	"birth of the stars and who now made their home beyond",
	"the fringes of known creation, others whispered",
	"fearfully and looked to the skies for the coming of their",
	"gods. Perhaps it didn't matter who had sent the",
	"call, for all the people of the stars could at least agree",
	"that the call was there.",
	"",
	"The wars were to end - or they would be ended. In a",
	"demonstration of power whoever had sent the call",
	"snuffed out the homeworld of the XXXX, the greatest",
	"empire of all the stars, in a heartbeat. One moment it",
	"was there, the next it was dust carried on the solar winds.",
	"All races had no choice but to heed the call.",
	"",
	"For most the call was a distant tug, a whispered warning",
	"that the wars were over, but for the greatest",
	"hero of each people it was more, it was a fire raging",
	"through their blood, a call to a new war, to the battle",
	"to end all battles. That fire burns in your blood, compelling",
	"you, the greatest warrior of your people, to fight",
	"in a distant and unknown arena, your honor and the",
	"future of your race at stake.",
	"",
	"Across the stars you traveled in search of this arena where",
	"the mightiest of the mighty would do battle,",
	"where you would stand face to face with your enemies",
	"in a duel to the death. Strange new weapons",
	"awaited you, weapons which you would have to master",
	"if you were to survive and emerge victorious from",
	"the complex and deadly arenas in which you were summoned to fight.",
	"",
	"The call to battle beats through your heart and soul",
	"like the drums of war. Will you be the one to rise",
	"through the ranks and conquer all others, the one who",
	"stands proud as the undefeated champion of the",
	"Alien Arena?",
	"",
	"Alien Arena (C)2007-2012 COR Entertainment, LLC",
	"All Rights Reserved.",
	0
};

static void M_Credits_MenuDraw (UNUSED menuframework_s *dummy, menuvec2_t offset)
{
	int i, y, scale;
	FNT_font_t		font;
	struct FNT_window_s	box;
	
	font = FNT_AutoGet( CL_menuFont );
	scale = font->size / 8.0;
	
	/*
	** draw the credits
	*/
	for ( i = 0, y = viddef.height - ( ( cls.realtime - credits_start_time ) / 40.0F ); credits[i]; y += 12*scale, i++ )
	{
		if ( y <= -12*scale )
			continue;
		
		box.y = offset.y + y;
		box.x = offset.x;
		box.height = 0;
		box.width = viddef.width;

		if ( credits[i][0] == '+' )
		{
			FNT_BoundedPrint (font, credits[i]+1, FNT_CMODE_NONE, FNT_ALIGN_CENTER, &box, FNT_colors[3]);
		}
		else
		{
			FNT_BoundedPrint (font, credits[i], FNT_CMODE_NONE, FNT_ALIGN_CENTER, &box, FNT_colors[7]);
		}
	}

	if ( y < 0 )
		credits_start_time = cls.realtime;
}

static const char *M_Credits_Key (UNUSED menuframework_s *dummy, int key)
{
	if (key == K_ESCAPE)
	{
		if (creditsBuffer)
			FS_FreeFile (creditsBuffer);
		M_PopMenu ();
	}

	return menu_out_sound;

}

static void M_Menu_Credits_f (void)
{
	static menuframework_s dummy;
	
	CHASELINK(dummy.rwidth) = viddef.width;
	
	creditsBuffer = NULL;
	credits = idcredits;
	credits_start_time = cls.realtime;

	M_PushMenu (M_Credits_MenuDraw, M_Credits_Key, &dummy);
}

/*
=============================================================================

GAME MENU

=============================================================================
*/

static void StartGame( void )
{
	extern cvar_t *name;
	char pw[64];

	// disable updates
	cl.servercount = -1;
	M_ForceMenuOff ();
	Cvar_SetValue( "deathmatch", 1 );
	Cvar_SetValue( "ctf", 0 );

	//listen servers are passworded
	sprintf(pw, "%s%4.2f", name->string, crand());
	Cvar_Set ("password", pw);

	Cvar_SetValue( "gamerules", 0 );		//PGM

	Cbuf_AddText ("loading ; killserver ; wait ; newgame\n");
	cls.key_dest = key_game;
}

static void SinglePlayerGameFunc (void *data)
{
	char skill[3];
	Com_sprintf(skill, sizeof(skill), "%d", ((menuaction_s*)data)->generic.localints[0]);
	Cvar_ForceSet ("skill", skill);
	StartGame ();
}

static void SinglePlayerGameFuncPrototype (void *_self)
{
	extern cvar_t *name;
	char pw[64];
	menuaction_s *self;
	char skill[2];
	
	self = (menuaction_s *)_self;
	
	skill[1] = '\0';
	skill[0] = self->generic.localints[0]+'0';
	Cvar_ForceSet ("skill", skill);
	
	// disable updates
	cl.servercount = -1;
	M_ForceMenuOff ();
	
	//listen servers are passworded (TODO: just unset public!)
	sprintf(pw, "%s%4.2f", name->string, crand());
	Cvar_Set ("password", pw);
	
	printf ("WANT MODE %d\n", self->generic.localints[1]);
	SetGameModeCvars (self->generic.localints[1]);
	
	Cbuf_AddText ("loading ; killserver ; wait ; newgame\n");
	cls.key_dest = key_game;
}

static void M_Menu_Game_f (void)
{
	static menuframework_s	s_game_screen;
	static menuframework_s	s_game_menu;

	static const char *singleplayer_skill_level_names[][2] = {
		{"Very Easy",	"Bot slaughter"},
		{"Easy",		"You will win"},
		{"Medium",		"You might win"},
		{"Hard",		"Very challenging"},
		{"Ultra",		"Only the best will win"}
	};
	#define num_singleplayer_skill_levels  static_array_size(singleplayer_skill_level_names)
	static menuaction_s		s_singleplayer_game_actions[num_singleplayer_skill_levels];
	
	int i;
	
	setup_window (s_game_screen, s_game_menu, "PRACTICE");
	
	for (i = 0; i < num_singleplayer_skill_levels; i++)
	{
		s_singleplayer_game_actions[i].generic.type = MTYPE_ACTION;
		s_singleplayer_game_actions[i].generic.flags = QMF_BUTTON;
		s_singleplayer_game_actions[i].generic.name = singleplayer_skill_level_names[i][0];
		s_singleplayer_game_actions[i].generic.tooltip = singleplayer_skill_level_names[i][1];
		s_singleplayer_game_actions[i].generic.localints[0] = i - 1;
		s_singleplayer_game_actions[i].generic.callback = SinglePlayerGameFunc;
		Menu_AddItem (&s_game_menu, &s_singleplayer_game_actions[i]);
	}

	Menu_AutoArrange (&s_game_screen);
	Menu_Center (&s_game_screen);
	
	M_PushMenu_Defaults (s_game_screen);
}

static void SinglePlayerSelectSkillFunc (void *_self)
{
	static char				selfname[1024];
	static menuaction_s		*self;
	
	static menuframework_s	s_skill_screen;
	static menuframework_s	s_skill_menu;

	static const char *singleplayer_skill_level_names[][2] = {
		{"Easy",	"You will win"},
		{"Medium",	"You might win"},
		{"Hard",	"Very challenging"},
		{"Ultra",	"Only the best will win"}
	};
	#define num_singleplayer_skill_levels  static_array_size(singleplayer_skill_level_names)
	static menuaction_s		s_singleplayer_skill_actions[num_singleplayer_skill_levels];
	
	int i;
	
	self = (menuaction_s *)_self;
	
	for (i = 0; i < sizeof(selfname) - 1 && self->generic.name[i]; i++)
		selfname[i] = toupper (self->generic.name[i]);
	selfname[i] = '\0';
	
	setup_window (s_skill_screen, s_skill_menu, selfname);
	
	for (i = 0; i < num_singleplayer_skill_levels; i++)
	{
		s_singleplayer_skill_actions[i].generic.type = MTYPE_ACTION;
		s_singleplayer_skill_actions[i].generic.flags = QMF_BUTTON;
		s_singleplayer_skill_actions[i].generic.name = singleplayer_skill_level_names[i][0];
		s_singleplayer_skill_actions[i].generic.tooltip = singleplayer_skill_level_names[i][1];
		s_singleplayer_skill_actions[i].generic.localints[0] = i;
		s_singleplayer_skill_actions[i].generic.localints[1] = self->generic.localints[0];
		s_singleplayer_skill_actions[i].generic.callback = SinglePlayerGameFuncPrototype;
		Menu_AddItem (&s_skill_menu, &s_singleplayer_skill_actions[i]);
	}

	Menu_AutoArrange (&s_skill_screen);
	Menu_Center (&s_skill_screen);
	
	M_PushMenu_Defaults (s_skill_screen);
}

static void M_Menu_Game_prototype_f (void)
{
	static menuframework_s	s_mode_screen;
	static menuframework_s	s_mode_menu;
	
	static menuaction_s		s_singleplayer_mode_actions[num_game_modes];
	
	int i;
	
	setup_window (s_mode_screen, s_mode_menu, "SINGLE PLAYER");
	
	for (i = 0; i < num_game_modes; i++)
	{
		s_singleplayer_mode_actions[i].generic.type = MTYPE_ACTION;
		s_singleplayer_mode_actions[i].generic.flags = QMF_BUTTON;
		s_singleplayer_mode_actions[i].generic.name = game_mode_names[i];
		// TODO: tooltip
		s_singleplayer_mode_actions[i].generic.localints[0] = i;
		s_singleplayer_mode_actions[i].generic.callback = SinglePlayerSelectSkillFunc;
		Menu_AddItem (&s_mode_menu, &s_singleplayer_mode_actions[i]);
	}

	Menu_AutoArrange (&s_mode_screen);
	Menu_Center (&s_mode_screen);
	
	M_PushMenu_Defaults (s_mode_screen);
}



/*
=============================================================================

JOIN SERVER MENU

=============================================================================
*/
#define MAX_LOCAL_SERVERS 128
#define MAX_SERVER_MODS 16
#define SERVER_LIST_COLUMNS 4

static const char *updown_names[] = {
	"menu/midarrow",
	"menu/dnarrow",
	"menu/uparrow",
	0
};


//Lists for all stock mutators and game modes, plus some of the more popular
//custom ones. (NOTE: For non-boolean cvars, i.e. those which have values
//other than 0 or 1, you get a string of the form cvar=value. In the future,
//we may do something special to parse these, but since no such cvars are
//actually recognized right now anyway, we currently don't.)

//TODO: Have a menu to explore this list?

//Names. If a cvar isn't recognized, the name of the cvar itself is used.
static char mod_names[] =
	//cannot be wider than this boundary:	|
	"\\ctf"             "\\capture the flag"
	"\\tca"             "\\team core assault"
	"\\all_out_assault" "\\all out assault"
	"\\instagib"        "\\instagib"
	"\\rocket_arena"    "\\rocket arena"
	"\\low_grav"        "\\low gravity"
	"\\regeneration"    "\\regeneration"
	"\\vampire"         "\\vampire"
	"\\excessive"       "\\excessive"
	"\\grapple"         "\\grappling hook"
	"\\classbased"      "\\class based"
	"\\g_duel"          "\\duel mode"
	"\\quickweap"       "\\quick switch"
	"\\anticamp"        "\\anticamp"
	"\\sv_joustmode"    "\\joust mode"
	"\\playerspeed"     "\\player speed"
	"\\insta_rockets"   "\\insta/rockets"
	"\\chaingun_arena"  "\\chaingun arena"
	"\\instavap"        "\\vaporizer arena"
	"\\vape_arena"      "\\vaporizer arena"
	"\\testcode"        "\\code testing"
	"\\testmap"         "\\map testing"
	"\\dodgedelay=0"    "\\rapid dodging"
	"\\g_tactical"      "\\aa tactical"
	"\\g_dm_lights"     "\\player lights"
	"\\";

//Descriptions. If a cvar isn't recognized, "(no description)" is used.
static char mods_desc[] =
	//cannot be wider than this boundary:									|
	"\\ctf"             "\\capture the enemy team's flag to earn points"
	"\\tca"             "\\destroy the enemy team's spider node to win"
	"\\all_out_assault" "\\random jet pack spawns, fast weapon spawns"
	"\\instagib"        "\\disruptor only, instant kill, infinite ammo"
	"\\rocket_arena"    "\\rocket launcher only, infinite ammo"
	"\\low_grav"        "\\reduced gravity"
	"\\regeneration"    "\\regain health over time"
	"\\vampire"         "\\regain health by damaging people"
	"\\excessive"       "\\all weapons enhanced, infinite ammo"
	"\\grapple"         "\\spawn with a grappling hook"
	"\\classbased"      "\\different races have different strengths"
	"\\g_duel"          "\\wait in line for your turn to duel"
	"\\quickweap"       "\\switch weapons instantly"
	"\\anticamp"        "\\you are punished for holding still too long"
	"\\sv_joustmode"    "\\you can still jump while in midair"
	"\\playerspeed"     "\\run much faster than normal"
	"\\insta_rockets"   "\\hybrid of instagib and rocket_arena"
	"\\chaingun_arena"  "\\chaingun only, infinite ammo"
	"\\instavap"        "\\vaporizer only, infinite ammo"
	"\\vape_arena"      "\\vaporizer only, infinite ammo"
	"\\testcode"        "\\server is testing experimental code"
	"\\testmap"         "\\server is testing an unfinished map"
	"\\dodgedelay=0"    "\\no minimum time between dodges"
	"\\g_tactical"      "\\humans vs martians, destroy enemy bases"
	"\\g_dm_lights"     "\\high-visibility lights on players"
	"\\";

static char *GetLine (char **contents, int *len)
{
	static char line[2048];
	int num;
	int i;

	num = 0;
	line[0] = '\0';

	if (*len <= 0)
		return NULL;

	for (i = 0; i < *len; i++) {
		if ((*contents)[i] == '\n') {
			*contents += (num + 1);
			*len -= (num + 1);
			line[num] = '\0';
			return line;
		} 
		line[num] = (*contents)[i];
		num++;
	}

	line[num] = '\0';
	return line;
}


SERVERDATA mservers[MAX_LOCAL_SERVERS];

PLAYERSTATS thisPlayer;

#define m_num_servers (s_serverlist_submenu.nitems)

static char local_mods_data[16][53]; //53 is measured max tooltip width


static struct
{
	menuframework_s	screen;
	menuframework_s	menu;
	
	menutxt_s		name;
	menuaction_s	connect;
	
	menuitem_s		levelshot;
	char			levelshot_path[MAX_QPATH];

	menuframework_s	serverinfo_submenu;
	menuframework_s	serverinfo_table;
	menuframework_s	serverinfo_rows[8];
	menutxt_s		serverinfo_columns[8][2];

	menuframework_s	modlist_submenu;
	menuaction_s	modlist[MAX_SERVER_MODS];
	char			modtxt[MAX_SERVER_MODS][48];
	char			modnames[MAX_SERVER_MODS][24];

	menuframework_s	playerlist_submenu;
	menuaction_s	playerlist_label;
	menuframework_s	playerlist_header;
	menuframework_s	playerlist_scrollingmenu;
	menutxt_s		playerlist_header_columns[SVDATA_PLAYERINFO];
	menuframework_s	playerlist_rows[MAX_PLAYERS];
	menutxt_s		playerlist_columns[MAX_PLAYERS][SVDATA_PLAYERINFO];
	char			ranktxt[MAX_PLAYERS][32];
} s_servers[MAX_LOCAL_SERVERS];

static int serverindex;

static void JoinServerFunc (UNUSED void *unused)
{
	int		i;
	char	buffer[128];
	
	if(!PLAYER_NAME_UNIQUE) {
		M_Menu_PlayerConfig_f();
		return;
	}
	
	cl.tactical = false;

	remoteserver_runspeed = 300; //default
	for ( i = 0; i < 16; i++)
	{
		if( !strcmp("aa tactical", Info_ValueForKey(mod_names, local_mods_data[i])) )
		{
			remoteserver_runspeed = 200; //for correct prediction
			M_Menu_Tactical_f();
			return;
		}
		else if( !strcmp("excessive", Info_ValueForKey(mod_names, local_mods_data[i])) )
			remoteserver_runspeed = 450;
		else if( !strcmp("playerspeed", Info_ValueForKey(mod_names, local_mods_data[i])) )
			remoteserver_runspeed = 450;
	} //TO DO:  We need to do the speed check on connect instead - meaning the server will need to be pinged and parsed there as well(but only if not done already through the menu).

	Com_sprintf (buffer, sizeof(buffer), "connect %s\n", NET_AdrToString (mservers[serverindex].local_server_netadr));
	Cbuf_AddText (buffer);
	M_ForceMenuOff ();
}

static void ModList_SubmenuInit (void)
{
	int i;
	char	modstring[64];
	char	*token;
	
	s_servers[serverindex].modlist_submenu.generic.type = MTYPE_SUBMENU;
	s_servers[serverindex].modlist_submenu.navagable = true;
	s_servers[serverindex].modlist_submenu.nitems = 0;
	for ( i = 0; i < MAX_SERVER_MODS; i++ )
	{
		s_servers[serverindex].modlist[i].generic.type	= MTYPE_ACTION;
		s_servers[serverindex].modlist[i].generic.flags = QMF_RIGHT_COLUMN;
		s_servers[serverindex].modlist[i].generic.name	= s_servers[serverindex].modnames[i];
		s_servers[serverindex].modlist[i].generic.tooltip = s_servers[serverindex].modtxt[i];
		
		Menu_AddItem( &s_servers[serverindex].modlist_submenu, &s_servers[serverindex].modlist[i] );
	}
	
	s_servers[serverindex].modlist_submenu.maxlines = 5;
	
	//Copy modstring over since strtok will modify it
	Q_strncpyz(modstring, mservers[serverindex].modInfo, sizeof(modstring));
	
	// populate all the data
	token = strtok(modstring, "%%");
	for (i=0; i<MAX_SERVER_MODS; i++) {
		if (!token)
			break;
		Com_sprintf(local_mods_data[i], sizeof(local_mods_data[i]), token);
		token = strtok(NULL, "%%");
		
		Com_sprintf (   s_servers[serverindex].modtxt[i], sizeof(s_servers[serverindex].modtxt[i]),
						Info_ValueForKey(mods_desc, local_mods_data[i])
					);
		if (!strlen(s_servers[serverindex].modtxt[i]))
			Com_sprintf (s_servers[serverindex].modtxt[i], sizeof(s_servers[serverindex].modtxt[i]), "(no description)");
		
		Com_sprintf (   s_servers[serverindex].modnames[i], sizeof(s_servers[serverindex].modnames[i]),
						Info_ValueForKey(mod_names, local_mods_data[i])
					);
		if (!strlen(s_servers[serverindex].modnames[i]))
			Com_sprintf (s_servers[serverindex].modnames[i], sizeof(s_servers[serverindex].modnames[i]), local_mods_data[i]);
	}
	s_servers[serverindex].modlist_submenu.nitems = i;
	s_servers[serverindex].modlist_submenu.yscroll = 0;
}

static void ServerInfo_SubmenuInit (void)
{
	size_t sizes[2] = {sizeof(menutxt_s), sizeof(menutxt_s)};
	#if STATS_ENABLED
		#define rows 7
	#else
		#define rows 6
	#endif

	#if STATS_ENABLED
		char *contents[2*rows+1] = {
			"Map:",			mservers[serverindex].szMapName,
			"Skill:",		mservers[serverindex].skill,
			"Admin:",		mservers[serverindex].szAdmin,
			"Website:",		mservers[serverindex].szWebsite,
			"Fraglimit:",	mservers[serverindex].fraglimit,
			"Timelimit:",	mservers[serverindex].timelimit,
			"Version:",		mservers[serverindex].szVersion,
			"Gameplay:"
		};
	#else
		char *contents[2*rows+1] = {
			"Map:",			mservers[serverindex].szMapName,
			"Admin:",		mservers[serverindex].szAdmin,
			"Website:",		mservers[serverindex].szWebsite,
			"Fraglimit:",	mservers[serverindex].fraglimit,
			"Timelimit:",	mservers[serverindex].timelimit,
			"Version:",		mservers[serverindex].szVersion,
			"Gameplay:"
		};
	#endif
	

	Com_sprintf (
		s_servers[serverindex].levelshot_path,
		sizeof(s_servers[serverindex].levelshot_path),
		"/levelshots/%s", mservers[serverindex].fullMapName
	);
	
	s_servers[serverindex].serverinfo_submenu.generic.type = MTYPE_SUBMENU;
	s_servers[serverindex].serverinfo_submenu.bordertexture = "menu/sm_";
	s_servers[serverindex].serverinfo_submenu.nitems = 0;
	s_servers[serverindex].serverinfo_submenu.navagable = true;
	
	s_servers[serverindex].name.generic.type = MTYPE_TEXT;
	s_servers[serverindex].name.generic.flags = QMF_RIGHT_COLUMN;
	s_servers[serverindex].name.generic.name = mservers[serverindex].szHostName;
	Menu_AddItem (&s_servers[serverindex].serverinfo_submenu, &s_servers[serverindex].name);
	
	s_servers[serverindex].levelshot.generic.type = MTYPE_NOT_INTERACTIVE;
	s_servers[serverindex].levelshot.generic.localstrings[0] = s_servers[serverindex].levelshot_path;
	// pretty close to 16:9
	VectorSet (s_servers[serverindex].levelshot.generic.localints, 21, 12, 0); 
	s_servers[serverindex].levelshot.generic.itemsizecallback = PicSizeFunc;
	s_servers[serverindex].levelshot.generic.itemdraw = PicDrawFunc;
	Menu_AddItem (&s_servers[serverindex].serverinfo_submenu, &s_servers[serverindex].levelshot);
	
	s_servers[serverindex].serverinfo_table.generic.type = MTYPE_SUBMENU;
	s_servers[serverindex].serverinfo_table.nitems = 0;
	
	s_servers[serverindex].serverinfo_columns[0][0].generic.type		= MTYPE_TEXT;
	s_servers[serverindex].serverinfo_columns[0][1].generic.type		= MTYPE_TEXT;
	s_servers[serverindex].serverinfo_columns[0][1].generic.flags	= QMF_RIGHT_COLUMN;
	
	Menu_MakeTable (&s_servers[serverindex].serverinfo_table, rows, 2, sizes, s_servers[serverindex].serverinfo_rows, s_servers[serverindex].serverinfo_rows, s_servers[serverindex].serverinfo_columns, contents);
	
	Menu_AddItem (&s_servers[serverindex].serverinfo_submenu, &s_servers[serverindex].serverinfo_table);
	
	s_servers[serverindex].serverinfo_rows[7].generic.type = MTYPE_SUBMENU;
	s_servers[serverindex].serverinfo_rows[7].horizontal = true;
	s_servers[serverindex].serverinfo_rows[7].navagable = true;
	s_servers[serverindex].serverinfo_rows[7].nitems = 0;
	
	LINK (s_servers[serverindex].serverinfo_rows[0].lwidth, s_servers[serverindex].serverinfo_rows[7].lwidth);
	LINK (s_servers[serverindex].serverinfo_rows[0].rwidth, s_servers[serverindex].serverinfo_rows[7].rwidth);
	
	s_servers[serverindex].serverinfo_columns[7][0].generic.type = MTYPE_TEXT;
	s_servers[serverindex].serverinfo_columns[7][0].generic.name = contents[rows*2+0];
	LINK (s_servers[serverindex].serverinfo_columns[0][0].generic.x, s_servers[serverindex].serverinfo_columns[7][0].generic.x);
	Menu_AddItem (&s_servers[serverindex].serverinfo_rows[7], &s_servers[serverindex].serverinfo_columns[7][0]);
	
	ModList_SubmenuInit ();
	LINK (s_servers[serverindex].serverinfo_columns[0][1].generic.x, s_servers[serverindex].modlist_submenu.generic.x);
	Menu_AddItem (&s_servers[serverindex].serverinfo_rows[7], &s_servers[serverindex].modlist_submenu);
	
	// don't add it to serverinfo_table because serverinfo_table isn't navagable
	if (s_servers[serverindex].modlist_submenu.nitems != 0)
		Menu_AddItem (&s_servers[serverindex].serverinfo_submenu, &s_servers[serverindex].serverinfo_rows[7]);
	
	Menu_AddItem (&s_servers[serverindex].menu, &s_servers[serverindex].serverinfo_submenu);
}

static void PlayerList_SubmenuInit (void)
{
	int i, j;
	//qboolean is_team_server = false;

	char *local_player_info_ptrs[MAX_PLAYERS*SVDATA_PLAYERINFO];
	size_t sizes[SVDATA_PLAYERINFO]
		= {sizeof(menutxt_s), sizeof(menutxt_s), sizeof(menutxt_s), sizeof(menutxt_s)};
	
	if (mservers[serverindex].players == 0)
		return;

	//
	//for ( i = 0; i < MAX_SERVER_MODS; i++ )
	//{
	//	if(!stricmp("ctf", s_servers[serverindex].modnames[i]) || !stricmp("g_tactical", s_servers[serverindex].modnames[i]))
	//		is_team_server = true;
	//}
	
	s_servers[serverindex].playerlist_submenu.generic.type = MTYPE_SUBMENU;
	s_servers[serverindex].playerlist_submenu.navagable = true;
	s_servers[serverindex].playerlist_submenu.nitems = 0;
	
	Menu_AddItem (&s_servers[serverindex].menu, &s_servers[serverindex].playerlist_submenu);
	
	s_servers[serverindex].playerlist_label.generic.type = MTYPE_TEXT;
	s_servers[serverindex].playerlist_label.generic.flags = QMF_RIGHT_COLUMN;
	s_servers[serverindex].playerlist_label.generic.name = "Players:";
	Menu_AddItem (&s_servers[serverindex].playerlist_submenu, &s_servers[serverindex].playerlist_label);
	
	s_servers[serverindex].playerlist_scrollingmenu.generic.type = MTYPE_SUBMENU;
	s_servers[serverindex].playerlist_scrollingmenu.navagable = true;
	s_servers[serverindex].playerlist_scrollingmenu.bordertexture = "menu/sm_";
	s_servers[serverindex].playerlist_scrollingmenu.nitems = 0;
	
	s_servers[serverindex].playerlist_header.generic.type = MTYPE_SUBMENU;
	s_servers[serverindex].playerlist_header.horizontal = true;
	s_servers[serverindex].playerlist_header.nitems = 0;
	
	s_servers[serverindex].playerlist_header_columns[SVDATA_PLAYERINFO_NAME].generic.name	= "^7Name";
	s_servers[serverindex].playerlist_header_columns[SVDATA_PLAYERINFO_SCORE].generic.name	= "^7Score";
	s_servers[serverindex].playerlist_header_columns[SVDATA_PLAYERINFO_PING].generic.name	= "^7Ping";
	s_servers[serverindex].playerlist_header_columns[SVDATA_PLAYERINFO_TEAM].generic.name	= "^7Team";

	for (i = 0; i < SVDATA_PLAYERINFO; i++)
	{
		s_servers[serverindex].playerlist_header_columns[i].generic.type			= MTYPE_TEXT;
		if (i > 0)
			s_servers[serverindex].playerlist_header_columns[i].generic.flags	= QMF_RIGHT_COLUMN;
		Menu_AddItem (&s_servers[serverindex].playerlist_header, &s_servers[serverindex].playerlist_header_columns[i]);
	}
	
	Menu_AddItem (&s_servers[serverindex].playerlist_submenu, &s_servers[serverindex].playerlist_header);
	
	for (i = 0; i < mservers[serverindex].players; i++)
	{
		int ranking = mservers[serverindex].playerRankings[i];
		if (ranking == 1000)
			Com_sprintf(s_servers[serverindex].ranktxt[i], sizeof(s_servers[serverindex].ranktxt[i]), "Player is unranked");
		else
			Com_sprintf(s_servers[serverindex].ranktxt[i], sizeof(s_servers[serverindex].ranktxt[i]), "Player is ranked %i", ranking);
		s_servers[serverindex].playerlist_rows[i].generic.tooltip = s_servers[serverindex].ranktxt[i];
		
		for (j = 0; j < SVDATA_PLAYERINFO; j++)
			local_player_info_ptrs[i*SVDATA_PLAYERINFO+j] = &mservers[serverindex].playerInfo[i][j][0];
	}
	
	Menu_MakeTable	(	&s_servers[serverindex].playerlist_scrollingmenu,
						mservers[serverindex].players, SVDATA_PLAYERINFO,
						sizes, &s_servers[serverindex].playerlist_header,
						s_servers[serverindex].playerlist_rows, s_servers[serverindex].playerlist_columns,
						local_player_info_ptrs
					);
	
	Menu_AddItem (&s_servers[serverindex].playerlist_submenu, &s_servers[serverindex].playerlist_scrollingmenu);
	
	s_servers[serverindex].playerlist_scrollingmenu.maxlines = 7;
	
	s_servers[serverindex].playerlist_scrollingmenu.nitems = mservers[serverindex].players;
	s_servers[serverindex].playerlist_scrollingmenu.yscroll = 0;
	
	LINK (s_servers[serverindex].serverinfo_submenu.rwidth, s_servers[serverindex].playerlist_scrollingmenu.rwidth);
	LINK (s_servers[serverindex].serverinfo_submenu.lwidth, s_servers[serverindex].playerlist_scrollingmenu.lwidth);
}

static void M_Menu_SelectedServer_f (void)
{
	setup_window (s_servers[serverindex].screen, s_servers[serverindex].menu, "SERVER");
	
	ServerInfo_SubmenuInit ();
	PlayerList_SubmenuInit ();
	
	// "connect" button at the bottom
	s_servers[serverindex].connect.generic.type = MTYPE_ACTION;
	s_servers[serverindex].connect.generic.flags = QMF_BUTTON | QMF_RIGHT_COLUMN;
	s_servers[serverindex].connect.generic.name = "Connect";
	s_servers[serverindex].connect.generic.callback = JoinServerFunc;
	Menu_AddItem (&s_servers[serverindex].menu, &s_servers[serverindex].connect);
	
	s_servers[serverindex].serverinfo_submenu.statusbar = NULL;
	s_servers[serverindex].connect.generic.statusbar = NULL;
			
	if (!PLAYER_NAME_UNIQUE)
		s_servers[serverindex].connect.generic.statusbar = "You must change your player name from the default before connecting!";
	else if (CL_ServerIsOutdated (mservers[serverindex].szVersion))
		s_servers[serverindex].serverinfo_submenu.statusbar = "Warning: server is ^1outdated!^7 It may have bugs or different gameplay.";
	else
		s_servers[serverindex].connect.generic.statusbar = "Hit ENTER or CLICK to connect";
	
	M_PushMenu_Defaults (s_servers[serverindex].screen);
	
	s_servers[serverindex].menu.default_cursor_selection = (menuitem_s *)&s_servers[serverindex].connect;
}

//TODO: Move this out of the menu section!
static qboolean M_ParseServerInfo (netadr_t adr, char *status_string, SERVERDATA *destserver)
{
	char *rLine;
	char *token;
	char skillLevel[24];
	char lasttoken[256];
	char seps[]   = "\\";
	int players = 0;
	int bots = 0;
	int result;
	
	char playername[PLAYERNAME_SIZE];
	char team[8];
	int score, ping, rankTotal, starttime;
	PLAYERSTATS     player;

	destserver->local_server_netadr = adr;
	// starttime now sourced per server.
	starttime = CL_GetPingStartTime(adr);
	if (starttime != 0)
		destserver->ping = Sys_Milliseconds() - starttime;
	else
	{
		// Local LAN?
		destserver->ping = 1;
	}
	if ( destserver->ping < 1 )
		destserver->ping = 1; /* for LAN and address book entries */

	//parse it

	result = strlen(status_string);

	//server info
	rLine = GetLine (&status_string, &result);

	/* Establish string and get the first token: */
	token = strtok( rLine, seps );
	if ( token != NULL )
	{
		Com_sprintf(lasttoken, sizeof(lasttoken), "%s", token);
		token = strtok( NULL, seps );
	}
	
	// HACK for backward compatibility
	memset (destserver->modInfo, 0, sizeof(destserver->modInfo));
	
	/* Loop through the rest of them */
	while( token != NULL ) 
	{
		/* While there are tokens in "string" */
		if (!Q_strcasecmp (lasttoken, "protocol"))
		{
			if(atoi(token) != PROTOCOL_VERSION)
				return false;
		}
		else if (!Q_strcasecmp (lasttoken, "admin"))
			Com_sprintf(destserver->szAdmin, sizeof(destserver->szAdmin), "%s", token);
		else if (!Q_strcasecmp (lasttoken, "website"))
			Com_sprintf(destserver->szWebsite, sizeof(destserver->szWebsite), "%s", token);
		else if (!Q_strcasecmp (lasttoken, "fraglimit"))
			Com_sprintf(destserver->fraglimit, sizeof(destserver->fraglimit), "%s", token);
		else if (!Q_strcasecmp (lasttoken, "timelimit"))
			Com_sprintf(destserver->timelimit, sizeof(destserver->timelimit), "%s", token);
		else if (!Q_strcasecmp (lasttoken, "version"))
			Com_sprintf(destserver->szVersion, sizeof(destserver->szVersion), "%s", token);
		else if (!Q_strcasecmp (lasttoken, "mapname"))
		{
			Com_sprintf(destserver->szMapName, sizeof(destserver->szMapName), "%s", token);
			Com_sprintf(destserver->fullMapName, sizeof(destserver->fullMapName), "%s", token);
		}
		else if (!Q_strcasecmp (lasttoken, "hostname"))
			Com_sprintf(destserver->szHostName, sizeof(destserver->szHostName), "%s", token);
		else if (!Q_strcasecmp (lasttoken, "maxclients"))
			Com_sprintf(destserver->maxClients, sizeof(destserver->maxClients), "%s", token);
		else if (!Q_strcasecmp (lasttoken, "mods"))
			Com_sprintf(destserver->modInfo, sizeof(destserver->modInfo), "%s", token);
		else if (!Q_strcasecmp (lasttoken, "sv_joustmode"))
			destserver->joust = atoi(token);
		else if (!Q_strcasecmp (lasttoken, "sv_tickrate"))
			server_tickrate = atoi(token);

		/* Get next token: */
		Com_sprintf(lasttoken, sizeof(lasttoken), "%s", token);
		token = strtok( NULL, seps );
	}	

	// playerinfo
	rankTotal = 0;
	strcpy (seps, " ");
	while ((rLine = GetLine (&status_string, &result)) && players < MAX_PLAYERS) 
	{
		/* Establish string and get the first token: */
		token = strtok( rLine, seps);
		score = atoi(token);

		token = strtok( NULL, seps);
		ping = atoi(token);

		token = strtok( NULL, "\"");

		if (token)
			strncpy (playername, token, sizeof(playername)-1);
		else
			playername[0] = '\0';

		playername[sizeof(playername)-1] = '\0';

		token = strtok( NULL, "\"");
		token = strtok( NULL, "\"");
		token = strtok( NULL, seps);
		if (token)
		{
			if(atoi(token) == 0)
				strcpy(team, "red");
			else if(atoi(token) == 1)
				strcpy(team, "blue");
			else if(atoi(token) == 3)
				strcpy(team, "alien");
			else if(atoi(token) == 4)
				strcpy(team, "human");
			else
				strcpy(team, "none");
		}
		else
			strcpy(team, "unknown");

		//get ranking
		Q_strncpyz2( player.playername, playername, sizeof(player.playername));
		player.totalfrags = player.totaltime = player.ranking = 0;
		player = getPlayerRanking ( player );

		Com_sprintf	(	destserver->playerInfo[players][SVDATA_PLAYERINFO_NAME],
						SVDATA_PLAYERINFO_COLSIZE,
						"%s", playername
					);
		Com_sprintf	(	destserver->playerInfo[players][SVDATA_PLAYERINFO_SCORE],
						SVDATA_PLAYERINFO_COLSIZE,
						"%i", score
					);
		Com_sprintf	(	destserver->playerInfo[players][SVDATA_PLAYERINFO_PING],
						SVDATA_PLAYERINFO_COLSIZE,
						"%i", ping
					);
		Com_sprintf	(	destserver->playerInfo[players][SVDATA_PLAYERINFO_TEAM],
						SVDATA_PLAYERINFO_COLSIZE,
						"%s", team
					);
		destserver->playerRankings[players] = player.ranking;

		rankTotal += player.ranking;

		players++;

		if(ping == 0)
			bots++;
	}

	if(players) 
	{
		if(thisPlayer.ranking < (rankTotal/players) - 100)
			strcpy(skillLevel, "Your Skill is ^1Higher");
		else if(thisPlayer.ranking > (rankTotal/players + 100))
			strcpy(skillLevel, "Your Skill is ^4Lower");
		else
			strcpy(skillLevel, "Your Skill is ^3Even");

		Com_sprintf(destserver->skill, sizeof(destserver->skill), "%s", skillLevel);
	}
	else
		Com_sprintf(destserver->skill, sizeof(destserver->skill), "Unknown");

	destserver->players = players;

	//build the string for the server (hostname - address - mapname - players/maxClients)
	if(strlen(destserver->maxClients) > 2)
		strcpy(destserver->maxClients, "??");
	
	Com_sprintf (destserver->szPlayers, sizeof(destserver->szPlayers), "%i(%i)/%s", min(99,players), min(99,bots), destserver->maxClients);
	Com_sprintf (destserver->szPing, sizeof(destserver->szPing), "%i", min(9999,destserver->ping));

	return true;
}

static menuframework_s	s_serverbrowser_screen;

static menuframework_s	s_joinserver_menu;

static menuframework_s	s_joinserver_header;

static menuframework_s	s_serverlist_submenu;
static menuframework_s	s_serverlist_header;
static menulist_s		s_serverlist_header_columns[SERVER_LIST_COLUMNS];
static menuframework_s	s_serverlist_rows[MAX_LOCAL_SERVERS];
static menutxt_s		s_serverlist_columns[MAX_LOCAL_SERVERS][SERVER_LIST_COLUMNS];

void M_AddToServerList (netadr_t adr, char *status_string)
{
	//if by some chance this gets called without the menu being up, return
	if(cls.key_dest != key_menu)
		return;

	if (m_num_servers == MAX_LOCAL_SERVERS)
		return;
	
	if(M_ParseServerInfo (adr, status_string, &mservers[m_num_servers]))
	{
	
		CON_Clear();
	
		m_num_servers++;
	}
}

void M_UpdateConnectedServerInfo (netadr_t adr, char *status_string)
{
	M_ParseServerInfo (adr, status_string, &connectedserver);
	remoteserver_jousting = connectedserver.joust;
}

static void DeselectServer (void)
{
	serverindex = -1;
}

static void SelectServer (int index)
{
	// used if the player hits enter without his mouse over the server list	
	serverindex = index;
	
	M_Menu_SelectedServer_f ();
}

//join on double click, return info on single click - to do - might consider putting player info in a tooltip on single click/right click
static void ClickServerFunc (void *self)
{
	int		index = ( menuframework_s * ) self - s_serverlist_rows;

	if(serverindex != index)
	{
		SelectServer (index);
		if (cursor.buttonclicks[MOUSEBUTTON1] != 2)
			return;
	}
	
	JoinServerFunc (NULL);
}

static void AddressBookFunc (UNUSED void *self)
{
	M_Menu_AddressBook_f();
}

static void PlayerRankingFunc (UNUSED void *self)
{
	M_Menu_PlayerRanking_f();
}

static void SearchLocalGames (void)
{
	m_num_servers = 0;
	DeselectServer ();
	s_serverlist_submenu.nitems = 0;
	s_serverlist_submenu.yscroll = 0;
	
	Draw_Fill (0, 0, viddef.width, viddef.height, RGBA (0, 0, 0, 0.85));
	SCR_CenterPrint ("Fetching server list...");
	SCR_DrawCenterString ();
	R_EndFrame ();

	// send out info packets
	CL_PingServers_f();
	
	CON_Clear();
	
	Com_Printf (" Got %d servers- stragglers may follow.\n", m_num_servers);
}

static void SearchLocalGamesFunc (UNUSED void *self)
{
	SearchLocalGames();
}

static qboolean QSortReverse;
static int QSortColumn;

static int SortServerList_Compare (const void *_a, const void *_b)
{
	int ret = 0;
	const menuframework_s *a, *b;
	const char *a_s, *b_s;
	
	a = *(menuframework_s **)_a;
	b = *(menuframework_s **)_b;
	
	a_s = ((menutxt_s *)(a->items[QSortColumn]))->generic.name;
	b_s = ((menutxt_s *)(b->items[QSortColumn]))->generic.name;
	
	if (QSortColumn > 1)
	{
		// do numeric sort for player count and ping
		if (atoi (a_s) > atoi (b_s))
			ret = 1;
		else if (atoi (a_s) < atoi (b_s))
			ret = -1;
	}
	else
		// because strcmp doesn't handle ^colors
		while (*a_s && *b_s)
		{
			if (*a_s == '^')
			{
				a_s++;
			}
			else if (*b_s == '^')
			{
				b_s++;
			}
			else if (tolower(*a_s) > tolower(*b_s))
			{
				ret = 1;
				break;
			}
			else if (tolower(*a_s) < tolower(*b_s))
			{
				ret = -1;
				break;
			}
			a_s++;
			b_s++;
		}
	
	if (QSortReverse)
		return -ret;
	return ret;
}

static void SortServerList_Func ( void *_self )
{
	int column_num, i;
	menulist_s *self = (menulist_s *)_self;
	
	column_num = self-s_serverlist_header_columns;
	
	for (i = 0; i < SERVER_LIST_COLUMNS; i++)
		if (i != column_num)
			s_serverlist_header_columns[i].curvalue = 0;
	
	if (self->curvalue == 0)
	{
		if (column_num == 3)
		{
			self->curvalue = 1;
		}
		else
		{
			s_serverlist_header_columns[3].curvalue = 1;
			SortServerList_Func (&s_serverlist_header_columns[3]);
			return;
		}
	}
	
	QSortColumn = column_num;
	QSortReverse = self->curvalue == 2;
	
	qsort (s_serverlist_submenu.items, s_serverlist_submenu.nitems, sizeof (void*), SortServerList_Compare);
	s_serverlist_submenu.yscroll = 0;
}

static void ServerList_SubmenuInit (void)
{
	int i, j;
	
	s_serverlist_submenu.generic.type = MTYPE_SUBMENU;
	s_serverlist_submenu.generic.flags = QMF_SUBMENU_CAPTURE;
	s_serverlist_submenu.navagable = true;
	s_serverlist_submenu.nitems = 0;
	s_serverlist_submenu.bordertexture = "menu/sm_";
	
	s_serverlist_header.generic.type = MTYPE_SUBMENU;
	s_serverlist_header.horizontal = true;
	s_serverlist_header.navagable = true;
	s_serverlist_header.nitems = 0;
	
	s_serverlist_header_columns[0].generic.name = "^3Server";
	s_serverlist_header_columns[1].generic.name = "^3Map";
	s_serverlist_header_columns[2].generic.name = "^3Players";
	s_serverlist_header_columns[3].generic.name = "^3Ping";
	
	for (j = 0; j < SERVER_LIST_COLUMNS; j++)
	{
		s_serverlist_header_columns[j].generic.type = MTYPE_SPINCONTROL;
		s_serverlist_header_columns[j].generic.flags = QMF_RIGHT_COLUMN|QMF_ALLOW_WRAP;
		s_serverlist_header_columns[j].itemnames = updown_names;
		s_serverlist_header_columns[j].generic.itemsizecallback = IconSpinSizeFunc;
		s_serverlist_header_columns[j].generic.itemdraw = IconSpinDrawFunc;
		s_serverlist_header_columns[j].curvalue = 0;
		s_serverlist_header_columns[j].generic.callback = SortServerList_Func;
		Menu_AddItem (&s_serverlist_header, &s_serverlist_header_columns[j]);
	}
	s_serverlist_header_columns[3].curvalue = 1;
	
	Menu_AddItem (&s_joinserver_menu, &s_serverlist_header);
	
	for ( i = 0; i < MAX_LOCAL_SERVERS; i++ )
	{
		s_serverlist_rows[i].generic.type	= MTYPE_SUBMENU;
		s_serverlist_rows[i].generic.callback = ClickServerFunc;
		s_serverlist_rows[i].nitems = 0;
		s_serverlist_rows[i].horizontal = true;
		s_serverlist_rows[i].enable_highlight = true;
		
		s_serverlist_columns[i][0].generic.name = mservers[i].szHostName;
		s_serverlist_columns[i][1].generic.name = mservers[i].szMapName;
		s_serverlist_columns[i][2].generic.name = mservers[i].szPlayers;
		s_serverlist_columns[i][3].generic.name = mservers[i].szPing;
		
		for (j = 0; j < SERVER_LIST_COLUMNS; j++)
		{
			s_serverlist_columns[i][j].generic.type = MTYPE_TEXT;
			s_serverlist_columns[i][j].generic.flags = QMF_RIGHT_COLUMN;
			LINK(s_serverlist_header_columns[j].generic.x, s_serverlist_columns[i][j].generic.x);
			Menu_AddItem (&s_serverlist_rows[i], &s_serverlist_columns[i][j]);
		}
		
		LINK(s_serverlist_header.lwidth, s_serverlist_rows[i].lwidth);
		LINK(s_serverlist_header.rwidth, s_serverlist_rows[i].rwidth);
		
		Menu_AddItem( &s_serverlist_submenu, &s_serverlist_rows[i] );
	}
	
	Menu_AddItem (&s_joinserver_menu, &s_serverlist_submenu);
	
	s_serverlist_submenu.maxlines = 25;
	
}

static void ServerListHeader_SubmenuInit (void)
{
	s_joinserver_header.generic.type = MTYPE_SUBMENU;
	s_joinserver_header.nitems = 0;
	s_joinserver_header.horizontal = true;
	s_joinserver_header.navagable = true;
	
	// doesn't actually do anything yet
	// add_action (s_joinserver_header, "Address Book", AddressBookFunc, 0);
	add_action (s_joinserver_header, "Refresh", SearchLocalGamesFunc, 0);
	
	if (STATS_ENABLED) {
		add_action (s_joinserver_header, "Rank/Stats", PlayerRankingFunc, 0);
	}

	Menu_AddItem (&s_joinserver_menu, &s_joinserver_header);
}

static void M_Menu_JoinServer_f (void)
{
	extern cvar_t *name;

	static qboolean gotServers = false;

	if(!gotServers && STATS_ENABLED)
	{
		STATS_getStatsDB();
	}
	
	ValidatePlayerName( name->string, (strlen(name->string)+1) );

	if (STATS_ENABLED) {
		Q_strncpyz2( thisPlayer.playername, name->string, sizeof(thisPlayer.playername) );
		thisPlayer.totalfrags = thisPlayer.totaltime = thisPlayer.ranking = 0;
		thisPlayer = getPlayerRanking ( thisPlayer );
	}

	serverindex = -1;

	if (!gotServers)
	{
		setup_window (s_serverbrowser_screen, s_joinserver_menu, "SERVER LIST");
	
		ServerListHeader_SubmenuInit ();
		ServerList_SubmenuInit ();
		
		SearchLocalGames();
		
		s_joinserver_menu.default_cursor_selection = (menuitem_s *)&s_serverlist_submenu;
	}
	
	gotServers = true;
	
	M_PushMenu_Defaults (s_serverbrowser_screen);
}

/*
=============================================================================

MUTATORS MENU

=============================================================================
*/
static menuframework_s s_mutators_screen;
static menuframework_s s_mutators_menu;

// weapon modes are different from regular mutators in that they cannot be
// combined
static const char *weaponModeNames[][2] = 
{
	{"instagib",		"instagib"},
	{"rocket arena",	"rocket_arena"},
	{"insta/rockets",	"insta_rockets"},
	{"excessive",		"excessive"},
	{"class based",		"classbased"},
	{"all out assault", "all_out_assault"}
};
#define num_weapon_modes static_array_size(weaponModeNames)
static menulist_s s_weaponmode_list[num_weapon_modes];

static const char *mutatorNames[][2] = 
{
	{"vampire",			"vampire"},
	{"regen",			"regeneration"},
	{"quick weapons",	"quickweap"},
	{"anticamp",		"anticamp"},
	{"speed",			"playerspeed"},
	{"low gravity",		"low_grav"},
	{"jousting",		"sv_joustmode"},
	{"grapple hook",	"grapple"}
};
#define num_mutators static_array_size(mutatorNames)
static menulist_s s_mutator_list[ num_mutators  ];
static menufield_s s_camptime;

static char dmflags_display_buffer[128];

static void DMFlagCallback( void *self )
{
	menulist_s *f = ( menulist_s * ) self;
	int flags;
	int bit;
	qboolean invert, enabled;

	flags = Cvar_VariableValue( "dmflags" );
	
	if (f != NULL)
	{
		bit = f->generic.localints[0];
		invert = f->generic.localints[1];
		enabled = f->curvalue != 0;
	
		if (invert != enabled)
			flags |= bit;
		else
			flags &= ~bit;
	}

	Cvar_SetValue ("dmflags", flags);

	Com_sprintf( dmflags_display_buffer, sizeof( dmflags_display_buffer ), "(dmflags = %d)", flags );
}

typedef struct {
	char		*display_name;
	qboolean	invert;
	int			bit;
} DMFlag_control_t;

static const DMFlag_control_t dmflag_control_names[] = {
	{"falling damage",		true,	DF_NO_FALLING},
	{"weapons stay",		false,	DF_WEAPONS_STAY},
	{"instant powerups",	false,	DF_INSTANT_ITEMS},
	{"allow powerups",		true,	DF_NO_ITEMS},
	{"allow health",		true,	DF_NO_HEALTH},
	{"allow armor",			true,	DF_NO_ARMOR},
	{"spawn farthest",		false,	DF_SPAWN_FARTHEST},
	{"same map",			false,	DF_SAME_LEVEL},
	{"force respawn",		false,	DF_FORCE_RESPAWN},
	{"team deathmatch",		false,	DF_SKINTEAMS},
	{"allow exit", 			false,	DF_ALLOW_EXIT},
	{"infinite ammo",		false,	DF_INFINITE_AMMO},
	{"quad drop",			false,	DF_QUAD_DROP},
	{"friendly fire",		true,	DF_NO_FRIENDLY_FIRE},
	{"bot chat",			false,	DF_BOTCHAT},
	{"bot perfect aim",		false,	DF_BOT_FUZZYAIM},
	{"auto node save",		false,	DF_BOT_AUTOSAVENODES},
	{"repeat level if "
	 "bot wins",			true,	DF_BOT_LEVELAD},
	{"bots in game",		true,	DF_BOTS}
};
#define num_dmflag_controls static_array_size(dmflag_control_names)

static menuframework_s	s_dmflags_submenu;
static menulist_s		s_dmflag_controls[num_dmflag_controls];

static void SetWeaponModeFunc (void *_self)
{
	menulist_s *self;
	int i, value;
	
	self = (menulist_s*)_self;
	
	value = self->curvalue;
	
	if (self->curvalue)
	{
		for (i = 0; i < num_weapon_modes; i++)
		{
			Cvar_SetValue (weaponModeNames[i][1], 0);
			s_weaponmode_list[i].curvalue = 0;
		}
	}
	
	Cvar_SetValue (self->generic.localstrings[0], value);
	self->curvalue = value;
}

static void M_Menu_Mutators_f (void)
{
	int i;
	
	int dmflags = Cvar_VariableValue( "dmflags" );
	
	setup_window (s_mutators_screen, s_mutators_menu, "MUTATORS");
	
	for (i = 0; i < num_weapon_modes; i++)
	{
		s_weaponmode_list[i].generic.name = weaponModeNames[i][0];
		s_weaponmode_list[i].generic.callback = SetWeaponModeFunc;
		s_weaponmode_list[i].generic.localstrings[0] = weaponModeNames[i][1];
		s_weaponmode_list[i].curvalue = Cvar_VariableValue (weaponModeNames[i][1]);
		setup_radiobutton (s_weaponmode_list[i]);
		Menu_AddItem (&s_mutators_menu, &s_weaponmode_list[i]);
	}
	
	s_camptime.generic.type = MTYPE_FIELD;
	s_camptime.generic.name = "camp time";
	s_camptime.generic.flags = QMF_NUMBERSONLY;
	s_camptime.generic.localstrings[0] = "camptime";
	s_camptime.length = 3;
	s_camptime.generic.visible_length = 3;
	strcpy( s_camptime.buffer, Cvar_VariableString("camptime") );
	s_camptime.generic.callback = IntFieldCallback;
	
	for (i = 0; i < num_mutators; i++)
	{
		s_mutator_list[i].generic.name = mutatorNames[i][0];
		s_mutator_list[i].generic.callback = SpinOptionFunc;
		s_mutator_list[i].generic.localstrings[0] = mutatorNames[i][1];
		s_mutator_list[i].curvalue = Cvar_VariableValue (mutatorNames[i][1]);
		setup_tickbox (s_mutator_list[i]);
		Menu_AddItem (&s_mutators_menu, &s_mutator_list[i]);
		
		// camptime goes after anticamp control-- we put this here so we can
		// insert it in the right place in the menu
		if (!strcmp (mutatorNames[i][0], "anticamp"))
			Menu_AddItem( &s_mutators_menu, &s_camptime );
	}
	
	add_text (s_mutators_menu, dmflags_display_buffer, 0);
	
	s_dmflags_submenu.generic.type = MTYPE_SUBMENU;
	s_dmflags_submenu.generic.flags = QMF_SNUG_LEFT | QMF_SUBMENU_CAPTURE;
	s_dmflags_submenu.navagable = true;
	s_dmflags_submenu.bordertexture = "menu/sm_";
	s_dmflags_submenu.nitems = 0;
	s_dmflags_submenu.maxlines = 15;
	for (i = 0; i < num_dmflag_controls; i++)
	{
		s_dmflag_controls[i].generic.name = dmflag_control_names[i].display_name;
		s_dmflag_controls[i].generic.callback = DMFlagCallback;
		setup_tickbox (s_dmflag_controls[i]);
		s_dmflag_controls[i].generic.localints[0] = dmflag_control_names[i].bit;
		s_dmflag_controls[i].generic.localints[1] = dmflag_control_names[i].invert;
		s_dmflag_controls[i].curvalue = (dmflags & dmflag_control_names[i].bit) != 0;
		if (dmflag_control_names[i].invert)
		{
			s_dmflag_controls[i].curvalue = s_dmflag_controls[i].curvalue == 0;
		}
		
		Menu_AddItem (&s_dmflags_submenu, &s_dmflag_controls[i]);
	}
	
	Menu_AddItem (&s_mutators_menu, &s_dmflags_submenu);
	
	// initialize the dmflags display buffer
	DMFlagCallback( 0 );
	
	M_PushMenu_Defaults (s_mutators_screen);
}

/*
=============================================================================

ADD BOTS MENU

=============================================================================
*/

// For going from weapon pickup name to weapon icon. Used for displaying icon
// previews of the bots' favorite weapons.
static char *weapon_icon_names[][2] =
{	
	{"Grapple",			"grapple"},
	{"Blaster",			"blaster"},
	{"Violator",		"violator"},
	{"Alien Smartgun",	"smartgun"},
	{"Chaingun",		"chaingun"},
	{"Flame Thrower",	"flamethrower"},
	{"Rocket Launcher",	"rocketlauncher"},
	{"Alien Disruptor",	"disruptor"},
	{"Disruptor",		"beamgun"},
	{"Alien Vaporizer",	"vaporizer"} // note the different spellings
};
#define num_weapon_icons static_array_size(weapon_icon_names)

static menuframework_s	s_addbots_screen;
static menuframework_s	s_addbots_menu;
static menuframework_s	s_addbots_header;
static menutxt_s		s_addbots_name_label;
static menutxt_s		s_addbots_skill_label;
static menutxt_s		s_addbots_faveweap_label;

int totalbots;

#define MAX_BOTS 16
struct botdata {
	char	name[32];
	char	model[64];
	char	userinfo[MAX_INFO_STRING];
	char	faveweap[64];
	int		skill;
	
	// menu entities
	menuframework_s	row;
	menuaction_s	action;
	char			skill_buf[2];
	menutxt_s		m_skill;
	menutxt_s		m_faveweap;
} bots[MAX_BOTS];

static menulist_s		s_startmap_list;
static menulist_s		s_rules_box;
static menulist_s   	s_bots_bot_action[8];
#define MAX_MAPS 256
static char *mapnames[MAX_MAPS + 2];

struct botinfo {
	char name[32];
	char userinfo[MAX_INFO_STRING];
} bot[8];

int slot;

static void LoadBotInfo (void)
{
	FILE *pIn;
	int i, count;
	char *name;
	char *skin;

	char fullpath[MAX_OSPATH];

	if ( !FS_FullPath( fullpath, sizeof(fullpath), "botinfo/allbots.tmp" ) )
	{
		Com_DPrintf("LoadBotInfo: %s/allbots.tmp not found\n", "botinfo" );
		return;
	}
	if( (pIn = fopen( fullpath, "rb" )) == NULL )
	{
		Com_DPrintf("LoadBotInfo: failed file open: %s\n", fullpath );
		return;
	}

	szr = fread(&count,sizeof (int),1,pIn);
	if(count>MAX_BOTS)
		count = MAX_BOTS;

	for(i=0;i<count;i++)
	{
		char *cfg, *s;
		char cfgpath[MAX_QPATH];
		const char *delim = "\r\n";
		
		szr = fread(bots[i].userinfo,sizeof(char) * MAX_INFO_STRING,1,pIn);

		name = Info_ValueForKey (bots[i].userinfo, "name");
		skin = Info_ValueForKey (bots[i].userinfo, "skin");
		strncpy(bots[i].name, name, sizeof(bots[i].name)-1);
		Com_sprintf (bots[i].model, sizeof(bots[i].model), "bots/%s_i", skin);
		
		// defaults for .cfg data
		bots[i].skill = 1; //medium
		strcpy (bots[i].faveweap, "None");
		Com_sprintf (bots[i].skill_buf, sizeof(bots[i].skill_buf), "%d", bots[i].skill);
		
		// load info from config file if possible
		
		Com_sprintf (cfgpath, sizeof(cfgpath), "%s/%s.cfg", "botinfo", name);
		if( FS_LoadFile (cfgpath, (void**)&cfg) == -1 )
		{
			Com_DPrintf("LoadBotInfo: failed file open: %s\n", fullpath );
			continue;
		}
		
		if ( (s = strtok( cfg, delim )) != NULL )
			bots[i].skill = atoi( s );
		if ( bots[i].skill < 0 )
			bots[i].skill = 0;
		
		Com_sprintf (bots[i].skill_buf, sizeof(bots[i].skill_buf), "%d", bots[i].skill);
		
		if ( s && ((s = strtok( NULL, delim )) != NULL) )
			strncpy( bots[i].faveweap, s, sizeof(bots[i].faveweap)-1 );
		
		FS_FreeFile(cfg);
	}
	totalbots = count;
	fclose(pIn);
}

static void AddbotFunc (void *self)
{
	int i, count;
	char startmap[MAX_QPATH];
	char bot_filename[MAX_OSPATH];
	FILE *pOut;
	menuframework_s *f = ( menuframework_s * ) self;

	//get the name and copy that config string into the proper slot name
	for(i = 0; i < totalbots; i++)
	{
		if (f == &bots[i].row)
		{ //this is our selected bot
			strcpy(bot[slot].name, bots[i].name);
			strcpy(bot[slot].userinfo, bots[i].userinfo);
			s_bots_bot_action[slot].generic.name = bots[i].name;
		}
	}

	//save off bot file
	count = 8;
	for(i = 0; i < 8; i++)
	{
		if(!strcmp(bot[i].name, "...empty slot"))
			count--;
	}
	strcpy( startmap, strchr( mapnames[s_startmap_list.curvalue], '\n' ) + 1 );
	for(i = 0; i < strlen(startmap); i++)
		startmap[i] = tolower(startmap[i]);


	if( is_team_game( s_rules_box.curvalue ))

	{ // team game
		FS_FullWritePath( bot_filename, sizeof(bot_filename), "botinfo/team.tmp" );
	}
	else
	{ // non-team, bots per map
		char relative_path[MAX_QPATH];
		Com_sprintf( relative_path, sizeof(relative_path), "botinfo/%s.tmp", startmap );
		FS_FullWritePath( bot_filename, sizeof(bot_filename), relative_path );
	}

	if((pOut = fopen(bot_filename, "wb" )) == NULL)
	{
		Com_DPrintf("AddbotFunc: failed fopen for write: %s\n", bot_filename );
		return; // bail
	}

	szr = fwrite(&count,sizeof (int),1,pOut); // Write number of bots

	for (i = 7; i > -1; i--) {
		if(strcmp(bot[i].name, "...empty slot"))
			szr = fwrite(bot[i].userinfo,sizeof (char) * MAX_INFO_STRING,1,pOut);
	}

	fclose(pOut);

	//kick back to previous menu
	M_PopMenu();

}

static void M_Menu_AddBots_f (void)
{
	int i, j;

	totalbots = 0;

	LoadBotInfo();

	setup_window (s_addbots_screen, s_addbots_menu, "CHOOSE A BOT");
	s_addbots_menu.maxlines = 16;
	
	s_addbots_header.generic.type = MTYPE_SUBMENU;
	s_addbots_header.horizontal = true;
	s_addbots_header.nitems = 0;
	
	s_addbots_name_label.generic.type = MTYPE_TEXT;
	s_addbots_name_label.generic.name = "^3bot";
	Menu_AddItem (&s_addbots_header, &s_addbots_name_label);
	
	s_addbots_skill_label.generic.type = MTYPE_TEXT;
	s_addbots_skill_label.generic.name = "^3skill";
	Menu_AddItem (&s_addbots_header, &s_addbots_skill_label);
	
	s_addbots_faveweap_label.generic.type = MTYPE_TEXT;
	s_addbots_faveweap_label.generic.flags = QMF_RIGHT_COLUMN;
	s_addbots_faveweap_label.generic.name = "^3favorite ^3weapon";
	Menu_AddItem (&s_addbots_header, &s_addbots_faveweap_label);
	
	Menu_AddItem (&s_addbots_menu, &s_addbots_header);

	for(i = 0; i < totalbots; i++) {
		bots[i].row.generic.type = MTYPE_SUBMENU;
		bots[i].row.generic.flags = QMF_SNUG_LEFT;
		bots[i].row.nitems = 0;
		bots[i].row.horizontal = true;
		bots[i].row.enable_highlight = true;
		
		bots[i].row.generic.callback = AddbotFunc;
	
		bots[i].action.generic.type	= MTYPE_ACTION;
		bots[i].action.generic.name	= bots[i].name;
		bots[i].action.generic.localstrings[0] = bots[i].model;
		VectorSet (bots[i].action.generic.localints, 2, 2, RCOLUMN_OFFSET);
		bots[i].action.generic.itemsizecallback = PicSizeFunc;
		bots[i].action.generic.itemdraw = PicDrawFunc;
		LINK(s_addbots_name_label.generic.x, bots[i].action.generic.x);
		Menu_AddItem (&bots[i].row, &bots[i].action);
		
		bots[i].m_skill.generic.type = MTYPE_TEXT;
		bots[i].m_skill.generic.name = bots[i].skill_buf;
		LINK(s_addbots_skill_label.generic.x, bots[i].m_skill.generic.x);
		Menu_AddItem (&bots[i].row, &bots[i].m_skill);
		
		bots[i].m_faveweap.generic.type = MTYPE_NOT_INTERACTIVE;
		bots[i].m_faveweap.generic.flags = QMF_RIGHT_COLUMN;
		// Start by assuming that we won't find a thumbnail image for the 
		// bot's favorite weapon, and set the widget up to simply show the
		// weapon's name.
		bots[i].m_faveweap.generic.itemsizecallback = NULL;
		bots[i].m_faveweap.generic.itemdraw = NULL;
		bots[i].m_faveweap.generic.name = bots[i].faveweap;
		for (j = 0; j < num_weapon_icons; j++)
		{
			if (!strcmp (bots[i].faveweap, weapon_icon_names[j][0]))
			{
				// We have found a matching thumbnail image, so disable the
				// display of text and instead show the image.
				bots[i].m_faveweap.generic.name = NULL;
				VectorSet (bots[i].m_faveweap.generic.localints, 4, 2, 0);
				bots[i].m_faveweap.generic.itemsizecallback = PicSizeFunc;
				bots[i].m_faveweap.generic.itemdraw = PicDrawFunc;
				bots[i].m_faveweap.generic.localstrings[0] = weapon_icon_names[j][1];
				break;
			}
		}
		LINK(s_addbots_faveweap_label.generic.x, bots[i].m_faveweap.generic.x);
		Menu_AddItem (&bots[i].row, &bots[i].m_faveweap);

		LINK(s_addbots_header.lwidth, bots[i].row.lwidth);
		LINK(s_addbots_header.rwidth, bots[i].row.rwidth);
		Menu_AddItem( &s_addbots_menu, &bots[i].row );
	}

	M_PushMenu_Defaults (s_addbots_screen);

}

/*
=============================================================================

START SERVER MENU

=============================================================================
*/


static menuframework_s s_startserver_screen;
static menuframework_s s_startserver_menu;
static menuframework_s s_startserver_main_submenu;
static int	  nummaps = 0;

static menufield_s	s_timelimit_field;
static menufield_s	s_fraglimit_field;
static menufield_s	s_maxclients_field;
static menufield_s	s_hostname_field;
static menulist_s	s_antilagprojectiles_box;
static menulist_s   s_public_box;
static menulist_s	s_dedicated_box;
static menulist_s   s_skill_box;

static menuframework_s	s_levelshot_submenu;
static menuitem_s		s_levelshot_preview;
static menutxt_s   		s_startserver_map_data[5];
static char			s_startserver_map_data_strings[5][128];

static void BotOptionsFunc (UNUSED void *self)
{
	M_Menu_BotOptions_f();
}

static void MutatorFunc (UNUSED void *self)
{
	M_Menu_Mutators_f();
}

static void MapInfoFunc (UNUSED void *self)
{
	FILE *desc_file;
	char line[500];
	char *pLine;
	char *rLine;
	int result;
	int i = 0;
	char seps[]   = "//";
	char *token;
	char startmap[128];
	char path[MAX_QPATH];
	static char levelshot[MAX_QPATH];

	//get a map description if it is there

	if(mapnames[0])
		strcpy( startmap, strchr( mapnames[s_startmap_list.curvalue], '\n' ) + 1 );
	else
		strcpy( startmap, "missing");

	Com_sprintf(path, sizeof(path), "levelshots/%s.txt", startmap);
	FS_FOpenFile(path, &desc_file);
	if (desc_file) {
		if(fgets(line, 500, desc_file))
		{
			pLine = line;

			result = strlen(line);

			rLine = GetLine (&pLine, &result);

			/* Establish string and get the first token: */
			token = strtok( rLine, seps );
			while (token != NULL && i < 5) {
				/* While there are tokens in "string" */
				Com_sprintf (s_startserver_map_data_strings[i], sizeof (s_startserver_map_data_strings[i]), "%s", token);
				/* Get next token: */
				token = strtok( NULL, seps );
				i++;
			}

		}

		fclose(desc_file);

	}
	
	// If no tokens were parsed, fill all the labels. Otherwise, only fill the
	// excess labels that weren't parsed.
	for (; i < 5; i++)
		Com_sprintf (s_startserver_map_data_strings[i], sizeof (s_startserver_map_data_strings[i]), "no data");
	
	Com_sprintf( levelshot, sizeof(levelshot), "/levelshots/%s_p", startmap );
	s_levelshot_preview.generic.localstrings[0] = levelshot;

}

static void RulesChangeFunc (UNUSED void *self) //this has been expanded to rebuild map list
{
	char *buffer;
	char  mapsname[1024];
	char *s;
	const char *buf_cursor;
	int length;
	int i, k;
	FILE *fp;
	char  shortname[MAX_TOKEN_CHARS];
	char  longname[MAX_TOKEN_CHARS];
	char  scratch[200];
	char *curMap;
	int nmaps = 0;
	int totalmaps;
	char **mapfiles;
	static char **bspnames;
	int		j, l;

	//clear out list first
	for ( i = 0; i < nummaps; i++ )
		free( mapnames[i] );

	nummaps = 0;

	/*
	** reload the list of map names, based on rules
	*/
	// maps.lst normally in "data1/"
	//  need  to add a function to FS_ if that is the only place it is allowed
	if ( !FS_FullPath( mapsname, sizeof( mapsname ), "maps.lst" ) )
	{
			Com_Error( ERR_DROP, "couldn't find maps.lst\n" );
		return; // for show, no maps.lst is fatal error
	}
	if ( ( fp = fopen( mapsname, "rb" ) ) == 0 )
	{
		Com_Error( ERR_DROP, "couldn't open maps.lst\n" );
		return; // for "show". above is fatal error.
	}

	length = FS_filelength( fp );
	buffer = malloc( length + 1 );
	szr = fread( buffer, length, 1, fp );
	buffer[length] = 0;

	i = 0;
	while ( i < length )
	{
		if ( buffer[i] == '\r' )
			nummaps++;
		i++;
	}
	totalmaps = nummaps;

	if ( nummaps == 0 )
	{
		fclose( fp );
		free( buffer );
		Com_Error( ERR_DROP, "no maps in maps.lst\n" );
		return; // for showing above is fatal.
	}

	memset( mapnames, 0, sizeof( char * ) * ( MAX_MAPS + 2 ) );

	bspnames = malloc( sizeof( char * ) * ( MAX_MAPS + 2 ) );  //was + 1, but caused memory errors
	memset( bspnames, 0, sizeof( char * ) * ( MAX_MAPS + 2 ) );

	buf_cursor = buffer;

	k = 0;
	for ( i = 0; i < nummaps; i++ )
	{

		strcpy( shortname, COM_Parse( &buf_cursor ) );
		l = strlen(shortname);
#if defined WIN32_VARIANT
		for (j=0 ; j<l ; j++)
			shortname[j] = tolower(shortname[j]);
#endif
		//keep a list of the shortnames for later comparison to bsp files
		bspnames[i] = malloc( strlen( shortname ) + 1 );
		strcpy(bspnames[i], shortname);

		strcpy( longname, COM_Parse( &buf_cursor ) );
		Com_sprintf( scratch, sizeof( scratch ), "%s\n%s", longname, shortname );
		
		// Each game mode has one or more map name prefixes. For example, if
		// the game mode is capture the flag, only maps that start with ctf
		// should make it into the mapnames list.
		for (j = 0; map_prefixes[s_rules_box.curvalue][j]; j++)
		{
			const char *curpfx = map_prefixes[s_rules_box.curvalue][j];
			if (!strncmp (curpfx, shortname, strlen(curpfx)))
			{
				// matched an allowable prefix
				mapnames[k] = malloc( strlen( scratch ) + 1 );
				strcpy( mapnames[k], scratch );
				k++;
				break;
			}
		}
	}
	// done with maps.lst
	fclose( fp );
	free( buffer );

	//now, check the folders and add the maps not in the list yet

	mapfiles = FS_ListFilesInFS( "maps/*.bsp", &nmaps, 0,
		SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

	for (i=0;i<nmaps && totalmaps<MAX_MAPS;i++)
	{
		int num;

		s = strstr( mapfiles[i], "maps/"); s++;
		s = strstr(s, "/"); s++;

		num = strlen(s)-4;
		s[num] = 0;

		curMap = s;

		l = strlen(curMap);

#if defined WIN32_VARIANT
		for (j=0 ; j<l ; j++)
			curMap[j] = tolower(curMap[j]);
#endif

		Com_sprintf( scratch, sizeof( scratch ), "%s\n%s", "Custom Map", curMap );

		//check game type, and if not already in maps.lst, add it
		l = 0;
		for ( j = 0 ; j < nummaps ; j++ )
		{
			l = Q_strcasecmp(curMap, bspnames[j]);
			if(!l)
				break; //already there, don't bother adding
		}
		if ( l )
		{ //didn't find it in our list
		
			// FIXME: copy and paste sux0rs
			// Each game mode has one or more map name prefixes. For example, if
			// the game mode is capture the flag, only maps that start with ctf
			// should make it into the mapnames list.
			for (j = 0; map_prefixes[s_rules_box.curvalue][j]; j++)
			{
				const char *curpfx = map_prefixes[s_rules_box.curvalue][j];
				if (!strncmp (curpfx, curMap, strlen(curpfx)))
				{
					// matched an allowable prefix
					mapnames[k] = malloc( strlen( scratch ) + 1 );
					strcpy( mapnames[k], scratch );
					k++;
					totalmaps++;
					break;
				}
			}
		}
	}

	if (mapfiles)
		FS_FreeFileList(mapfiles, nmaps);

	for(i = k; i<=nummaps; i++) 
	{
		free(mapnames[i]);
		mapnames[i] = 0;
	}

	s_startmap_list.generic.name	= "initial map";
	s_startmap_list.itemnames = (const char **)mapnames;
	s_startmap_list.curvalue = 0;

	//set map info
	MapInfoFunc(NULL);
}

static void StartServerActionFunc (UNUSED void *self)
{
	char	startmap[128];
	int		timelimit;
	int		fraglimit;
	int		maxclients;

	strcpy( startmap, strchr( mapnames[s_startmap_list.curvalue], '\n' ) + 1 );

	maxclients  = atoi( s_maxclients_field.buffer );
	timelimit	= atoi( s_timelimit_field.buffer );
	fraglimit	= atoi( s_fraglimit_field.buffer );

	Cvar_SetValue( "maxclients", ClampCvar( 0, maxclients, maxclients ) );
	Cvar_SetValue ("timelimit", ClampCvar( 0, timelimit, timelimit ) );
	Cvar_SetValue ("fraglimit", ClampCvar( 0, fraglimit, fraglimit ) );
	Cvar_Set("hostname", s_hostname_field.buffer );
	Cvar_SetValue("sv_public", s_public_box.curvalue );

// Running a dedicated server from menu does not always work right in Linux, if program is
//  invoked from a gui menu system. Listen server should be ok.
// Removing option from menu for now, since it is possible to start server running in
//  background without realizing it.
	if(s_dedicated_box.curvalue) {
#if defined WIN32_VARIANT
		Cvar_ForceSet("dedicated", "1");
#else
		Cvar_ForceSet("dedicated", "0");
#endif
		Cvar_Set("sv_maplist", startmap);
		Cbuf_AddText (sprintf("setmaster %s %s\n", DEFAULT_MASTER_1, DEFAULT_MASTER_2));
	}
	Cvar_SetValue( "skill", s_skill_box.curvalue );
	Cvar_SetValue( "g_antilagprojectiles", s_antilagprojectiles_box.curvalue);
	
	SetGameModeCvars (s_rules_box.curvalue);

	Cbuf_AddText (va("startmap %s\n", startmap));
	
	M_ForceMenuOff ();
}

static void M_Menu_StartServer_f (void)
{
	int i;

	
	static const char *skill[] =
	{
		"easy",
		"medium",
		"hard",
		0
	};
	
	setup_window (s_startserver_screen, s_startserver_menu, "HOST SERVER");
	setup_panel (s_startserver_menu, s_startserver_main_submenu);

	s_startmap_list.generic.type = MTYPE_SPINCONTROL;
	s_startmap_list.generic.name	= "initial map";
	s_startmap_list.itemnames = (const char **) mapnames;
	s_startmap_list.generic.callback = MapInfoFunc;
	Menu_AddItem( &s_startserver_main_submenu, &s_startmap_list );
	
	s_levelshot_submenu.generic.type = MTYPE_SUBMENU;
	s_levelshot_submenu.generic.flags = QMF_SNUG_LEFT;
	s_levelshot_submenu.nitems = 0;
	
	s_levelshot_preview.generic.type = MTYPE_NOT_INTERACTIVE;
	s_levelshot_preview.generic.localstrings[0] = NULL;
	VectorSet (s_levelshot_preview.generic.localints, 21, 12, 0);
	s_levelshot_preview.generic.itemsizecallback = PicSizeFunc;
	s_levelshot_preview.generic.itemdraw = PicDrawFunc;
	Menu_AddItem (&s_levelshot_submenu, &s_levelshot_preview);

	for ( i = 0; i < 5; i++) { 
		s_startserver_map_data[i].generic.type	= MTYPE_TEXT;
		s_startserver_map_data[i].generic.name	= s_startserver_map_data_strings[i];
		Com_sprintf (s_startserver_map_data_strings[i], sizeof (s_startserver_map_data_strings[i]), "no data");
		s_startserver_map_data[i].generic.flags	= QMF_RIGHT_COLUMN;
		Menu_AddItem( &s_levelshot_submenu, &s_startserver_map_data[i] );
	}
	
	Menu_AddItem (&s_startserver_main_submenu, &s_levelshot_submenu);
	
	add_text (s_startserver_main_submenu, NULL, 0); //spacer

	s_rules_box.generic.type = MTYPE_SPINCONTROL;
	s_rules_box.generic.name	= "rules";
	s_rules_box.itemnames = game_mode_names;
	s_rules_box.curvalue = 0;
	s_rules_box.generic.callback = RulesChangeFunc;
	Menu_AddItem( &s_startserver_main_submenu, &s_rules_box );
	
	s_antilagprojectiles_box.generic.name	= "antilag projectiles";
	setup_tickbox (s_antilagprojectiles_box);
	s_antilagprojectiles_box.curvalue = 1;
	Menu_AddItem( &s_startserver_main_submenu, &s_antilagprojectiles_box );

	s_timelimit_field.generic.type = MTYPE_FIELD;
	s_timelimit_field.generic.name = "time limit";
	s_timelimit_field.generic.flags = QMF_NUMBERSONLY;
	s_timelimit_field.generic.tooltip = "0 = no limit";
	s_timelimit_field.length = 3;
	s_timelimit_field.generic.visible_length = 3;
	strcpy( s_timelimit_field.buffer, Cvar_VariableString("timelimit") );
	Menu_AddItem( &s_startserver_main_submenu, &s_timelimit_field );

	s_fraglimit_field.generic.type = MTYPE_FIELD;
	s_fraglimit_field.generic.name = "frag limit";
	s_fraglimit_field.generic.flags = QMF_NUMBERSONLY;
	s_fraglimit_field.generic.tooltip = "0 = no limit";
	s_fraglimit_field.length = 3;
	s_fraglimit_field.generic.visible_length = 3;
	strcpy( s_fraglimit_field.buffer, Cvar_VariableString("fraglimit") );
	Menu_AddItem( &s_startserver_main_submenu, &s_fraglimit_field );

	/*
	** maxclients determines the maximum number of players that can join
	** the game.  If maxclients is only "1" then we should default the menu
	** option to 8 players, otherwise use whatever its current value is.
	** Clamping will be done when the server is actually started.
	*/
	s_maxclients_field.generic.type = MTYPE_FIELD;
	s_maxclients_field.generic.name = "max players";
	s_maxclients_field.generic.flags = QMF_NUMBERSONLY;
	s_maxclients_field.length = 3;
	s_maxclients_field.generic.visible_length = 3;
	if ( Cvar_VariableValue( "maxclients" ) == 1 )
		strcpy( s_maxclients_field.buffer, "8" );
	else
		strcpy( s_maxclients_field.buffer, Cvar_VariableString("maxclients") );
	Menu_AddItem( &s_startserver_main_submenu, &s_maxclients_field );

	s_hostname_field.generic.type = MTYPE_FIELD;
	s_hostname_field.generic.name = "server name";
	s_hostname_field.generic.flags = 0;
	s_hostname_field.length = 12;
	s_hostname_field.generic.visible_length = LONGINPUT_SIZE;
	strcpy( s_hostname_field.buffer, Cvar_VariableString("hostname") );
	Menu_AddItem( &s_startserver_main_submenu, &s_hostname_field );

	s_public_box.generic.name = "public server";
	setup_tickbox (s_public_box);
	s_public_box.curvalue = 1;
	Menu_AddItem( &s_startserver_main_submenu, &s_public_box );

#if defined WIN32_VARIANT
	s_dedicated_box.generic.name = "dedicated server";
	setup_tickbox (s_dedicated_box);
	Menu_AddItem( &s_startserver_main_submenu, &s_dedicated_box );
#else
	// may or may not need this when disabling dedicated server menu
	s_dedicated_box.generic.type = -1;
	s_dedicated_box.generic.name = NULL;
	s_dedicated_box.curvalue = 0;
#endif

	s_skill_box.generic.type = MTYPE_SPINCONTROL;
	s_skill_box.generic.name	= "skill level";
	s_skill_box.itemnames = skill;
	s_skill_box.curvalue = 1;
	Menu_AddItem( &s_startserver_main_submenu, &s_skill_box );
	

	add_action (s_startserver_menu, "Mutators", MutatorFunc, QMF_RIGHT_COLUMN);
	add_action (s_startserver_menu, "Bot Options", BotOptionsFunc, QMF_RIGHT_COLUMN);
	add_action (s_startserver_menu, "Begin", StartServerActionFunc, QMF_RIGHT_COLUMN);
	
	
	// call this now to set proper inital state
	RulesChangeFunc (NULL);
	MapInfoFunc (NULL);
	
	M_PushMenu_Defaults (s_startserver_screen);
}

/*
=============================================================================

BOT OPTIONS MENU

=============================================================================
*/

static menuframework_s s_botoptions_screen;
static menuframework_s s_botoptions_menu;

static void Read_Bot_Info ()
{
	FILE *pIn;
	int i, count;
	char *info;
	char bot_filename[MAX_OSPATH];
	char stem[MAX_QPATH];
	char relative_path[MAX_QPATH];

	if ( is_team_game( s_rules_box.curvalue ) )
	{
		strcpy( stem, "team" );
	}
	else
	{ // non-team, bots per map
		strcpy( stem, strchr( mapnames[s_startmap_list.curvalue], '\n' ) + 1 );
		for(i = 0; i < strlen(stem); i++)
			stem[i] = tolower( stem[i] );
	}
	Com_sprintf( relative_path, sizeof(relative_path), "botinfo/%s.tmp", stem );
	if ( !FS_FullPath( bot_filename, sizeof(bot_filename), relative_path ) )
	{
		Com_DPrintf("Read_Bot_Info: %s/%s not found\n", "botinfo", relative_path );
		return;
	}

	if((pIn = fopen(bot_filename, "rb" )) == NULL)
	{
		Com_DPrintf("Read_Bot_Info: failed file open for read: %s", bot_filename );
		return;
	}

	szr = fread(&count,sizeof (int),1,pIn);
	if(count>8)
		count = 8;

	for(i=0;i<count;i++)
	{

		szr = fread(bot[i].userinfo,sizeof(char) * MAX_INFO_STRING,1,pIn);

		info = Info_ValueForKey (bot[i].userinfo, "name");
		strcpy(bot[i].name, info);
	}

	fclose(pIn);
}

void BotAction (void *self);

static void M_Menu_BotOptions_f (void)
{
	int i;

	for(i = 0; i < 8; i++)
		strcpy(bot[i].name, "...empty slot");

	Read_Bot_Info();
	
	setup_window (s_botoptions_screen, s_botoptions_menu, "BOT OPTIONS");
	
	for (i = 0; i < 8; i++) {
		s_bots_bot_action[i].generic.type = MTYPE_ACTION;
		s_bots_bot_action[i].generic.name = bot[i].name;
		s_bots_bot_action[i].generic.flags = QMF_BUTTON;
		s_bots_bot_action[i].generic.callback = BotAction;
		s_bots_bot_action[i].curvalue = i;
		Menu_AddItem( &s_botoptions_menu, &s_bots_bot_action[i]);
	}

	M_PushMenu_Defaults (s_botoptions_screen);
}

void BotAction( void *self )
{
	FILE *pOut;
	int i, count;

	char stem[MAX_QPATH];
	char relative_path[MAX_QPATH];
	char bot_filename[MAX_OSPATH];

	menulist_s *f = ( menulist_s * ) self;

	slot = f->curvalue;

	count = 8;

	if(!strcmp(f->generic.name, "...empty slot")) {
		//open the bot menu
		M_Menu_AddBots_f();
		for(i = 0; i < 8; i++) {
			if(!strcmp(s_bots_bot_action[i].generic.name, "...empty slot")) {
				//clear it, it's slot is empty
				strcpy(bot[i].name, "...empty slot");
				bot[i].userinfo[0] = 0;
				count--;
			}
		}
	}
	else {
		f->generic.name = "...empty slot";
		//clear the bot out of the struct...hmmm...kinda hokey, but - need to know which slot
		for(i = 0; i < 8; i++) {
			if(!strcmp(s_bots_bot_action[i].generic.name, "...empty slot")) {
				//clear it, it's slot is empty
				strcpy(bot[i].name, "...empty slot");
				bot[i].userinfo[0] = 0;
				count--;
			}
		}
	}

	//write out bot file
	if ( is_team_game( s_rules_box.curvalue ) )
	{
		strcpy( stem, "team" );
	}
	else
	{ // non-team, bots per map
		strcpy( stem, strchr( mapnames[s_startmap_list.curvalue], '\n' ) + 1 );
		for(i = 0; i < strlen(stem); i++)
			stem[i] = tolower( stem[i] );
	}
	Com_sprintf( relative_path, sizeof(relative_path), "botinfo/%s.tmp", stem );
	FS_FullWritePath( bot_filename, sizeof(bot_filename), relative_path );

	if((pOut = fopen(bot_filename, "wb" )) == NULL)
	{
		Com_DPrintf("BotAction: failed fopen for write: %s\n", bot_filename );
		return; // bail
	}

	szr = fwrite(&count,sizeof (int),1,pOut); // Write number of bots

	for (i = 7; i > -1; i--) {
		if(strcmp(bot[i].name, "...empty slot"))
			szr = fwrite(bot[i].userinfo,sizeof (char) * MAX_INFO_STRING,1,pOut);
	}

	fclose(pOut);

	return;
}

/*
=============================================================================

ADDRESS BOOK MENU

=============================================================================
*/
#define NUM_ADDRESSBOOK_ENTRIES 9

static menuframework_s	s_addressbook_menu;
static char				s_addressbook_cvarnames[NUM_ADDRESSBOOK_ENTRIES][20];
static menufield_s		s_addressbook_fields[NUM_ADDRESSBOOK_ENTRIES];

static void M_Menu_AddressBook_f (void)
{
	int i;

	s_addressbook_menu.nitems = 0;

	for ( i = 0; i < NUM_ADDRESSBOOK_ENTRIES; i++ )
	{
		cvar_t *adr;

		Com_sprintf( s_addressbook_cvarnames[i], sizeof( s_addressbook_cvarnames[i] ), "adr%d", i );

		adr = Cvar_Get( s_addressbook_cvarnames[i], "", CVAR_ARCHIVE );

		s_addressbook_fields[i].generic.type			= MTYPE_FIELD;
		s_addressbook_fields[i].generic.callback		= StrFieldCallback;
		s_addressbook_fields[i].generic.localstrings[0]	= &s_addressbook_cvarnames[i][0];
		s_addressbook_fields[i].cursor					= strlen (adr->string);
		s_addressbook_fields[i].generic.visible_length	= LONGINPUT_SIZE;

		strcpy( s_addressbook_fields[i].buffer, adr->string );

		Menu_AddItem( &s_addressbook_menu, &s_addressbook_fields[i] );
	}
	
	Menu_AutoArrange (&s_addressbook_menu);
	Menu_Center (&s_addressbook_menu);
	
	M_PushMenu_Defaults (s_addressbook_menu);
}

/*
=============================================================================

PLAYER RANKING MENU

=============================================================================
*/

static menuframework_s	s_playerranking_screen;
static menuframework_s	s_playerranking_menu;
static menuaction_s		s_playerranking_title;
static menuaction_s		s_playerranking_ttheader;
static menuaction_s		s_playerranking_topten[10];
char rank[32];
char fragrate[32];
char playername[64]; // a print field, not just name
char totaltime[32];
char totalfrags[32];
char topTenList[10][64];

static void M_Menu_PlayerRanking_f (void)
{
	extern cvar_t *name;
	PLAYERSTATS player;
	PLAYERSTATS topTenPlayers[10];
	int i;

	setup_window (s_playerranking_screen, s_playerranking_menu, "PLAYER RANKINGS");

	Q_strncpyz2( player.playername, name->string, sizeof(player.playername) );

	player.totalfrags = player.totaltime = player.ranking = 0;
	player = getPlayerRanking ( player );

	Com_sprintf(playername, sizeof(playername), "Name: %s", player.playername);
	if(player.ranking > 0)
		Com_sprintf(rank, sizeof(rank), "Rank: ^1%i", player.ranking);
	else
		Com_sprintf(rank, sizeof(rank), "Rank: ^1Unranked");
	if ( player.totaltime > 1.0f )
		Com_sprintf(fragrate, sizeof(fragrate), "Frag Rate: %6.2f", (float)(player.totalfrags)/(player.totaltime - 1.0f) );
	else
		Com_sprintf(fragrate, sizeof(fragrate), "Frag Rate: 0" );
	Com_sprintf(totalfrags, sizeof(totalfrags), "Total Frags: ^1%i", player.totalfrags);
	Com_sprintf(totaltime, sizeof(totaltime), "Total Time: %6.2f", player.totaltime - 1.0f);

	s_playerranking_title.generic.type	= MTYPE_ACTION;
	s_playerranking_title.generic.name	= "Player Ranking and Stats";
	s_playerranking_title.generic.flags	= QMF_RIGHT_COLUMN;
	Menu_AddItem( &s_playerranking_menu, &s_playerranking_title );
	
	add_text(s_playerranking_menu, playername, QMF_RIGHT_COLUMN);
	add_text(s_playerranking_menu, rank, QMF_RIGHT_COLUMN);
	add_text(s_playerranking_menu, fragrate, QMF_RIGHT_COLUMN);
	add_text(s_playerranking_menu, totalfrags, QMF_RIGHT_COLUMN);
	add_text(s_playerranking_menu, totaltime, QMF_RIGHT_COLUMN);

	s_playerranking_ttheader.generic.type	= MTYPE_ACTION;
	s_playerranking_ttheader.generic.name	= "Top Ten Players";
	s_playerranking_ttheader.generic.flags	= QMF_RIGHT_COLUMN;
	Menu_AddItem (&s_playerranking_menu, &s_playerranking_ttheader);

	for(i = 0; i < 10; i++) {

		topTenPlayers[i].totalfrags = topTenPlayers[i].totaltime = topTenPlayers[i].ranking = 0;
		topTenPlayers[i] = getPlayerByRank ( i+1, topTenPlayers[i] );

		if(i < 9)
			Com_sprintf(topTenList[i], sizeof(topTenList[i]), "Rank: ^1%i %s", topTenPlayers[i].ranking, topTenPlayers[i].playername);
		else
			Com_sprintf(topTenList[i], sizeof(topTenList[i]), "Rank:^1%i %s", topTenPlayers[i].ranking, topTenPlayers[i].playername);

		s_playerranking_topten[i].generic.type	= MTYPE_TEXT;
		s_playerranking_topten[i].generic.name	= topTenList[i];
		s_playerranking_topten[i].generic.flags	= QMF_RIGHT_COLUMN;

		Menu_AddItem( &s_playerranking_menu, &s_playerranking_topten[i] );
	}
	
	M_PushMenu_Defaults (s_playerranking_screen);
}

/*
=============================================================================

PLAYER CONFIG MENU

=============================================================================
*/
static qboolean s_switched = true;

typedef struct 
{
	menucommon_s generic;
	const char *name;
	const char *skin;
	float w, h;
	float mframe, yaw;
	int frametime;
} menumodel_s;

static menuvec2_t PlayerModelSizeFunc (void *_self, FNT_font_t font)
{
	menuvec2_t ret;
	menumodel_s *self = (menumodel_s*) _self;
	
	ret.x = (self->w+2)*font->size;
	ret.y = (self->h+2)*font->size;
	
	return ret;
}

static void PlayerModelDrawFunc (void *_self, FNT_font_t font)
{
	refdef_t refdef;
	char scratch[MAX_OSPATH];
	FILE *modelfile;
	int i;
	int renderHelmet = 0;
	extern float CalcFov( float fov_x, float w, float h );
	entity_t entity[5];
	menumodel_s *self = (menumodel_s*) _self;
	float old_frame = self->mframe;
	
	//181 - 197 to start from rising position

	if(self->mframe == 0 || s_switched)
		self->mframe = 181;

	self->mframe += cls.frametime * 15.0f;

	if (self->mframe > 197)
		self->mframe = 1;
	
	if(self->mframe < 181)
	{
		if (self->mframe > 39)
			self->mframe = 1;
		if (self->mframe < 1)
			self->mframe = 1;
	}
	
	// for IQM lerping
	if ((int)old_frame != (int)self->mframe)
		self->frametime = Sys_Milliseconds ();

	if(self->mframe > 0 && self->mframe < 40)
		self->yaw += cls.frametime*50;
	if (self->yaw > 360 )
		self->yaw = 0;
	if(s_switched)
		self->yaw = 180;

	if(s_switched)
	{
		S_StartLocalSound ("misc/tele1.wav");
		s_switched = false;
	}

	memset( &refdef, 0, sizeof( refdef ) );
	
	refdef.width = self->w*font->size;
	refdef.height = self->h*font->size;
	refdef.x = Item_GetX(*self);
	refdef.y = Item_GetY(*self);
	refdef.x -= refdef.width;

	Menu_DrawBox (refdef.x, refdef.y, refdef.width, refdef.height, 1, NULL, "menu/sm_");
	
	refdef.width -= font->size;
	refdef.height -= font->size;
	
	refdef.fov_x = 35;
	refdef.fov_y = CalcFov( refdef.fov_x, refdef.width, refdef.height );
	refdef.time = cls.realtime*0.001;
	
	memset( &entity, 0, sizeof( entity ) );

	Com_sprintf( scratch, sizeof( scratch ), "players/%s/tris.iqm", self->name );
	entity[0].model = R_RegisterModel( scratch );
	Com_sprintf( scratch, sizeof( scratch ), "players/%s/%s.jpg", self->name, self->skin );
	entity[0].skin = R_RegisterSkin( scratch );
	entity[0].flags = RF_FULLBRIGHT | RF_MENUMODEL;

	Com_sprintf( scratch, sizeof( scratch ), "players/%s/weapon.iqm", self->name );
	entity[1].model = R_RegisterModel( scratch );
	Com_sprintf( scratch, sizeof( scratch ), "players/%s/weapon.tga", self->name );
	entity[1].skin = R_RegisterSkin( scratch );
	entity[1].flags = RF_FULLBRIGHT | RF_MENUMODEL;
	
	refdef.num_entities = 2;

	entity[2].model = R_RegisterModel("models/objects/dmspot/tris.iqm");
	entity[2].flags = RF_FULLBRIGHT | RF_MENUMODEL;
	refdef.num_entities = 3;

	//if a helmet or other special devce
	Com_sprintf( scratch, sizeof( scratch ), "players/%s/helmet.iqm", self->name );
	FS_FOpenFile( scratch, &modelfile );
	if ( modelfile )
	{
		fclose(modelfile);
		
		entity[3].model = R_RegisterModel( scratch );
		Com_sprintf( scratch, sizeof( scratch ), "players/%s/helmet.tga", self->name );
		entity[3].skin = R_RegisterSkin( scratch );
		entity[3].flags = RF_FULLBRIGHT | RF_TRANSLUCENT | RF_MENUMODEL;
		entity[3].alpha = 0.4;
		
		refdef.num_entities = 4;
		renderHelmet = 1;
	}

	
	if(self->mframe >= 181)
	{
		entity[3+renderHelmet].model = R_RegisterModel("models/objects/powerdome/tris.iqm");
		entity[3+renderHelmet].flags = RF_FULLBRIGHT | RF_MENUMODEL;

		refdef.num_entities = 4+renderHelmet;
	}
	
	for (i = 0; i < refdef.num_entities; i++)
	{
		// seems a little odd to use frame-1 for oldframe and frame%1 for 
		// backlerp, but it works out
		entity[i].frame = (int)self->mframe;
		entity[i].oldframe = entity[i].frame - 1;
		// for MD2 lerping:
		entity[i].backlerp = self->mframe - (float)entity[i].frame;
		entity[i].angles[1] = (int)self->yaw;
		// for IQM lerping:
		entity[i].frametime = self->frametime;
		
		VectorSet (entity[i].origin, 80, 0, -3 - (i==2?6.5:0.0));
		VectorCopy (entity[i].origin, entity[i].oldorigin);
	}
		
	refdef.areabits = 0;
	refdef.entities = entity;
	refdef.lightstyles = 0;
	refdef.rdflags = RDF_NOWORLDMODEL;
	
	R_RenderFramePlayerSetup( &refdef );
}

static menuframework_s	s_player_config_menu;

static menufield_s		s_player_name_field;

static menuframework_s	s_player_password_submenu;
static menuframework_s	s_player_password_field_submenu;
static menufield_s		s_player_password_field;

static menuframework_s	s_player_skin_submenu;
static menuframework_s	s_player_skin_controls_submenu;
static menulist_s		s_player_model_box;
static menulist_s		s_player_skin_box;
static menuitem_s   	s_player_thumbnail;

static menuframework_s	s_player_skin_preview_submenu;
static menumodel_s		s_player_skin_preview;

#define MAX_DISPLAYNAME 16
#define MAX_PLAYERMODELS 1024

typedef struct
{
	int		nskins;
	char	**skindisplaynames;
	char	displayname[MAX_DISPLAYNAME];
	char	directory[MAX_OSPATH];
} playermodelinfo_s;

static playermodelinfo_s s_pmi[MAX_PLAYERMODELS];
static char *s_pmnames[MAX_PLAYERMODELS];
static int s_numplayermodels = 0;

static void ModelCallback (UNUSED void *unused)
{
	s_player_skin_box.itemnames = (const char **) s_pmi[s_player_model_box.curvalue].skindisplaynames;
	s_player_skin_box.curvalue = 0;

	s_switched = true;
	
	Menu_ActivateItem ((menuitem_s *)&s_player_skin_box);
}

static void SkinCallback (UNUSED void *unused)
{
	char scratch[MAX_QPATH];
	
	Com_sprintf( scratch, sizeof( scratch ), "%s/%s",
		s_pmi[s_player_model_box.curvalue].directory,
		s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue] );

	Cvar_Set( "skin", scratch );
}

static qboolean IconOfSkinExists( char *skin, char **pcxfiles, int npcxfiles )
{
	int i;
	char scratch[1024];

	strcpy( scratch, skin );
	*strrchr( scratch, '.' ) = 0;
	strcat( scratch, "_i.tga" );

	for ( i = 0; i < npcxfiles; i++ )
	{
		if ( strcmp( pcxfiles[i], scratch ) == 0 )
			return true;
	}

	strcpy( scratch, skin );
	*strrchr( scratch, '.' ) = 0;
	strcat( scratch, "_i.jpg" );

	for ( i = 0; i < npcxfiles; i++ )
	{
		if ( strcmp( pcxfiles[i], scratch ) == 0 )
			return true;
	}

	return false;
}

static void PlayerConfig_ScanDirectories( void )
{
	char scratch[1024];
	int ndirs = 0, npms = 0;
	char **dirnames;
	int i;

	// check if we need to do anything
	if (s_numplayermodels != 0)
		return;

	//get dirs from gamedir first.
	dirnames = FS_ListFilesInFS( "players/*.*", &ndirs, 0, 0 );

	if ( !dirnames )
		return;

	/*
	** go through the subdirectories
	*/
	npms = ndirs;
	if ( npms > MAX_PLAYERMODELS )
		npms = MAX_PLAYERMODELS;

	for ( i = 0; i < npms; i++ )
	{
		int k, s;
		char *a, *b, *c;
		char **pcxnames;
		char **skinnames;
		int npcxfiles;
		int nskins = 0;

		if ( dirnames[i] == 0 )
			continue;

		// verify the existence of tris.md2
		strcpy( scratch, dirnames[i] );
		strcat( scratch, "/tris.md2" );
		if (!FS_FileExists(scratch))
		{
			//try for tris.iqm if no md2
			strcpy( scratch, dirnames[i] );
			strcat( scratch, "/tris.iqm" );
			if (!FS_FileExists(scratch))
			{
				free( dirnames[i] );
				dirnames[i] = 0;
				continue;
			}
		}

		// verify the existence of at least one skin(note, do not mix .tga and .jpeg)
		strcpy( scratch, dirnames[i] );
		strcat( scratch, "/*.jpg" );
		pcxnames = FS_ListFilesInFS( scratch, &npcxfiles, 0,
			SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );

		if(!pcxnames) {
			// check for .tga, though this is no longer used for current models
			strcpy( scratch, dirnames[i] );
			strcat( scratch, "/*.tga" );
			pcxnames = FS_ListFilesInFS( scratch, &npcxfiles, 0,
				SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM );
		}

		if ( !pcxnames )
		{
			free( dirnames[i] );
			dirnames[i] = 0;
			continue;
		}

		// count valid skins, which consist of a skin with a matching "_i" icon
		for ( k = 0; k < npcxfiles; k++ )
		{
			if ( !strstr( pcxnames[k], "_i.tga" ) || !strstr( pcxnames[k], "_i.jpg" ))
			{
				if ( IconOfSkinExists( pcxnames[k], pcxnames, npcxfiles) )
				{
					nskins++;
				}
			}
		}
		if ( !nskins )
			continue;

		skinnames = malloc( sizeof( char * ) * ( nskins + 1 ) );
		memset( skinnames, 0, sizeof( char * ) * ( nskins + 1 ) );

		// copy the valid skins
		for ( s = 0, k = 0; k < npcxfiles; k++ )
		{
			char *a, *b, *c;

			if ( !strstr( pcxnames[k], "_i.tga" ) )
			{
				if ( IconOfSkinExists( pcxnames[k], pcxnames, npcxfiles ) )
				{
					a = strrchr( pcxnames[k], '/' );
					b = strrchr( pcxnames[k], '\\' );

					if ( a > b )
						c = a;
					else
						c = b;

					strcpy( scratch, c + 1 );

					if ( strrchr( scratch, '.' ) )
						*strrchr( scratch, '.' ) = 0;

					skinnames[s] = _strdup( scratch );
					s++;
				}
			}
		}

		// at this point we have a valid player model
		s_pmi[s_numplayermodels].nskins = nskins;
		s_pmi[s_numplayermodels].skindisplaynames = skinnames;

		// make short name for the model
		a = strrchr( dirnames[i], '/' );
		b = strrchr( dirnames[i], '\\' );

		if ( a > b )
			c = a;
		else
			c = b;

		strncpy( s_pmi[s_numplayermodels].displayname, c + 1, MAX_DISPLAYNAME-1 );
		strcpy( s_pmi[s_numplayermodels].directory, c + 1 );

		FS_FreeFileList( pcxnames, npcxfiles );

		s_numplayermodels++;
	}
	if ( dirnames )
		free( dirnames );
}

static int pmicmpfnc( const void *_a, const void *_b )
{
	const playermodelinfo_s *a = ( const playermodelinfo_s * ) _a;
	const playermodelinfo_s *b = ( const playermodelinfo_s * ) _b;

	/*
	** sort by male, female, then alphabetical
	*/
	if ( strcmp( a->directory, "male" ) == 0 )
		return -1;
	else if ( strcmp( b->directory, "male" ) == 0 )
		return 1;

	if ( strcmp( a->directory, "female" ) == 0 )
		return -1;
	else if ( strcmp( b->directory, "female" ) == 0 )
		return 1;

	return strcmp( a->directory, b->directory );
}

static void PlayerPicDrawFunc (void *_self, FNT_font_t font)
{
	int x, y;
	char scratch[MAX_QPATH];
	menuitem_s *self = (menuitem_s *)_self;
	x = Item_GetX (*self);
	y = Item_GetY (*self);
	
	Com_sprintf( scratch, sizeof( scratch ), "/players/%s_i.tga",
			Cvar_VariableString ("skin") );
	
	Draw_StretchPic (x, y, font->size*5, font->size*5, scratch);
}

static void PasswordCallback (void *_self)
{
	menufield_s *self = (menufield_s *)_self;
	
	//was the password changed?
	if(strcmp("********", self->buffer))
	{
		//if this is a virgin password, don't change, just authenticate
		if(!strcmp(stats_password->string, "password"))
		{
			Cvar_FullSet( "stats_password", self->buffer, CVAR_PROFILE);
			stats_password = Cvar_Get("stats_password", "password", CVAR_PROFILE);
			Cvar_FullSet( "stats_pw_hashed", "0", CVAR_PROFILE);
			currLoginState.validated = false;
			STATS_RequestVerification();
		}
		else
		{
			Cvar_FullSet( "stats_password", self->buffer, CVAR_PROFILE);
			stats_password = Cvar_Get("stats_password", "password", CVAR_PROFILE);
			Cvar_FullSet( "stats_pw_hashed", "0", CVAR_PROFILE);
			STATS_RequestPwChange();
		}
	}
}

static void PConfigApplyFunc (void *self)
{
	Menu_ApplyMenu (Menu_GetItemTree ((menuitem_s *)self));
}

static menuvec2_t PlayerConfigModelSizeFunc (void *_self, FNT_font_t font)
{
	menuvec2_t ret;
	menumodel_s *self = (menumodel_s*) _self;
	
	ret.x = 20*font->size;
	ret.y = 29*font->size;
	
	self->w = (float)ret.x/(float)font->size;
	self->h = (float)ret.y/(float)font->size;
	
	return ret;
}

static void PlayerConfig_MenuInit (void)
{
	extern cvar_t *name;
	// extern cvar_t *team; // unused
	// extern cvar_t *skin; // unused
	char currentdirectory[1024];
	char currentskin[1024];
	int i = 0;
	int currentdirectoryindex = 0;
	int currentskinindex = 0;
	cvar_t *hand = Cvar_Get( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE );

	PlayerConfig_ScanDirectories();

	if (s_numplayermodels == 0)
		return;

	if ( hand->value < 0 || hand->value > 2 )
		Cvar_SetValue( "hand", 0 );

	Q_strncpyz( currentdirectory, Cvar_VariableString ("skin"), sizeof(currentdirectory)-1);

	if ( strchr( currentdirectory, '/' ) )
	{
		strcpy( currentskin, strchr( currentdirectory, '/' ) + 1 );
		*strchr( currentdirectory, '/' ) = 0;
	}
	else if ( strchr( currentdirectory, '\\' ) )
	{
		strcpy( currentskin, strchr( currentdirectory, '\\' ) + 1 );
		*strchr( currentdirectory, '\\' ) = 0;
	}
	else
	{
		strcpy( currentdirectory, "martianenforcer" );
		strcpy( currentskin, "default" );
	}

	qsort( s_pmi, s_numplayermodels, sizeof( s_pmi[0] ), pmicmpfnc );

	memset( s_pmnames, 0, sizeof( s_pmnames ) );
	for ( i = 0; i < s_numplayermodels; i++ )
	{
		s_pmnames[i] = s_pmi[i].displayname;
		if ( Q_strcasecmp( s_pmi[i].directory, currentdirectory ) == 0 )
		{
			int j;

			currentdirectoryindex = i;

			for ( j = 0; j < s_pmi[i].nskins; j++ )
			{
				if ( Q_strcasecmp( s_pmi[i].skindisplaynames[j], currentskin ) == 0 )
				{
					currentskinindex = j;
					break;
				}
			}
		}
	}

	setup_window (s_player_config_screen, s_player_config_menu, "PLAYER SETUP");

	s_player_name_field.generic.type = MTYPE_FIELD;
	s_player_name_field.generic.name = "name";
	s_player_name_field.generic.localstrings[0] = "name";
	s_player_name_field.generic.callback = StrFieldCallback;
	s_player_name_field.length	= 20;
	s_player_name_field.generic.visible_length = LONGINPUT_SIZE;
	Q_strncpyz2( s_player_name_field.buffer, name->string, sizeof(s_player_name_field.buffer) );
	s_player_name_field.cursor = strlen( s_player_name_field.buffer );
	
	if (STATS_ENABLED) {
		// Horizontal submenu with two items. The first is a password field. The 
		// second is an apply button for the password.
		s_player_password_submenu.generic.type = MTYPE_SUBMENU;
		// Keep the password field horizontally lined up:
		s_player_password_submenu.generic.flags = QMF_SNUG_LEFT;
		s_player_password_submenu.navagable = true;
		s_player_password_submenu.horizontal = true;
		s_player_password_submenu.nitems = 0;
		
		// sub-submenu for the password field. Purely for formatting/layout 
		// purposes.
		s_player_password_field_submenu.generic.type = MTYPE_SUBMENU;
		s_player_password_field_submenu.navagable = true;
		s_player_password_field_submenu.horizontal = true;
		s_player_password_field_submenu.nitems = 0;
		// keep the password field horizontally lined up:
		LINK (s_player_config_menu.lwidth, s_player_password_field_submenu.lwidth);
		// keep it vertically centered on the apply button
		LINK (s_player_password_submenu.height, s_player_password_field_submenu.height);
		
		s_player_password_field.generic.type = MTYPE_FIELD;
		s_player_password_field.generic.name = "password";
		s_player_password_field.generic.flags = QMF_ACTION_WAIT;
		s_player_password_field.generic.callback = PasswordCallback;
		s_player_password_field.length	= 20;
		s_player_password_field.generic.visible_length = LONGINPUT_SIZE;
		s_player_password_field.generic.statusbar = "COR Entertainment is not responsible for lost or stolen passwords";
		Q_strncpyz2( s_player_password_field.buffer, "********", sizeof(s_player_password_field.buffer) );
		s_player_password_field.cursor = 0;
		Menu_AddItem( &s_player_password_submenu, &s_player_password_field_submenu);
		Menu_AddItem( &s_player_password_field_submenu, &s_player_password_field);
		
		add_action (s_player_password_submenu, "Apply", PConfigApplyFunc, 0);
	}
	
	// Horizontal submenu with two items. The first is a submenu with the
	// model/skin controls. The second is just a thumbnail of the current
	// selection.
	s_player_skin_submenu.generic.type = MTYPE_SUBMENU;
	// Keep the model/skin controls horizontally lined up:
	s_player_skin_submenu.generic.flags = QMF_SNUG_LEFT;
	s_player_skin_submenu.navagable = true;
	s_player_skin_submenu.horizontal = true;
	s_player_skin_submenu.nitems = 0;
	
	// Vertical sub-submenu with two items. The first is the model control. 
	// The second is the skin control.
	s_player_skin_controls_submenu.generic.type = MTYPE_SUBMENU;
	s_player_skin_controls_submenu.navagable = true;
	s_player_skin_controls_submenu.nitems = 0;
	// keep the model/skin controls horizontally lined up:
	LINK (s_player_config_menu.lwidth, s_player_skin_controls_submenu.lwidth);

	s_player_model_box.generic.type = MTYPE_SPINCONTROL;
	s_player_model_box.generic.name = "model";
	s_player_model_box.generic.callback = ModelCallback;
	s_player_model_box.curvalue = currentdirectoryindex;
	s_player_model_box.itemnames = (const char **) s_pmnames;

	s_player_skin_box.generic.type = MTYPE_SPINCONTROL;
	s_player_skin_box.generic.callback = SkinCallback;
	s_player_skin_box.generic.name = "skin";
	s_player_skin_box.curvalue = currentskinindex;
	s_player_skin_box.itemnames = (const char **) s_pmi[currentdirectoryindex].skindisplaynames;
	
	Menu_AddItem( &s_player_skin_controls_submenu, &s_player_model_box );
	if ( s_player_skin_box.itemnames )
		Menu_AddItem( &s_player_skin_controls_submenu, &s_player_skin_box );
	
	Menu_AddItem (&s_player_skin_submenu, &s_player_skin_controls_submenu);
	
	// TODO: click this to cycle skins
	s_player_thumbnail.generic.type = MTYPE_NOT_INTERACTIVE;
	VectorSet(s_player_thumbnail.generic.localints, 5, 5, 0);
	s_player_thumbnail.generic.itemsizecallback = PicSizeFunc;
	s_player_thumbnail.generic.itemdraw = PlayerPicDrawFunc;
	Menu_AddItem (&s_player_skin_submenu, &s_player_thumbnail);

	s_player_skin_preview_submenu.generic.type = MTYPE_SUBMENU;
	s_player_skin_preview_submenu.generic.flags = QMF_SNUG_LEFT;
	s_player_skin_preview_submenu.nitems = 0;
	
	Menu_AddItem( &s_player_config_menu, &s_player_name_field );
	if (STATS_ENABLED) {
		Menu_AddItem( &s_player_config_menu, &s_player_password_submenu);
	}
	Menu_AddItem( &s_player_config_menu, &s_player_skin_submenu);
		
	s_player_skin_preview.generic.type = MTYPE_NOT_INTERACTIVE;
	s_player_skin_preview.generic.namesizecallback = PlayerConfigModelSizeFunc;
	s_player_skin_preview.generic.namedraw = PlayerModelDrawFunc;
	
	Menu_AddItem (&s_player_config_menu, &s_player_skin_preview_submenu);
	Menu_AddItem (&s_player_skin_preview_submenu, &s_player_skin_preview);
	
	//add in shader support for player models, if the player goes into the menu before entering a
	//level, that way we see the shaders.  We only want to do this if they are NOT loaded yet.
	scriptsloaded = Cvar_Get("scriptsloaded", "0", 0);
	if(!scriptsloaded->value)
	{
		Cvar_SetValue("scriptsloaded", 1); //this needs to be reset on vid_restart
		RS_ScanPathForScripts();
		RS_LoadScript("scripts/models.rscript");
		RS_LoadScript("scripts/caustics.rscript");
		RS_LoadSpecialScripts();
	}
}

static void PlayerConfig_MenuDraw (UNUSED menuframework_s *dummy, menuvec2_t offset)
{
	if(!PLAYER_NAME_UNIQUE)
		s_player_config_menu.statusbar = "You must change your player name before joining a server!";

	if ( s_pmi[s_player_model_box.curvalue].skindisplaynames )
	{
		s_player_skin_preview.name = s_pmi[s_player_model_box.curvalue].directory;
		s_player_skin_preview.skin = s_pmi[s_player_model_box.curvalue].skindisplaynames[s_player_skin_box.curvalue];
		Screen_Draw (&s_player_config_screen, offset);
	}
}

void M_Menu_PlayerConfig_f (void)
{
	PlayerConfig_MenuInit();
	M_PushMenu (PlayerConfig_MenuDraw, Default_MenuKey, &s_player_config_screen);
}

/*
=======================================================================

ALIEN ARENA TACTICAL MENU

=======================================================================
*/

	
//to do - allow for listen server to call this and host server
//or - abort this entirely, and have an in-game way to join a team like we do in CTF

static menuframework_s	s_tactical_screen;

#define num_tactical_teams		2
#define num_tactical_classes	3
static const char *tactical_skin_names[num_tactical_teams][num_tactical_classes][2] =
{
	//ALIEN CLASSES
	{
		{"Enforcer(detonators)",	"martianenforcer"},
		{"Warrior(bombs)",			"martianwarrior"},
		{"Overlord(minderaser)",	"martianoverlord"}
	},
	//HUMAN CLASSES
	{
		{"Femborg(detonators)",		"femborg"},
		{"Enforcer(bombs)",			"enforcer"},
		{"Commander(minderaser)",	"commander"}
	}
};

static const char *tactical_team_names[num_tactical_teams] =
{
	"ALIENS",
	"HUMANS"
};

static menuframework_s	s_tactical_menus[num_tactical_teams];
static menuframework_s	s_tactical_columns[num_tactical_teams][num_tactical_classes];
static menuaction_s 	s_tactical_skin_actions[num_tactical_teams][num_tactical_classes];
static menumodel_s 		s_tactical_skin_previews[num_tactical_teams][num_tactical_classes];

static void TacticalJoinFunc ( void *item )
{
	menuaction_s *self;
	char buffer[128];
	
	self = (menuaction_s*)item;
	
	cl.tactical = true;
	
	//set skin and model
	Com_sprintf (buffer, sizeof(buffer), "%s/default", self->generic.localstrings[0]);
	Cvar_Set ("skin", buffer);

	//join server
	Com_sprintf (buffer, sizeof(buffer), "connect %s\n", NET_AdrToString (mservers[serverindex].local_server_netadr));
	Cbuf_AddText (buffer);
	M_ForceMenuOff ();
}

static void TacticalScreen_Draw (menuframework_s *screen, menuvec2_t offset)
{
	FNT_font_t font = FNT_AutoGet (CL_menuFont);
	screen->x = offset.x;
	Menu_AutoArrange (screen);
	// force it to use up the whole screen
	CHASELINK(s_tactical_screen.rwidth) = viddef.width - CHASELINK(s_tactical_screen.lwidth);
	Menu_Draw (screen, font);
}

static void M_Menu_Tactical_f (void)
{
	extern cvar_t *name;
	int i, j;
	
	for (i = 0; i < num_tactical_teams; i++)
	{
		// kinda hacky but this is the only place we have two windows in one
		// screen
		setup_nth_window (s_tactical_screen, i, s_tactical_menus[i], tactical_team_names[i]);
		
		s_tactical_menus[i].horizontal = true;
		
		for (j = 0; j < num_tactical_classes; j++)
		{
			s_tactical_columns[i][j].generic.type = MTYPE_SUBMENU;
			s_tactical_columns[i][j].nitems = 0;
			s_tactical_columns[i][j].navagable = true;
			Menu_AddItem (&s_tactical_menus[i], &s_tactical_columns[i][j]);

			s_tactical_skin_previews[i][j].generic.type = MTYPE_NOT_INTERACTIVE;
			s_tactical_skin_previews[i][j].generic.namesizecallback = PlayerModelSizeFunc;
			s_tactical_skin_previews[i][j].generic.namedraw = PlayerModelDrawFunc;
			s_tactical_skin_previews[i][j].name = tactical_skin_names[i][j][1];
			s_tactical_skin_previews[i][j].skin = "default";
			s_tactical_skin_previews[i][j].h = 14;
			s_tactical_skin_previews[i][j].w = 10;
			Menu_AddItem (&s_tactical_columns[i][j], &s_tactical_skin_previews[i][j]);
			
			s_tactical_skin_actions[i][j].generic.type = MTYPE_ACTION;
			s_tactical_skin_actions[i][j].generic.flags = QMF_BUTTON;
			s_tactical_skin_actions[i][j].generic.name = tactical_skin_names[i][j][0];
			s_tactical_skin_actions[i][j].generic.localstrings[0] = tactical_skin_names[i][j][1];
			s_tactical_skin_actions[i][j].generic.callback = TacticalJoinFunc;
			Menu_AddItem (&s_tactical_columns[i][j], &s_tactical_skin_actions[i][j]);
		}
	}

	//add in shader support for player models, if the player goes into the menu before entering a
	//level, that way we see the shaders.  We only want to do this if they are NOT loaded yet.
	scriptsloaded = Cvar_Get("scriptsloaded", "0", 0);
	if(!scriptsloaded->value)
	{
		Cvar_SetValue("scriptsloaded", 1); //this needs to be reset on vid_restart
		RS_ScanPathForScripts();
		RS_LoadScript("scripts/models.rscript");
		RS_LoadScript("scripts/caustics.rscript");
		RS_LoadSpecialScripts();
	}
	
	M_PushMenu (TacticalScreen_Draw, Default_MenuKey, &s_tactical_screen);
}


/*
=======================================================================

QUIT MENU

=======================================================================
*/

static menuframework_s	s_quit_screen;
static menuframework_s	s_quit_menu;

static void quitActionNo (UNUSED void *blah)
{
	M_PopMenu();
}
static void quitActionYes (UNUSED void *blah)
{
	CL_Quit_f();
}

static void M_Menu_Quit_f (void)
{
	setup_window (s_quit_screen, s_quit_menu, "EXIT ALIEN ARENA");

	add_text (s_quit_menu, "Are you sure?", 0);
	add_action (s_quit_menu, "Yes", quitActionYes, 0);
	add_action (s_quit_menu, "No", quitActionNo, 0);
	
	Menu_AutoArrange (&s_quit_screen);
	Menu_Center (&s_quit_screen);
	
	M_PushMenu_Defaults (s_quit_screen);
}

//=============================================================================
/* Menu Subsystem */


/*
=================
M_Init
=================
*/
void M_Init (void)
{
	Cmd_AddCommand ("menu_main", M_Menu_Main_f);
	Cmd_AddCommand ("menu_quit", M_Menu_Quit_f);
}


/*
=================================
Menu Mouse Cursor
=================================
*/

void refreshCursorLink (void)
{
	Cursor_SelectItem (NULL);
	cursor.click_menuitem = NULL;
}

static int Slider_CursorPositionX (menuslider_s *s)
{
	float		range;
	FNT_font_t	font;
	
	font = FNT_AutoGet( CL_menuFont );

	range = ( s->curvalue - s->minvalue ) / ( float ) ( s->maxvalue - s->minvalue );

	if ( range < 0)
		range = 0;
	if ( range > 1)
		range = 1;

	return ( int )( font->width + RCOLUMN_OFFSET + (SLIDER_SIZE) * font->width * range );
}

static int newSliderValueForX (int x, menuslider_s *s)
{
	float 		newValue;
	int 		newValueInt;
	FNT_font_t	font;
	int			pos;
	
	font = FNT_AutoGet( CL_menuFont );
	
	pos = x - (font->width + RCOLUMN_OFFSET + CHASELINK(s->generic.x)) - Menu_GetCtrX(*(s->generic.parent));

	newValue = ((float)pos)/((SLIDER_SIZE-1)*font->width);
	newValueInt = s->minvalue + newValue * (float)( s->maxvalue - s->minvalue );

	return newValueInt;
}

static void Slider_CheckSlide (menuslider_s *s)
{
	if ( s->curvalue > s->maxvalue )
		s->curvalue = s->maxvalue;
	else if ( s->curvalue < s->minvalue )
		s->curvalue = s->minvalue;

	//TO DO - this generates a scary warning - incompatible menuslider_s vs menuitem_s * - there are several similar instances in the menu code.
	Menu_ActivateItem (s);
}

static void Menu_DragSlideItem (void)
{
	menuslider_s *slider = ( menuslider_s * ) cursor.menuitem;

	slider->curvalue = newSliderValueForX(cursor.x, slider);
	Slider_CheckSlide ( slider );
}

static void Menu_ClickSlideItem (void)
{
	int min, max;
	menuslider_s *slider = ( menuslider_s * ) cursor.menuitem;

	min = Item_GetX (*slider) + Slider_CursorPositionX(slider) - 4;
	max = Item_GetX (*slider) + Slider_CursorPositionX(slider) + 4;

	if (cursor.x < min)
		Menu_SlideItem (-1 );
	if (cursor.x > max)
		Menu_SlideItem (1);
}

static void Menu_DragVertScrollItem (void)
{
	float			scrollbar_pos;
	menuframework_s	*menu = cursor.menuitem->generic.parent;
	
	scrollbar_pos = (float)cursor.y - menu->scroll_top;
	menu->yscroll = scrollbar_pos*menu->maxscroll/(menu->scroll_range-menu->scrollbar_size);
	
	if (menu->yscroll < 0)
		menu->yscroll = 0;
	if (menu->yscroll > menu->maxscroll)
		menu->yscroll = menu->maxscroll;
}

static void M_Draw_Cursor (void)
{
	Draw_Pic (cursor.x, cursor.y, "m_mouse_cursor");
}


// draw all menus on screen
void M_Draw (void)
{
	if (cls.key_dest != key_menu && (cls.state != ca_disconnected || cls.key_dest != key_console))
		return;

	Draw_Fill (0, 0, viddef.width, viddef.height, RGBA(0, 0, 0, 1));
	Menuscreens_Animate ();
	if (mstate.state == mstate_steady)
		Menu_DrawHighlight ();
	M_Draw_Cursor();
}

// send key presses to the appropriate menu
void M_Keydown (int key)
{
	const char *s;
	
	if (mstate.state != mstate_steady)
		return;
	
	if (key == K_ESCAPE && mstate.active.num_layers > 0)
	{
		if ((s = layergroup_last(mstate.active).key (layergroup_last(mstate.active).screen, key)))
			S_StartLocalSound (s);
		return;
	}
	
	if (cursor.menulayer == -1)
		M_Main_Key (key);
	else if (activelayer(cursor.menulayer).key != NULL && (s = activelayer(cursor.menulayer).key (activelayer(cursor.menulayer).screen, key)))
		S_StartLocalSound (s);
}

// send mouse movement to the appropriate menu
void M_Think_MouseCursor (void)
{
	int coordidx;
	menuframework_s *m; 
	char * sound = NULL;
	
	if (mstate.state != mstate_steady)
		return;
	
	coordidx = activelayer_coordidx (cursor.x);
	if (coordidx < 0)
	{
		CheckMainMenuMouse ();
		return;
	}
	
	if (cursor.buttondown[MOUSEBUTTON2] && cursor.buttonclicks[MOUSEBUTTON2] == 2 && !cursor.buttonused[MOUSEBUTTON2])
	{
		M_PopMenu ();
		
		// we've "used" the click sequence and will begin another
		refreshCursorButton (MOUSEBUTTON2);
		S_StartLocalSound (menu_out_sound);
		return;
	}
	
	if (coordidx == mstate.active.num_layers)
	{
		if (cursor.mouseaction)
			cursor.menuitem = NULL;
		return;
	}
	
	if (coordidx != cursor.menulayer && cursor.mouseaction)
		Cursor_SelectMenu(activelayer(coordidx).screen);
	
	Menu_AssignCursor (activelayer(coordidx).screen);
	
	if (cursor.menuitem == NULL)
		return;
	
	m = cursor.menuitem->generic.parent;
	
	if (!m)
		return;
	
	if (cursor.buttondown[MOUSEBUTTON1] && !cursor.suppress_drag)
	{
		if (cursor.click_menuitem != NULL)
			Cursor_SelectItem (cursor.click_menuitem);
		else if (cursor.menuitem != NULL)
			cursor.click_menuitem = cursor.menuitem;
	}
	else
		cursor.click_menuitem = NULL;
	
	if (!cursor.buttondown[MOUSEBUTTON1])
		cursor.suppress_drag = false;
	else if (!cursor.menuitem)
		cursor.suppress_drag = true;
	
	if (cursor.suppress_drag || cursor.menuitem == NULL)
		return;
	
	//MOUSE1
	if (cursor.buttondown[MOUSEBUTTON1])
	{
		if (cursor.menuitem->generic.type == MTYPE_SLIDER)
		{
			Menu_DragSlideItem ();
		}
		else if (cursor.menuitem->generic.type == MTYPE_VERT_SCROLLBAR)
		{
			Menu_DragVertScrollItem ();
		}
		else if (!cursor.buttonused[MOUSEBUTTON1])
		{
			if (cursor.menuitem->generic.type == MTYPE_SPINCONTROL)
				Menu_SlideItem (1);
			else
				Menu_ActivateItem (cursor.menuitem);
			
			// we've "used" the click sequence and will begin another
			refreshCursorButton (MOUSEBUTTON1);
			sound = menu_move_sound;
		}
	}
	//MOUSE2
	else if (cursor.buttondown[MOUSEBUTTON2] && !cursor.buttonused[MOUSEBUTTON2])
	{
		if (cursor.menuitem->generic.type == MTYPE_SPINCONTROL)
			Menu_SlideItem (-1);
		else if (cursor.menuitem->generic.type == MTYPE_SLIDER)
			Menu_ClickSlideItem ();
		else
			return;
		
		// we've "used" the click sequence and will begin another
		refreshCursorButton (MOUSEBUTTON2);
		sound = menu_move_sound;
	}

	if ( sound )
		S_StartLocalSound( sound );
} 
