/*
Copyright (C) 2011 COR Entertainment

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

//Account server used for working with Alien Arena and Alien Arena's Statsgen programs

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <malloc.h>
#include <stdio.h>
#include <winsock.h>
#include <time.h>
#else
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#define SOCKET int
#define SOCKET_ERROR -1
#define TIMEVAL struct timeval
#define WSAGetLastError() errno
#define strnicmp strncasecmp
#endif

#include "accountserv.h"

#ifndef _DEBUG
#define printf printf
#else
#define printf printf
#endif

player_t players;
char vString[32];

SYSTEMTIME st;

struct sockaddr_in listenaddress;
SOCKET out;
SOCKET listener;
TIMEVAL delay;
#ifdef WIN32
WSADATA ws;
#endif
fd_set set;
char incoming[150000];
int retval;

void DropPlayer (player_t *player)
{
	if(!player)
		return;

	printf("%s dropped...\n", player->name);

	//unlink
	if (player->next)
		player->next->prev = player->prev;

	if (player->prev)
		player->prev->next = player->next;

	//free
	free (player);
}

//check list for possible expired players(5 hour window)
void CheckPlayers (void)
{
	player_t	*player = &players;
	WORD		curTime;

	while (player->next)
	{
		player = player->next;
		
		GetSystemTime(&st);
		curTime = st.wHour;
		if(player)
		{
			if(player->time)
			{
				if(player->time > curTime)
					curTime += 24;
				if(curTime - player->time > 5)
					DropPlayer(player);
				break;
			}
		}
	}
}

//logout player
void RemovePlayer (char name[64])
{
	player_t	*player = &players;

	if(strlen(name) < 1)
		return;

	//check if this player is already in the list
	while (player->next)
	{
		player = player->next;
		if (!_stricmp(player->name, name))
		{
			DropPlayer(player);
			break;
		}
	}

	DumpValidPlayersToFile();
}

//this called only when a packet of "login" is received, and the player is validated
void AddPlayer (char name[64], char rawname[64])
{
	player_t	*player = &players;

	if(strlen(name) < 1)
		return;

	//check if this player is already in the list
	while (player->next)
	{
		player = player->next;
		if (!_stricmp(player->name, name))
		{
			return;
		}
	}

	player->next = (player_t *)malloc(sizeof(player_s));

	player->next->prev = player;
	player = player->next;

	//copy name
	strncpy_s(player->name, name, 32);
	strncpy_s(player->rawname, rawname, 32);
	player->next = NULL;

	//timestamp
	GetSystemTime(&st);
	player->time = st.wHour;

	printf ("%s added to queue!\n", name);

	//dump all current players to file for statsgen to read
	DumpValidPlayersToFile();

	//check for expired players(those who never sent a logout)
	CheckPlayers();
}

void SendValidationToClient (struct sockaddr_in *from)
{
	int				buflen;
	char			buff[0xFFFF];

	buflen = 0;
	memset (buff, 0, sizeof(buff));

	memcpy (buff, "ÿÿÿÿvalidated", 13);
	buflen += 13;
	
	if ((sendto (listener, buff, buflen, 0, (struct sockaddr *)from, sizeof(*from))) == SOCKET_ERROR)
	{
		printf ("[E] socket error on send! code %d.\n", WSAGetLastError());
	}
}

void SendVStringToClient (char name[64], struct sockaddr_in *from)
{
	int				buflen;
	char			buff[0xFFFF];

	buflen = 0;
	memset (buff, 0, sizeof(buff));

	memcpy (buff, "ÿÿÿÿvstring ", 12);
	buflen += 12;

	//get player's unique string
	ObtainVStringForPlayer(name);

	memcpy (buff + buflen, vString, strlen(vString));
			buflen += strlen(vString);
	
	if ((sendto (listener, buff, buflen, 0, (struct sockaddr *)from, sizeof(*from))) == SOCKET_ERROR)
	{
		printf ("[E] socket error on send! code %d.\n", WSAGetLastError());
	}
}

void ParseResponse (struct sockaddr_in *from, char *data, int dglen)
{
	char *cmd = data;
	char *token;
	char seps[] = "\\";
	char name[128];
	char rawname[128];
	char password[1024];
	char new_password[1024];
	char pVString[128];	

	printf("Processing %s.\n", data);
	
	if (_strnicmp (data, "ÿÿÿÿrequestvstring", 18) == 0)
	{	
		if(strlen(data) > 64)
		{
			printf("[E0] Invalid or malicious request detected from %s\n", inet_ntoa (from->sin_addr));
			return;
		}

		token = strtok( cmd, seps ); 
		if(token)
			token = strtok( NULL, seps ); //protocol - may need this later on
		else 
		{
			printf ("[E1] Invalid command %s from %s!\n", cmd, inet_ntoa (from->sin_addr));
			return;
		}

		if(token)
			token = strtok( NULL, seps );
		else 
		{
			printf ("[E2] Invalid command %s from %s!\n", cmd, inet_ntoa (from->sin_addr));
			return;
		}

		if(token)
		{
			strncpy_s(name, token, 32);
			SendVStringToClient(name, from);
		}
	}
	else if (_strnicmp (data, "ÿÿÿÿlogin", 9) == 0)
	{		
		if(strlen(data) > 364)
		{
			printf("[E0] Invalid or malicious request detected from %s\n", inet_ntoa (from->sin_addr));
			return;
		}

		//parse string, etc validate or create new profile file
		token = strtok( cmd, seps ); 
		if(token)
			token = strtok( NULL, seps ); //protocol - may need this later on
		else 
		{
			printf ("[E1] Invalid command %s from %s!\n", cmd, inet_ntoa (from->sin_addr));
			return;
		}

		if(token)
			token = strtok( NULL, seps );
		else
		{
			printf ("[E2] Invalid command %s from %s!\n", cmd, inet_ntoa (from->sin_addr));
			return;
		}
		if(token)
		{
			strncpy_s(name, token, 32);
			strncpy_s(rawname, token, 32);
			token = strtok( NULL, seps );
		}
		else 
		{
			printf ("[E3] Invalid command %s from %s!\n", cmd, inet_ntoa (from->sin_addr));
			return;
		}

		if(token)
		{
			strncpy_s(password, token, 256);
			token = strtok( NULL, seps );
		}
		else 
		{
			printf ("[E4] Invalid command %s from %s:%s!\n", cmd, name, inet_ntoa (from->sin_addr));
			return;
		}

		if(token)
		{
			strncpy_s(pVString, token, 32);
		}
		else 
		{
			printf ("[E5] Invalid command %s from %s:%s!\n", cmd, name, inet_ntoa (from->sin_addr));
			return;
		}

		if(ValidatePlayer(name, password, pVString))
		{
			printf("Validated name\n");
			AddPlayer(name, rawname);

			//let the client know he was validated
			SendValidationToClient (from);
		}
	}
	else if (_strnicmp (data, "ÿÿÿÿlogout", 10) == 0)
	{
		if(strlen(data) > 364)
		{
			printf("[E0] Invalid or malicious request detected from %s\n", inet_ntoa (from->sin_addr));
			return;
		}

		//parse string, etc validate or create new profile file
		token = strtok( cmd, seps );
		if(token)
			token = strtok( NULL, seps ); //protocol - may need this later on
		else 
		{
			printf ("[E1] Invalid command %s from %s!\n", cmd, inet_ntoa (from->sin_addr));
			return;
		}

		if(token)
			token = strtok( NULL, seps );
		else 
		{
			printf ("[E2] Invalid command %s from %s!\n", cmd, inet_ntoa (from->sin_addr));
			return;
		}

		if(token)
		{
			strncpy_s(name, token, 32);
			token = strtok( NULL, seps );
		}
		else 
		{
			printf ("[E3] Invalid command %s from %s!\n", cmd, inet_ntoa (from->sin_addr));
			return;
		}

		if(token)
		{
			strncpy_s(password, token, 256);
			token = strtok( NULL, seps );
		}
		else 
		{
			printf ("[E4] Invalid command %s from %s:%s!\n", cmd, name, inet_ntoa (from->sin_addr));
			return;
		}

		if(token)
		{
			strncpy_s(pVString, token, 32);
		}
		else 
		{
			printf ("[E5] Invalid command %s from %s:%s!\n", cmd, name, inet_ntoa (from->sin_addr));
			return;
		}

		if(ValidatePlayer(name, password, pVString))
		{
			RemovePlayer(name);
		}
	}
	else if (_strnicmp (data, "ÿÿÿÿchangepw", 12) == 0)
	{	

		if(strlen(data) > 620)
		{
			printf("[E0] Invalid or malicious request detected from %s\n", inet_ntoa (from->sin_addr));
			return;
		}

		//parse string, etc validate or create new profile file
		token = strtok( cmd, seps ); 
		if(token)
			token = strtok( NULL, seps ); //protocol - may need this later on
		else 
		{
			printf ("[E1] Invalid command %s from %s!\n", cmd, inet_ntoa (from->sin_addr));
			return;
		}

		if(token)
			token = strtok( NULL, seps );
		else
		{
			printf ("[E2] Invalid command %s from %s!\n", cmd, inet_ntoa (from->sin_addr));
			return;
		}
		if(token)
		{
			strncpy_s(name, token, 32);
			token = strtok( NULL, seps );
		}
		else 
		{
			printf ("[E3] Invalid command %s from %s!\n", cmd, inet_ntoa (from->sin_addr));
			return;
		}

		if(token)
		{
			strncpy_s(password, token, 256);
			token = strtok( NULL, seps );
		}
		else 
		{
			printf ("[E4] Invalid command %s from %s:%s!\n", cmd, name, inet_ntoa (from->sin_addr));
			return;
		}

		if(token)
		{
			strncpy_s(new_password, token, 256);
			token = strtok( NULL, seps );
		}
		else 
		{
			printf ("[E5] Invalid command %s from %s:%s!\n", cmd, name, inet_ntoa (from->sin_addr));
			return;
		}

		if(token)
		{
			strncpy_s(pVString, token, 32);
		}
		else 
		{
			printf ("[E6] Invalid command %s from %s:%s!\n", cmd, name, inet_ntoa (from->sin_addr));
			return;
		}

		if(!strcmp(password, "password")) //a player either starting the first time, or moving to a new system
		{
			if(ValidatePlayer(name, new_password, pVString))
			{
				ChangePlayerPassword(name, new_password, pVString);

				//let the client know he was validated
				SendValidationToClient (from);
			}
		}
		else
		{
			if(ValidatePlayer(name, password, pVString))
			{
				ChangePlayerPassword(name, new_password, pVString);

				//let the client know he was validated
				SendValidationToClient (from);
			}
		}
	}
	else
	{
		printf ("[E] Unknown command %s from %s!\n", cmd, inet_ntoa (from->sin_addr));
	}
}

int main (int argc, char argv[])
{
	int len;
	int fromlen;
	struct sockaddr_in from;
		
	printf ("Alien Arena Account Server 0.01\n(c) 2011 COR Entertainment\n\n");

#ifdef WIN32
	WSAStartup ((WORD)MAKEWORD (1,1), &ws);
#endif

	listener = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	out = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	memset (&listenaddress, 0, sizeof(listenaddress));

	listenaddress.sin_family = AF_INET;
	listenaddress.sin_port = htons(27902);
	listenaddress.sin_addr.s_addr = INADDR_ANY; 

	if ((bind (listener, (struct sockaddr *)&listenaddress, sizeof(listenaddress))) == SOCKET_ERROR)
	{
		printf ("[E] Couldn't bind to port 27902 UDP (something is probably using it)\n");
		return 1;
	}

	delay.tv_sec = 1;
	delay.tv_usec = 0;

	FD_SET (listener, &set);

	fromlen = sizeof(from);

	memset (&players, 0, sizeof(players));

	printf ("listening on port 27902 (UDP)\n");

	while (1)
	{
		FD_SET (listener, &set);
		delay.tv_sec = 1;
		delay.tv_usec = 0;
				
		retval = select(listener+1, &set, NULL, NULL, &delay);
		if (retval == 1)
		{
			len = recvfrom (listener, incoming, sizeof(incoming), 0, (struct sockaddr *)&from, &fromlen);
			if (len != SOCKET_ERROR)
			{
				if (len > 4)
				{
					//parse this packet
					ParseResponse (&from, incoming, len);
				}
				else
				{
					printf ("[W] runt packet from %s:%d\n", inet_ntoa (from.sin_addr), ntohs(from.sin_port));
				}

				//reset for next packet
				memset (incoming, 0, sizeof(incoming));
			} else {
				printf ("[E] socket error during select from %s:%d (%d)\n", inet_ntoa (from.sin_addr), ntohs(from.sin_port), WSAGetLastError());
			}
		}
	}
}
