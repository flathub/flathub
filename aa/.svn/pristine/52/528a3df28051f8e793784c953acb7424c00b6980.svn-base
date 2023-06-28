
#include "stdafx.h"
#include <windows.h>
#include <winuser.h>
#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <shlwapi.h>
#include <commctrl.h>

#include <Mmsystem.h>
#include <string.h>
#include <direct.h>
#include <Wininet.h>
#include <afxcmn.h>	/* For progress bar access */

#define MAXSERVERS 256
#define FD_SETSIZE MAXSERVERS  /* This sets the size of fd_set structure in winsock.h */
#define PING_RETEST_THRESH 150 /* ping times, in ms, that will be retested accurately */

#include <winsock.h>

#include "PollServer.h"

SOCKET master;

SERVERINFO servers[MAXSERVERS];

unsigned int numServers = 0;
unsigned int totalPlayers = 0;

void GetServerList (void) {
	
	HOSTENT *hp;
	int i, result;
	struct timeval delay;
	fd_set stoc;
	struct sockaddr_in dgFrom;
	char recvBuff[0xFFFF], *p;
	int tries = 0;

	hp = gethostbyname ("master.corservers.com");

	if (!hp) {
		hp = gethostbyname ("master2.corservers.com"); //second master support
		if(!hp) {
			AfxMessageBox("couldn't resolve master server");
			return; //couldn't resolve master server
		}
	}
retrieve:
	if(tries) {
		hp = gethostbyname ("master2.corservers.com");
		if(!hp) {
			AfxMessageBox("couldn't resolve master server");
			return; //couldn't resolve master server
		}
	}

	memset (recvBuff, 0, sizeof(recvBuff));

	for (i = 0; i < 3; i++) {
		dgFrom.sin_family = AF_INET;
		dgFrom.sin_port = htons (27900);
		memset (&dgFrom.sin_zero, 0, sizeof(dgFrom.sin_zero));
		memcpy (&dgFrom.sin_addr, hp->h_addr_list[0], sizeof(dgFrom.sin_addr));

		result = sendto (master, "query", 5, 0, (const struct sockaddr *)&dgFrom, sizeof (dgFrom));
		if (result == SOCKET_ERROR) {
			return; //couldn't contact master server
		}
		memset (&stoc, 0, sizeof(stoc));
		FD_SET (master, &stoc);
		delay.tv_sec = 1;
		delay.tv_usec = 0;
		result = select (0, &stoc, NULL, NULL, &delay);
		if (result) {
			int fromlen = sizeof(dgFrom);
			result = recvfrom (master, recvBuff, sizeof(recvBuff), 0, (struct sockaddr *)&dgFrom, &fromlen);
			if (result >= 0) {
				break;
			} else if (result == -1) {
				return; //couldn't contact master server
			}
		} 
	}

	if (!result) {
		if(!tries) {
			tries = 1;
			goto retrieve;
		}
		return; //couldn't contact master server
	}

	p = recvBuff + 12;

	result -=12;

	numServers = 0;

	while (result) {
		memcpy (&servers[numServers].ip, p, sizeof (servers[numServers].ip));
		p += 4;
		memcpy (&servers[numServers].port, p, sizeof (servers[numServers].port));
		servers[numServers].port = ntohs(servers[numServers].port);
		p += 2;
		result -= 6;

		if (++numServers == MAXSERVERS)
			break;
	}
}

char *GetLine (char **contents, int *len)
{
	int num;
	int i;
	char line[2048];
	char *ret;

	num = 0;
	line[0] = '\0';

	if (*len <= 0)
		return NULL;

	for (i = 0; i < *len; i++) {
		if ((*contents)[i] == '\n') {
			*contents += (num + 1);
			*len -= (num + 1);
			line[num] = '\0';
			ret = (char *)malloc (sizeof(line));
			strcpy (ret, line);
			return ret;
		} else {
			line[num] = (*contents)[i];
			num++;
		}
	}

	ret = (char *)malloc (sizeof(line));
	strcpy (ret, line);
	return ret;
}

/*****************************************************************************
 10th Jan 2007 - Tony: Parallel replacement for old slow one-at-a-time version
 *****************************************************************************/
void PingServers (SERVERINFO *server, CProgressCtrl *m_refreshprogress)
{
	char *p, *rLine;
	char lasttoken[256];
	char recvBuff[4096];
	char tmptxt[256];
	fd_set readfds;
	fd_set masterfds;
	int result;
	int fromlen;
	struct sockaddr dgFrom;
	TIMEVAL delay;
	SOCKADDR_IN addr;
	char request[] = "ÿÿÿÿstatus\n";
	char seps[]   = "\\";
	char *token;
	int players = 0;
	DWORD now;
	unsigned int msecs, i, j, k, serverindex;
	unsigned int max_fd = 0;
	unsigned int socketcount = 0;
	unsigned int numResponses = 1; /* Used to update progress bar */

	FD_ZERO(&masterfds);
	FD_ZERO(&readfds);

	m_refreshprogress->SetRange(1, (numServers*2)+1);
	m_refreshprogress->SetPos(numResponses);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;

	timeBeginPeriod (1);

	/* Open sockets */
	for(serverindex = 0; serverindex < numServers; serverindex++)
	{
		server[serverindex].socket = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	}

	for(serverindex = 0; serverindex < numServers; serverindex++)
	{
		addr.sin_addr.S_un.S_addr = server[serverindex].ip;
		addr.sin_port = htons(server[serverindex].port);

		server[serverindex].startPing = timeGetTime (); /* Time how long it takes to get a response */
		server[serverindex].responded = 0;

		result = sendto (server[serverindex].socket, request, sizeof(request), 0, (struct sockaddr *)&addr, sizeof(addr));
		if (result == SOCKET_ERROR)
		{
			AfxMessageBox ("Can't send: Winsock error!");
			/* We will carry on with the queries on the other sockets anyway */
			break;
		}
		else
		{
			FD_SET(server[serverindex].socket, &masterfds); /* Add socket to master set */
			socketcount++; /* Record of how many sockets are open, for searching in select() */
			if(server[serverindex].socket > max_fd) /* Work out the maximum file descriptor used for select() */
				max_fd = server[serverindex].socket;
		}
	}

	/* Use a copy of the master file descriptor list, as select() modifies the SET bits to indicate which sockets have an event */
	memcpy(&readfds, &masterfds, sizeof(fd_set));

	/* Set timeout - this is reset every time select() receives a packet, so is not an absolute time across all servers */
	delay.tv_sec = 0;
	delay.tv_usec = 250000l; /* 250ms */
	while((result = select (max_fd+1, &readfds, NULL, NULL, &delay)) > 0) /* Result is number of sockets ready to read */
	{
		/* The whole contents of this section takes about 1-8ms, depending on the number of sockets needing reading */
		now = timeGetTime();  /* Grab time to measure server ping */
		
		/* Find which sockets are ready to read */
		for(i = 0; i < socketcount; i++)
		{
			if(FD_ISSET(server[i].socket, &readfds))  
			{
				/* Ready to read */
				memset(&recvBuff, 0, sizeof(recvBuff));
				fromlen = sizeof(dgFrom);
				result = recvfrom (server[i].socket, recvBuff, sizeof(recvBuff), 0, &dgFrom, &fromlen);
				/* Reading a socket is very fast */
	
				if (result >= 0)
				{
					char seps[]   = "\\";
					p = recvBuff;

					//discard print
					rLine = GetLine (&p, &result);
					free (rLine);

					msecs = now - server[i].startPing;

					server[i].ping = msecs;
					server[i].responded = 1;

					//serverinfo
					rLine = GetLine (&p, &result);

					/* Establish string and get the first token: */
					token = strtok( rLine, seps );
					while( token != NULL )
					{
						/* While there are tokens in "string" */
						if (!_stricmp (lasttoken, "mapname"))
							strcpy (server[i].szMapName, token);
						else if (!_stricmp (lasttoken, "maxclients"))
							server[i].maxClients = atoi(token);
						else if (!_stricmp (lasttoken, "hostname"))
							strcpy (server[i].szHostName, token);
						else if (!_stricmp (lasttoken, "admin"))
							strcpy (server[i].szAdmin, token);
						else if (!_stricmp (lasttoken, "website"))
							strcpy (server[i].szWebSite, token);
						else if (!_stricmp (lasttoken, "fraglimit"))
							strcpy (server[i].szFragLimit, token);
						else if (!_stricmp (lasttoken, "timelimit"))
							strcpy (server[i].szTimeLimit, token);
						else if (!_stricmp (lasttoken, "version"))
							strcpy (server[i].szVersion, token);


						/* Get next token: */
						strcpy (lasttoken, token);
						token = strtok( NULL, seps );
					}

					free (rLine);

					//clean up server name of any color escape chars
					k = 0;
					for(j=0; j<256; j++) {
						if(server[i].szHostName[j] == '^' && j < strlen( server[j].szHostName ) - 1) {
							if(server[i].szHostName[j+1] != '^')
							j++;
							continue;
						}
						tmptxt[k] = server[i].szHostName[j];
						k++;
					}
					strcpy(server[i].szHostName, tmptxt);

					//playerinfo
					strcpy (seps, " ");
					players = 0;
					while (rLine = GetLine (&p, &result)) {
						/* Establish string and get the first token: */
						token = strtok( rLine, seps);
						server[i].players[players].score = atoi(token);

						token = strtok( NULL, seps);
						server[i].players[players].ping = atoi(token);

						token = strtok( NULL, "\"");
						//token++;

						if (token) {
							strncpy (server[i].players[players].playername, token, sizeof(server[i].players[players].playername)-1);
							//clean up name of any color escape chars
							k = 0;
							for(j=0; j<32; j++) {
								if(server[i].players[players].playername[j] == '^' && j < strlen( server[i].players[players].playername ) - 1) {
									if(server[i].players[players].playername[j+1] != '^')
									j++;
									continue;
								}
								tmptxt[k] = server[i].players[players].playername[j];
								k++;
							}
							strcpy(server[i].players[players].playername, tmptxt);
						}
						else
							server[i].players[players].playername[0] = '\0';

						players++;
						free (rLine);
					}

					server[i].curClients = players;
					totalPlayers += players;
				}

				/* Update progress bar */
				m_refreshprogress->SetPos(numResponses++);

				/* Close socket and remove file descriptor from master list */
				closesocket(server[i].socket);
				FD_CLR(server[i].socket, &masterfds);
			}
		}

		/* Restore read fd list from master (modified by select call) */
		memcpy(&readfds, &masterfds, sizeof(fd_set));
	}

	/* Now check for (and close) any remaining sockets */
	for(i = 0; i < socketcount; i++)
	{
		if(FD_ISSET(server[i].socket, &masterfds))
		{
			closesocket(server[i].socket);  
			m_refreshprogress->SetPos(numResponses++);
		}
	}

	/* Quickly recheck any servers that responded to get the accurate ping */
	for(serverindex = 0; serverindex < numServers; serverindex++)
	{
		if(server[serverindex].responded)
		{
			FD_ZERO(&masterfds);
			memset(&addr, 0, sizeof(addr));
		
			addr.sin_family = AF_INET;
			server[serverindex].socket = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
			
			addr.sin_addr.S_un.S_addr = server[serverindex].ip;
			addr.sin_port = htons(server[serverindex].port);

			timeBeginPeriod (1);
			server[serverindex].startPing = timeGetTime (); /* Time how long it takes to get a response */

			result = sendto (server[serverindex].socket, request, sizeof(request), 0, (struct sockaddr *)&addr, sizeof(addr)); /* 80ms over 40 sockets */
		
			if (result == SOCKET_ERROR)
			{
				AfxMessageBox ("Can't send: Winsock error!");
				/* We will carry on with the queries on the other sockets anyway */
				continue;
			}
			else
			{
				FD_SET(server[serverindex].socket, &masterfds); /* Add socket to master set */
			}

			delay.tv_sec = 1;
			delay.tv_usec = 0;
	
			if((result = select (server[serverindex].socket+1, &masterfds, NULL, NULL, &delay)) > 0) /* Result is number of sockets ready to read */
			{
				server[serverindex].ping = timeGetTime() - server[serverindex].startPing;
			}
			
			/* If server didn't respond second time around, leave the old value */
			timeEndPeriod (1);
			m_refreshprogress->SetPos(numResponses++);
			closesocket(server[serverindex].socket);
		}
	}
	


	m_refreshprogress->SetPos(0);

	/* All done - have a nice day! */
	return;
}

void Open_Winsock()
{
	WSADATA ws;
	
	int error;

	//initialize Winsock
	error = WSAStartup ((WORD)MAKEWORD (1,1), &ws);

	if (error) {
		AfxMessageBox("Couldn't load Winsock!");
	}

	//open a socket for polling the master
	master = socket (AF_INET, SOCK_DGRAM, 0);
	
}

void Close_Winsock()
{

	WSACleanup(); 
}
