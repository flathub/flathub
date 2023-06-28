/*
Copyright (C) 2010-2014 COR Entertainment, LLC.

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"
#include "r_ragdoll.h"
#include <GL/gl.h>

extern void MYgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);

// GL_EXT_framebuffer_object
GLboolean(APIENTRY * qglIsRenderbufferEXT) (GLuint renderbuffer);
void            (APIENTRY * qglBindRenderbufferEXT) (GLenum target, GLuint renderbuffer);
void            (APIENTRY * qglDeleteRenderbuffersEXT) (GLsizei n, const GLuint * renderbuffers);
void            (APIENTRY * qglGenRenderbuffersEXT) (GLsizei n, GLuint * renderbuffers);
void            (APIENTRY * qglRenderbufferStorageEXT) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
void            (APIENTRY * qglGetRenderbufferParameterivEXT) (GLenum target, GLenum pname, GLint * params);

GLboolean(APIENTRY * qglIsFramebufferEXT) (GLuint framebuffer);
void            (APIENTRY * qglBindFramebufferEXT) (GLenum target, GLuint framebuffer);
void            (APIENTRY * qglDeleteFramebuffersEXT) (GLsizei n, const GLuint * framebuffers);
void            (APIENTRY * qglGenFramebuffersEXT) (GLsizei n, GLuint * framebuffers);

GLenum(APIENTRY * qglCheckFramebufferStatusEXT) (GLenum target);
void            (APIENTRY * qglFramebufferTexture1DEXT) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
														 GLint level);
void            (APIENTRY * qglFramebufferTexture2DEXT) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
														 GLint level);
void            (APIENTRY * qglFramebufferTexture3DEXT) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture,
														 GLint level, GLint zoffset);
void            (APIENTRY * qglFramebufferRenderbufferEXT) (GLenum target, GLenum attachment, GLenum renderbuffertarget,
															GLuint renderbuffer);
void            (APIENTRY * qglGetFramebufferAttachmentParameterivEXT) (GLenum target, GLenum attachment, GLenum pname,
																		GLint * params);
void            (APIENTRY * qglGenerateMipmapEXT) (GLenum target);

// GL_EXT_framebuffer_blit
void			(APIENTRY * qglBlitFramebufferEXT) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

GLuint			fboId[deptex_num];

void R_CheckFBOExtensions (void)
{
	gl_state.fbo = true;
	gl_state.hasFBOblit = false;

	if ( !GL_QueryExtension("GL_EXT_framebuffer_object") )
	{
		Com_Printf("...Cannot find OpenGL Framebuffer extension, CANNOT use FBO\n");
		gl_state.fbo = false;
		return;
	}

	qglGenFramebuffersEXT		= (PFNGLGENFRAMEBUFFERSEXTPROC)		qwglGetProcAddress("glGenFramebuffersEXT");
	qglBindFramebufferEXT		= (PFNGLBINDFRAMEBUFFEREXTPROC)		qwglGetProcAddress("glBindFramebufferEXT");
	qglFramebufferTexture2DEXT	= (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)	qwglGetProcAddress("glFramebufferTexture2DEXT");
	qglCheckFramebufferStatusEXT	= (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)	qwglGetProcAddress("glCheckFramebufferStatusEXT");
	qglGenRenderbuffersEXT		= (PFNGLGENRENDERBUFFERSEXTPROC)qwglGetProcAddress("glGenRenderbuffersEXT");
	qglBindRenderbufferEXT		= (PFNGLBINDRENDERBUFFEREXTPROC)qwglGetProcAddress("glBindRenderbufferEXT");
	qglRenderbufferStorageEXT	= (PFNGLRENDERBUFFERSTORAGEEXTPROC)qwglGetProcAddress("glRenderbufferStorageEXT");
	qglFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)qwglGetProcAddress("glFramebufferRenderbufferEXT");
	qglBlitFramebufferEXT = (PFNGLBLITFRAMEBUFFEREXTPROC)qwglGetProcAddress("glBlitFramebufferEXT");
	qglGenerateMipmapEXT = (PFNGLGENERATEMIPMAPEXTPROC)qwglGetProcAddress("glGenerateMipmapEXT");

	if(!qglGenFramebuffersEXT || !qglBindFramebufferEXT || !qglFramebufferTexture2DEXT || !qglCheckFramebufferStatusEXT
		|| !qglGenRenderbuffersEXT || !qglBindRenderbufferEXT || !qglRenderbufferStorageEXT || !qglFramebufferRenderbufferEXT
		|| !qglGenerateMipmapEXT)
	{
		Com_Printf("...Cannot find OpenGL Framebuffer extension, CANNOT use FBO\n");
		gl_state.fbo = false;
		return;
	}

	// Framebuffer object blit
	if ( GL_QueryExtension("GL_EXT_framebuffer_blit") )
	{
		Com_Printf("...using GL_EXT_framebuffer_blit\n");
		gl_state.hasFBOblit = true;
	} else
	{
		Com_Printf("...GL_EXT_framebuffer_blit not found\n");
	}
	
	//must check for ability to blit(Many old ATI drivers do not support)
	//TODO: redundant with previous check?
	if(gl_state.hasFBOblit) {
		if(!qglBlitFramebufferEXT) {
			Com_Printf("glBlitFramebufferEXT not found...\n");
			gl_state.hasFBOblit = false;
		}
	}
}
    

//used for shadowmapping
void R_GenerateShadowFBO()
{
	int size;
	int i;
	GLenum FBOstatus;
	
	if (!gl_state.fbo || !gl_state.hasFBOblit)
	return;
	
	for (i = 0; i < deptex_num; i++)
	{
		//FBO for shadowmapping
		qglBindTexture(GL_TEXTURE_2D, r_depthtextures[i]->texnum);

		// GL_LINEAR removes pixelation - GL_NEAREST removes artifacts on outer edges in some cases
		if (i == deptex_dynamic)
		{
			qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			// Remove artefact on the edges of the shadowmap
			qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
			qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		}
		else
		{
			const GLfloat bordercolor[] = {1.0f, 1.0f, 1.0f, 1.0f};
			
			qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			qglTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			
			// Remove artefact on the edges of the shadowmap
			qglTexParameterfv (GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bordercolor);
			qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			qglTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
		}

		// This is to allow usage of shadow2DProj function in the shader
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
		qglTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
		qglTexParameteri(GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY); 

		// No need to force GL_DEPTH_COMPONENT24, drivers usually give you the max precision if available
		size = r_depthtextures[i]->upload_width;
		qglTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, size, size, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);
		qglBindTexture(GL_TEXTURE_2D, 0);

		// create a framebuffer object
		qglGenFramebuffersEXT(1, &fboId[i]);

		qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fboId[i]);

		// Instruct openGL that we won't bind a color texture with the currently binded FBO
		qglDrawBuffer(GL_NONE);
		qglReadBuffer(GL_NONE);

		// attach the texture to FBO depth attachment point
		qglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,GL_TEXTURE_2D, r_depthtextures[i]->texnum, 0);

		// check FBO status
		FBOstatus = qglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		if(FBOstatus != GL_FRAMEBUFFER_COMPLETE_EXT)
		{
			Com_Printf("GL_FRAMEBUFFER_COMPLETE_EXT failed, CANNOT use FBO\n");
			gl_state.fbo = false;
			qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		}
	}
	
	// switch back to window-system-provided framebuffer
	qglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

#define M3D_INV_PI_DIV_180 (57.2957795130823229)
#define m3dRadToDeg(x)	((x)*M3D_INV_PI_DIV_180)

static void normalize( float vec[3] )
{
	float len;
	len = sqrt( vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2] );
	if ( len > 0 ) {
		vec[0] /= len;
		vec[1] /= len;
		vec[2] /= len;
	}
}

static void cross( float result[3] , float v1[3] , float v2[3] )
{
	result[0] = v1[1] * v2[2] - v1[2] * v2[1];
	result[1] = v1[2] * v2[0] - v1[0] * v2[2];
	result[2] = v1[0] * v2[1] - v1[1] * v2[0];
}


static void lookAt( float position_x , float position_y , float position_z , float lookAt_x , float lookAt_y , float lookAt_z )
{
	float forward[3] , side[3] , up[3];
	float matrix[4][4];
	int i;

	// forward = normalized( lookAt - position )
	forward[0] = lookAt_x - position_x;
	forward[1] = lookAt_y - position_y;
	forward[2] = lookAt_z - position_z;
	normalize( forward );

	// side = normalized( forward x [0;1;0] )
	side[0] = -forward[2];
	side[1] = 0;
	side[2] = forward[0];
	normalize( side );

	// up = side x forward
	cross( up , side , forward );

	// Build matrix
	for ( i = 0 ; i < 3 ; i ++ ) {
		matrix[i][0] = side[i];
		matrix[i][1] = up[i];
		matrix[i][2] = - forward[i];
	}
	for ( i = 0 ; i < 3 ; i ++ )
		matrix[i][3] = matrix[3][i] = 0;
	matrix[3][3] = 1;

	qglMultMatrixf( (const GLfloat *) &matrix[0][0] );
	qglTranslated( -position_x , -position_y , -position_z );
}

void transform_point(float out[4], const float m[16], const float in[4]);
static void SM_ZoomCenter (const float *world_matrix, const float *project_matrix, vec3_t out)
{
	float origin[4], model[4], proj[4];
	
	VectorCopy (r_origin, origin);
	origin[3] = 1.0;
	
	transform_point (model, world_matrix, origin);
	transform_point (proj, project_matrix, model);
	
	VectorScale (proj, 1.0f/proj[3], out);
}

// for point light sources, we generate the shadow depth buffer using a
// perspective transform
static void SM_SetupMatrices (float position_x, float position_y, float position_z, float lookAt_x, float lookAt_y, float lookAt_z)
{
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity();
	MYgluPerspective(120.0f, r_newrefdef.width/r_newrefdef.height, 10.0f, 4096.0f);
	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity();
	lookAt( position_x , position_y , position_z , lookAt_x , lookAt_y , lookAt_z );
}

// for the sun, we generate the shadow depth buffer using an orthographic
// transform, and we pan and zoom so that only the area surrounding the player
// gets shadowed, thus requiring a smaller shadow buffer for the same detail
static void SM_SetupSunMatrices (float position_x, float position_y, float position_z, float lookAt_x, float lookAt_y, float lookAt_z, qboolean followCamera, qboolean zoom)
{
	float	sm_world_matrix[16], sm_project_matrix[16];
	vec3_t	camera_sm_pos;

	// Keep the homogeneous coordinates equivalent but boost the w-coordinate
	// above 1 everywhere. The shader wants w to be > 1 for the shadows to take
	// effect, and that is necessary to make perspective shadows work without
	// artifacts. But for ortho shadows, we need to artifically boost the w.
	const GLdouble scale[16] = {
		2.0, 0.0, 0.0, 0.0,
		0.0, 2.0, 0.0, 0.0,
		0.0, 0.0, 2.0, 0.0,
		0.0, 0.0, 0.0, 2.0};
	
	qglMatrixMode (GL_PROJECTION);
	qglLoadMatrixd (scale);
	qglOrtho (-2048, 2048, -2048, 2048, 10, 4096);
	qglMatrixMode (GL_MODELVIEW);
	qglLoadIdentity ();
	lookAt (position_x, position_y, position_z, lookAt_x, lookAt_y, lookAt_z);
	
	if (!followCamera && !zoom)
		return;
	
	qglGetFloatv (GL_PROJECTION_MATRIX, sm_project_matrix);
	qglMatrixMode (GL_PROJECTION);
	qglLoadIdentity ();
	
	if (zoom)
	{
		float zoomFactor = 2.0f; // TODO: calculate this somehow!
		qglScalef (zoomFactor, zoomFactor, 1.0);
	}
	
	if (followCamera)
	{
		qglGetFloatv (GL_MODELVIEW_MATRIX, sm_world_matrix);
		SM_ZoomCenter (sm_world_matrix, sm_project_matrix, camera_sm_pos);
		qglTranslatef (-camera_sm_pos[0], -camera_sm_pos[1], 0.0);
	}
	
	qglMultMatrixf (sm_project_matrix);
	qglMatrixMode (GL_MODELVIEW);
}

static void SM_SetTextureMatrix (int mapnum)
{
	static double modelView[16];
	static double projection[16];

	// Moving from unit cube [-1,1] to [0,1]
	const GLdouble bias[16] = {
		0.5, 0.0, 0.0, 0.0,
		0.0, 0.5, 0.0, 0.0,
		0.0, 0.0, 0.5, 0.0,
		0.5, 0.5, 0.5, 1.0};
	
	GL_InvalidateTextureState (); // FIXME

	// Grab modelview and transformation matrices
	qglGetDoublev(GL_MODELVIEW_MATRIX, modelView);
	qglGetDoublev(GL_PROJECTION_MATRIX, projection);

	qglMatrixMode(GL_TEXTURE);
	GL_SelectTexture (mapnum);

	qglLoadIdentity();
	qglLoadMatrixd(bias);

	// concatating all matrice into one.
	qglMultMatrixd (projection);
	qglMultMatrixd (modelView);

	// Go back to normal matrix mode
	qglMatrixMode(GL_MODELVIEW);

}

/*
================
SM_RecursiveWorldNode - this variant of the classic routine renders both sides for shadowing
================
*/

void SM_RecursiveWorldNode (mnode_t *node, int clipflags)
{
	int			c;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;

	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != r_visframecount)
		return;

	if (!r_nocull->integer)
	{
		int i, clipped;
		cplane_t *clipplane;

		for (i=0,clipplane=frustum ; i<4 ; i++,clipplane++)
		{
			if (!(clipflags & (1<<i)))
				continue;
			clipped = BoxOnPlaneSide (node->minmaxs, node->minmaxs+3, clipplane);

			if (clipped == 1)
				clipflags &= ~(1<<i);	// node is entirely on screen
			else if (clipped == 2)
				return;					// fully clipped
		}
	}

	// if a leaf node, draw stuff
	if (node->contents != -1)
	{
		pleaf = (mleaf_t *)node;

		// check for door connected areas
		if (r_newrefdef.areabits)
		{
			if (! (r_newrefdef.areabits[pleaf->area>>3] & (1<<(pleaf->area&7)) ) )
				return;		// not visible
		}

		mark = pleaf->firstmarksurface;
		if (! (c = pleaf->nummarksurfaces) )
			return;

		do
		{
			(*mark++)->visframe = r_framecount;
		} while (--c);

		return;
	}	

	// recurse down the children, front side first
	SM_RecursiveWorldNode (node->children[0], clipflags);

	// draw stuff
	for ( c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c ; c--, surf++)
	{
		if (surf->visframe != r_framecount)
			continue;


		if (R_CullBox (surf->mins, surf->maxs)) 
			continue;
			
		if (surf->texinfo->flags & SURF_SKY || surf->texinfo->flags & SURF_NOSHADOW)
		{	// no skies here
			continue;
		}
		else if (SurfaceIsTranslucent(surf) && !SurfaceIsAlphaMasked (surf))
		{	// no trans surfaces
			continue;
		}
		else
		{
			if (!( surf->iflags & ISURF_DRAWTURB ) )
			{
				BSP_AddSurfToVBOAccum (surf);
			}
		}
	}

	// recurse down the back side
	SM_RecursiveWorldNode (node->children[1], clipflags);
}


static inline float point_dist_from_plane (const cplane_t *plane, const vec3_t point)
{
	switch (plane->type)
	{
	case PLANE_X:
		return point[0] - plane->dist;
	case PLANE_Y:
		return point[1] - plane->dist;
	case PLANE_Z:
		return point[2] - plane->dist;
	default:
		return DotProduct (point, plane->normal) - plane->dist;
	}
}

/*
================
SM_RecursiveWorldNode2 - this variant of the classic routine renders only one side for shadowing
================
*/

static float fadeshadow_cutoff;

static qboolean SM_SurfaceIsShadowed (const msurface_t *surf, const vec3_t origin, const vec3_t absmins, const vec3_t absmaxs, const vec3_t statLightPosition)
{
	const glpoly_t	*p = surf->polys;
	const float		*v, *v2;
	int			i;
	float		sq_len, vsq_len;
	vec3_t		vDist;
	vec3_t		tmp;
	qboolean	ret = false;

	VectorSubtract (absmaxs, absmins, tmp);

	/* lengths used for comparison only, so squared lengths may be used.*/
	sq_len = tmp[0]*tmp[0] + tmp[1]*tmp[1] + tmp[2]*tmp[2] ;
	if ( sq_len <  4096.0f )
	sq_len = 4096.0f; /* 64^2 */
	else if ( sq_len > 65536.0f )
	sq_len = 65536.0f; /* 256^2 */

	for ( v = p->verts[0], i = 0; i < p->numverts; i++, v += VERTEXSIZE )
	{
		// Generate plane out of the triangle between v, v+1, and the 
		// light point. This plane will be one of the borders of the
		// 3D volume within which an object may cast a shadow on the
		// world polygon.
		vec3_t plane_line_1, plane_line_2;
		cplane_t plane;

		// generate two vectors representing two sides of the triangle
		VectorSubtract (v, statLightPosition, plane_line_1);
		v2 = p->verts[(i+1)%p->numverts];
		VectorSubtract (v2, v, plane_line_2);

		// generate the actual plane
		CrossProduct (plane_line_1, plane_line_2, plane.normal);
		VectorNormalize (plane.normal);
		plane.type = PLANE_ANYZ;
		plane.dist = DotProduct (v, plane.normal);
		plane.signbits = SignbitsForPlane (&plane);

		// CullBox-type operation
		if (BoxOnPlaneSide (absmins, absmaxs, &plane) == 2)
			//completely clipped; we can skip this surface
			return false;

		// If at least one of the verts is within a certain distance of the
		// camera, the surface should be rendered. (No need to check this vert
		// if we already found one.)
		if (!ret)
		{
			VectorSubtract( origin, v, vDist );
			vsq_len = vDist[0]*vDist[0] + vDist[1]*vDist[1] + vDist[2]*vDist[2];
			if ( vsq_len < sq_len )
				// Don't just return true yet, the bounding-box culling might
				// rule this surface out on a later vertex.
				ret = true; 
		}

	}
	return ret;
}

static void SM_RecursiveWorldNode2 (const entity_t *ent, mnode_t *node, int clipflags, const vec3_t absmins, const vec3_t absmaxs, const vec3_t statLightPosition)
{
	int			c;
	float		dist, dist_model, dist_light;
	int			side, side_model, side_light;
	cplane_t	*plane;
	msurface_t	*surf, **mark;
	mleaf_t		*pleaf;
	qboolean	caster_off_plane;

	if (node->contents == CONTENTS_SOLID)
		return;		// solid

	if (node->visframe != r_visframecount)
		return;

	if (!r_nocull->integer)
	{
		int i, clipped;
		cplane_t *clipplane;

		for (i=0,clipplane=frustum ; i<4 ; i++,clipplane++)
		{
			if (!(clipflags & (1<<i)))
				continue;
			clipped = BoxOnPlaneSide (node->minmaxs, node->minmaxs+3, clipplane);

			if (clipped == 1)
				clipflags &= ~(1<<i);	// node is entirely on screen
			else if (clipped == 2)
				return;					// fully clipped
		}
	}

	// if a leaf node, draw stuff
	if (node->contents != -1)
	{
		pleaf = (mleaf_t *)node;

		// check for door connected areas
		if (r_newrefdef.areabits)
		{
			if (! (r_newrefdef.areabits[pleaf->area>>3] & (1<<(pleaf->area&7)) ) )
				return;		// not visible
		}

		mark = pleaf->firstmarksurface;
		if (! (c = pleaf->nummarksurfaces) )
			return;

		do
		{
			(*mark++)->visframe = r_framecount;
		} while (--c);

		return;
	}

	// node is just a decision point, so go down the apropriate sides

	// find which side of the node we are on
	plane = node->plane;
	
	dist = point_dist_from_plane (plane, r_origin);
	dist_model = point_dist_from_plane (plane, ent->origin);
	dist_light = point_dist_from_plane (plane, statLightPosition);
	
	side = dist < 0;
	side_model = dist_model < 0;
	side_light = dist_light < 0;
	
	dist = fabs (dist);
	dist_model = fabs (dist_model);
	dist_light = fabs (dist_light);
	
	//TODO: figure out the cutoff distance based on mins and maxs?
	caster_off_plane = (dist_model > 64.0f) || (BoxOnPlaneSide (absmins, absmaxs, plane) != 3);
	
	if (side != side_model && caster_off_plane)
	{
		if (side != side_light)
		{
			if (dist_light < dist_model)
				goto skip_draw;
		}
		else
			goto skip_draw;
	}
	
	// recurse down the children, front side first
	SM_RecursiveWorldNode2 (ent, node->children[side], clipflags, absmins, absmaxs, statLightPosition);

	// draw stuff
	for ( c = node->numsurfaces, surf = r_worldmodel->surfaces + node->firstsurface; c ; c--, surf++)
	{
		if (surf->visframe != r_framecount)
			continue;

		if (R_CullBox (surf->mins, surf->maxs)) 
			continue;
			
		if (surf->texinfo->flags & SURF_SKY || surf->texinfo->flags & SURF_NOSHADOW)
		{	// no skies here
			continue;
		}
		else if (SurfaceIsTranslucent(surf) || SurfaceIsAlphaMasked (surf))
		{	// no trans surfaces - creates major artifacts because of draw order!
			continue;
		}
		else
		{
			if (!( surf->iflags & ISURF_DRAWTURB ) )
			{
				if (SM_SurfaceIsShadowed (surf, ent->origin, absmins, absmaxs, statLightPosition))
					BSP_AddSurfToVBOAccum (surf);
			}
		}
	}
	
	if (side == side_model && caster_off_plane)
	{
		if (side == side_light)
		{
			if (dist_light < dist_model)
				return;
		}
		else
			return;
	}
	
skip_draw:

	if (dist >= fadeshadow_cutoff+512)
		return;

	// recurse down the back side
	SM_RecursiveWorldNode2 (ent, node->children[!side], clipflags, absmins, absmaxs, statLightPosition);
}

static void R_RedrawWorldWithShadow (const entity_t *ent, const model_t *mod, const vec3_t statLightPosition)
{
	vec3_t absmins, absmaxs;
	
	if (!r_drawworld->integer)
		return;

	if (r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	glUseProgramObjectARB (g_shadowprogramObj);
	
	R_SetShadowmapUniforms (&secondpass_bsp_shadow_uniforms.shadowmap_uniforms, 6, true);
	glUniform1fARB (secondpass_bsp_shadow_uniforms.fade, fadeShadow);
	
	VectorAdd (mod->mins, ent->origin, absmins);
	VectorAdd (mod->maxs, ent->origin, absmaxs);

	qglEnableClientState (GL_VERTEX_ARRAY);
	
	qglDepthMask (false);
	SM_RecursiveWorldNode2 (ent, r_worldmodel->nodes, 15, absmins, absmaxs, statLightPosition);
	qglDepthMask (true);
	
	BSP_FlushVBOAccum ();
	
	R_KillVArrays();

	glUseProgramObjectARB( 0 );
}

void R_DrawBSPShadowCasters (void)
{
	int i;

	if (!r_drawworld->integer)
		return;

	qglEnableClientState (GL_VERTEX_ARRAY);
	
	SM_RecursiveWorldNode (r_worldmodel->nodes, 15);
	
	// Flush the VBO accumulator now because each brush model will mess
	// with the modelview matrix when rendering its own surfaces.
	BSP_FlushVBOAccum ();

	// draw brush models
	for (i = 0; i < r_newrefdef.num_entities; i++)
	{
		currententity = &r_newrefdef.entities[i];
		if (currententity->flags & RF_TRANSLUCENT)
			continue;	// transluscent

		currentmodel = currententity->model;

		if (!currentmodel)
		{
			continue;
		}
		if( currentmodel->type == mod_brush)
			BSP_DrawTexturelessBrushModel (currententity);
		else
			continue;
	}
	
	R_KillVArrays();
}

static void SM_SetupGLState ()
{
	//Disable color rendering, we only want to write to the Z-Buffer
	qglColorMask (GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	// Culling switching, rendering only frontfaces
	qglCullFace (GL_BACK);

	qglEnable (GL_POLYGON_OFFSET_FILL);
	qglPolygonOffset (0.5f, 0.5f);	

	qglHint (GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}

static void SM_TeardownGLState (void)
{
	qglPolygonOffset( 0.0f, 0.0f );
	qglDisable( GL_POLYGON_OFFSET_FILL );
	qglCullFace(GL_FRONT);

	// back to previous screen coordinates
	R_SetupViewport ();

	//Enabling color write (previously disabled for light POV z-buffer rendering)
	qglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

#include "r_lodcalc.h"

extern cvar_t *cl_simpleitems;

// returns false if the entity can cast a shadow
static qboolean SM_Cull (const entity_t *ent, qboolean receive_shadows)
{
	return
		((ent->flags & RF_NOSHADOWS) && !receive_shadows) || (ent->flags & RF_TRANSLUCENT) ||
		!ent->model || ent->model->type <= mod_brush ||
		//TODO: simple items casting shadows?
		(cl_simpleitems->integer && ent->model->simple_texnum != 0) || 
		(r_ragdolls->integer && ent->model->type == mod_iqm && ent->model->hasRagDoll && !ent->ragdoll && ent->frame > 198 && ent->frame < 260);
}

static void R_DrawDynamicCasterEntity (entity_t *ent, vec3_t lightOrigin)
{
	vec3_t	dist;
	model_t *mod;
	
	if (SM_Cull (ent, false))
		return;

	//distance from light, if too far, don't render(to do - check against brightness for dist!)
	VectorSubtract (lightOrigin, ent->origin, dist);
	if (VectorLength (dist) > 256.0f)
		return;

	//trace visibility from light - we don't render objects the light doesn't hit!
	if (!CM_FastTrace (ent->origin, lightOrigin, r_worldmodel->firstnode, MASK_OPAQUE))
		return;

	mod = ent->model;

	//get distance
	VectorSubtract (r_origin, ent->origin, dist);
	
	//set lod if available
	if(VectorLength(dist) > LOD_DIST*(3.0/5.0))
	{
		if (ent->lod2)
			mod = ent->lod2;
	}
	else if(VectorLength(dist) > LOD_DIST*(1.0/5.0))
	{
		if (ent->lod1)
			mod = ent->lod1;
	}
	if (ent->lod2)
		mod = ent->lod2;

	R_Mesh_DrawCaster (ent, mod);
}

static void R_DrawDynamicCaster(void)
{
	int		i;
	int		RagDollID;
	vec3_t	lightOrigin;
		
	if (r_newrefdef.num_dlights == 0)
		return; //we have no lights of consequence
	
	qglViewport (0, 0, r_depthtextures[deptex_dynamic]->upload_width, r_depthtextures[deptex_dynamic]->upload_height);
	
	VectorCopy (r_newrefdef.dlights[0].origin, lightOrigin);
	
	qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, fboId[deptex_dynamic]);
	
	//Clear previous frame values
	qglClear( GL_DEPTH_BUFFER_BIT);
	
	//set camera
	SM_SetupMatrices (lightOrigin[0], lightOrigin[1], lightOrigin[2] + 64, lightOrigin[0], lightOrigin[1], lightOrigin[2] - 128);

	//render world - very basic geometry
	R_DrawBSPShadowCasters ();

	//render entities near light
	for (i=0 ; i<r_newrefdef.num_entities ; i++)
	{
		R_DrawDynamicCasterEntity (&r_newrefdef.entities[i], lightOrigin);
	}

	if (r_ragdolls->integer)
	{
		for (RagDollID = 0; RagDollID < MAX_RAGDOLLS; RagDollID++)
		{
			if (RagDoll[RagDollID].destroyed)
				continue;
		
			R_DrawDynamicCasterEntity (&RagDollEntity[RagDollID], lightOrigin);
		}
	}

	SM_SetTextureMatrix (7);
	
	GL_InvalidateTextureState (); // FIXME

}

static void R_Vectoangles (vec3_t value1, vec3_t angles)
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
		else if (value1[1] > 0.0f)
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

void R_DrawVegetationCasters ( qboolean forShadows )
{
	int		i;
	grass_t *grass;
	vec3_t	dir, angle, right, up;

	if(r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;

	GL_SetupVegetationVBO ();
	glUseProgramObjectARB (g_vegetationprogramObj);
	VectorSubtract (r_sunLight->origin, r_sunLight->target, dir);
	R_Vectoangles (dir, angle);
	AngleVectors (angle, NULL, right, up);
	glUniform1fARB (vegetation_uniforms.rsTime, rs_realtime);
	glUniform3fARB (vegetation_uniforms.up, up[0], up[1], up[2]);
	glUniform3fARB (vegetation_uniforms.right, right[0], right[1], right[2]);
	
	qglColor4f (0, 0, 0, 1);
	GL_SelectTexture (0);
	GLSTATE_ENABLE_ALPHATEST

	for (grass = r_grasses, i = 0; i < r_numgrasses; i++, grass++) 
	{
		if (grass->type != 1)
			continue; //only deal with leaves, grass shadows look kind of bad

		if (grass->sunVisible) 
		{
			r_sunShadowsOn = forShadows;
			
			GL_Bind (grass->tex->texnum);
			
			R_DrawVarrays (GL_QUADS, grass->vbo_first_vert, grass->vbo_num_verts);
		}
	}

	glUseProgramObjectARB (0);
	R_KillVArrays ();

	qglColor4f( 1,1,1,1 );
	GLSTATE_DISABLE_ALPHATEST
}

static void R_DrawEntityCaster (entity_t *ent)
{
	vec3_t	dist;
	const model_t *mod = ent->model;
	
	// We only do LODs for non-ragdoll meshes-- for now.
	if (!ent->ragdoll)
	{
		//get view distance
		VectorSubtract (r_origin, ent->origin, dist);
		
		//set lod if available
		if(VectorLength(dist) > LOD_DIST*(3.0/5.0))
		{
			if (ent->lod2)
				mod = ent->lod2;
		}
		else if(VectorLength(dist) > LOD_DIST*(1.0/5.0))
		{
			if (ent->lod1)
				mod = ent->lod1;
		}
		if (ent->lod2)
			mod = ent->lod2;
	}

	R_Mesh_DrawCaster (ent, mod);
}

static void R_DrawSunShadows (void)
{
	vec3_t dist;
	int i;

	r_sunShadowsOn = false;
	
	if (r_nosun || !r_sunLight->has_Sun)
		return; //no point if there is no sun!
	
	qglViewport (0, 0, r_depthtextures[deptex_sunstatic]->upload_width, r_depthtextures[deptex_sunstatic]->upload_height);

	qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, fboId[deptex_sunstatic]); 
	
	//Clear previous frame values
	qglClear (GL_DEPTH_BUFFER_BIT);
	
	//get sun light origin and target from map info, at map load
	//set camera
	// FIXME: hack for zooming!
	SM_SetupSunMatrices(r_sunLight->origin[0],r_sunLight->origin[1],r_sunLight->origin[2],r_sunLight->target[0],r_sunLight->target[1],r_sunLight->target[2], true, true);

	//render vegetation
	if (r_hasleaves)
		R_DrawVegetationCasters (true);
	
	for (i = 0 ; i < r_newrefdef.num_entities; i++)
	{
		trace_t tr;
		vec3_t start;
		entity_t *ent = &r_newrefdef.entities[i];

		if (SM_Cull (ent, false))
			continue;

		if (ent->model->type != mod_iqm && ent->model->type != mod_md2)
			continue;

		if (ent->flags & (RF_WEAPONMODEL | RF_SHELL_ANY))
			continue;

		// We don't do a trace to the sun entity because that results in shadows
		// popping in and out abruptly. We'll need a different mechanism for
		// detecting indoors.

		//for now do it anyway - make sure it's unblocked from the sun - otherwise horrible artifacts occur indoors
		VectorCopy (ent->origin, start);
		start[2] += 128; //in cases of being buried a bit in terrain, or bobbing
		tr = CM_BoxTrace(start, r_sunLight->origin, vec3_origin, vec3_origin, 0, MASK_VISIBILILITY);

		if(tr.fraction != 1.0)
		{
			if(!(tr.surface->flags & SURF_SKY))
					continue;
		}

		//if this entity isn't close to the player, don't bother
		VectorSubtract (r_origin, ent->origin, dist);
		if (VectorLength (dist) > 5.0 * r_shadowcutoff->value)
			continue;
		
		r_sunShadowsOn = true;

		R_DrawEntityCaster (ent);
	}
	
	SM_SetTextureMatrix (5);
}

// There are two global shadowmaps: one cast by the sun, and one cast by the
// dynamic light source. Static, non-sun shadows are not done globally.
void R_GenerateGlobalShadows (void)
{
	qglEnable(GL_DEPTH_TEST);
	qglEnable(GL_CULL_FACE);
	SM_SetupGLState ();
	
	R_DrawDynamicCaster ();
	R_DrawSunShadows ();
	
	SM_TeardownGLState ();
	qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
}

static void SM_SetupCasterMatrices (const vec3_t origin, float zOffset, const vec3_t statLightPosition)
{
	vec3_t adjLightPos;
	vec3_t lightVec;
	vec2_t xyDist;
	
	qglMatrixMode(GL_PROJECTION);
	qglPushMatrix();
	qglMatrixMode(GL_MODELVIEW);
	qglPushMatrix();

	//get light origin
	VectorCopy(statLightPosition, adjLightPos);

	//adjust height/distance ratio of light source to limit the shadow projection 
	//for preventing artifacts of massive shadows projecting all over the place	
	VectorSubtract(statLightPosition, origin, lightVec);
	xyDist[0] = lightVec[0];
	xyDist[1] = lightVec[1];
	if (abs (VectorLength (xyDist)) > abs (lightVec[2]) * 1.5)
		adjLightPos[2] += abs (VectorLength (xyDist)) - abs (lightVec[2]) * 1.5;
	
	//set camera
	SM_SetupMatrices (adjLightPos[0], adjLightPos[1], adjLightPos[2]+zOffset, origin[0], origin[1], origin[2]);
}

static void SM_TeardownCasterMatrices ()
{
	qglPopMatrix ();
	qglMatrixMode (GL_PROJECTION);
	qglPopMatrix ();
	qglMatrixMode (GL_MODELVIEW);
}

void R_GenerateEntityShadow (entity_t *ent, const vec3_t statLightPosition)
{
	r_nonSunStaticShadowsOn = false;

	if (gl_shadowmaps->integer)
	{
		vec3_t dist, origin, tmp;
		float rad, zOffset;
		int i;

		if(r_newrefdef.rdflags & RDF_NOWORLDMODEL || ent->flags & RF_MENUMODEL)
			return;
		
		if (SM_Cull (ent, true))
			return;
		
		// don't bother casting the shadow unless there are non-sun lights!
		if (cl_persistent_ents[ent->number].oldnumlights < 3.0f)
 			return;

		//root entity camera info
		VectorCopy (ent->origin, origin);
		zOffset = ent->model->maxs[2];

		VectorSubtract (ent->model->maxs, ent->model->mins, tmp);
		VectorScale (tmp, 1.666, tmp);
		rad = VectorLength (tmp);		

		if (R_CullSphere (ent->origin, rad, 15))
			return;

		//get view distance
		VectorSubtract (r_origin, ent->origin, dist);

		//fade out shadows both by distance from view, and by distance from light.  Smaller entities fade out faster.
		fadeshadow_cutoff = r_shadowcutoff->value * (LOD_DIST/LOD_BASE_DIST) * (zOffset/64 > 1.0 ? 1.0 : zOffset/64); 

		if (r_shadowcutoff->value < 0.1)
			fadeShadow = 1.0;
		else if (VectorLength (dist) > fadeshadow_cutoff)
		{
			fadeShadow = VectorLength(dist) - fadeshadow_cutoff;
			if (fadeShadow > 512)
				return;
			else
				fadeShadow = 1.0 - fadeShadow/512.0; //fade out smoothly over 512 units.
		}
		else
			fadeShadow = 1.0;
		
		// heavily fade out non-sun static shadows if there will be a sun
		// static shadow cast by this entity
		if (!r_nosun && r_sunLight->has_Sun && CM_FastTrace (r_sunLight->origin, origin, r_worldmodel->firstnode, MASK_VISIBILILITY))
			fadeShadow /= 4.0f;
		
		r_nonSunStaticShadowsOn = true;

		if(!(ent->flags & RF_NOSHADOWS))
		{
			SM_SetupCasterMatrices (origin, zOffset, statLightPosition);
			qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, fboId[deptex_otherstatic]);
			qglClear (GL_DEPTH_BUFFER_BIT);
			SM_SetupGLState ();
			qglViewport (0, 0, r_depthtextures[deptex_otherstatic]->upload_width, r_depthtextures[deptex_otherstatic]->upload_height);
		
			R_DrawEntityCaster (ent);
		
			SM_SetTextureMatrix (6);
			SM_TeardownGLState ();
			SM_TeardownCasterMatrices ();
			qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);

			//re-render affected polys with shadowmap for this entity
			GLSTATE_ENABLE_BLEND
			GL_BlendFunction (GL_ZERO, GL_SRC_COLOR);

			R_RedrawWorldWithShadow (ent, ent->model, statLightPosition);

			GL_BlendFunction (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			GLSTATE_DISABLE_BLEND
		}
		else
		{
			qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, fboId[deptex_otherstatic]);
			qglClear (GL_DEPTH_BUFFER_BIT);
		}

		if (!(ent->flags & RF_VIEWERMODEL))
		{
			SM_SetupCasterMatrices (origin, zOffset, statLightPosition);
			qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, fboId[deptex_otherstatic]);
			SM_SetupGLState ();
			qglViewport (0, 0, r_depthtextures[deptex_otherstatic]->upload_width, r_depthtextures[deptex_otherstatic]->upload_height);

			for (i = 0 ; i < r_newrefdef.num_entities; i++)
			{
				entity_t *ent2 = &r_newrefdef.entities[i];

				//don't add this entity to the depth texture - it's already in there!
				if (ent2 == ent)
					continue;
				
				if (SM_Cull (ent2, false))
					continue;

				if (ent2->model->type != mod_iqm && ent2->model->type != mod_md2)
					continue;

				if (ent2->flags & (RF_WEAPONMODEL | RF_SHELL_ANY))
					continue;

				if ((ent->flags & RF_WEAPONMODEL) && (ent2->flags & RF_VIEWERMODEL))
					continue;

				//We don't need fade out the shadow here really.  This is because if it's by distance
				//to r_origin, then it's not getting this far.  We also don't need to fade by lightmap
				//because the shadow is subtracting from the mesh's lit surface, and therefore this matches
				//up pretty good with how the bsp shadow is faded.

				//if close enough to cast a shadow on this ent and in line of sight - render it into depth buffer
				VectorSubtract (ent2->origin, origin, dist);
				{
					vec3_t lDist;
					
					if(VectorLength(dist) > 128)
						continue;

					VectorSubtract(statLightPosition, origin, lDist);
					//Wrong side of light!
					if(VectorLength(dist) > VectorLength(lDist))
						continue;
				}

				R_DrawEntityCaster (ent2);
			}
			SM_SetTextureMatrix (6);
			SM_TeardownGLState ();
			SM_TeardownCasterMatrices ();
			qglBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
		}
	}
}

void R_SetShadowmapUniforms (shadowmap_uniform_location_t *uniforms, int tmu, qboolean enable)
{
	if (uniforms->sunStatic.texture != ~0u)
	{
		glUniform1iARB (uniforms->sunStatic.enabled, r_sunShadowsOn && enable);
		if (r_sunShadowsOn && enable)
		{
			float offs = 1.0 / r_depthtextures[deptex_sunstatic]->upload_width;
			glUniform1fARB (uniforms->sunStatic.pixelOffset, offs);
			GL_MBind (tmu, r_depthtextures[deptex_sunstatic]->texnum);
			glUniform1iARB (uniforms->sunStatic.texture, tmu++);
		}
	}
	
	if (uniforms->otherStatic.texture != ~0u)
	{
		glUniform1iARB (uniforms->otherStatic.enabled, r_nonSunStaticShadowsOn && enable);
		if (r_nonSunStaticShadowsOn && enable)
		{
			float offs = 1.0 / r_depthtextures[deptex_otherstatic]->upload_width;
			glUniform1fARB (uniforms->otherStatic.pixelOffset, offs);
			GL_MBind (tmu, r_depthtextures[deptex_otherstatic]->texnum);
			glUniform1iARB (uniforms->otherStatic.texture, tmu++);
		}
	}
	
	if (uniforms->dynamic.texture != ~0u)
	{
		float offs = 1.0 / r_depthtextures[deptex_dynamic]->upload_width;
		glUniform1fARB (uniforms->dynamic.pixelOffset, offs);
		GL_MBind (tmu, r_depthtextures[deptex_dynamic]->texnum);
		glUniform1iARB (uniforms->dynamic.texture, tmu++);
	}
}
