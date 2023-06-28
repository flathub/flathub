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

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"
#include "r_iqm.h"
#include "r_ragdoll.h"

// This function will compute the quaternion's W based on its X, Y, and Z.)
void Vec4_CompleteQuatW (vec4_t q)
{
	vec3_t q_xyz;
	
	VectorCopy (q, q_xyz);
	q[3] = -sqrt (max (1.0 - pow (VectorLength (q_xyz), 2), 0.0));
}

void Matrix3x4ForEntity (matrix3x4_t *out, const entity_t *ent)
{
	matrix3x4_t rotmat;
	vec3_t rotaxis;
	Vector4Set(out->a, 1, 0, 0, 0);
	Vector4Set(out->b, 0, 1, 0, 0);
	Vector4Set(out->c, 0, 0, 1, 0);
	if(ent->angles[2])
	{
		VectorSet(rotaxis, -1, 0, 0);
		Matrix3x4GenRotate(&rotmat, DEG2RAD (ent->angles[2]), rotaxis);
		Matrix3x4_Multiply(out, rotmat, *out);
	}
	if(ent->angles[0])
	{
		VectorSet(rotaxis, 0, -1, 0);
		Matrix3x4GenRotate(&rotmat, DEG2RAD (ent->angles[0]), rotaxis);
		Matrix3x4_Multiply(out, rotmat, *out);
	}
	if(ent->angles[1])
	{
		VectorSet(rotaxis, 0, 0, 1);
		Matrix3x4GenRotate(&rotmat, DEG2RAD (ent->angles[1]), rotaxis);
		Matrix3x4_Multiply(out, rotmat, *out);
	}
	out->a[3] += ent->origin[0];
	out->b[3] += ent->origin[1];
	out->c[3] += ent->origin[2];
}

void Matrix3x4GenFromODE(matrix3x4_t *out, const dReal *rot, const dReal *trans)
{
	Vector4Set(out->a, rot[0], rot[1], rot[2], trans[0]);
	Vector4Set(out->b, rot[4], rot[5], rot[6], trans[1]);
	Vector4Set(out->c, rot[8], rot[9], rot[10], trans[2]);
}

static qboolean IQM_ReadSkinFile(char skin_file[MAX_OSPATH], char *skinpath)
{
	FILE *fp;
	int length;
	char *buffer;
	const char *s;

	if((fp = fopen(skin_file, "rb" )) == NULL)
	{
		return false;
	}
	else
	{
		size_t sz;
		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		buffer = malloc( length + 1 );
		sz = fread( buffer, length, 1, fp );
		buffer[length] = 0;
	}
	s = buffer;

	strcpy (skinpath, COM_Parse (&s));
	skinpath[length] = 0; //clear any possible garbage

	if ( fp != 0 )
	{
		fclose(fp);
		free( buffer );
	}
	else
	{
		FS_FreeFile( buffer );
	}
	return true;
}

qboolean IQM_ReadRagDollFile(char ragdoll_file[MAX_OSPATH], model_t *mod)
{
	FILE *fp;
	int length;
	char *buffer;
	const char *s;
	int result;
	char a_string[128];
	int i;

	if((fp = fopen(ragdoll_file, "rb" )) == NULL)
	{
		return false;
	}
	else
	{
		length = FS_filelength( fp );

		buffer = malloc( length + 1 );
		if ( buffer != NULL )
		{
			buffer[length] = 0;
			result = fread( buffer, length, 1, fp );
			if ( result == 1 )
			{
				s = buffer;

				for(i = 0; i < RAGDOLL_DIMS; i++)
				{
					strcpy (a_string, COM_Parse (&s));
					mod->ragdoll.RagDollDims[i] = atof(a_string);
				}
				strcpy (a_string, COM_Parse (&s));
				mod->ragdoll.hasHelmet = atoi(a_string);
			}
			else
			{
				Com_Printf("IQM_ReadRagDollFile: read fail\n");
			}
			free( buffer );
		}
		fclose( fp );
	}

	return true;
}

// NOTE: this should be the only function that needs to care what version the
// IQM file is.
qboolean Mod_INTERQUAKEMODEL_Load(model_t *mod, void *buffer)
{
	iqmheader_t *header;
	int i, j, k;
	unsigned int *vtriangles = NULL;
	skeletal_basevbo_t *basevbo = NULL;
	mesh_framevbo_t *framevbo = NULL;
	unsigned char *pbase;
	iqmjoint_t *joint = NULL;
	iqmjoint2_t *joint2 = NULL;
	matrix3x4_t	*inversebaseframe;
	iqmpose_t *poses;
	iqmpose2_t *poses2;
	iqmbounds_t *bounds;
	iqmvertexarray_t *va;
	unsigned short *framedata;
	char *text;
	char skinname[MAX_QPATH], shortname[MAX_QPATH], fullname[MAX_OSPATH];
	char *pskinpath_buffer;
	int skinpath_buffer_length;
	const char *parse_string;
	int remaining_varray_types;

	pbase = (unsigned char *)buffer;
	header = (iqmheader_t *)buffer;
	if (memcmp(header->id, "INTERQUAKEMODEL", 16))
	{
		Com_Printf ("Mod_INTERQUAKEMODEL_Load: %s is not an Inter-Quake Model", mod->name);
		return false;
	}

	if (LittleLong(header->version) != 1 && LittleLong(header->version) != 2)
	{
		Com_Printf ("Mod_INTERQUAKEMODEL_Load: only version 1 or 2 models are currently supported (name = %s)", mod->name);
		return false;
	}

	mod->type = mod_iqm;
	mod->typeFlags = MESH_CASTSHADOWMAP | MESH_SKELETAL | MESH_INDEXED | MESH_DOSHADING;

	// byteswap header
	header->version = LittleLong(header->version);
	header->filesize = LittleLong(header->filesize);
	header->flags = LittleLong(header->flags);
	header->num_text = LittleLong(header->num_text);
	header->ofs_text = LittleLong(header->ofs_text);
	header->num_meshes = LittleLong(header->num_meshes);
	header->ofs_meshes = LittleLong(header->ofs_meshes);
	header->num_vertexarrays = LittleLong(header->num_vertexarrays);
	header->num_vertexes = LittleLong(header->num_vertexes);
	header->ofs_vertexarrays = LittleLong(header->ofs_vertexarrays);
	header->num_triangles = LittleLong(header->num_triangles);
	header->ofs_triangles = LittleLong(header->ofs_triangles);
	header->num_joints = LittleLong(header->num_joints);
	header->ofs_joints = LittleLong(header->ofs_joints);
	header->num_poses = LittleLong(header->num_poses);
	header->ofs_poses = LittleLong(header->ofs_poses);
	header->num_anims = LittleLong(header->num_anims);
	header->ofs_anims = LittleLong(header->ofs_anims);
	header->num_frames = LittleLong(header->num_frames);
	header->num_framechannels = LittleLong(header->num_framechannels);
	header->ofs_frames = LittleLong(header->ofs_frames);
	header->ofs_bounds = LittleLong(header->ofs_bounds);
	header->num_comment = LittleLong(header->num_comment);
	header->ofs_comment = LittleLong(header->ofs_comment);
	header->num_extensions = LittleLong(header->num_extensions);
	header->ofs_extensions = LittleLong(header->ofs_extensions);

	if (header->num_triangles < 1 || header->num_vertexes < 3 || header->num_vertexarrays < 1 || header->num_meshes < 1)
	{
		Com_Printf("%s has no geometry\n", mod->name);
		return false;
	}
	if (header->num_frames < 1 || header->num_anims < 1)
	{
		Com_Printf("%s has no animations\n", mod->name);
		return false;
	}
	
	mod->num_frames = header->num_anims;
	mod->num_joints = header->num_joints;
	mod->num_poses = header->num_frames;
	mod->numvertexes = header->num_vertexes;
	mod->num_triangles = header->num_triangles;

	mod->extradata = Hunk_Begin (0x150000);
	
	basevbo = malloc (mod->numvertexes * sizeof(*basevbo));
	framevbo = malloc (mod->numvertexes * sizeof(*framevbo));
	
	remaining_varray_types = 6;
#define map_vertdata(dest,field,fieldtype,fieldconvert) \
do { \
	int j; \
	const fieldtype *buf = (fieldtype *)(pbase + va[i].offset); \
	for (j = 0; j < mod->numvertexes; j++) \
	{ \
		unsigned k; \
		for (k = 0; k < va[i].size; k++) \
			dest[j].field[k] = fieldconvert (*buf++); \
	} \
	remaining_varray_types--; \
} while (0) // do-while to allow semicolon after

	va = (iqmvertexarray_t *)(pbase + header->ofs_vertexarrays);
	for (i = 0;i < (int)header->num_vertexarrays;i++)
	{
		va[i].type = LittleLong(va[i].type);
		va[i].flags = LittleLong(va[i].flags);
		va[i].format = LittleLong(va[i].format);
		va[i].size = LittleLong(va[i].size);
		va[i].offset = LittleLong(va[i].offset);
		switch (va[i].type)
		{
		case IQM_POSITION:
			if (va[i].format == IQM_FLOAT && va[i].size == 3)
				map_vertdata (framevbo, vertex, float, LittleFloat);
			break;
		case IQM_TEXCOORD:
			if (va[i].format == IQM_FLOAT && va[i].size == 2)
				map_vertdata (basevbo, common.st, float, LittleFloat);
			break;
		case IQM_NORMAL:
			if (va[i].format == IQM_FLOAT && va[i].size == 3)
				map_vertdata (framevbo, normal, float, LittleFloat);
			break;
		case IQM_TANGENT:
			if (va[i].format == IQM_FLOAT && va[i].size == 4)
				map_vertdata (framevbo, tangent, float, LittleFloat);
			break;
		case IQM_BLENDINDEXES:
			if (va[i].format == IQM_UBYTE && va[i].size == 4)
				map_vertdata (basevbo, blendindices, byte,);
			break;
		case IQM_BLENDWEIGHTS:
			if (va[i].format == IQM_UBYTE && va[i].size == 4)
				map_vertdata (basevbo, blendweights, byte,);
			break;
		}
	}
	if (remaining_varray_types != 0)
	{
		Com_Printf("%s is missing vertex array data\n", mod->name);
		mod->extradatasize = Hunk_End ();
		free (framevbo);
		free (basevbo);
		return false;
	}

	text = header->num_text && header->ofs_text ? (char *)(pbase + header->ofs_text) : "";

	mod->jointname = (char *)Hunk_Alloc(header->num_text * sizeof(char *));
	memcpy(mod->jointname, text, header->num_text * sizeof(char *));

	// load the joints
	mod->joints = (iqmjoint2_t*)Hunk_Alloc (header->num_joints * sizeof(iqmjoint2_t));
	if (header->version == 1)
	{
		joint = (iqmjoint_t *) (pbase + header->ofs_joints);
		for (i = 0;i < mod->num_joints;i++)
		{
			mod->joints[i].name = LittleLong(joint[i].name);
			mod->joints[i].parent = LittleLong(joint[i].parent);
			for (j = 0;j < 3;j++)
			{
				mod->joints[i].origin[j] = LittleFloat(joint[i].origin[j]);
				mod->joints[i].rotation[j] = LittleFloat(joint[i].rotation[j]);
				mod->joints[i].scale[j] = LittleFloat(joint[i].scale[j]);
			}
			// If the quaternion w is not already included, calculate it.
			Vec4_CompleteQuatW (mod->joints[i].rotation);
		}
	}
	else
	{
		joint2 = (iqmjoint2_t *) (pbase + header->ofs_joints);
		for (i = 0;i < mod->num_joints;i++)
		{
			mod->joints[i].name = LittleLong(joint2[i].name);
			mod->joints[i].parent = LittleLong(joint2[i].parent);
			for (j = 0;j < 3;j++)
			{
				mod->joints[i].origin[j] = LittleFloat(joint2[i].origin[j]);
				mod->joints[i].rotation[j] = LittleFloat(joint2[i].rotation[j]);
				mod->joints[i].scale[j] = LittleFloat(joint2[i].scale[j]);
			}
			mod->joints[i].rotation[3] = LittleFloat(joint2[i].rotation[3]);
		}
	}
	
	// needed for bending the model in other ways besides the built-in
	// animation.
	mod->baseframe = (matrix3x4_t*)Hunk_Alloc (header->num_joints * sizeof(matrix3x4_t));
	
	// this doesn't need to be a part of mod - remember to free it
	inversebaseframe = (matrix3x4_t*)malloc (header->num_joints * sizeof(matrix3x4_t));

	for(i = 0; i < (int)header->num_joints; i++)
	{
		iqmjoint2_t j = mod->joints[i];

		Matrix3x4_FromQuatAndVectors(&mod->baseframe[i], j.rotation, j.origin, j.scale);
		Matrix3x4_Invert(&inversebaseframe[i], mod->baseframe[i]);

		assert(j.parent < (int)header->num_joints);

		if(j.parent >= 0)
		{
			matrix3x4_t temp;
			Matrix3x4_Multiply(&temp, mod->baseframe[j.parent], mod->baseframe[i]);
			mod->baseframe[i] = temp;
			Matrix3x4_Multiply(&temp, inversebaseframe[i], inversebaseframe[j.parent]);
			inversebaseframe[i] = temp;
		}
	}

	{
		int translate_size, rotate_size, scale_size;
		
		if (header->version == 1)
		{
			translate_size = 3;
			rotate_size = 3;

			scale_size = 3;
		}
		else
		{
			translate_size = 3;
			rotate_size = 4; // quaternion w is included in the v2 file format
			scale_size = 3;
		}
		
		poses = (iqmpose_t *) (pbase + header->ofs_poses);
		poses2 = (iqmpose2_t *) (pbase + header->ofs_poses);
		mod->frames = (matrix3x4_t*)Hunk_Alloc (header->num_frames * header->num_poses * sizeof(matrix3x4_t));
		framedata = (unsigned short *) (pbase + header->ofs_frames);

		for(i = 0; i < header->num_frames; i++)
		{
			for(j = 0; j < header->num_poses; j++)
			{
				int datanum;
				
				signed int parent;
				unsigned int channelmask;
				float *channeloffset, *channelscale;
				vec3_t translate, scale;
				vec4_t rotate;
				matrix3x4_t m, temp;
				
				if (header->version == 1)
				{
					parent = LittleLong(poses[j].parent);
					channelmask = LittleLong(poses[j].channelmask);
					channeloffset = poses[j].channeloffset;
					channelscale = poses[j].channelscale;
				}
				else
				{
					parent = LittleLong(poses2[j].parent);
					channelmask = LittleLong(poses2[j].channelmask);
					channeloffset = poses2[j].channeloffset;
					channelscale = poses2[j].channelscale;
				}

				datanum = 0;
				for (k = 0; k < translate_size; k++, datanum++)
				{
					translate[k] = LittleFloat (channeloffset[datanum]);
					if ((channelmask & (1<<datanum)))
						translate[k] += (unsigned short)LittleShort(*framedata++) * LittleFloat (channelscale[datanum]);
				}
				for (k = 0; k < rotate_size; k++, datanum++)
				{
					rotate[k] = LittleFloat (channeloffset[datanum]);
					if ((channelmask & (1<<datanum)))
						rotate[k] += (unsigned short)LittleShort(*framedata++) * LittleFloat (channelscale[datanum]);
				}
				for (k = 0; k < scale_size; k++, datanum++)
				{
					scale[k] = LittleFloat (channeloffset[datanum]);
					if ((channelmask & (1<<datanum)))
						scale[k] += (unsigned short)LittleShort(*framedata++) * LittleFloat (channelscale[datanum]);
				}
				// Concatenate each pose with the inverse base pose to avoid doing this at animation time.
				// If the joint has a parent, then it needs to be pre-concatenated with its parent's base pose.
				// Thus it all negates at animation time like so:
				//   (parentPose * parentInverseBasePose) * (parentBasePose * childPose * childInverseBasePose) =>
				//   parentPose * (parentInverseBasePose * parentBasePose) * childPose * childInverseBasePose =>
				//   parentPose * childPose * childInverseBasePose

				// If the quaternion w is not already included, calculate it.
				if (header->version == 1)
					Vec4_CompleteQuatW (rotate);

				Matrix3x4_FromQuatAndVectors(&m, rotate, translate, scale);

				if(parent >= 0)
				{
					Matrix3x4_Multiply(&temp, mod->baseframe[parent], m);
					Matrix3x4_Multiply(&mod->frames[i*header->num_poses+j], temp, inversebaseframe[j]);
				}
				else
					Matrix3x4_Multiply(&mod->frames[i*header->num_poses+j], m, inversebaseframe[j]);
			}
		}
	}
	
	// load bounding box data
	if (header->ofs_bounds)
	{
		float radius = 0;
		bounds = (iqmbounds_t *) (pbase + header->ofs_bounds);
		VectorClear(mod->mins);
		VectorClear(mod->maxs);
		for (i = 0; i < (int)header->num_frames;i++)
		{
			for (j = 0; j < 3; j++)
			{
				bounds[i].mins[j] = LittleFloat(bounds[i].mins[j]);
				if (mod->mins[j] > bounds[i].mins[j])
					mod->mins[j] = bounds[i].mins[j];
				
				bounds[i].maxs[j] = LittleFloat(bounds[i].maxs[j]);
				if (mod->maxs[j] < bounds[i].maxs[j])
					mod->maxs[j] = bounds[i].maxs[j];
			}
			
			bounds[i].radius = LittleFloat(bounds[i].radius);
			if (bounds[i].radius > radius)
				radius = bounds[i].radius;
		}

		mod->radius = radius;
	}

	//compute a full bounding box
	for ( i = 0; i < 8; i++ )
	{
		vec3_t   tmp;

		if ( i & 1 )
			tmp[0] = mod->mins[0];
		else
			tmp[0] = mod->maxs[0];

		if ( i & 2 )
			tmp[1] = mod->mins[1];
		else
			tmp[1] = mod->maxs[1];

		if ( i & 4 )
			tmp[2] = mod->mins[2];
		else
			tmp[2] = mod->maxs[2];

		VectorCopy( tmp, mod->bbox[i] );
	}	

	vtriangles = (unsigned int *) (pbase + header->ofs_triangles);
	for(i = 0; i < mod->numvertexes*3; i++)
		vtriangles[i] = LittleLong (vtriangles[i]);

	/*
	 * get skin pathname from <model>.skin file and initialize skin
	 */
	COM_StripExtension( mod->name, shortname );
	strcat( shortname, ".skin" );
	skinpath_buffer_length = FS_LoadFile( shortname, (void**)&pskinpath_buffer );
		// note: FS_LoadFile handles upper/lowercase, nul-termination,
		//  and path search sequence

	if ( skinpath_buffer_length > 0 )
	{ // <model>.skin file found and read,
		// data is in Z_Malloc'd buffer pointed to by pskin_buffer

		// get relative image pathname for model's skin from .skin file data
		parse_string = pskinpath_buffer;
		strcpy (skinname, COM_Parse (&parse_string));
		Z_Free( pskinpath_buffer ); // free Z_Malloc'd read buffer

		// get image file for skin
		mod->skins[0] = GL_FindImage( skinname, it_skin );
		if ( mod->skins[0] != NULL )
		{ // image file for skin exists
			strcpy( mod->skinname, skinname );

			//load shader for skin
			mod->script = mod->skins[0]->script;
			if ( mod->script )
				RS_ReadyScript( (rscript_t *)mod->script );
		}
	}
	
	/*
	 * get ragdoll info from <model>.rgd file
	 */
	mod->hasRagDoll = false;
	COM_StripExtension( mod->name, shortname );
	strcat( shortname, ".rgd" );
	if ( FS_FullPath( fullname, sizeof(fullname), shortname ) )
	{
		mod->hasRagDoll = IQM_ReadRagDollFile( fullname, mod );
	}
	
	//free temp non hunk mem
	if(inversebaseframe)
		free(inversebaseframe);
	
	// load the VBO data
	R_Mesh_LoadVBO (mod, 0, basevbo, vtriangles, framevbo);
	
	mod->extradatasize = Hunk_End ();
	free (framevbo);
	free (basevbo);
	
	return true;
}

extern int server_tickrate;
static float IQM_SelectFrame (const entity_t *ent)
{
	float time;
	float FRAMETIME = 1.0f/(float)server_tickrate;
	
	//frame interpolation
	time = (Sys_Milliseconds () - ent->frametime) / 100;
	
	if (time > (1.0f - FRAMETIME))
		time = 1.0f - FRAMETIME;

	//Check for stopped death anims
	if (ent->frame == 257 || ent->frame == 237 || ent->frame == 219)
		time = 0.0f;	
	
	return ent->frame + time;
}

static int IQM_NextFrame (const entity_t *ent, const model_t *mod)
{
	int outframe;
	int frame = ent->frame;

	//just for now
	if (ent->flags & RF_WEAPONMODEL)
	{
		outframe = frame + 1;
	}
	else
	{
		switch(frame)
		{
			//map models can be 24 or 40 frames
			case 23:
				if (mod->num_poses > 24)
					outframe = frame + 1;
				else
					outframe = 0;
				break;
			//player standing
			case 39:
				outframe = 0;
				break;
			//player running
			case 45:
				outframe = 40;
				break;
			//player shooting
			case 53:
				outframe = 46;
				break;
			//player jumping - this one may not be necessary - it is not a looped animation
			case 71:
				outframe = 0;
				break;
			//player crouched
			case 153:
				outframe = 135;
				break;
			//player crouched walking
			case 159:
				outframe = 154;
				break;
			//player crouched shooting
			case 168:
				outframe = 160;
				break;
			//player sneaking
			case 180:
				outframe = 173;
				break;
			//deaths - do not advance animations at end
			case 219:
				outframe = 219;
				break;
			case 237:
				outframe = 237;
				break;
			case 257:
				outframe = 257;
				break;
			case 265:
				outframe = 260;
				break;
			case 271:
				outframe = 266;
				break;
			case 277:
				outframe = 272;
				break;
			case 284:
				outframe = 278;
				break;
			case 295:
				outframe = 285;
				break;
			default:
				outframe = frame + 1;
				break;
		}
	}
	return outframe;
}

static inline void IQM_Bend (matrix3x4_t *temp, matrix3x4_t rmat, matrix3x4_t *basejoint, matrix3x4_t *outjoint)
{
	vec3_t		basePosition, oldPosition, newPosition;
	
	// concatenate the rotation with the bone
	Matrix3x4_Multiply(temp, rmat, *outjoint);

	// get the position of the bone in the base frame
	VectorSet(basePosition, basejoint->a[3], basejoint->b[3], basejoint->c[3]);

	// add in the correct old bone position and subtract off the wrong new bone position to get the correct rotation pivot
	VectorSet(oldPosition,  DotProduct(basePosition, outjoint->a) + outjoint->a[3],
		 DotProduct(basePosition, outjoint->b) + outjoint->b[3],
		 DotProduct(basePosition, outjoint->c) + outjoint->c[3]);

	VectorSet(newPosition, DotProduct(basePosition, temp->a) + temp->a[3],
		 DotProduct(basePosition, temp->b) + temp->b[3],
		 DotProduct(basePosition, temp->c) + temp->c[3]);

	temp->a[3] += oldPosition[0] - newPosition[0];
	temp->b[3] += oldPosition[1] - newPosition[1];
	temp->c[3] += oldPosition[2] - newPosition[2];

	// replace the old matrix with the rotated one
	Matrix3x4_Copy(outjoint, *temp);
}

static void IQM_AnimateFrame_standard (const entity_t *ent, const model_t *mod, matrix3x4_t outframe[SKELETAL_MAX_BONEMATS])
{
	int frame1, frame2;
	float frameoffset;
	float curframe;
	int nextframe;
	const matrix3x4_t *mat1, *mat2;
	int i;
	float modelpitch;
	float modelroll;
	
	curframe = IQM_SelectFrame (ent);
	nextframe = IQM_NextFrame (ent, mod);

	frame1 =  (int)floor (curframe);
	frame2 = nextframe;
	frameoffset = curframe - frame1;
	frame1 %= mod->num_poses;
	frame2 %= mod->num_poses;

	mat1 = &mod->frames[frame1 * mod->num_joints];
	mat2 = &mod->frames[frame2 * mod->num_joints];
	
	modelpitch = DEG2RAD (ent->angles[PITCH]); 
	modelroll = DEG2RAD (ent->angles[ROLL]);
	
	// Interpolate matrixes between the two closest frames and concatenate with parent matrix if necessary.
	// Concatenate the result with the inverse of the base pose.
	// You would normally do animation blending and inter-frame blending here in a 3D engine.

	for (i = 0; i < mod->num_joints; i++)
	{
		const char *currentjoint_name;
		signed int currentjoint_parent;
		matrix3x4_t mat, rmat, temp;
		vec3_t rot;
		Matrix3x4_Scale (&mat, mat1[i], 1-frameoffset);
		Matrix3x4_Scale (&temp, mat2[i], frameoffset);

		Matrix3x4_Add (&mat, mat, temp);
		
		currentjoint_name = &mod->jointname[mod->joints[i].name];
		currentjoint_parent = mod->joints[i].parent;

		if (currentjoint_parent >= 0)
			Matrix3x4_Multiply (&outframe[i], outframe[currentjoint_parent], mat);
		else
			Matrix3x4_Copy (&outframe[i], mat);

		//bend the model at the waist for player pitch
		if (!strcmp(currentjoint_name, "Spine") || !strcmp(currentjoint_name, "Spine.001"))
		{
			VectorSet(rot, 0, 1, 0); //remember .iqm's are 90 degrees rotated from reality, so this is the pitch axis
			Matrix3x4GenRotate(&rmat, modelpitch, rot);
			
			IQM_Bend (&temp, rmat, &mod->baseframe[i], &outframe[i]);
		}
		//now rotate the legs back
		if (!strcmp(currentjoint_name, "hip.l") || !strcmp(currentjoint_name, "hip.r"))
		{
			VectorSet(rot, 0, 1, 0);
			Matrix3x4GenRotate(&rmat, -modelpitch, rot);
			
			IQM_Bend (&temp, rmat, &mod->baseframe[i], &outframe[i]);
		}

		//bend the model at the waist for player roll
		if (!strcmp(currentjoint_name, "Spine") || !strcmp(currentjoint_name, "Spine.001"))
		{
			VectorSet(rot, 1, 0, 0); //remember .iqm's are 90 degrees rotated from reality, so this is the pitch axis
			Matrix3x4GenRotate(&rmat, modelroll, rot);
			
			IQM_Bend (&temp, rmat, &mod->baseframe[i], &outframe[i]);
		}
		//now rotate the legs back
		if (!strcmp(currentjoint_name, "hip.l") || !strcmp(currentjoint_name, "hip.r"))
		{
			VectorSet(rot, 1, 0, 0);
			Matrix3x4GenRotate(&rmat, -modelroll, rot);
			
			IQM_Bend (&temp, rmat, &mod->baseframe[i], &outframe[i]);
		}
	}
}

static void IQM_AnimateFrame_ragdoll (const entity_t *ent, const model_t *mod, matrix3x4_t outframe[SKELETAL_MAX_BONEMATS])
{
	//we only deal with one frame

	//animate using the rotations from our corresponding ODE objects.
	int i, j;
	{
		for (i = 0; i < mod->num_joints; i++)
		{
			int parent;

			for (j = 0; j < RagDollBindsCount; j++)
			{
				if (!strcmp (&mod->jointname[mod->joints[i].name], RagDollBinds[j].name))
				{
					matrix3x4_t rmat;
					RagDollObject_t *object = &ent->RagDollData->RagDollObject[RagDollBinds[j].object];
					const dReal *odeRot = dBodyGetRotation (object->body);
					const dReal *odePos = dBodyGetPosition (object->body);
					Matrix3x4GenFromODE (&rmat, odeRot, odePos);
					Matrix3x4_Multiply (&outframe[i], rmat, object->initmat);
					goto nextjoint;
				}
			}

			parent = mod->joints[i].parent;
			if (parent >= 0)
			{
				matrix3x4_t mat;
				// FIXME: inefficient
				Matrix3x4_Invert (&mat, ent->RagDollData->initframe[parent]);
				Matrix3x4_Multiply (&mat, mat, ent->RagDollData->initframe[i]);
				Matrix3x4_Multiply (&outframe[i], outframe[parent], mat);
			}
			else
				memset (&outframe[i], 0, sizeof(matrix3x4_t));

		nextjoint:;
		}
	}
}

void IQM_AnimateFrame (const entity_t *ent, const model_t *mod, matrix3x4_t outframe[SKELETAL_MAX_BONEMATS])
{
	if (ent->ragdoll)
		IQM_AnimateFrame_ragdoll (ent, mod, outframe);
	else
		IQM_AnimateFrame_standard (ent, mod, outframe);
}
