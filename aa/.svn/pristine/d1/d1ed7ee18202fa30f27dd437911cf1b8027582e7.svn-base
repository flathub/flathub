/*
 * Module ID: windebug.cpp
 * Title    : Implementation of debugging services specific to Windows.
 * Purpose  : Provide debugging macros specific to Windows
 *
 * Author   : Olivier Langlois <olivier@olivierlanglois.net>
 * Date     : December, 6 2005
 *
 * Note     : Adapted from Andrew Schetinin code
 *            http://www.codeproject.com/debug/qafdebug.asp
 */

#include "win/windebug.h"
#include <tchar.h>
#include <stdio.h>     // For fopen(), fclose() and fwrite()
#include <io.h>        // For access() and _filelength()
#include <direct.h>    // For mkdir()

#define BUFFER_SIZE     2048
#define MAX_APP         MAX_PATH
#define PATH_DELIMITER  '\\'
#define EOL             __TEXT("\n")

// 3 parameters
// "filename(line) : error message"
#define DEBUG_FMT_SPOS __TEXT("%.250s(%d) : %.600s")

/// End of line + tab
#define DEBUG_FMT_PREFIX EOL __TEXT("\t")

// 7 parameters - formatting date and time
// "        time:        2003-12-07 15:48:07:507"
#define DEBUG_FMT_DATE DEBUG_FMT_PREFIX __TEXT("time\t: ") __TEXT("%04d-%02d-%02d %02d:%02d:%02d:%03d")

// 1 parameter - formatting process info
// "        process:     0x00000E24"
#define DEBUG_FMT_PROC DEBUG_FMT_PREFIX __TEXT("process\t: ") __TEXT("0x%08X")

// 1 parameter - formatting thread info
// "        thread:      0x00000E2C"
#define DEBUG_FMT_THRE DEBUG_FMT_PREFIX __TEXT("thread\t: ") __TEXT("0x%08X")

// 1 parameter - formatting application info
// "        application: WINWORD.EXE <1.4.6.7>"
#define DEBUG_FMT_APPL DEBUG_FMT_PREFIX __TEXT("application : ") __TEXT("%.250s")

// 2 parameter - formatting last error info
// "        last error:  6, The handle is invalid."
#define DEBUG_FMT_LERR DEBUG_FMT_PREFIX __TEXT("last error : ") __TEXT("%d, %.250s")

// 1 parameter - formatting expression info
// "        expression:  DuplicateHandle( NULL, NULL, NULL, NULL, 0, FALSE, 0 )"
#define DEBUG_FMT_EXPR DEBUG_FMT_PREFIX __TEXT("expression : ") __TEXT("%.250s")

// The resulting format strings
// 16 parameters
#define DEBUG_STD_FORMAT \
			DEBUG_FMT_SPOS \
			DEBUG_FMT_DATE \
			DEBUG_FMT_PROC \
			DEBUG_FMT_THRE \
			DEBUG_FMT_APPL \
			DEBUG_FMT_LERR \
			DEBUG_FMT_EXPR EOL

// The resulting format strings
// 14 parameters
#define DEBUG_NOERR_FORMAT \
			DEBUG_FMT_SPOS \
			DEBUG_FMT_DATE \
			DEBUG_FMT_PROC \
			DEBUG_FMT_THRE \
			DEBUG_FMT_APPL \
			DEBUG_FMT_EXPR EOL

// Fixed error messages for faults in the debug engine
#define DEBUG_ERROR_OPEN_LOG_FILE  DEBUG_ERROR_PREFIX __TEXT("Debug log file cannot be opened.") EOL
#define DEBUG_ERROR_CLOSE_LOG_FILE DEBUG_ERROR_PREFIX __TEXT("Cannot close the debug log file.") EOL
#define DEBUG_ERROR_BACKUP         DEBUG_ERROR_PREFIX __TEXT("Cannot create a backup copy of the debug log file") EOL

inline void ODS( LPCTSTR szMessage )
{
	OutputDebugString( szMessage );
}

class CWinDebug
{
private:
	// Name of the log file, it is initialized by the constructor
	TCHAR m_szLogFileName[MAX_PATH];

	// Application name
	TCHAR m_szApplicationName[MAX_APP];

	FILE *m_fLogFile;

	bool openLogFile(void)
	{
		bool bRet = true;

		// Open logfile if not open
		if( !m_fLogFile )
		{
			m_fLogFile = _tfopen(m_szLogFileName, __TEXT("a+") );
			if( m_fLogFile == NULL )
			{
				ODS( DEBUG_ERROR_OPEN_LOG_FILE );
				bRet = false;
			}
		}
		return bRet;
	}

	bool closeLogFile(void)
	{
		bool bRet = true;
		if( m_fLogFile )
		{
			bRet = !fclose(m_fLogFile);
			if( !bRet )
			{
				ODS(DEBUG_ERROR_CLOSE_LOG_FILE);
			}
			m_fLogFile = NULL;
		}
		return bRet;
	}
protected:

/******************************************************************************
 *
 * Name      : GetLogFileName
 *
 * Purpose   : Generate the file name and directory (check that the file is here).
 *             If the folders are missing on the disk, they are created.
 *
 * Parameters:
 *     lpszOutputBuffer (LPTSTR)      Output Buffer
 *     dwBufSize        (const DWORD) Output buffer size
 *
 * Return value : (DWORD) the length of the written string
 *                        or 0 in a case of any error.
 *
 ****************************************************************************/
	static DWORD GetLogFileName( LPTSTR lpszOutputBuffer, const DWORD dwBufSize );

public:
    CWinDebug(void);
	~CWinDebug(void);

/******************************************************************************
 *
 * Name      : GetLogModule
 *
 * Purpose   : Get the full path and version number of the given module and
 *             record them in the output buffer.
 *
 * Parameters:
 *     hModule          (HMODULE)     Module Handle
 *     lpszOutputBuffer (LPTSTR)      Output Buffer
 *     dwBufSize        (const DWORD) Output buffer size
 *
 * Return value : (DWORD) the number of characters in the output buffer
 *                        or 0 in a case of any error.
 *
 ****************************************************************************/
	static DWORD GetLogModule( HMODULE hModule, LPTSTR lpszOutputBuffer,
		                       const DWORD dwBufSize );

	// Return a pointer to a null-terminated string with the application name and version.
	static LPCTSTR GetApplication(void)
	{
		return instance().m_szApplicationName;
	}

	// Get the shared instance of the log class
	static CWinDebug &instance(void)
	{
		// Initialize the static instance (it will be a global variable)
 		static CWinDebug std_err;
		// Return the instance
		return std_err;
	}

    FILE *GetLogFile( void );
};

CWinDebug::CWinDebug(void)
{
	m_fLogFile = NULL;

	if( GetLogModule( NULL, m_szApplicationName, MAX_APP ) == 0 )
	{
		m_szApplicationName[0] = 0;
	}

	// Initialize the file name
	if( 0 == GetLogFileName( m_szLogFileName, MAX_PATH ) )
	{
		m_szLogFileName[0] = 0;
	}
}

CWinDebug::~CWinDebug(void)
{
	closeLogFile();
}

/*
 * CWinDebug::GetLogFileName function
 */
DWORD CWinDebug::GetLogFileName( LPTSTR lpszOutputBuffer, const DWORD dwBufSize )
{
	DWORD dwRet = NWinDebug::GetLogDir( lpszOutputBuffer, dwBufSize );

	dwRet += wsprintf( &lpszOutputBuffer[dwRet], DEBUG_LOG_FILE_NAME );
	return dwRet;
}

/*
 * CWinDebug::SetLogModule function
 *
 * Since this function is called only once and is the only function
 * needing version.dll, Explicit dynamic loading is used instead of implicit
 * with the import library (version.lib). This allows to reduce the WinDebug
 * users to reduce their memory footprint of 40KB!
 */

#ifdef UNICODE
#define GETFILEVERSIONINFOSIZEORDINAL 3
#define GETFILEVERSIONINFOORDINAL     4
#define VERQUERYVALUEORDINAL         14
#else
#define GETFILEVERSIONINFOSIZEORDINAL 2
#define GETFILEVERSIONINFOORDINAL     1
#define VERQUERYVALUEORDINAL         11
#endif

typedef DWORD (APIENTRY *GetFileVersionInfoSizeType)(LPTSTR,LPDWORD);
typedef BOOL (APIENTRY *GetFileVersionInfoType)(LPTSTR,DWORD,DWORD,LPVOID);
typedef BOOL (APIENTRY *VerQueryValueType)(const LPVOID,LPTSTR,LPVOID,PUINT);

class CVersionDLLLoader
{
public:
	CVersionDLLLoader();
	~CVersionDLLLoader();

	BOOL LoadLibrary(void);
	BOOL FreeLibrary(void)
	{
		BOOL bRes = FALSE;
		if( m_hDllHandle )
		{
			bRes = ::FreeLibrary(m_hDllHandle);
		}
		return bRes;
	}
	/*
	 * The next functions assumes that LoadLibrary returned TRUE
	 */
	DWORD GetFileVersionInfoSize(LPTSTR lptstrFilename,LPDWORD lpdwHandle)
	{
		return m_lpfnGetFileVersionInfoSize(lptstrFilename,lpdwHandle);
	}
	BOOL GetFileVersionInfo(LPTSTR lptstrFilename,DWORD dwHandle,DWORD dwLen,LPVOID lpData)
	{
		return m_lpfnGetFileVersionInfo(lptstrFilename,dwHandle,dwLen,lpData);
	}
	BOOL VerQueryValue(const LPVOID pBlock,LPTSTR lpSubBlock, LPVOID *lplpBuffer, PUINT puLen)
	{
		return m_lpfnVerQueryValue(pBlock,lpSubBlock,lplpBuffer,puLen);
	}

private:
	HMODULE m_hDllHandle;
	GetFileVersionInfoSizeType m_lpfnGetFileVersionInfoSize;
	GetFileVersionInfoType m_lpfnGetFileVersionInfo;
	VerQueryValueType m_lpfnVerQueryValue;

};

CVersionDLLLoader::CVersionDLLLoader()
{
	m_hDllHandle = NULL;
    m_lpfnGetFileVersionInfoSize = NULL;
	m_lpfnGetFileVersionInfo = NULL;
	m_lpfnVerQueryValue = NULL;
}

CVersionDLLLoader::~CVersionDLLLoader()
{
	FreeLibrary();
}

BOOL CVersionDLLLoader::LoadLibrary(void)
{
	FreeLibrary();
	m_hDllHandle = ::LoadLibrary(__TEXT("version.dll"));

	if( m_hDllHandle )
	{
		m_lpfnGetFileVersionInfoSize = (GetFileVersionInfoSizeType)
			                   GetProcAddress(m_hDllHandle,
			                   MAKEINTRESOURCE(GETFILEVERSIONINFOSIZEORDINAL));
		m_lpfnGetFileVersionInfo = (GetFileVersionInfoType)
			                   GetProcAddress(m_hDllHandle,
			                   MAKEINTRESOURCE(GETFILEVERSIONINFOORDINAL));
		m_lpfnVerQueryValue = (VerQueryValueType)
			                   GetProcAddress(m_hDllHandle,
			                   MAKEINTRESOURCE(VERQUERYVALUEORDINAL));
	}

	return m_hDllHandle &&
	       m_lpfnGetFileVersionInfoSize &&
		   m_lpfnGetFileVersionInfo &&
		   m_lpfnVerQueryValue;
}

#define MAX_VER_BUF 32

DWORD CWinDebug::GetLogModule( HMODULE hModule, LPTSTR lpszOutputBuffer,
		                       const DWORD dwBufSize )
{
	DWORD dwRet;
	BOOL  bRet;
	CVersionDLLLoader versionLoader;
	DWORD dwModuleFileNameLen = 0;
	DWORD dwReserved = 0;
	UINT  uiSize     = 0;
	VS_FIXEDFILEINFO *pFixedVerInfo = NULL;
	void *pVerInfo = NULL;

	if( (lpszOutputBuffer == NULL) || (dwBufSize < 2) )
	{
		goto SetLogModuleExit;
	}
	lpszOutputBuffer[0] = 0;

	dwModuleFileNameLen = GetModuleFileName( hModule, lpszOutputBuffer, dwBufSize - 1 );

	if( (dwModuleFileNameLen == 0) ||
		((dwModuleFileNameLen + MAX_VER_BUF) > (dwBufSize - 1)) )
	{
		goto SetLogModuleExit;
	}

	bRet = versionLoader.LoadLibrary();
	if( !bRet )
	{
		goto SetLogModuleExit;
	}

	dwRet = versionLoader.GetFileVersionInfoSize( lpszOutputBuffer,
		                                          &dwReserved );
	if( dwRet == 0 )
	{
		goto SetLogModuleExit;
	}

	pVerInfo = malloc( dwRet );
	if( pVerInfo == NULL )
	{
		goto SetLogModuleExit;
	}
	::ZeroMemory( pVerInfo, dwRet );
	bRet = versionLoader.GetFileVersionInfo( lpszOutputBuffer, dwReserved,
		                                     dwRet, pVerInfo );
	if( !bRet )
	{
		goto SetLogModuleExit;
	}

	bRet = versionLoader.VerQueryValue( pVerInfo, __TEXT("\\"),
		                                (LPVOID *)(&pFixedVerInfo), &uiSize );
	if( !bRet || (pFixedVerInfo == NULL ) ||
		(sizeof(VS_FIXEDFILEINFO) != uiSize) )
	{
		goto SetLogModuleExit;
	}

	dwModuleFileNameLen += wsprintf( &lpszOutputBuffer[dwModuleFileNameLen],
		                             __TEXT(" <%d.%d.%d.%d>"),
		                             HIWORD(pFixedVerInfo->dwFileVersionMS),
			                         LOWORD(pFixedVerInfo->dwFileVersionMS),
		                             HIWORD(pFixedVerInfo->dwFileVersionLS),
			                         LOWORD(pFixedVerInfo->dwFileVersionLS) );
	lpszOutputBuffer[dwBufSize - 1] = 0;

SetLogModuleExit:
    free( pVerInfo );
	return dwModuleFileNameLen;
}

/*
 * CWinDebug::GetLogFile function
 *
 * Note: It should eventually be thread safe if used in a
 *       multithread environnement.
 */
FILE *CWinDebug::GetLogFile( void )
{
	long   lFileSize;
	TCHAR  szOldLogFilename[MAX_PATH];
	LPTSTR szFileNamePos;

	// Open logfile if not open
	if( !openLogFile() )
	{
		goto GetLogFileExit;
	}

	// Check log file size
	lFileSize = _filelength(_fileno(m_fLogFile));
	if( lFileSize == -1 )
	{
		// Something bad happened
		closeLogFile();
		goto GetLogFileExit;
	}

	// If file size reached the watermark.
	if( lFileSize > (DEBUG_LOG_FILE_MAX_SIZE / 2) )
	{
		/*
		 * Overwrite the old log file with
		 * the current one and create a new log file.
		 */
		if( !closeLogFile() )
		{
			goto GetLogFileExit;
		}
		lstrcpyn( szOldLogFilename, m_szLogFileName, MAX_PATH );
		szFileNamePos = _tcsrchr( szOldLogFilename, PATH_DELIMITER );
		if( szFileNamePos )
		{
			szFileNamePos++;
		}
		else
		{
			szFileNamePos = szOldLogFilename;
		}
		lstrcpy(szFileNamePos,DEBUG_OLD_LOG_FILE_NAME);

		// ignore the error if the file does not exist (should return 0 on success)
		_tremove( szOldLogFilename );
		if( _trename(m_szLogFileName,szOldLogFilename) )
		{
			ODS(DEBUG_ERROR_BACKUP);
			goto GetLogFileExit;
		}
		// Reopen a new empty log file.
		openLogFile();
	}
GetLogFileExit:
	return m_fLogFile;
}

/*
 * NWinDebug::GetLogDir function
 */
DWORD NWinDebug::GetLogDir( LPTSTR lpszOutputBuffer, const DWORD dwBufSize )
{
	int iStrLen;
	lstrcpyn(lpszOutputBuffer,CWinDebug::GetApplication(),dwBufSize);

	// Get the app path.
	iStrLen = lstrlen(lpszOutputBuffer);
	while( --iStrLen >= 0 && lpszOutputBuffer[iStrLen] != PATH_DELIMITER );
	lpszOutputBuffer[++iStrLen] = '\0';
	iStrLen += wsprintf(&lpszOutputBuffer[iStrLen],DEBUG_LOG_SUBDIR);

    if( _taccess(lpszOutputBuffer,0) != 0 )
	{
		_tmkdir(lpszOutputBuffer);
	}

	// Add final '\'
	lpszOutputBuffer[iStrLen++] = PATH_DELIMITER;
    lpszOutputBuffer[iStrLen]   = '\0';

	return iStrLen;
}

/*
 * NWinDebug::OutputDebugStringEx function
 */
BOOL NWinDebug::OutputDebugStringEx( DWORD err, LPCTSTR szFileName, int iLine,
									 LPCTSTR szStatement,
									 LPCTSTR szErrorMessage,LPTSTR res, BOOL bWarnUser )
{
  int     iNumChar;
  TCHAR   buf[BUFFER_SIZE];
  LPTSTR  bufPtr;
  LPTSTR  szErrBuf = NULL;
  LPCTSTR szStrippedFileName;
  DWORD   dwRet;
  SYSTEMTIME st;

  // test input parameters
  if( szFileName == NULL )
  {
	szFileName = __TEXT("");
  }
  if( szStatement == NULL )
  {
	szStatement = __TEXT("");
  }
  if( szErrorMessage == NULL )
  {
	szErrorMessage = __TEXT("");
  }

  if( res )
  {
	  bufPtr = res;
  }
  else
  {
	  bufPtr = buf;
  }

  // Strip path from the file name
  szStrippedFileName = _tcsrchr( szFileName, PATH_DELIMITER );
  if( szStrippedFileName )
  {
    szStrippedFileName++;
  }
  else
  {
    szStrippedFileName = szFileName;
  }

  GetLocalTime( &st );

  if( err )
  {
    dwRet = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
	                       FORMAT_MESSAGE_FROM_SYSTEM |
                           FORMAT_MESSAGE_IGNORE_INSERTS,
  	                       NULL,
	                       err,
				           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				           (LPTSTR)&szErrBuf,
                           0,
				           NULL );
	if( (dwRet == 0) || (szErrBuf == NULL) )
	{
		szErrBuf = DEBUG_ERROR_NO_MESSAGE;
	}
	iNumChar = wsprintf(bufPtr,DEBUG_STD_FORMAT,
		       szStrippedFileName, iLine, szErrorMessage,
		       st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
		       GetCurrentProcess(),
		       GetCurrentThread(),
		       CWinDebug::GetApplication(),
		       err, szErrBuf,
		       szStatement );
	if( dwRet )
	{
		LocalFree(szErrBuf);
	}
  }
  else
  {
	iNumChar = wsprintf(bufPtr,DEBUG_NOERR_FORMAT,
		       szStrippedFileName, iLine, szErrorMessage,
		       st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
		       GetCurrentProcess(),
		       GetCurrentThread(),
		       CWinDebug::GetApplication(),
		       szStatement );
  }

  if( !res )
  {
	  FILE *f;
	  ODS(bufPtr);
	  f = CWinDebug::instance().GetLogFile();
	  if( f )
	  {
		  fwrite( bufPtr, iNumChar, sizeof(TCHAR), f );
		  // Could add a fflush() call
	  }
	  if( bWarnUser )
	  {
		MessageBox(NULL,bufPtr,__TEXT("Error : Please contact the software developer"),
			       MB_ICONERROR|MB_OK|MB_TOPMOST);
	  }

  }
  return FALSE;
}

/*
 * NWinDebug::IsValidAddress function
 */
BOOL NWinDebug::IsValidAddress(const void* lp, UINT nBytes,
	                           BOOL bWrite  /*= TRUE*/)
{
	// simple version using Win-32 APIs for pointer validation.
	return (lp != NULL && !IsBadReadPtr(lp, nBytes) &&
		(!bWrite || !IsBadWritePtr((LPVOID)lp, nBytes)));
}
