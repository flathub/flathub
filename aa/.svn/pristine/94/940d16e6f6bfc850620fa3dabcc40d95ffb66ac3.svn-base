#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <winsock.h>
#include <iostream.h>
#include "userinfo.h"
#include "socket.h"
#include "functions.h"

void cSocket::init(int port,char* server)
{
	BUF_LEN=200; 
   
   wVersionRequested = MAKEWORD( 1, 1 );

   if ( WSAStartup( wVersionRequested, &wsaData ) != 0 )
 //  handle_error();

 

   address.sin_family=AF_INET;       /* internet */
   address.sin_port = htons(port);    

    sprintf(HostName, server) ;

    if ( (host=gethostbyname(HostName)) == NULL )
 //	handle_error();

    address.sin_addr.s_addr=*((unsigned long *) host->h_addr);
  
}

void cSocket::sendData(char *msg)
{
send(sock, msg,strlen(msg),0); 
};

void cSocket::connectf(void) 
{
  if ( (connect(sock,(struct sockaddr *) &address, sizeof(address))) != 0)
  //handle_error();
}

void cSocket::getData(void)
{
	len=0;	
	
	
	{
		//memset(texto.text, 31, sizeof(texto.text));
		memset(File_Buf,0,3000);
		if((len=recv(sock,File_Buf,3000,0))>0) 

		{
		    // si recivimos el PING del servidor...
			if (!strnicmp(File_Buf,"PING",4)) 
			{
			File_Buf[1]='O';
			sendData(File_Buf);
//			m_chatthread.SetWindowText("%s",File_Buf);
			};
		}

		
	
	}

}

