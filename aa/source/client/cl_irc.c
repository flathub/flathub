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

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cl_irc.c  -- irc client



#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "client.h"
#include "qcommon/htable.h"

#if defined WIN32_VARIANT
# include <winsock.h>
# include <process.h>
  typedef SOCKET irc_socket_t;
#else
# if defined HAVE_UNISTD_H
#  include <unistd.h>
# endif
# include <sys/socket.h>
# include <sys/time.h>
# include <netinet/in.h>
# include <netdb.h>
# include <sys/param.h>
# include <sys/ioctl.h>
# include <sys/uio.h>
# include <errno.h>
# include <pthread.h>
  typedef int irc_socket_t;
# if !defined HAVE_CLOSESOCKET
#  define closesocket close
# endif
# if !defined INVALID_SOCKET
#  define INVALID_SOCKET -1
# endif
#endif


/* IRC control cvars */
cvar_t * cl_IRC_connect_at_startup;
cvar_t * cl_IRC_server;
cvar_t * cl_IRC_channel;
cvar_t * cl_IRC_port;
cvar_t * cl_IRC_override_nickname;
cvar_t * cl_IRC_nickname;
cvar_t * cl_IRC_kick_rejoin;
cvar_t * cl_IRC_reconnect_delay;


/*
 * Timing controls
 *
 * In order to avoid actively waiting like crazy, there are many parts of the
 * IRC client code that need to sleep or wait for a timeout. However, if the
 * wait is too long, it makes the whole thing unreactive to e.g. the irc_say
 * command; if the wait is too shot, it starts using CPU time.
 *
 * The constants below control the timeouts.
 */

#define IRC_TIMEOUT_MS		250
#define IRC_TIMEOUT_US		( IRC_TIMEOUT_MS * 1000 )
#define IRC_TIMEOUTS_PER_SEC	( 1000 / IRC_TIMEOUT_MS)



/* Ctype-like macros */
#define IS_UPPER(c) ( (c) >= 'A' && (c) <= 'Z' )
#define IS_LOWER(c) ( (c) >= 'a' && (c) <= 'z' )
#define IS_DIGIT(c) ( (c) >= '0' && (c) <= '9' )
#define IS_CNTRL(c) ( (c) >= 0 && (c) <= 31 )
#define IS_ALPHA(c) ( IS_UPPER(c) || IS_LOWER(c) )
#define IS_ALNUM(c) ( IS_ALPHA(c) || IS_DIGIT(c) )



/* IRC command status; used to determine if connection should be re-attempted or not */
#define IRC_CMD_SUCCESS		0	// Success
#define IRC_CMD_FATAL		1	// Fatal error, don't bother retrying
#define IRC_CMD_RETRY		2	// Recoverable error, command should be attempted again


/* Constants that indicate the state of the IRC thread. */
#define IRC_THREAD_DEAD		0	// Thread is dead or hasn't been started
#define IRC_THREAD_INITIALISING	1	// Thread is being initialised
#define IRC_THREAD_CONNECTING	2	// Thread is attempting to connect
#define IRC_THREAD_SETNICK	3	// Thread is trying to set the player's
					// nick
#define IRC_THREAD_CONNECTED	4	// Thread established a connection to
					// the server and will attempt to join
					// the channel
#define IRC_THREAD_JOINED	5	// Channel joined, ready to send or
					// receive messages
#define IRC_THREAD_QUITTING	6	// The thread is being killed


/* Function that sets the thread status when the thread dies. Since that is
 * system-dependent, it can't be done in the thread's main code.
 */
static void IRC_SetThreadDead( );


/* Status of the IRC thread */
static int IRC_ThreadStatus = IRC_THREAD_DEAD;

/* Quit requested? */
static qboolean IRC_QuitRequested;

/* Socket handler */
static irc_socket_t IRC_Socket;               // Socket


/*
 * The protocol parser uses a finite state machine, here are the various
 * states' definitions as well as a variable containing the current state
 * and various other variables for message building.
 */
#define IRC_PARSER_RECOVERY		(-1)	// Error recovery
#define IRC_PARSER_START		0	// Start of a message
#define IRC_PARSER_PFX_NOS_START	1	// Prefix start
#define IRC_PARSER_PFX_NOS		2	// Prefix, server or nick name
#define IRC_PARSER_PFX_USER_START	3	// Prefix, start of user name
#define IRC_PARSER_PFX_USER		4	// Prefix, user name
#define IRC_PARSER_PFX_HOST_START	5	// Prefix, start of host name
#define IRC_PARSER_PFX_HOST		6	// Prefix, host name
#define IRC_PARSER_COMMAND_START	7	// Start of command after a prefix
#define IRC_PARSER_STR_COMMAND		8	// String command
#define IRC_PARSER_NUM_COMMAND_2	9	// Numeric command, second character
#define IRC_PARSER_NUM_COMMAND_3	10	// Numeric command, third character
#define IRC_PARSER_NUM_COMMAND_4	11	// Numeric command end
#define IRC_PARSER_PARAM_START		12	// Parameter start
#define IRC_PARSER_MID_PARAM		13	// "Middle" parameter
#define IRC_PARSER_TRAILING_PARAM	14	// Trailing parameter
#define IRC_PARSER_LF			15	// End of line

static int IRC_ParserState;
static qboolean IRC_ParserInMessage;
static qboolean IRC_ParserError;


/* 
 * According to RFC 1459, maximal message size is 512 bytes, including trailing
 * CRLF.
 */

#define IRC_MESSAGE_SIZE 512
#define IRC_SEND_BUF_SIZE IRC_MESSAGE_SIZE
#define IRC_RECV_BUF_SIZE (IRC_MESSAGE_SIZE * 2)


/*
 * IRC messages consist in:
 * 1) an optional prefix, which contains either a server name or a nickname,
 * 2) a command, which may be either a word or 3 numbers,
 * 3) any number of arguments.
 *
 * RFC 2812 says that there are at most 14 "middle" parameters and a trailing
 * parameter. However, UnrealIRCd does not respect this, and sends messages
 * that contain an extra parameter. While the message in question could be
 * ignored, it's better to avoid entering the error recovery state.
 *
 * Since we won't be handling messages in parallel, we will create a
 * static record and use that to store everything, as we can definitely
 * spare 130k of memory (note: we could have something smaller but it'd
 * probably be a pointless exercise).
 */


#define irc_string_t(len) struct { \
	unsigned int length; \
	char string[ len ]; \
}


#define IRC_MAX_NICK_LEN 64
#define IRC_MAX_ARG_LEN 509
#define IRC_MAX_PARAMS 16

struct irc_message_t {
	// Prefix
	irc_string_t(IRC_MAX_NICK_LEN) pfx_nickOrServer;
	irc_string_t(IRC_MAX_NICK_LEN) pfx_user;
	irc_string_t(IRC_MAX_NICK_LEN) pfx_host;

	// Command
	irc_string_t(32) cmd_string;

	// Arguments
	irc_string_t(IRC_MAX_ARG_LEN) arg_values[IRC_MAX_PARAMS];
	unsigned int arg_count;
};

static struct irc_message_t IRC_ReceivedMessage;


// Macros to access the message's various fields
#define IRC_String(N) (IRC_ReceivedMessage.N.string)
#define IRC_Length(N) (IRC_ReceivedMessage.N.length)


/*
 * IRC command handlers are called when some command is received;
 * they are stored in hash tables.
 */

typedef int (*irc_handler_func_t)( );
typedef int (*ctcp_handler_func_t)( qboolean is_channel , const char * message );

struct irc_handler_t {
	char cmd_string[33];
	void * handler;
};

static hashtable_t IRC_Handlers;
static hashtable_t IRC_CTCPHandlers;


/*
 * Username, nickname, etc...
 */

struct irc_user_t {
	char nick[16];
	int nicklen;
	int nickattempts;
	char username[16];
	char email[100];
};

static struct irc_user_t IRC_User;


/*
 * Events that can be displayed and flags that apply to them.
 */

#define IRC_EVT_SAY		0x00000000	// Standard message
#define IRC_EVT_ACT		0x00000001	// /me message
#define IRC_EVT_JOIN		0x00000002	// Join
#define IRC_EVT_PART		0x00000003	// Part
#define IRC_EVT_QUIT		0x00000004	// Quit
#define IRC_EVT_KICK		0x00000005	// Kick
#define IRC_EVT_NICK_CHANGE	0x00000006	// Nick change
#define IRC_EVTF_SELF		0x00000100	// Event applies to current user


#define IRC_EventType(evt) ( evt & 0xff )
#define IRC_EventIsSelf(evt) ( ( evt & IRC_EVTF_SELF ) == IRC_EVTF_SELF )
#define IRC_MakeEvent(type,isself) ( IRC_EVT_##type | ( (isself) ? IRC_EVTF_SELF : 0 ) )



/*
 * Rate limiters for various events.
 *
 * The rate limiter works on a per-event basis, it doesn't know nor care
 * about users.
 * Its threshold and increase constants (which will be scaled depending on
 * the timing controls) determine the amount of responses per second, while
 * also allowing "bursts".
 */

/* Rate limiter threshold - above that, no response */
#define IRC_LIMIT_THRESHOLD	3

/* Rate limiter increase per check */
#define IRC_LIMIT_INCREASE	1

#define IRC_RL_MESSAGE		0
#define IRC_RL_PING		1
#define IRC_RL_VERSION		2

static unsigned int IRC_RateLimiter[ 3 ];




/*--------------------------------------------------------------------------*/
/* FUNCTIONS THAT MANAGE IRC COMMAND HANDLERS                               */
/*--------------------------------------------------------------------------*/


/*
 * Initialises the handler tables
 */
static inline void IRC_InitHandlers( )
{
	IRC_Handlers = HT_Create( 100 , HT_FLAG_INTABLE | HT_FLAG_CASE ,
				  sizeof( struct irc_handler_t ) ,
				  HT_OffsetOfField( struct irc_handler_t , cmd_string ) ,
				  32 );
	IRC_CTCPHandlers = HT_Create( 100 , HT_FLAG_INTABLE | HT_FLAG_CASE ,
				      sizeof( struct irc_handler_t ) ,
				      HT_OffsetOfField( struct irc_handler_t , cmd_string ) ,
				      32 );
}


/*
 * Frees the list of handlers (used when the IRC thread dies).
 */
static void IRC_FreeHandlers( )
{
	HT_Destroy( IRC_Handlers );
	HT_Destroy( IRC_CTCPHandlers );
}


/*
 * Registers a new IRC command handler.
 */
static inline void IRC_AddHandler( const char * command , irc_handler_func_t handler )
{
	qboolean created;
	struct irc_handler_t * rv;
	rv = HT_GetItem( IRC_Handlers , command , &created );
	assert( created );
	rv->handler = handler;
}


/*
 * Registers a new CTCP command handler.
 */
static void IRC_AddCTCPHandler( const char * command , ctcp_handler_func_t handler )
{
	qboolean created;
	struct irc_handler_t * rv;
	rv = HT_GetItem( IRC_CTCPHandlers , command , &created );
	assert( created );
	rv->handler = handler;
}


/*
 * Executes the command handler for the currently stored command. If there is
 * no registered handler matching the command, ignore it.
 */
static int IRC_ExecuteHandler( )
{
	struct irc_handler_t * handler;
	handler = HT_GetItem( IRC_Handlers , IRC_String(cmd_string) , NULL );
	if ( handler == NULL )
		return IRC_CMD_SUCCESS;
	return ((irc_handler_func_t)(handler->handler))( );
}


/*
 * Executes a CTCP command handler.
 */
static int IRC_ExecuteCTCPHandler( const char * command , qboolean is_channel , const char *argument )
{
	struct irc_handler_t * handler;
	handler = HT_GetItem( IRC_CTCPHandlers , command , NULL );
	if ( handler == NULL )
		return IRC_CMD_SUCCESS;
	return ((ctcp_handler_func_t)(handler->handler))( is_channel , argument );
}



/*--------------------------------------------------------------------------*/
/* IRC DELAYED EXECUTION                                                    */
/*--------------------------------------------------------------------------*/

/* Structure for the delayed execution queue */
struct irc_delayed_t
{
	irc_handler_func_t	handler;	// Handler to call
	int			time_left;	// "Time" left before call
	struct irc_delayed_t *	next;		// Next record
};

/* Delayed execution queue head & tail */
static struct irc_delayed_t * IRC_DEQueue = NULL;


/*
 * This function sets an IRC handler function to be executed after some time.
 */
static void IRC_SetTimeout( irc_handler_func_t function , int time )
{
	struct irc_delayed_t * qe , * find;
	assert( time > 0 );

	// Create entry
	qe = (struct irc_delayed_t *) malloc( sizeof( struct irc_delayed_t ) );
	qe->handler = function;
	qe->time_left = time * IRC_TIMEOUTS_PER_SEC;

	// Find insert location
	if ( IRC_DEQueue ) {
		if ( IRC_DEQueue->time_left >= time ) {
			qe->next = IRC_DEQueue;
			IRC_DEQueue = qe;
		} else {
			find = IRC_DEQueue;
			while ( find->next && find->next->time_left < time )
				find = find->next;
			qe->next = find->next;
			find->next = qe;
		}
	} else {
		qe->next = NULL;
		IRC_DEQueue = qe;
	}
}


/*
 * This function dequeues an entry from the delayed execution queue.
 */
static qboolean IRC_DequeueDelayed( )
{
	struct irc_delayed_t * found;

	if ( ! IRC_DEQueue )
		return false;

	found = IRC_DEQueue;
	IRC_DEQueue = found->next;
	free( found );
	return true;
}


/*
 * This function deletes all remaining entries from the delayed execution
 * queue.
 */
static void IRC_FlushDEQueue( )
{
	while ( IRC_DequeueDelayed( ) ) {
		// PURPOSEDLY EMPTY
	}
}


/*
 * This function processes the delayed execution queue.
 */
static int IRC_ProcessDEQueue( )
{
	struct irc_delayed_t * iter;
	int err_code;

	iter = IRC_DEQueue;
	while ( iter ) {
		if ( iter->time_left == 1 ) {
			err_code = (iter->handler)( );
			IRC_DequeueDelayed( );
			if ( err_code != IRC_CMD_SUCCESS )
				return err_code;
			iter = IRC_DEQueue;
		} else {
			iter->time_left --;
			iter = iter->next;
		}
	}

	return IRC_CMD_SUCCESS;
}



/*--------------------------------------------------------------------------*/
/* IRC MESSAGE PARSER                                                       */
/*--------------------------------------------------------------------------*/


/* Parser macros, 'cause I'm lazy */
#define P_SET_STATE(S) IRC_ParserState = IRC_PARSER_##S
#define P_INIT_MESSAGE(S) { \
	P_SET_STATE(S); \
	IRC_ParserInMessage = true; \
	memset( &IRC_ReceivedMessage , 0 , sizeof( struct irc_message_t ) ); \
}
#if defined DEBUG_DUMP_IRC
#define P_ERROR(S) { \
	if ( ! IRC_ParserError ) { \
		Com_Printf( "IRC PARSER ERROR (state: %d , received: %d)\n" , IRC_ParserState , next ); \
	} \
	P_SET_STATE(S); \
	IRC_ParserError = true; \
}
#else // defined DEBUG_DUMP_IRC
#define P_ERROR(S) { \
	P_SET_STATE(S); \
	IRC_ParserError = true; \
}
#endif // defined DEBUG_DUMP_IRC
#define P_AUTO_ERROR { \
	if ( next == '\r' ) { \
		P_ERROR(LF); \
	} else { \
		P_ERROR(RECOVERY); \
	} \
}
#define P_INIT_STRING(S) { \
	IRC_ReceivedMessage.S.string[0] = next; \
	IRC_ReceivedMessage.S.length = 1; \
}
#define P_ADD_STRING(S) { \
	if ( IRC_ReceivedMessage.S.length == sizeof( IRC_ReceivedMessage.S.string ) - 1 ) { \
		P_ERROR(RECOVERY); \
	} else { \
		IRC_ReceivedMessage.S.string[IRC_ReceivedMessage.S.length ++] = next; \
	} \
}
#define P_NEXT_PARAM { \
	if ( ( ++ IRC_ReceivedMessage.arg_count ) == IRC_MAX_PARAMS ) { \
		P_ERROR(RECOVERY); \
	} \
}
#define P_START_PARAM { \
	if ( ( ++ IRC_ReceivedMessage.arg_count ) == IRC_MAX_PARAMS ) { \
		P_ERROR(RECOVERY); \
	} else \
		P_INIT_STRING(arg_values[IRC_ReceivedMessage.arg_count - 1]) \
}
#define P_ADD_PARAM P_ADD_STRING(arg_values[IRC_ReceivedMessage.arg_count - 1])



/*
 * Main parsing function that uses a FSM to parse one character at a time.
 * Returns true when a full message is read and no error has occured.
 */

static qboolean IRC_Parser( char next )
{
	qboolean has_msg = false;

	switch ( IRC_ParserState ) {

		/* Initial state; clear the message, then check input. ':'
		 * indicates there is a prefix, a digit indicates a numeric
		 * command, an upper-case letter indicates a string command.
		 * It's also possible we received an empty line - just skip
		 * it. Anything else is an error.
		 */
		case IRC_PARSER_START:
			IRC_ParserError = false;
			IRC_ParserInMessage = false;
			if ( next == ':' ) {
				P_INIT_MESSAGE(PFX_NOS_START);
			} else if ( next == '\r' ) {
				P_SET_STATE(LF);
			} else if ( IS_DIGIT( next ) ) {
				P_INIT_MESSAGE(NUM_COMMAND_2);
				P_INIT_STRING(cmd_string);
			} else if ( IS_UPPER( next ) ) {
				P_INIT_MESSAGE(STR_COMMAND);
				P_INIT_STRING(cmd_string);
			} else {
				P_ERROR(RECOVERY);
			}
			break;

		/*
		 * Start of prefix; anything is accepted, except for '!', '@', ' '
		 * and control characters which all cause an error recovery.
		 */
		case IRC_PARSER_PFX_NOS_START:
			if ( next == '!' || next == '@' || next == ' ' || IS_CNTRL( next ) ) {
				P_AUTO_ERROR;
			} else {
				P_SET_STATE(PFX_NOS);
				P_INIT_STRING(pfx_nickOrServer);
			}
			break;

		/*
		 * Prefix, server or nick name. Control characters cause an error,
		 * ' ', '!' and '@' cause state changes.
		 */
		case IRC_PARSER_PFX_NOS:
			if ( next == '!' ) {
				P_SET_STATE(PFX_USER_START);
			} else if ( next == '@' ) {
				P_SET_STATE(PFX_HOST_START);
			} else if ( next == ' ' ) {
				P_SET_STATE(COMMAND_START);
			} else if IS_CNTRL( next ) {
				P_AUTO_ERROR;
			} else {
				P_ADD_STRING(pfx_nickOrServer);
			}
			break;

		/*
		 * Start of user name; anything goes, except for '!', '@', ' '
		 * and control characters which cause an error.
		 */
		case IRC_PARSER_PFX_USER_START:
			if ( next == '!' || next == '@' || next == ' ' || IS_CNTRL( next ) ) {
				P_AUTO_ERROR;
			} else {
				P_SET_STATE(PFX_USER);
				P_INIT_STRING(pfx_user);
			}
			break;

		/*
		 * User name; '@' will cause state changes, '!' , ' ' and
		 * control characters will cause errors.
		 */
		case IRC_PARSER_PFX_USER:
			if ( next == '@' ) {
				P_SET_STATE(PFX_HOST_START);
			} else if ( next == '!' || next == ' ' || IS_CNTRL( next ) ) {
				P_AUTO_ERROR;
			} else {
				P_ADD_STRING(pfx_user);
			}
			break;

		/*
		 * Start of host name; anything goes, except for '!', '@', ' '
		 * and control characters which cause an error.
		 */
		case IRC_PARSER_PFX_HOST_START:
			if ( next == '!' || next == '@' || next == ' ' || IS_CNTRL( next ) ) {
				P_AUTO_ERROR;
			} else {
				P_SET_STATE(PFX_HOST);
				P_INIT_STRING(pfx_host);
			}
			break;

		/*
		 * Host name; ' ' will cause state changes, '!' and control
		 * characters will cause errors.
		 */
		case IRC_PARSER_PFX_HOST:
			if ( next == ' ' ) {
				P_SET_STATE(COMMAND_START);
			} else if ( next == '!' || next == '@' || IS_CNTRL( next ) ) {
				P_AUTO_ERROR;
			} else {
				P_ADD_STRING(pfx_host);
			}
			break;

		/*
		 * Start of command, will accept start of numeric and string
		 * commands; anything else is an error.
		 */
		case IRC_PARSER_COMMAND_START:
			if ( IS_DIGIT( next ) ) {
				P_SET_STATE(NUM_COMMAND_2);
				P_INIT_STRING(cmd_string);
			} else if ( IS_UPPER( next ) ) {
				P_SET_STATE(STR_COMMAND);
				P_INIT_STRING(cmd_string);
			} else {
				P_AUTO_ERROR;
			}
			break;

		/*
		 * String command. Uppercase letters will cause the parser
		 * to continue on string commands, ' ' indicates a parameter
		 * is expected, '\r' means we're done. Anything else is an
		 * error.
		 */
		case IRC_PARSER_STR_COMMAND:
			if ( next == ' ' ) {
				P_SET_STATE(PARAM_START);
			} else if ( next == '\r' ) {
				P_SET_STATE(LF);
			} else if ( IS_UPPER( next ) ) {
				P_ADD_STRING(cmd_string);
			} else {
				P_ERROR(RECOVERY);
			}
			break;

		/*
		 * Second/third digit of numeric command; anything but a digit
		 * is an error.
		 */
		case IRC_PARSER_NUM_COMMAND_2:
		case IRC_PARSER_NUM_COMMAND_3:
			if ( IS_DIGIT( next ) ) {
				IRC_ParserState ++;
				P_ADD_STRING(cmd_string);
			} else {
				P_AUTO_ERROR;
			}
			break;

		/*
		 * End of numeric command, could be a ' ' or a '\r'.
		 */
		case IRC_PARSER_NUM_COMMAND_4:
			if ( next == ' ' ) {
				P_SET_STATE(PARAM_START);
			} else if ( next == '\r' ) {
				P_SET_STATE(LF);
			} else {
				P_ERROR(RECOVERY);
			}
			break;

		/*
		 * Start of parameter. ':' means it's a trailing parameter,
		 * spaces and control characters shouldn't be here, and
		 * anything else is a "middle" parameter.
		 */
		case IRC_PARSER_PARAM_START:
			if ( next == ':' ) {
				P_SET_STATE(TRAILING_PARAM);
				P_NEXT_PARAM;
			} else if ( next == '\r' ) {
				P_SET_STATE(LF);
			} else if ( IS_CNTRL( next ) ) {
				P_AUTO_ERROR;
			} else if ( next != ' ' ) {
				if ( next & 0x80 )
					next = '?';
				P_SET_STATE(MID_PARAM);
				P_START_PARAM;
			}
			break;

		/*
		 * "Middle" parameter; ' ' means there's another parameter coming,
		 * '\r' means the end of the message, control characters are not
		 * accepted, anything else is part of the parameter.
		 */
		case IRC_PARSER_MID_PARAM:
			if ( next == ' ' ) {
				P_SET_STATE(PARAM_START);
			} else if ( next == '\r' ) {
				P_SET_STATE(LF);
			} else if ( IS_CNTRL( next ) ) {
				P_ERROR(RECOVERY);
			} else {
				if ( next & 0x80 )
					next = '?';
				P_ADD_PARAM;
			}
			break;

		/*
		 * Trailing parameter; '\r' means the end of the command,
		 * and anything else is just added to the string.
		 */
		case IRC_PARSER_TRAILING_PARAM:
			if ( next == '\r' ) {
				P_SET_STATE(LF);
			} else {
				if ( next & 0x80 ) {
					next = '?';
				}
				P_ADD_PARAM;
			}
			break;

		/*
		 * End of line, expect '\n'. If found, we may have a message
		 * to handle (unless there were errors). Anything else is an
		 * error.
		 */
		case IRC_PARSER_LF:
			if ( next == '\n' ) {
				has_msg = IRC_ParserInMessage;
				P_SET_STATE(START);
			} else {
				P_AUTO_ERROR;
			}
			break;

		/*
		 * Error recovery: wait for an '\r'.
		 */
		case IRC_PARSER_RECOVERY:
			if ( next == '\r' )
				P_SET_STATE(LF);
			break;
	}

	return has_msg && !IRC_ParserError;
}


/*
 * Debugging function that dumps the IRC message.
 */
#ifdef DEBUG_DUMP_IRC
static void IRC_DumpMessage( )
{
	int i;

	Com_Printf( "----------- IRC MESSAGE RECEIVED -----------\n" );
	Com_Printf( " (pfx) nick/server .... [%.3d]%s\n" , IRC_Length( pfx_nickOrServer ) , IRC_String( pfx_nickOrServer ) );
	Com_Printf( " (pfx) user ........... [%.3d]%s\n" , IRC_Length( pfx_user ) , IRC_String( pfx_user ) );
	Com_Printf( " (pfx) host ........... [%.3d]%s\n" , IRC_Length( pfx_host ) , IRC_String( pfx_host ) );
	Com_Printf( " command string ....... [%.3d]%s\n" , IRC_Length( cmd_string ) , IRC_String( cmd_string ) );
	Com_Printf( " arguments ............  %.3d\n" , IRC_ReceivedMessage.arg_count );
	for ( i = 0 ; i < IRC_ReceivedMessage.arg_count ; i ++ ) {
		Com_Printf( " ARG %d = [%.3d]%s\n" , i + 1 , IRC_Length( arg_values[ i ] ) , IRC_String( arg_values[ i ] ) );
	}
}
#endif // DEBUG_DUMP_IRC



/*--------------------------------------------------------------------------*/
/* "SYSTEM" FUNCTIONS                                                       */
/*--------------------------------------------------------------------------*/


#if defined WIN32_VARIANT
static void IRC_HandleError(void)
{
	switch ( WSAGetLastError() )
	{
		case 0: // No error
			return;

		case WSANOTINITIALISED :
			Com_Printf("Unable to initialise socket.\n");
		break;
		case WSAEAFNOSUPPORT :
			Com_Printf("The specified address family is not supported.\n");
		break;
		case WSAEADDRNOTAVAIL :
			Com_Printf("Specified address is not available from the local machine.\n");
		break;
		case WSAECONNREFUSED :
			Com_Printf("The attempt to connect was forcefully rejected.\n");
		break;
		case WSAEDESTADDRREQ :
			Com_Printf("address destination address is required.\n");
		break;
		case WSAEFAULT :
			Com_Printf("The namelen argument is incorrect.\n");
		break;
		case WSAEINVAL :
			Com_Printf("The socket is not already bound to an address.\n");
		break;
		case WSAEISCONN :
			Com_Printf("The socket is already connected.\n");
		break;
		case WSAEADDRINUSE :
			Com_Printf("The specified address is already in use.\n");
		break;
		case WSAEMFILE :
			Com_Printf("No more file descriptors are available.\n");
		break;
		case WSAENOBUFS :
			Com_Printf("No buffer space available. The socket cannot be created.\n");
		break;
		case WSAEPROTONOSUPPORT :
			Com_Printf("The specified protocol is not supported.\n");
		break;
		case WSAEPROTOTYPE :
			Com_Printf("The specified protocol is the wrong type for this socket.\n");
		break;
		case WSAENETUNREACH :
			Com_Printf("The network can't be reached from this host at this time.\n");
		break;
		case WSAENOTSOCK :
		 	Com_Printf("The descriptor is not a socket.\n");
		break;
		case WSAETIMEDOUT :
			Com_Printf("Attempt timed out without establishing a connection.\n");
		break;
		case WSAESOCKTNOSUPPORT :
		 	Com_Printf("Socket type is not supported in this address family.\n");
		break;
		case WSAENETDOWN :
			Com_Printf("Network subsystem failure.\n");
		break;
		case WSAHOST_NOT_FOUND :
			Com_Printf("Authoritative Answer Host not found.\n");
		break;
		case WSATRY_AGAIN :
			Com_Printf("Non-Authoritative Host not found or SERVERFAIL.\n");
		break;
		case WSANO_RECOVERY :
		 	Com_Printf("Non recoverable errors, FORMERR, REFUSED, NOTIMP.\n");
		break;
		case WSANO_DATA :
			Com_Printf("Valid name, no data record of requested type.\n");
		break;
		case WSAEINPROGRESS :
			Com_Printf("address blocking Windows Sockets operation is in progress.\n");
		break;
		default :
			Com_Printf("Unknown connection error.\n");
		break;
	}

	WSASetLastError( 0 );
}
#elif defined UNIX_VARIANT
static void IRC_HandleError( void )
{
	Com_Printf( "IRC socket connection error: %s\n" , strerror( errno ) );
}
#endif


#if defined MSG_NOSIGNAL
# define IRC_SEND_FLAGS MSG_NOSIGNAL
#else
# define IRC_SEND_FLAGS 0
#endif

/*
 * Attempt to format then send a message to the IRC server. Will return
 * true on success, and false if an overflow occurred or if send() failed.
 */
static int IRC_Send( const char * format , ... )
{
	char buffer[ IRC_SEND_BUF_SIZE + 1 ];
	va_list args;
	int len , sent;

	// Format message
	va_start( args , format );
	len = vsnprintf( buffer , IRC_SEND_BUF_SIZE - 1 , format , args );
	va_end( args );
	if ( len >= IRC_SEND_BUF_SIZE - 1 ) {
		// This is a bug, return w/ a fatal error
		Com_Printf( "...IRC: send buffer overflow (%d characters)\n" , len );
		return IRC_CMD_FATAL;
	}

	// Add CRLF terminator
#if defined DEBUG_DUMP_IRC
	Com_Printf( "SENDING IRC MESSAGE: %s\n" , buffer );
#endif
	buffer[ len++ ] = '\r';
	buffer[ len++ ] = '\n';

	// Send message
	sent = send(IRC_Socket, buffer , len , IRC_SEND_FLAGS );
	if ( sent < len ) {
		IRC_HandleError( );
		return IRC_CMD_RETRY;
	}

	return IRC_CMD_SUCCESS;
}


/*
 * This function is used to prevent the IRC thread from turning the CPU into
 * a piece of molten silicium while it waits for the server to send data.
 *
 * If data is received, SUCCESS is returned; otherwise, RETRY will be returned
 * on timeout and FATAL on error.
 */

#if defined WIN32_VARIANT
# define SELECT_ARG 0
# define SELECT_CHECK ( rv == -1 && WSAGetLastError() == WSAEINTR )
#elif defined UNIX_VARIANT
# define SELECT_ARG ( IRC_Socket + 1 )
# define SELECT_CHECK ( rv == -1 && errno == EINTR )
#endif

static int IRC_Wait( )
{
	struct timeval timeout;
	fd_set read_set;
	int rv;

	// Wait for data to be available
	do {
		FD_ZERO( &read_set );
		FD_SET( IRC_Socket, &read_set );
		timeout.tv_sec = 0;
		timeout.tv_usec = IRC_TIMEOUT_US;
		rv = select( SELECT_ARG , &read_set , NULL , NULL , &timeout );
	} while ( SELECT_CHECK );

	// Something wrong happened
	if ( rv < 0 ) {
		IRC_HandleError( );
		return IRC_CMD_FATAL;
	}

	return ( rv == 0 ) ? IRC_CMD_RETRY : IRC_CMD_SUCCESS;
}


/*
 * Wait for some seconds.
 */

static void IRC_Sleep( int seconds )
{
	int i;
	assert( seconds > 0 );
	for ( i = 0 ; i < seconds * IRC_TIMEOUTS_PER_SEC && !IRC_QuitRequested ; i ++ ) {
#if defined WIN32_VARIANT
		Sleep( IRC_TIMEOUT_MS );
#elif defined UNIX_VARIANT
		usleep( IRC_TIMEOUT_US );
#endif
	}
}



/*--------------------------------------------------------------------------*/
/* RATE LIMITS                                                              */
/*--------------------------------------------------------------------------*/


/*
 * Checks if some action can be effected using the rate limiter. If it can,
 * the rate limiter's status will be updated.
 */
static inline qboolean IRC_CheckEventRate( int event_type )
{
	if ( IRC_RateLimiter[ event_type ] >= IRC_LIMIT_THRESHOLD * IRC_TIMEOUTS_PER_SEC )
		return false;
	IRC_RateLimiter[ event_type ] += IRC_LIMIT_INCREASE * IRC_TIMEOUTS_PER_SEC;
	return true;
}


/*
 * Decrease all non-zero rate limiter entries.
 */
static inline void IRC_UpdateRateLimiter( )
{
	int i;
	for ( i = 0 ; i < sizeof( IRC_RateLimiter ) / sizeof( unsigned int ) ; i ++ )
		if ( IRC_RateLimiter[ i ] ) {
			IRC_RateLimiter[ i ] --;
		}
}


/*
 * Initialise the rate limiter.
 */
static inline void IRC_InitRateLimiter( )
{
	int i;
	for ( i = 0 ; i < sizeof( IRC_RateLimiter ) / sizeof( unsigned int ) ; i ++ )
		IRC_RateLimiter[ i ] = 0;
}



/*--------------------------------------------------------------------------*/
/* DISPLAY CODE                                                             */
/*--------------------------------------------------------------------------*/


static void IRC_NeutraliseString( char * buffer , const char * source )
{
	while ( *source ) {
		char c = *source;
		if ( IS_CNTRL( c ) ) {
			*( buffer ++ ) = ' ';
		} else if ( c & 0x80 ) {
			*( buffer ++ ) = '?';
		} else if ( c == Q_COLOR_ESCAPE ) {
			*( buffer ++ ) = Q_COLOR_ESCAPE;
			*( buffer ++ ) = Q_COLOR_ESCAPE;
		} else {
			*( buffer ++ ) = c;
		}
		source ++;
	}
	*buffer = 0;
}


static void IRC_Display( int event , const char * nick , const char *message )
{
	char buffer[ IRC_RECV_BUF_SIZE * 2 ];
	char nick_copy[ IRC_MAX_NICK_LEN * 2 ];
	char message_copy[ IRC_MAX_ARG_LEN * 2 ];
	const char *fmt_string;
	qboolean has_nick;
	qboolean has_message;

	// If we're quitting, just skip this
	if ( IRC_QuitRequested )
		return;

	// Determine message format
	switch ( IRC_EventType( event ) ) {
		case IRC_EVT_SAY:
			has_nick = has_message = true;
			if ( IRC_EventIsSelf( event ) ) {
				fmt_string = "^2<^7%s^2> %s";
			} else if ( strstr( message , IRC_User.nick ) ) {
				fmt_string = "^3<^7%s^3> %s";
			} else {
				fmt_string = "^1<^7%s^1> %s";
			}
			break;
		case IRC_EVT_ACT:
			has_nick = has_message = true;
			if ( IRC_EventIsSelf( event ) ) {
				fmt_string = "^2* ^7%s^2 %s";
			} else if ( strstr( message , IRC_User.nick ) ) {
				fmt_string = "^3* ^7%s^3 %s";
			} else {
				fmt_string = "^1* ^7%s^1 %s";
			}
			break;
		case IRC_EVT_JOIN:
			has_message = false;
			has_nick = !IRC_EventIsSelf( event );
			if ( has_nick ) {
				fmt_string = "^5-> ^7%s^5 has entered the channel.";
			} else {
				fmt_string = "^2Joined IRC chat.";
			}
			break;
		case IRC_EVT_PART:
			// The AlienArena IRC client never parts, so it's
			// someone else.
			has_nick = true;
			has_message = ( message[0] != 0 );
			if ( has_message ) {
				fmt_string = "^5<- ^7%s^5 has left the channel: %s.";
			} else {
				fmt_string = "^5<- ^7%s^5 has left the channel.";
			}
			break;
		case IRC_EVT_QUIT:
			has_nick = !IRC_EventIsSelf( event );
			if ( has_nick ) {
				has_message = ( message[0] != 0 );
				if ( has_message ) {
					fmt_string = "^5<- ^7%s^5 has quit: %s.";
				} else {
					fmt_string = "^5<- ^7%s^5 has quit.";
				}
			} else {
				has_message = true;
				fmt_string = "^2Quit IRC chat: %s.";
			}
			break;
		case IRC_EVT_KICK:
			has_nick = has_message = true;
			if ( IRC_EventIsSelf( event ) ) {
				fmt_string = "^2Kicked by ^7%s^2: %s.";
			} else {
				fmt_string = "^5<- ^7%s^5 has been kicked: %s.";
			}
			break;
		case IRC_EVT_NICK_CHANGE:
			has_nick = has_message = true;
			if ( IRC_EventIsSelf( event ) ) {
				fmt_string = "^2** ^7%s^2 is now known as ^7%s^2.";
			} else {
				fmt_string = "^5** ^7%s^5 is now known as ^7%s^5.";
			}
			break;
		default:
			has_nick = has_message = false;
			fmt_string = "unknown message received";
			break;
	}

	// Neutralise required strings
	if ( has_nick )
		IRC_NeutraliseString( nick_copy , nick );
	if ( has_message )
		IRC_NeutraliseString( message_copy , message );

	// Format message
	if ( has_nick && has_message ) {
		sprintf( buffer , fmt_string , nick_copy , message_copy );
	} else if ( has_nick ) {
		sprintf( buffer , fmt_string , nick_copy );
	} else if ( has_message ) {
		sprintf( buffer , fmt_string , message_copy );
	} else {
		strncpy( buffer , fmt_string , IRC_RECV_BUF_SIZE * 2 - 1 );
	}
	buffer[ IRC_RECV_BUF_SIZE * 2 - 1 ] = 0;

	SCR_IRCPrintf( "^1IRC: %s", buffer );
}



/*--------------------------------------------------------------------------*/
/* IRC MESSAGE HANDLERS                                                     */
/*--------------------------------------------------------------------------*/


/*
 * Send the user's nickname.
 */
static int IRC_SendNickname( )
{
	return IRC_Send( "NICK %s" , IRC_User.nick );
}


/*
 * Join the channel
 */
static int IRC_JoinChannel( )
{
	return IRC_Send( "JOIN #%s" , cl_IRC_channel->string );
}


/*
 * Handles a PING by replying with a PONG.
 */
static int IRCH_Ping( )
{
	if ( IRC_ReceivedMessage.arg_count == 1 )
		return IRC_Send( "PONG :%s" , IRC_String( arg_values[ 0 ] ) );
	return IRC_CMD_SUCCESS;
}


/*
 * Handles server errors
 */
static int IRCH_ServerError( )
{
	if ( IRC_ThreadStatus == IRC_THREAD_QUITTING ) {
		return IRC_CMD_SUCCESS;
	}

	if ( IRC_ReceivedMessage.arg_count == 1 ) {
		Com_Printf( "IRC: server error - %s\n" , IRC_String( arg_values[ 0 ] ) );
	} else {
		Com_Printf( "IRC: server error\n" );
	}
	return IRC_CMD_RETRY;
}


/*
 * Some fatal error was received, the IRC thread must die.
 */
static int IRCH_FatalError( )
{
	IRC_Display( IRC_MakeEvent(QUIT,1) , "" , "fatal error" );
	IRC_Send( "QUIT :Something went wrong" );
	return IRC_CMD_RETRY;
}


/*
 * Nickname error. If received while the thread is in the SETNICK state,
 * we might want to try again. Otherwise, we ignore the error as it should
 * not have been received anyway.
 */
#define RANDOM_NUMBER_CHAR ( '0' + rand() % 10 )
static int IRCH_NickError( )
{
	int i;

	if ( IRC_ThreadStatus == IRC_THREAD_SETNICK ) {
		if ( ++ IRC_User.nickattempts == 4 ) {
			IRC_Send( "QUIT :Could not set nickname" );
			return IRC_CMD_FATAL;
		}

		if ( IRC_User.nicklen < 15 ) {
			IRC_User.nick[ IRC_User.nicklen ++ ] = RANDOM_NUMBER_CHAR;
		} else {
			for ( i = IRC_User.nicklen - 3 ; i < IRC_User.nicklen ; i ++ ) {
				IRC_User.nick[ i ] = RANDOM_NUMBER_CHAR;
			}
		}

		IRC_SetTimeout( IRC_SendNickname , 2 );
	} else {
		Com_Printf( "...IRC: got spurious nickname error\n" );
	}

	return IRC_CMD_SUCCESS;
}


/*
 * Connection established, we will be able to join a channel
 */
static int IRCH_Connected( )
{
	if ( IRC_ThreadStatus != IRC_THREAD_SETNICK ) {
		IRC_Display( IRC_MakeEvent(QUIT,1) , "" , "IRC client bug" );
		IRC_Send( "QUIT :AlienArena IRC bug!" );
		return IRC_CMD_RETRY;
	}
	IRC_ThreadStatus = IRC_THREAD_CONNECTED;
	IRC_SetTimeout( &IRC_JoinChannel , 1 );
	return IRC_CMD_SUCCESS;
}


/*
 * Received JOIN
 */
static int IRCH_Joined( )
{
	int event;

	if ( IRC_ThreadStatus < IRC_THREAD_CONNECTED ) {
		IRC_Display( IRC_MakeEvent(QUIT,1) , "" , "IRC client bug" );
		IRC_Send( "QUIT :AlienArena IRC bug!" );
		return IRC_CMD_RETRY;
	}

	if ( !strcmp( IRC_String( pfx_nickOrServer ) , IRC_User.nick ) ) {
		IRC_ThreadStatus = IRC_THREAD_JOINED;
		event = IRC_MakeEvent(JOIN,1);
	} else {
		event = IRC_MakeEvent(JOIN,0);
	}
	IRC_Display( event , IRC_String( pfx_nickOrServer ) , NULL );
	return IRC_CMD_SUCCESS;
}


/*
 * Received PART
 */
static int IRCH_Part( )
{
	IRC_Display( IRC_MakeEvent(PART, 0) , IRC_String( pfx_nickOrServer ) , IRC_String( arg_values[ 1 ] ) );
	return IRC_CMD_SUCCESS;
}


/*
 * Received QUIT
 */
static int IRCH_Quit( )
{
	IRC_Display( IRC_MakeEvent(QUIT, 0) , IRC_String( pfx_nickOrServer ) , IRC_String( arg_values[ 0 ] ) );
	return IRC_CMD_SUCCESS;
}


/*
 * Received KICK
 */
static int IRCH_Kick( )
{
	if ( !strcmp( IRC_String( arg_values[ 1 ] ) , IRC_User.nick ) ) {
		IRC_Display( IRC_MakeEvent(KICK, 1) , IRC_String( pfx_nickOrServer ) , IRC_String( arg_values[ 2 ] ) );
		if ( cl_IRC_kick_rejoin->integer > 0 ) {
			IRC_ThreadStatus = IRC_THREAD_CONNECTED;
			IRC_SetTimeout( &IRC_JoinChannel , cl_IRC_kick_rejoin->integer );
		} else {
			IRC_Display( IRC_MakeEvent(QUIT, 1) , "" , "kicked from channel.." );
			IRC_Send( "QUIT :b&!" );
			return IRC_CMD_FATAL;
		}
	} else {
		IRC_Display( IRC_MakeEvent(KICK, 0) , IRC_String( arg_values[ 1 ] ) , IRC_String( arg_values[ 2 ] ) );
	}
	return IRC_CMD_SUCCESS;
}


/*
 * Received NICK
 *
 * While the AA client does not support changing the current nickname,
 * it is still possible to receive a NICK applying to the connected user
 * because of e.g. OperServ's SVSNICK command.
 */
static int IRCH_Nick( )
{
	int event;

	if ( IRC_ReceivedMessage.arg_count != 1 )
		return IRC_CMD_SUCCESS;

	if ( !strcmp( IRC_String( pfx_nickOrServer ) , IRC_User.nick ) ) {
		strncpy( IRC_User.nick , IRC_String( arg_values[ 0 ] ) , 15 );
		Com_Printf( "%s\n", IRC_User.nick );
		event = IRC_MakeEvent(NICK_CHANGE, 1);
	} else {
		event = IRC_MakeEvent(NICK_CHANGE, 0);
	}
	IRC_Display( event , IRC_String( pfx_nickOrServer ) , IRC_String( arg_values[ 0 ] ) );
	return IRC_CMD_SUCCESS;
}


/*
 * Handles an actual message.
 */
static int IRC_HandleMessage( qboolean is_channel , const char * string )
{
	if ( is_channel ) {
		IRC_Display( IRC_MakeEvent(SAY, 0) , IRC_String( pfx_nickOrServer ) , string );
		return IRC_CMD_SUCCESS;
	}

	if ( IRC_CheckEventRate( IRC_RL_MESSAGE ) )
		return IRC_Send( "PRIVMSG %s :Sorry, AlienArena's IRC client does not support private messages" , IRC_String( pfx_nickOrServer ) );
	return IRC_CMD_SUCCESS;
}


/*
 * Splits a CTCP message into action and argument, then call
 * its handler (if there is one).
 */
static int IRC_HandleCTCP( qboolean is_channel , char * string , int string_len )
{
	char * end_of_action;

	end_of_action = strchr( string , ' ' );
	if ( end_of_action == NULL ) {
		end_of_action = string + string_len - 1;
		*end_of_action = 0;
	} else {
		*( string + string_len - 1 ) = 0;
		*end_of_action = 0;
		end_of_action ++;
	}

#if defined DEBUG_DUMP_IRC
	Com_Printf( "--- IRC/CTCP ---\n" );
	Com_Printf( " Command:     %s\n Argument(s): %s\n" , string , end_of_action );
#endif

	return IRC_ExecuteCTCPHandler( string , is_channel , end_of_action );
}



/*
 * Received PRIVMSG.
 *
 * This is either an actual message (to the channel or to the user) or a
 * CTCP command (action, version, etc...)
 */
static int IRCH_PrivMsg( )
{
	qboolean is_channel;

	if ( IRC_ReceivedMessage.arg_count != 2 ) {
		return IRC_CMD_SUCCESS;
	}

	// Check message to channel (bail out if it isn't our channel)
	is_channel = IRC_String( arg_values[ 0 ] )[ 0 ] == '#';
	if ( is_channel && strcmp( &( IRC_String( arg_values[ 0 ] )[ 1 ] ) , cl_IRC_channel->string ) )
		return IRC_CMD_SUCCESS;

	if ( IRC_Length( arg_values[ 1 ] ) > 2
			&& IRC_String( arg_values[ 1 ] )[ 0 ] == 1
			&& IRC_String( arg_values[ 1 ] )[ IRC_Length( arg_values[ 1 ] ) - 1 ] == 1 ) {
		return IRC_HandleCTCP( is_channel , IRC_String( arg_values[ 1 ] ) + 1 , IRC_Length( arg_values[ 1 ] ) - 1 );
	}

	return IRC_HandleMessage( is_channel , IRC_String( arg_values[ 1 ] ) );
}


/*
 * User is banned. Leave and do not come back.
 */
static int IRCH_Banned( )
{
	IRC_Display( IRC_MakeEvent(QUIT, 1) , "" , "banned from channel.." );
	IRC_Send( "QUIT :b&!" );
	return IRC_CMD_FATAL;
}



/*--------------------------------------------------------------------------*/
/* CTCP COMMAND HANDLERS                                                    */
/*--------------------------------------------------------------------------*/

/*
 * Action command aka "/me"
 */
static int CTCP_Action( qboolean is_channel , const char * argument )
{
	if ( !*argument )
		return IRC_CMD_SUCCESS;

	if ( is_channel ) {
		IRC_Display( IRC_MakeEvent(ACT, 0) , IRC_String( pfx_nickOrServer ) , argument );
		return IRC_CMD_SUCCESS;
	}

	if ( IRC_CheckEventRate( IRC_RL_MESSAGE ) )
		return IRC_Send( "PRIVMSG %s :Sorry, AlienArena's IRC client does not support private messages" , IRC_String( pfx_nickOrServer ) );
	return IRC_CMD_SUCCESS;
}


/*
 * PING requests
 */
static int CTCP_Ping( qboolean is_channel , const char * argument )
{
	if ( is_channel || !IRC_CheckEventRate( IRC_RL_PING ) )
		return IRC_CMD_SUCCESS;

	if ( *argument )
		return IRC_Send( "NOTICE %s :\001PING %s\001" , IRC_String( pfx_nickOrServer ) , argument );

	return IRC_Send( "NOTICE %s :\001PING\001" , IRC_String( pfx_nickOrServer ) );
}


/*
 * VERSION requests, let's advertise AA a lil'.
 */
static int CTCP_Version( qboolean is_channel , const char * argument )
{
	if ( is_channel || !IRC_CheckEventRate( IRC_RL_VERSION ) )
		return IRC_CMD_SUCCESS;

	return IRC_Send( "NOTICE %s :\001VERSION AlienArena IRC client - v" VERSION "\001" , IRC_String( pfx_nickOrServer ) );
}



/*--------------------------------------------------------------------------*/
/* MESSAGE SENDING                                                          */
/*--------------------------------------------------------------------------*/

/* Maximal message length */
#define IRC_MAX_SEND_LEN	400

/*
 * The message sending queue is used to avoid having to send stuff from the
 * game's main thread, as it could block or cause mix-ups in the printing
 * function.
 */

struct irc_sendqueue_t
{
	qboolean has_content;
	qboolean is_action;
	char string[IRC_MAX_SEND_LEN];
};

/* Length of the IRC send queue */
#define IRC_SENDQUEUE_SIZE	16

/* Index of the next message to process */
static int IRC_SendQueue_Process = 0;
/* Index of the next message to write */
static int IRC_SendQueue_Write = 0;

/* The queue */
static struct irc_sendqueue_t IRC_SendQueue[ IRC_SENDQUEUE_SIZE ];


/*
 * Initialise the send queue.
 */
static inline void IRC_InitSendQueue( )
{
	memset( &IRC_SendQueue , 0 , sizeof( IRC_SendQueue ) );
}


/*
 * Writes an entry to the send queue.
 */
static qboolean IRC_AddSendItem( qboolean is_action , const char * string )
{
	if ( IRC_SendQueue[ IRC_SendQueue_Write ].has_content )
		return false;

	strcpy( IRC_SendQueue[ IRC_SendQueue_Write ].string , string );
	IRC_SendQueue[ IRC_SendQueue_Write ].is_action = is_action;
	IRC_SendQueue[ IRC_SendQueue_Write ].has_content = true;
	IRC_SendQueue_Write = ( IRC_SendQueue_Write + 1 ) % IRC_SENDQUEUE_SIZE;
	return true;
}


/*
 * Sends an IRC message (console command).
 */
void CL_IRCSay( )
{
	char m_sendstring[480];
	qboolean send_result;

	if (Cmd_Argc() != 2) {
		Com_Printf ("usage: irc_say <text>\n");
		return;
	}

	if ( IRC_ThreadStatus != IRC_THREAD_JOINED ) {
		Com_Printf("IRC: Not connected\n");
		return;
	}

	memset( m_sendstring , 0 , sizeof( m_sendstring ) );
	strncpy( m_sendstring , Cmd_Argv(1) , 479 );
	if ( m_sendstring[ 0 ] == 0 )
		return;

	if ( ( m_sendstring[ 0 ] == '/' || m_sendstring[ 0 ] == '.' ) && !Q_strnicmp( m_sendstring + 1 , "me " , 3 ) && m_sendstring[ 4 ] != 0 ) {
		send_result = IRC_AddSendItem( true , m_sendstring + 4 );
	} else {
		send_result = IRC_AddSendItem( false , m_sendstring );
	}

	if ( !send_result )
		Com_Printf( "IRC: flood detected, message not sent\n" );
}


/*
 * Processes the next item on the send queue, if any.
 */
static qboolean IRC_ProcessSendQueue( )
{
	const char * fmt_string;
	int event , rv;

	if ( !IRC_SendQueue[ IRC_SendQueue_Process ].has_content )
		return true;

	if ( IRC_SendQueue[ IRC_SendQueue_Process ].is_action ) {
		fmt_string = "PRIVMSG #%s :\001ACTION %s\001";
		event = IRC_MakeEvent(ACT, 1);
	} else {
		fmt_string = "PRIVMSG #%s :%s";
		event = IRC_MakeEvent(SAY, 1);
	}

	rv = IRC_Send( fmt_string , cl_IRC_channel->string , IRC_SendQueue[ IRC_SendQueue_Process ].string );
	if ( rv == IRC_CMD_SUCCESS ) {
		IRC_Display( event , IRC_User.nick , IRC_SendQueue[ IRC_SendQueue_Process ].string );
	}
	IRC_SendQueue[ IRC_SendQueue_Process ].has_content = false;
	IRC_SendQueue_Process = ( IRC_SendQueue_Process + 1 ) % IRC_SENDQUEUE_SIZE;
	return ( rv == IRC_CMD_SUCCESS );
}



/*
 * Attempts to receive data from the server. If data is received, parse it
 * and attempt to execute a handler for each complete message.
 */
static int IRC_ProcessData(void)
{
	char buffer[ IRC_RECV_BUF_SIZE ];
	int i , len , err_code;

	len = recv( IRC_Socket, buffer, IRC_RECV_BUF_SIZE, 0 );

	// Handle errors / remote disconnects
	if ( len <= 0 ) {
		if ( len < 0 )
			IRC_HandleError( );
		IRC_ThreadStatus = IRC_THREAD_QUITTING;
		return IRC_CMD_RETRY;
	}

	for ( i = 0 ; i < len ; i ++ ) {
		if ( IRC_Parser( buffer[ i ] ) ) {
#ifdef DEBUG_DUMP_IRC
			IRC_DumpMessage( );
#endif // DEBUG_DUMP_IRC
			err_code = IRC_ExecuteHandler( );
			if ( err_code != IRC_CMD_SUCCESS )
				return err_code;
		}
	}

	return IRC_CMD_SUCCESS;
}


/*
 * Prepares the user record which is used when issuing the USER command.
 */
static qboolean IRC_InitialiseUser( const char * name )
{
	qboolean ovrnn;
	const char * source;
	int i = 0, j = 0;
	int replaced = 0;
	char c;

	ovrnn = cl_IRC_override_nickname->integer && strlen( cl_IRC_nickname->name );
	source = ovrnn ? cl_IRC_nickname->string : name;

	// Strip color chars for the player's name, and remove special
	// characters
	IRC_User.nicklen = 0;
	IRC_User.nickattempts = 1;
	while ( j < 15 ) {
		if ( !ovrnn ) {
			// Only process color escape codes if the nickname
			// is being computed from the player source
			if ( i == 32 || !source[i] ) {
				IRC_User.nick[j ++] = 0;
				continue;
			}
			if ( source[i] == Q_COLOR_ESCAPE ) {
				i ++;
				if ( source[i] != Q_COLOR_ESCAPE ) {
					if ( source[i] )
						i ++;
					continue;
				}
			}
		}

		c = source[i ++];
		if ( j == 0 && !( IS_ALPHA( c ) || strchr( "[]\\`_^{|}" , c ) ) ) {
			c = '_';
			replaced ++;
		} else if ( j > 0 && !( IS_ALNUM( c ) || strchr( "-[]\\`_^{|}" , c ) ) ) {
			c = '_';
			replaced ++;
		}
		IRC_User.nick[j] = c;

		// User names are even more sensitive
		if ( ! ( c == '-' || c == '.' || c == '_' || IS_ALNUM( c ) ) )
			c = '_';
		IRC_User.username[j] = c;

		IRC_User.nicklen = ++j;
	}

	// If the nickname is overriden and its modified value differs,
	// then it is invalid
	if ( ovrnn && strcmp( source , IRC_User.nick ) )
		return false;

	// Set static address
	strcpy( IRC_User.email, "mymail@mail.com" );

	return ( IRC_User.nicklen > 0 && replaced < IRC_User.nicklen / 2 );
}


/*
 * Establishes the IRC connection, sets the nick, etc...
 */
#define CHECK_SHUTDOWN { if ( IRC_QuitRequested ) return IRC_CMD_FATAL; }
#define CHECK_SHUTDOWN_CLOSE { if ( IRC_QuitRequested ) { closesocket( IRC_Socket ); return IRC_CMD_FATAL; } }
static int IRC_AttemptConnection( )
{
	struct sockaddr_in address;			// socket address
	struct hostent * host;           		// host lookup
	char host_name[100];				// host name
	char name[32];					// player's name
	int err_code;
	int port;

	CHECK_SHUTDOWN;
	Com_Printf("...IRC: connecting to server\n");

	// Force players to use a non-default name
	strcpy( name, Cvar_VariableString( "name" ) );
	if (! Q_strnicmp( name , "player" , 7 ) ) {
		Com_Printf("...IRC: rejected due to unset player name\n");
		return IRC_CMD_FATAL;
	}

	// Prepare USER record
	if (! IRC_InitialiseUser( name ) ) {
		Com_Printf("...IRC: rejected due to mostly unusable player name\n");
		return IRC_CMD_FATAL;
	}

	// Find server address
	Q_strncpyz2( host_name, cl_IRC_server->string, sizeof(host_name) );
	if ( (host=gethostbyname(host_name)) == NULL ) {
		Com_Printf("...IRC: unknown server\n");
		return IRC_CMD_FATAL;
	}

	// Create socket
	CHECK_SHUTDOWN;
	if ( (IRC_Socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET ) {
		IRC_HandleError( );
		return IRC_CMD_FATAL;
	}

	// Initialise socket address
	port = cl_IRC_port->integer;
	if ( port <= 0 || port >= 65536 ) {
		Com_Printf("IRC: invalid port number, defaulting to 6667\n");
		port = 6667;
	}
	address.sin_family = AF_INET;
	address.sin_port = htons( port );
	address.sin_addr.s_addr = *((unsigned long *) host->h_addr);

	// Attempt connection
	if ( (connect(IRC_Socket,(struct sockaddr *) &address, sizeof(address))) != 0) {
		closesocket(IRC_Socket);
		Com_Printf("...IRC connection refused.\n");
		return IRC_CMD_RETRY;
	}

	// Send username and nick name
	CHECK_SHUTDOWN_CLOSE;
	err_code = IRC_Send( "USER %s %s %s :%s" , IRC_User.username , IRC_User.email , host_name , IRC_User.nick );
	if ( err_code == IRC_CMD_SUCCESS )
		err_code = IRC_SendNickname( );
	if ( err_code != IRC_CMD_SUCCESS ) {
		closesocket(IRC_Socket);
		return err_code;
	}

	// Initialise parser and set thread state
	IRC_ParserState = IRC_PARSER_START;
	IRC_ThreadStatus = IRC_THREAD_SETNICK;

	CHECK_SHUTDOWN_CLOSE;
	Com_Printf("...Connected to IRC server\n");
	return IRC_CMD_SUCCESS;
}



/*
 * Attempt to connect to the IRC server for the first time.
 * Only retry a few times and assume the server's dead/does not exist if
 * connection can't be established.
 */
static qboolean IRC_InitialConnect( )
{
	int err_code , retries = 3;
	int rc_delay = cl_IRC_reconnect_delay->integer;
	if ( rc_delay < 5 )
		rc_delay = 5;

	err_code = IRC_CMD_SUCCESS;
	IRC_ThreadStatus = IRC_THREAD_CONNECTING;
	do {
		// If we're re-attempting a connection, wait a little bit,
		// or we might just piss the server off.
		if ( err_code == IRC_CMD_RETRY ) {
			IRC_Sleep( rc_delay );
		} else if ( IRC_QuitRequested ) {
			return false;
		}

		err_code = IRC_AttemptConnection( );
	} while ( err_code == IRC_CMD_RETRY && --retries > 0 );

	return ( err_code == IRC_CMD_SUCCESS );
}



/*
 * Attempt to reconnect to the IRC server. Only stop trying on fatal errors
 * or if the thread's status is set to QUITTING.
 */
static int IRC_Reconnect( )
{
	int err_code;
	int rc_delay = cl_IRC_reconnect_delay->integer;
	if ( rc_delay < 5 )
		rc_delay = 5;

	err_code = IRC_CMD_SUCCESS;
	IRC_ThreadStatus = IRC_THREAD_CONNECTING;
	do {
		IRC_Sleep( ( err_code == IRC_CMD_SUCCESS ) ? ( rc_delay >> 1 ) : rc_delay );
		if ( IRC_QuitRequested ) {
			return IRC_CMD_FATAL;
		}
		err_code = IRC_AttemptConnection( );
	} while ( err_code == IRC_CMD_RETRY );

	return err_code;
}





/*
 * IRC main loop. Once the initial connection has been established, either
 * 1) pump messages or 2) handle delayed functions. Try re-connecting if
 * connection is lost.
 */
static void IRC_MainLoop()
{
	int err_code;

	// Connect to server
	if (! IRC_InitialConnect() )
		return;

	do {
		do {
			// If we must quit, send the command.
			if ( IRC_QuitRequested && IRC_ThreadStatus != IRC_THREAD_QUITTING ) {
				IRC_ThreadStatus = IRC_THREAD_QUITTING;
				IRC_Display( IRC_MakeEvent(QUIT,1) , "" , "quit from menu" );
				err_code = IRC_Send( "QUIT :AlienArena IRC %s" , VERSION );
			} else {
				// Wait for data or 1s timeout
				err_code = IRC_Wait( );
				if ( err_code == IRC_CMD_SUCCESS ) {
					// We have some data, process it
					err_code = IRC_ProcessData( );
				} else if ( err_code == IRC_CMD_RETRY ) {
					// Timed out, handle timers and update rate limiter
					err_code = IRC_ProcessDEQueue();
					IRC_UpdateRateLimiter( );
				} else {
					// Disconnected, but reconnection should be attempted
					err_code = IRC_CMD_RETRY;
				}
				
				if ( err_code == IRC_CMD_SUCCESS && ! IRC_QuitRequested )
					err_code = IRC_ProcessSendQueue( ) ? IRC_CMD_SUCCESS : IRC_CMD_RETRY;
			}
		} while ( err_code == IRC_CMD_SUCCESS );
		closesocket( IRC_Socket );

		// If we must quit, let's skip trying to reconnect
		if ( IRC_QuitRequested || err_code == IRC_CMD_FATAL )
			return;

		// Reconnect to server
		do {
			err_code = IRC_Reconnect( );
		} while ( err_code == IRC_CMD_RETRY );
	} while ( err_code != IRC_CMD_FATAL );
}



/*
 * Main function of the IRC thread: initialise command handlers,
 * start the main loop, and uninitialise handlers after the loop
 * exits.
 */
static void IRC_Thread( )
{
	// Init. send queue & rate limiter
	IRC_InitSendQueue( );
	IRC_InitRateLimiter( );
	IRC_InitHandlers( );

	// Init. IRC handlers
	IRC_AddHandler( "PING" , &IRCH_Ping );		// Ping request
	IRC_AddHandler( "ERROR" , &IRCH_ServerError );	// Server error
	IRC_AddHandler( "JOIN" , &IRCH_Joined );	// Channel join
	IRC_AddHandler( "PART" , &IRCH_Part );		// Channel part
	IRC_AddHandler( "QUIT" , &IRCH_Quit );		// Client quit
	IRC_AddHandler( "PRIVMSG" , &IRCH_PrivMsg );	// Message or CTCP
	IRC_AddHandler( "KICK" , &IRCH_Kick );		// Kick
	IRC_AddHandler( "NICK" , &IRCH_Nick );		// Nick change
	IRC_AddHandler( "001" , &IRCH_Connected );	// Connection established
	IRC_AddHandler( "404" , &IRCH_Banned );		// Banned (when sending message)
	IRC_AddHandler( "432" , &IRCH_FatalError );	// Erroneous nick name
	IRC_AddHandler( "433" , &IRCH_NickError );	// Nick name in use
	IRC_AddHandler( "474" , &IRCH_Banned );		// Banned (when joining)

	// Init. CTCP handlers
	IRC_AddCTCPHandler( "ACTION" , &CTCP_Action );	// "/me"
	IRC_AddCTCPHandler( "PING" , &CTCP_Ping );
	IRC_AddCTCPHandler( "VERSION" , &CTCP_Version );

	// Enter loop
	IRC_MainLoop( );

	// Clean up
	Com_Printf( "...IRC: disconnected from server\n" );
	IRC_FlushDEQueue( );
	IRC_FreeHandlers( );
	IRC_SetThreadDead( );
}



/*
 * Caution: IRC_SystemThreadProc(), IRC_StartThread() and IRC_WaitThread()
 *  have separate "VARIANTS".
 *
 * Note different prototypes for IRC_SystemThreadProc() and completely
 * different IRC_StartThread()/IRC_WaitThread() implementations.
 */
#if defined WIN32_VARIANT

/****** THREAD HANDLING - WINDOWS VARIANT ******/

static HANDLE IRC_ThreadHandle = NULL;

static DWORD WINAPI IRC_SystemThreadProc( LPVOID dummy)
{
	IRC_Thread( );
	return 0;
}


static void IRC_StartThread()
{
	if ( IRC_ThreadHandle == NULL )
		IRC_ThreadHandle = CreateThread( NULL , 0 , IRC_SystemThreadProc , NULL , 0 , NULL );
}


static void IRC_SetThreadDead( )
{
	IRC_ThreadStatus = IRC_THREAD_DEAD;
	IRC_ThreadHandle = NULL;
}


static void IRC_WaitThread()
{
	if ( IRC_ThreadHandle != NULL ) {
		if ( IRC_ThreadStatus != IRC_THREAD_DEAD ) {
			WaitForSingleObject( IRC_ThreadHandle , 10000 );
			CloseHandle( IRC_ThreadHandle );
		}
		IRC_ThreadHandle = NULL;
	}
}

#elif defined UNIX_VARIANT

/****** THREAD HANDLING - UNIX VARIANT ******/

static pthread_t IRC_ThreadHandle = (pthread_t) NULL;


static void *IRC_SystemThreadProc(void *dummy)
{
	IRC_Thread( );
	return NULL;
}


static void IRC_StartThread(void)
{
	if ( IRC_ThreadHandle == (pthread_t) NULL )
		pthread_create( &IRC_ThreadHandle , NULL , IRC_SystemThreadProc , NULL );
}


static void IRC_SetThreadDead( )
{
	IRC_ThreadStatus = IRC_THREAD_DEAD;
	IRC_ThreadHandle = (pthread_t) NULL;
}


static void IRC_WaitThread() 
{
	if ( IRC_ThreadHandle != (pthread_t) NULL ) {
		if ( IRC_ThreadStatus != IRC_THREAD_DEAD )
			pthread_join( IRC_ThreadHandle , NULL );
		IRC_ThreadHandle = (pthread_t) NULL;
	}
}

#endif


void CL_IRCSetup(void)
{
	cl_IRC_connect_at_startup = Cvar_Get( "cl_IRC_connect_at_startup" , "1" , CVAR_ARCHIVE );
	cl_IRC_server = Cvar_Get( "cl_IRC_server" , DEFAULT_IRC_SERVER , CVAR_ARCHIVE );
	if (USE_ALIENARENA_ORG_IRC) {
		// Repair if the old defaults are used
		CL_RepairStringCvarIfDefault(cl_IRC_server, IRC_PLANETARENA, IRC_ALIENARENA_ORG);
		CL_RepairStringCvarIfDefault(cl_IRC_server, IRC_HAL_NANOID, IRC_ALIENARENA_ORG);
	}
	cl_IRC_channel = Cvar_Get( "cl_IRC_channel" , "alienarena" , CVAR_ARCHIVE );
	cl_IRC_port = Cvar_Get( "cl_IRC_port" , "6667" , CVAR_ARCHIVE );
	cl_IRC_override_nickname = Cvar_Get( "cl_IRC_override_nickname" , "0" , CVAR_ARCHIVE );
	cl_IRC_nickname = Cvar_Get( "cl_IRC_nickname" , "" , CVAR_ARCHIVE );
	cl_IRC_kick_rejoin = Cvar_Get( "cl_IRC_kick_rejoin" , "0" , CVAR_ARCHIVE );
	cl_IRC_reconnect_delay = Cvar_Get( "cl_IRC_reconnect_delay" , "100" , CVAR_ARCHIVE );

	if ( cl_IRC_connect_at_startup->value )
		CL_InitIRC( );
}


void CL_InitIRC(void)
{
	if ( IRC_ThreadStatus != IRC_THREAD_DEAD ) {
		Com_Printf( "...IRC thread is already running\n" );
		return;
	}
	IRC_QuitRequested = false;
	IRC_ThreadStatus = IRC_THREAD_INITIALISING;
	IRC_StartThread( );
}


void CL_IRCInitiateShutdown(void)
{
	IRC_QuitRequested = true;
}


void CL_IRCWaitShutdown( void )
{
	IRC_WaitThread( );
}


qboolean CL_IRCIsConnected(void)
{
	return ( IRC_ThreadStatus == IRC_THREAD_JOINED );
}


qboolean CL_IRCIsRunning(void)
{
	return ( IRC_ThreadStatus != IRC_THREAD_DEAD );
}
