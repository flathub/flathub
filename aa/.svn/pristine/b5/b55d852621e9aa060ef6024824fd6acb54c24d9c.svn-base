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
// qcommon.h -- definitions common between client and server, but not game module

#if !defined Q_COMMON_H
#define Q_COMMON_H

#include "game/q_shared.h"

#if !defined VERSION
#define	VERSION		"7.7"
#endif

// To test the outdated version message:
// #define	VERSION		"7.71.3"

#define MENU_STATIC_WIDTH	720.0f

#define DEFAULTMODEL		"martianenforcer"
#define DEFAULTSKIN			"default"

#if defined WIN32_VARIANT

#define BUFFER_DEBUG 0

#if !defined BUILDSTRING
#ifdef NDEBUG
#define BUILDSTRING "Win32 RELEASE"
#else
#define BUILDSTRING "Win32 DEBUG"
#endif
#endif

#if !defined CPUSTRING
#define	CPUSTRING	"x86"
#endif

#else

#if !defined BUILDSTRING
#define BUILDSTRING "?OS?"
#endif

#if !defined CPUSTRING
#define CPUSTRING "?CPU?"
#endif

#endif

// All urls below use http and not https because on windows they are giving problems with libcurl.
// It results into error "Protocol "https" not supported or disabled in libcurl."

// Enable/disable the old stats
#define STATS_ENABLED false
#define DEFAULT_STATS_URL "http://martianbackup.com"


// All game operations including IRC
#define USE_ALIENARENA_ORG true
#define USE_ALIENARENA_ORG_IRC true

// Obsolete:
#define VERSION_CHECK_PLANETARENA_URL "http://martianbackup.com/version/crx_version"
#define VERSION_CHECK_MARTIANBACKUP_URL "http://martianbackup.com/version/crx_version"

#define VERSION_CHECK_ALIENARENA_ORG_URL "http://invader.alienarena.org/version/crx_version"
#define DEFAULT_VERSION_CHECK_URL (USE_ALIENARENA_ORG ? VERSION_CHECK_ALIENARENA_ORG_URL : VERSION_CHECK_MARTIANBACKUP_URL)

// Obsolete:
#define IRC_PLANETARENA "irc.planetarena.org"
#define IRC_HAL_NANOID "hal.nanoid.net"

#define IRC_ALIENARENA_ORG "irc.alienarena.org"
#define DEFAULT_IRC_SERVER (USE_ALIENARENA_ORG_IRC ? IRC_ALIENARENA_ORG : IRC_HAL_NANOID)

// Obsolete:
#define DEFAULT_MASTER_COR_1 "master.corservers.com"
#define DEFAULT_MASTER_COR_2 "master2.corservers.com"

#define DEFAULT_MASTER_1 (USE_ALIENARENA_ORG ? "master.alienarena.org" : DEFAULT_MASTER_COR_1)
#define DEFAULT_MASTER_2 (USE_ALIENARENA_ORG ? "master2.alienarena.org" : DEFAULT_MASTER_COR_2)

// NOTE: the game does *not* depend on these being the same for server and client.
#define DEFAULT_DOWNLOAD_URL_1 (USE_ALIENARENA_ORG ? "http://invader.alienarena.org/sv_downloadurl" : "http://martianbackup.com/sv_downloadurl")
#define DEFAULT_DOWNLOAD_URL_2 (USE_ALIENARENA_ORG ? "http://martianbackup.com/sv_downloadurl" : "http://red.planetarena.org/sv_downloadurl")

#define NEWSFEED_URL (USE_ALIENARENA_ORG ? "http://invader.alienarena.org/newsfeed.db" : "http://martianbackup.com/newsfeed.db")

#define MAPPACK_URL "http://invader.alienarena.org/arena/files"

/* ---- Relative path names for game data ---*/
/*
 * BASE_GAMEDATA : the principal resource subdirectory.
 * GAME_GAMEDATA : the game and game subdirectory.
 * USER_GAMEDATA : the user home subdirectory.
 */
#define BASE_GAMEDATA "data1"
#define GAME_GAMEDATA "arena"

#ifdef UNIX_VARIANT
# define USER_GAMEDATA "cor-games"
#endif

#define FOFS(type,field) (ptrdiff_t) &(((type *)0)->field)

//============================================================================

// Hash key computation macro, because that piece of code gets around a lot

#define COMPUTE_HASH_KEY_CHAR(key, schar ) \
	key = 31 * key + tolower(schar)

#define COMPUTE_HASH_KEY(key, str, counter) \
{ \
	key = 0; \
	for ( counter = 0; str[counter] ; counter ++ ) \
		COMPUTE_HASH_KEY_CHAR(key, str[counter]); \
}

//============================================================================

typedef struct sizebuf_s
{
	qboolean	allowoverflow;	// if false, do a Com_Error
	qboolean	overflowed;		// set to true if the buffer size failed
	byte		*data;
	int		maxsize;
	int		cursize;
	int		readcount;
#ifdef	BUFFER_DEBUG
	char		name[48];
#endif	//BUFFER_DEBUG
} sizebuf_t;

void SZ_Init (sizebuf_t *buf, byte *data, int length);
void SZ_Clear (sizebuf_t *buf);
void *SZ_GetSpace (sizebuf_t *buf, int length);
void SZ_Write (sizebuf_t *buf, void *data, int length);
void SZ_Print (sizebuf_t *buf, char *data);	// strcats onto the sizebuf

#ifdef	BUFFER_DEBUG
void SZ_SetName( sizebuf_t *buf, const char * name, qboolean print_it );
#else	//BUFFER_DEBUG
# define SZ_SetName(buf, name, print)
#endif	// BUFFER_DEBUG

//============================================================================

struct usercmd_s;
struct entity_state_s;

void MSG_WriteChar (sizebuf_t *sb, int c);
void MSG_WriteByte (sizebuf_t *sb, int c);
void MSG_WriteShort (sizebuf_t *sb, int c);
void MSG_WriteLong (sizebuf_t *sb, int c);
void MSG_WriteFloat (sizebuf_t *sb, float f);
void MSG_WriteSizeInt (sizebuf_t *sb, int bytes, int c);
void MSG_WriteString (sizebuf_t *sb, char *s);
void MSG_WriteCoord (sizebuf_t *sb, float f);
void MSG_WritePos (sizebuf_t *sb, vec3_t pos);
void MSG_WriteAngle (sizebuf_t *sb, float f);
void MSG_WriteAngle16 (sizebuf_t *sb, float f);
void MSG_WriteDeltaUsercmd (sizebuf_t *sb, struct usercmd_s *from, struct usercmd_s *cmd);
void MSG_WriteDeltaEntity (struct entity_state_s *from, struct entity_state_s *to, sizebuf_t *msg, qboolean force, qboolean newentity);
void MSG_WriteDir (sizebuf_t *sb, vec3_t vector);


void	MSG_BeginReading (sizebuf_t *sb);

int	MSG_ReadChar (sizebuf_t *sb);
int	MSG_ReadByte (sizebuf_t *sb);
int	MSG_ReadShort (sizebuf_t *sb);
int	MSG_ReadLong (sizebuf_t *sb);
int MSG_ReadSizeInt (sizebuf_t *msg_read, int bytes);
float	MSG_ReadFloat (sizebuf_t *sb);
char	*MSG_ReadString (sizebuf_t *sb);
char	*MSG_ReadStringLine (sizebuf_t *sb);

float	MSG_ReadCoord (sizebuf_t *sb);
void	MSG_ReadPos (sizebuf_t *sb, vec3_t pos);
float	MSG_ReadAngle (sizebuf_t *sb);
float	MSG_ReadAngle16 (sizebuf_t *sb);
void	MSG_ReadDeltaUsercmd (sizebuf_t *sb, struct usercmd_s *from, struct usercmd_s *cmd);

void	MSG_ReadDir (sizebuf_t *sb, vec3_t vector);

void	MSG_ReadData (sizebuf_t *sb, void *buffer, int size);

//============================================================================

extern	qboolean	bigendien;

extern	short	BigShort (short l);
extern	short	LittleShort (short l);
extern	int	BigLong (int l);
extern	int	LittleLong (int l);
extern	float	BigFloat (float l);
extern	float	LittleFloat (float l);

//============================================================================


int COM_Argc (void);
char *COM_Argv (int arg);	// range and null checked
void COM_ClearArgv (int arg);
int COM_CheckParm (char *parm);
void COM_AddParm (char *parm);

void COM_Init (void);
void COM_InitArgv (int argc, char **argv);

char *CopyString (const char *in);
qboolean Com_PatternMatch (const char *string, const char *pattern);

//============================================================================

void Info_Print (char *s);


/* crc.h */

void CRC_Init(unsigned short *crcvalue);
void CRC_ProcessByte(unsigned short *crcvalue, byte data);
unsigned short CRC_Value(unsigned short crcvalue);
unsigned short CRC_Block (byte *start, int count);



/*
==============================================================

PROTOCOL

==============================================================
*/

// protocol.h -- communications protocols

#define	PROTOCOL_VERSION	36

//=========================================

#define	PORT_MASTER	27900
#define	PORT_CLIENT	27901
#define PORT_STATS	27902
#define	PORT_SERVER	27910


//=========================================

#define	UPDATE_BACKUP	16	// copies of entity_state_t to keep buffered
							// must be power of two
#define	UPDATE_MASK		(UPDATE_BACKUP-1)



//==================
// the svc_strings[] array in cl_parse.c should mirror this
//==================

//
// server to client
//
enum svc_ops_e
{
	svc_bad,

	// these ops are known to the game dll
	svc_muzzleflash,
	svc_muzzleflash2,
	svc_temp_entity,
	svc_layout,
	svc_inventory,

	// the rest are private to the client and server
	svc_nop,
	svc_disconnect,
	svc_reconnect,
	svc_sound,					// <see code>
	svc_print,					// [byte] id [string] null terminated string
	svc_stufftext,				// [string] stuffed into client's console buffer, should be \n terminated
	svc_serverdata,				// [long] protocol ...
	svc_configstring,			// [short] [string]
	svc_spawnbaseline,
	svc_centerprint,			// [string] to put in center of the screen
	svc_download,				// [short] size [size bytes]
	svc_playerinfo,				// variable
	svc_packetentities,			// [...]
	svc_deltapacketentities,	// [...]
	svc_frame
};

//==============================================

//
// client to server
//
enum clc_ops_e
{
	clc_bad,
	clc_nop,
	clc_move,				// [[usercmd_t]
	clc_userinfo,			// [[userinfo string]
	clc_stringcmd			// [string] message
};

//==============================================

// plyer_state_t communication

#define	PS_M_TYPE		(1<<0)
#define	PS_M_ORIGIN		(1<<1)
#define	PS_M_VELOCITY		(1<<2)
#define	PS_M_TIME		(1<<3)
#define	PS_M_FLAGS		(1<<4)
#define	PS_M_GRAVITY		(1<<5)
#define	PS_M_DELTA_ANGLES	(1<<6)

#define	PS_VIEWOFFSET		(1<<7)
#define	PS_VIEWANGLES		(1<<8)
#define	PS_KICKANGLES		(1<<9)
#define	PS_BLEND		(1<<10)
#define	PS_FOV			(1<<11)
#define	PS_WEAPONINDEX		(1<<12)
#define	PS_WEAPONFRAME		(1<<13)
#define	PS_RDFLAGS		(1<<14)

//==============================================

// user_cmd_t communication

// ms and light always sent, the others are optional
#define	CM_ANGLE1 	(1<<0)
#define	CM_ANGLE2 	(1<<1)
#define	CM_ANGLE3 	(1<<2)
#define	CM_FORWARD	(1<<3)
#define	CM_SIDE		(1<<4)
#define	CM_UP		(1<<5)
#define	CM_BUTTONS	(1<<6)
#define	CM_IMPULSE	(1<<7)

//==============================================

// a sound without an ent or pos will be a local only sound
#define	SND_VOLUME		(1<<0)		// a byte
#define	SND_ATTENUATION		(1<<1)		// a byte
#define	SND_POS			(1<<2)		// three coordinates
#define	SND_ENT			(1<<3)		// a short 0-2: channel, 3-12: entity
#define	SND_OFFSET		(1<<4)		// a byte, msec offset from frame start

#define DEFAULT_SOUND_PACKET_VOLUME	1.0
#define DEFAULT_SOUND_PACKET_ATTENUATION 1.0

//==============================================

// entity_state_t communication

// try to pack the common update flags into the first byte
#define	U_ORIGIN1	(1<<0)
#define	U_ORIGIN2	(1<<1)
#define	U_ANGLE2	(1<<2)
#define	U_ANGLE3	(1<<3)
#define	U_FRAME8	(1<<4)		// frame is a byte
#define	U_EVENT		(1<<5)
#define	U_REMOVE	(1<<6)		// REMOVE this entity, don't add it
#define	U_MOREBITS1	(1<<7)		// read one additional byte

// second byte
#define	U_NUMBER16	(1<<8)		// NUMBER8 is implicit if not set
#define	U_ORIGIN3	(1<<9)
#define	U_ANGLE1	(1<<10)
#define	U_MODEL		(1<<11)
#define U_RENDERFX8	(1<<12)		// fullbright, etc
#define	U_EFFECTS8	(1<<14)		// autorotate, trails, etc
#define	U_MOREBITS2	(1<<15)		// read one additional byte

// third byte
#define	U_SKIN8		(1<<16)
#define	U_FRAME16	(1<<17)		// frame is a short
#define	U_RENDERFX16	(1<<18)		// 8 + 16 = 32
#define	U_EFFECTS16	(1<<19)		// 8 + 16 = 32
#define	U_MODEL2	(1<<20)		// weapons, flags, etc
#define	U_MODEL3	(1<<21)
#define	U_MODEL4	(1<<22)
#define	U_MOREBITS3	(1<<23)		// read one additional byte

// fourth byte
#define	U_OLDORIGIN	(1<<24)		// FIXME: get rid of this
#define	U_SKIN16	(1<<25)
#define	U_SOUND		(1<<26)
#define	U_SOLID		(1<<27)


/*
==============================================================

CMD

Command text buffering and command execution

==============================================================
*/

/*

Any number of commands can be added in a frame, from several different sources.
Most commands come from either keybindings or console line input, but remote
servers can also send across commands and entire text files can be execed.

The + command line options are also added to the command buffer.

The game starts with a Cbuf_AddText ("exec quake.rc\n"); Cbuf_Execute ();

*/

#define	EXEC_NOW	0	// don't return until completed
#define	EXEC_INSERT	1	// insert at current position, but don't run yet
#define	EXEC_APPEND	2	// add to end of the command buffer

// allocates an initial text buffer that will grow as needed
void Cbuf_Init (void);

// as new commands are generated from the console or keybindings,
// the text is added to the end of the command buffer.
void Cbuf_AddText (char *text);

// when a command wants to issue other commands immediately, the text is
// inserted at the beginning of the buffer, before any remaining unexecuted
// commands.
void Cbuf_InsertText (char *text);

// this can be used in place of either Cbuf_AddText or Cbuf_InsertText
void Cbuf_ExecuteText (int exec_when, char *text);

// adds all the +set commands from the command line
void Cbuf_AddEarlyCommands (qboolean clear);

// adds all the remaining + commands from the command line
// Returns true if any late commands were added, which
// will keep the demoloop from immediately starting
qboolean Cbuf_AddLateCommands (void);

// Pulls off \n terminated lines of text from the command buffer and sends
// them through Cmd_ExecuteString.  Stops when the buffer is empty.
// Normally called once per frame, but may be explicitly invoked.
// Do not call inside a command function!
void Cbuf_Execute (void);

// These two functions are used to defer any pending commands while a map
// is being loaded
void Cbuf_CopyToDefer (void);
void Cbuf_InsertFromDefer (void);

//===========================================================================

/*

Command execution takes a null terminated string, breaks it into tokens,
then searches for a command or variable that matches the first token.

*/

typedef void (*xcommand_t) (void);
typedef qboolean (*xcompletionchecker_t) (int argnum, int data);
typedef char *(*xcompleter_t) (int argnum, int data);

void	Cmd_Init (void);

// called by the init functions of other parts of the program to
// register commands and functions to call for them.
// The cmd_name is referenced later, so it should not be in temp memory
// if function is NULL, the command will be forwarded to the server
// as a clc_stringcmd instead of executed locally
void	Cmd_AddCommand (char *cmd_name, xcommand_t function);
void	Cmd_RemoveCommand (char *cmd_name);

// used by the cvar code to check for cvar / command name overlap
qboolean Cmd_Exists (char *cmd_name);

// For extended completion capabilities: add the optional argument completer
// callbacks.
void	Cmd_SetCompleter (char *cmd_name, xcompleter_t completer, xcompletionchecker_t checker, int data);

// For extended completion capabilities: what kind of token is valid to be typed
// next.
#define COMPLETION_CVARS	1
#define COMPLETION_ALIASES	2
#define COMPLETION_COMMANDS	4
#define COMPLETION_ALL		(~0)

// For extended completion capabilities: use this for the actual substitution
// of the completion.
char *Cmd_MakeCompletedCommand (int argnum, const char *new_tok);
char *Cmd_CompleteWithInexactMatch (int argnum, int nmatch, char **matches);

// The following two functions require that Cmd_TokenizeString be called first
// on the partial command. They each take an argnum indicating which token
// should be checked first. User code should always start with 0.

// attempts to match a partial command for automatic command line completion
// returns NULL if nothing fits
char 	*Cmd_CompleteCommand (int argnum, int flags);
// returns true if the command, as it stands now, is valid.
qboolean Cmd_IsComplete (int argnum, int flags);

// The functions that execute commands get their parameters with these
// functions. Cmd_Argv () will return an empty string, not a NULL
// if arg > argc, so string operations are always safe.
int	Cmd_Argc (void);
char	*Cmd_Argv (int arg);
char	*Cmd_Args (void);

// Takes a null terminated string.  Does not need to be /n terminated.
// breaks the string up into arg tokens.
void	Cmd_TokenizeString (const char *text, qboolean macroExpand);

// Parses a single line of text into arguments and tries to execute it
// as if it was typed at the console
void	Cmd_ExecuteString (char *text);

// adds the current command line as a clc_stringcmd to the client message.
// things like godmode, noclip, etc, are commands directed to the server,
// so when they are typed in at the console, they will need to be forwarded.
void	Cmd_ForwardToServer (void);


/*
==============================================================

CVAR

==============================================================
*/

/*

cvar_t variables are used to hold scalar or string variables that can be changed or displayed at the console or prog code as well as accessed directly
in C code.

The user can access cvars from the console in three ways:
r_draworder			prints the current value
r_draworder 0		sets the current value to 0
set r_draworder 0	as above, but creates the cvar if not present
Cvars are restricted from having the same names as commands to keep this
interface from being ambiguous.
*/

extern	cvar_t	*cvar_vars;

// Creates the variable if it doesn't exist, or returns the existing one
// if it exists, the value will not be changed, but flags will be ORed in.
// That allows variables to be unarchived without needing bitflags. NOTE: the
// value and name strings are copied, so you don't need to keep track of any
// strings you use with this function.
cvar_t *Cvar_Get (const char *var_name, const char *value, int flags);

// will create the variable if it doesn't exist
void	Cvar_Set (const char *var_name, const char *value);

// will set the variable even if NOSET or LATCH
cvar_t *Cvar_ForceSet (const char *var_name, const char *value);

void	Cvar_FullSet (const char *var_name, const char *value, int flags);

// expands value to a string and calls Cvar_Set
void	Cvar_SetValue (const char *var_name, float value);

// returns 0 if not defined or non numeric
float	Cvar_VariableValue (const char *var_name);

// returns an empty string if not defined
char	*Cvar_VariableString (const char *var_name);

// attempts to match a partial variable name for command line completion
// returns NULL if nothing fits
char 	*Cvar_CompleteVariable (const char *partial);

// any CVAR_LATCHED variables that have been set will now take effect
void	Cvar_GetLatchedVars (void);

// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
// command.  Returns true if the command was a variable reference that
// was handled. (print or change)
qboolean Cvar_Command (void);

// appends lines containing "set variable value" for all variables
// with the archive flag set to true.
void 	Cvar_WriteVariables (char *path);

// setup the cvar subsystem and related console commands
void	Cvar_Init (void);

// returns an info string containing all the CVAR_USERINFO cvars
char	*Cvar_Userinfo (void);

// returns an info string containing all the CVAR_SERVERINFO cvars
char	*Cvar_Serverinfo (void);

// Initializes a new cvar struct holding just a value, with no name in it.
// Used in some places where a weak-typed variable is needed.
void	Anon_Cvar_Set(cvar_t *nvar, const char *var_value);

// Copies the description string into the cvar. Useful for documenting cvars.
// The description string is copied, so it need not be kept in memory for the
// life of the cvar.
void	Cvar_Describe(cvar_t *var, const char *description_string);
void	Cvar_Describe_ByName(const char *var_name, const char *description_string);

// this is set each time a CVAR_USERINFO variable is changed
// so that the client knows to send it to the server
extern	qboolean	userinfo_modified;

/*
==============================================================

NET

==============================================================
*/

// net.h -- quake's interface to the networking layer

#define	PORT_ANY	-1

#define	MAX_MSGLEN		2800		// max length of a message
#define	PACKET_HEADER	20			// two ints and a short

typedef enum {NA_LOOPBACK, NA_BROADCAST, NA_IP, NA_IPX, NA_BROADCAST_IPX} netadrtype_t;

typedef enum {NS_CLIENT, NS_SERVER} netsrc_t;

typedef struct
{
	netadrtype_t	type;

	byte	ip[4];
	byte	ipx[10];

	unsigned short	port; /* network byte order (bigendian) */
} netadr_t;

void		NET_Init (void);
void		NET_Shutdown (void);

void		NET_Config (qboolean multiplayer);

qboolean	NET_GetPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message);
void		NET_SendPacket (netsrc_t sock, int length, void *data, netadr_t to);

qboolean	NET_CompareAdr (netadr_t a, netadr_t b);
qboolean	NET_CompareBaseAdr (netadr_t a, netadr_t b);
qboolean	NET_IsLocalAddress (netadr_t adr);
char		*NET_AdrToString (netadr_t a);
qboolean	NET_StringToAdr (char *s, netadr_t *a);
void		NET_Sleep(int msec);

#define NET_IsLocalHost(x) \
	((x)->type == NA_LOOPBACK)
//============================================================================

#define	OLD_AVG		0.99		// total = oldtotal*OLD_AVG + new*(1-OLD_AVG)

#define	MAX_LATENT	32

typedef struct
{
	qboolean	fatal_error;

	netsrc_t	sock; // either NS_CLIENT or NS_SERVER

	int			dropped;			// between last packet and previous

	int			last_received;		// for timeouts
	int			last_sent;			// for retransmits

	netadr_t	remote_address;
	int			qport;				// qport value to write when transmitting

// sequencing variables
	int			incoming_sequence;
	int			incoming_acknowledged;
	int			incoming_reliable_acknowledged;	// single bit

	int			incoming_reliable_sequence;		// single bit, maintained local
	int			outgoing_sequence;

	int			reliable_sequence;			// single bit
	int			last_reliable_sequence;		// sequence number of last send

// reliable staging and holding areas
	sizebuf_t	message;		// writing buffer to send to server
	byte		message_buf[MAX_MSGLEN-16];		// leave space for header

// message is copied to this buffer when it is first transfered
	int			reliable_length;
	byte		reliable_buf[MAX_MSGLEN-16];	// unacked reliable message
} netchan_t;

extern	netadr_t	net_from;
extern	sizebuf_t	net_message;
extern	byte		net_message_buffer[MAX_MSGLEN];


void Netchan_Init (void);
void Netchan_Setup (netsrc_t sock, netchan_t *chan, netadr_t adr, int qport);

qboolean Netchan_NeedReliable (netchan_t *chan);
void Netchan_Transmit (netchan_t *chan, int length, byte *data);
void Netchan_OutOfBand (int net_socket, netadr_t adr, int length, byte *data);
void Netchan_OutOfBandPrint (int net_socket, netadr_t adr, char *format, ...);
qboolean Netchan_Process (netchan_t *chan, sizebuf_t *msg);

qboolean Netchan_CanReliable (netchan_t *chan);


/*
==============================================================

CMODEL

==============================================================
*/


#include "qcommon/qfiles.h"

cmodel_t	*CM_LoadMap (char *name, qboolean clientload, unsigned *checksum);
cmodel_t	*CM_LoadBSP (char *name, qboolean clientload, unsigned *checksum);
cmodel_t	*CM_InlineModel (char *name);	// *1, *2, etc

int			CM_NumClusters (void);
int			CM_NumInlineModels (void);
char		*CM_EntityString (void);

void		CM_FilterParseEntities (const char *fieldname, int numvals, const char *vals[], void (*process_ent_callback) (char *match, char *block));

// creates a clipping hull for an arbitrary box
int			CM_HeadnodeForBox (vec3_t mins, vec3_t maxs);

void CM_TerrainLightPoint (vec3_t in_point, vec3_t out_point, vec3_t out_color);

// returns an ORed contents mask
int			CM_PointContents (vec3_t p, int headnode);
int			CM_TransformedPointContents (vec3_t p, int headnode, vec3_t origin, vec3_t angles);

trace_t		CM_BoxTrace (const vec3_t start, const vec3_t end,
						  vec3_t mins, vec3_t maxs,
						  int headnode, int brushmask);
qboolean CM_FastTrace (const vec3_t start, const vec3_t end, int headnode, int brushmask);
trace_t		CM_TransformedBoxTrace (vec3_t start, vec3_t end,
						  vec3_t mins, vec3_t maxs,
						  int headnode, int brushmask,
						  vec3_t origin, vec3_t angles);

byte		*CM_ClusterPVS (int cluster);
byte		*CM_ClusterPHS (int cluster);

int			CM_PointLeafnum (vec3_t p);

// call with topnode set to the headnode, returns with topnode
// set to the first node that splits the box
int			CM_BoxLeafnums (vec3_t mins, vec3_t maxs, int *list,
							int listsize, int *topnode);

int			CM_LeafContents (int leafnum);
int			CM_LeafCluster (int leafnum);
int			CM_LeafArea (int leafnum);

void		CM_SetAreaPortalState (int portalnum, qboolean open);
qboolean	CM_AreasConnected (int area1, int area2);

int			CM_WriteAreaBits (byte *buffer, int area);
qboolean	CM_HeadnodeVisible (int headnode, byte *visbits);

void		CM_WritePortalState (FILE *f);
void		CM_ReadPortalState (FILE *f);

qboolean 	CM_inPVS (vec3_t p1, vec3_t p2);
qboolean	CM_inPVS_leafs (int leafnum1, int leafnum2);
qboolean 	CM_inPHS (vec3_t p1, vec3_t p2);

extern char map_name[MAX_QPATH];

int			CM_NumVertices (void);
int			CM_NumTriangles (void);
void		CM_GetVertex (int num, vec3_t out);
void		CM_GetTriangle (int num, int out[3]);

/*
==============================================================

PLAYER MOVEMENT CODE

Common between server and client so prediction matches

==============================================================
*/

extern float pm_airaccelerate;
qboolean remoteserver_jousting;
int	remoteserver_runspeed;

void Pmove (pmove_t *pmove);

/*
==============================================================

FILESYSTEM

==============================================================
*/
/**
 * @brief
 *
 * @param f 
 * @return 
 */
int FS_filelength( FILE *f );

/**
 * @brief
 *
 */
void FS_InitFilesystem( void );

/**
 * @brief Sets game and game sub-directory.
 *
 * @param dir The relative sub-directory name string. 
 */
void FS_SetGamedir( char *dir );

/**
 * @brief
 *
 * @param prevpath 
 * @return 
 */
char *FS_NextPath( char *prevpath );

/**
 * @brief
 *
 *
 */
void FS_ExecAutoexec( void );

/**
 * @brief
 *
 * @param full_path 
 * @param pathsize 
 * @param relative_path 
 * @return 
 */
qboolean FS_FullPath( char *full_path, size_t pathsize,
					  const char *relative_path );

/**
 * @brief
 *
 * @param full_path 
 * @param pathsize 
 * @param relative_path 
 */
void FS_FullWritePath( char *full_path, size_t pathsize,
					   const char *relative_path );

void FS_WriteAPPDATA( char *full_path, size_t pathsize, 
					   const char* relative_path);

/**
 * @brief
 *
 * @param filename 
 * @param file 
 * @return 
 */
int  FS_FOpenFile (const char *filename, FILE **file);

/**
 * @brief
 *
 * @param f 
 */
void FS_FCloseFile( FILE *f );

/**
 * @brief
 *
 * @param path 
 * @param buffer 
 * @return 
 */
int FS_LoadFile (const char *path, void **buffer);

/**
 * @brief
 *
 * @param path 
 * @param buffer 
 * @param statbuffer 
 * @param statbuffer_len 
 * @return 
 */
int FS_LoadFile_TryStatic (const char *path, void **buffer, void *statbuffer,
							size_t statbuffer_len);

/**
 * @brief
 *
 * @param buffer 
 * @param len 
 * @param f 
 */
void FS_Read( void *buffer, int len, FILE *f );

/**
 * @brief
 *
 * @param buffer 
 */
void FS_FreeFile( void *buffer );

/**
 * @brief
 *
 * @param path 
 */
void FS_CreatePath( char *path );

/**
 * @brief
 *
 * @param path 
 * @return 
 */
qboolean FS_FileExists( char *path );

/*
Checks if a file exists with the specified full path.
*/
qboolean FS_CheckFile(const char * search_path);

/**
 * @brief Get the absolute path to the game directory
 *        where configuration and non-official game
 *        resources reside.
 *
 * @return The absolute pathname string.
 */
const char *FS_Gamedir( void );

#define SFF_INPACK  0x20 /* For FS_ListFilesInFS(). */
/**
 * @brief
 *
 * @param findname 
 * @param numfiles 
 * @param musthave 
 * @param canthave 
 * @return 
 */
char **FS_ListFiles( char *findname, int *numfiles, unsigned musthave,
					 unsigned canthave );

/**
 * @brief
 *
 * @param findname 
 * @param numfiles 
 * @param musthave 
 * @param canthave 
 * @return 
 */
char **FS_ListFilesInFS( char *findname, int *numfiles, unsigned musthave,
						 unsigned canthave );

/**
 * @brief
 *
 * @param list 
 * @param n 
 */
void FS_FreeFileList( char **list, int n );

/*
==============================================================

MISC

==============================================================
*/


#define	ERR_FATAL	0		// exit the entire game with a popup window
#define	ERR_DROP	1		// print to console and disconnect from game
#define	ERR_QUIT	2		// not an error, just a normal exit

// redundant
//#define	EXEC_NOW	0		// don't return until completed
//#define	EXEC_INSERT	1		// insert at current position, but don't run yet
//#define	EXEC_APPEND	2		// add to end of the command buffer

#define	PRINT_ALL		0
#define PRINT_DEVELOPER	1	// only print when "developer 1"

void		Com_BeginRedirect (int target, char *buffer, int buffersize, void (*flush));
void		Com_EndRedirect (void);
void 		Com_Printf (char *fmt, ...);
void 		Com_DPrintf (char *fmt, ...);
void 		Com_Error (int code, char *fmt, ...);
void 		Com_Quit (void);

int			Com_ServerState (void);		// this should have just been a cvar...
void		Com_SetServerState (int state);

unsigned	Com_BlockChecksum (void *buffer, int length);
byte		COM_BlockSequenceCRCByte (byte *base, int length, int sequence);

float	frand(void);	// 0 ti 1
float	crand(void);	// -1 to 1

extern	cvar_t	*developer;
extern	cvar_t	*dedicated;
extern	cvar_t	*host_speeds;

extern	cvar_t	*log_stats;

cvar_t *fasttrace_verify; // Test for CM_FastTrace
cvar_t *test; // cvar to be used for testing/development purposes

extern	FILE *log_stats_file;

// host_speeds times
extern	int		time_before_game;
extern	int		time_after_game;
extern	int		time_before_ref;
extern	int		time_after_ref;

void Z_Free (void *ptr);
void *Z_Malloc (int size);			// returns 0 filled memory
void *Z_TagMalloc (int size, int tag);
void Z_FreeTags (int tag);

void Qcommon_Init (int argc, char **argv);
void Qcommon_Frame (int msec);
void Qcommon_Shutdown (void);

#define NUMVERTEXNORMALS	162
extern	vec3_t	bytedirs[NUMVERTEXNORMALS];

// this is in the client code, but can be used for debugging from server
void SCR_DebugGraph (float value, const float color[]);

//used for render effect
void CL_GlassShards(vec3_t org, vec3_t dir, int count);

#define static_array_size(array) (sizeof(array)/sizeof((array)[0]))

/*
==============================================================

NON-PORTABLE SYSTEM SERVICES

==============================================================
*/
char	*Sys_GetCommand (void);
void	Sys_Print (const char *text);
void	Sys_ShowConsole (qboolean show);

void	Sys_Init (void);

void	Sys_AppActivate (void);

void	Sys_UnloadGame (void);
void	*Sys_GetGameAPI (void *parms);
// loads the game dll and calls the api init function

char	*Sys_ConsoleInput (void);
void	Sys_ConsoleOutput (char *string);
void	Sys_SendKeyEvents (void);
void	Sys_Error (char *error, ...);
void	Sys_Quit (void);
char	*Sys_GetClipboardData( void );
void	Sys_CopyProtect (void);

/*
==============================================================

CLIENT / SERVER SYSTEMS

==============================================================
*/

void CL_Init (void);
void CL_Drop (void);
void CL_Shutdown (void);
void CL_Frame (int msec);
void CON_Print (const char *text);
void SCR_BeginLoadingPlaque (void);

void SV_Init (void);
void SV_Shutdown (char *finalmsg, qboolean reconnect);
void SV_Frame (int msec);


//
// compression
//
void qdecompress (sizebuf_t *src, sizebuf_t *dst, int type);


/*
==============================================================

IMAGE LOADING

==============================================================
*/

void LoadTGA (const char *name, byte **pic, int *width, int *height);
void bilinear_sample (const byte *texture, int tex_w, int tex_h, float u, float v, vec4_t out);

/*
==============================================================

TERRAIN LOADING/SIMPLIFICATION

==============================================================
*/

// decorations are things like vegetation, rocks/pebbles, etc.
typedef struct
{
	vec3_t	origin;
	float	size;
	int		type;
	const char *path; // ptr into decoration_variant_paths
} terraindec_t;

typedef struct
{
	char			*hmtex_path;
	char			*texture_path;
	char			*lightmap_path;
	int				num_vertices;
	float			*vert_positions;
	float			*vert_texcoords;
	int				num_triangles;
	unsigned int	*tri_indices;
	vec3_t			mins, maxs;
	int				num_decorations;
	terraindec_t	*decorations;
	char			*decoration_variant_paths; // lots of NULL-separated strings
} terraindata_t;

// out will be populated with a simplified version of the mesh. 
// name is just the path of the .terrain file, only used for error messages.
// oversampling_factor indicates how much detail to sample the heightmap 
// at before simplification. 2.0 means 4x as many samples as there are pixels,
// 0.5 means 0.25x as many.
// reduction_amt indicates how many times fewer triangles the simplified mesh
// should have.
// buf is a string containing the text of a .terrain file.
void LoadTerrainFile (terraindata_t *out, const char *name, qboolean decorations_only, float oversampling_factor, int reduction_amt, char *buf);

// writes out entire terraindata_t struct to file
void WriteTerrainData (terraindata_t *in, const char *name, int forRender); 
// read in terraindata_t struct from file
qboolean ReadTerrainData (terraindata_t *out, const char *name, int forRender);

// Frees any allocated buffers in dat.
void CleanupTerrainData (terraindata_t *dat);

#endif /* Q_COMMON_H */


