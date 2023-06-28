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
//  acebot_cmds.c - Main internal command processor
//
///////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "game/g_local.h"
#include "acebot.h"

qboolean debug_mode=false;

///////////////////////////////////////////////////////////////////////
// Special command processor
///////////////////////////////////////////////////////////////////////
qboolean ACECM_Commands(edict_t *ent)
{
	char	*cmd;
	int node;

	cmd = gi.argv(0);

	if(Q_strcasecmp (cmd, "addnode") == 0 && debug_mode)
		ACEND_AddNode(ent,atoi(gi.argv(1)));

	else if(Q_strcasecmp (cmd, "removelink") == 0 && debug_mode)
		ACEND_RemoveNodeEdge(ent,atoi(gi.argv(1)), atoi(gi.argv(2)));

	else if(Q_strcasecmp (cmd, "addlink") == 0 && debug_mode)
		ACEND_UpdateNodeEdge(atoi(gi.argv(1)), atoi(gi.argv(2)));

	else if(Q_strcasecmp (cmd, "showpath") == 0 && debug_mode)
    	ACEND_ShowPath(ent,atoi(gi.argv(1)));
    
    else if(Q_strcasecmp (cmd, "shownode") == 0 && debug_mode)
    	ACEND_ShowNode(atoi(gi.argv(1)));
    
    else if(Q_strcasecmp (cmd, "hidenode") == 0 && debug_mode)
    {
        node = atoi(gi.argv(1));
        if (node_showents[node])
        {
            G_FreeEdict(node_showents[node]);
            node_showents[node] = NULL;
        }
    }

	else if(Q_strcasecmp (cmd, "findnode") == 0 && debug_mode)
	{
	    if (gi.argc() == 2)
	        node = atoi(gi.argv(1));
	    else
		    node = ACEND_FindClosestReachableNode(ent,NODE_DENSITY, NODE_ALL);
		safe_bprintf(PRINT_MEDIUM,"node: %d type: %d x: %f y: %f z %f\n",node,nodes[node].type,nodes[node].origin[0],nodes[node].origin[1],nodes[node].origin[2]);
	}

	else if(Q_strcasecmp (cmd, "movenode") == 0 && debug_mode)
	{
		node = atoi(gi.argv(1));
		nodes[node].origin[0] = atof(gi.argv(2));
		nodes[node].origin[1] = atof(gi.argv(3));
		nodes[node].origin[2] = atof(gi.argv(4));
		safe_bprintf(PRINT_MEDIUM,"node: %d moved to x: %f y: %f z %f\n",node, nodes[node].origin[0],nodes[node].origin[1],nodes[node].origin[2]);
	}

	else
		return false;

	return true;
}


///////////////////////////////////////////////////////////////////////
// Called when the level changes, store maps and bots (disconnected)
///////////////////////////////////////////////////////////////////////
void ACECM_Store()
{
	ACEND_SaveNodes();
}

///////////////////////////////////////////////////////////////////////
// These routines are bot safe print routines, all id code needs to be
// changed to these so the bots do not blow up on messages sent to them.
// Do a find and replace on all code that matches the below criteria.
//
// (Got the basic idea from Ridah)
//
//  change: gi.cprintf to safe_cprintf
//  change: gi.bprintf to safe_bprintf
//  change: gi.centerprintf to safe_centerprintf
//
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// Debug print, could add a "logging" feature to print to a file
///////////////////////////////////////////////////////////////////////
void debug_printf(char *fmt, ...)
{
	int     i;
	char	bigbuffer[0x10000];
	int		len;
	va_list	argptr;
	edict_t	*cl_ent;

	va_start (argptr,fmt);
	len = vsnprintf (bigbuffer,sizeof(bigbuffer),fmt,argptr);
	va_end (argptr);

	if (g_dedicated->value)
		gi.cprintf(NULL, PRINT_MEDIUM, "%s", bigbuffer);

	for (i=0 ; i<g_maxclients->value ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || cl_ent->is_bot)
			continue;

		gi.cprintf(cl_ent,  PRINT_MEDIUM, "%s", bigbuffer);
	}

}

///////////////////////////////////////////////////////////////////////
// botsafe cprintf
///////////////////////////////////////////////////////////////////////
void safe_cprintf (edict_t *ent, int printlevel, char *fmt, ...)
{
	char	bigbuffer[0x10000];
	va_list		argptr;
	int len;

	if (ent && (!ent->inuse || ent->is_bot))
		return;

	va_start (argptr,fmt);
	len = vsnprintf (bigbuffer,sizeof(bigbuffer),fmt,argptr);
	va_end (argptr);

	gi.cprintf(ent, printlevel, "%s", bigbuffer);

}

///////////////////////////////////////////////////////////////////////
// botsafe centerprintf
///////////////////////////////////////////////////////////////////////
void safe_centerprintf (edict_t *ent, char *fmt, ...)
{
	char	bigbuffer[0x10000];
	va_list		argptr;
	int len;

	if (!ent->inuse || ent->is_bot)
		return;

	// If the MOTD was displayed and is being "protected" from overwrite,
	// ignore the call.
	if ( ent->client && ent->client->motd_frames ) {
		return;
	}

	va_start (argptr,fmt);
	len = vsnprintf (bigbuffer,sizeof(bigbuffer),fmt,argptr);
	va_end (argptr);

	gi.centerprintf(ent,"%s", bigbuffer);

}

///////////////////////////////////////////////////////////////////////
// botsafe bprintf
///////////////////////////////////////////////////////////////////////
void safe_bprintf (int printlevel, char *fmt, ...)
{
	int i;
	char	bigbuffer[0x10000];
	int		len;
	va_list		argptr;
	edict_t	*cl_ent;

	va_start (argptr,fmt);
	len = vsnprintf (bigbuffer,sizeof(bigbuffer),fmt,argptr);
	va_end (argptr);

	if (g_dedicated->value)
		gi.cprintf(NULL, printlevel, "%s", bigbuffer);

	for (i=0 ; i<g_maxclients->value ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || cl_ent->is_bot)
			continue;

		gi.cprintf(cl_ent, printlevel, "%s", bigbuffer);
	}
}

