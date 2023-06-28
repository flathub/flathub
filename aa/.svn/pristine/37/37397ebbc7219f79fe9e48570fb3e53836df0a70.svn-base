#include "stdafx.h"
#include <windows.h>
#include <winuser.h>
#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <winsock.h>
#include <Mmsystem.h>
#include <string.h>
#include <direct.h>
#include <Wininet.h>
#include <io.h>
#include <iomanip> 
#include <fstream>

using namespace std;

bool CheckVersion(void) {

	//read the version number that is current
	ifstream version;
	HINTERNET hINet, hFile;
	char data[32];
	CHAR buffer[32];

	//get a copy of the latest news
	hINet = InternetOpen("InetURL/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
	if ( !hINet )
	{
		return true; //we don't ever want to attempt to update if we cannot access the net
	}
	hFile = InternetOpenUrl( hINet, "http://cor.planetquake.gamespy.com/codered/files/version.txt", NULL, 0, 0, 0 );
	if(hFile) {
		DWORD dwRead;
		while ( InternetReadFile( hFile, buffer, 31, &dwRead ) )
		{
			if ( dwRead == 0 )
				break;	
			buffer[dwRead] = 0;
		}
		InternetCloseHandle( hFile );
	}
	else {
		InternetCloseHandle(hINet);
		return true; //don't update, there was a problem accessing the net
	}
	InternetCloseHandle( hINet );

	version.open("version.txt");
	if(version.good()) {
		version.getline(data, 32);
	}
	else {
		version.close();
		return true; //never update unless everything is kosher
	}
	version.close();

	//check against what is installed
	if(!strcmp(buffer, data))
		return true;
	else
		return false; //we don't match, so the user has a version that is not current

}