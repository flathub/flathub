/*
Copyright (C) 1997 - 2001 Id Software, Inc.
Copyright (C) 2007 - 2014 COR Entertainment, LLC.

This program is free software; you can redistribute it and / or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111 - 1307, USA.

*/

/*
==========================================================================================================================================

VERTEX ARRAYS SUBSYSTEM - Adopted from MHQ2 and modified for CRX

This was written to enable much the same rendering code to be used for almost everything.  A smallish enough vertex array buffer is
used to enable a balance of fast transfers of smaller chunks of data to the 3D card (with - hopefully - a decent level of amortization)
and a reduced number of API calls.  This system allows up to MAX_VERTS verts to be transferred at once.  In reality though, the transfer
has to take place at least every time the primitive type changes (that's vertex arrays for you).

==========================================================================================================================================
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"
#include "r_ragdoll.h"

float VArrayVerts[MAX_VARRAY_VERTS * MAX_VARRAY_VERTEX_SIZE];

float *VArray = &VArrayVerts[0];

float	tex_array[MAX_ARRAY][2];
float	vert_array[MAX_ARRAY][3];

// sizes of our vertexes.  the vertex type can be used as an index into this array
int VertexSizes[] = {5, 5, 7, 7, 9, 11, 5, 3, 12, 5};

static long int KillFlags = 0;
#define MAX_TEXCOORDS		2
#define KILL_TMU0_POINTER	1
#define KILL_NORMAL_POINTER (KILL_TMU0_POINTER<<MAX_TEXCOORDS)

static long int KillAttribFlags = 0;

void R_TexCoordPointer (int tmu, GLsizei stride, const GLvoid *pointer)
{
	assert (tmu < MAX_TEXCOORDS);
	
	qglClientActiveTextureARB (GL_TEXTURE0 + tmu);
	qglEnableClientState (GL_TEXTURE_COORD_ARRAY);
	qglTexCoordPointer (2, GL_FLOAT, stride, pointer);
	
	KillFlags |= KILL_TMU0_POINTER << tmu;
}

void R_VertexPointer (GLint size, GLsizei stride, const GLvoid *pointer)
{
	qglEnableClientState (GL_VERTEX_ARRAY);
	qglVertexPointer (size, GL_FLOAT, stride, pointer);
}

void R_NormalPointer (GLsizei stride, const GLvoid *pointer)
{
	qglEnableClientState (GL_NORMAL_ARRAY);
	qglNormalPointer (GL_FLOAT, stride, pointer);
	
	KillFlags |= KILL_NORMAL_POINTER;
}

void R_AttribPointer (	GLuint index, GLint size, GLenum type,
						GLboolean normalized, GLsizei stride,
						const GLvoid *pointer )
{
	assert (index < MAX_ATTR_IDX);
	
	glEnableVertexAttribArrayARB (index);
	glVertexAttribPointerARB (index, size, type, normalized, stride, pointer);
	
	KillAttribFlags |= 1 << index;
}

/*
=================
R_InitVArrays

Sets up the current vertex arrays we're going to use for rendering.
=================
*/
void R_InitVArrays (int varraytype)
{
	// it's assumed that the programmer has already called glDrawArrays for everything before calling this, so
	// here we will just re-init our pointer and counter
	VArray = &VArrayVerts[0];

	// init the kill flags so we'll know what to kill
	KillAttribFlags = KillFlags = 0;

	// all vertex types bring up a vertex pointer
	// uses array indices 0, 1, 2
	R_VertexPointer (3, sizeof (float) * VertexSizes[varraytype], &VArrayVerts[0]);

	// the simplest possible textured render uses a texcoord pointer for TMU 0
	if (varraytype == VERT_SINGLE_TEXTURED)
	{
		// uses array indices 3, 4
		R_TexCoordPointer (0, sizeof (float) * VertexSizes[VERT_SINGLE_TEXTURED], &VArrayVerts[3]);
		return;
	}
}


/*
=================
R_KillVArrays

Shuts down the specified vertex arrays.  Again, for maximum flexibility no programmer
hand-holding is done.
=================
*/
void R_KillVArrays (void)
{
	int tmu, attr_idx;
	
	BSP_InvalidateVBO ();
	
	if(KillFlags & KILL_NORMAL_POINTER)
		qglDisableClientState (GL_NORMAL_ARRAY);
	
	for (tmu = 0; tmu < MAX_TEXCOORDS; tmu++)
	{
		if ((KillFlags & (KILL_TMU0_POINTER << tmu)) != 0)
		{
			qglClientActiveTextureARB (GL_TEXTURE0 + tmu);
			qglDisableClientState (GL_TEXTURE_COORD_ARRAY);
		}
	}
	
	for (attr_idx = 0; attr_idx < MAX_ATTR_IDX; attr_idx++)
	{
		if ((KillAttribFlags & (1 << attr_idx)) != 0)
			glDisableVertexAttribArrayARB (attr_idx);
	}

	// always kill
	qglDisableClientState (GL_VERTEX_ARRAY);
	
	KillAttribFlags = KillFlags = 0;
}

void R_DrawVarrays(GLenum mode, GLint first, GLsizei count)
{
	if(count < 1)
		return; //do not send arrays of zero size to GPU!

	qglDrawArrays (mode, first, count);
}

/*
====================
R_InitQuadVarrays

Used for 2 dimensional quads
====================
*/
void R_InitQuadVarrays(void) {

	R_TexCoordPointer (0, sizeof(tex_array[0]), tex_array[0]);
	R_VertexPointer (3, sizeof(vert_array[0]), vert_array[0]);
}
