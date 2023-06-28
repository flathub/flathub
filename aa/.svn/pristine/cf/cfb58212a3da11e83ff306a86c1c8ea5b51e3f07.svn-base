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

// q_shared.h -- included first by ALL program modules

#if !defined Q_SHARED_H_
#define Q_SHARED_H_

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

typedef unsigned char 		byte;

// TODO: check for implementation defined boolean
// typedef enum {false, true}	qboolean;
#ifndef true
# define false 0
# define true 1
#endif
typedef int qboolean;

#ifndef NULL
#define NULL ((void *)0)
#endif

// GCC-only intrinsics to tell the compiler what you're expecting
#ifdef __GNUC__
#define likely(expr)    __builtin_expect((expr), !0)
#define unlikely(expr)  __builtin_expect((expr), 0
#else
#define likely
#define unlikely
#endif


// angle indexes
#define	PITCH				0		// up / down
#define	YAW					1		// left / right
#define	ROLL				2		// fall over

#define	MAX_STRING_CHARS	1024	// max length of a string passed to Cmd_TokenizeString
#define	MAX_STRING_TOKENS	80		// max tokens resulting from Cmd_TokenizeString
#define	MAX_TOKEN_CHARS		1024		// max length of an individual token

// MAX_QPATH must be 64 for Alien Arena client/server protocol.
#define	MAX_QPATH			64		// max length of a quake game pathname

#if defined UNIX_VARIANT
#define	MAX_OSPATH			256		// max length of a filesystem pathname
#else
#define	MAX_OSPATH			128		// max length of a filesystem pathname
#endif

#define PLAYERNAME_GLYPHS  15  // maximum visible characters in a player name
#define PLAYERNAME_SIZE    32  // maximum bytes in player name string including color escapes

//
// per-level limits
//
#define	MAX_CLIENTS			256		// absolute limit
#define	MAX_EDICTS			1024	// must change protocol to increase more
#define	MAX_LIGHTSTYLES		256
#define	MAX_MODELS			256		// these are sent over the net as bytes
#define	MAX_SOUNDS			256		// so they cannot be blindly increased
#define	MAX_IMAGES			256
#define	MAX_ITEMS			256
#define MAX_GENERAL			(MAX_CLIENTS*2)	// general config strings


// game print flags
#define	PRINT_LOW			0		// pickup messages
#define	PRINT_MEDIUM		1		// death messages
#define	PRINT_HIGH			2		// critical messages
#define	PRINT_CHAT			3		// chat messages



#define	ERR_FATAL			0		// exit the entire game with a popup window
#define	ERR_DROP			1		// print to console and disconnect from game
#define	ERR_DISCONNECT		2		// don't kill server

#define	PRINT_ALL			0
#define PRINT_DEVELOPER		1		// only print when "developer 1"
#define PRINT_ALERT			2


// destination class for gi.multicast()
typedef enum
{
MULTICAST_ALL,
MULTICAST_PHS,
MULTICAST_PVS,
MULTICAST_ALL_R,
MULTICAST_PHS_R,
MULTICAST_PVS_R
} multicast_t;


/*
==============================================================

MATHLIB

==============================================================
*/

typedef float vec_t;
typedef vec_t vec2_t[2];
typedef vec_t vec3_t[3];
typedef vec_t vec4_t[4];
typedef vec_t vec5_t[5];

typedef	int	fixed4_t;
typedef	int	fixed8_t;
typedef	int	fixed16_t;

typedef struct matrix3x3_s
{
	vec3_t a;
	vec3_t b;
	vec3_t c;
}
matrix3x3_t;

typedef struct matrix3x4_s
{
	vec4_t a;
	vec4_t b;
	vec4_t c;
}
matrix3x4_t;

typedef struct matrix4x4_s
{
	vec4_t a;
	vec4_t b;
	vec4_t c;
	vec4_t d;
}
matrix4x4_t;

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

struct cplane_s;

extern vec3_t vec3_origin;

#define	nanmask (255<<23)

#define	IS_NAN(x) (((*(int *)&x)&nanmask)==nanmask)

#define Q_ftol( f ) ( long ) (f)

#define Vector2Copy(i,o)			((o)[0]=(i)[0],(o)[1]=(i)[1])
#define Vector2Set(v, x, y)			((v)[0]=(x), (v)[1]=(y))
#define Vector2Subtract(a,b,o)		((o)[0]=(a)[0]-(b)[0],(o)[1]=(a)[1]-(b)[1])
#define Vector2Add(a,b,o)			((o)[0]=(a)[0]+(b)[0],(o)[1]=(a)[1]+(b)[1])
#define Vector2Clear(a)				((a)[0]=(a)[1]=0)

void Vector2MA (const vec2_t veca, float scale, const vec2_t vecb, vec2_t vecc);
vec_t Vector2Normalize (vec2_t v);		// returns vector length

#define DotProduct(x,y)				((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define VectorSubtract(a,b,o)		((o)[0]=(a)[0]-(b)[0],(o)[1]=(a)[1]-(b)[1],(o)[2]=(a)[2]-(b)[2])
#define VectorAdd(a,b,o)			((o)[0]=(a)[0]+(b)[0],(o)[1]=(a)[1]+(b)[1],(o)[2]=(a)[2]+(b)[2])
#define VectorCopy(i,o)				((o)[0]=(i)[0],(o)[1]=(i)[1],(o)[2]=(i)[2])
#define VectorClear(a)				((a)[0]=(a)[1]=(a)[2]=0)
#define VectorNegate(i,o)			((o)[0]=-(i)[0],(o)[1]=-(i)[1],(o)[2]=-(i)[2])
#define VectorSet(v, x, y, z)		((v)[0]=(x), (v)[1]=(y), (v)[2]=(z))
#define	SnapVector(v)				{(v)[0]=(int)(v)[0];(v)[1]=(int)(v)[1];(v)[2]=(int)(v)[2];}
// o will be the elementwise product (i.e. Hadamard product) of a and b:
#define VectorComponentMul(a,b,o)	((o)[0]=(a)[0]*(b)[0],(o)[1]=(a)[1]*(b)[1],(o)[2]=(a)[2]*(b)[2])

void VectorMA (const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc);

#define Vector4Set(v, a, b, c, d)	((v)[0]=(a),(v)[1]=(b),(v)[2]=(c),(v)[3] = (d))
#define Vector4Copy(a,b)		((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2],(b)[3]=(a)[3])
#define Vector4Scale(in,scale,out)		((out)[0]=(in)[0]*scale,(out)[1]=(in)[1]*scale,(out)[2]=(in)[2]*scale,(out)[3]=(in)[3]*scale)
#define Vector4Add(a,b,c)		((c)[0]=(((a[0])+(b[0]))),(c)[1]=(((a[1])+(b[1]))),(c)[2]=(((a[2])+(b[2]))),(c)[3]=(((a[3])+(b[3]))))
#define Vector4Sub(a,b,c)		((c)[0]=(((a[0])-(b[0]))),(c)[1]=(((a[1])-(b[1]))),(c)[2]=(((a[2])-(b[2]))),(c)[3]=(((a[3])-(b[3]))))
#define Vector4Avg(a,b,c)		((c)[0]=(((a[0])+(b[0]))*0.5f),(c)[1]=(((a[1])+(b[1]))*0.5f),(c)[2]=(((a[2])+(b[2]))*0.5f),(c)[3]=(((a[3])+(b[3]))*0.5f))
#define Vector4Clear(a)			((a)[0]=(a)[1]=(a)[2]=(a)[3]=0)

#define DEG2RAD( a ) (( (a) * (float)M_PI ) / 180.0F)
#define RAD2DEG( a ) (( (a) * 180.0F ) / (float)M_PI)

// just in case you do't want to use the macros
vec_t _DotProduct (vec3_t v1, vec3_t v2);
void _VectorSubtract (vec3_t veca, vec3_t vecb, vec3_t out);
void _VectorAdd (vec3_t veca, vec3_t vecb, vec3_t out);
void _VectorCopy (vec3_t in, vec3_t out);

void ClearBounds (vec3_t mins, vec3_t maxs);
void AddPointToBounds (vec3_t v, vec3_t mins, vec3_t maxs);
int VectorCompare (vec3_t v1, vec3_t v2);
vec_t VectorLength (vec3_t v);
void CrossProduct (const vec3_t v1, const vec3_t v2, vec3_t cross);
vec_t VectorNormalize (vec3_t v);		// returns vector length
vec_t VectorNormalize2 (vec3_t v, vec3_t out);
void VectorInverse (vec3_t v);
void VectorScale (const vec3_t in, vec_t scale, vec3_t out);
int Q_log2(int val);

void AngleVectors (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
void vectoangles (vec3_t value1, vec3_t angles);
int BoxOnPlaneSide (const vec3_t emins, const vec3_t emaxs, const struct cplane_s *plane);
float	anglemod(float a);
float LerpAngle (float a1, float a2, float frac);

#define BOX_ON_PLANE_SIDE(emins, emaxs, p)	\
	(((p)->type < 3)?						\
	(										\
		((p)->dist <= (emins)[(p)->type])?	\
			1								\
		:									\
		(									\
			((p)->dist >= (emaxs)[(p)->type])?\
				2							\
			:								\
				3							\
		)									\
	)										\
	:										\
		BoxOnPlaneSide( (emins), (emaxs), (p)))

void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees );


//=============================================

/* FIXME: Beware - _vsnprintf does not end with \0 - vsnprintf (*nix) does */
#if defined WIN32_VARIANT
#define vsnprintf	_vsnprintf
#endif

const char *COM_SkipPath (const char *pathname);
void COM_StripExtension (const char *in, char *out);
qboolean COM_HasExtension (const char *path, const char *extension);
char *COM_FileExtension (char *in);
void COM_FileBase (char *in, char *out);
void COM_FilePath (char *in, char *out);
void COM_DefaultExtension (char *path, char *extension);

char *COM_Parse (const char **data_p);
// data is an in/out parm, returns a parsed out token

char *Com_ParseExt (char **data_p, qboolean allowNewLines);
char *Com_SkipWhiteSpace (char *data_p, qboolean *hasNewLines);
void Com_SkipRestOfLine (char **data_p);

int com_parseLine;

void Com_sprintf (char *dest, int size, char *fmt, ...);

void Com_PageInMemory (byte *buffer, int size);

//=============================================

// portable case insensitive compare
int Q_strcasecmp (const char *s1, const char *s2);
int Q_strncasecmp (const char *s1, const char *s2, int n);
void Q_strcat (char *dst, const char *src, int dstSize);
int Q_strnicmp (const char *string1, const char *string2, int n);
char *Q_strlwr(char *s);
//
void Q_strncpyz2 (char *dst, const char *src, int dstSize);

//=============================================

short	BigShort(short l);
short	LittleShort(short l);
int		BigLong (int l);
int		LittleLong (int l);
float	BigFloat (float l);
float	LittleFloat (float l);

void	Swap_Init (void);
char	*va(char *format, ...);
float	*atv(void);
float	*tv (float x, float y, float z);

//=============================================

//
// key / value info strings
//
#define	MAX_INFO_KEY		64
#define	MAX_INFO_VALUE		64
#define	MAX_INFO_STRING		512

char *Info_ValueForKey (const char *s, const char *key);
qboolean Info_KeyExists (const char *s, const char *key);
void Info_RemoveKey (char *s, const char *key);
void Info_SetValueForKey (char *s, const char *key, const char *value);
qboolean Info_Validate (const char *s);

size_t ValidatePlayerName( char *player_name, size_t player_name_size );

/*
==============================================================

SYSTEM SPECIFIC

==============================================================
*/

extern	int	curtime;		// time returned by last Sys_Milliseconds

int		Sys_Milliseconds (void);
void	Sys_Mkdir (char *path);

// large block stack allocation routines
void	*Hunk_Begin (int maxsize);
void	*Hunk_Alloc (int size);
void	Hunk_Free (void *buf);
int		Hunk_End (void);

// directory searching
#define SFF_ARCH    0x01
#define SFF_HIDDEN  0x02
#define SFF_RDONLY  0x04
#define SFF_SUBDIR  0x08
#define SFF_SYSTEM  0x10

/*
** pass in an attribute mask of things you wish to REJECT
*/
char	*Sys_FindFirst (char *path, unsigned musthave, unsigned canthave );
char	*Sys_FindNext ( unsigned musthave, unsigned canthave );
void	Sys_FindClose (void);


// this is only here so the functions in q_shared.c and q_shwin.c can link
void Sys_Error (char *error, ...);
void Com_Printf (char *msg, ...);

/*
==========================================================

CVARS (console variables)

==========================================================
*/

#ifndef CVAR
#define	CVAR

// These are all the flags that actually do anything.
// NOTE: there is no need to maintain any kind of compatibility, we can change
// these if we want to.
#define	CVAR_ARCHIVE	1	// set to cause it to be saved to vars.rc
#define	CVAR_USERINFO	2	// added to userinfo  when changed
#define	CVAR_SERVERINFO	4	// added to serverinfo when changed
#define	CVAR_NOSET		8	// don't allow change from console at all,
							// but can be set from the command line
#define	CVAR_LATCH		16	// save changes until server restart
#define CVAR_ROM		32  // cannot be set by user at all
#define CVAR_GAMEINFO   64  // added to the 'mods' field of the serverinfo
                            // string when changed
#define CVAR_PROFILE	128 // profile information

// These flags are purely for documentation (used by the "help" command.)
// Usually, at most one of these will be set. They are not "enforced" by the
// code in any way.
// TODO: go through and add all these flags for cvars as appropriate.
// TODO: others like address, path, IP address, etc?
#define CVARDOC_BOOL	512
#define CVARDOC_STR		1024
#define CVARDOC_FLOAT	2048
#define CVARDOC_INT		4096

// nothing outside the Cvar_*() functions should modify these fields!
typedef struct cvar_s
{
	char		*name;
	unsigned int	hash_key;
	char		*string;
	char		*latched_string;	// for CVAR_LATCH vars
	int		flags;
	qboolean	modified;		// set each time the cvar is changed
	float		value;
	float		default_value;
	int		integer;
	char		*description; // Optional (may be NULL)
	struct cvar_s	*next;
} cvar_t;

#endif		// CVAR

/*
==============================================================

COLLISION DETECTION

==============================================================
*/

// lower bits are stronger, and will eat weaker brushes completely
#define	CONTENTS_SOLID			1		// an eye is never valid in a solid
#define	CONTENTS_WINDOW			2		// translucent, but not watery
#define	CONTENTS_AUX			4
#define	CONTENTS_LAVA			8
#define	CONTENTS_SLIME			16
#define	CONTENTS_WATER			32
#define	CONTENTS_MIST			64
#define	LAST_VISIBLE_CONTENTS	64

// remaining contents are non-visible, and don't eat brushes

#define	CONTENTS_AREAPORTAL		0x8000

#define	CONTENTS_PLAYERCLIP		0x10000
#define	CONTENTS_MONSTERCLIP	0x20000

// currents can be added to any other contents, and may be mixed
#define	CONTENTS_CURRENT_0		0x40000
#define	CONTENTS_CURRENT_90		0x80000
#define	CONTENTS_CURRENT_180	0x100000
#define	CONTENTS_CURRENT_270	0x200000
#define	CONTENTS_CURRENT_UP		0x400000
#define	CONTENTS_CURRENT_DOWN	0x800000

#define	CONTENTS_ORIGIN			0x1000000	// removed before bsping an entity

#define	CONTENTS_MONSTER		0x2000000	// should never be on a brush, only in game
#define	CONTENTS_DEADMONSTER	0x4000000
#define	CONTENTS_DETAIL			0x8000000	// brushes to be added after vis leafs
#define	CONTENTS_TRANSLUCENT	0x10000000	// auto set if any surface has trans
#define	CONTENTS_LADDER			0x20000000

#define	SURF_LIGHT		0x1		// value will hold the light strength

#define	SURF_SLICK		0x2		// effects game physics

#define	SURF_SKY		0x4		// don't draw, but add to skybox
#define	SURF_WARP		0x8		// turbulent water warp
#define	SURF_TRANS33	0x10
#define	SURF_TRANS66	0x20
#define	SURF_FLOWING	0x40	// scroll towards angle
#define	SURF_NODRAW		0x80	// don't bother referencing the texture
#define SURF_BLOOD		0x400	// dripping blood surface
#define SURF_WATER		0x800	// dripping water surface
#define SURF_SHINY		0x1000  // shiny wet surface
#define SURF_UNDERWATER	0x2000	// reflecting ripples; applied automatically 
								// to underwater surfs but could be applied
								// manually as well
#define SURF_NOSHADOW	0x4000  // don't draw shadowmaps on this surface

// content masks
#define	MASK_ALL				(-1)
#define	MASK_SOLID				(CONTENTS_SOLID|CONTENTS_WINDOW)
#define	MASK_PLAYERSOLID		(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define	MASK_DEADSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW)
#define	MASK_MONSTERSOLID		(CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define	MASK_WATER				(CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define	MASK_OPAQUE				(CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define	MASK_SHOT				(CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEADMONSTER)
#define MASK_CURRENT			(CONTENTS_CURRENT_0|CONTENTS_CURRENT_90|CONTENTS_CURRENT_180|CONTENTS_CURRENT_270|CONTENTS_CURRENT_UP|CONTENTS_CURRENT_DOWN)
#define MASK_VISIBILILITY   (CONTENTS_SOLID/*|CONTENTS_WINDOW*/|CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)

// gi.BoxEdicts() can return a list of either solid or trigger entities
// FIXME: eliminate AREA_ distinction?
#define	AREA_SOLID		1
#define	AREA_TRIGGERS	2

// plane_t structure
// !!! if this is changed, it must be changed in asm code too !!!
typedef struct cplane_s
{
	vec3_t	normal;
	float	dist;
	byte	type;			// for fast side tests
	byte	signbits;		// signx + (signy<<1) + (signz<<1)
	byte	pad[2];
} cplane_t;

typedef struct cmodel_s
{
	vec3_t			mins, maxs;
	vec3_t			origin;		// for sounds or lights
	int				headnode;
} cmodel_t;

typedef struct csurface_s
{
	char		name[16];
	int			flags;
	int			value;
} csurface_t;

typedef struct mapsurface_s  // used internally due to name len probs //ZOID
{
	csurface_t	c;
	char		rname[32];
} mapsurface_t;

// a trace is returned when a box is swept through the world
typedef struct
{
	qboolean	allsolid;	// if true, plane is not valid
	qboolean	startsolid;	// if true, the initial point was in a solid area
	float		fraction;	// time completed, 1.0 = didn't hit anything
	vec3_t		endpos;		// final position
	cplane_t	plane;		// surface normal at impact
	csurface_t	*surface;	// surface hit
	int			contents;	// contents on other side of surface hit
	struct edict_s	*ent;		// not set by CM_*() functions
} trace_t;

// pmove_state_t is the information necessary for client side movement
// prediction
typedef enum
{
	// can accelerate and turn
	PM_NORMAL,
	PM_SPECTATOR,
	// no acceleration or turning
	PM_DEAD,
	PM_GIB,		// different bounding box
	PM_FREEZE
} pmtype_t;

// pmove->pm_flags
#define	PMF_DUCKED			1
#define	PMF_JUMP_HELD		2
#define	PMF_ON_GROUND		4
#define	PMF_TIME_WATERJUMP	8	// pm_time is waterjump
#define	PMF_TIME_LAND		16	// pm_time is time before rejump
#define	PMF_TIME_TELEPORT	32	// pm_time is non-moving time
#define PMF_NO_PREDICTION	64	// temporarily disables prediction (used for grappling hook)
#define PMF_SNEAKING		128 // for sneaking movement, silent, slow

// this structure needs to be communicated bit-accurate
// from the server to the client to guarantee that
// prediction stays in sync, so no floats are used.
// if any part of the game code modifies this struct, it
// will result in a prediction error of some degree.
typedef struct
{
	pmtype_t	pm_type;

	int 		origin[3];		// 12.3
	short		velocity[3];	// 12.3
	byte		pm_flags;		// ducked, jump_held, etc
	byte		pm_time;		// each unit = 8 ms
	short		gravity;
	short		delta_angles[3];	// add to command angles to get view direction
									// changed by spawns, rotating objects, and teleporters
} pmove_state_t;

//
// button bits
//
#define	BUTTON_ATTACK		1
#define	BUTTON_USE			2
#define BUTTON_ATTACK2		4
#define BUTTON_LEANLEFT		8
#define BUTTON_LEANRIGHT	16
#define BUTTON_ZOOM			32
#define BUTTON_SNEAK		64
#define	BUTTON_ANY			128			// any key whatsoever

// usercmd_t is sent to the server each client frame
typedef struct usercmd_s
{
	byte	msec;
	byte	buttons;
	short	angles[3];
	short	forwardmove, sidemove, upmove;
	byte	impulse;		// remove?
	byte	lightlevel;		// light level the player is standing on
} usercmd_t;

#define	MAXTOUCH	32
typedef struct
{
	// state (in / out)
	pmove_state_t	s;

	// command (in)
	usercmd_t		cmd;
	qboolean		snapinitial;	// if s has been changed outside pmove

	// results (out)
	int			numtouch;
	struct edict_s	*touchents[MAXTOUCH];

	vec3_t		viewangles;			// clamped
	float		viewheight;

	vec3_t		mins, maxs;			// bounding box size

	struct edict_s	*groundentity;
	int			watertype;
	int			waterlevel;
	int			joustattempts;

	// callbacks to test the world
	trace_t		(*trace) (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);
	int			(*pointcontents) (vec3_t point);
} pmove_t;

// entity_state_t->effects
// Effects are things handled on the client side (lights, particles, frame animations)
// that happen constantly on the given entity.
// An entity that has effects will be sent to the client
// even if it has a zero index model.
#define	EF_ROTATE			0x00000001		// rotate (bonus items)
#define	EF_GIB				0x00000002		// leave a trail
#define EF_FLAMETHROWER		0x00000004
#define	EF_BLASTER			0x00000008		
#define	EF_ROCKET			0x00000010		
#define	EF_FIRE				0x00000020
#define	EF_HYPERBLASTER		0x00000040
#define	EF_SPAWNPROTECTED	0x00000080
#define EF_COLOR_SHELL		0x00000100
#define EF_SILENT			0x00000200
#define	EF_ANIM01			0x00000400		// automatically cycle between frames 0 and 1 at 2 hz
#define	EF_ANIM23			0x00000800		// automatically cycle between frames 2 and 3 at 2 hz
#define EF_ANIM_ALL			0x00001000		// automatically cycle through all frames at 2hz
#define EF_ANIM_ALLFAST		0x00002000		// automatically cycle through all frames at 10hz
#define	EF_BUBBLES			0x00004000
#define	EF_QUAD				0x00008000
#define	EF_PENT				0x00010000
#define	EF_TELEPORTER		0x00020000		// particle fountain
#define EF_TEAM1			0x00040000
#define EF_TEAM2			0x00080000
#define EF_BLUEMZF			0x00100000
#define EF_GREENGIB			0x00200000
#define EF_CHAINGUN			0x00400000
#define EF_GREENMZF			0x00800000
#define EF_PLASMA			0x01000000
#define EF_SHIPEXHAUST  	0x02000000
#define EF_SHOCKBALL		0x04000000
#define EF_SMARTMZF			0x08000000
#define EF_PLASMAMZF		0x10000000
#define EF_ROCKETMZF		0x20000000
#define EF_MEMZF			0x40000000
#define EF_ROCKETEXHAUST	0x80000000

// entity_state_t->renderfx flags
#define	RF_MINLIGHT			1		// allways have some light (viewmodel)
#define	RF_VIEWERMODEL		2		// don't draw through eyes, only mirrors
#define	RF_WEAPONMODEL		4		// only draw through eyes
#define	RF_FULLBRIGHT		8		// allways draw full intensity
#define	RF_DEPTHHACK		16		// for view weapon Z crunching
#define	RF_TRANSLUCENT		32
#define	RF_FRAMELERP		64
#define RF_BEAM				128		// unused
#define	RF_CUSTOMSKIN		256		// skin is an index in image_precache
#define	RF_GLOW				512		// pulse lighting for bonus items
#define RF_SHELL_RED		1024
#define	RF_SHELL_GREEN		2048
#define RF_SHELL_BLUE		4096
#define RF_BOBBING			0x00008000		// 32768
#define	RF_SHELL_DOUBLE		0x00010000		// 65536
#define	RF_SHELL_HALF_DAM	0x00020000
#define RF_NOSHADOWS		0x00040000 //use this one for turning off shadows, etc.
#define RF_MONSTER  		0x00080000
#define	RF_NODRAW			0x00100000 //use this instead of a 0 modelindex for compatibility.
#define RF_MENUMODEL		0x01280000 //for player menu

#define RF_SHELL_ANY		(RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_HALF_DAM | RF_SHELL_DOUBLE)


// player_state_t->refdef flags
#define	RDF_UNDERWATER		1		// warp the screen as apropriate
#define RDF_NOWORLDMODEL	2		// used for player configuration screen
#define	RDF_IRGOGGLES		4
#define RDF_UVGOGGLES		8

//
// muzzle flashes / player effects
//
#define	MZ_BLASTER			0
#define MZ_MACHINEGUN		1
#define	MZ_SHOTGUN			2
#define	MZ_CHAINGUN1		3
#define	MZ_CHAINGUN2		4
#define	MZ_CHAINGUN3		5
#define	MZ_RAILGUN			6
#define	MZ_ROCKET			7
#define	MZ_GRENADE			8
#define	MZ_LOGIN			9
#define	MZ_LOGOUT			10
#define	MZ_RESPAWN			11
#define	MZ_BFG				12
#define	MZ_SSHOTGUN			13
#define	MZ_HYPERBLASTER		14
#define	MZ_ITEMRESPAWN		15
#define MZ_SILENCED			128		// bit flag ORed with one of the above numbers

// temp entity events
//
// Temp entity events are for things that happen
// at a location seperate from any existing entity.
// Temporary entity messages are explicitly constructed
// and broadcast.

#define	TE_GUNSHOT					0
#define	TE_BLOOD					1
#define	TE_BLASTER					2
#define	TE_RAILTRAIL				3
#define	TE_LASERBEAM				4
#define	TE_EXPLOSION1				5
#define	TE_EXPLOSION2				6
#define	TE_ROCKET_EXPLOSION			7
#define	TE_SPARKS					9
#define	TE_SPLASH					10
#define	TE_BUBBLETRAIL				11
#define	TE_SCREEN_SPARKS			12
#define	TE_DEATHFIELD2				13
#define	TE_BULLET_SPARKS			14
#define	TE_LASER_SPARKS				15
#define	TE_GREEN_MUZZLEFLASH		16
#define TE_ROCKET_EXPLOSION_WATER	17
#define	TE_REDLASER					19
#define	TE_BFG_BIGEXPLOSION			21
#define	TE_BOSSTPORT				22			
#define	TE_FLAMETHROWER				23
#define	TE_GREENBLOOD				26
#define	TE_LIGHTNING				33
#define	TE_VAPORBEAM				38
#define	TE_STEAM					40
#define	TE_SAYICON					45
#define	TE_TELEPORT_EFFECT			48
#define	TE_CHAINGUNSMOKE			57
#define	TE_BLUE_MUZZLEFLASH			58
#define	TE_SMART_MUZZLEFLASH		59
#define	TE_VOLTAGE					60
#define	TE_DEATHFIELD				61
#define	TE_BLASTERBEAM				62
#define	TE_STAIN					63
#define	TE_FIRE						64
#define	TE_SMOKE					66
#define TE_JETEXHAUST				67
#define TE_DUST						68
// Steamworks stat events
#define TE_PLAYERWON				69
#define TE_PLAYERLOST				70
#define TE_KILL						71
#define TE_FLAGCAPTURE				72
#define TE_HEADSHOT					73
#define TE_GODLIKE					74
#define TE_BASEKILL					75
#define TE_KILLSTREAK				76
#define TE_RAMPAGE					77
#define TE_UNSTOPPABLE				78
#define TE_FLAGRETURN				79
#define TE_MINDERASED				80
#define TE_DISINTEGRATED			81
#define TE_VIOLATED 				82
#define TE_MIDAIRSHOT				83

#define SPLASH_UNKNOWN		0
#define SPLASH_SPARKS		1
#define SPLASH_BLUE_WATER	2
#define SPLASH_BROWN_WATER	3
#define SPLASH_SLIME		4
#define	SPLASH_LAVA			5
#define SPLASH_BLOOD		6


// sound channels
// channel 0 never willingly overrides
// other channels (1-7) allways override a playing sound on that channel
#define	CHAN_AUTO               0
#define	CHAN_WEAPON             1
#define	CHAN_VOICE              2
#define	CHAN_ITEM               3
#define	CHAN_BODY               4
// modifier flags
#define	CHAN_NO_PHS_ADD			8	// send to all clients, not just ones in PHS (ATTN 0 will also do this)
#define	CHAN_RELIABLE			16	// send by reliable message, not datagram


// sound attenuation values
#define	ATTN_NONE               0	// full volume the entire level
#define	ATTN_NORM               1
#define	ATTN_IDLE               2
#define	ATTN_STATIC             3	// diminish very rapidly with distance


// player_state->stats[] indexes
#define STAT_HEALTH_ICON		0
#define	STAT_HEALTH				1
#define	STAT_AMMO_ICON			2
#define	STAT_AMMO				3
#define	STAT_ARMOR_ICON			4
#define	STAT_ARMOR				5
#define	STAT_RED_MATCHES		6
#define	STAT_BLUE_MATCHES		7
#define STAT_TACTICAL_SCORE		8
#define	STAT_TIMER_ICON			9
#define	STAT_TIMER				10
#define	STAT_HELPICON			11
#define	STAT_SELECTED_ITEM		12
#define	STAT_LAYOUTS			13
#define	STAT_FRAGS				14
#define	STAT_FLASHES			15		// cleared each frame, 1 = health, 2 = armor
#define STAT_FLAGS				15		// misc boolean values, use only bits 3-16
#define STAT_CHASE				16
#define STAT_SPECTATOR			17
#define STAT_SCOREBOARD			18
#define STAT_DEATHS				19
#define STAT_HIGHSCORE			20
#define STAT_REDSCORE			21
#define STAT_BLUESCORE			22
#define STAT_FLAG_ICON			23
#define STAT_ZOOMED				24
#define STAT_WEAPN1				25
#define STAT_WEAPN2				26
#define STAT_WEAPN3				27
#define STAT_WEAPN4				28
#define STAT_WEAPN5				29
#define STAT_WEAPN6				30
#define STAT_WEAPN7				31

#define	MAX_STATS				32

// bit flags for use in STAT_FLAGS
#define STAT_FLAGS_CROSSHAIRPOSITION	(4|8)
#define STAT_FLAGS_CROSSHAIRPOS1		0 //default
#define STAT_FLAGS_CROSSHAIRCENTER		4
#define STAT_FLAGS_CROSSHAIRPOS2		8
#define STAT_FLAGS_CROSSHAIRPOS3		12


// dmflags->value flags
#define	DF_NO_HEALTH		0x00000001	// 1
#define	DF_NO_ITEMS			0x00000002	// 2
#define	DF_WEAPONS_STAY		0x00000004	// 4
#define	DF_NO_FALLING		0x00000008	// 8
#define	DF_INSTANT_ITEMS	0x00000010	// 16
#define	DF_SAME_LEVEL		0x00000020	// 32
#define DF_SKINTEAMS		0x00000040	// 64
#define DF_NO_FRIENDLY_FIRE	0x00000100	// 256
#define	DF_SPAWN_FARTHEST	0x00000200	// 512
#define DF_FORCE_RESPAWN	0x00000400	// 1024
#define DF_NO_ARMOR			0x00000800	// 2048
#define DF_ALLOW_EXIT		0x00001000	// 4096
#define DF_INFINITE_AMMO	0x00002000	// 8192
#define DF_QUAD_DROP		0x00004000	// 16384

// RAFAEL
#define	DF_QUADFIRE_DROP	0x00010000	// 65536

//CODERED

#define DF_BOT_AUTOSAVENODES 0x00020000 //131072
#define DF_BOTCHAT			0x00040000 //262144
#define DF_BOT_FUZZYAIM		0x00080000 //524288
#define DF_BOTS			    0x00100000 //1048576
#define DF_BOT_LEVELAD		0x00200000 //2097152

/*
==========================================================

  ELEMENTS COMMUNICATED ACROSS THE NET

==========================================================
*/

#define	ANGLE2SHORT(x)	((int)((x)*65536/360) & 65535)
#define	SHORT2ANGLE(x)	((x)*(360.0/65536))

// Number of bytes per axis of world coordinates in the net protocol.
// 2 is the default, backward-compatible number.
// TODO: make this a variable, have it set based on the map size, server kicks
// clients that don't support big maps.
#define	coord_bytes		2


//
// config strings are a general means of communication from
// the server to all connected clients.
// Each config string can be at most MAX_QPATH characters.
// Alien Arena client/server protocol depends on MAX_QPATH being 64
//
#define	CS_NAME				0
#define	CS_SKY				2
#define	CS_SKYAXIS			3		// %f %f %f format
#define	CS_SKYROTATE		4
#define	CS_STATUSBAR		5		// display program string

#define CS_AIRACCEL			29		// air acceleration control
#define	CS_MAXCLIENTS		30
#define	CS_MAPCHECKSUM		31		// for catching cheater maps

#define	CS_MODELS			32
#define	CS_SOUNDS			(CS_MODELS+MAX_MODELS)
#define	CS_IMAGES			(CS_SOUNDS+MAX_SOUNDS)
#define	CS_LIGHTS			(CS_IMAGES+MAX_IMAGES)
#define	CS_ITEMS			(CS_LIGHTS+MAX_LIGHTSTYLES)
#define	CS_PLAYERSKINS		(CS_ITEMS+MAX_ITEMS)
#define CS_GENERAL			(CS_PLAYERSKINS+MAX_CLIENTS)
#define	MAX_CONFIGSTRINGS	(CS_GENERAL+MAX_GENERAL)


//==============================================


// entity_state_t->event values
// ertity events are for effects that take place reletive
// to an existing entities origin.  Very network efficient.
// All muzzle flashes really should be converted to events...
typedef enum
{
	EV_NONE,
	EV_ITEM_RESPAWN,
	EV_FOOTSTEP,
	EV_FALLSHORT,
	EV_FALL,
	EV_FALLFAR,
	EV_PLAYER_TELEPORT,
	EV_OTHER_TELEPORT,
	EV_WADE
} entity_event_t;


// entity_state_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
typedef struct entity_state_s
{
	int		number;			// edict index

	vec3_t	origin;
	vec3_t	angles;
	vec3_t	old_origin;		// for lerping
	int		modelindex;
	int		modelindex2, modelindex3, modelindex4;	// weapons, CTF flags, etc
	int		frame;
	int		skinnum;
	unsigned int		effects;		// PGM - we're filling it, so it needs to be unsigned
	int		renderfx;
	int		solid;			// for client side prediction, 8*(bits 0-4) is x/y radius
							// 8*(bits 5-9) is z down distance, 8(bits10-15) is z up
							// gi.linkentity sets this properly
	int		sound;			// for looping sounds, to guarantee shutoff
	int		event;			// impulse events -- muzzle flashes, footsteps, etc
							// events only go out for a single frame, they
							// are automatically cleared each frame
	vec3_t	spawn_pos;  //used for remembering the original spawn position of an entity
} entity_state_t;

//==============================================

//bot score info
//data used to track bot scores for server status strings
typedef struct
{
	char name[MAX_INFO_STRING];
	int score;
	int dmteam;
} bot_t;

// player_state_t is the information needed in addition to pmove_state_t
// to rendered a view.  There will only be 10 player_state_t sent each second,
// but the number of pmove_state_t changes will be reletive to client
// frame rates
typedef struct
{
	pmove_state_t	pmove;		// for prediction

	// these fields do not need to be communicated bit-precise

	vec3_t		viewangles;		// for fixed views
	vec3_t		viewoffset;		// add to pmovestate->origin
	vec3_t		kick_angles;	// add to view direction to get render angles
								// set by weapon kicks, pain effects, etc

	vec3_t		gunangles;
	vec3_t		gunoffset;
	int			gunindex;
	int			gunframe;

	float		blend[4];		// rgba full screen effect

	float		fov;			// horizontal field of view

	int			rdflags;		// refdef flags

	short		stats[MAX_STATS];		// fast status bar updates

	//bot score info
	int botnum;
	bot_t bots[100];

} player_state_t;

//colored text
//=============================================

#define Q_COLOR_ESCAPE	'^'
#define Q_IsColorString(p)	( p && *(p) == Q_COLOR_ESCAPE && *((p)+1) && *((p)+1) != Q_COLOR_ESCAPE )

#define COLOR_BLACK		'0'
#define COLOR_RED		'1'
#define COLOR_GREEN		'2'
#define COLOR_YELLOW	'3'
#define COLOR_BLUE		'4'
#define COLOR_CYAN		'5'
#define COLOR_MAGENTA	'6'
#define COLOR_WHITE		'7'
#define ColorIndex(c)	( ( (c) - '0' ) & 7 )

#define	COLOR_R(rgba)		((rgba) & 0xFF)
#define	COLOR_G(rgba)		(((rgba) >> 8) & 0xFF)
#define	COLOR_B(rgba)		(((rgba) >> 16) & 0xFF)
#define	COLOR_A(rgba)		(((rgba) >> 24) & 0xFF)
#define COLOR_RGB(r,g,b)	(((r) << 0)|((g) << 8)|((b) << 16))
#define COLOR_RGBA(r,g,b,a) (((r) << 0)|((g) << 8)|((b) << 16)|((a) << 24))

#define S_COLOR_BLACK	"^0"
#define S_COLOR_RED		"^1"
#define S_COLOR_GREEN	"^2"
#define S_COLOR_YELLOW	"^3"
#define S_COLOR_BLUE	"^4"
#define S_COLOR_CYAN	"^5"
#define S_COLOR_MAGENTA	"^6"
#define S_COLOR_WHITE	"^7"

//unlagged - lag simulation #2
#define MAX_LATENT_CMDS 64
//unlagged - lag simulation #2

//
// types of compression
//
enum compressiontypes_e  {
    compression_zlib_raw,
    compression_lzo,
    compression_zlib_header,
    last_compressiontype_known
};

#endif  /* Q_SHARED_H_ */
