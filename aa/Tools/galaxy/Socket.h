#if !defined(zSock)
#define zSock

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <winsock.h>
#include <io.h>
#include "userinfo.h"

class cSocket 
{
 
  public:
  WORD wVersionRequested;          /* socket dll version info */ 
  WSADATA wsaData;                 /* data for socket lib initialisation */
  SOCKET sock;                     /* socket details */
  struct sockaddr_in address;      /* socket address stuff */
  struct hostent * host;           /* host stuff */
  int err;                         /* error trapping */
  float socklib_ver;               /* socket dll version */
  char File_Buf[3000];          /* file buffer */
  bool LineCompleted;
  char DomainName[100];            /* domain name from user */
  char HostName[100];              /* host name from user */
  int len;                         
  int ichar;
  time_t now;                      /* for date and time */
  int BUF_LEN;           /* Buffer size for transfers */
  public:
  void init(int, char*);
  void getData(void);
  void sendData(char*);
  void connectf(void);
  void handle_error(void);
};

#endif
 
