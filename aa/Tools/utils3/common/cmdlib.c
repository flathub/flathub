// cmdlib.c

#ifdef WIN32
#include <windows.h>
#include <crtdbg.h>
#endif

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#include "cmdlib.h"
#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
#include <direct.h>
#endif

#ifdef NeXT
#include <libc.h>
#endif

#ifdef __unix__
#include <unistd.h>
#endif

#define PATHSEPERATOR   '/'

// set these before calling CheckParm
int myargc;
char **myargv;

char		com_token[1024];
qboolean	com_eof;

qboolean		archive;
char			archivedir[1024];

/*
===================
ExpandWildcards

Mimic unix command line expansion
===================
*/
#define	MAX_EX_ARGC	1024
int		ex_argc;
char	*ex_argv[MAX_EX_ARGC];
#ifdef _WIN32
#include "io.h"
void ExpandWildcards (int *argc, char ***argv)
{
	struct _finddata_t fileinfo;
	int		handle;
	int		i;
	char	filename[1024];
	char	filebase[1024];
	char	*path;

	ex_argc = 0;
	for (i=0 ; i<*argc ; i++)
	{
		path = (*argv)[i];
		if ( path[0] == '-'
			|| ( !strstr(path, "*") && !strstr(path, "?") ) )
		{
			ex_argv[ex_argc++] = path;
			continue;
		}

		handle = _findfirst (path, &fileinfo);
		if (handle == -1)
			return;

		ExtractFilePath (path, filebase);

		do
		{
			sprintf (filename, "%s%s", filebase, fileinfo.name);
			ex_argv[ex_argc++] = copystring (filename);
		} while (_findnext( handle, &fileinfo ) != -1);

		_findclose (handle);
	}

	*argc = ex_argc;
	*argv = ex_argv;
}
#else
void ExpandWildcards (int *argc, char ***argv)
{
}
#endif

#ifdef WIN_ERROR
#include <windows.h>
/*
=================
Error

For abnormal program terminations in windowed apps
=================
*/
void Error (char *error, ...)
{
	va_list argptr;
	char	text[1024];
	char	text2[1024];
	int		err;

	err = GetLastError ();

	va_start (argptr,error);
	vsprintf (text, error,argptr);
	va_end (argptr);

	sprintf (text2, "%s\nGetLastError() = %i", text, err);
    MessageBox(NULL, text2, "Error", 0 /* MB_OK */ );

	exit (1);
}

#else
/*
=================
Error

For abnormal program terminations in console apps
=================
*/
void Error (char *error, ...)
{
	va_list argptr;

	printf ("\n************ ERROR ************\n");

	va_start (argptr,error);
	vprintf (error,argptr);
	va_end (argptr);
	printf ("\n");

	exit (1);
}
#endif

// only printf if in verbose mode
qboolean verbose = false;
void qprintf (char *format, ...)
{
	va_list argptr;

	if (!verbose)
		return;

	va_start (argptr,format);
	vprintf (format,argptr);
	va_end (argptr);
	fflush( stdout );
}

/*
 * Path determination
 *
 * assumes
 *   moddir is parent of whatever directory contains the .map/.bsp
 *   gamedir is parent of moddir
 *   qdir is parent of gamedir
 */
char qdir[1024] = "";
char gamedir[1024] = "" ;
char moddir[1024] = "";

void SetQdirFromPath( char *path )
{
	char cwd_map[512];
	char *lastslash;
	
	memset (cwd_map, 0, sizeof(cwd_map));
	strncpy (cwd_map, path, sizeof(cwd_map)-1);
	
#ifdef WIN32
	lastslash = strrchr (cwd_map, '\\');
	// I believe Win32 can use either slash type. -Max
	if (lastslash == NULL)
#endif
	lastslash = strrchr (cwd_map, '/');

	if (lastslash == NULL)
	{
		// no slashes: CWD
		Q_getwd( cwd_map ); // appends path separator
	}
	else
	{
		// get rid of everything after last path separator
		lastslash++;
		*lastslash = '\0';
	}

	strcpy( moddir, cwd_map );
#ifdef WIN32
	strcat( moddir, "..\\");
#else
	strcat( moddir, "../");
#endif

	strcpy( gamedir, moddir );
#ifdef WIN32
	strcat( gamedir, "..\\");
#else
	strcat( gamedir, "../");
#endif

	strcpy( qdir, gamedir );
#ifdef WIN32
	strcat( qdir, "..\\");
#else
	strcat( qdir, "../");
#endif

}

char *ExpandArg (char *path)
{
	static char full[1024];

	if (path[0] != '/' && path[0] != '\\' && path[1] != ':')
	{
		Q_getwd (full);
		strcat (full, path);
	}
	else
		strcpy (full, path);
	return full;
}

char *ExpandPath (char *path)
{
	static char full[1024];
	if (!qdir[0])
		Error ("ExpandPath called without qdir set");
	if (path[0] == '/' || path[0] == '\\' || path[1] == ':')
		return path;
	sprintf (full, "%s%s", qdir, path);
	return full;
}

char *ExpandPathAndArchive (char *path)
{
	char	*expanded;
	char	archivename[1024];

	expanded = ExpandPath (path);

	if (archive)
	{
		sprintf (archivename, "%s/%s", archivedir, path);
		QCopyFile (expanded, archivename);
	}
	return expanded;
}


char *copystring(char *s)
{
	char	*b;
	b = malloc(strlen(s)+1);
	strcpy (b, s);
	return b;
}



/*
================
I_FloatTime
================
*/
double I_FloatTime (void)
{
	time_t	t;

	time (&t);

	return t;
#if 0
// more precise, less portable
	struct timeval tp;
	struct timezone tzp;
	static int		secbase;

	gettimeofday(&tp, &tzp);

	if (!secbase)
	{
		secbase = tp.tv_sec;
		return tp.tv_usec/1000000.0;
	}

	return (tp.tv_sec - secbase) + tp.tv_usec/1000000.0;
#endif
}

static char *rslt;
void Q_getwd (char *out)
{
#ifdef WIN32
   _getcwd (out, 256);
   strcat (out, "\\");
#else
   rslt = getcwd (out, 256);
   strcat (out, "/");
#endif
}


void Q_mkdir (char *path)
{
#ifdef WIN32
	if (_mkdir (path) != -1)
		return;
#else
	if (mkdir (path, 0777) != -1)
		return;
#endif
	if (errno != EEXIST)
		Error ("mkdir %s: %s",path, strerror(errno));
}

/*
============
FileTime

returns -1 if not present
============
*/
int	FileTime (char *path)
{
	struct	stat	buf;

	if (stat (path,&buf) == -1)
		return -1;

	return buf.st_mtime;
}



/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char *COM_Parse (char **data_p)
{
	int		c;
	int		len;
	char	*data;

	data = *data_p;
	len = 0;
	com_token[0] = 0;

	if (!data)
	{
		*data_p = NULL;
		return "";
	}

// skip whitespace
skipwhite:
	while ( (c = *data) <= ' ')
	{
		if (c == 0)
		{
			*data_p = NULL;
			return "";
		}
		data++;
	}

// skip // comments
	if (c=='/' && data[1] == '/')
	{
		while (*data && *data != '\n')
			data++;
		goto skipwhite;
	}

// handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				com_token[len] = 0;
				*data_p = data;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS)
			{
				com_token[len] = c;
				len++;
			}
		}
	}

// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS)
		{
			com_token[len] = c;
			len++;
		}
		data++;
		c = *data;
	} while (c>32);

	if (len == MAX_TOKEN_CHARS)
	{
//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	com_token[len] = 0;

	*data_p = data;
	return com_token;
}


int Q_strncasecmp (char *s1, char *s2, int n)
{
	int		c1, c2;

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
			return 0;		// strings are equal until end point

		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');
			if (c1 != c2)
				return -1;		// strings not equal
		}
	} while (c1);

	return 0;		// strings are equal
}

int Q_strcasecmp (char *s1, char *s2)
{
	return Q_strncasecmp (s1, s2, 99999);
}

#if !defined WIN32
char *strupr (char *start)
{
	char	*in;
	in = start;
	while (*in)
	{
		*in = toupper(*in);
		in++;
	}
	return start;
}
#endif

char *strlower (char *start)
{
	char	*in;
	in = start;
	while (*in)
	{
		*in = tolower(*in);
		in++;
	}
	return start;
}


/*
=============================================================================

						MISC FUNCTIONS

=============================================================================
*/


/*
=================
CheckParm

Checks for the given parameter in the program's command line arguments
Returns the argument number (1 to argc-1) or 0 if not present
=================
*/
int CheckParm (char *check)
{
	int             i;

	for (i = 1;i<myargc;i++)
	{
		if ( !Q_strcasecmp(check, myargv[i]) )
			return i;
	}

	return 0;
}




/*
================
Q_filelength
================
*/
int Q_filelength (FILE *f)
{
	int		pos;
	int		end;

	pos = ftell (f);
	fseek (f, 0, SEEK_END);
	end = ftell (f);
	fseek (f, pos, SEEK_SET);

	return end;
}


FILE *SafeOpenWrite (char *filename)
{
	FILE	*f;

	f = fopen(filename, "wb");

	if (!f)
		Error ("Error opening %s: %s",filename,strerror(errno));

	return f;
}

FILE *SafeOpenRead (char *filename)
{
	FILE	*f;

	f = fopen(filename, "rb");

	if (!f)
		Error ("Error opening %s: %s",filename,strerror(errno));

	return f;
}


void SafeRead (FILE *f, void *buffer, int count)
{
	if ( fread (buffer, 1, count, f) != (size_t)count)
		Error ("File read failure");
}


void SafeWrite (FILE *f, void *buffer, int count)
{
	if (fwrite (buffer, 1, count, f) != (size_t)count)
		Error ("File write failure");
}


/*
==============
FileExists
==============
*/
qboolean	FileExists (char *filename)
{
	FILE	*f;

	f = fopen (filename, "r");
	if (!f)
		return false;
	fclose (f);
	return true;
}

/*
==============
LoadFile
==============
*/
int    LoadFile (char *filename, void **bufferptr)
{
	FILE	*f;
	int    length;
	void    *buffer;

	f = SafeOpenRead (filename);
	length = Q_filelength (f);
	buffer = malloc (length+1);
	((char *)buffer)[length] = 0;
	SafeRead (f, buffer, length);
	fclose (f);

	*bufferptr = buffer;
	return length;
}


/*
==============
TryLoadFile

Allows failure
==============
*/
int    TryLoadFile (char *filename, void **bufferptr, int print_error)
{
	FILE	*f;
	int    length;
	void    *buffer;

	*bufferptr = NULL;


	f = fopen (filename, "rb");

	if (!f)
        {
        if(print_error)
            printf("  File %s failed to open\n", filename);

		return -1;
        }

	length = Q_filelength (f);
	buffer = malloc (length+1);
	((char *)buffer)[length] = 0;
	SafeRead (f, buffer, length);
	fclose (f);

	*bufferptr = buffer;
	return length;
}


/*
==============
TryLoadFileFromPak

Allows failure
==============
*/
typedef struct
{
  unsigned char magic[4];      // Name of the new WAD format
  long diroffset;               // Position of WAD directory from start of file
  long dirsize;                 // Number of entries * 0x40 (64 char)

  unsigned char bogus[50];
} pakheader_t;

typedef struct
{
  unsigned char filename[0x38];        // Name of the file, Unix style, with extension,
                                // 56 chars, padded with '\0'.
  long offset;                  // Position of the entry in PACK file
  long size;                    // Size of the entry in PACK file
} pakentry_t;

int    TryLoadFileFromPak (char *filename, void **bufferptr, char *gd)
    {
    FILE	*f;
	int     n, i, ret_len;
    long    dir_ents;
	void    *buffer;
    pakheader_t pak_header;
    pakentry_t *pak_entry;
    char file[1024];

    for(n = 0; filename[n] != 0; n++)
        {
        if(filename[n] == '\\')
            filename[n] = '/';
        }

	*bufferptr = NULL;

    ret_len = -1;

    for(n = 0;; n++)
        {
        if(ret_len != -1)
            break;

        sprintf(file, "%spak%d.pak", gd, n);

        f = fopen (file, "rb");

        if(!f)
            {
            if(errno == ENOENT)
                return -1;

            continue;
            }

        SafeRead(f, &pak_header, sizeof(pakheader_t));

        dir_ents = pak_header.dirsize / sizeof(pakentry_t);

        pak_entry = malloc(pak_header.dirsize);

        if(pak_entry != NULL)
            {
            if(!fseek(f, pak_header.diroffset, SEEK_SET))
                {
                memset(pak_entry, 0, pak_header.dirsize);

                SafeRead(f, pak_entry, pak_header.dirsize);

                for(i = 0; i < dir_ents; i++)
                    {
                    for(n = 0; pak_entry[i].filename[n] != 0; n++)
                        {
                        if(pak_entry[i].filename[n] == '\\')
                            pak_entry[i].filename[n] = '/';
                        }

                    if(!Q_strncasecmp((char*)pak_entry[i].filename, filename, 56))
                        {
                        if(!fseek(f, pak_entry[i].offset, SEEK_SET))
                            {
                            buffer = malloc (pak_entry[i].size+1);
	                        ((char *)buffer)[pak_entry[i].size] = 0;
	                        SafeRead (f, buffer, pak_entry[i].size);
	                        *bufferptr = buffer;

                            ret_len = pak_entry[i].size;
                            }

                        break;
                        }
                    }
                }
            }

        if(pak_entry != NULL)
            free(pak_entry);

        fclose(f);
        }

    return ret_len;
}
/*
*/
/*
==============
SaveFile
==============
*/
void    SaveFile (char *filename, void *buffer, int count)
{
	FILE	*f;

	f = SafeOpenWrite (filename);
	SafeWrite (f, buffer, count);
	fclose (f);
}



void DefaultExtension (char *path, char *extension)
{
	char    *src;
//
// if path doesnt have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen(path) - 1;

	while (*src != PATHSEPERATOR && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	strcat (path, extension);
}


void DefaultPath (char *path, char *basepath)
{
	char    temp[128];

	if (path[0] == PATHSEPERATOR)
		return;                   // absolute path location
	strcpy (temp,path);
	strcpy (path,basepath);
	strcat (path,temp);
}


void    StripFilename (char *path)
{
	int             length;

	length = strlen(path)-1;
	while (length > 0 && path[length] != PATHSEPERATOR)
		length--;
	path[length] = 0;
}

void    StripExtension (char *path)
{
	int             length;

	length = strlen(path)-1;
	while (length > 0 && path[length] != '.')
	{
		length--;
		if (path[length] == '/')
			return;		// no extension
	}
	if (length)
		path[length] = 0;
}


/*
====================
Extract file parts
====================
*/
// FIXME: should include the slash, otherwise
// backing to an empty path will be wrong when appending a slash
void ExtractFilePath (char *path, char *dest)
{
	char    *src;

	src = path + strlen(path) - 1;

//
// back up until a \ or the start
//
	while (src != path && *(src-1) != '\\' && *(src-1) != '/')
		src--;

	memcpy (dest, path, src-path);
	dest[src-path] = 0;
}

void ExtractFileBase (char *path, char *dest)
{
	char    *src;

	src = path + strlen(path) - 1;

//
// back up until a \ or the start
//
	while (src != path && *(src-1) != PATHSEPERATOR)
		src--;

	while (*src && *src != '.')
	{
		*dest++ = *src++;
	}
	*dest = 0;
}

void ExtractFileExtension (char *path, char *dest)
{
	char    *src;

	src = path + strlen(path) - 1;

//
// back up until a . or the start
//
	while (src != path && *(src-1) != '.')
		src--;
	if (src == path)
	{
		*dest = 0;	// no extension
		return;
	}

	strcpy (dest,src);
}


/*
==============
ParseNum / ParseHex
==============
*/
int ParseHex (char *hex)
{
	char    *str;
	int    num;

	num = 0;
	str = hex;

	while (*str)
	{
		num <<= 4;
		if (*str >= '0' && *str <= '9')
			num += *str-'0';
		else if (*str >= 'a' && *str <= 'f')
			num += 10 + *str-'a';
		else if (*str >= 'A' && *str <= 'F')
			num += 10 + *str-'A';
		else
			Error ("Bad hex number: %s",hex);
		str++;
	}

	return num;
}


int ParseNum (char *str)
{
	if (str[0] == '$')
		return ParseHex (str+1);
	if (str[0] == '0' && str[1] == 'x')
		return ParseHex (str+2);
	return atol (str);
}



/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

#ifdef _SGI_SOURCE
#define	__BIG_ENDIAN__
#endif

#ifdef __BIG_ENDIAN__

short   LittleShort (short l)
{
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short   BigShort (short l)
{
	return l;
}


int    LittleLong (int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int    BigLong (int l)
{
	return l;
}


float	LittleFloat (float l)
{
	union {byte b[4]; float f;} in, out;

	in.f = l;
	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];

	return out.f;
}

float	BigFloat (float l)
{
	return l;
}


#else


short   BigShort (short l)
{
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short   LittleShort (short l)
{
	return l;
}


int    BigLong (int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int    LittleLong (int l)
{
	return l;
}

float	BigFloat (float l)
{
	union {byte b[4]; float f;} in, out;

	in.f = l;
	out.b[0] = in.b[3];
	out.b[1] = in.b[2];
	out.b[2] = in.b[1];
	out.b[3] = in.b[0];

	return out.f;
}

float	LittleFloat (float l)
{
	return l;
}


#endif


//=======================================================


// FIXME: byte swap?

// this is a 16 bit, non-reflected CRC using the polynomial 0x1021
// and the initial and final xor values shown below...  in other words, the
// CCITT standard CRC used by XMODEM

#define CRC_INIT_VALUE	0xffff
#define CRC_XOR_VALUE	0x0000

static unsigned short crctable[256] =
{
	0x0000,	0x1021,	0x2042,	0x3063,	0x4084,	0x50a5,	0x60c6,	0x70e7,
	0x8108,	0x9129,	0xa14a,	0xb16b,	0xc18c,	0xd1ad,	0xe1ce,	0xf1ef,
	0x1231,	0x0210,	0x3273,	0x2252,	0x52b5,	0x4294,	0x72f7,	0x62d6,
	0x9339,	0x8318,	0xb37b,	0xa35a,	0xd3bd,	0xc39c,	0xf3ff,	0xe3de,
	0x2462,	0x3443,	0x0420,	0x1401,	0x64e6,	0x74c7,	0x44a4,	0x5485,
	0xa56a,	0xb54b,	0x8528,	0x9509,	0xe5ee,	0xf5cf,	0xc5ac,	0xd58d,
	0x3653,	0x2672,	0x1611,	0x0630,	0x76d7,	0x66f6,	0x5695,	0x46b4,
	0xb75b,	0xa77a,	0x9719,	0x8738,	0xf7df,	0xe7fe,	0xd79d,	0xc7bc,
	0x48c4,	0x58e5,	0x6886,	0x78a7,	0x0840,	0x1861,	0x2802,	0x3823,
	0xc9cc,	0xd9ed,	0xe98e,	0xf9af,	0x8948,	0x9969,	0xa90a,	0xb92b,
	0x5af5,	0x4ad4,	0x7ab7,	0x6a96,	0x1a71,	0x0a50,	0x3a33,	0x2a12,
	0xdbfd,	0xcbdc,	0xfbbf,	0xeb9e,	0x9b79,	0x8b58,	0xbb3b,	0xab1a,
	0x6ca6,	0x7c87,	0x4ce4,	0x5cc5,	0x2c22,	0x3c03,	0x0c60,	0x1c41,
	0xedae,	0xfd8f,	0xcdec,	0xddcd,	0xad2a,	0xbd0b,	0x8d68,	0x9d49,
	0x7e97,	0x6eb6,	0x5ed5,	0x4ef4,	0x3e13,	0x2e32,	0x1e51,	0x0e70,
	0xff9f,	0xefbe,	0xdfdd,	0xcffc,	0xbf1b,	0xaf3a,	0x9f59,	0x8f78,
	0x9188,	0x81a9,	0xb1ca,	0xa1eb,	0xd10c,	0xc12d,	0xf14e,	0xe16f,
	0x1080,	0x00a1,	0x30c2,	0x20e3,	0x5004,	0x4025,	0x7046,	0x6067,
	0x83b9,	0x9398,	0xa3fb,	0xb3da,	0xc33d,	0xd31c,	0xe37f,	0xf35e,
	0x02b1,	0x1290,	0x22f3,	0x32d2,	0x4235,	0x5214,	0x6277,	0x7256,
	0xb5ea,	0xa5cb,	0x95a8,	0x8589,	0xf56e,	0xe54f,	0xd52c,	0xc50d,
	0x34e2,	0x24c3,	0x14a0,	0x0481,	0x7466,	0x6447,	0x5424,	0x4405,
	0xa7db,	0xb7fa,	0x8799,	0x97b8,	0xe75f,	0xf77e,	0xc71d,	0xd73c,
	0x26d3,	0x36f2,	0x0691,	0x16b0,	0x6657,	0x7676,	0x4615,	0x5634,
	0xd94c,	0xc96d,	0xf90e,	0xe92f,	0x99c8,	0x89e9,	0xb98a,	0xa9ab,
	0x5844,	0x4865,	0x7806,	0x6827,	0x18c0,	0x08e1,	0x3882,	0x28a3,
	0xcb7d,	0xdb5c,	0xeb3f,	0xfb1e,	0x8bf9,	0x9bd8,	0xabbb,	0xbb9a,
	0x4a75,	0x5a54,	0x6a37,	0x7a16,	0x0af1,	0x1ad0,	0x2ab3,	0x3a92,
	0xfd2e,	0xed0f,	0xdd6c,	0xcd4d,	0xbdaa,	0xad8b,	0x9de8,	0x8dc9,
	0x7c26,	0x6c07,	0x5c64,	0x4c45,	0x3ca2,	0x2c83,	0x1ce0,	0x0cc1,
	0xef1f,	0xff3e,	0xcf5d,	0xdf7c,	0xaf9b,	0xbfba,	0x8fd9,	0x9ff8,
	0x6e17,	0x7e36,	0x4e55,	0x5e74,	0x2e93,	0x3eb2,	0x0ed1,	0x1ef0
};

void CRC_Init(unsigned short *crcvalue)
{
	*crcvalue = CRC_INIT_VALUE;
}

void CRC_ProcessByte(unsigned short *crcvalue, byte data)
{
	*crcvalue = (*crcvalue << 8) ^ crctable[(*crcvalue >> 8) ^ data];
}

unsigned short CRC_Value(unsigned short crcvalue)
{
	return crcvalue ^ CRC_XOR_VALUE;
}
//=============================================================================

/*
============
CreatePath
============
*/
void	CreatePath (char *path)
{
	char	*ofs, c;

	if (path[1] == ':')
		path += 2;

	for (ofs = path+1 ; *ofs ; ofs++)
	{
		c = *ofs;
		if (c == '/' || c == '\\')
		{	// create the directory
			*ofs = 0;
			Q_mkdir (path);
			*ofs = c;
		}
	}
}


/*
============
QCopyFile

  Used to archive source files
============
*/
void QCopyFile (char *from, char *to)
{
	void	*buffer;
	int		length;

	length = LoadFile (from, &buffer);
	CreatePath (to);
	SaveFile (to, buffer, length);
	free (buffer);
}


