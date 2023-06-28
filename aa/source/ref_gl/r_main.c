/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2007 - 2014 COR Entertainment, LLC.

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
// r_main.c

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"
#include "r_script.h"
#include "r_ragdoll.h"
#include "r_text.h"


void R_Clear (void);

viddef_t	vid;

int r_viewport[4];

int GL_TEXTURE0, GL_TEXTURE1, GL_TEXTURE2, GL_TEXTURE3, GL_TEXTURE4, GL_TEXTURE5, GL_TEXTURE6, GL_TEXTURE7;

model_t		*r_worldmodel;

float			gldepthmin, gldepthmax;
float			r_frametime;

glconfig_t		gl_config;
glstate_t		gl_state;

cvar_t	*r_meshnormalmaps;
cvar_t	*r_worldnormalmaps;
cvar_t  *gl_shadowmaps;
cvar_t	*gl_fog;

entity_t	*currententity;
model_t		*currentmodel;

cplane_t	frustum[4];

int		r_visframecount;	// bumped when going to a new PVS
int		r_framecount;		// used for dlight push checking

// performance counters for r_speeds reports
int last_c_brush_polys, c_brush_polys;
int last_c_alias_polys, c_alias_polys;
int c_flares;
int c_grasses;
int c_beams;
extern int c_vbo_batches;
extern int c_visible_lightmaps;
extern int c_visible_textures;

float		v_blend[4];			// final blending color

float		r_farclip, r_farclip_min, r_farclip_bias = 256.0f;

//
// view origin
//
vec3_t	vup;
vec3_t	vpn;
vec3_t	vright;
vec3_t	r_origin;
int		r_origin_leafnum;

float	r_world_matrix[16];
float	r_project_matrix[16];

//
// screen size info
//
refdef_t	r_newrefdef;

int		r_viewcluster, r_viewcluster2, r_oldviewcluster, r_oldviewcluster2;

cvar_t	*r_norefresh;
cvar_t	*r_drawentities;
cvar_t	*r_drawworld;
cvar_t	*r_fullbright;
cvar_t	*r_novis;
cvar_t	*r_nocull;
cvar_t	*r_lerpmodels;
cvar_t	*r_lefthand;

cvar_t	*r_shadowmapscale;

cvar_t	*r_overbrightbits;

cvar_t	*gl_vlights;

cvar_t	*gl_nosubimage;

cvar_t	*gl_bitdepth;
cvar_t	*gl_drawbuffer;
cvar_t	*gl_driver;
cvar_t	*gl_lightmap;
cvar_t	*gl_mode;
cvar_t	*gl_dynamic;
cvar_t	*gl_modulate;
cvar_t	*gl_nobind;
cvar_t	*gl_picmip;
cvar_t	*gl_skymip;
cvar_t	*gl_showtris;
cvar_t	*gl_showpolys;
cvar_t	*gl_showcollisionmesh;
cvar_t	*gl_finish;
cvar_t	*gl_clear;
cvar_t	*gl_cull;
cvar_t	*gl_polyblend;
cvar_t	*gl_swapinterval;
cvar_t	*gl_texturemode;
cvar_t	*gl_texturealphamode;
cvar_t	*gl_texturesolidmode;
cvar_t	*gl_lockpvs;

cvar_t	*vid_fullscreen;
cvar_t	*vid_preferred_fullscreen;
cvar_t	*vid_gamma;
cvar_t  *vid_contrast;
cvar_t	*vid_ref;

cvar_t *r_anisotropic;
cvar_t *r_alphamasked_anisotropic;
cvar_t *r_ext_max_anisotropy;
cvar_t *r_antialiasing;

cvar_t	*r_shaders;
cvar_t	*r_bloom;
cvar_t	*r_lensflare;
cvar_t	*r_lensflare_intens;
cvar_t	*r_vegetation;
cvar_t	*r_drawsun;
cvar_t	*r_lightbeam;
cvar_t	*r_godrays;
cvar_t  *r_godray_intensity;
cvar_t	*r_optimize;

cvar_t	*r_lightmapfiles;

qboolean	map_fog;

cvar_t	*con_font;

cvar_t	*r_minimap_size;
cvar_t	*r_minimap_zoom;
cvar_t	*r_minimap_style;
cvar_t	*r_minimap;

cvar_t	*sys_affinity;
cvar_t	*sys_priority;

cvar_t	*gl_screenshot_type;
cvar_t	*gl_screenshot_jpeg_quality;

//no blood
extern cvar_t *cl_noblood;

//first time running game
cvar_t	*r_firstrun;

//for testing
cvar_t  *r_test;
static cvar_t *r_tracetest, *r_fasttracetest, *r_tracetestlength;
static cvar_t *r_tracebox, *r_showbox;

//ODE initialization error check
int r_odeinit_success; // 0 if dODEInit2() fails, 1 otherwise.

//fog script stuff
struct r_fog
{
	float red;
	float green;
	float blue;
	float start;
	float end;
	float density;
} fog;
unsigned r_weather;
unsigned r_nosun;
float r_sunX;
float r_sunY;
float r_sunZ;
float r_sunIntens;
float r_skyangleX;
float r_skyangleY;
float r_skyangleZ;

/*
=================
R_ReadFogScript
=================
*/

void R_ReadFogScript( char *config_file )
{
	FILE *fp;
	int length;
	char a_string[128];
	char *buffer;
	const char *s;
	size_t result;	

	if((fp = fopen(config_file, "rb" )) == NULL)
	{
		return;
	}

	length = FS_filelength( fp );

	buffer = malloc( length + 1 );
	if ( buffer != NULL )
	{
		buffer[length] = 0;
		result = fread( buffer, length, 1, fp );
		if ( result == 1 )
		{
			s = buffer;

			strcpy( a_string, COM_Parse( &s ) );
			fog.red = atof(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			fog.green = atof(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			fog.blue = atof(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			fog.start = atof(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			fog.end = atof(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			fog.density = atof(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			r_weather = atoi(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			r_nosun = atoi(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			r_sunX = atof(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			r_sunY = atof(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			r_sunZ = atof(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			r_sunIntens = atof(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			r_skyangleX = atof(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			r_skyangleY = atof(a_string);
			strcpy( a_string, COM_Parse( &s ) );
			r_skyangleZ = atof(a_string);

			if(fog.density > 0)
				map_fog = true;

		}
		else
		{
			Com_DPrintf("R_ReadFogScript: read fail: %s\n", config_file);
		}
		free( buffer );
	}
	fclose( fp );
	
	if (gl_fog->integer < 1)
	{
		map_fog = false;
		r_weather = false;
	}

	return;
}

/*
=================
R_ReadMusicScript
=================
*/

//to do - read in secondary music location(for CTF music shift)
void R_ReadMusicScript( char *config_file )
{
	FILE *fp;
	int length;
	char *buffer;
	const char *s;
	size_t result;

	if((fp = fopen(config_file, "rb" )) == NULL)
	{
		return;
	}

	length = FS_filelength( fp );

	buffer = malloc( length + 1 );
	if ( buffer != NULL )
	{
		result = fread( buffer, length, 1, fp );
		if ( result == 1 )
		{
			buffer[length] = 0;
			s = buffer;
			strcpy( map_music, COM_Parse( &s ) );
			map_music[length] = 0; //clear any possible garbage
		}
		else
		{
			Com_DPrintf("R_ReadMusicScript: read fail: %s\n", config_file);
		}

		free( buffer );
	}
	fclose( fp );

	return;
}

/*
=================
R_CullBox

Returns true if the box is completely outside the frustom
=================
*/
qboolean R_CullBox (vec3_t mins, vec3_t maxs)
{
	int		i;
	cplane_t *p;

	if (r_nocull->integer)
		return false;

	for (i=0,p=frustum ; i<4; i++,p++)
	{
		switch (p->signbits)
		{
		case 0:
			if (p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 1:
			if (p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 2:
			if (p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 3:
			if (p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*maxs[2] < p->dist)
				return true;
			break;
		case 4:
			if (p->normal[0]*maxs[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		case 5:
			if (p->normal[0]*mins[0] + p->normal[1]*maxs[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		case 6:
			if (p->normal[0]*maxs[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		case 7:
			if (p->normal[0]*mins[0] + p->normal[1]*mins[1] + p->normal[2]*mins[2] < p->dist)
				return true;
			break;
		default:
			return false;
		}
	}

	return false;
}

/*
=================
R_CullOrigin

Returns true if the origin is completely outside the frustom
=================
*/

qboolean R_CullOrigin(vec3_t origin)
{
	int i;

	for (i = 0; i < 4; i++)
		if (BOX_ON_PLANE_SIDE(origin, origin, &frustum[i]) == 2)
			return true;
	return false;
}

qboolean R_CullSphere( const vec3_t centre, const float radius, const int clipflags )
{
	int		i;
	cplane_t *p;

	if (r_nocull->integer)
		return false;

	for (i=0,p=frustum ; i<4; i++,p++)
	{
		if ( !(clipflags & (1<<i)) ) {
			continue;
		}

		if ( DotProduct ( centre, p->normal ) - p->dist <= -radius )
			return true;
	}

	return false;
}

// should be able to handle every mesh type
void R_RotateForEntity (const entity_t *e)
{
    qglTranslatef (e->origin[0],  e->origin[1],  e->origin[2]);

    qglRotatef (e->angles[YAW],		0, 0, 1);
    // pitch and roll are handled by IQM_AnimateFrame for player models(any IQM greater than 173 frames). 
    if (e->model == NULL || (e->flags & RF_WEAPONMODEL) || (e->model->num_poses < 173))
    {
		{
			qglRotatef (e->angles[PITCH],	0, 1, 0);
			qglRotatef (e->angles[ROLL],	1, 0, 0);
		}
	}
}

/*
=============
R_DrawNullModel
=============
*/
void R_DrawNullModel (void)
{
	vec3_t	shadelight;

	if ( currententity->flags & RF_FULLBRIGHT )
		shadelight[0] = shadelight[1] = shadelight[2] = 1.0F;
	else
		R_LightPoint (currententity->origin, shadelight, true);

    qglPushMatrix ();
	R_RotateForEntity (currententity);
	qglScalef (16, 16, 16);

	GL_EnableTexture (0, false);
	qglColor3fv (shadelight);
	
	GL_DrawNullModel ();

	qglColor3f (1,1,1);
	qglPopMatrix ();
	GL_EnableTexture (0, true);
}

void R_DrawMark (vec3_t origin, float size, const float rgba[])
{
	qglPushMatrix ();
	qglTranslatef (origin[0],  origin[1],  origin[2]);
	qglScalef (size, size, size);

	GL_EnableTexture (0, false);
	qglColor4fv (rgba);

	GL_DrawNullModel ();

	qglColor3f (1,1,1);
	qglPopMatrix ();
	GL_EnableTexture (0, true);
}

#include "r_lodcalc.h"

extern cvar_t *cl_simpleitems;
static void R_DrawEntity (void)
{
	rscript_t	*rs = NULL;
	vec3_t		dist, span;
	float		size;
	
	if (currententity->nodraw) //don't draw this model, it's been overriden by the engine for one reason or another
		return;
	
	currentmodel = currententity->model;
	
	if (cl_simpleitems->integer && currentmodel && currentmodel->simple_texnum)
		return;
	
	if (currentmodel && r_shaders->integer)
	{
		rs=(rscript_t *)currentmodel->script;

		//custom player skin (must be done here)
		if (currententity->skin)
		{
		    rs = currententity->skin->script;
            if(rs)
                RS_ReadyScript(rs);
        }

		if (rs)
			currententity->script = rs;
		else
			currententity->script = NULL;
	}
	
	if (!currentmodel)
	{
		R_DrawNullModel ();
		return;
	}

	//get distance
	VectorSubtract(r_origin, currententity->origin, dist);
	
	//get diagonal size
	VectorSubtract (currentmodel->maxs, currentmodel->mins, span);
	size = VectorLength (span);
	
	// Cull very distant meshes. In practice, only tiny things like small
	// rocks and pebbles ever actually get culled.
	if (VectorLength (dist) > LOD_DIST*size)
		return;
	
	//set lod if available
	if(VectorLength(dist) > LOD_DIST*2.0 && currententity->lod2 != NULL)
		currentmodel = currententity->lod2;
	else if(VectorLength(dist) > LOD_DIST && currententity->lod1 != NULL)
		currentmodel = currententity->lod1;

	switch (currentmodel->type)
	{
	    case mod_md2:
	    case mod_iqm:
	    case mod_terrain:
	    case mod_decal:
	        R_Mesh_Draw (currententity, currentmodel);
			break;
		case mod_brush:
			R_DrawBrushModel ();
			break;
		default:
			Com_Error(ERR_DROP, "Bad modeltype");
			break;
	}
}

static void R_DrawEntityList (entity_t *list, int numentities)
{
	int		i;
	
	// draw non-transparent first
	for (i = 0 ; i < numentities; i++)
	{
		currententity = &list[i];
		if (currententity->flags & RF_TRANSLUCENT)
			continue;	// transluscent
		
		R_DrawEntity ();
	}

	// draw transparent entities
	// we could sort these if it ever becomes a problem...
	qglDepthMask (0);		// no z writes
	for (i = 0 ; i < numentities; i++)
	{
		currententity = &list[i];
		if (!(currententity->flags & RF_TRANSLUCENT))
			continue;	// solid
		
		fadeShadow = 1.0;
		R_DrawEntity ();
	}
	qglDepthMask (1);		// back to writing
}


/*
=============
R_DrawEntitiesOnList
=============
*/
static void R_DrawEntitiesOnList (void)
{
	if (!r_drawentities->integer)
		return;

	if ( !r_odeinit_success )
	{ // ODE init failed, force ragdolls off
		r_ragdolls = Cvar_ForceSet("r_ragdolls", "0");
	}
	
	R_DrawEntityList (r_newrefdef.entities, r_newrefdef.num_entities);
}

static void R_DrawRagdollsOnList (void)
{
	if (!r_ragdolls->integer)
		return;
	
	if (!r_drawentities->integer)
		return;
	
	// Just using MAX_RAGDOLLS is fine, the inactive ones have nodraw set
	R_DrawEntityList (RagDollEntity, MAX_RAGDOLLS);
}

static void R_DrawViewEntitiesOnList (void)
{
	if (!r_drawentities->integer)
		return;

	if(r_newrefdef.rdflags & RDF_NOWORLDMODEL)
		return;
	
	R_DrawEntityList (r_newrefdef.viewentities, r_newrefdef.num_viewentities);
}

static void R_DrawTerrain (void)
{
	if (!r_drawworld->integer)
		return;
	
	R_DrawEntityList (terrain_entities, num_terrain_entities);
	R_DrawEntityList (decal_entities, num_decal_entities);
	R_DrawEntityList (rock_entities, num_rock_entities);
}

extern int r_drawing_fbeffect;
extern int	r_fbFxType;
extern float r_fbeffectTime;
extern cvar_t *cl_paindist;

static void R_Flash (void)
{
	if (!gl_polyblend->integer)
		return;
	if (!v_blend[3])
		return;

	if(!r_drawing_fbeffect && cl_paindist->integer) {
		if(v_blend[0] > 2*v_blend[1] && v_blend[0] > 2*v_blend[2]) {
			r_drawing_fbeffect = true;
			r_fbFxType = 2; //FLASH DISTORTION
			r_fbeffectTime = rs_realtime;
		}
	}

	GLSTATE_DISABLE_ALPHATEST
	GLSTATE_ENABLE_BLEND
	qglDisable (GL_DEPTH_TEST);
	GL_EnableTexture (0, false);

	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();
	qglOrtho (0, 1, 1, 0, -99999, 99999);

	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity ();

	qglColor4fv (v_blend);

	GL_SetupWholeScreen2DVBO (wholescreen_blank);
	R_DrawVarrays (GL_QUADS, 0, 4);
	R_KillVArrays ();

	GLSTATE_DISABLE_BLEND
	GL_EnableTexture (0, true);
	GLSTATE_ENABLE_ALPHATEST

	qglColor4f(1,1,1,1);
}

//=======================================================================

int SignbitsForPlane (cplane_t *out)
{
	int	bits, j;

	// for fast box on planeside test

	bits = 0;
	for (j=0 ; j<3 ; j++)
	{
		if (out->normal[j] < 0)
			bits |= 1<<j;
	}
	return bits;
}


static void R_SetFrustum (void)
{
	int		i;

	// rotate VPN right by FOV_X/2 degrees
	RotatePointAroundVector( frustum[0].normal, vup, vpn, -(90-r_newrefdef.fov_x / 2 ) );
	// rotate VPN left by FOV_X/2 degrees
	RotatePointAroundVector( frustum[1].normal, vup, vpn, 90-r_newrefdef.fov_x / 2 );
	// rotate VPN up by FOV_X/2 degrees
	RotatePointAroundVector( frustum[2].normal, vright, vpn, 90-r_newrefdef.fov_y / 2 );
	// rotate VPN down by FOV_X/2 degrees
	RotatePointAroundVector( frustum[3].normal, vright, vpn, -( 90 - r_newrefdef.fov_y / 2 ) );

	for (i=0 ; i<4 ; i++)
	{
		frustum[i].type = PLANE_ANYZ;
		frustum[i].dist = DotProduct (r_origin, frustum[i].normal);
		frustum[i].signbits = SignbitsForPlane (&frustum[i]);
	}
}

//=======================================================================

/*
===============
R_SetupFrame
===============
*/
static void R_SetupFrame (void)
{
	int i;
	mleaf_t *leaf;

	r_framecount++;

// build the transformation matrix for the given view angles
	VectorCopy (r_newrefdef.vieworg, r_origin);

	AngleVectors (r_newrefdef.viewangles, vpn, vright, vup);

// current viewcluster
	if ( !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
	{
		r_oldviewcluster = r_viewcluster;
		r_oldviewcluster2 = r_viewcluster2;
		r_viewleaf = leaf = Mod_PointInLeaf (r_origin, r_worldmodel);
		r_origin_leafnum = CM_PointLeafnum (r_origin);
		r_viewcluster = r_viewcluster2 = leaf->cluster;

		// check above and below so crossing solid water doesn't draw wrong
		if (!leaf->contents)
		{	// look down a bit
			vec3_t	temp;

			VectorCopy (r_origin, temp);
			temp[2] -= 16;
			r_viewleaf2 = leaf = Mod_PointInLeaf (temp, r_worldmodel);
			if ( !(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
		else
		{	// look up a bit
			vec3_t	temp;

			VectorCopy (r_origin, temp);
			temp[2] += 16;
			r_viewleaf2 = leaf = Mod_PointInLeaf (temp, r_worldmodel);
			if ( !(leaf->contents & CONTENTS_SOLID) &&
				(leaf->cluster != r_viewcluster2) )
				r_viewcluster2 = leaf->cluster;
		}
	}

	for (i=0 ; i<4 ; i++)
		v_blend[i] = r_newrefdef.blend[i];

	c_brush_polys = 0;
	c_alias_polys = 0;

}

void MYgluPerspective( GLdouble fovy, GLdouble aspect,
		     GLdouble zNear, GLdouble zFar )
{
	GLdouble xmin, xmax, ymin, ymax;

	ymax = zNear * tan( fovy * M_PI / 360.0 );
	ymin = -ymax;

	xmin = ymin * aspect;
	xmax = ymax * aspect;

	xmin += -( 2 * gl_state.camera_separation ) / zNear;
	xmax += -( 2 * gl_state.camera_separation ) / zNear;

	qglFrustum( xmin, xmax, ymin, ymax, zNear, zFar );

}



/*
=============
R_SetupViewport
=============
*/
void R_SetupViewport (void)
{
	int		x, y, w, h;

	// The viewport info in r_newrefdef is constructed with the upper left 
	// corner as the origin, whereas glViewport treats the lower left corner
	// as the origin. So we have to do some math to fix the y-coordinates.
	
	x = r_newrefdef.x;
	w = r_newrefdef.width;
	y = viddef.height - r_newrefdef.y - r_newrefdef.height;
	h = r_newrefdef.height;
	qglViewport (x, y, w, h);	// MPO : note this happens every frame interestingly enough
}



/*
=============
R_SetupGL
=============
*/
static void R_SetupGL (void)
{
	float	screenaspect;
	
	R_SetupViewport ();

	//
	// set up projection matrix
	//
    screenaspect = (float)r_newrefdef.width/r_newrefdef.height;
	qglMatrixMode(GL_PROJECTION);
    qglLoadIdentity ();

	if(r_newrefdef.fov_y < 90)
		MYgluPerspective (r_newrefdef.fov_y,  screenaspect,  4,  128000);
	else
		MYgluPerspective(r_newrefdef.fov_y, screenaspect, 4 * 74 / r_newrefdef.fov_y, 15000); //Phenax

	qglCullFace(GL_FRONT);

	qglMatrixMode(GL_MODELVIEW);
    qglLoadIdentity ();

    qglRotatef (-90, 1, 0, 0);	    // put Z going up
    qglRotatef (90,  0, 0, 1);	    // put Z going up

    qglRotatef (-r_newrefdef.viewangles[2],  1, 0, 0);
	qglRotatef (-r_newrefdef.viewangles[0],  0, 1, 0);
	qglRotatef (-r_newrefdef.viewangles[1],  0, 0, 1);
	qglTranslatef (-r_newrefdef.vieworg[0],  -r_newrefdef.vieworg[1],  -r_newrefdef.vieworg[2]);

	qglGetFloatv (GL_MODELVIEW_MATRIX, r_world_matrix);
	qglGetFloatv(GL_PROJECTION_MATRIX, r_project_matrix);
	qglGetIntegerv(GL_VIEWPORT, (int *) r_viewport);

	//
	// set drawing parms
	//

	if (gl_cull->integer)
		qglEnable(GL_CULL_FACE);

	GLSTATE_DISABLE_BLEND
	GLSTATE_DISABLE_ALPHATEST
	qglEnable(GL_DEPTH_TEST);
}

/*
=============
R_Clear
=============
*/
extern cvar_t *info_spectator;
extern cvar_t *cl_add_blend;
extern qboolean have_stencil;
void R_Clear (void)
{
	if (gl_clear->integer)
		qglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	else if (!cl_add_blend->integer && info_spectator->integer && (CM_PointContents(r_newrefdef.vieworg, 0) & CONTENTS_SOLID))
		qglClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //out of map
	else
		qglClear (GL_DEPTH_BUFFER_BIT);

	gldepthmin = 0;
	gldepthmax = 1;
	qglDepthFunc (GL_LEQUAL);

	qglDepthRange (gldepthmin, gldepthmax);

	 //our shadow system uses a combo of shadmaps and stencil volumes.
    if (have_stencil && gl_shadowmaps->integer) {

        qglClearStencil(0);
        qglClear(GL_STENCIL_BUFFER_BIT);
    }
}

/*
================
R_RenderView

r_newrefdef must be set before the first call
================
*/

static void R_DrawTerrainTri (const vec_t *verts[3], const vec3_t normal, qboolean does_intersect)
{
	int i;
	
	GL_EnableTexture (0, false);
	qglDisable (GL_DEPTH_TEST);
	if (does_intersect)
	{
		qglColor4f (1, 0, 0, 1);
		qglLineWidth (5.0);
	}
	else if (normal[2] < 0.0f)
	{
		qglColor4f (0, 0, 1, 1);
		qglLineWidth (2.0);
	}
	else
	{
		qglColor4f (0, 1, 0, 1);
		qglLineWidth (1.0);
	}
	qglBegin (GL_LINE_LOOP);
	for (i = 0; i < 3; i++)
		qglVertex3fv (verts[i]);
	qglEnd ();
	qglEnable (GL_DEPTH_TEST);
	GL_EnableTexture (0, true);
}

void R_SetupFog (float distance_boost)
{
	GLfloat colors[4] = {(GLfloat) fog.red, (GLfloat) fog.green, (GLfloat) fog.blue, (GLfloat) 0.1};
	
	if(map_fog)
	{
		qglFogi(GL_FOG_MODE, GL_LINEAR);
		qglFogfv(GL_FOG_COLOR, colors);
		qglFogf(GL_FOG_START, fog.start * distance_boost);
		qglFogf(GL_FOG_END, fog.end * distance_boost);
		qglFogf(GL_FOG_DENSITY, fog.density);
		qglEnable(GL_FOG);
	}
}

void R_DrawVector (const vec3_t a, const vec3_t b)
{
	GL_EnableTexture (0, false);
	
	qglLineWidth (2.0);
	qglBegin (GL_LINES);
	qglColor4f (0, 0, 1, 1);
	qglVertex3fv (a);
	qglColor4f (1, 0, 0, 1);
	qglVertex3fv (b);
	qglEnd ();
	
	GL_EnableTexture (0, true);
}

void R_DrawBBox (const vec3_t mins, const vec3_t maxs)
{
	vec3_t	minmaxs[2];
	int		side;
	int		square[4][3] = 
		{{0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0}};
	
	GL_EnableTexture (0, false);
	qglColor4f (1, 0, 0, 1);
	qglLineWidth (5.0);
	
	VectorCopy (mins, minmaxs[0]);
	VectorCopy (maxs, minmaxs[1]);
	
	for (side = 0; side < 6; side++)
	{
		int vertnum;
		qglBegin (GL_LINE_LOOP);
		for (vertnum = 0; vertnum < 4; vertnum++)
		{
			int axisnum;
			vec3_t vert;
			for (axisnum = 0; axisnum < 3; axisnum++)
			{
				int axis = (axisnum + (side >> 1)) % 3;
				vert[axisnum] = minmaxs[square[vertnum][axis] != (side & 1)][axisnum];
			}
			qglVertex3fv (vert);
		}
		qglEnd ();
	}
	
	GL_EnableTexture (0, true);
}

static void R_DrawEntityBBox (entity_t *e)
{
	qglPushMatrix ();

	R_RotateForEntity (e);
	
	R_DrawBBox (e->model->mins, e->model->maxs);
	
	qglPopMatrix ();
	GL_EnableTexture (0, true);
}

void R_RenderView (refdef_t *fd)
{
	vec3_t forward;

	numRadarEnts = 0;

	if (r_norefresh->integer)
		return;

	r_newrefdef = *fd;

	//shadowmaps
	if(gl_shadowmaps->integer)
	{
		R_GenerateGlobalShadows ();
	}

	if (!r_worldmodel && !( r_newrefdef.rdflags & RDF_NOWORLDMODEL ) )
		Com_Error (ERR_DROP, "R_RenderView: NULL worldmodel");
	
	// init r_speeds counters
	last_c_brush_polys = c_brush_polys;
	last_c_alias_polys = c_alias_polys;
	c_brush_polys = 0;
	c_alias_polys = 0;
	c_flares = 0;
	c_grasses = 0;
	c_beams = 0;
	c_vbo_batches = 0;
	
	num_rendered_models = 0;

	R_PushDlights ();

	R_SetupFrame ();
	R_UpdateDlights ();

	R_SetFrustum ();
	
	R_MarkWorldSurfs ();	// done here so we know if we're in water
	
	if (gl_finish->integer)
		qglFinish ();
	
	// OpenGL calls come after here

	R_SetupGL ();

	R_SetupFog (1);
	
	//qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, pp_FBO);
	
	R_DrawWorldSurfs ();
	
	R_DrawTerrain ();

	if(r_lensflare->integer)
		R_RenderFlares ();

	R_DrawEntitiesOnList ();
	
	if (r_vegetation->integer)
		R_DrawVegetationSurface ();

	R_DrawSimpleItems ();

	R_SimulateAllRagdolls(); 
	R_DrawRagdollsOnList (); 
	
	R_DrawViewEntitiesOnList ();

	R_DrawAlphaSurfaces ();

	if (r_lightbeam->integer)
		R_DrawBeamSurface ();

	R_DrawParticles ();

	if(gl_mirror->integer) 
	{
		int ms = Sys_Milliseconds ();
		if (	ms < r_newrefdef.last_mirrorupdate_time || 
				(ms-r_newrefdef.last_mirrorupdate_time) >= 16)
		{
			GL_SelectTexture (0);
			GL_Bind (r_mirrortexture->texnum);
			qglCopyTexSubImage2D(GL_TEXTURE_2D, 0,
						0, 0, 0, r_mirrortexture->upload_height/2, 
						r_mirrortexture->upload_width, 
						r_mirrortexture->upload_height);
			r_newrefdef.last_mirrorupdate_time = ms;
		}
	}

	// Postprocessing
	R_BloomBlend (&r_newrefdef);
	R_RenderSun ();
	//qglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER, 0);
	R_GLSLPostProcess ();
	R_Flash ();
	
	AngleVectors (r_newrefdef.viewangles, forward, NULL, NULL);
	
	if (gl_showcollisionmesh->integer)
	{
		qglColor4f (0.0f, 1.0f, 0.0f, 1.0f);
		qglDisable (GL_TEXTURE_2D);
		qglPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
	
		if (gl_showcollisionmesh->integer >= 2) qglDisable (GL_DEPTH_TEST);
		qglLineWidth (gl_showcollisionmesh->integer >= 2 ? 2.5f : 1.5f); // when there are lots of lines, make them narrower
		
		GL_DrawCollisionMesh ();
		
		if (gl_showcollisionmesh->integer >= 2) qglEnable (GL_DEPTH_TEST);
	
		qglLineWidth (1.0f);
		qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
		qglEnable (GL_TEXTURE_2D);
	}
	
	if (gl_showdecals->integer)
	{	
		int i;
		
		for (i = 0; i < num_decal_entities; i++)
			R_DrawEntityBBox (&decal_entities[i]);
	}
	if (r_tracetest->integer > 0)
	{
		int		i;
		vec3_t	targ;
		VectorMA (r_origin, r_tracetestlength->value, forward, targ);
		for (i = 0; i < r_tracetest->integer; i++)
			CM_BoxTrace (r_origin, targ, vec3_origin, vec3_origin, r_worldmodel->firstnode, MASK_OPAQUE);
	}
	if (r_fasttracetest->integer > 0)
	{
		int		i;
		vec3_t	targ;
		VectorMA (r_origin, r_tracetestlength->value, forward, targ);
		for (i = 0; i < r_fasttracetest->integer; i++)
			CM_FastTrace (r_origin, targ, r_worldmodel->firstnode, MASK_OPAQUE);
	}
	if (r_tracebox->integer)
	{
		vec3_t	mins = {-16, -16, -24};
		vec3_t	maxs = {16, 16, 32};
		vec3_t start, targ;
		trace_t t;

		VectorCopy (r_origin, start);
		start[2] -= 22;
		VectorMA (start, r_tracetestlength->value, forward, targ);
		t = CM_BoxTrace (start, targ, mins, maxs, r_worldmodel->firstnode, MASK_PLAYERSOLID);
		VectorAdd (mins, t.endpos, mins);
		VectorAdd (maxs, t.endpos, maxs);
		R_DrawBBox (mins, maxs);
	}
	if (*r_showbox->string)
	{
		vec3_t	mins = {-16, -16, -24};
		vec3_t	maxs = {16, 16, 32};
		vec3_t	ctr;
		sscanf (r_showbox->string, "%f %f %f", ctr, ctr + 1, ctr + 2);
		VectorAdd (mins, ctr, mins);
		VectorAdd (maxs, ctr, maxs);
		R_DrawBBox (mins, maxs);
		R_DrawMark (ctr, 3.0f, RGBA (1, 0, 0, 1));
	}

	if(map_fog)
		qglDisable(GL_FOG);

	R_DrawRadar();
	
	*fd = r_newrefdef;
}

static void	R_SetGL2D (void)
{
	// set 2D virtual screen size
	qglViewport (0, 0, viddef.width, viddef.height);
	qglMatrixMode(GL_PROJECTION);
	qglLoadIdentity ();
	qglOrtho  (0, viddef.width, viddef.height, 0, -99999, 99999);
	qglMatrixMode(GL_MODELVIEW);
	qglLoadIdentity ();
	qglDisable (GL_DEPTH_TEST);
	qglDisable (GL_CULL_FACE);
	GLSTATE_DISABLE_BLEND
	GLSTATE_ENABLE_ALPHATEST
	qglColor4f (1,1,1,1);
}

/*
@@@@@@@@@@@@@@@@@@@@@
R_RenderFrame

@@@@@@@@@@@@@@@@@@@@@
*/
#include "r_text.h"
extern FNT_auto_t	CL_gameFont;
void R_TransformVectorToScreen( refdef_t *rd, vec3_t in, vec2_t out );
void R_RenderFrame (refdef_t *fd)
{
	R_RenderView( fd );

	R_SetGL2D ();
	
	if (r_showpolycounts->integer)
	{
		vec3_t				above_mod;
		FNT_font_t			font;
		struct FNT_window_s	box;
		vec2_t				screen_pos;
		int					i;
		
		font = FNT_AutoGet (CL_gameFont);
		
		for (i = 0; i < num_rendered_models; i++)
		{
			VectorCopy (rendered_models[i].ent->origin, above_mod);
			above_mod[2] += rendered_models[i].mod->maxs[2];
			R_TransformVectorToScreen (&r_newrefdef, above_mod, screen_pos);
			box.x = (int)screen_pos[0];
			box.y = r_newrefdef.height-(int)screen_pos[1]-font->height;
			box.width = box.height = 0;
			FNT_BoundedPrint (font, va ("%d", rendered_models[i].mod->num_triangles), FNT_CMODE_QUAKE_SRS,
				FNT_ALIGN_LEFT, &box, FNT_colors[2]);
		}
	}
}

// FIXME HACK: this should really just setup the frustum and viewport, then 
// do a single call to R_Mesh_Draw. RDF_NOWORLDMODEL needs to go away, it's
// just as annoying as having ifdefs everywhere.
void R_RenderFramePlayerSetup( refdef_t *rfdf )
{

	numRadarEnts = 0;

	r_newrefdef = *rfdf;

	R_SetupFrame();
	R_UpdateDlights ();
	R_SetFrustum();
	R_SetupGL();
	qglClear (GL_DEPTH_BUFFER_BIT);
	R_DrawEntitiesOnList();

	R_SetGL2D();
	
	// stop the set rdflags from messing up the map loading process.
	memset (&r_newrefdef, 0, sizeof(r_newrefdef));

}

void R_Register( void )
{

	con_font = Cvar_Get ("con_font", "default", CVAR_ARCHIVE);

	r_lefthand = Cvar_Get( "hand", "0", CVAR_USERINFO | CVAR_ARCHIVE | CVARDOC_INT);
	Cvar_Describe (r_lefthand, "0 means show gun on the right, 1 means show gun on the left, 2 means hide the gun altogether.");
	r_norefresh = Cvar_Get ("r_norefresh", "0", 0);
	r_fullbright = Cvar_Get ("r_fullbright", "0", 0);
	r_drawentities = Cvar_Get ("r_drawentities", "1", 0);
	r_drawworld = Cvar_Get ("r_drawworld", "1", 0);
	r_novis = Cvar_Get ("r_novis", "0", 0);
	r_nocull = Cvar_Get ("r_nocull", "0", 0);
	r_lerpmodels = Cvar_Get ("r_lerpmodels", "1", 0);

	gl_nosubimage = Cvar_Get( "gl_nosubimage", "0", 0 );

	gl_modulate = Cvar_Get ("gl_modulate", "2", CVAR_ARCHIVE|CVARDOC_FLOAT);
	Cvar_Describe (gl_modulate, "Brightness setting. Higher means brighter.");
	gl_bitdepth = Cvar_Get( "gl_bitdepth", "0", 0 );
	gl_mode = Cvar_Get( "gl_mode", "3", CVAR_ARCHIVE );
	gl_lightmap = Cvar_Get ("gl_lightmap", "0", 0);
	gl_nobind = Cvar_Get ("gl_nobind", "0", 0);
	gl_picmip = Cvar_Get ("gl_picmip", "0", CVAR_ARCHIVE|CVARDOC_INT);
	Cvar_Describe (gl_picmip, "Texture detail. 0 means full detail. Each higher setting has 1/4 less detail.");
	gl_skymip = Cvar_Get ("gl_skymip", "0", 0);
	gl_showtris = Cvar_Get ("gl_showtris", "0", 0);
	gl_showpolys = Cvar_Get ("gl_showpolys", "0", CVARDOC_INT);
	Cvar_Describe (gl_showpolys, "Useful tool for mappers. 1 means show world polygon outlines for visible surfaces. 2 means show outlines for all surfaces in the PVS, even if they are hidden.");
	gl_showcollisionmesh = Cvar_Get ("gl_showcollisionmesh", "0", CVARDOC_INT);
	Cvar_Describe (gl_showcollisionmesh, "Useful tool for mappers. Shows all the collision geometry for terrain in wireframe. 2 disables depth culling for this wireframe.");
	gl_finish = Cvar_Get ("gl_finish", "0", CVAR_ARCHIVE|CVARDOC_BOOL);
	Cvar_Describe (gl_finish, "Waits for graphics driver to finish drawing each frame before drawing the next one. Hurts performance but may improve smoothness on very low-end machines.");
	gl_clear = Cvar_Get ("gl_clear", "0", CVARDOC_BOOL);
	gl_cull = Cvar_Get ("gl_cull", "1", CVARDOC_BOOL);
	Cvar_Describe (gl_cull, "Avoid rendering anything that's off the edge of the screen. Good for performance, recommend leaving it on.");
	gl_polyblend = Cvar_Get ("gl_polyblend", "1", 0);

// OPENGL_DRIVER defined by in config.h
	gl_driver = Cvar_Get( "gl_driver", OPENGL_DRIVER, 0 );

	gl_texturemode = Cvar_Get( "gl_texturemode", "GL_LINEAR_MIPMAP_LINEAR", CVAR_ARCHIVE );
	gl_texturealphamode = Cvar_Get( "gl_texturealphamode", "default", CVAR_ARCHIVE );
	gl_texturesolidmode = Cvar_Get( "gl_texturesolidmode", "default", CVAR_ARCHIVE );
	gl_lockpvs = Cvar_Get( "gl_lockpvs", "0", 0 );

	gl_drawbuffer = Cvar_Get( "gl_drawbuffer", "GL_BACK", 0 );
	gl_swapinterval = Cvar_Get( "gl_swapinterval", "1", CVAR_ARCHIVE|CVARDOC_BOOL );
	Cvar_Describe (gl_swapinterval, "Sync to Vblank. Eliminates \"tearing\" effects, but it can hurt framerates.");

	r_shaders = Cvar_Get ("r_shaders", "1", CVAR_ARCHIVE|CVARDOC_BOOL);

	r_overbrightbits = Cvar_Get( "r_overbrightbits", "2", CVAR_ARCHIVE );

	gl_mirror = Cvar_Get("gl_mirror", "1", CVAR_ARCHIVE|CVARDOC_BOOL);

	// see windowmode_t;
	// 0 = windowed
	// 1 = borderless windowed
	// 2 = exclusive fullscreen
	vid_fullscreen = Cvar_Get( "vid_fullscreen", "1", CVAR_ARCHIVE|CVARDOC_INT);

	// Last used fullscreen mode: borderless windowed (1) or exclusive fullscreen (2).
	// Used for toggling between fullscreen and windowed with alt-enter.
	vid_preferred_fullscreen = Cvar_Get( "vid_preferred_fullscreen", "1", CVAR_ARCHIVE|CVARDOC_INT);

	vid_gamma = Cvar_Get( "vid_gamma", "1.0", CVAR_ARCHIVE );
	vid_contrast = Cvar_Get( "vid_contrast", "1.0", CVAR_ARCHIVE);
	//TODO: remove, unless we decide to add GL ES support or something.
	vid_ref = Cvar_Get( "vid_ref", "gl", CVAR_ARCHIVE|CVAR_ROM ); 

	gl_vlights = Cvar_Get("gl_vlights", "1", CVAR_ARCHIVE);

	// MSAA samples: 0 = off, 2, 4, 8, 16
	r_antialiasing = Cvar_Get( "r_antialiasing", "0", CVAR_ARCHIVE|CVARDOC_INT );
	
	r_meshnormalmaps = Cvar_Get("r_meshnormalmaps", "1", CVAR_ARCHIVE|CVARDOC_BOOL);
	r_worldnormalmaps = Cvar_Get("r_worldnormalmaps", "0", CVAR_ARCHIVE|CVARDOC_BOOL);
	gl_shadowmaps = Cvar_Get("gl_shadowmaps", "0", CVAR_ARCHIVE|CVARDOC_BOOL);
	gl_fog = Cvar_Get ("gl_fog", "1", CVAR_ARCHIVE|CVARDOC_BOOL);
	Cvar_Describe (gl_fog, "Fog and weather effects.");

	r_shadowmapscale = Cvar_Get( "r_shadowmapscale", "1", CVAR_ARCHIVE );
	r_shadowcutoff = Cvar_Get( "r_shadowcutoff", "880", CVAR_ARCHIVE );

	r_lensflare = Cvar_Get( "r_lensflare", "1", CVAR_ARCHIVE|CVARDOC_BOOL);
	r_lensflare_intens = Cvar_Get ("r_lensflare_intens", "3", CVAR_ARCHIVE|CVARDOC_INT);
	r_vegetation = Cvar_Get ("r_vegetation", "1", CVAR_ARCHIVE|CVARDOC_BOOL);
	r_drawsun =	Cvar_Get("r_drawsun", "2", CVAR_ARCHIVE);
	r_lightbeam = Cvar_Get ("r_lightbeam", "1", CVAR_ARCHIVE|CVARDOC_BOOL);
	r_godrays = Cvar_Get ("r_godrays", "0", CVAR_ARCHIVE|CVARDOC_BOOL);
	r_godray_intensity = Cvar_Get ("r_godray_intensity", "1.0", CVAR_ARCHIVE);
	r_optimize = Cvar_Get ("r_optimize", "1", CVAR_ARCHIVE|CVARDOC_BOOL);
	Cvar_Describe (r_optimize, "Skip BSP recursion unless you move. Good for performance, recommend leaving it on.");
	
	r_lightmapfiles = Cvar_Get("r_lightmapfiles", "1", CVAR_ARCHIVE|CVARDOC_BOOL);
	Cvar_Describe (r_lightmapfiles, "Enables the loading of .lightmap files, with more detailed light and shadow. Turn this off if video RAM is limited.");

	r_minimap_size = Cvar_Get ("r_minimap_size", "256", CVAR_ARCHIVE );
	r_minimap_zoom = Cvar_Get ("r_minimap_zoom", "1", CVAR_ARCHIVE );
	r_minimap_style = Cvar_Get ("r_minimap_style", "1", CVAR_ARCHIVE );
	r_minimap = Cvar_Get ("r_minimap", "0", CVAR_ARCHIVE|CVARDOC_BOOL );

	r_ragdolls = Cvar_Get ("r_ragdolls", "1", CVAR_ARCHIVE|CVARDOC_BOOL);
	r_ragdoll_debug = Cvar_Get("r_ragdoll_debug", "0", CVAR_ARCHIVE|CVARDOC_BOOL);

	sys_priority = Cvar_Get("sys_priority", "0", CVAR_ARCHIVE);
	sys_affinity = Cvar_Get("sys_affinity", "1", CVAR_ARCHIVE);

	gl_screenshot_type = Cvar_Get("gl_screenshot_type", "jpeg", CVAR_ARCHIVE|CVARDOC_STR);
	gl_screenshot_jpeg_quality = Cvar_Get("gl_screenshot_jpeg_quality", "85", CVAR_ARCHIVE|CVARDOC_INT);

	r_firstrun = Cvar_Get("r_firstrun", "0", CVAR_ARCHIVE|CVARDOC_BOOL); //first time running the game
	Cvar_Describe (r_firstrun, "Set this to 0 if you want the game to auto detect your graphics settings next time you run it.");

	r_test = Cvar_Get("r_test", "0", CVAR_ARCHIVE); //for testing things
	r_tracetest = Cvar_Get("r_tracetest", "0", CVARDOC_INT); // BoxTrace performance test
	Cvar_Describe (r_tracetest, "Performs a number of CM_BoxTrace operations starting at the player's origin along the player's view vector for up to r_tracetestlength units. The value of the cvar controls how many traces to do. Useful for profiling & benchmarking.");
	r_fasttracetest = Cvar_Get("r_fasttracetest", "0", CVARDOC_INT); // FastTace performance test
	Cvar_Describe (r_fasttracetest, "Performs a number of CM_FastTrace operations starting at the player's origin along the player's view vector for up to r_tracetestlength units. The value of the cvar controls how many traces to do. Useful for profiling & benchmarking.");
	r_tracetestlength = Cvar_Get("r_tracetestlength", "8192", CVARDOC_FLOAT);
	Cvar_Describe (r_tracetestlength, "Length in game units of the traces for r_tracetest, r_fasttracetest, and r_tracebox.");
	r_showbox = Cvar_Get("r_showbox", "", 0);
	Cvar_Describe (r_showbox, "Shows a box the size of the player's bounding box at the specified coordinates. Use a value of the form \"X Y Z\" including the quotes. Useful for visualizing where a certain coordinate is.");
	r_tracebox = Cvar_Get("r_tracebox", "", CVARDOC_BOOL);
	Cvar_Describe (r_tracebox, "Trace from the player's origin along the player's view vector for up to r_tracetestlength units or until an obstacle is reached. Shows a box the size of the player's bounding box at the obstacle.");
	
	gl_showdecals = Cvar_Get("gl_showdecals", "0", CVARDOC_INT);
	Cvar_Describe (gl_showdecals, "Set this to 1 to show terrain decal bounding boxes. Set this to 2 to show terrain decals in wireframe.");
	
	r_showpolycounts = Cvar_Get("r_showpolycounts", "0", CVARDOC_BOOL);
	Cvar_Describe (r_showpolycounts, "Set this to 1 to show the polycount of each mesh, hovering over the mesh itself.");
	
	// FIXME HACK copied over from the video menu code. These are initialized
	// again elsewhere. TODO: work out any complications that may arise from
	// deleting these duplicate initializations.
	Cvar_Get( "r_bloom", "0", CVAR_ARCHIVE );
	Cvar_Get( "r_bloom_intensity", "0.75", CVAR_ARCHIVE);
	Cvar_Get( "r_overbrightbits", "2", CVAR_ARCHIVE);
	Cvar_Get( "vid_width", "640", CVAR_ARCHIVE);
	Cvar_Get( "vid_height", "400", CVAR_ARCHIVE);

	Cmd_AddCommand( "imagelist", GL_ImageList_f );
	Cmd_AddCommand( "screenshot", GL_ScreenShot_f );
	Cmd_AddCommand( "modellist", Mod_Modellist_f );
	Cmd_AddCommand( "gl_strings", GL_Strings_f );
}

/*

==================
R_SetMode
==================
*/
static qboolean R_SetMode (void)
{
	rserr_t err;
	windowmode_t windowmode;

	windowmode = vid_fullscreen->integer;

	vid_fullscreen->modified = false;
	gl_mode->modified = false;

	if ( ( err = GLimp_SetMode( &vid.width, &vid.height, gl_mode->integer, windowmode ) ) == rserr_ok )
	{
		gl_state.prev_mode = gl_mode->integer;
	}
	else
	{
		if ( err == rserr_invalid_fullscreen )
		{
			Cvar_SetValue( "vid_fullscreen", windowmode_windowed);
			vid_fullscreen->modified = false;
			Com_Printf ("ref_gl::R_SetMode() - fullscreen unavailable in this mode\n" );
			if ( ( err = GLimp_SetMode( &vid.width, &vid.height, gl_mode->integer, windowmode_windowed ) ) == rserr_ok )
				return true;
		}
		else if ( err == rserr_invalid_mode )
		{
			Cvar_SetValue( "gl_mode", gl_state.prev_mode );
			gl_mode->modified = false;
			Com_Printf ("ref_gl::R_SetMode() - invalid mode\n" );
		}

		// try setting it back to something safe
		if ( ( err = GLimp_SetMode( &vid.width, &vid.height, gl_state.prev_mode, windowmode_windowed ) ) != rserr_ok )
		{
			Com_Printf ("ref_gl::R_SetMode() - could not revert to safe mode\n" );
			return false;
		}
	}
	return true;
}

static void R_SetGraphicalPreset (const char *cfgname, const char *desc)
{
	Cmd_ExecuteString (va ("exec graphical_presets/%s.cfg", cfgname));
	Cbuf_Execute ();
	Com_Printf("...autodetected %s game setting\n", desc);
}

#if defined WIN32_VARIANT
double CPUSpeed()
{
	DWORD BufSize = _MAX_PATH;
	DWORD dwMHz = _MAX_PATH;
	HKEY hKey;	// open the key where the proc speed is hidden:

	long lError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
		0,
		KEY_READ,
		&hKey);

	if(lError != ERROR_SUCCESS)
		return 0;

	// query the key:
	RegQueryValueEx(hKey, "~MHz", NULL, NULL, (LPBYTE) &dwMHz, &BufSize);
	return (double)dwMHz;
}
#endif

/*
===============
R_Init
===============
*/
int R_Init( void *hinstance, void *hWnd )
{
	int		err;
	
	Draw_GetPalette ();

	R_Register();

	// initialize our QGL dynamic bindings
	if ( !QGL_Init( gl_driver->string ) )
	{
		QGL_Shutdown();
		Com_Printf ("ref_gl::R_Init() - could not load \"%s\"\n", gl_driver->string );
		return -1;
	}

	// initialize OS-specific parts of OpenGL
	if ( !GLimp_Init( hinstance, hWnd ) )
	{
		QGL_Shutdown();
		return -1;
	}
	
	// reset GL_BlendFunc state variables.
	gl_state.bFunc1 = gl_state.bFunc2 = -1;

	// set our "safe" modes
	gl_state.prev_mode = 3;

	// create the window and set up the context
	if ( !R_SetMode () )
	{
		QGL_Shutdown();
		Com_Printf ("ref_gl::R_Init() - could not R_SetMode()\n" );
		return -1;
	}

	// Initialise TrueType fonts
	if ( ! FNT_Initialise( ) ) {
		QGL_Shutdown( );
		Com_Printf( "ref_gl::R_Init() - could not initialise text drawing front-end\n" );
		return -1;
	}

	/*
	** get our various GL strings
	*/
	gl_config.vendor_string = (const char*)qglGetString (GL_VENDOR);
	Com_Printf ("GL_VENDOR: %s\n", gl_config.vendor_string );
	gl_config.renderer_string = (const char*)qglGetString (GL_RENDERER);
	Com_Printf ("GL_RENDERER: %s\n", gl_config.renderer_string );
	gl_config.version_string = (const char*)qglGetString (GL_VERSION);
	Com_Printf ("GL_VERSION: %s\n", gl_config.version_string );
	gl_config.extensions_string = (const char*)qglGetString (GL_EXTENSIONS);

	GL_PrintExtensions();

	/*
	** grab extensions
	*/
	if ( GL_QueryExtension("GL_EXT_compiled_vertex_array")
		 || GL_QueryExtension("GL_SGI_compiled_vertex_array") )
	{
		Com_Printf ("...enabling GL_EXT_compiled_vertex_array\n" );
		qglLockArraysEXT = ( void * ) qwglGetProcAddress( "glLockArraysEXT" );
		qglUnlockArraysEXT = ( void * ) qwglGetProcAddress( "glUnlockArraysEXT" );
	}
	else
	{
		Com_Printf ("...GL_EXT_compiled_vertex_array not found\n" );
	}

#if defined WIN32_VARIANT
	if ( GL_QueryExtension("WGL_EXT_swap_control") )
	{
		qwglSwapIntervalEXT = ( BOOL (WINAPI *)(int)) qwglGetProcAddress( "wglSwapIntervalEXT" );
		Com_Printf ("...enabling WGL_EXT_swap_control\n" );
	}
	else
	{
		Com_Printf ("...WGL_EXT_swap_control not found\n" );
	}
#endif

	R_InitImageSubsystem();

	R_LoadVBOSubsystem();

	//always do this check for ATI drivers - they are somewhat bugged in regards to shadowmapping and use of shadow2dproj command
	if(!strcmp(gl_config.vendor_string, "ATI Technologies Inc."))
		gl_state.ati = true;

	//load shader programs
	R_LoadGLSLPrograms();

	//if running for the very first time, automatically set video settings
	// TODO: Maybe change/update some of this logic a bit?
	if(!r_firstrun->integer)
	{
		qboolean ati_nvidia = false;
		double CPUTotalSpeed = 4000.0; //default to this
		double OGLVer = atof(&gl_config.version_string[0]);
		//int OGLSubVer = atoi(&gl_config.version_string[2]);

#if defined WIN32_VARIANT
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);

		Com_Printf("...CPU: %4.2f Cores: %d\n", CPUSpeed(), sysInfo.dwNumberOfProcessors);

		CPUTotalSpeed = sysInfo.dwNumberOfProcessors * CPUSpeed();

#else
		FILE	*fp;
        char	res[128];
		int		cores;
		size_t	szrslt;
		int     irslt;
        fp = popen("/bin/cat /proc/cpuinfo | grep -c '^processor'","r");
        if ( fp == NULL )
        	goto cpuinfo_error;
        szrslt = fread(res, 1, sizeof(res)-1, fp);
        res[szrslt] = 0;
        pclose(fp);
        if ( !szrslt )
        	goto cpuinfo_error;
		cores = atoi( &res[0] );
		fp = popen("/bin/cat /proc/cpuinfo | grep '^cpu MHz'","r");
		if ( fp == NULL )
			goto cpuinfo_error;
        szrslt = fread(res, 1, sizeof(res)-1, fp);  // about 20 bytes/cpu
        res[szrslt] = 0;
		pclose(fp);
		if ( !szrslt )
			goto cpuinfo_error;
		irslt = sscanf( res, "cpu MHz : %lf", &CPUTotalSpeed );
		if ( !irslt )
			goto cpuinfo_error;
		Com_Printf("...CPU: %4.2f Cores: %d\n", CPUTotalSpeed, cores);
	    CPUTotalSpeed *= cores;
	    goto cpuinfo_exit;
cpuinfo_error:
		Com_Printf("...Reading /proc/cpuinfo failed.\n");
cpuinfo_exit:
#endif

		//check to see if we are using ATI or NVIDIA, otherwise, we don't want to
		//deal with high settings on offbrand GPU's like Intel or Unichrome
		if(!strcmp(gl_config.vendor_string, "ATI Technologies Inc.") || !strcmp(gl_config.vendor_string, "NVIDIA Corporation"))
			ati_nvidia = true;

		if(OGLVer < 2.1)
		{
			//weak GPU, set to maximum compatibility
			R_SetGraphicalPreset ("compatibility", "MAX COMPATIBILITY");
		}
		else if(OGLVer >= 3)
		{
			//GPU is modern, check CPU
			if(CPUTotalSpeed > 3800.0 && ati_nvidia)
				R_SetGraphicalPreset ("maxquality", "MAX QUALITY");
			else
				R_SetGraphicalPreset ("quality", "QUALITY");
		}
		else 
		{
			if(CPUTotalSpeed > 3800.0 && ati_nvidia)
				R_SetGraphicalPreset ("quality", "QUALITY");
			else
				R_SetGraphicalPreset ("performance", "PERFORMANCE");
		}

		//never run again
		Cvar_SetValue("r_firstrun", 1);
	}

	GL_SetDefaultState();
	
	R_CheckFBOExtensions ();

	Com_DPrintf("%s : %d\n", __FILE__, __LINE__ );

	GL_InitImages ();
	Com_DPrintf("%s : %d\n", __FILE__, __LINE__ );

	Mod_Init ();
	Com_DPrintf("%s : %d\n", __FILE__, __LINE__ );

	R_InitParticleTexture ();
	Com_DPrintf("%s : %d\n", __FILE__, __LINE__ );

	Draw_InitLocal ();
	Com_DPrintf("%s : %d\n", __FILE__, __LINE__ );

	R_GenerateShadowFBO();
	Com_DPrintf("%s : %d\n", __FILE__, __LINE__ );

	//Initialize ODE
	// ODE assert failures sometimes occur, this may or may not help.
	// see in ODE: include/odeinit.h
	r_odeinit_success = dInitODE2( dAllocateMaskAll );
	//ODE - clear out any ragdolls;
	if ( r_odeinit_success )
	{
		Com_Printf("...ODE initialized.\n");
		R_ClearAllRagdolls();
		Com_Printf("...Ragdolls initialized.\n");
	}
	else
	{
		Com_Printf("...ODE initialization failed.\n...Ragdolls are disabled.\n");
	}

	scr_playericonalpha = 0.0;

	err = qglGetError();
	if ( err != GL_NO_ERROR )
		Com_Printf ("glGetError() = 0x%x\n", err);
	
	GL_InvalidateTextureState ();


	return 0;
}

/*
===============
R_Shutdown
===============
*/
void R_Shutdown (void)
{
	Cmd_RemoveCommand ("modellist");
	Cmd_RemoveCommand ("screenshot");
	Cmd_RemoveCommand ("imagelist");
	Cmd_RemoveCommand ("gl_strings");

	Mod_FreeAll ();
	
	R_VCShutdown();

	FNT_Shutdown( );
	GL_ShutdownImages ();

	/*
	** shut down OS specific OpenGL stuff like contexts, etc.
	*/
	GLimp_Shutdown();

	/*
	** shutdown our QGL subsystem
	*/
	glDeleteObjectARB( g_vertexShader );
	glDeleteObjectARB( g_fragmentShader );
	// FIXME: should we be deleting the rest of them?
	glDeleteObjectARB( *g_worldprogramObj );

	QGL_Shutdown();

	//Shutdown ODE
	// ODE might fail assert on close when not intialized
	if ( r_odeinit_success )
	{
		Com_DPrintf("Closing ODE\n");
		// see in ODE: include/odeinit.h
		dCloseODE();
	}
}



/*
@@@@@@@@@@@@@@@@@@@@@
R_BeginFrame
@@@@@@@@@@@@@@@@@@@@@
*/
void R_BeginFrame (float camera_separation)
{
#if defined UNIX_VARIANT
	extern qboolean vid_restart;
#endif

	gl_state.camera_separation = camera_separation;

	/*
	** change modes if necessary
	*/
	if ( gl_mode->modified || vid_fullscreen->modified )
	{	// FIXME: only restart if CDS is required
		cvar_t	*ref;

		ref = Cvar_Get ("vid_ref", "gl", 0);
#if defined UNIX_VARIANT
		vid_restart = true;
#else
		ref->modified = true;
#endif		
	}

	GLimp_BeginFrame( camera_separation );

	/*
	** go into 2D mode
	*/
	R_SetGL2D ();

	/*
	** draw buffer stuff
	*/
	if ( gl_drawbuffer->modified )
	{
		gl_drawbuffer->modified = false;

		if ( gl_state.camera_separation == 0 || !gl_state.stereo_enabled )
		{
			if ( Q_strcasecmp( gl_drawbuffer->string, "GL_FRONT" ) == 0 )
				qglDrawBuffer( GL_FRONT );
			else
				qglDrawBuffer( GL_BACK );
		}
	}

	/*
	** texturemode stuff
	*/
	if ( gl_texturemode->modified )
	{
		GL_TextureMode( gl_texturemode->string );
		gl_texturemode->modified = false;
	}

	if ( gl_texturealphamode->modified )
	{
		GL_TextureAlphaMode( gl_texturealphamode->string );
		gl_texturealphamode->modified = false;
	}

	if ( gl_texturesolidmode->modified )
	{
		GL_TextureSolidMode( gl_texturesolidmode->string );
		gl_texturesolidmode->modified = false;
	}

	/*
	** swapinterval stuff
	*/
	GL_UpdateSwapInterval();

	//
	// clear screen if desired
	//
	R_Clear ();
}

/*
=============
R_AppActivate
=============
*/
void R_AppActivate (qboolean active)
{
	GLimp_AppActivate (active);
}

/*
=============
R_EndFrame
=============
*/
void R_EndFrame (void)
{
	 GLimp_EndFrame ();
}

/*
===============
R_FarClip
===============
*/
static float R_FarClip (void)
{
	float farclip, farclip_dist;
	int i;
	vec_t mins[4];
	vec_t maxs[4];
	vec_t dist;

	farclip_dist = DotProduct( r_origin, vpn );

	if(r_farclip_min > 256.0f)
		farclip = farclip_dist + r_farclip_min;
	else
		farclip = farclip_dist + 256.0f;

	if( r_worldmodel && !(r_newrefdef.rdflags & RDF_NOWORLDMODEL) ) {

		for (i = 0; i < 3; i++) {
			mins[i] = r_worldmodel->nodes[0].minmaxs[i];
			maxs[i] = r_worldmodel->nodes[0].minmaxs[3+i];
		}
		dist = (vpn[0] < 0 ? mins[0] : maxs[0]) * vpn[0] +
			(vpn[1] < 0 ? mins[1] : maxs[1]) * vpn[1] +
			(vpn[2] < 0 ? mins[2] : maxs[2]) * vpn[2];
		if( dist > farclip )
			farclip = dist;
	}

	if((farclip - farclip_dist + r_farclip_bias) > r_farclip)
		return ( farclip - farclip_dist + r_farclip_bias);
	else
		return r_farclip;
}

/*
=============
R_SetupProjectionMatrix
=============
*/
static void R_SetupProjectionMatrix (refdef_t *rd, mat4x4_t m)
{
	double xMin, xMax, yMin, yMax, zNear, zFar;

	r_farclip = R_FarClip ();

	zNear = 4;
	zFar = r_farclip;

	yMax = zNear * tan( rd->fov_y * M_PI / 360.0 );
	yMin = -yMax;

	xMin = yMin * rd->width / rd->height;
	xMax = yMax * rd->width / rd->height;

	xMin += -( 2 * gl_state.camera_separation ) / zNear;
	xMax += -( 2 * gl_state.camera_separation ) / zNear;

	m[0] = (2.0 * zNear) / (xMax - xMin);
	m[1] = 0.0f;
	m[2] = 0.0f;
	m[3] = 0.0f;
	m[4] = 0.0f;
	m[5] = (2.0 * zNear) / (yMax - yMin);
	m[6] = 0.0f;
	m[7] = 0.0f;
	m[8] = (xMax + xMin) / (xMax - xMin);
	m[9] = (yMax + yMin) / (yMax - yMin);
	m[10] = -(zFar + zNear) / (zFar - zNear);
	m[11] = -1.0f;
	m[12] = 0.0f;
	m[13] = 0.0f;
	m[14] = -(2.0 * zFar * zNear) / (zFar - zNear);
	m[15] = 0.0f;
}
/*
=============
R_SetupModelviewMatrix
=============
*/
static void R_SetupModelviewMatrix (refdef_t *rd, mat4x4_t m)
{
	Vector4Set( &m[0], 0, 0, -1, 0 );
	Vector4Set( &m[4], -1, 0, 0, 0 );
	Vector4Set( &m[8], 0, 1, 0, 0 );
	Vector4Set( &m[12], 0, 0, 0, 1 );

	Matrix4_Rotate( m, -rd->viewangles[2], 1, 0, 0 );
	Matrix4_Rotate( m, -rd->viewangles[0], 0, 1, 0 );
	Matrix4_Rotate( m, -rd->viewangles[1], 0, 0, 1 );
	Matrix4_Translate( m, -rd->vieworg[0], -rd->vieworg[1], -rd->vieworg[2] );
}

void R_TransformVectorToScreen (refdef_t *rd, vec3_t in, vec2_t out)
{
   mat4x4_t p, m;
   vec4_t temp, temp2;

   if( !rd || !in || !out )
      return;

   temp[0] = in[0];
   temp[1] = in[1];
   temp[2] = in[2];
   temp[3] = 1.0f;

   R_SetupProjectionMatrix( rd, p );
   R_SetupModelviewMatrix( rd, m );

   Matrix4_Multiply_Vector( m, temp, temp2 );
   Matrix4_Multiply_Vector( p, temp2, temp );

   if( !temp[3] )
      return;
   out[0] = rd->x + (temp[0] / temp[3] + 1.0f) * rd->width * 0.5f;
   out[1] = rd->y + (temp[1] / temp[3] + 1.0f) * rd->height * 0.5f;
}
