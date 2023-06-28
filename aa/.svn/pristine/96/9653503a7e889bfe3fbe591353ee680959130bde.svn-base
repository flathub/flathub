
#include "cmdlib.h"
#include "mathlib.h"
#include "bspfile.h"

int main (int argc, char **argv)
{
	int			i;
	char		source[1024];
	int			size;
	FILE		*f;

	if (argc == 1)
		Error ("usage: bspinfo bspfile [bspfiles]");

	for (i=1 ; i<argc ; i++)
	{
		printf ("---------------------\n");
		strcpy (source, argv[i]);
		DefaultExtension (source, ".bsp");
		f = fopen (source, "rb");
		if (f)
		{
			size = Q_filelength (f);
			fclose (f);
		}
		else
			size = 0;
		printf ("%s: %i\n", source, size);

		LoadBSPFile (source);
		PrintBSPFileSizes ();
		printf ("---------------------\n");
	}

	return 0;
}
