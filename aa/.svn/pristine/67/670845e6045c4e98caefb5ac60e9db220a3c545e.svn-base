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

// game.h -- game module information visible to server

#define	GAME_API_VERSION	3

// edict->svflags

#define	SVF_NOCLIENT			0x00000001	// don't send entity to clients, even if it has effects
#define	SVF_DEADMONSTER			0x00000002	// treat as CONTENTS_DEADMONSTER for collision
#define	SVF_MONSTER				0x00000004	// treat as CONTENTS_MONSTER for collision
#define SVF_FAKECLIENT		0x00000020	// do not try to send anything to this client

// edict->solid values

typedef enum
{
	SOLID_NOT,			// no interaction with other objects
	SOLID_TRIGGER,		// only touch when inside, after moving
	SOLID_BBOX,			// touch on edge
	SOLID_BSP			// bsp clip, touch on edge
} solid_t;

//===============================================================

// link_t is only used for entity area links now
typedef struct link_s
{
	struct link_s	*prev, *next;
} link_t;

#define	MAX_ENT_CLUSTERS	16


typedef struct edict_s edict_t;
typedef struct gclient_s gclient_t;


#ifndef GAME_INCLUDE

struct gclient_s
{
	player_state_t	ps;		// communicated by server to clients
	int				ping;

	// the game dll can add anything it wants after
	// this point in the structure

};


struct edict_s
{
	entity_state_t	s;
	struct gclient_s	*client;
	qboolean	inuse;
	int			linkcount;

	// FIXME: move these fields to a server private sv_entity_t
	link_t		area;				// linked to a division node or leaf
	
	int			num_clusters;		// if -1, use headnode instead
	int			clusternums[MAX_ENT_CLUSTERS];
	int			headnode;			// unused if num_clusters != -1
	int			areanum, areanum2;

	//================================

	int			svflags;			// SVF_NOCLIENT, SVF_DEADMONSTER, SVF_MONSTER, etc
	vec3_t		mins, maxs;
	vec3_t		absmin, absmax, size;
	solid_t		solid;
	int			clipmask;
	edict_t		*owner;
	
	int			redirect_number;	//for ghost mode
	
	// must be accessible to server code for collision detection
	int			dmteam;
	int			teamset;
#define RED_TEAM				0
#define BLUE_TEAM				1
#define NO_TEAM					2
#define ALIEN_TEAM				3
#define HUMAN_TEAM				4

	// the game dll can add anything it wants after
	// this point in the structure
};

#endif		// GAME_INCLUDE

//===============================================================

//
// functions provided by the main engine
//
typedef struct
{

	// special messages
	void	(*bprintf) (int printlevel, char *fmt, ...);
	void	(*dprintf) (char *fmt, ...);
	void	(*cprintf) (edict_t *ent, int printlevel, char *fmt, ...);
	void	(*centerprintf) (edict_t *ent, char *fmt, ...);
	void	(*sound) (edict_t *ent, int channel, int soundindex, float volume, float attenuation, float timeofs);
	void	(*positioned_sound) (vec3_t origin, edict_t *ent, int channel, int soundinedex, float volume, float attenuation, float timeofs);

	// config strings hold all the index strings, the lightstyles,
	// and misc data like the sky definition and cdtrack.
	// All of the current configstrings are sent to clients when
	// they connect, and changes are sent to all connected clients.
	void	(*configstring) (int num, char *string);

	void	(*error) (char *fmt, ...);

	// the *index functions create configstrings and some internal server state
	int		(*modelindex) (char *name);
	int		(*soundindex) (char *name);
	int		(*imageindex) (char *name);
	
	// for checking if assets are already loaded - return 0 if they are not
	int		(*checkmodelindex) (char *name);
	int		(*checksoundindex) (char *name);
	int		(*checkimageindex) (char *name);

	void	(*setmodel) (edict_t *ent, char *name);

	// collision detection
	trace_t	(*trace) (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, edict_t *passent, int contentmask);
	int		(*pointcontents) (vec3_t point);
	qboolean	(*inPVS) (vec3_t p1, vec3_t p2);
	qboolean	(*inPHS) (vec3_t p1, vec3_t p2);
	void		(*SetAreaPortalState) (int portalnum, qboolean open);
	qboolean	(*AreasConnected) (int area1, int area2);

	// an entity will never be sent to a client or used for collision
	// if it is not passed to linkentity.  If the size, position, or
	// solidity changes, it must be relinked.
	void	(*linkentity) (edict_t *ent);
	void	(*unlinkentity) (edict_t *ent);		// call before removing an interactive edict
	int		(*BoxEdicts) (vec3_t mins, vec3_t maxs, edict_t **list,	int maxcount, int areatype);
	void	(*Pmove) (pmove_t *pmove);		// player movement code common with client prediction

	// network messaging
	void	(*multicast) (vec3_t origin, multicast_t to);
	void	(*unicast) (edict_t *ent, qboolean reliable);
	void	(*WriteChar) (int c);
	void	(*WriteByte) (int c);
	void	(*WriteShort) (int c);
	void	(*WriteLong) (int c);
	void	(*WriteFloat) (float f);
	void	(*WriteString) (char *s);
	void	(*WritePosition) (vec3_t pos);	// some fractional bits
	void	(*WriteDir) (vec3_t pos);		// single byte encoded, very coarse
	void	(*WriteAngle) (float f);

	// managed memory allocation
	void	*(*TagMalloc) (int size, int tag);
	void	(*TagFree) (void *block);
	void	(*FreeTags) (int tag);

	// console variable interaction
	cvar_t	*(*cvar) (const char *var_name, const char *value, int flags);
	void	(*cvar_set) (const char *var_name, const char *value);
	cvar_t	*(*cvar_forceset) (const char *var_name, const char *value);
	void	(*cvar_describe) (cvar_t *var, const char *description_string);

	// ClientCommand and ServerCommand parameter access
	int		(*argc) (void);
	char	*(*argv) (int n);
	char	*(*args) (void);	// concatenation of all argv >= 1

	// add commands to the server console as if they were typed in
	// for map changing, etc
	void	(*AddCommandString) (char *text);

	void	(*DebugGraph) (float value, const float color[]);

	int		(*Sys_Milliseconds) (void);

	// File System path from relative path (added 2010-08)
	qboolean (*FullPath)(char *full_path, size_t pathsize, const char *relative_path);
	void (*FullWritePath)(char *full_path, size_t pathsize,	const char *relative_path);

} game_import_t;

//
// functions exported by the game subsystem
//
typedef struct
{
	int			apiversion;

	// the init function will only be called when a game starts,
	// not each time a level is loaded.  Persistant data for clients
	// and the server can be allocated in init
	void		(*Init) (void);
	void		(*Shutdown) (void);

	// each new level entered will cause a call to SpawnEntities
	void		(*SpawnEntities) (char *mapname, const char *entstring, char *spawnpoint);

	qboolean	(*ClientConnect)( edict_t *ent, char *userinfo);
	
	void		(*ClientBegin) (edict_t *ent);
	void		(*ClientUserinfoChanged) (edict_t *ent, char *userinfo, int whereFrom);
	void		(*ClientDisconnect) (edict_t *ent);
	void		(*ClientCommand) (edict_t *ent);
	void		(*ClientThink) (edict_t *ent, usercmd_t *cmd);

	int			(*ACESP_FindBotNum) (void);

	void		(*RunFrame) (void);
	void		(*ForceExitIntermission) (void); 

	// ServerCommand will be called when an "sv <command>" command is issued on the
	// server console.
	// The game can issue gi.argc() / gi.argv() commands to get the rest
	// of the parameters
	void		(*ServerCommand) (void);

	//
	// global variables shared between game and server
	//

	// The edict array is allocated in the game dll so it
	// can vary in size from one game to another.
	// 
	// The size will be fixed when ge->Init() is called
	struct edict_s	*edicts;
	int			edict_size;
	int			num_edicts;		// current number, <= max_edicts
	int			max_edicts;
} game_export_t;

game_export_t *GetGameApi (game_import_t *import);
