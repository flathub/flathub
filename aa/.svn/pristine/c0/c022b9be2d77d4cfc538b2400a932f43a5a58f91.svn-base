///////////////////////////////////////////////////////////////////////
//
//  ACE - Quake II Bot Base Code
//
//  Version 1.0
//
//  This file is Copyright(c), Steve Yeager 1998, All Rights Reserved
//
//
//	All other files are Copyright(c) Id Software, Inc.
//
//	Please see liscense.txt in the source directory for the copyright
//	information regarding those files belonging to Id Software, Inc.
//
//	Should you decide to release a modified version of ACE, you MUST
//	include the following text (minus the BEGIN and END lines) in the
//	documentation for your modification.
//
//	--- BEGIN ---
//
//	The ACE Bot is a product of Steve Yeager, and is available from
//	the ACE Bot homepage, at http://www.axionfx.com/ace.
//
//	This program is a modification of the ACE Bot, and is therefore
//	in NO WAY supported by Steve Yeager.

//	This program MUST NOT be sold in ANY form. If you have paid for
//	this product, you should contact Steve Yeager immediately, via
//	the ACE Bot homepage.
//
//	--- END ---
//
//	I, Steve Yeager, hold no responsibility for any harm caused by the
//	use of this source code, especially to small children and animals.
//  It is provided as-is with no implied warranty or support.
//
//  I also wish to thank and acknowledge the great work of others
//  that has helped me to develop this code.
//
//  John Cricket    - For ideas and swapping code.
//  Ryan Feltrin    - For ideas and swapping code.
//  SABIN           - For showing how to do true client based movement.
//  BotEpidemic     - For keeping us up to date.
//  Telefragged.com - For giving ACE a home.
//  Microsoft       - For giving us such a wonderful crash free OS.
//  id              - Need I say more.
//
//  And to all the other testers, pathers, and players and people
//  who I can't remember who the heck they were, but helped out.
//
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
//
//  acebot.h - Main header file for ACEBOT
//
//
///////////////////////////////////////////////////////////////////////

#ifndef _ACEBOT_H
#define _ACEBOT_H

#if !defined BOT_GAMEDATA
#define BOT_GAMEDATA "botinfo"
#endif

// Only 100 allowed for now (probably never be enough edicts for 'em
#define MAX_BOTS 100

// Platform states
#define	STATE_TOP			0
#define	STATE_BOTTOM		1
#define STATE_UP			2
#define STATE_DOWN			3

// Maximum nodes
#define MAX_NODES 1000

// Link types
#define INVALID -1

// Node types
#define NODE_MOVE 0
#define NODE_LADDER 1
#define NODE_PLATFORM 2
#define NODE_TELEPORTER 3
#define NODE_ITEM 4
#define NODE_WATER 5
#define NODE_GRAPPLE 6
#define NODE_JUMP 7
#define NODE_REDBASE 8
#define NODE_BLUEBASE 9
#define NODE_DEFEND 10
#define NODE_ALL 99 // For selecting all nodes

// Density setting for nodes
#define NODE_DENSITY 256

// Bot state types
#define STATE_STAND 0
#define STATE_MOVE 1
#define STATE_ATTACK 2
#define STATE_WANDER 3
#define STATE_FLEE 4

#define MOVE_LEFT 0
#define MOVE_RIGHT 1
#define MOVE_FORWARD 2
#define MOVE_BACK 3

// Special CONTENT mask for acebots, so they can detect and climb ladders
// replaces MASK_OPAQUE defined in game/q_shared.h
#define BOTMASK_OPAQUE (CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA|CONTENTS_LADDER)

// Node structure
typedef struct node_s
{
	vec3_t origin; // Using Id's representation
	int type;   // type of node

} node_t;

typedef struct item_table_s
{
	edict_t *ent;
	int node;
} item_table_t;

// Bot config structure
struct botvals_s
{
	int skill;
	char faveweap[64];
	float weapacc[10];
	float awareness;
	char		chatmsg[8][128];
} botvals;

extern edict_t *players[MAX_CLIENTS];		// pointers to all players in the game

// extern decs
edict_t *node_showents[MAX_NODES]; //ents created by shownode
extern node_t nodes[MAX_NODES];
extern item_table_t item_table[MAX_EDICTS];
extern qboolean debug_mode;
extern int bot_numnodes;
extern int num_items;
extern int num_bots;

// id Function Protos I need
void     ClientObituary (edict_t *self, edict_t *inflictor, edict_t *attacker);
void     TossClientWeapon (edict_t *self);
void     ClientThink (edict_t *ent, usercmd_t *ucmd);
void     SelectSpawnPoint (edict_t *ent, vec3_t origin, vec3_t angles);
void     ClientUserinfoChanged (edict_t *ent, char *userinfo, int whereFrom);
void     CopyToBodyQue (edict_t *ent);

qboolean ClientConnect (edict_t *ent, char *userinfo);

void     Use_Plat (edict_t *ent, edict_t *other, edict_t *activator);

// acebot_ai.c protos
void     ACEAI_Think (edict_t *self);
void     ACEAI_PickLongRangeGoal(edict_t *self);
void     ACEAI_PickShortRangeGoal(edict_t *self);
qboolean ACEAI_FindEnemy(edict_t *self);
void     ACEAI_ChooseWeapon(edict_t *self);
qboolean ACEAI_CheckShot(edict_t *self);

// acebot_cmds.c protos
qboolean ACECM_Commands(edict_t *ent);
void     ACECM_Store();

// acebot_items.c protos
qboolean ACEIT_IsVisible(edict_t *self, vec3_t goal);
qboolean ACEIT_IsReachable(edict_t *self,vec3_t goal);
qboolean ACEIT_ChangeWeapon (edict_t *ent, gitem_t *item);
qboolean ACEIT_CanUseArmor (gitem_t *item, edict_t *other);
float	 ACEIT_ItemNeed (edict_t *self, gitem_t *item);
void     ACEIT_BuildItemNodeTable (qboolean rebuild);

// acebot_movement.c protos
qboolean ACEMV_SpecialMove(edict_t *self,usercmd_t *ucmd);
void     ACEMV_Move(edict_t *self, usercmd_t *ucmd);
void     ACEMV_Attack (edict_t *self, usercmd_t *ucmd);
void     ACEMV_Wander (edict_t *self, usercmd_t *ucmd);

// acebot_nodes.c protos
int      ACEND_FindCost(int from, int to);
int      ACEND_FindCloseReachableNode(edict_t *self, int dist, int type);
int      ACEND_FindClosestReachableNode(edict_t *self, int range, int type);
void     ACEND_SetGoal(edict_t *self, int goal_node);
qboolean ACEND_FollowPath(edict_t *self);
void     ACEND_GrapFired(edict_t *self);
qboolean ACEND_CheckForLadder(edict_t *self);
void     ACEND_PathMap(edict_t *self);
void     ACEND_InitNodes(void);
void     ACEND_ShowNode(int node);
void     ACEND_DrawPath();
void     ACEND_ShowPath(edict_t *self, int goal_node);
int      ACEND_AddNode(edict_t *self, int type);
void     ACEND_UpdateNodeEdge(int from, int to);
void     ACEND_RemoveNodeEdge(edict_t *self, int from, int to);
void     ACEND_ResolveAllPaths();
void     ACEND_SaveNodes();
void     ACEND_LoadNodes();

// acebot_spawn.c protos
void     ACESP_UpdateBots(void);
void	 ACESP_SaveBots();
void	 ACESP_LoadBots(edict_t *ent);
int		 ACESP_FindBotNum(void);
edict_t *ACESP_FindBot(const char *name);
void	 ACESP_KickBot(edict_t *bot);
void     ACESP_SpawnInitializeAI (edict_t *ent);
edict_t *ACESP_FindFreeClient (void);
void     ACESP_SetName(edict_t *bot, char *name, char *skin, char *userinfo );
qboolean ACESP_SpawnBot (char *name, char *skin, char *userinfo);
void     ACESP_ReAddBots();
void     ACESP_RemoveBot(char *name);
void	 safe_cprintf (edict_t *ent, int printlevel, char *fmt, ...);
void     safe_centerprintf (edict_t *ent, char *fmt, ...);
void     safe_bprintf (int printlevel, char *fmt, ...);
void     debug_printf (char *fmt, ...);
#endif
