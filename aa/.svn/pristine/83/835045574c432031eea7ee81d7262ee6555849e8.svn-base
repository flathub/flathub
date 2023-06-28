/*
 * Module ID: windebug.h
 * Title    : Header file for debugging services specific to Windows.
 * Purpose  : Provide debugging macros specific to Windows
 *
 * Author   : Olivier Langlois <olivier@olivierlanglois.net>
 * Date     : December, 6 2005
 *
 * Note     : Adapted from Andrew Schetinin code
 *            http://www.codeproject.com/debug/qafdebug.asp
 *
 * Revision :
 *
 */

#ifndef   _WINDEBUG_H_
#define   _WINDEBUG_H_

#include "debug.h"
#include <windows.h>

#define DEBUG_LOG_SUBDIR        __TEXT("Log")
#define DEBUG_LOG_FILE_NAME     __TEXT("error.log")
#define DEBUG_OLD_LOG_FILE_NAME __TEXT("error.log.old")

/*
 * Maximum log file size
 *
 * Maximum log file size (there are two log files - one current and second previous).
 * When the log file size exceeds half of this limit, it is renamed to the second name
 * (thus both files together cannot take more than this maximum size).
 * The size is in bytes. Usually 1 record takes about 500 characters, so it reserve
 * space for about 2,000 records with 1 Mb limit.
 */
#define DEBUG_LOG_FILE_MAX_SIZE  (1024*1024)

// Errors in the reporting engine
#define DEBUG_ERROR_PREFIX __TEXT("Debug System Error --> ")

// Fixed error message for unknown error
#define DEBUG_ERROR_NO_MESSAGE __TEXT("[Could not find any description for the error]")

// Fixed error message for assertion raised
#define DEBUG_ERROR_ASSERTION __TEXT("Assertion raised")
#define DEBUG_LASTERROR       __TEXT("Win32 Error")

/*
 * Release macros
 */
#define WINASSERTR(a) (a)?TRUE:NWinDebug::OutputDebugStringEx(0, __TEXT(__FILE__),\
                                __LINE__,__TEXT(#a), DEBUG_ERROR_ASSERTION)
#define LASTERRORDISPLAYR(a) (a)?TRUE:NWinDebug::OutputDebugStringEx(GetLastError(),\
                                       __TEXT(__FILE__),__LINE__,__TEXT(#a),\
                                       DEBUG_LASTERROR)
#define TRACER(a)            NWinDebug::OutputDebugStringEx(0,\
                                       __TEXT(__FILE__),__LINE__,__TEXT(""),\
                                       a,NULL,FALSE)

/*
 * Debug macros
 */
#ifdef DEBUG
#define WINASSERTD(a)        WINASSERTR(a)
#define LASTERRORDISPLAYD(a) LASTERRORDISPLAYR(a)
#define TRACED(a)            TRACER(a)
#else
#define WINASSERTD(a)
#define LASTERRORDISPLAYD(a) (a)
#define TRACED(a)
#endif

namespace NWinDebug
{
/******************************************************************************
 *
 * Name      : OutputDebugStringEx
 *
 * Purpose   : Report on screen the error and log it.
 *
 * Parameters:
 *     err            (DWORD)     Error code returned by GetLastError()
 *     szFileName     (LPCTSTR)   Filename where the error occured.
 *     iLine          (const int) Line number where the error occured.
 *     szStatement    (LPCTSTR)   Statement that produced the error.
 *     szErrorMessage (LPCTSTR)   Error message.
 *     res            (LPTSTR)    If not NULL, the formatted string will be
 *                                stored in this buffer.
 *     bWarnUser      (BOOL)      Display a message box if true
 *
 * Return value : None
 *
 ****************************************************************************/
BOOL OutputDebugStringEx( DWORD err, LPCTSTR szFileName, const int iLine,
                          LPCTSTR szStatement, LPCTSTR szErrorMessage = __TEXT(""),
					      LPTSTR res = NULL, BOOL bWarnUser = TRUE );

/******************************************************************************
 *
 * Name      : GetLogDir
 *
 * Purpose   : Return an accessible directory name for all log files.
 *             If the folders are missing on the disk, they are created.
 *
 * Parameters:
 *     lpszOutputBuffer (LPTSTR)      Output Buffer
 *     dwBufSize        (const DWORD) Output buffer size
 *
 * Return value : (DWORD) the number of characters in the output buffer
 *                        or 0 in a case of any error.
 *
 ****************************************************************************/
	DWORD GetLogDir( LPTSTR lpszOutputBuffer, const DWORD dwBufSize );

/******************************************************************************
 *
 * Name      : IsValidAddress
 *
 * Purpose   : Determine if lp is a valid address.
 *
 * Parameters:
 *     lp      (const void *) Address to test
 *     nBytes  (UINT)         buffer size
 *     bWrite  (BOOL)         Flag used to test Address write access.
 *
 * Return value : (BOOL) TRUE if the passed parameter points
 *                       to at least nBytes of accessible memory.
 *
 ****************************************************************************/
	BOOL IsValidAddress(const void* lp, UINT nBytes,
	                    BOOL bWrite  = TRUE);
};

#endif /* _WINDEBUG_H_ */
