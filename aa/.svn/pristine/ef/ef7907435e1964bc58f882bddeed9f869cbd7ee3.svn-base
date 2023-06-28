/*
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2004-2014 COR Entertainment, LLC

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

// r_mesh.c: triangle model rendering functions, shared by all mesh types

#include <stdarg.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"
#include "r_ragdoll.h"

extern void MYgluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);
extern image_t *r_mirrortexture;

extern cvar_t *cl_gun;

extern float sun_alpha;

cvar_t *gl_mirror;

static void R_Mesh_CompleteNormalsTangents
	(model_t *mod, int calcflags, const void *basevbo, size_t basevbo_stride, mesh_framevbo_t *framevbo, const unsigned int *vtriangles)
{
	int i, j, ndiscarded = 0;
	unsigned int nonindexed_triangle[3];
	qboolean *merged, *do_merge;
	merged = Z_Malloc (mod->numvertexes * sizeof(qboolean));
	do_merge = Z_Malloc (mod->numvertexes * sizeof(qboolean));
	
	// can't force handedness without calculating tangents
	assert (!(calcflags & MESHLOAD_FORCE_HANDEDNESS) || (calcflags & MESHLOAD_CALC_TANGENT));
	// can't calculate tangents without calculating normals (FIXME)
	assert (!(calcflags & MESHLOAD_CALC_TANGENT) || (calcflags & MESHLOAD_CALC_NORMAL));
	
	for (i = 0; i < mod->num_triangles; i++)
	{
		vec3_t v1, v2, normal;
		const unsigned int *triangle;
		
		if ((mod->typeFlags & MESH_INDEXED))
		{
			triangle = &vtriangles[3*i];
		}
		else
		{
			triangle = nonindexed_triangle;
			for (j = 0; j < 3; j++)
				nonindexed_triangle[j] = 3*i+j;
		}
		
		if ((calcflags & (MESHLOAD_CALC_NORMAL|MESHLOAD_CALC_TANGENT)))
		{
			VectorSubtract (framevbo[triangle[0]].vertex, framevbo[triangle[1]].vertex, v1);
			VectorSubtract (framevbo[triangle[2]].vertex, framevbo[triangle[1]].vertex, v2);
			CrossProduct (v2, v1, normal);
			VectorScale (normal, -1.0f, normal);
		}
		
		if ((calcflags & MESHLOAD_CALC_TANGENT))
		{
			vec4_t tmp_tangent;
			const nonskeletal_basevbo_t *basevbo_vert[3];
			
			for (j = 0; j < 3; j++)
				basevbo_vert[j] = (nonskeletal_basevbo_t *)(((byte *)basevbo) + triangle[j] * basevbo_stride);
			
			R_CalcTangent ( framevbo[triangle[0]].vertex,
							framevbo[triangle[1]].vertex,
							framevbo[triangle[2]].vertex,
							basevbo_vert[0]->st,
							basevbo_vert[1]->st,
							basevbo_vert[2]->st,
							normal, tmp_tangent );
			
			if ((calcflags & MESHLOAD_FORCE_HANDEDNESS) && tmp_tangent[3] == 1.0f)
			{
				// TODO: discard the entire triangle!
				ndiscarded++;
				goto discard_tangent_and_normal;
			}
			
			for (j = 0; j < 3; j++)
			{			
				if (framevbo[triangle[j]].tangent[3] != 0.0f)
				{
					if ((framevbo[triangle[j]].tangent[3] > 0.0f) != (tmp_tangent[3] > 0.0f))
						Com_DPrintf ("WARN: Inconsistent triangle handedness in %s! %f != %f\n",
									mod->name, framevbo[triangle[j]].tangent[3], tmp_tangent[3]);
				}

				Vector4Add (framevbo[triangle[j]].tangent, tmp_tangent, framevbo[triangle[j]].tangent);
			}
		}
		
		if ((calcflags & MESHLOAD_CALC_NORMAL))
		{
			for (j = 0; j < 3; j++)
				VectorAdd (framevbo[triangle[j]].normal, normal, framevbo[triangle[j]].normal);
		}
discard_tangent_and_normal:;
	}
	
	if (ndiscarded > 0)
		Com_Printf ("WARN: normals/tangents from %d triangles disregared in %s due to bad handedness!\n",
					ndiscarded, mod->name);
	
	// In the case of texture seams in the mesh, we merge tangents/normals for
	// verts with the same positions. This is also necessary for non-indexed
	// meshes.
	for (i = 0; i < mod->numvertexes; i++)
	{
		if (merged[i]) continue;
		
		// Total normals/tangents up, as long as vertex coords are the same
		// and vertex handedness matches
		for (j = i + 1; j < mod->numvertexes; j++)
		{
			if (	VectorCompare (framevbo[i].vertex, framevbo[j].vertex) &&
					(framevbo[i].tangent[3] > 0.0f) == (framevbo[j].tangent[3] > 0.0f)
				)
			{
				do_merge[j] = true;
				
				if ((calcflags & MESHLOAD_CALC_NORMAL))
					VectorAdd (framevbo[i].normal, framevbo[j].normal, framevbo[i].normal);
				if ((calcflags & MESHLOAD_CALC_TANGENT))
					VectorAdd (framevbo[i].tangent, framevbo[j].tangent, framevbo[i].tangent);
			}
		}
		
		// Normalize the average normals and tangents
		if ((calcflags & MESHLOAD_CALC_NORMAL))
		{
			VectorNormalize (framevbo[i].normal);
		}
		if ((calcflags & MESHLOAD_CALC_TANGENT))
		{
			VectorNormalize (framevbo[i].tangent);
			framevbo[i].tangent[3] /= fabsf (framevbo[i].tangent[3]);
		}
		
		// Copy the averages where they need to go
		for (j = i + 1; j < mod->numvertexes; j++)
		{
			if (do_merge[j])
			{
				merged[j] = true;
				do_merge[j] = false;
				
				if ((calcflags & MESHLOAD_CALC_NORMAL))
					VectorCopy (framevbo[i].normal, framevbo[j].normal);
				if ((calcflags & MESHLOAD_CALC_TANGENT))
					Vector4Copy (framevbo[i].tangent, framevbo[j].tangent);
			}
		}
	}
	
	Z_Free (merged);
	Z_Free (do_merge);
}

static size_t R_Mesh_GetBaseVBO_Stride (int typeFlags)
{
	switch (typeFlags & (MESH_SKELETAL | MESH_LM_SEPARATE_COORDS))
	{
		default: // this line is only to shut the compiler warning up
		case 0:
			return sizeof (nonskeletal_basevbo_t);
		case MESH_SKELETAL:
			return sizeof (skeletal_basevbo_t);
		case MESH_LM_SEPARATE_COORDS:
			return sizeof (nonskeletal_lm_basevbo_t);
		case MESH_LM_SEPARATE_COORDS | MESH_SKELETAL:
			return sizeof (skeletal_lm_basevbo_t);
	}
}

void R_Mesh_LoadVBO (model_t *mod, int calcflags, ...)
{
	const int maxframes = ((mod->typeFlags & MESH_MORPHTARGET) ? mod->num_frames : 1);
	const int frames_idx = ((mod->typeFlags & MESH_INDEXED) ? 2 : 1);
	mesh_framevbo_t *framevbo;
	const unsigned int *vtriangles = NULL;
	const void *basevbo;
	size_t basevbo_stride;
	va_list ap;
	
	mod->vboIDs = malloc ((maxframes + frames_idx) * sizeof(*mod->vboIDs));
	qglGenBuffersARB (maxframes + frames_idx, mod->vboIDs);
	
	va_start (ap, calcflags);
	
	GL_BindVBO (mod->vboIDs[0]);
	basevbo = va_arg (ap, const void *);
	basevbo_stride = R_Mesh_GetBaseVBO_Stride (mod->typeFlags);
	qglBufferDataARB (GL_ARRAY_BUFFER_ARB, mod->numvertexes * basevbo_stride, basevbo, GL_STATIC_DRAW_ARB);
	
	if ((mod->typeFlags & MESH_INDEXED))
	{
		GL_BindVBO (mod->vboIDs[1]);
		vtriangles = va_arg (ap, const unsigned int *);
		qglBufferDataARB (GL_ARRAY_BUFFER_ARB, mod->num_triangles * 3 * sizeof(unsigned int), vtriangles, GL_STATIC_DRAW_ARB);
	}
	
	if (!(mod->typeFlags & MESH_MORPHTARGET))
	{
		framevbo = va_arg (ap, mesh_framevbo_t *);
		if (calcflags != 0)
			R_Mesh_CompleteNormalsTangents (mod, calcflags, basevbo, basevbo_stride, framevbo, vtriangles);
		GL_BindVBO (mod->vboIDs[frames_idx]);
		qglBufferDataARB (GL_ARRAY_BUFFER_ARB, mod->numvertexes * sizeof(*framevbo), framevbo, GL_STATIC_DRAW_ARB);
	}
	else
	{
		int framenum;
		Mesh_GetFrameVBO_Callback callback = va_arg (ap, Mesh_GetFrameVBO_Callback);
		void *data = va_arg (ap, void *);
		
		for (framenum = 0; framenum < mod->num_frames; framenum++)
		{
			mesh_framevbo_t *framevbo = callback (data, framenum);
			if (calcflags != 0)
				R_Mesh_CompleteNormalsTangents (mod, calcflags, basevbo, basevbo_stride, framevbo, vtriangles);
			GL_BindVBO (mod->vboIDs[framenum + frames_idx]);
			qglBufferDataARB (GL_ARRAY_BUFFER_ARB, mod->numvertexes * sizeof(*framevbo), framevbo, GL_STATIC_DRAW_ARB);
		}
	}
	
	GL_BindVBO (0);
	
	va_end (ap);
}

void R_Mesh_FreeVBO (model_t *mod)
{
	const int maxframes = ((mod->typeFlags & MESH_MORPHTARGET) ? mod->num_frames : 1);
	const int frames_idx = ((mod->typeFlags & MESH_INDEXED) ? 2 : 1);
	
	qglDeleteBuffersARB (maxframes + frames_idx, mod->vboIDs);
	free (mod->vboIDs);
}

//This routine bascially finds the average light position, by factoring in all lights and
//accounting for their distance, visiblity, and intensity.
extern int server_tickrate;
static void R_GetStaticLightingForEnt (const entity_t *ent, const model_t *mod, vec3_t out_position, vec3_t out_color)
{
	int i, j;
	float dist;
	vec3_t	temp, tempOrg, lightAdd;
	float numlights, nonweighted_numlights, weight;
	float bob;
	qboolean copy;

	if (ent->flags & RF_MENUMODEL)
	{
		VectorSet (out_position, -15.0, 180.0, 140.0);
	}
	else
	{
		//light shining down if there are no lights at all
		VectorCopy (ent->origin, out_position);
		out_position[2] += 128;

		if ((ent->flags & RF_BOBBING))
			bob = ent->bob;
		else
			bob = 0;

		VectorCopy (ent->origin, tempOrg);
			tempOrg[2] += 24 - bob; //generates more consistent tracing

		numlights = nonweighted_numlights = 0;
		VectorClear(lightAdd);
	
		copy = cl_persistent_ents[ent->number].setlightstuff;
		// ragdolls share the same "number" as the corresponding player, so if a
		// player and his corpse are on screen at the same time, the code will
		// interpret this as oscillation between the two positions, so the cached
		// lighting values won't be saved. (FIXME)
		for (i = 0; i < 3; i++)
		{
			if (fabs (ent->origin[i] - cl_persistent_ents[ent->number].oldorigin[i]) > 0.0001)
			{
				copy = false;
				break;
			}
		}
	
		if (copy)
		{
			numlights = cl_persistent_ents[ent->number].oldnumlights;
			VectorCopy (cl_persistent_ents[ent->number].oldlightadd, lightAdd);
			if (cl_persistent_ents[ent->number].useoldstaticlight)
				VectorCopy (cl_persistent_ents[ent->number].oldstaticlight, out_color);
			else
				cl_persistent_ents[ent->number].useoldstaticlight = !R_StaticLightPoint (ent->origin, out_color);
		}
		else
		{
			cl_persistent_ents[ent->number].useoldstaticlight = !R_StaticLightPoint (ent->origin, out_color);
			VectorCopy (out_color, cl_persistent_ents[ent->number].oldstaticlight);
			for (i = 0; i < r_lightgroups; i++)
			{
				if (mod->type == mod_terrain || mod->type == mod_decal) {}
					//terrain meshes can actually occlude themselves. TODO: move to precompiled lightmaps for terrain.
				else if ((ent->flags & RF_WEAPONMODEL)) {}
					//don't do traces for lights above weapon models, not smooth enough
				else if (!CM_inPVS (tempOrg, LightGroups[i].group_origin))
					continue;
				else if (!CM_FastTrace (tempOrg, LightGroups[i].group_origin, r_worldmodel->firstnode, MASK_OPAQUE))
					continue;

				VectorSubtract(ent->origin, LightGroups[i].group_origin, temp);
				dist = VectorLength(temp);
				if (dist == 0)
					dist = 1;
				dist = dist*dist;
				weight = (int)250000/(dist/(LightGroups[i].avg_intensity+1.0f));
				for (j = 0; j < 3; j++)
					lightAdd[j] += LightGroups[i].group_origin[j]*weight;
				numlights+=weight;
				nonweighted_numlights++;
			}

			// TODO: see if we can reintroduce this effect without affecting shadows
			//A simple but effective effect to mimick the effect of sun silouetting(i.e, hold your hand up and partially block the sun, the 
			//backside appears to grow very dark - so we can do this by weighting our static light average heavily towards the sun's origin.
			/*if(sun_alpha > 0.0)
			{
				weight = 10000 * pow(sun_alpha, 10);
				for (j = 0; j < 3; j++)
					lightAdd[j] += r_sunLight->origin[j]*weight;
				numlights+=weight;
				nonweighted_numlights++;
			}*/
		
			cl_persistent_ents[ent->number].oldnumlights = numlights;
			VectorCopy (lightAdd, cl_persistent_ents[ent->number].oldlightadd);
			cl_persistent_ents[ent->number].setlightstuff = true;
			VectorCopy (ent->origin, cl_persistent_ents[ent->number].oldorigin);
		}

		if (numlights > 0.0)
		{
			for (i = 0; i < 3; i++)
				out_position[i] = lightAdd[i]/numlights;
		}
	}
	
	// override static light value for some effects
	if ((ent->flags & RF_SHELL_ANY))
	{
		VectorClear (out_color);
		if ((ent->flags & RF_SHELL_HALF_DAM))
			VectorSet (out_color, 0.56, 0.59, 0.45);
		if ((ent->flags & RF_SHELL_DOUBLE))
		{
			out_color[0] = 0.9;
			out_color[1] = 0.7;
		}
		if ((ent->flags & RF_SHELL_RED))
			out_color[0] = 1.0;
		if ((ent->flags & RF_SHELL_GREEN))
		{
			out_color[1] = 1.0;
			out_color[2] = 0.6;
		}
		if ((ent->flags & RF_SHELL_BLUE))
		{
			out_color[2] = 1.0;
			out_color[0] = 0.6;
		}
	}
	else if ((ent->flags & RF_FULLBRIGHT) || mod->type == mod_terrain || mod->type == mod_decal)
	{
		// Because decals and terrain have their own precompiled lightmaps, we
		// set them fullbright as well (so as not to dim their lighting.)
		VectorSet (out_color, 1.0, 1.0, 1.0);
	}
	
	// adjust static light value for some effects
	if ((ent->flags & RF_MINLIGHT))
	{
		float minlight;

		if (r_meshnormalmaps->integer)
			minlight = 0.1;
		else
			minlight = 0.2;
		for (i=0 ; i<3 ; i++)
			if (out_color[i] > minlight)
				break;
		if (i == 3)
		{
			out_color[0] = minlight;
			out_color[1] = minlight;
			out_color[2] = minlight;
		}
	}
	if ((ent->flags & RF_GLOW))
	{	// bonus items will pulse with time
		float	scale;
		float	minlight;
		float FRAMETIME = 1.0/(float)server_tickrate;

		scale = 0.2 * sin(r_newrefdef.time * 7 * FRAMETIME/0.1);
		if (r_meshnormalmaps->integer)
			minlight = 0.1;
		else
			minlight = 0.2;
		for (i=0 ; i<3 ; i++)
		{
			out_color[i] += scale;
			if (out_color[i] < minlight)
				out_color[i] = minlight;
		}
	}
}

// calculate a combined value of static and dynamic lights (FOR SUBSURFACE
// SCATTERING ONLY!)
static void R_GetCombinedLightVals (const entity_t *ent, vec3_t out_position, vec3_t out_color)
{
	vec3_t temp, lightAdd, dynamiclight, staticlight;
	int i, j, lnum, numlights;
	dlight_t *dl;
	float dist;
	
	if (ent->flags & RF_MENUMODEL)
	{
		VectorSet (out_position, -15.0, 180.0, 140.0);
		VectorSet (out_color, 0.75, 0.75, 0.75);
		return;
	}
	
	numlights = cl_persistent_ents[ent->number].oldnumlights;
	VectorCopy (cl_persistent_ents[ent->number].oldlightadd, lightAdd);
	VectorCopy (cl_persistent_ents[ent->number].oldstaticlight, staticlight);
	
	if (gl_dynamic->integer != 0)
	{
		R_DynamicLightPoint (ent->origin, dynamiclight);
		dl = r_newrefdef.dlights;
		//limit to five lights(maybe less)?
		for (lnum=0; lnum<(r_newrefdef.num_dlights > 5 ? 5: r_newrefdef.num_dlights); lnum++, dl++)
		{
			VectorSubtract (ent->origin, dl->origin, temp);
			dist = VectorLength (temp);
			if (dist >= dl->intensity)
				continue;

			VectorCopy (ent->origin, temp);
			temp[2] += 24; //generates more consistent tracing

			if (CM_FastTrace (temp, dl->origin, r_worldmodel->firstnode, MASK_OPAQUE))
			{
				//make dynamic lights more influential than world
				for (j = 0; j < 3; j++)
					lightAdd[j] += dl->origin[j]*50*dl->intensity;
				numlights+=50*dl->intensity;
			}
		}

		//scale up this for a stronger subsurface effect
		VectorScale(dynamiclight, 5.0, dynamiclight);
	}
	else
	{
		VectorClear (dynamiclight);
	}
	
	VectorAdd (staticlight, dynamiclight, out_color);

	if (numlights > 0.0)
	{
		for (i = 0; i < 3; i++)
			out_position[i] = lightAdd[i]/numlights;
	}
}

void R_ModelViewTransform(const vec3_t in, vec3_t out){
	const float *v = in;
	const float *m = r_world_matrix;

	out[0] = m[0] * v[0] + m[4] * v[1] + m[8]  * v[2] + m[12];
	out[1] = m[1] * v[0] + m[5] * v[1] + m[9]  * v[2] + m[13];
	out[2] = m[2] * v[0] + m[6] * v[1] + m[10] * v[2] + m[14];
}

// Unlike R_CullBox, this takes an *oriented* (rotated) bounding box.
static qboolean R_Mesh_CullBBox (vec3_t bbox[8])
{
	int p, f, aggregatemask = ~0;

	for (p = 0; p < 8; p++)
	{
		int mask = 0;

		for (f = 0; f < 4; f++)
		{
			if (DotProduct (frustum[f].normal, bbox[p]) < frustum[f].dist)
				mask |= 1 << f;
		}

		aggregatemask &= mask;
	}

	return aggregatemask != 0;
}

// should be able to handle all mesh types
typedef enum {draw_none, draw_shadow_only, draw_full} cull_result_t;
static cull_result_t R_Mesh_CullModel (const entity_t *ent, const model_t *mod)
{
	int		i;
	vec3_t	vectors[3];
	vec3_t  angles;
	vec3_t	bbox[8];
	vec3_t	size;
	vec3_t  tmp;
	qboolean occlusion_cull;
	float velocity = 1.0;
	float rad;

	// Don't render your own avatar unless it's for shadows
	if ((ent->flags & RF_VIEWERMODEL))
		return draw_shadow_only;
	
	// Menu models
	if ((r_newrefdef.rdflags & RDF_NOWORLDMODEL))
		return ((ent->flags & RF_MENUMODEL)) ? draw_full : draw_none;
	
	// Weapon invisibility cvars
	if ((ent->flags & RF_WEAPONMODEL))
		return (cl_gun->integer && r_lefthand->integer != 2) ? draw_full : draw_none;
	
	/*
	** rotate the bounding box
	*/
	VectorCopy (ent->angles, angles);
	angles[YAW] = -angles[YAW];
	AngleVectors (angles, vectors[0], vectors[1], vectors[2]);

	for (i = 0; i < 8; i++)
	{
		vec3_t tmp;

		VectorCopy (mod->bbox[i], tmp);

		bbox[i][0] = DotProduct (vectors[0], tmp);
		bbox[i][1] = -DotProduct (vectors[1], tmp);
		bbox[i][2] = DotProduct (vectors[2], tmp);

		VectorAdd (ent->origin, bbox[i], bbox[i]);
	}
	
	// Occlusion culling-- check if *any* part of the mesh is visible. 
	// Possible to have meshes culled that shouldn't be, but quite rare.
	// TODO: parallelize?
	VectorSubtract (mod->maxs, mod->mins, size);
	occlusion_cull =	r_worldmodel && mod->type != mod_terrain &&
						mod->type != mod_decal &&
						// HACK: culling rocks is currently too slow, so we don't.
						!strstr (mod->name, "rock") &&
						// Do not occlusion-cull large meshes - too many artifacts occur
						size[0] <= 72 && size[1] <= 72 && size[2] <= 72;
	if (occlusion_cull)
	{
		qboolean unblocked = CM_FastTrace (ent->origin, r_origin, r_worldmodel->firstnode, MASK_OPAQUE);
		for (i = 0; i < 8 && !unblocked; i++)
			unblocked = CM_FastTrace (bbox[i], r_origin, r_worldmodel->firstnode, MASK_OPAQUE);
		
		if (!unblocked)
			return draw_none;
	}
	
	// TODO: could probably find a better place for this.
	if (r_ragdolls->integer && (mod->typeFlags & MESH_SKELETAL) && !ent->ragdoll)
	{
		//Ragdolls take over at beginning of each death sequence
		if	(	!(ent->flags & RF_TRANSLUCENT) && mod->hasRagDoll && 
				(ent->frame == 208 || ent->frame == 220 || ent->frame == 238)
			)
		{
			if (ent->frame == 208)
				velocity = 0.0001;
			RGD_AddNewRagdoll (ent, velocity);
		}
		//Do not render deathframes if using ragdolls - do not render translucent helmets
		if ((mod->hasRagDoll || (ent->flags & RF_TRANSLUCENT)) && (ent->frame > 207 && ent->frame < 260))
			return draw_none;
	}
	

	//Cull by frustom - switched to Cullsphere, as CallBBox was too harsh, and causing far too many meshes to blip in and out.
	VectorSubtract (mod->maxs,mod->mins, tmp);
	VectorScale (tmp, 1.666, tmp);
	rad = VectorLength (tmp);		
	if (R_CullSphere (ent->origin, rad, 15))
		return draw_shadow_only;

	return draw_full;
}

static void R_Mesh_SetupAnimUniforms (const entity_t *ent, const model_t *mod, mesh_anim_uniform_location_t *uniforms)
{
	int			animtype;
	
	// only applicable to MD2
	qboolean	lerped;
	float		frontlerp;
	
	lerped = ent->backlerp != 0.0 && (ent->frame != 0 || mod->num_frames != 1);
	frontlerp = 1.0 - ent->backlerp;
	
	animtype = 0;
	
	if ((mod->typeFlags & MESH_MORPHTARGET) && lerped)
		animtype |= 2;
	
	if ((mod->typeFlags & MESH_SKELETAL))
	{
		static matrix3x4_t outframe[SKELETAL_MAX_BONEMATS];
		assert (mod->type == mod_iqm);
		IQM_AnimateFrame (ent, mod, outframe);
		glUniformMatrix3x4fvARB (uniforms->outframe, mod->num_joints, GL_FALSE, (const GLfloat *) outframe);
		animtype |= 1;
	}
	
	// XXX: the vertex shader won't actually support a value of 3 for animtype
	// yet.
	
	glUniform1iARB (uniforms->useGPUanim, animtype);
	glUniform1fARB (uniforms->lerp, frontlerp);
}

// Cobbled together from two different functions. TODO: clean this mess up!
static void R_Mesh_SetupStandardRender (const entity_t *ent, const model_t *mod, rscript_t *rs, qboolean fragmentshader, qboolean shell, const vec3_t statLightVec, const vec3_t statLightColor)
{
	image_t *skin;
	float alpha;
	mesh_uniform_location_t *uniforms = fragmentshader ? &mesh_uniforms[CUR_NUM_DLIGHTS] : &mesh_vertexonly_uniforms[CUR_NUM_DLIGHTS];
	
	if (shell || (mod->typeFlags & MESH_DECAL))
	{
		GLSTATE_ENABLE_BLEND
	}
	else
	{
		GLSTATE_ENABLE_ALPHATEST
	}
	
	if (!shell)
		alpha = 1.0f;
	else
	{
		if (ent->ragdoll)
			alpha = ent->shellAlpha;
		else
		{
			if (ent->flags & RF_GLOW)
				alpha = 0.95f * sin(r_newrefdef.time*2.0f);
			else
				alpha = 0.33f; 
		}

		GL_BlendFunction(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	
	if (fragmentshader)
		glUseProgramObjectARB (g_meshprogramObj[CUR_NUM_DLIGHTS]);
	else
		glUseProgramObjectARB (g_vertexonlymeshprogramObj[CUR_NUM_DLIGHTS]);
	
	R_SetDlightUniforms (&uniforms->dlight_uniforms);
	
	glUniform3fvARB (uniforms->staticLightPosition, 1, (const GLfloat *)statLightVec);
	
	{
		vec3_t lightVal;
		float lightVal_magnitude;
		
		VectorCopy (statLightColor, lightVal);
		lightVal_magnitude = 1.65f * VectorNormalize (lightVal);
		lightVal_magnitude = clamp (lightVal_magnitude, 0.35f, 0.35f * gl_modulate->value);
		VectorScale (lightVal, lightVal_magnitude, lightVal);
		glUniform3fvARB (uniforms->staticLightColor, 1, (const GLfloat *)lightVal);
	}
	
	// select skin
	if (shell)
		skin = (ent->flags & RF_GLOW) ? r_shelltexture : r_shelltexture2;
	else if (ent->skin) 
		skin = ent->skin;
	else
		skin = mod->skins[0];
	if (!skin)
		skin = r_notexture;	// fallback..
	
	GL_MBind (0, skin->texnum);
	glUniform1iARB (uniforms->baseTex, 0);
	
	if (mod->lightmap != NULL)
	{
		glUniform1iARB (uniforms->lightmap, (mod->typeFlags & MESH_LM_SEPARATE_COORDS) ? rs_lightmap_separate_texcoords : rs_lightmap_on);
		glUniform1iARB (uniforms->lightmapTexture, 1);
		GL_MBind (1, mod->lightmap->texnum);
		
		if (!fragmentshader)
		{
			GL_EnableTexture (1, true);
			R_SetLightingMode ();
		}
	}
	else
	{
		glUniform1iARB (uniforms->lightmap, rs_lightmap_off);
	}

	if (fragmentshader)
	{
		glUniform1iARB (uniforms->normTex, 2);
		
		if (!shell)
		{
			GL_MBind (2, rs->stage->texture->texnum);

			GL_MBind (3, rs->stage->texture2->texnum);
			glUniform1iARB (uniforms->fxTex, 3);

			GL_MBind (4, rs->stage->texture3->texnum);
			glUniform1iARB (uniforms->fx2Tex, 4);

			R_SetShadowmapUniforms (&uniforms->shadowmap_uniforms, 5, true);

			if (r_nonSunStaticShadowsOn || r_sunShadowsOn)
			{
				vec3_t angles;
				float rotationMatrix[3][3];

				// TODO: 4x4 matrix with translation, instead of separate 
				// translation and rotation matrices.
						
				// because we are translating our entities, we need to supply the shader with the actual position of this mesh
				glUniform3fvARB (uniforms->meshPosition, 1, (const GLfloat *)ent->origin);
				
				// pitch and roll are handled by IQM_AnimateFrame.
				VectorClear (angles); 
				if (mod->type != mod_iqm || mod->num_poses > 173)
					VectorCopy (ent->angles, angles);
				else
					angles[YAW] = ent->angles[YAW];
				AnglesToMatrix3x3 (angles, rotationMatrix);
				glUniformMatrix3fvARB (uniforms->meshRotation, 1, GL_TRUE, (const GLfloat *) rotationMatrix);
			}
		}
		else
		{
			if (ent->flags & RF_GLOW)
				GL_MBind (1, r_shellnormal->texnum);
			else
				GL_MBind (1, r_shellnormal2->texnum);
		}
		
		glUniform1iARB (uniforms->useFX, shell ? 0 : rs->stage->fx);
		glUniform1iARB (uniforms->useGlow, shell ? 0 : rs->stage->glow);
		glUniform1iARB (uniforms->useCube, shell ? 0 : rs->stage->cube);
		
		glUniform1iARB (uniforms->fromView, (ent->flags & RF_WEAPONMODEL) != 0);
		
		// for subsurface scattering, we hack in the old algorithm.
		if (!shell && !rs->stage->cube)
		{
			vec3_t lightVec, tmp, color;
			
			R_GetCombinedLightVals (ent, tmp, color);
			
			VectorSubtract (tmp, ent->origin, tmp);
			R_ModelViewTransform (tmp, lightVec);

			glUniform3fvARB (uniforms->totalLightPosition, 1, (const GLfloat *)lightVec);
			glUniform3fvARB (uniforms->totalLightColor, 1, (const GLfloat *)color);
		}
	}
	
	glUniform1fARB (uniforms->time, rs_realtime);	
	
	glUniform1iARB (uniforms->fog, map_fog);
	glUniform1iARB (uniforms->team, ent->team);

	glUniform1fARB (uniforms->useShell, shell ? (ent->ragdoll?1.6:0.4) : 0.0);
	glUniform1fARB (uniforms->shellAlpha, alpha);
	
	glUniform1iARB (uniforms->doShading, (mod->typeFlags & MESH_DOSHADING) != 0);
	
	R_Mesh_SetupAnimUniforms (ent, mod, &uniforms->anim_uniforms);
	
	// set up the fixed-function pipeline too
	if (!fragmentshader)
		qglColor4f (statLightColor[0], statLightColor[1], statLightColor[2], alpha);
}

static void R_Mesh_SetupGlassRender (const entity_t *ent, const model_t *mod, const vec3_t lightVec)
{
	vec3_t left, up;
	int type;
	qboolean mirror, glass;
	
	mirror = glass = false;
	
	if ((r_newrefdef.rdflags & RDF_NOWORLDMODEL))
		glass = true;
	else if (gl_mirror->integer)
		mirror = true;
	else
		glass = true;
	
	if (mirror && !(ent->flags & RF_WEAPONMODEL))
		glass = true;
	
	glUseProgramObjectARB (g_glassprogramObj);
	
	glUniform3fARB (glass_uniforms.lightPos, lightVec[0], lightVec[1], lightVec[2]);

	AngleVectors (ent->angles, NULL, left, up);
	glUniform3fARB (glass_uniforms.left, left[0], left[1], left[2]);
	glUniform3fARB (glass_uniforms.up, up[0], up[1], up[2]);
	
	type = 0;
	
	if (glass)
	{
		glUniform1iARB (glass_uniforms.refTexture, 0);
		GL_MBind (0, r_mirrorspec->texnum);
		type |= 2;
	}
	
	if (mirror)
	{
		glUniform1iARB (glass_uniforms.mirTexture, 1);
		GL_MBind (1, r_mirrortexture->texnum);
		type |= 1;
	}
	
	glUniform1iARB (glass_uniforms.type, type);
	
	glUniform1iARB (glass_uniforms.fog, map_fog);
	
	GLSTATE_ENABLE_BLEND
	GL_BlendFunction (GL_ONE, GL_ONE);
	
	R_Mesh_SetupAnimUniforms (ent, mod, &glass_uniforms.anim_uniforms);
}

// Should be able to handle all mesh types. This is the component of the 
// mesh rendering process that does not need to care which GLSL shader is
// being used-- they all support the same vertex data and vertex attributes.
static void R_Mesh_DrawVBO (const entity_t *ent, const model_t *mod, qboolean lerped)
{
	const int typeFlags = mod->typeFlags;
	const int frames_idx =  ((typeFlags & MESH_INDEXED) ? 2 : 1);
	const int framenum = ((typeFlags & MESH_MORPHTARGET) ? ent->frame : 0);
	const size_t framestride = sizeof(mesh_framevbo_t);
	const size_t basestride = R_Mesh_GetBaseVBO_Stride (typeFlags);
	
	// base (non-frame-specific) data
	GL_BindVBO (mod->vboIDs[0]);
	R_TexCoordPointer (0, basestride, (void *)FOFS (nonskeletal_basevbo_t, st));
	if ((typeFlags & MESH_SKELETAL))
	{
		// Note: bone positions are sent separately as uniforms to the GLSL shader.
		R_AttribPointer (ATTR_WEIGHTS_IDX, 4, GL_UNSIGNED_BYTE, GL_TRUE, basestride, (void *)FOFS (skeletal_basevbo_t, blendweights));
		R_AttribPointer (ATTR_BONES_IDX, 4, GL_UNSIGNED_BYTE, GL_FALSE, basestride, (void *)FOFS (skeletal_basevbo_t, blendindices));
		if ((typeFlags & MESH_LM_SEPARATE_COORDS))
			R_TexCoordPointer (1, basestride, (void *)FOFS (skeletal_lm_basevbo_t, lm_st));
	}
	else if ((typeFlags & MESH_LM_SEPARATE_COORDS))
	{
		R_TexCoordPointer (1, basestride, (void *)FOFS (nonskeletal_lm_basevbo_t, lm_st));
	}
	
	// primary frame data
	GL_BindVBO (mod->vboIDs[framenum + frames_idx]);
	R_VertexPointer (3, framestride, (void *)FOFS (mesh_framevbo_t, vertex));
	R_NormalPointer (framestride, (void *)FOFS (mesh_framevbo_t, normal));
	R_AttribPointer (ATTR_TANGENT_IDX, 4, GL_FLOAT, GL_FALSE, framestride, (void *)FOFS (mesh_framevbo_t, tangent));
	
	// secondary frame data
	if ((typeFlags & MESH_MORPHTARGET) && lerped)
	{
		// Note: the lerp position is sent separately as a uniform to the GLSL shader.
		GL_BindVBO (mod->vboIDs[ent->oldframe + frames_idx]);
		R_AttribPointer (ATTR_OLDVTX_IDX, 3, GL_FLOAT, GL_FALSE, framestride, (void *)FOFS (mesh_framevbo_t, vertex));
		R_AttribPointer (ATTR_OLDNORM_IDX, 3, GL_FLOAT, GL_FALSE, framestride, (void *)FOFS (mesh_framevbo_t, normal));
		R_AttribPointer (ATTR_OLDTAN_IDX, 4, GL_FLOAT, GL_FALSE, framestride, (void *)FOFS (mesh_framevbo_t, tangent));
	}
	
	GL_BindVBO (0);
	
	// render
	if ((typeFlags & MESH_INDEXED))
	{
		GL_BindIBO (mod->vboIDs[1]);
		qglDrawElements (GL_TRIANGLES, mod->num_triangles*3, GL_UNSIGNED_INT, 0);
		GL_BindIBO (0);
	}
	else
	{
		qglDrawArrays (GL_TRIANGLES, 0, mod->num_triangles*3);
	}
	
	R_KillVArrays ();
}

static const entity_t *callback_ent;
static const model_t *callback_mod;
static void R_Mesh_DrawVBO_Callback (void)
{
	R_Mesh_DrawVBO (callback_ent, callback_mod, false);
}

/*
=============
R_Mesh_DrawFrame: should be able to handle all types of meshes.
=============
*/
static void R_Mesh_DrawFrame (const entity_t *ent, const model_t *mod, const vec3_t statLightPosition, const vec3_t statLightColor)
{
	vec3_t tmp;
	
	// model-space static light position
	vec3_t		lightVec;
	
	// only applicable to MD2
	qboolean	lerped;
	
	// if true, then render through the RScript code
	qboolean	rs_slowpath;
	
	rscript_t	*rs = NULL;
	
	qboolean	fixed_function_lightmap = false;
	
	if (r_shaders->integer)
		rs = (rscript_t *)ent->script;
	
	//check for valid script
	rs_slowpath = rs != NULL && rs->stage != NULL && (
		(rs->flags & RS_PREVENT_FASTPATH) != 0 ||
		rs->stage->texture == r_notexture ||
		((rs->stage->fx || rs->stage->glow) && rs->stage->texture2 == r_notexture) ||
		(rs->stage->cube && rs->stage->texture3 == r_notexture) ||
		rs->stage->num_blend_textures != 0 || rs->stage->next != NULL
	);
	
	lerped = ent->backlerp != 0.0 && (ent->frame != 0 || mod->num_frames != 1);
	
	if ((((mod->typeFlags & MESH_MORPHTARGET) && lerped) || (mod->typeFlags & MESH_SKELETAL) != 0) && rs_slowpath)
	{
		// FIXME: rectify this.
		Com_Printf ("WARN: Cannot apply a multi-stage RScript %s to model %s\n", rs->outname, mod->name);
		rs = NULL;
		rs_slowpath = false;
	}
	
	if (rs_slowpath)
	{
		int lmtex = 0;
		rs_lightmaptype_t lm = rs_lightmap_off;
		vec2_t rotate_center = {0.5f, 0.5f};
		
		if (mod->lightmap != NULL)
		{
			lmtex = mod->lightmap->texnum;
			lm = (mod->typeFlags & MESH_LM_SEPARATE_COORDS) ? rs_lightmap_separate_texcoords : rs_lightmap_on;
		}

		callback_ent = ent;
		callback_mod = mod;
		RS_Draw (	rs, ent, lmtex, rotate_center, vec3_origin, false, lm,
					true, true, R_Mesh_DrawVBO_Callback );
		
		return;
	}
	
	VectorSubtract (statLightPosition, ent->origin, tmp);
	VectorMA (statLightPosition, 5.0, tmp, tmp);
	R_ModelViewTransform (tmp, lightVec);
	
	if ((ent->flags & RF_TRANSLUCENT) && !(ent->flags & RF_SHELL_ANY))
	{
		qglDepthMask(false);
		
		R_Mesh_SetupGlassRender (ent, mod, lightVec);
	}
	else
	{
		qboolean fragmentshader, shell;
		
		if (rs && rs->stage->depthhack)
			qglDepthMask(false);
		
		shell = (ent->flags & RF_SHELL_ANY) != 0;
		fragmentshader = r_meshnormalmaps->integer && (shell || (rs && rs->stage->normalmap));
		fixed_function_lightmap = mod->lightmap != NULL && !fragmentshader;
		
		R_Mesh_SetupStandardRender (ent, mod, rs, fragmentshader, shell, lightVec, statLightColor);
	}
	
	R_Mesh_DrawVBO (ent, mod, lerped);
	
	glUseProgramObjectARB (0);
	qglDepthMask (true);
	
	if (fixed_function_lightmap)
	{
		GL_EnableTexture (1, false);
		GL_SelectTexture (0);
		GL_TexEnv (GL_MODULATE);
	}

	GLSTATE_DISABLE_ALPHATEST
	GLSTATE_DISABLE_BLEND
	
	// FIXME: make this unnecessary
	// Without this, the player options menu goes all funny due to
	// R_Mesh_SetupGlassRender changing the blendfunc. The proper solution is
	// to just never rely on the blendfunc being any default value.
	GL_BlendFunction (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

}

static void R_Mesh_DrawBlankMeshFrame (const entity_t *ent, const model_t *mod)
{
	image_t *skin;

	glUseProgramObjectARB (g_blankmeshprogramObj);
	R_Mesh_SetupAnimUniforms (ent, mod, &blankmesh_uniforms.anim_uniforms);
	
	if (ent->skin) 
		skin = ent->skin;
	else
		skin = mod->skins[0];
	if (!skin)
		skin = r_notexture;	// fallback..

	GL_MBind (0, skin->texnum);
	glUniform1iARB (blankmesh_uniforms.baseTex, 0);

	R_Mesh_DrawVBO (ent, mod, ent->frame != 0 || mod->num_frames != 1);

	glUseProgramObjectARB (0);
}

/*
=================
R_Mesh_Draw - animate and render a mesh. Should support all mesh types.
=================
*/
void R_Mesh_Draw (entity_t *ent, const model_t *mod)
{
	vec3_t statLightPosition, statLightColor;
	cull_result_t cullresult;
	
	cullresult = R_Mesh_CullModel (ent, mod);

	if (cullresult == draw_none)
		return;
	
	R_GetStaticLightingForEnt (ent, mod, statLightPosition, statLightColor);

	if(mod->type == mod_terrain)
	{
		if(r_nosun)
			VectorCopy (statLightPosition, r_worldLightVec);
		else
			VectorCopy (r_sunLight->origin, r_worldLightVec);
	}

	if ((mod->typeFlags & MESH_CASTSHADOWMAP))
		R_GenerateEntityShadow (ent, statLightPosition);
	
	if (cullresult == draw_shadow_only)
		return;
	
	if (r_showpolycounts->integer)
	{
		rendered_models[num_rendered_models].ent = ent;
		rendered_models[num_rendered_models].mod = mod;
		num_rendered_models++;
	}
	
	if (mod->type == mod_md2)
		MD2_SelectFrame (ent, mod);
	// New model types go here

	if (!(ent->flags & RF_WEAPONMODEL))
		c_alias_polys += mod->num_triangles; /* for rspeed_epoly count */

	if ((ent->flags & RF_DEPTHHACK)) // hack the depth range to prevent view model from poking into walls
		qglDepthRange (gldepthmin, gldepthmin + 0.3*(gldepthmax-gldepthmin));

	if ((ent->flags & RF_WEAPONMODEL))
	{
		qglMatrixMode(GL_PROJECTION);
		qglPushMatrix();
		qglLoadIdentity();

		if (r_lefthand->integer == 1)
		{
			qglScalef(-1, 1, 1);
			qglCullFace(GL_BACK);
		}
		if (r_newrefdef.fov_y < 75.0f)
			MYgluPerspective(r_newrefdef.fov_y, (float)r_newrefdef.width / (float)r_newrefdef.height, 4.0f, 4096.0f);
		else
			MYgluPerspective(75.0f, (float)r_newrefdef.width / (float)r_newrefdef.height, 4.0f, 4096.0f);

		qglMatrixMode(GL_MODELVIEW);
	}

	qglPushMatrix ();
	if (!ent->ragdoll) // HACK
		R_RotateForEntity (ent);

	qglShadeModel (GL_SMOOTH);
	GL_MTexEnv (0, GL_MODULATE);
	
	if ((mod->typeFlags & MESH_DECAL))
	{
		qglEnable (GL_POLYGON_OFFSET_FILL);
		qglPolygonOffset (-3, -2);
		qglDepthMask (false);
	}

	R_Mesh_DrawFrame (ent, mod, statLightPosition, statLightColor);
	
	if ((mod->typeFlags & MESH_DECAL))
	{
		qglDisable (GL_POLYGON_OFFSET_FILL);
		qglDepthMask (true);
	}
	
	if (((mod->typeFlags & MESH_DECAL) && gl_showdecals->integer > 1) || gl_showtris->integer)
	{
		qglLineWidth (3.0);
		qglPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
		qglDepthMask (false);
		if (gl_showtris->integer > 1) qglDisable (GL_DEPTH_TEST);
		R_Mesh_DrawBlankMeshFrame (ent, mod);
		if (gl_showtris->integer > 1) qglEnable (GL_DEPTH_TEST);
		qglDepthMask (true);
		qglPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
		qglLineWidth (1.0);
	}
	
	GL_MTexEnv (0, GL_REPLACE);
	qglShadeModel (GL_FLAT);

	qglPopMatrix ();
	
	if ((ent->flags & RF_WEAPONMODEL))
	{
		qglMatrixMode (GL_PROJECTION);
		qglPopMatrix ();
		qglMatrixMode (GL_MODELVIEW);
		qglCullFace (GL_FRONT);
	}

	r_nonSunStaticShadowsOn = false; // reset after backend render
	
	if ((ent->flags & RF_DEPTHHACK)) // restore depth range
		qglDepthRange (gldepthmin, gldepthmax);
	
	qglColor4f (1,1,1,1);

	if (r_minimap->integer && !ent->ragdoll && (ent->flags & RF_MONSTER))
	{
		Vector4Set (RadarEnts[numRadarEnts].color, 1.0, 0.0, 2.0, 1.0);
		VectorCopy (ent->origin, RadarEnts[numRadarEnts].org);
		numRadarEnts++;
	}
}

// TODO - alpha and alphamasks possible?
// Should support every mesh type
void R_Mesh_DrawCaster (entity_t *ent, const model_t *mod)
{
	// don't draw weapon model shadow casters or shells
	if ((ent->flags & (RF_WEAPONMODEL|RF_SHELL_ANY)))
		return;

	if (R_Mesh_CullModel (ent, mod) == draw_none)
		return;

	qglPushMatrix ();
	if (!ent->ragdoll) // HACK
	{
		qglTranslatef (ent->origin[0],  ent->origin[1],  ent->origin[2]);
		qglRotatef (ent->angles[YAW],		0, 0, 1);
		// pitch and roll are handled by IQM_AnimateFrame for player models(any IQM greater than 173 frames). 
		if (ent->model == NULL || (ent->flags & RF_WEAPONMODEL) || (ent->model->num_poses < 173))
		{
			qglRotatef (ent->angles[PITCH],	0, 1, 0);
			qglRotatef (ent->angles[ROLL],	1, 0, 0);
		}
	}
	
	if (mod->type == mod_md2)
		MD2_SelectFrame (ent, mod);
	// New model types go here

	GLSTATE_ENABLE_ALPHATEST

	R_Mesh_DrawBlankMeshFrame (ent, mod);

	qglPopMatrix();

	GLSTATE_DISABLE_ALPHATEST
}

void R_Mesh_ExtractLightmap (entity_t *ent, const model_t *mod)
{
	vec3_t statLightPosition, statLightColor;
	vec3_t lightVec, lightVal;
	vec3_t tmp;
	float lightVal_magnitude;

	if (mod->type == mod_md2)
		MD2_SelectFrame (ent, mod);
	// New model types go here
	
	R_GetStaticLightingForEnt (ent, mod, statLightPosition, statLightColor);
	
	VectorSubtract (statLightPosition, ent->origin, tmp);
	VectorMA (statLightPosition, 5.0, tmp, tmp);
	R_ModelViewTransform (tmp, lightVec);
	
	glUseProgramObjectARB (g_extractlightmapmeshprogramObj);
	
	R_Mesh_SetupAnimUniforms (ent, mod, &mesh_extract_lightmap_uniforms.anim_uniforms);
	
	glUniform3fvARB (mesh_extract_lightmap_uniforms.staticLightPosition, 1, (const GLfloat *)lightVec);
	
	VectorCopy (statLightColor, lightVal);
	lightVal_magnitude = 1.65f * VectorNormalize (lightVal);
	lightVal_magnitude = clamp (lightVal_magnitude, 0.1f, 0.35f * gl_modulate->value);
	VectorScale (lightVal, lightVal_magnitude, lightVal);
	glUniform3fvARB (mesh_extract_lightmap_uniforms.staticLightColor, 1, (const GLfloat *)lightVal);
	
	R_Mesh_DrawVBO (ent, mod, ent->frame != 0 || mod->num_frames != 1);

	glUseProgramObjectARB (0);
}
