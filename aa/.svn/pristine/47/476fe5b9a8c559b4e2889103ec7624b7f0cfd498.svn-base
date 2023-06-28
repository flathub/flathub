#include <stdio.h>
#include <assert.h>

#include "qcommon.h"

// stand-in functions

int Sys_Milliseconds (void) {return 0;}

char	*Sys_GetCommand (void) {return NULL;}
void	Sys_Print (const char *text) {}
void	Sys_ShowConsole (qboolean show) {}

void	Sys_Init (void) {}

void	Sys_AppActivate (void) {}

void	Sys_UnloadGame (void) {}
void	*Sys_GetGameAPI (void *parms) {return NULL;}

char	*Sys_ConsoleInput (void) {return NULL;}
void	Sys_ConsoleOutput (char *string) {}
void	Sys_SendKeyEvents (void) {}
void	Sys_Error (char *error, ...) {}
void	Sys_Quit (void) {}
char	*Sys_GetClipboardData( void ) {return NULL;}
void	Sys_CopyProtect (void) {}

void	Sys_Mkdir (char *path) {}

char	*Sys_FindFirst (char *path, unsigned musthave, unsigned canthave ) {return NULL;}
char	*Sys_FindNext ( unsigned musthave, unsigned canthave ) {return NULL;}
void	Sys_FindClose (void) {}

cvar_t *developer;
cvar_t _developer[1] = {{.integer = 0}};

cvar_t *Cvar_Get (const char *var_name, const char *value, int flags)
{
    static cvar_t v;
    v.string = "";
    return &v;
}
void Cvar_FullSet (const char *var_name, const char *value, int flags) {}
char *Cvar_VariableString (const char *var_name) {return NULL;}

void *Z_Malloc (int size) {return malloc (size);}
void Z_Free (void *ptr) {free (ptr);}

void Cbuf_AddText (char *text) {}

void Com_Printf (char *fmt, ...)
{
	va_list		argptr;

	va_start (argptr,fmt);
	vprintf(fmt, argptr);
	va_end (argptr);
}

void Com_DPrintf (char *fmt, ...)
{
	va_list		argptr;

	va_start (argptr,fmt);
	vprintf(fmt, argptr);
	va_end (argptr);
}

void Com_Error (int code, char *fmt, ...)
{
	va_list		argptr;

	va_start (argptr,fmt);
	vprintf(fmt, argptr);
	va_end (argptr);
}

char *CopyString (const char *in)
{
	char	*out;

	out = Z_Malloc (strlen(in)+1);
	strcpy (out, in);
	return out;
}

int	Cmd_Argc (void) {return 0;}
char	*Cmd_Argv (int arg) {return NULL;}

void	Cmd_AddCommand (char *cmd_name, xcommand_t function) {}

float	frand(void)
{
	const float randmax_scale = 1.0f/(float)RAND_MAX;

	return (float)rand() * randmax_scale;
}

void standin_init (void)
{
    developer = _developer;
    Swap_Init ();
    FS_InitFilesystem ();
}
