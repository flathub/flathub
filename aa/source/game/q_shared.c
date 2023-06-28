/*
Copyright (C) 1997-2001 Id Software, Inc.

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

#include <float.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "q_shared.h"

vec3_t vec3_origin = {0.0f,0.0f,0.0f};

//============================================================================

/**
 * Rotates a point (or vector) around an arbitrary axis by the given angle.
 * The axis ('dir') must be a unit vector.
 *
 * 2014-11 Note: Called only from r_main::R_SetFrustum().  Not
 * efficient for transforming multiple points by same axis and angle,
 * but that is not needed.
 */
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees )
{
	float rot[3][3];
	float angle;
	float c, cc, s; /* cos, 1-cos, sin */
	float ax, ay, az;
	float ccaxay, ccaxaz, ccayaz;
	float sax, say, saz;
	float tmp;
	int   i;

	angle = DEG2RAD(degrees);
	c  = cosf(angle);
	s  = sinf(angle);
	cc = 1.0f - c;

	/* axis unit vector */
	ax = dir[0];
	ay = dir[1];
	az = dir[2];

	/* rotation matrix sub-components */
	sax = s * ax;
	say = s * ay;
	saz = s * az;
	tmp = cc * ax;
	ccaxay = tmp * ay;
	ccaxaz = tmp * az;
	ccayaz = cc * ay * az;

	/* rotation matrix */
	rot[0][0] = c + (cc * ax * ax);
	rot[0][1] = ccaxay + saz;
	rot[0][2] = ccaxaz - say;

	rot[1][0] = ccaxay - saz;
	rot[1][1] = c + (cc * ay * ay);
	rot[1][2] = ccayaz + sax;

	rot[2][0] = ccaxaz + say;
	rot[2][1] = ccayaz - sax;
	rot[2][2] = c + (cc * az * az);

	/* rotate */
	for ( i = 0; i < 3; i++ )
		dst[i] = rot[i][0] * point[0] + rot[i][1] * point[1] + rot[i][2] * point[2];

}

/**
 * Transforms vec3 of <pitch, yaw, roll> angles to forward, right, and
 * up vectors. Arguments forward, right and up may be NULL if only a subset
 * of the outputs is needed.
 */
void AngleVectors (const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up)
{
	float		angle;
	float		sr, sp, sy, cr, cp, cy;

	angle = DEG2RAD(angles[YAW]);
	sy = sinf( angle );
	cy = cosf( angle );
	angle = DEG2RAD(angles[PITCH]);
	sp = sinf( angle );
	cp = cosf( angle );
	if (forward)
	{
		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;
	}
	if( right || up )
	{
		angle = DEG2RAD(angles[ROLL]);
		sr = sinf( angle );
		cr = cosf( angle );
		if (right)
		{
			right[0] = -(sr * sp * cy) + (cr * sy);
			right[1] = -(sr * sp * sy) - (cr * cy);
			right[2] = -(sr * cp);
		}
		if (up)
		{
			up[0] = (cr * sp * cy) + (sr * sy);
			up[1] = (cr * sp * sy) - (sr * cy);
			up[2] = cr * cp;
		}
	}
}


void vectoangles (vec3_t value1, vec3_t angles)
{
	float	forward;
	float	yaw, pitch;

	if (value1[1] == 0.0f && value1[0] == 0.0f )
	{
		yaw = 0.0f;
		if (value1[2] > 0.0f)
			pitch = 90.0f;
		else
			pitch = 270.0f;
	}
	else
	{
	// PMM - fixed to correct for pitch of 0
		if (value1[0])
			yaw = (atan2f(value1[1], value1[0]) * 180.0f / (float)M_PI);
		else if (value1[1] > 0)
			yaw = 90.0f;
		else
			yaw = 270.0f;

		if (yaw < 0.0f)
			yaw += 360.0f;

		forward = sqrtf(value1[0]*value1[0] + value1[1]*value1[1]);
		pitch = (atan2f(value1[2], forward) * 180.0f / (float)M_PI);
		if (pitch < 0.0f)
			pitch += 360.0f;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0.0f;
}


/*
===============
LerpAngle

===============
*/
float LerpAngle (float a2, float a1, float frac)
{
	if (a1 - a2 > 180.0f)
		a1 -= 360.0f;
	if (a1 - a2 < -180.0f)
		a1 += 360.0f;
	return a2 + frac * (a1 - a2);
}

/** @brief  Angle "modulo 360"
 *
 * @param   a  Angle in degrees
 * @return  Angle in range [0,360)
 */
float anglemod( float a )
{
	if ( a < 0.0f )
		a += ceilf( -a/360.0f ) * 360.0f;
	else if ( a >= 360.0f )
		a -= floorf( a/360.0f ) * 360.0f;

	return a;
}

// this is the slow, general version
int BoxOnPlaneSide2 (vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
	int		i;
	float	dist1, dist2;
	int		sides;
	vec3_t	corners[2];

	for (i=0 ; i<3 ; i++)
	{
		if (p->normal[i] < 0)
		{
			corners[0][i] = emins[i];
			corners[1][i] = emaxs[i];
		}
		else
		{
			corners[1][i] = emins[i];
			corners[0][i] = emaxs[i];
		}
	}
	dist1 = DotProduct (p->normal, corners[0]) - p->dist;
	dist2 = DotProduct (p->normal, corners[1]) - p->dist;
	sides = 0;
	if (dist1 >= 0.0f)
		sides = 1;
	if (dist2 < 0.0f)
		sides |= 2;

	return sides;
}

/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/

int BoxOnPlaneSide (const vec3_t emins, const vec3_t emaxs, const struct cplane_s *p)
{
	float	dist1, dist2;
	int		sides;

// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
			return 1;
		if (p->dist >= emaxs[p->type])
			return 2;
		return 3;
	}

// general case
	switch (p->signbits)
	{
	case 0:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 1:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
		break;
	case 2:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 3:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
		break;
	case 4:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 5:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emaxs[2];
		break;
	case 6:
dist1 = p->normal[0]*emaxs[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emins[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	case 7:
dist1 = p->normal[0]*emins[0] + p->normal[1]*emins[1] + p->normal[2]*emins[2];
dist2 = p->normal[0]*emaxs[0] + p->normal[1]*emaxs[1] + p->normal[2]*emaxs[2];
		break;
	default:
		dist1 = dist2 = 0;		// shut up compiler
		assert( 0 );
		break;
	}

	sides = 0;
	if (dist1 >= p->dist)
		sides = 1;
	if (dist2 < p->dist)
		sides |= 2;

	return sides;
}


void ClearBounds (vec3_t mins, vec3_t maxs)
{
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}

void AddPointToBounds (vec3_t v, vec3_t mins, vec3_t maxs)
{
	int		i;
	vec_t	val;

	for (i=0 ; i<3 ; i++)
	{
		val = v[i];
		if (val < mins[i])
			mins[i] = val;
		if (val > maxs[i])
			maxs[i] = val;
	}
}


int VectorCompare (vec3_t v1, vec3_t v2)
{
	if (fabs (v1[0] - v2[0]) > FLT_EPSILON || fabs (v1[1] - v2[1]) > FLT_EPSILON || fabs (v1[2] - v2[2]) > FLT_EPSILON)
		return 0;

	return 1;
}


vec_t VectorNormalize (vec3_t v)
{
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	length = sqrtf(length);

	if (length)
	{
		ilength = 1.0f/length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}

	return length;

}

vec_t Vector2Normalize (vec2_t v)
{
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1];
	length = sqrtf(length);

	if (length)
	{
		ilength = 1.0f/length;
		v[0] *= ilength;
		v[1] *= ilength;
	}

	return length;

}

vec_t VectorNormalize2 (vec3_t v, vec3_t out)
{
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];
	length = sqrtf(length);

	if (length)
	{
		ilength = 1.0f/length;
		out[0] = v[0]*ilength;
		out[1] = v[1]*ilength;
		out[2] = v[2]*ilength;
	}

	return length;

}

void VectorMA (const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc)
{
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}

void Vector2MA (const vec2_t veca, float scale, const vec2_t vecb, vec2_t vecc)
{
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
}

vec_t _DotProduct (vec3_t v1, vec3_t v2)
{
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void _VectorSubtract (vec3_t veca, vec3_t vecb, vec3_t out)
{
	out[0] = veca[0]-vecb[0];
	out[1] = veca[1]-vecb[1];
	out[2] = veca[2]-vecb[2];
}

void _VectorAdd (vec3_t veca, vec3_t vecb, vec3_t out)
{
	out[0] = veca[0]+vecb[0];
	out[1] = veca[1]+vecb[1];
	out[2] = veca[2]+vecb[2];
}

void _VectorCopy (vec3_t in, vec3_t out)
{
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void CrossProduct (const vec3_t v1, const vec3_t v2, vec3_t cross)
{
	cross[0] = v1[1]*v2[2] - v1[2]*v2[1];
	cross[1] = v1[2]*v2[0] - v1[0]*v2[2];
	cross[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

vec_t VectorLength(vec3_t v)
{
	int		i;
	float	length;

	length = 0;
	for (i=0 ; i< 3 ; i++)
		length += v[i]*v[i];
	length = sqrtf(length);

	return length;
}

void VectorInverse (vec3_t v)
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void VectorScale (const vec3_t in, vec_t scale, vec3_t out)
{
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
}


int Q_log2(int val)
{
	int answer=0;
	while (val>>=1)
		answer++;
	return answer;
}



//====================================================================================

/*
============
COM_SkipPath
============
*/
const char *COM_SkipPath (const char *pathname)
{
	const char	*last;

	last = pathname;
	while (*pathname)
	{
		if (*pathname=='/')
			last = pathname+1;
		pathname++;
	}
	return last;
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension (const char *in, char *out)
{
	while (*in && *in != '.')
		*out++ = *in++;
	*out = 0;
}

qboolean COM_HasExtension (const char *path, const char *extension)
{
	return	strlen (path) > strlen (extension) &&
			!strcmp (path + strlen (path) - strlen (extension), extension);
}


/*
============
COM_FileExtension
============
*/
char *COM_FileExtension (char *in)
{
    char *src;
	static char exten[32];
	int		i;

    src = in + strlen (in) - 1;
    while (src != in && *(src - 1) != '.')
		src--;
	if (src == in)
		return "";
	for (i=0 ; i<31 && *src ; i++,src++)
		exten[i] = *src;
	exten[i] = 0;
	return exten;
}

/*
============
COM_FileBase
============
*/
void COM_FileBase (char *in, char *out)
{
	char *s, *s2;

	s = in + strlen(in) - 1;

	while (s != in && *s != '.')
		s--;

	for (s2 = s ; s2 != in && *s2 != '/' ; s2--)
	;

	if (s-s2 < 2)
		out[0] = 0;
	else
	{
		s--;
		strncpy (out,s2+1, s-s2);
		out[s-s2] = 0;
	}
}

/*
============
COM_FilePath

Returns the path up to, but not including the last /
============
*/
void COM_FilePath (char *in, char *out)
{
	char *s;

	s = in + strlen(in) - 1;

	while (s != in && *s != '/')
		s--;

	strncpy (out,in, s-in);
	out[s-in] = 0;
}


/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension (char *path, char *extension)
{
	char    *src;
//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen(path) - 1;

	while (*src != '/' && src != path)
	{
		if (*src == '.')
			return;                 // it has an extension
		src--;
	}

	strcat (path, extension);
}

/*
============================================================================

					BYTE ORDER FUNCTIONS

============================================================================
*/

qboolean	bigendien;

// can't just use function pointers, or dll linkage can
// mess up when qcommon is included in multiple places
short	(*_BigShort) (short l);
short	(*_LittleShort) (short l);
int		(*_BigLong) (int l);
int		(*_LittleLong) (int l);
float	(*_BigFloat) (float l);
float	(*_LittleFloat) (float l);

short	BigShort(short l){return _BigShort(l);}
short	LittleShort(short l) {return _LittleShort(l);}
int		BigLong (int l) {return _BigLong(l);}
int		LittleLong (int l) {return _LittleLong(l);}
float	BigFloat (float l) {return _BigFloat(l);}
float	LittleFloat (float l) {return _LittleFloat(l);}

short   ShortSwap (short l)
{
	byte    b1,b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

short	ShortNoSwap (short l)
{
	return l;
}

int    LongSwap (int l)
{
	byte    b1,b2,b3,b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

int	LongNoSwap (int l)
{
	return l;
}

float FloatSwap (float f)
{
	union
	{
		float	f;
		byte	b[4];
	} dat1, dat2;


	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

float FloatNoSwap (float f)
{
	return f;
}

/*
================
Swap_Init
================
*/
void Swap_Init (void)
{
	byte	swaptest[2] = {1,0};

// set the byte swapping variables in a portable manner
	if ( *(short *)swaptest == 1)
	{
		bigendien = false;
		_BigShort = ShortSwap;
		_LittleShort = ShortNoSwap;
		_BigLong = LongSwap;
		_LittleLong = LongNoSwap;
		_BigFloat = FloatSwap;
		_LittleFloat = FloatNoSwap;
	}
	else
	{
		bigendien = true;
		_BigShort = ShortNoSwap;
		_LittleShort = ShortSwap;
		_BigLong = LongNoSwap;
		_LittleLong = LongSwap;
		_BigFloat = FloatNoSwap;
		_LittleFloat = FloatSwap;
	}

}

/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
/*
 * 2010-11 Use multiple static buffers to avoid collisions.
 *  Like g_utils::tv(),vtos()
 */
char	*va(char *format, ...)
{
	va_list		argptr;
	static int index = 0;
	static char string[8][1024];

	index = (index + 1) & 0x07;
	va_start (argptr, format);
	vsnprintf(string[index], sizeof(string[0]), format, argptr);
	va_end (argptr);

	return string[index];
}


char	com_token[MAX_TOKEN_CHARS];

/*
==============
COM_Parse

Parse a token out of a string
==============
*/
char *COM_Parse (const char **data_p)
{
	int		c;
	int		len;
	const char	*data;

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

int Q_strnicmp (const char *string1, const char *string2, int n)
{
	int c1, c2;

	if (string1 == NULL)
	{
		if (string2 == NULL)
			return 0;
		else
			return -1;
	}
	else if (string2 == NULL)
		return 1;

	do
	{
		c1 = *string1++;
		c2 = *string2++;

		if (!n--)
			return 0;// Strings are equal until end point

		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
				c1 -= ('a' - 'A');
			if (c2 >= 'a' && c2 <= 'z')
				c2 -= ('a' - 'A');

			if (c1 != c2)
				return c1 < c2 ? -1 : 1;
		}
	} while (c1);

	return 0;// Strings are equal
}
void Q_strncpyz2 (char *dst, const char *src, int dstSize)
{
	if (!dst)
		Sys_Error(ERR_FATAL, "Q_strncpyz: NULL dst");

	if (!src)
		Sys_Error(ERR_FATAL, "Q_strncpyz: NULL src");

	if (dstSize < 1)
		Sys_Error(ERR_FATAL, "Q_strncpyz: dstSize < 1");

	strncpy(dst, src, dstSize-1);
	dst[dstSize-1] = 0;
}

void Q_strcat (char *dst, const char *src, int dstSize)
{

	int len;

	len = strlen(dst);
	if (len >= dstSize)
		Sys_Error(ERR_FATAL, "Q_strcat: already overflowed");

	Q_strncpyz2(dst + len, src, dstSize - len);
}

char *Com_SkipWhiteSpace (char *data_p, qboolean *hasNewLines)
{
	int c;

	while ((c = *data_p) <= ' ')
	{
		if (!c)
			return NULL;

		if (c == '\n')
		{
			com_parseLine++;
			*hasNewLines = true;
		}
		data_p++;
	}
	return data_p;
}
void Com_SkipRestOfLine (char **data_p)
{
	char*data;
	int c;

	data = *data_p;
	while ((c = *data++) != 0)
	{
		if (c == '\n')
		{
			com_parseLine++;
			break;
		}
	}

	*data_p = data;
}
char *Com_ParseExt (char **data_p, qboolean allowNewLines)
{
	int c, len = 0;
	char *data;
	qboolean hasNewLines = false;

	data = *data_p;
	com_token[0] = 0;

	// Make sure incoming data is valid
	if (!data)
	{
		*data_p = NULL;
		return com_token;
	}

	// Backup the session data so we can unget easily
//	Com_BackupParseSession(data_p);

	while (1)
	{
		// Skip whitespace
		data = Com_SkipWhiteSpace(data, &hasNewLines);
		if (!data)
		{
			*data_p = NULL;
			return com_token;
		}

		if (hasNewLines && !allowNewLines)
		{
			*data_p = data;
			return com_token;
		}

		c = *data;

		// Skip // comments
		if (c == '/' && data[1] == '/')
		{
			while (*data && *data != '\n')
			data++;
		}

		// Skip /* */ comments
		else if (c == '/' && data[1] == '*')
		{
			data += 2;

			while (*data && (*data != '*' || data[1] != '/'))
			{
				if (*data == '\n')
					com_parseLine++;

				data++;
			}

			if (*data)
			data += 2;
		}

			// An actual token
			else
			break;
	}

	// Handle quoted strings specially
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c == '\n')
				com_parseLine++;

			if (c == '\"' || !c)
			{
				*data_p = data;
				com_token[len] = 0;
				return com_token;
			}
			if (len < MAX_TOKEN_CHARS)
			com_token[len++] = c;
		}
	}

	// Parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS)
			com_token[len++] = c;

		data++;
		c = *data;
	} while (c > 32);

	if (len == MAX_TOKEN_CHARS)
		len = 0;

	com_token[len] = 0;

	*data_p = data;
	return com_token;
}

/*
===============
Com_PageInMemory

===============
*/
int	paged_total;

void Com_PageInMemory (byte *buffer, int size)
{
	int		i;

	for (i=size-1 ; i>0 ; i-=4096)
		paged_total += buffer[i];
}



/*
============================================================================

					LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/


int Q_strncasecmp (const char *s1, const char *s2, int n)
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

int Q_strcasecmp (const char *s1, const char *s2)
{
	return Q_strncasecmp (s1, s2, 99999);
}



void Com_sprintf (char *dest, int size, char *fmt, ...)
{
	int		len;
	va_list		argptr;
	char	bigbuffer[0x10000];

	va_start (argptr,fmt);
	len = vsnprintf (bigbuffer,sizeof(bigbuffer), fmt,argptr);
	va_end (argptr);
	if (len >= size)
		Com_Printf ("Com_sprintf: overflow of %i in %i\n", len, size);

	//JD - Fix for potential server crashes.
	bigbuffer[size-1] = '\0';
	//strcpy (dest, bigbuffer);
	strncpy (dest, bigbuffer, size-1);
}

/*
============================================================================

				MORE LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

// not used, may be obsolete (2010-08)
/*
==============
Q_strncpyz
==============
*/
/*
void Q_strncpyz( char *dest, const char *src, size_t size )
{
#ifdef HAVE_STRLCPY
	strlcpy( dest, src, size );
#else
	if( size ) {
		while( --size && (*dest++ = *src++) );
		*dest = '\0';
	}
#endif
}
*/
/*
==============
Q_strncatz
==============
*/

/*
void Q_strncatz( char *dest, const char *src, size_t size )
{
#ifdef HAVE_STRLCAT
	strlcat( dest, src, size );
#else
	if( size ) {
		while( --size && *dest++ );
		if( size ) {
			dest--;
			while( --size && (*dest++ = *src++) );
		}
		*dest = '\0';
	}
#endif
}
*/

/*
==============
Q_strlwr
==============
*/
char *Q_strlwr( char *s )
{
	char *p;

	if( s ) {
		for( p = s; *s; s++ )
			*s = tolower( *s );
		return p;
	}

	return NULL;
}

/*
=====================================================================

  INFO STRINGS

=====================================================================
*/

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
===============
*/
char *Info_ValueForKey (const char *s, const char *key)
{
	/*
	 * 2010-11 Increase static buffers from 2 to 8 to avoid collisions
	 *  Like g_utils.c::tv(),vtos()
	 */
	char	pkey[512];
	static	char value[8][512];
	static	int	valueindex = 0;
	char	*o;

	valueindex = (valueindex + 1) & 0x07;
	if (*s == '\\')
		s++;
	while (1)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != '\\' && *s)
		{
			if (!*s)
				return "";
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp (key, pkey) )
			return value[valueindex];

		if (!*s)
			return "";
		s++;
	}
}

void Info_RemoveKey (char *s, const char *key)
{
	char	*start;
	char	pkey[512];
	char	value[512];
	char	*o;

	if (strstr (key, "\\"))
	{
//		Com_Printf ("Can't use a key with a \\\n");
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
			s++;
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
				return;
			*o++ = *s++;
		}
		*o = 0;

		if (!strcmp (key, pkey) )
		{
			/* Two strcpy's instead of one - this was messing up
			 * on modern, 64-bit Linux system due to "s" being modified
			 * while it is being copied.
			 * - E.B.
			 */
			strcpy( value , s );
			strcpy( start , value );
			return;
		}

		if (!*s)
			return;
	}

}


/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing
==================
*/
qboolean Info_Validate (const char *s)
{
	if (strstr (s, "\""))
		return false;
	if (strstr (s, ";"))
		return false;
	return true;
}

void Info_SetValueForKey (char *s, const char *key, const char *value)
{
	char	newi[MAX_INFO_STRING], *v;
	int		c;
	int		maxsize = MAX_INFO_STRING;

	if (strstr (key, "\\") || strstr (value, "\\") )
	{
		Com_Printf ("Can't use keys or values with a \\\n");
		return;
	}

	if (strstr (key, ";") )
	{
		Com_Printf ("Can't use keys or values with a semicolon\n");
		return;
	}

	if (strstr (key, "\"") || strstr (value, "\"") )
	{
		Com_Printf ("Can't use keys or values with a \"\n");
		return;
	}

	if (strlen(key) > MAX_INFO_KEY-1 || strlen(value) > MAX_INFO_KEY-1)
	{
		Com_Printf ("Keys and values must be < 64 characters.\n");
		return;
	}
	Info_RemoveKey (s, key);
	if (!value || !strlen(value))
		return;

	Com_sprintf (newi, sizeof(newi), "\\%s\\%s", key, value);

	if (strlen(newi) + strlen(s) > maxsize)
	{
		Com_Printf ("Info string length exceeded\n");
		return;
	}

	// only copy ascii values
	s += strlen(s);
	v = newi;
	while (*v)
	{
		c = *v++;
		c &= 127;		// strip high bits
		if (c >= 32 && c < 127)
			*s++ = c;
	}
	*s = 0;
}
qboolean Info_KeyExists (const char *s, const char *key)
{
	char	pkey[512];
	char	*o;

	if (*s == '\\')
		s++;

	for (;;)
	{
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
				return false;
			*o++ = *s++;
		}
		*o = 0;
		s++;

		while (*s != '\\' && *s)
			s++;

		if (!strcmp (key, pkey) )
			return true;

		if (!*s)
			return false;
		s++;
	}
}
//====================================================================

/*
 * ValidatePlayerName()
 *
 * checks validity of a player name string.
 * may modify the string:
 *   - player_name must NOT be const.
 *   - player_name_size passes size of caller's char array
 *
 * returns: the number of glyphs (visible chars) in the player's name
 */
size_t ValidatePlayerName( char *player_name, size_t player_name_size )
{
	char *pch;
	size_t count;
	size_t char_count;
	size_t glyph_count;
	size_t char_count_limit;

	assert( player_name != NULL );
	assert( player_name_size > 0);
	assert( strlen( player_name ) < 127 );

	for ( pch = player_name, count = strlen( player_name); count--; pch++ )
	{ // translate bad chars to space
		if ( !isascii( *pch ) || !isgraph( *pch ) )
			*pch = ' ';
	}

	if ( player_name_size < PLAYERNAME_SIZE )
	{
		char_count_limit = player_name_size - 1;
	}
	else
	{
		char_count_limit = PLAYERNAME_SIZE - 1;
	}
	char_count = glyph_count = 0;
	pch = player_name;
	while ( *pch
			&& glyph_count < PLAYERNAME_GLYPHS
			&& char_count < char_count_limit
			)
	{ // while chars and 1+ glyphs possible and 1+ chars possible.
		if ( Q_IsColorString( pch ) )
		{
			if ( char_count < (char_count_limit-3) )
			{ // room for 3 chars is available
				char_count += 2;
				pch += 2;
			}
			else
			{ // no room for color escape and glyph, done
				break;
			}
		}
		else
		{
			++char_count;
			++glyph_count;
			++pch;
		}
	}
	assert( char_count <= char_count_limit );
	assert( glyph_count <= PLAYERNAME_GLYPHS );

	player_name[ char_count ] = '\0'; // possible truncation

	return glyph_count;
}


/*
=============
AllocTempVector

This is just a convenience function
for making temporary vectors for function calls
=============
*/
float	*atv (void)
{
	static	int		index;
	static	vec3_t	vecs[8];
	float	*v;

	// use an array so that multiple tempvectors won't collide
	// for a while
	v = vecs[index];
	index = (index + 1)&7;

	VectorClear (v);

	return v;
}

/*
=============
TempVector

This is just a convenience function
for making temporary vectors for function calls
=============
*/
float	*tv (float x, float y, float z)
{
	float	*v = atv();

	VectorSet (v, x, y, z);

	return v;
}
