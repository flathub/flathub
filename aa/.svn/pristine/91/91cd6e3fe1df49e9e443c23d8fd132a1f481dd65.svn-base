#include  <ctype.h>
#include  <string.h>
#include  <iostream>
#include  <fstream>
#include  <time.h>
#include  <Windows.h>

#include "accountserv.h"

using namespace std;

extern SYSTEMTIME st;

char RandomChar()
{   
	return (char)((rand() % 78) + 30);
}

void StripIllegalPathChars( char *name )
{
	int i = 0;

	while(name[i])
	{
		if(name[i] == ':' || name[i] == '*' || name[i] == '"' || name[i] == '/' ||
			name[i] == '?' || name[i] == '\\' || name[i] == '|' || name[i] == '<' ||
			name[i] == '>')
			name[i] = ' ';
		i++;
	} 
}

void ObtainVStringForPlayer(char name[64])
{
	ifstream playerProfileFile;
	char szPath[256];
	char szTmp[256];
	int i;

	StripIllegalPathChars(name);

	//look for existing account
	sprintf(szPath, "playerprofiles/%s", name);

	//open file
	playerProfileFile.open(szPath);

	if(!playerProfileFile)
	{
		//create randomstring
		for(i = 0; i < 32; i++)
		{
			vString[i] = RandomChar();
		}

		vString[31] = 0;
	}
	else
	{
		playerProfileFile.getline(szTmp, 256);
		playerProfileFile.getline(vString, 32);
		playerProfileFile.close();
	}

}

bool ValidatePlayer(char name[64], char password[512], char pVString[64])
{
	ifstream playerProfileFile;
	ofstream newPlayerProfileFile;
	char szPath[256];
	char svPass[256];
	char svTime[32];

	StripIllegalPathChars(name);

	sprintf(szPath, "playerprofiles/%s", name);
	
	//open file
	playerProfileFile.open(szPath);

	//if no file, create one and return true
	if(!playerProfileFile)
	{
		printf("Creating new profile for %s\n", name);

		GetSystemTime(&st);
		sprintf(svTime, "%i-%i-%i-%i", st.wYear, st.wMonth, st.wDay, st.wHour);
		newPlayerProfileFile.open(szPath);
		newPlayerProfileFile << password << endl;
		newPlayerProfileFile << pVString << endl;
		newPlayerProfileFile << svTime << endl;
		newPlayerProfileFile.close();

		return true;
	}
	else
	{
		printf("Reading profile for %s\n", name);

		playerProfileFile.getline(svPass, 256);
		playerProfileFile.close();

		if(!_stricmp(svPass, password))
		{
			//matched!
			return true;
		}
		else
		{
			printf("[A]Invalid password for %s!\n", name);
			printf("Was expecting %s and got %s\n", svPass, password);
			return false;
		}
	}	

	return false;
}

void ChangePlayerPassword(char name[64], char new_password[512], char pVString[64])
{
	ofstream playerProfileFile;
	char szPath[256];
	char svTime[32];	
	
	printf("Changing password for %s\n", name);

	StripIllegalPathChars(name);

	sprintf(szPath, "playerprofiles/%s", name);
	
	remove(szPath);

	GetSystemTime(&st);
	sprintf(svTime, "%i-%i-%i-%i", st.wYear, st.wMonth, st.wDay, st.wHour);
	playerProfileFile.open(szPath);
	playerProfileFile << new_password << endl;
	playerProfileFile << pVString << endl;
	playerProfileFile << svTime << endl;
	playerProfileFile.close();
}

void DumpValidPlayersToFile(void)
{
	ofstream	currPlayers;
	player_t	*player = &players;

	currPlayers.open("validated", ios::out | ios::trunc);

	while (player->next)
	{
		player = player->next;
		currPlayers << player->rawname << endl;		
	}
	currPlayers.close();
}
