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

#ifndef __CL_CONSOLE_H
#define __CL_CONSOLE_H

//
// console
//


/**************************************************************************/
/* CONSTANTS                                                              */
/**************************************************************************/

/* Maximal length of a command line */
#define MAXCMDLINE		256

/* Maximal amount of lines in the console's buffer */
#define	CON_MAX_LINES		4096

/* Length of a line in the console's buffer */
#define	CON_LINE_LENGTH		128

/* Maximal amount of in-game notifications */
#define CON_MAX_NOTIFY		4



/**************************************************************************/
/* DATA STRUCTURES                                                        */
/**************************************************************************/

/*
 * The console's main data structure.
 *
 * It includes the console's buffers, notification timestamps, and display
 * status.
 */
struct CON_console_s
{
	/* Indicates that the console has been initialised */
	qboolean	initialised;

	/* Text in the console buffer */
	char		text[ CON_MAX_LINES ][ CON_LINE_LENGTH ];
	int		lCount[ CON_MAX_LINES ];

	/* Height of the various lines */
	int		heights[ CON_MAX_LINES ];

	/* Amount of lines in the console buffer */
	int		lines;

	/* Current line */
	int		curLine;

	/* Offset in the current line */
	int		offset;

	/* Time at which the last lines were generated - used for notifications */
	float		times[ CON_MAX_NOTIFY ];
	int		curTime;

	/* Previous display parameters, if any */
	int		pWidth;
	char		pFace[ FNT_FACE_NAME_MAX ];
	unsigned int	pSize;

	/* Offset from which the console is being displayed */
	int		displayOffset;
};

/* The console's main structure */
extern struct CON_console_s	CON_console;



/**************************************************************************/
/* CONSOLE FUNCTIONS                                                      */
/**************************************************************************/

/*
 * Initialise the console.
 *
 * This function prepares the console for text storage, and registers CVars
 * and commands associated with both the console and the in-game chat line.
 */
void CON_Initialise( );


/*
 * Add text to the console.
 *
 * This function prints text to the console's buffer. Notification timestamps
 * are updated, and line heights are set to 0 for modified lines.
 *
 * Parameters:
 *	text	the text to add
 */
void CON_Print( const char * text );


/*
 * Clear the console.
 *
 * This function resets the console's buffer as well as notification
 * timestamps.
 */
void CON_Clear( );


/*
 * Draw the console.
 *
 * Parameters:
 *	relSize		vertical size of the console relative to the screen's
 *			size
 */
void CON_DrawConsole( float relSize );


/*
 * Show or hide the console
 */
void CON_ToggleConsole( );



/**************************************************************************/
/* NOTIFICATION FUNCTIONS                                                 */
/**************************************************************************/

/*
 * Clear notification timestamps.
 *
 * This function resets the console's notification timestamps without
 * affecting the rest of the console's data.
 */
void CON_ClearNotify( );

/*
 * Draw the few last lines of the console transparently over the game.
 */
void CON_DrawNotify( );



/**************************************************************************/
/* CONSOLE LINE EDITION DATA                                              */
/**************************************************************************/

extern	char			key_lines[32][MAXCMDLINE];
extern	int			edit_line;
extern	int			key_linepos;
extern	int			key_linelen;


#endif // __CL_CONSOLE_H
