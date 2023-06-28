/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2010 COR Entertainment, LLC.

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
// cvar.c -- dynamic variable tracking

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "qcommon.h"

cvar_t	*cvar_vars;

/**
 * \brief Make sure skin cvar contains exactly one '/'.
 *
 * If user sets skin to a value without the slash, the info string,
 * config.cfg, team membership and probably other things get really
 * messed up.
 *
 * \param skin The skin string value to check.
 *
 * \return If ok return original, otherwise return a default skin.
 */
static const char *Cvar_SkinValidate( const char *skin )
{
	const char* pc = skin;
	int count = 0;
	while ( *pc )
	{
		if ( *pc == '/' )
			++count;
		++pc;
	}
	if ( count == 1 )
	{
		return skin;
	}
	else
	{
		Com_Printf("Invalid skin name. Using default.\n");
		return "martianenforcer/default";
	}
}

/*
============
Cvar_InfoValidate
============
*/
static qboolean Cvar_InfoValidate (const char *s)
{
	if (strstr (s, "\\"))
		return false;
	if (strstr (s, "\""))
		return false;
	if (strstr (s, ";"))
		return false;
	return true;
}

/*
============
Cvar_FindVar
============
*/
static cvar_t *Cvar_FindVar (const char *var_name)
{
	cvar_t		*var;
	unsigned int	i, hash_key;

	COMPUTE_HASH_KEY( hash_key, var_name , i );
	for (var=cvar_vars ; var && var->hash_key <= hash_key ; var=var->next)
		if (var->hash_key == hash_key && !Q_strcasecmp (var_name, var->name))
			return var;

	return NULL;
}

/*
============
Cvar_VariableValue
============
*/
float Cvar_VariableValue (const char *var_name)
{
	cvar_t	*var;

	var = Cvar_FindVar (var_name);
	if (!var)
		return 0;
	return var->value;
}


/*
============
Cvar_VariableString
============
*/
char *Cvar_VariableString (const char *var_name)
{
	cvar_t *var;

	var = Cvar_FindVar (var_name);
	if (!var)
		return "";
	return var->string;
}


/*
============
Cvar_CompleteVariable
============
*/
char *Cvar_CompleteVariable (const char *partial)
{
	cvar_t		*cvar;
	int		len, i;
	unsigned int	hash_key;

	len = strlen(partial);
	if (!len)
		return NULL;

	// check exact match
	COMPUTE_HASH_KEY( hash_key, partial , i );
	for (cvar=cvar_vars ; cvar && cvar->hash_key <= hash_key; cvar=cvar->next)
		if (cvar->hash_key == hash_key && !Q_strcasecmp (partial,cvar->name))
			return cvar->name;

	// check partial match
	for (cvar=cvar_vars ; cvar ; cvar=cvar->next)
		if (!Q_strncasecmp (partial,cvar->name, len))
			return cvar->name;

	return NULL;
}


/*
============
Cvar_Allocate

Creates a new variable's record
============
*/
inline static cvar_t *Cvar_Allocate(const char *var_name, const char *var_value, int flags, unsigned int hash_key)
{
	cvar_t *nvar;

	nvar = Z_Malloc (sizeof(cvar_t));
	nvar->name = CopyString (var_name);
	nvar->string = CopyString (var_value);
	nvar->modified = true;
	nvar->value = atof (nvar->string);
	nvar->default_value = atof (nvar->string);
	nvar->integer = atoi (nvar->string);
	nvar->flags = flags;
	nvar->hash_key = hash_key;
	nvar->description = NULL;

	return nvar;
}


/*
============
Anon_Cvar_Allocate

Initializes a new cvar struct holding just a value, with no name in it.
Used in some places where a weak-typed variable is needed.
============
*/ 
void Anon_Cvar_Set(cvar_t *nvar, const char *var_value)
{
	if (nvar->string)
		Z_Free (nvar->string);
	nvar->string = CopyString (var_value);
	nvar->value = atof (nvar->string);
	nvar->default_value = atof (nvar->string);
	nvar->integer = atoi (nvar->string);
}


/*
============
Cvar_AddBetween

Adds a variable between two others.
============
*/
static cvar_t *Cvar_AddBetween(
	const char *var_name, 
	const char *var_value,
	int flags,
	unsigned int hash_key,
	cvar_t **prev, 
	cvar_t *next )
{
	cvar_t *nvar;

	// variable needs to be created, check parameters
	if (!var_value)
		return NULL;

	if ( !Q_strcasecmp( var_name, "skin" ) )
	{
		var_value = Cvar_SkinValidate( var_value );
	}

	if (flags & (CVAR_USERINFO | CVAR_SERVERINFO | CVAR_GAMEINFO))
	{
		if (!Cvar_InfoValidate (var_value))
		{
			Com_Printf("invalid info cvar value\n");
			return NULL;
		}
	}

	// create the variable
	nvar = Cvar_Allocate( var_name , var_value , flags , hash_key );

	// link the variable in
	nvar->next = next;
	*prev = nvar;

	return nvar;
}


/*
============
Cvar_Get

If the variable already exists, the value will not be set
The flags will be or'ed in if the variable exists.
============
*/
cvar_t *Cvar_Get (const char *var_name, const char *var_value, int flags)
{
	cvar_t		*var, **prev;
	unsigned int	i, hash_key;

	if ( !Q_strcasecmp( var_name, "skin" ) )
	{
		var_value = Cvar_SkinValidate( var_value );
	}

	// validate variable name
	if (flags & (CVAR_USERINFO | CVAR_SERVERINFO | CVAR_GAMEINFO))
	{
		if (!Cvar_InfoValidate (var_value))
		{
			Com_Printf("invalid info cvar value\n");
			return NULL;
		}
	}

	// try finding the variable
	prev = &cvar_vars;
	COMPUTE_HASH_KEY( hash_key , var_name , i );
	for (var = cvar_vars ; var && var->hash_key <= hash_key ; var=var->next)
	{
		if (var->hash_key == hash_key && !Q_strcasecmp (var_name, var->name))
		{
			var->flags |= flags;
			if (var_value)
			    var->default_value = atof (var_value);
			return var;
		}
		prev = &( var->next );
	}

	return Cvar_AddBetween(var_name , var_value , flags , hash_key , prev , var);
}


/*
============
Cvar_FindOrCreate

This function is mostly similar to Cvar_Get(name,value,0). However, it starts
by attempting to find the variable, as there are no flags. In addition, it
returns a boolean depending on whether the variable was found or created, and
sets the pointer to the variable through a parameter.
============
*/
inline static qboolean Cvar_FindOrCreate (
		const char *var_name, const char *var_value, int flags, cvar_t **found)
{
	cvar_t		*var, **prev;
	unsigned int	i, hash_key;

	// try finding the variable
	prev = &cvar_vars;
	COMPUTE_HASH_KEY( hash_key , var_name , i );
	for (var = cvar_vars ; var && var->hash_key <= hash_key ; var=var->next)
	{
		if (var->hash_key == hash_key && !Q_strcasecmp (var_name, var->name))
		{
			var->flags |= flags;
			*found = var;
			return true;
		}
		prev = &( var->next );
	}

	*found = Cvar_AddBetween(var_name , var_value , flags , hash_key , prev , var);
	return false;
}


/*
============
Cvar_Set2
============
*/
cvar_t *Cvar_Set2 (const char *var_name, const char *value, qboolean force)
{
	cvar_t	*var;

    /*  debug/test cvar tracing */
	/* Com_Printf("[Cvar_Set2: %s := %s]\n", var_name, value ); */

	// Find the variable; if it does not exist, create it and return at once
	if (!Cvar_FindOrCreate (var_name, value, 0, &var))
	{
		return var;
	}

	if ( var->flags & CVAR_ROM )
	{
		Com_Printf( "%s is read-only.\n", var_name );
		return var;
	}

	if ( !Q_strcasecmp( var_name, "skin" ) )
	{
		value = Cvar_SkinValidate( value );
	}

	if (var->flags & (CVAR_USERINFO | CVAR_SERVERINFO | CVAR_GAMEINFO))
	{
		if (!Cvar_InfoValidate (value))
		{
			Com_Printf("invalid info cvar value\n");
			return var;
		}
	}

	if (!force) /* Cvar_Set */
	{
		if (var->flags & CVAR_NOSET )
		{
			Com_Printf ("%s is write protected.\n", var_name);
			return var;
		}

		if (var->flags & CVAR_LATCH)
		{
			if (var->latched_string)
			{
				if (strcmp(value, var->latched_string) == 0)
					return var;
				Z_Free (var->latched_string);
			}
			else
			{
				if (strcmp(value, var->string) == 0)
					return var;
			}

			if (Com_ServerState())
			{
				Com_Printf ("%s cvar will be changed for next game.\n", var_name);
				Com_Printf( " startmap <mapname> will start next game.\n");
				var->latched_string = CopyString(value);
			}
			else
			{
				var->string  = CopyString( value );
				var->value   = atof( var->string );
				var->integer = atoi( var->string );
			}
			return var;
		}
	}
	else /* Cvar_ForceSet */
	{
		if (var->latched_string)
		{
			Z_Free (var->latched_string);
			var->latched_string = NULL;
		}
	}

	if (!strcmp(value, var->string))
		return var;		// not changed

	var->modified = true;

	if (var->flags & CVAR_USERINFO)
		userinfo_modified = true;	// transmit at next oportunity

	Z_Free (var->string);	// free the old value string

	var->string = CopyString(value);
	var->value = atof (var->string);
	var->integer = atoi(var->string);

	return var;
}

/*
============
Cvar_ForceSet
============
*/
cvar_t *Cvar_ForceSet (const char *var_name, const char *value)
{
	/* true means "force" */
	return Cvar_Set2 (var_name, value, true);
}

/*
============
Cvar_Set
============
*/
void Cvar_Set (const char *var_name, const char *value)
{
	/* false means "do not force" */
	(void)Cvar_Set2 (var_name, value, false);
}

/*
============
Cvar_FullSet
============
*/
void Cvar_FullSet (const char *var_name, const char *value, int flags)
{
	cvar_t	*var;

	if (!Cvar_FindOrCreate (var_name, value, flags, &var))
		return;

	var->modified = true;

	if (var->flags & CVAR_USERINFO)
		userinfo_modified = true;	// transmit at next oportunity

	Z_Free (var->string);	// free the old value string

	var->string = CopyString(value);
	var->value = atof (var->string);
	var->default_value = var->value;
	var->integer = atoi(var->string);
	var->flags = flags;
}

/*
============
Cvar_SetValue
============
*/
void Cvar_SetValue (const char *var_name, float value)
{
	char	val[32];

	if (value == (int)value)
		Com_sprintf (val, sizeof(val), "%i",(int)value);
	else
		Com_sprintf (val, sizeof(val), "%f",value);
	Cvar_Set (var_name, val);
}


/*
============
Cvar_GetLatchedVars

Any variables with latched values will now be updated
============
*/
void Cvar_GetLatchedVars (void)
{
	cvar_t	*var;
	for (var = cvar_vars ; var ; var = var->next)
	{
		if (!var->latched_string)
			continue;

		// observe when latched cvars are updated
		Com_Printf("Updating latched cvar %s from %s to %s\n",
				var->name, var->string, var->latched_string );

		Z_Free (var->string);
		var->string = var->latched_string;
		var->latched_string = NULL;
		var->value = atof(var->string);
		var->integer = atoi(var->string);
	}
}

/*
============
Cvar_Command

Handles variable inspection and changing from the console
============
*/
qboolean Cvar_Command (void)
{
	cvar_t			*v;

// check variables
	v = Cvar_FindVar (Cmd_Argv(0));
	if (!v)
		return false;

// perform a variable print or set
	if (Cmd_Argc() == 1)
	{
		Com_Printf ("\"%s\" is \"%s\"\n", v->name, v->string);
		return true;
	}

	Cvar_Set (v->name, Cmd_Argv(1));
	return true;
}


/*
============
Cvar_Set_f

Allows setting and defining of arbitrary cvars from console
============
*/
void Cvar_Set_f (void)
{
	int		c;
	int		flags;

	c = Cmd_Argc();
	if (c != 3 && c != 4)
	{
		Com_Printf ("usage: set <variable> <value> [u / s / g]\n");
		return;
	}
	
	if (strchr (Cmd_Argv(1), '*'))
	{
		Com_Printf ("set: variable names may not contain the \"*\" character!\n");
		return;
	}

	if (c == 4)
	{
		if (!strcmp(Cmd_Argv(3), "u"))
			flags = CVAR_USERINFO;
		else if (!strcmp(Cmd_Argv(3), "s"))
			flags = CVAR_SERVERINFO;
		else if (!strcmp(Cmd_Argv(3), "g"))
		    flags = CVAR_GAMEINFO;
		else
		{
			Com_Printf ("flags can only be 'u', 's', or 'g'\n");
			return;
		}
		Cvar_FullSet (Cmd_Argv(1), Cmd_Argv(2), flags);
	}
	else
		Cvar_Set (Cmd_Argv(1), Cmd_Argv(2));
}


/*
============
Cvar_WriteVariables

Appends lines containing "set variable value" for all variables
with the archive flag set to true.
============
*/
void Cvar_WriteVariables (char *path)
{
	cvar_t	*var;
	char	buffer[1024];
	FILE	*f;

	f = fopen (path, "a");
	for (var = cvar_vars ; var ; var = var->next)
	{
		if (var->flags & CVAR_ARCHIVE)
		{
			Com_sprintf (buffer, sizeof(buffer), "set %s \"%s\"\n", var->name, var->string);
			fprintf (f, "%s", buffer);
		}
	}
	fclose (f);
}

/*
============
Cvar_List_f

List all cvars, also displaying their documentation if appropriate
============
*/
void Cvar_List_f (void)
{
	cvar_t	*var;
	int		matching, pattern;

	matching = 0;
	for (var = cvar_vars; var; var = var->next)
	{
		qboolean failedmatch = false;
		for (pattern = 1; pattern < Cmd_Argc () && !failedmatch; pattern++)
		{
			failedmatch = 
				!Com_PatternMatch (var->name, Cmd_Argv (pattern)) &&
				(var->description == NULL || !Com_PatternMatch (var->description, Cmd_Argv (pattern)));
		}
		if (failedmatch)
			continue;
		matching++;
		
		if (var->flags & CVAR_ARCHIVE)
			Com_Printf ("*");
		else
			Com_Printf (" ");
		if (var->flags & CVAR_USERINFO)
			Com_Printf ("U");
		else
			Com_Printf (" ");
		if (var->flags & CVAR_SERVERINFO)
			Com_Printf ("S");
		else
			Com_Printf (" ");
		if (var->flags & CVAR_NOSET)
			Com_Printf ("-");
		else if (var->flags & CVAR_LATCH)
			Com_Printf ("L");
		else
			Com_Printf (" ");
		Com_Printf (" %s \"%s\"", var->name, var->string);
		if (var->description)
			Com_Printf (" - %s", var->description);
		Com_Printf ("\n");
	}
	Com_Printf ("%i matching cvars\n", matching);
	if (Cmd_Argc () == 1)
		Com_Printf ("Try cvarlist <pattern1> <pattern2>, <pattern3>... to narrow it down.\n");
	if (Cmd_Argc () == 1 || matching == 0)
	{
		Com_Printf ("You may use \"*\" as a wildcard in your search patterns.\n");
		Com_Printf ("All patterns must match in the cvar name or description in order for a cvar to be listed.\n");
	}
}

/*
============
Cvar_Help_f

If the cvar exists, display its current value and documentation
============
*/
void Cvar_Help_f (void)
{
	cvar_t	*var;
	char	*var_name;
	int		flags;
	
	if (Cmd_Argc() != 2)
	{
		Com_Printf ("Usage: help <variable>\n");
		Com_Printf ("Displays the cvar's type, value, and help string if present.\n");
		return;
	}
	
	var_name = Cmd_Argv(1);
	var = Cvar_Get (var_name, 0, 0);
	
	if (var == NULL)
	{
		Com_Printf ("Nonexistant cvar \"%s\"\n", var_name);
		return;
	}
	
	Com_Printf ("Name: \"%s\"\n", var_name);
	
	flags = var->flags;
	if (flags != 0)
	{
		Com_Printf ("Type: ");
		
#define HANDLE_FLAG(flag,str)\
		if (flags & (flag))\
		{\
			Com_Printf (str);\
			flags &= ~(flag);\
			if (flags != 0) \
				Com_Printf (", ");\
		}
		
		HANDLE_FLAG (CVAR_ARCHIVE, "saved in config.cfg");
		HANDLE_FLAG (CVAR_USERINFO, "player setting");
		HANDLE_FLAG (CVAR_SERVERINFO, "server setting");
		HANDLE_FLAG (CVAR_NOSET, "can be set only at the command line");
		HANDLE_FLAG (CVAR_LATCH, "changes not applied until next game");
		HANDLE_FLAG (CVAR_ROM, "read-only");
		HANDLE_FLAG (CVAR_GAMEINFO, "gameplay setting");
		HANDLE_FLAG (CVAR_PROFILE, "saved in profile.cfg");
		
		// Usually, at most one of these will be set.
		HANDLE_FLAG (CVARDOC_BOOL, "0 for disabled, nonzero for enabled");
		HANDLE_FLAG (CVARDOC_STR, "not a number");
		HANDLE_FLAG (CVARDOC_FLOAT, "floating-point number");
		HANDLE_FLAG (CVARDOC_INT, "integer");
			
#undef HANDLE_FLAG
		
		Com_Printf ("\n");
	}
	
	Com_Printf ("Value: \"%s\"\n", var->string);
	
	if (var->description != NULL)
		Com_Printf ("Description: %s\n", var->description);
}

qboolean userinfo_modified;


char	*Cvar_BitInfo (int bit)
{
	static char	info[MAX_INFO_STRING];
	cvar_t	*var;

	info[0] = 0;

	for (var = cvar_vars ; var ; var = var->next)
	{
		if (var->flags & bit)
		{
			Info_SetValueForKey (info, var->name, var->string);
		}
	}
	return info;
}

// returns an info string containing all the CVAR_USERINFO cvars
char	*Cvar_Userinfo (void)
{
	return Cvar_BitInfo (CVAR_USERINFO);
}

// returns an info string containing all the CVAR_SERVERINFO cvars
char	*Cvar_Serverinfo (void)
{
	//add the "mods" field
    char *gameinfo;
    char ruleset[MAX_INFO_KEY];
    char *token;
    char lasttoken[MAX_INFO_KEY];
    char current_rule[MAX_INFO_KEY];
    static char info[MAX_INFO_STRING];
    cvar_t *cur_cvr;
    Com_sprintf(info, sizeof(info), Cvar_BitInfo (CVAR_SERVERINFO));    
    gameinfo = Cvar_BitInfo (CVAR_GAMEINFO);
    
    lasttoken[0] = 0;
    memset(ruleset, 0, sizeof(ruleset));
    token = strtok (gameinfo, "\\");
    while (token) {
        cur_cvr = Cvar_Get(lasttoken, NULL, 0);
        if (!cur_cvr || !strlen(token) || (atof(token) == cur_cvr->default_value)) {
            //cvar either doesn't exist or is set to its default value
            //we only send existent, non-default cvars
            current_rule[0] = 0; //empty string
        } else if (atof(token) == 1.0f) {
            //if the value is 1, don't add the value
            Com_sprintf(current_rule, sizeof(current_rule), "%%%s", lasttoken);
        } else {
            //any value which is not 1 will be displayed
            Com_sprintf(current_rule, sizeof(current_rule), "%%%s=%s", lasttoken, token);
        }
        if (strlen(current_rule)+strlen(ruleset)+strlen("mods")+strlen("\\\\")+strlen(info) >= MAX_INFO_STRING)
            //this mod cannot be added to the ruleset because it would make 
            //ruleset too long to fit in info.
            break;
        strncat(ruleset, current_rule, MAX_INFO_KEY-strlen(ruleset)-1);
        Com_sprintf(lasttoken, sizeof(lasttoken), "%s", token);
        token = strtok(NULL, "\\");
    }
    Info_SetValueForKey (info, "mods", ruleset);
    return info;
}

void	Cvar_Describe(cvar_t *var, const char *description_string)
{
	if (var == NULL)
		return;
	if (var->description != NULL)
		Z_Free (var->description);
	if (description_string == NULL)
		var->description = NULL;
	else
		var->description = CopyString (description_string);
}

void	Cvar_Describe_ByName(const char *var_name, const char *description_string)
{
	cvar_t *var;
	if (var_name == NULL)
		return;
	var = Cvar_Get (var_name, 0, 0);
	Cvar_Describe (var, description_string);
}

/*
============
Cvar_Init

Reads in all archived cvars
============
*/
void Cvar_Init (void)
{
	Cmd_AddCommand ("set", Cvar_Set_f);
	Cmd_SetCompleter ("set", Cmd_CompleteCommand, Cmd_IsComplete, COMPLETION_CVARS);
	Cmd_AddCommand ("cvarlist", Cvar_List_f);
	Cmd_AddCommand ("help", Cvar_Help_f);
	Cmd_SetCompleter ("help", Cmd_CompleteCommand, Cmd_IsComplete, COMPLETION_CVARS);

}
