/*
**  FCE.H (FCE.H2) --- Version 2.5
**
**  Use for both Win16 & Win32.
**
**  For Win32 compilers supporting the "declspec" keyword.
*/

#ifdef STATIC_LIBRARY
  #ifdef WIN32
    #define DLL_IMPORT_EXPORT
  #else
    #define DLL_IMPORT_EXPORT FAR PASCAL
  #endif
#else
  #ifdef WIN32
    #ifdef DLL_SOURCE_CODE
       #define DLL_IMPORT_EXPORT __declspec(dllexport) __stdcall
    #else
       #define DLL_IMPORT_EXPORT __declspec(dllimport) __stdcall
    #endif
  #else
    #define DLL_IMPORT_EXPORT FAR PASCAL
  #endif
#endif

#ifdef __cplusplus
  #define NoMangle extern "C"
#else
  #define NoMangle
#endif

#define FCE_SET_CONNECT_WAIT       1
#define FCE_SET_MIN_RESPONSE_WAIT  2
#define FCE_SET_MAX_RESPONSE_WAIT  3
#define FCE_SET_MIN_LINE_WAIT      4
#define FCE_SET_MAX_LINE_WAIT      5
#define FCE_SET_AUTO_CALL_DRIVER   6
#define FCE_SET_SLEEP_TIME         8
#define FCE_SET_FTP_PORT           9
#define FCE_SET_CLOSE_LINGER      10
#define FCE_SET_WRITE_BUFSIZE     11
#define FCE_SET_PASSIVE           12
#define FCE_SET_MAX_LISTEN_WAIT   13
#define FCE_SET_MASTER_INDEX      14
#define FCE_SET_APPEND_MODE       15
#define FCE_SET_DATA_PORT         16
#define FCE_SET_RENAME_DELIMITER  17
#define FCE_SET_SERVER_OFFSET     18
#define FCE_SET_CLIENT_OFFSET     19
#define FCE_SET_LOG_FILE          20
#define FCE_WRITE_TO_LOG          21
#define FCE_SET_MAX_COMPLETE_WAIT 22
#define FCE_SET_BLOCKING_MODE     23
#define FCE_AUTO_LOG_CLOSE        24
#define FCE_CLOSE_LOG_FILE        25

#define FCE_SET_CONNECT_WAIT_IN_SECS      101
#define FCE_SET_MAX_RESPONSE_WAIT_IN_SECS 102
#define FCE_SET_MAX_LINE_WAIT_IN_SECS     103

#define FCE_GET_COUNTER            2
#define FCE_GET_RESPONSE           3
#define FCE_GET_SOCK_ERROR         4

#define FCE_GET_FILE_BYTES_RCVD    10
#define FCE_GET_TOTAL_BYTES_RCVD   11
#define FCE_GET_FILE_BYTES_SENT    12
#define FCE_GET_TOTAL_BYTES_SENT   13
#define FCE_GET_SOCKET             14
#define FCE_GET_CONNECT_STATUS     15
#define FCE_GET_REGISTRATION       16

#define FCE_GET_LAST_RESPONSE      18
#define FCE_GET_SERVER_IP          19
#define FCE_GET_VERSION            20
#define FCE_GET_BUILD              21
#define FCE_GET_LINE_COUNT         22
#define FCE_GET_LOCAL_IP           23
#define FCE_GET_ERROR_LINE         24
#define FCE_GET_QUEUE_ZERO         25
#define FCE_GET_DATA_PORT          26
#define FCE_GET_DAYS_LEFT          27

#define FCE_FULL_LIST               0
#define FCE_NAME_LIST               1
#define FCE_FULL_LIST_FILE          2
#define FCE_NAME_LIST_FILE          3

#define BYTE  unsigned char
#define ULONG unsigned long

NoMangle int   DLL_IMPORT_EXPORT fceAbort(int);
NoMangle int   DLL_IMPORT_EXPORT fceAttach(int,long);
NoMangle int   DLL_IMPORT_EXPORT fceClose(int);
NoMangle int   DLL_IMPORT_EXPORT fceCommand(int,LPSTR);
NoMangle int   DLL_IMPORT_EXPORT fceConnect(int,LPSTR,LPSTR,LPSTR);
NoMangle int   DLL_IMPORT_EXPORT fceDebug(int,int,LPSTR,int);
NoMangle int   DLL_IMPORT_EXPORT fceDelFile(int,LPSTR);
NoMangle int   DLL_IMPORT_EXPORT fceDelServerDir(int,LPSTR);
NoMangle int   DLL_IMPORT_EXPORT fceDriver(int);
NoMangle int   DLL_IMPORT_EXPORT fceErrorText(int,int,LPSTR,int);
NoMangle int   DLL_IMPORT_EXPORT fceExtract(LPSTR,int,int,LPSTR,int);
NoMangle ULONG DLL_IMPORT_EXPORT fceFileLength(LPSTR,int,int);
NoMangle int   DLL_IMPORT_EXPORT fceGetFile(int,LPSTR);
NoMangle ULONG DLL_IMPORT_EXPORT fceGetInteger(int,int);
NoMangle int   DLL_IMPORT_EXPORT fceGetList(int,int,LPSTR,int);
NoMangle int   DLL_IMPORT_EXPORT fceGetLocalDir(int,LPSTR,int);
NoMangle int   DLL_IMPORT_EXPORT fceGetLocalFList(int,LPSTR,int); 
NoMangle ULONG DLL_IMPORT_EXPORT fceGetLocalFSize(int,LPSTR);
NoMangle int   DLL_IMPORT_EXPORT fceGetServerDir(int,LPSTR,int);
NoMangle int   DLL_IMPORT_EXPORT fceGetString(int,int,LPSTR,int);
NoMangle int   DLL_IMPORT_EXPORT fceHello(int);
NoMangle int   DLL_IMPORT_EXPORT fceMakeServerDir(int, LPSTR);
NoMangle ULONG DLL_IMPORT_EXPORT fceMatchFile(LPSTR,ULONG,LPSTR,int,LPSTR,int);
NoMangle int   DLL_IMPORT_EXPORT fcePutFile(int,LPSTR);
NoMangle int   DLL_IMPORT_EXPORT fceRelease(void);
NoMangle int   DLL_IMPORT_EXPORT fceSetInteger(int,int,ULONG);
NoMangle int   DLL_IMPORT_EXPORT fceSetLocalDir(int,LPSTR);
NoMangle int   DLL_IMPORT_EXPORT fceSetMode(int,char);
NoMangle int   DLL_IMPORT_EXPORT fceSetServerDir(int,LPSTR);
NoMangle int   DLL_IMPORT_EXPORT fceSetString(int,int,LPSTR);
NoMangle int   DLL_IMPORT_EXPORT fceShortToByte(LPSTR);
NoMangle int   DLL_IMPORT_EXPORT fceByteToShort(LPSTR);

