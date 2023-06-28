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
// cmd.c -- Quake script command processing module

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined UNIX_VARIANT
#if defined HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif

#include "qcommon.h"


#define	MAX_ALIAS_NAME	32

typedef struct cmdalias_s
{
	struct cmdalias_s	*next;
	unsigned int		hash_key;
	char			name[MAX_ALIAS_NAME];
	char			*value;
} cmdalias_t;

cmdalias_t	*cmd_alias;

qboolean	cmd_wait;

#define	ALIAS_LOOP_COUNT	16
int		alias_count;		// for detecting runaway loops


//=============================================================================

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
============
*/
void Cmd_Wait_f (void)
{
	cmd_wait = true;
}


/*
=============================================================================

						COMMAND BUFFER

=============================================================================
*/

sizebuf_t	cmd_text;
byte		cmd_text_buf[8192];

byte		defer_text_buf[8192];

/*
============
Cbuf_Init
============
*/
void Cbuf_Init (void)
{
	SZ_Init (&cmd_text, cmd_text_buf, sizeof(cmd_text_buf));
	SZ_SetName (&cmd_text, "Command buffer", true);
}

/*
============
Cbuf_AddText

Adds command text at the end of the buffer
============
*/
void Cbuf_AddText (char *text)
{
	int		l;

	l = strlen (text);

	if (cmd_text.cursize + l >= cmd_text.maxsize)
	{
		Com_Printf ("Cbuf_AddText: overflow! Discarding text: %s\n", text);
		return;
	}
	SZ_Write (&cmd_text, text, l);
}


/*
============
Cbuf_InsertText

Adds command text immediately after the current command
Adds a \n to the text
FIXME: actually change the command buffer to do less copying
============
*/
void Cbuf_InsertText (char *text)
{
	char	*temp;
	int		templen;

// copy off any commands still remaining in the exec buffer
	templen = cmd_text.cursize;
	if (templen)
	{
		temp = Z_Malloc (templen);
		memcpy (temp, cmd_text.data, templen);
		SZ_Clear (&cmd_text);
	}
	else
		temp = NULL;	// shut up compiler

// add the entire text of the file
	Cbuf_AddText (text);
	Cbuf_AddText ("\n");

// add the copied off data
	if (templen)
	{
		if (cmd_text.cursize + templen >= cmd_text.maxsize) 
			/* possible "graceful" solution: Cbuf_Execute here to flush the 
			 * buffer? Would change behavior of successive Cbuf_InsertText 
			 * calls and probably screw things up with exec'd cfgs within 
			 * exec'd cfgs, possibly resulting in infinite recursion loops.
			 * TODO: study this option with some pathological test cfgs.
			 */ 
			Com_Printf ("Cbuf_InsertText: overflow! Discarding text: %s\n", temp);
		else
			SZ_Write (&cmd_text, temp, templen);
		Z_Free (temp);
	}
}


/*
============
Cbuf_CopyToDefer
============
*/
void Cbuf_CopyToDefer (void)
{
	memcpy(defer_text_buf, cmd_text_buf, cmd_text.cursize);
	defer_text_buf[cmd_text.cursize] = 0;
	cmd_text.cursize = 0;
}

/*
============
Cbuf_InsertFromDefer
============
*/
void Cbuf_InsertFromDefer (void)
{
	Cbuf_InsertText ( (char *)defer_text_buf );
	defer_text_buf[0] = 0;
}


/*
============
Cbuf_ExecuteText
============
*/
void Cbuf_ExecuteText (int exec_when, char *text)
{
	switch (exec_when)
	{
	case EXEC_NOW:
		Cmd_ExecuteString (text);
		break;
	case EXEC_INSERT:
		Cbuf_InsertText (text);
		break;
	case EXEC_APPEND:
		Cbuf_AddText (text);
		break;
	default:
		Com_Error (ERR_FATAL, "Cbuf_ExecuteText: bad exec_when");
	}
}

/*
============
Cbuf_Execute
============
*/
void Cbuf_Execute (void)
{
	int		i;
	char	*text;
	char	line[1024];
	int		quotes;

	alias_count = 0;		// don't allow infinite alias loops

	while (cmd_text.cursize)
	{
// find a \n or ; line break
		text = (char *)cmd_text.data;

		quotes = 0;
		for (i=0 ; i< cmd_text.cursize ; i++)
		{
			if (text[i] == '"')
				quotes++;
			if ( !(quotes&1) &&  text[i] == ';')
				break;	// don't break if inside a quoted string
			if (text[i] == '\n')
				break;
		}

		// sku - removed potentional buffer overflow vulnerability
		if( i > sizeof( line ) - 1 ) {
			i = sizeof( line ) - 1;
		}

		memcpy (line, text, i);
		line[i] = 0;

// delete the text from the command buffer and move remaining commands down
// this is necessary because commands (exec, alias) can insert data at the
// beginning of the text buffer

		if (i == cmd_text.cursize)
			cmd_text.cursize = 0;
		else
		{
			i++;
			cmd_text.cursize -= i;
			memmove (text, text+i, cmd_text.cursize);
		}

// execute the command line
		Cmd_ExecuteString (line);

		if (cmd_wait)
		{
			// skip out while text still remains in buffer, leaving it
			// for next frame
			cmd_wait = false;
			break;
		}
	}
}

/*
===============
Cbuf_AddEarlyCommands

Adds command line parameters as script statements
Commands lead with a +, and continue until another +

Set commands are added early, so they are guaranteed to be set before
the client and server initialize for the first time.

Other commands are added late, after all initialization is complete.
===============
*/
void Cbuf_AddEarlyCommands (qboolean clear)
{
	int		i;
	char	*s;

	for (i=0 ; i<COM_Argc() ; i++)
	{
		s = COM_Argv(i);
		if (Q_strcasecmp (s, "+set"))
			continue;
		Cbuf_AddText (va("set %s %s\n", COM_Argv(i+1), COM_Argv(i+2)));
		if (clear)
		{
			COM_ClearArgv(i);
			COM_ClearArgv(i+1);
			COM_ClearArgv(i+2);
		}
		i+=2;
	}
}

/*
=================
Cbuf_AddLateCommands

Adds command line parameters as script statements
Commands lead with a + and continue until another + or -
quake +vid_ref gl +map amlev1

Returns true if any late commands were added, which
will keep the demoloop from immediately starting
=================
*/
qboolean Cbuf_AddLateCommands (void)
{
	int		i;
	char	*t, *u;
	int		s;
	char	*text, *build, c;
	int		argc;
	qboolean	ret;

// build the combined string to parse from
	s = 0;
	argc = COM_Argc();
	for (i=1 ; i<argc ; i++)
	{
		s += strlen (COM_Argv(i)) + 1;
	}
	if (!s)
		return false;

	text = Z_Malloc (s+1);
	text[0] = 0;
	for (i=1 ; i<argc ; i++)
	{
		strcat (text,COM_Argv(i));
		if (i != argc-1)
			strcat (text, " ");
	}

// pull out the commands
	build = Z_Malloc (s+1);
	build[0] = 0;

	// Logic changed May 11 2013: hyphens still terminate commands, but must
	// be preceeded by a space or tab character. Reason: allows hyphens in
	// map names when specified as command line arguments to the game binary,
	// as in: alienarena +map dm-deathray
	// -Max
	for (t = text ; t < &text[s-1] ; t++)
	{
		if (*t == '+')
		{
			t++;

			u = t;
			while (*u != '+' && *u != '\0' && !((*(u-1) == ' ' || *(u-1) == '\t') && *u == '-'))
				u++;

			c = *u;
			*u = 0;

			strcat (build, t);
			strcat (build, "\n");
			*u = c;
			t = u-1;
		}
	}

	ret = (build[0] != 0);
	if (ret)
		Cbuf_AddText (build);

	Z_Free (text);
	Z_Free (build);

	return ret;
}


/*
==============================================================================

						SCRIPT COMMANDS

==============================================================================
*/


/*
=================
Cmd_SimulateHang_f

(FOR TESTING) Simulate the program hanging for a specified number of
milliseconds.
=================
*/
void Cmd_SimulateHang_f (void)
{
	if (Cmd_Argc() != 2) {
		Com_Printf ("Usage: hang <msec>\n");
	}
#if defined UNIX_VARIANT
#if defined HAVE_UNISTD_H
	usleep (atoi(Cmd_Argv(1))*1000);
#endif
#endif
}


/*
===============
Cmd_Exec_f
===============
*/
void Cmd_Exec_f (void)
{
	char	*f;
	int		len;

	if (Cmd_Argc () != 2)
	{
		Com_Printf ("exec <filename> : execute a script file\n");
		return;
	}

	len = FS_LoadFile (Cmd_Argv(1), (void **)&f);
	if (!f)
	{
		Com_Printf ("Could not exec %s\n",Cmd_Argv(1));
		return;
	}
	Com_Printf ("execing %s\n",Cmd_Argv(1));

	Cbuf_InsertText(f); /* FS_LoadFile nul-terminates the buffer */
	FS_FreeFile (f);
}

void Cmd_Exec_f_APPDATA (void)
{
	char	*f;
	int		len;

	if (Cmd_Argc () != 2)
	{
		Com_Printf ("exec <filename> : execute a script file\n");
		return;
	}

	len = FS_LoadFile (Cmd_Argv(1), (void **)&f);
	if (!f)
	{
		Com_Printf ("Could not exec %s\n",Cmd_Argv(1));
		return;
	}
	Com_Printf ("execing %s\n",Cmd_Argv(1));

	Cbuf_InsertText(f); /* FS_LoadFile nul-terminates the buffer */
	FS_FreeFile (f);
}


/*
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
void Cmd_Echo_f (void)
{
	int		i;

	for (i=1 ; i<Cmd_Argc() ; i++)
		Com_Printf ("%s ",Cmd_Argv(i));
	Com_Printf ("\n");
}

/*
===============
Cmd_Alias_f

Creates a new command that executes a command string (possibly ; seperated)
===============
*/
void Cmd_Alias_f (void)
{
	cmdalias_t	*a, *na, **prev;
	char		cmd[1024];
	unsigned int	hash_key;
	int		i, c;
	char		*s;

	if (Cmd_Argc() == 1)
	{
		Com_Printf ("Current alias commands:\n");
		for (a = cmd_alias ; a ; a=a->next)
			Com_Printf ("%s : %s\n", a->name, a->value);
		return;
	}

	s = Cmd_Argv(1);
	if (strlen(s) >= MAX_ALIAS_NAME)
	{
		Com_Printf ("Alias name is too long\n");
		return;
	}

	// compute the hash key for this alias
	COMPUTE_HASH_KEY( hash_key, s , i );

	// if the alias already exists, reuse it
	prev = &cmd_alias;
	for (a = cmd_alias ; a && a->hash_key <= hash_key ; a=a->next)
	{
		if (a->hash_key == hash_key && !Q_strcasecmp(s, a->name))
		{
			Z_Free (a->value);
			break;
		}
		prev = &( a->next );
	}

	if (!a)
	{
		a = Z_Malloc (sizeof(cmdalias_t));
		a->next = NULL;
		*prev = a;
	}
	else if ( a->hash_key != hash_key )
	{
		na = Z_Malloc (sizeof(cmdalias_t));
		na->next = a;
		*prev = na;
		a = na;
	}
	strcpy (a->name, s);
	a->hash_key = hash_key;

// copy the rest of the command line
	cmd[0] = 0;		// start out with a null string
	c = Cmd_Argc();
	for (i=2 ; i< c ; i++)
	{
		strcat (cmd, Cmd_Argv(i));
		if (i != (c - 1))
			strcat (cmd, " ");
	}
	strcat (cmd, "\n");

	a->value = CopyString (cmd);
}

/*
===============
Cmd_Unalias_f

Removes an existing alias using its name to look it up
===============
*/
void Cmd_Unalias_f (void)
{
	cmdalias_t	*a, **prev;
	unsigned int	hash_key;
	int		i;
	char		*s;

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("usage: unalias <alias>\n");
		return;
	}

	s = Cmd_Argv(1);

	// compute the hash key for this alias
	COMPUTE_HASH_KEY( hash_key, s , i );

	// find the alias
	prev = &cmd_alias;
	for (a = cmd_alias ; a && a->hash_key <= hash_key ; a=a->next)
	{
		if (a->hash_key == hash_key && !Q_strcasecmp(s, a->name))
		{
			break;
		}
		prev = &( a->next );
	}

	if (!a || a->hash_key != hash_key)
	{
		Com_Printf ("Alias not found\n");
		return;
	}

	*prev = a->next;
	Z_Free (a->value);
	Z_Free (a);
}

/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

typedef struct cmd_function_s
{
	struct cmd_function_s		*next;
	unsigned int			hash_key;
	char					*name;
	xcommand_t				function;
	xcompleter_t			completer;
	xcompletionchecker_t	checker;
	int						completion_data;
} cmd_function_t;


static	qboolean	cmd_force_command;
static	int		cmd_argc;
static	char		*cmd_argv[MAX_STRING_TOKENS];
static	char		*cmd_null_string = "";
static	char		cmd_args[MAX_STRING_CHARS];

static	cmd_function_t	*cmd_functions;		// possible commands to execute

/*
============
Cmd_Argc
============
*/
int		Cmd_Argc (void)
{
	return cmd_argc;
}

/*
============
Cmd_Argv
============
*/
char	*Cmd_Argv (int arg)
{
	if ( (unsigned)arg >= cmd_argc )
		return cmd_null_string;
	return cmd_argv[arg];
}

/*
============
Cmd_Args

Returns a single string containing argv(1) to argv(argc()-1)
============
*/
char		*Cmd_Args (void)
{
	return cmd_args;
}


/*
======================
Cmd_MacroExpandString
======================
*/
static const char *Cmd_MacroExpandString (const char *text)
{
	int		i, j, count, len;
	qboolean	inquote;
	const char	*scan;
	static	char	expanded[MAX_STRING_CHARS];
	char	temporary[MAX_STRING_CHARS];
	const char	*token, *start;

	inquote = false;
	scan = text;

	len = strlen (scan);
	if (len >= MAX_STRING_CHARS)
	{
		Com_Printf ("Line exceeded %i chars, discarded.\n", MAX_STRING_CHARS);
		return NULL;
	}

	count = 0;

	for (i=0 ; i<len ; i++)
	{
		if (scan[i] == '"')
			inquote ^= 1;
		if (inquote)
			continue;	// don't expand inside quotes
		if (scan[i] != '$')
			continue;
		// scan out the complete macro
		start = scan+i+1;
		token = COM_Parse (&start);
		if (!start)
			continue;

		token = Cvar_VariableString (token);

		j = strlen(token);
		len += j;
		if (len >= MAX_STRING_CHARS)
		{
			Com_Printf ("Expanded line exceeded %i chars, discarded.\n", MAX_STRING_CHARS);
			return NULL;
		}

		strncpy (temporary, scan, i);
		strcpy (temporary+i, token);
		strcpy (temporary+i+j, start);

		strcpy (expanded, temporary);
		scan = expanded;
		i--;

		if (++count == 100)
		{
			Com_Printf ("Macro expansion loop, discarded.\n");
			return NULL;
		}
	}

	if (inquote)
	{
		Com_Printf ("Line has unmatched quote, discarded.\n");
		return NULL;
	}

	return scan;
}


/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
$Cvars will be expanded unless they are in a quoted token
============
*/
void Cmd_TokenizeString (const char *text, qboolean macroExpand)
{
	int		i;
	char	*com_token;

// clear the args from the last string
	for (i=0 ; i<cmd_argc ; i++)
		Z_Free (cmd_argv[i]);

	cmd_argc = 0;
	cmd_args[0] = 0;
	cmd_force_command = false;

	// macro expand the text
	if (macroExpand)
		text = Cmd_MacroExpandString (text);
	if (!text)
		return;

	while (1)
	{
// skip whitespace up to a /n
		while (*text && *text <= ' ' && *text != '\n')
		{
			text++;
		}

		if (*text == '\n')
		{	// a newline seperates commands in the buffer
			text++;
			break;
		}

		if (!*text)
			return;

		// set cmd_args to everything after the first arg
		if (cmd_argc == 1)
		{
			int		l;

			// sku - removed potentional buffer overflow vulnerability
			strncpy( cmd_args, text, sizeof( cmd_args ) );

			// strip off any trailing whitespace
			l = strlen(cmd_args) - 1;
			for ( ; l >= 0 ; l--)
				if (cmd_args[l] <= ' ')
					cmd_args[l] = 0;
				else
					break;
		}

		com_token = COM_Parse (&text);
		if (!text)
			return;

		// if we're parsing the first arguments, check for "forced"
		// commands beginning with slashes or backslashes
		if ( cmd_argc == 0 && ! cmd_force_command )
		{
			cmd_force_command = ( com_token[0] == '/' || com_token[0] == '\\' );

			if ( cmd_force_command )
			{
				// if there was white space after the slash
				// or backslash, skip to the next token
				if ( com_token[1] == 0 )
					continue;

				// if not, skip the first character
				com_token ++;
			}
		}

		if (cmd_argc < MAX_STRING_TOKENS)
		{
			cmd_argv[cmd_argc] = Z_Malloc (strlen(com_token)+1);
			strcpy (cmd_argv[cmd_argc], com_token);
			cmd_argc++;
		}
	}

}


/*
============
Cmd_AddCommand
============
*/
void	Cmd_AddCommand (char *cmd_name, xcommand_t function)
{
	cmd_function_t	*cmd, **prev, *ncmd;
	unsigned int	hash_key, i;

	// fail if the command is a variable name
	if (Cvar_VariableString(cmd_name)[0])
	{
		Com_Printf ("Cmd_AddCommand: %s already defined as a var\n", cmd_name);
		return;
	}

	// compute the hash key for this command
	COMPUTE_HASH_KEY( hash_key, cmd_name , i );

	// fail if the command already exists (harmless if it's already the same.)
	prev = &cmd_functions;
	for (cmd=cmd_functions ; cmd && cmd->hash_key <= hash_key ; cmd=cmd->next)
	{
		if (cmd->hash_key == hash_key && !Q_strcasecmp (cmd_name, cmd->name))
		{
			if (cmd->function != function)
				Com_Printf ("Cmd_AddCommand: %s already defined\n", cmd_name);
			return;
		}
		prev = &( cmd->next );
	}

	ncmd = Z_Malloc (sizeof(cmd_function_t));
	ncmd->name = cmd_name;
	ncmd->hash_key = hash_key;
	ncmd->function = function;
	ncmd->completer = NULL;
	ncmd->next = cmd;
	*prev = ncmd;
}

/*
============
Cmd_RemoveCommand
============
*/
void	Cmd_RemoveCommand (char *cmd_name)
{
	cmd_function_t	*cmd, **back;
	unsigned int i, hash_key;

// compute the hash key for the command
	COMPUTE_HASH_KEY( hash_key, cmd_name , i );

	back = &cmd_functions;
	while (1)
	{
		cmd = *back;
		if (!cmd || cmd->hash_key > hash_key)
		{
			Com_Printf ("Cmd_RemoveCommand: %s not added\n", cmd_name);
			return;
		}
		if (cmd->hash_key == hash_key && !Q_strcasecmp (cmd_name, cmd->name))
		{
			*back = cmd->next;
			Z_Free (cmd);
			return;
		}
		back = &cmd->next;
	}
}

/*
============
Cmd_Exists
============
*/
qboolean	Cmd_Exists (char *cmd_name)
{
	cmd_function_t	*cmd;
	unsigned int i, hash_key;

	// compute the hash key for the command name
	COMPUTE_HASH_KEY (hash_key, cmd_name, i);

	for (cmd=cmd_functions ; cmd && cmd->hash_key <= hash_key; cmd=cmd->next)
	{
		if (cmd->hash_key == hash_key && !Q_strcasecmp (cmd_name,cmd->name))
			return true;
	}

	return false;
}

/*
============
Cmd_SetCompleter
============
*/
void	Cmd_SetCompleter (char *cmd_name, xcompleter_t completer, xcompletionchecker_t checker, int data)
{
	cmd_function_t	*cmd;
	unsigned int i, hash_key;

	// compute the hash key for the command name
	COMPUTE_HASH_KEY (hash_key, cmd_name, i);

	for (cmd=cmd_functions ; cmd && cmd->hash_key <= hash_key; cmd=cmd->next)
	{
		if (cmd->hash_key == hash_key && !Q_strcasecmp (cmd_name,cmd->name))
		{
			cmd->completer = completer;
			cmd->checker = checker;
			cmd->completion_data = data;
			return;
		}
	}
}



/*
============
Cmd_CompleteCommand
============
*/
char *Cmd_MakeCompletedCommand (int argnum, const char *new_tok)
{
	int i;
	static char		retval[256];
	
	retval[0] = '\0';
	
	for (i = 0; i < Cmd_Argc (); i++)
	{
		const char *txt = (i == argnum) ? new_tok : Cmd_Argv (i);
		strncat (retval, txt, sizeof (retval) - strlen (retval) - 1);
		if (i + 1 < Cmd_Argc ())
			strncat (retval, " ", sizeof (retval) - strlen (retval) - 1);
	}
	
	return retval;
}

char *Cmd_CompleteWithInexactMatch (int argnum, int nmatches, char **matches)
{
	qboolean        diff = false;
	int				i, j;
	char			matching_portion[256];
	
	if (nmatches == 0)
		return NULL;
	
	if (nmatches == 1)
		return Cmd_MakeCompletedCommand (argnum, matches[0]);
	
	Com_Printf("\nListing matches for '%s'...\n", Cmd_Argv (argnum));
	for (i = 0; i < nmatches; i++)
		Com_Printf("  %s\n", matches[i]);

	strcpy (matching_portion, "");
	j = 0;
	while (!diff && j < 256)
	{
		matching_portion[j] = matches[0][j];
		for (i = 0; i < nmatches; i++)
		{
			if (j > strlen (matches[i]))
				continue;
			if (matching_portion[j] != matches[i][j])
			{
				matching_portion[j] = 0;
				diff = true;
			}
		}
		j++;
	}
	Com_Printf ("Found %i matches\n", nmatches);
	return Cmd_MakeCompletedCommand (argnum, matching_portion);
}

char *Cmd_CompleteCommand (int argnum, int flags)
{
	static char		retval[256];
	char			*partial;
	cmd_function_t *cmd;
	int             len, i;
	cmdalias_t     *a;
	cvar_t         *cvar;
	char           *pmatch[1024];
	unsigned int	hash_key;

	if (argnum == 0 && Cmd_Argc () == 0)
		return NULL;
	if (Cmd_Argc () <= argnum)
		return Cmd_MakeCompletedCommand (argnum, NULL);
	partial = Cmd_Argv (argnum);
	len = strlen(partial);

    /* check for exact match */
	COMPUTE_HASH_KEY (hash_key, partial, i);

	if (flags & COMPLETION_COMMANDS)
		for (cmd = cmd_functions; cmd && cmd->hash_key <= hash_key; cmd = cmd->next)
			if (hash_key == cmd->hash_key && !Q_strcasecmp(partial, cmd->name))
			{
				if (argnum == 0 && cmd->completer != NULL)
					return cmd->completer (1, cmd->completion_data); 
				return Cmd_MakeCompletedCommand (argnum, cmd->name);
			}

	if (flags & COMPLETION_ALIASES)
		for (a = cmd_alias; a && a->hash_key <= hash_key; a = a->next)
			if (hash_key == a->hash_key && !Q_strcasecmp(partial, a->name))
				return Cmd_MakeCompletedCommand (argnum, a->name);

	if (flags & COMPLETION_CVARS)
		for (cvar = cvar_vars; cvar && cvar->hash_key <= hash_key; cvar = cvar->next)
			if (hash_key == cvar->hash_key && !Q_strcasecmp(partial, cvar->name))
				return Cmd_MakeCompletedCommand (argnum, cvar->name);

	// clear matches
	for (i = 0; i < 1024; i++)
		pmatch[i] = NULL;
	i = 0;

	/* check for partial match */
	if (flags & COMPLETION_COMMANDS)
		for (cmd = cmd_functions; cmd; cmd = cmd->next)
			if (!Q_strncasecmp(partial, cmd->name, len)) {
				pmatch[i] = cmd->name;
				i++;
			}
	if (flags & COMPLETION_ALIASES)
		for (a = cmd_alias; a; a = a->next)
			if (!Q_strncasecmp(partial, a->name, len)) {
				pmatch[i] = a->name;
				i++;
			}
	if (flags & COMPLETION_CVARS)
		for (cvar = cvar_vars; cvar; cvar = cvar->next)
			if (!Q_strncasecmp(partial, cvar->name, len)) {
				pmatch[i] = cvar->name;
				i++;
			}
	
	return Cmd_CompleteWithInexactMatch (argnum, i, pmatch);
}

qboolean Cmd_IsComplete (int argnum, int flags)
{
	char *command;
	cmd_function_t *cmd;
	cmdalias_t     *a;
	cvar_t         *cvar;
	unsigned int	hash_key, i;
	
	if (Cmd_Argc () <= argnum)
		return argnum > 0;
	command = Cmd_Argv (argnum);
	
	/* check for exact match */
	COMPUTE_HASH_KEY (hash_key, command, i);

	if (flags & COMPLETION_COMMANDS)
		for (cmd = cmd_functions; cmd && cmd->hash_key <= hash_key; cmd = cmd->next)
			if (hash_key == cmd->hash_key && !Q_strcasecmp(command, cmd->name))
			{
				if (argnum == 0 && cmd->checker != NULL)
					return cmd->checker (1, cmd->completion_data);
				return Cmd_Argc () - 1 == argnum;
			}

	// Never attempt to complete multi-token commands starting with aliases or
	// cvars.
	if (Cmd_Argc () > argnum + 1)
		return false;

	if (flags & COMPLETION_ALIASES)
		for (a = cmd_alias; a && a->hash_key <= hash_key; a = a->next)
			if (hash_key == a->hash_key && !Q_strcasecmp(command, a->name))
				return true;

	if (flags & COMPLETION_CVARS)
		for (cvar = cvar_vars; cvar && cvar->hash_key <= hash_key; cvar = cvar->next)
			if (hash_key == cvar->hash_key && !Q_strcasecmp(command, cvar->name))
				return true;

	return false;
}

/*
============
Cmd_ExecuteString

A complete command line has been parsed, so try to execute it
FIXME: lookupnoadd the token to speed search?
============
*/
void	Cmd_ExecuteString (char *text)
{
	cmd_function_t	*cmd;
	cmdalias_t	*a;
	unsigned int	i, hash_key;

	Cmd_TokenizeString (text, true);

	// execute the command line
	if (!Cmd_Argc())
		return;		// no tokens

	// compute the hash key for the command
	COMPUTE_HASH_KEY( hash_key, cmd_argv[0] , i );

	// check functions
	for (cmd=cmd_functions ; cmd && cmd->hash_key <= hash_key; cmd=cmd->next)
	{
		if (cmd->hash_key == hash_key && !Q_strcasecmp (cmd_argv[0],cmd->name))
		{
			if (!cmd->function)
			{	// forward to server command
				Cmd_ExecuteString (va("cmd %s %s", cmd_argv[0], cmd_args));
			}
			else
				cmd->function ();
			return;
		}
	}

	// check alias
	for (a=cmd_alias ; a && a->hash_key <= hash_key ; a=a->next)
	{
		if (a->hash_key == hash_key && !Q_strcasecmp (cmd_argv[0], a->name))
		{
			if (++alias_count == ALIAS_LOOP_COUNT)
			{
				Com_Printf ("ALIAS_LOOP_COUNT\n");
				return;
			}
			Cbuf_InsertText (a->value);
			return;
		}
	}

	// check cvars
	if (Cvar_Command ())
		return;

	// send it as a server command if we are connected
	// and if the command wasn't forced
	if ( ! cmd_force_command )
		Cmd_ForwardToServer ();
	else
		Com_Printf ("Unknown command \"%s\"\n", cmd_argv[0]);
}

/*
============
Cmd_List_f
============
*/
void Cmd_List_f (void)
{
	cmd_function_t	*cmd;
	int				matching, pattern;

	matching = 0;
	for (cmd = cmd_functions; cmd; cmd = cmd->next)
	{
		qboolean failedmatch = false;
		for (pattern = 1; pattern < Cmd_Argc () && !failedmatch; pattern++)
			failedmatch = !Com_PatternMatch (cmd->name, Cmd_Argv (pattern));
		if (failedmatch)
			continue;
		matching++;
		
		Com_Printf ("%s\n", cmd->name);
	}
	
	Com_Printf ("%i matching commands\n", matching);
	if (Cmd_Argc () == 1)
		Com_Printf ("Try cmdlist <pattern1> <pattern2>, <pattern3>... to narrow it down.\n");
	if (Cmd_Argc () == 1 || matching == 0)
	{
		Com_Printf ("You may use \"*\" as a wildcard in your search patterns.\n");
		Com_Printf ("All patterns must match in the command name in order for a cmd to be listed.\n");
	}
}

/*
============
Cmd_Init
============
*/
void Cmd_Init (void)
{
//
// register our commands
//
	Cmd_AddCommand ("cmdlist",Cmd_List_f);
	Cmd_AddCommand ("exec",Cmd_Exec_f);
	Cmd_AddCommand ("echo",Cmd_Echo_f);
	Cmd_AddCommand ("alias",Cmd_Alias_f);
	Cmd_AddCommand ("unalias",Cmd_Unalias_f);
	Cmd_SetCompleter ("unalias", Cmd_CompleteCommand, Cmd_IsComplete, COMPLETION_ALIASES);
	Cmd_AddCommand ("wait", Cmd_Wait_f);
	Cmd_AddCommand ("hang", Cmd_SimulateHang_f);
}

