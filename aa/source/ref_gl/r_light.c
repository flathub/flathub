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
// r_light.c

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

int	r_dlightframecount;

/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
R_MarkLights
=============
*/
static int		num_dlight_surfaces, old_dlightframecount;
static qboolean	new_dlight;
void R_MarkLights (dlight_t *light, int bit, mnode_t *node)
{
          cplane_t  *splitplane;
          float              dist;
          msurface_t         *surf;
          int                i, sidebit;

          if (node->contents != -1)
                   return;

          splitplane = node->plane;
          dist = DotProduct (light->origin, splitplane->normal) - splitplane->dist;

          if (dist > light->intensity-DLIGHT_CUTOFF) {
                   R_MarkLights (light, bit, node->children[0]);
                   return;
          }
          if (dist < -light->intensity+DLIGHT_CUTOFF) {
                   R_MarkLights (light, bit, node->children[1]);
                   return;
          }

// mark the polygons
          surf = r_worldmodel->surfaces + node->firstsurface;
          for (i=0 ; i<node->numsurfaces ; i++, surf++)
          {
                   dist = DotProduct (light->origin, surf->plane->normal) - surf->plane->dist;         //Discoloda
                   if (dist >= 0)                                                                              //Discoloda
                             sidebit = 0;                                                                      //Discoloda
                   else                                                                                        //Discoloda
                             sidebit = ISURF_PLANEBACK;                                                //Discoloda

                   if ( (surf->iflags & ISURF_PLANEBACK) != sidebit )                                   //Discoloda
                             continue;                                                                //Discoloda

                   if (surf->dlightframe != r_dlightframecount)
                   {
                             if (surf->dlightframe != old_dlightframecount)
                                       new_dlight = true;
                             num_dlight_surfaces++;
                             surf->dlightbits = bit;
                             surf->dlightframe = r_dlightframecount;
                   } else
                             surf->dlightbits |= bit;
          }

          R_MarkLights (light, bit, node->children[0]);
          R_MarkLights (light, bit, node->children[1]);
}


/*
=============
R_PushDlights
=============
*/
void R_PushDlights (void)
{
	int			i;
	dlight_t	*l;

	old_dlightframecount = r_dlightframecount;
	new_dlight = false;
	num_dlight_surfaces = 0;
	
	r_dlightframecount = r_framecount + 1;	// because the count hasn't
											//  advanced yet for this frame
	l = r_newrefdef.dlights;
	for (i=0 ; i<r_newrefdef.num_dlights ; i++, l++)
		R_MarkLights ( l, 1<<i, r_worldmodel->nodes );
	
	r_newrefdef.dlights_changed = false;
	if (num_dlight_surfaces != r_newrefdef.num_dlight_surfaces || new_dlight)
		r_newrefdef.dlights_changed = true;
	r_newrefdef.num_dlight_surfaces = num_dlight_surfaces;
}

/*
=============
R_PushDlightsForBModel
=============
*/
void R_PushDlightsForBModel (entity_t *e)
{
	int			k;
	dlight_t	*lt;

	lt = r_newrefdef.dlights;

	if (e->angles[0] || e->angles[1] || e->angles[2])
	{
		vec3_t	temp;
		vec3_t	forward, right, up;

		AngleVectors (e->angles, forward, right, up);

		for (k=0 ; k<r_newrefdef.num_dlights ; k++, lt++)
		{
			VectorSubtract (lt->origin, e->origin, temp);
			lt->origin[0] = DotProduct (temp, forward);
			lt->origin[1] = -DotProduct (temp, right);
			lt->origin[2] = DotProduct (temp, up);
			R_MarkLights (lt, 1<<k, e->model->nodes + e->model->firstnode);
			VectorAdd (temp, e->origin, lt->origin);
		}
	}
	else
	{
		for (k=0 ; k<r_newrefdef.num_dlights ; k++, lt++)
		{
			VectorSubtract (lt->origin, e->origin, lt->origin);
			R_MarkLights (lt, 1<<k, e->model->nodes + e->model->firstnode);
			VectorAdd (lt->origin, e->origin, lt->origin);
		}
	}
}

static int compare_dlight (const void *a, const void *b)
{
	float diff = ((const dlight_t *)a)->sort_key - ((const dlight_t *)b)->sort_key;
	
	if (diff < 0.0f)
		return 1;
	else if (diff > 0.0f)
		return -1;
	return 0;
}

static struct
{
	vec3_t eyeSpaceOrigin[GLSL_MAX_DLIGHTS];
	vec3_t lightAmountSquared[GLSL_MAX_DLIGHTS];
	float lightCutoffSquared[GLSL_MAX_DLIGHTS];
} dlight_uniform_values;

void R_UpdateDlights (void)
{
	int			i;
	dlight_t	*l;
	
	l = r_newrefdef.dlights;
	for (i = 0; i < r_newrefdef.num_dlights; i++, l++)
	{
		vec3_t relativePosition;
		
		l->intensitySquared = l->intensity - DLIGHT_CUTOFF;

		if (l->intensitySquared <= 0.0f)
			l->intensitySquared = 0.0f;

		l->intensitySquared *= 2.0f;
		l->intensitySquared *= l->intensitySquared;
		
		VectorScale (l->color, l->intensitySquared, l->lightAmountSquared);
		
		VectorSubtract (r_origin, l->origin, relativePosition);
		l->sort_key = l->intensity - VectorLength (relativePosition) / 10.0f;
	}	
	
	// Sort from most important to least important
	qsort (r_newrefdef.dlights, r_newrefdef.num_dlights, sizeof (dlight_t), compare_dlight);
	
	// Those dynamic lights of negative importance are never used
	while (r_newrefdef.num_dlights > 0 && r_newrefdef.dlights[r_newrefdef.num_dlights - 1].sort_key <= 0.0f)
		r_newrefdef.num_dlights--;
	
	// Build parallel arrays for sending to GLSL
	l = r_newrefdef.dlights;
	for (i = 0; i < CUR_NUM_DLIGHTS; i++, l++)
	{
		R_ModelViewTransform (l->origin, dlight_uniform_values.eyeSpaceOrigin[i]);
		VectorCopy (l->lightAmountSquared, dlight_uniform_values.lightAmountSquared[i]);
		dlight_uniform_values.lightCutoffSquared[i] = l->intensitySquared;
	}
}

void R_SetDlightUniforms (dlight_uniform_location_t *uniforms)
{
	glUniform3fvARB (uniforms->lightPosition, CUR_NUM_DLIGHTS, (const GLfloat *) dlight_uniform_values.eyeSpaceOrigin);
	glUniform3fvARB (uniforms->lightAmountSquared, CUR_NUM_DLIGHTS, (const GLfloat *) dlight_uniform_values.lightAmountSquared);
	glUniform1fvARB (uniforms->lightCutoffSquared, CUR_NUM_DLIGHTS, (const GLfloat *) dlight_uniform_values.lightCutoffSquared);
}

/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

vec3_t			pointcolor;
cplane_t		*lightplane;		// used as shadow plane
vec3_t			lightspot;

static int RecursiveLightPoint (mnode_t *node, vec3_t start, vec3_t end, qboolean *out_uses_lightstyle)
{
	float		front, back, frac;
	int			side;
	cplane_t	*plane;
	vec3_t		mid;
	msurface_t	*surf;
	int64_t		s, t, ds, dt;
	int			i;
	mtexinfo_t	*tex;
	byte		*lightmap;
	int			maps;
	int			r;
	vec3_t		scale;
	int			stylesize;

	if (node->contents != -1)
		return -1;		// didn't hit anything

// calculate mid point

// FIXME: optimize for axial
	plane = node->plane;
	front = DotProduct (start, plane->normal) - plane->dist;
	back = DotProduct (end, plane->normal) - plane->dist;
	side = front < 0;

	if ( (back < 0) == side)
		return RecursiveLightPoint (node->children[side], start, end, out_uses_lightstyle);

	frac = front / (front-back);
	mid[0] = start[0] + (end[0] - start[0])*frac;
	mid[1] = start[1] + (end[1] - start[1])*frac;
	mid[2] = start[2] + (end[2] - start[2])*frac;

// go down front side
	r = RecursiveLightPoint (node->children[side], start, mid, out_uses_lightstyle);
	if (r >= 0)
		return r;		// hit something

	if ( (back < 0) == side )
		return -1;		// didn't hit anuthing

// check for impact on this node
	VectorCopy (mid, lightspot);
	lightplane = plane;

	surf = r_worldmodel->surfaces + node->firstsurface;
	for (i=0 ; i<node->numsurfaces ; i++, surf++)
	{
		tex = surf->texinfo;

		if (tex->flags & (SURF_WARP|SURF_SKY|SURF_TRANS33|SURF_NODRAW))
			continue;	// no lightmaps

		s = DotProduct (mid, tex->vecs[0]) + tex->vecs[0][3];
		if (s < surf->texturemins[0])
			continue;

		ds = s - surf->texturemins[0];
		if (ds > surf->extents[0])
			continue;

		t = DotProduct (mid, tex->vecs[1]) + tex->vecs[1][3];
		if (t < surf->texturemins[1])
			continue;

		dt = t - surf->texturemins[1];
		if (dt > surf->extents[1])
			continue;

		lightmap = surf->samples;
		if (!surf->samples)
			return 0;

		ds = floor((float)ds/surf->lightmap_xscale);
		dt = floor((float)dt/surf->lightmap_yscale);
		
		VectorClear (pointcolor);

		lightmap += 3*(dt * ((int)floor((float)surf->extents[0]/surf->lightmap_xscale)+1) + ds);
		stylesize = 3 * ((int)floor((float)surf->extents[1]/surf->lightmap_yscale)+1) * ((int)floor((float)surf->extents[0]/surf->lightmap_xscale)+1);

		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;	maps++)
		{
			for (i=0 ; i<3 ; i++)
				scale[i] = gl_modulate->value*r_newrefdef.lightstyles[surf->styles[maps]].rgb[i];

			pointcolor[0] += lightmap[0] * scale[0] * (1.0/255);
			pointcolor[1] += lightmap[1] * scale[1] * (1.0/255);
			pointcolor[2] += lightmap[2] * scale[2] * (1.0/255);
			if (surf->styles[maps] != 0)
				*out_uses_lightstyle = true;
			lightmap += stylesize;
		}

		return 1;
	}

// go down back side
	return RecursiveLightPoint (node->children[!side], mid, end, out_uses_lightstyle);
}

/*
===============
R_LightPoint
===============
*/

// Return true if the point included light from a changing lightstyle. If the
// return is true, 
qboolean R_StaticLightPoint (const vec3_t p, vec3_t color)
{
	vec3_t		end;
	qboolean	uses_lightstyle = false;
	
	color[0] = color[1] = color[2] = 1.0f;

	if (!(r_newrefdef.rdflags & RDF_NOWORLDMODEL))
	{
		CM_TerrainLightPoint (p, end, color);
		VectorScale (color, gl_modulate->value, color);
	
		if (r_worldmodel->lightdata && RecursiveLightPoint (r_worldmodel->nodes, p, end, &uses_lightstyle) != -1)
			VectorCopy (pointcolor, color);
	}

	return uses_lightstyle;
}

void R_DynamicLightPoint (const vec3_t p, vec3_t color)
{
	int			lnum;
	dlight_t	*dl;
	float		light;
	vec3_t		dist, dlightcolor;
	float		add;
	
	//
	// add dynamic lights
	//
	light = 0;
	dl = r_newrefdef.dlights;
	VectorClear ( dlightcolor );
	for (lnum=0 ; lnum<r_newrefdef.num_dlights ; lnum++, dl++)
	{
		VectorSubtract (p,
						dl->origin,
						dist);
		add = dl->intensity - VectorLength(dist);
		if (add > 0)
		{
			add *= (1.0/256);
			VectorMA (dlightcolor, add, dl->color, dlightcolor);
		}
	}
	
	VectorScale (dlightcolor, gl_modulate->value, color);
}


void R_LightPoint (vec3_t p, vec3_t color, qboolean addDynamic)
{
	vec3_t		dynamic;
	
	R_StaticLightPoint (p, color);
	if (addDynamic)
	{
		R_DynamicLightPoint (p, dynamic);
		VectorAdd (color, dynamic, color);
	}
}

//===================================================================

static float s_blocklights[1024*1024*3];

/*
** R_SetCacheState
*/
void R_SetCacheState( msurface_t *surf )
{
	int maps;

	for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
		 maps++)
	{
		surf->cached_light[maps] = r_newrefdef.lightstyles[surf->styles[maps]].white;
	}
}

/*
===============
R_BuildLightMap

Combine and scale multiple lightmaps into the floating format in blocklights
===============
*/
void R_BuildLightMap (msurface_t *surf, byte *dest, int smax, int tmax, int stride)
{
	int			r, g, b, a, max;
	int			i, j, size;
	byte		*lightmap, *old_dest;
	float		scale[4];
	int			nummaps;
	float		*bl;
	lightstyle_t	*style;

	if ( SurfaceHasNoLightmap( surf ) )
		Com_Error (ERR_DROP, "R_BuildLightMap called for non-lit surface");

	size = smax*tmax;
	if (size > (sizeof(s_blocklights)>>4) )
		Com_Error (ERR_DROP, "Bad s_blocklights size");

// set to full bright if no light data
	if (!surf->samples)
	{
		int maps;

		for (i=0 ; i<size*3 ; i++)
			s_blocklights[i] = 255;
		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			style = &r_newrefdef.lightstyles[surf->styles[maps]];
		}
		goto store;
	}

	// count the # of maps
	for ( nummaps = 0 ; nummaps < MAXLIGHTMAPS && surf->styles[nummaps] != 255 ;
		 nummaps++)
		;

	lightmap = surf->samples;

	// add all the lightmaps
	if ( nummaps == 1 )
	{
		int maps;

		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			bl = s_blocklights;

			for (i=0 ; i<3 ; i++)
				scale[i] = gl_modulate->value*r_newrefdef.lightstyles[surf->styles[maps]].rgb[i];

			if ( scale[0] == 1.0F &&
				 scale[1] == 1.0F &&
				 scale[2] == 1.0F )
			{
				for (i=0 ; i<size ; i++, bl+=3)
				{
					bl[0] = lightmap[i*3+0];
					bl[1] = lightmap[i*3+1];
					bl[2] = lightmap[i*3+2];
				}
			}
			else
			{
				for (i=0 ; i<size ; i++, bl+=3)
				{
					bl[0] = lightmap[i*3+0] * scale[0];
					bl[1] = lightmap[i*3+1] * scale[1];
					bl[2] = lightmap[i*3+2] * scale[2];
				}
			}
			lightmap += size*3;		// skip to next lightmap
		}
	}
	else
	{
		int maps;

		memset( s_blocklights, 0, sizeof( s_blocklights[0] ) * size * 3 );
		
		for (maps = 0 ; maps < MAXLIGHTMAPS && surf->styles[maps] != 255 ;
			 maps++)
		{
			bl = s_blocklights;

			for (i=0 ; i<3 ; i++)
				scale[i] = gl_modulate->value*r_newrefdef.lightstyles[surf->styles[maps]].rgb[i];

			if ( scale[0] == 1.0F &&
				 scale[1] == 1.0F &&
				 scale[2] == 1.0F )
			{
				for (i=0 ; i<size ; i++, bl+=3 )
				{
					bl[0] += lightmap[i*3+0];
					bl[1] += lightmap[i*3+1];
					bl[2] += lightmap[i*3+2];
				}
			}
			else
			{
				for (i=0 ; i<size ; i++, bl+=3)
				{
					bl[0] += lightmap[i*3+0] * scale[0];
					bl[1] += lightmap[i*3+1] * scale[1];
					bl[2] += lightmap[i*3+2] * scale[2];
				}
			}
			lightmap += size*3;		// skip to next lightmap
		}
	}

// put into texture format
store:
	stride -= (smax<<2);
	bl = s_blocklights;
	
	old_dest = dest;

	for (i=0 ; i<tmax ; i++, dest += stride)
	{
		for (j=0 ; j<smax ; j++)
		{
			r = Q_ftol( bl[0] );
			g = Q_ftol( bl[1] );
			b = Q_ftol( bl[2] );

			// catch negative lights
			if (r < 0)
				r = 0;
			if (g < 0)
				g = 0;
			if (b < 0)
				b = 0;
			/*
			** determine the brightest of the three color components
			*/
			if (r > g)
				max = r;
			else
				max = g;
			if (b > max)
				max = b;

			//255 is best for alpha testing, so textures don't "disapear" in the dark
			a = 255;

			/*
			** rescale all the color components if the intensity of the greatest
			** channel exceeds 1.0
			** NOTE: we used to scale alpha here also, but it caused problems.
			*/
			if (max > 255)
			{
				float t = 255.0F / max;
				r = r*t;
				g = g*t;
				b = b*t;
			}

			// GL_BGRA
			dest[0] = b;
			dest[1] = g;
			dest[2] = r;
			dest[3] = a;

			bl += 3;
			dest += 4;
		}
	}

	#define GET_SAMPLE(x,y) (old_dest+((y*smax+x)*4)+y*stride)
	#define SAMPLEDIST(a,b) sqrt((float)((a[0]-b[0])*(a[0]-b[0])+(a[1]-b[1])*(a[1]-b[1])+(a[2]-b[2])*(a[2]-b[2])))
}
