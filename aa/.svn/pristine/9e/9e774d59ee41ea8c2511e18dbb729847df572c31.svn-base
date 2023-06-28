/// Funciones varias /////////////
#include "functions.h";
#include "stdio.h";
#include "string.h";
#include "userinfo.h"



void analizeText(char text[3000])
{
	int ichar =0;
	char line[3000];
	memset(line,0,3000);
	for (int t=0;t<strlen(text);t++)
	{
		line[ichar]= text[t];
		ichar++;
		if(text[t]==13) {
			ichar=0;
			analizeLine(line);
			memset(line,0,3000);
		};
	}
}

