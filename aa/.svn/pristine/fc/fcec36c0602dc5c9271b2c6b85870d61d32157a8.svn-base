/*
Copyright (C) 2010 COR Entertainment, LLC.

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

// gl_script.c - scripted texture rendering

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "r_local.h"

#if defined WIN32_VARIANT
#include <io.h>
#endif

#define		TOK_DELIMINATORS "\r\n\t "

float		rs_realtime = 0;
rscript_t	*rs_rootscript = NULL;

static int RS_Animate (rs_stage_t *stage)
{
	anim_stage_t	*anim = stage->last_anim;

	while (stage->last_anim_time < rs_realtime)
	{
		anim = anim->next;
		if (!anim)
			anim = stage->anim_stage;
		stage->last_anim_time += stage->anim_delay;
	}

	stage->last_anim = anim;

	return anim->texture->texnum;
}

static void RS_ResetScript (rscript_t *rs)
{
	rs_stage_t		*stage = rs->stage, *tmp_stage;
	anim_stage_t	*anim, *tmp_anim;
	random_stage_t	*randStage, *tmp_rand;

	rs->name[0] = 0;

	while (stage != NULL)
	{
		if (stage->anim_count)
		{
			anim = stage->anim_stage;
			while (anim != NULL)
			{
				tmp_anim = anim;
				
				anim = anim->next;
				free (tmp_anim);
			}
		}
		if (stage->rand_count)
		{
			randStage = stage->rand_stage;
			while (randStage != NULL)
			{
				tmp_rand = randStage;

				randStage = randStage->next;
				free (tmp_rand);
			}
		}

		tmp_stage = stage;
		stage = stage->next;

		free (tmp_stage);
	}
	
	rs->stage = NULL;
	rs->dontflush = false;	
	rs->ready = false;
}

static void rs_free_if_subexpr (rs_cond_val_t *expr)
{
	if (expr->lval.string)
		Z_Free (expr->lval.string);
	if (expr->subexpr1)
		rs_free_if_subexpr (expr->subexpr1);
	if (expr->subexpr2)
		rs_free_if_subexpr (expr->subexpr2);
	Z_Free (expr);
}

static void RS_ClearStage (rs_stage_t *stage)
{
	anim_stage_t	*anim = stage->anim_stage, *tmp_anim;
	random_stage_t	*randStage = stage->rand_stage, *tmp_rand;


	if (stage->condv != NULL)
	{
		rs_free_if_subexpr (stage->condv);
		stage->condv = NULL;
	}
	while (anim != NULL)
	{
		tmp_anim = anim;
		anim = anim->next;
		free (tmp_anim);
	}
	while (randStage != NULL)
	{
		tmp_rand = randStage;
		randStage = randStage->next;
		free (tmp_rand);
	}
	
	memset (stage, 0, sizeof(*stage));

	stage->lightmap = true;
}

// Create a new script with the given name. Reuse the "old" struct if possible
// to avoid breaking old pointers.
static rscript_t *RS_NewScript (char *name, rscript_t *old)
{
	rscript_t	*rs;
	unsigned int	i;

	if (old != NULL)
	{
		RS_ResetScript (old);
		rs = old;
	}
	else if (!rs_rootscript)
	{
		rs_rootscript = (rscript_t *)malloc(sizeof(rscript_t));
		rs = rs_rootscript;
		rs->next = NULL;
	}
	else
	{
		rs = rs_rootscript;

		while (rs->next != NULL)
			rs = rs->next;

		rs->next = (rscript_t *)malloc(sizeof(rscript_t));
		rs = rs->next;
		rs->next = NULL;
	}

	COMPUTE_HASH_KEY(rs->hash_key, name, i);
	strncpy (rs->name, name, sizeof(rs->name));

	rs->stage = NULL;
	rs->dontflush = false;	
	rs->ready = false;
	
	rs->flags = 0;

	return rs;
}

static rs_stage_t *RS_NewStage (rscript_t *rs)
{
	rs_stage_t	*stage;

	if (rs->stage == NULL)
	{
		rs->stage = (rs_stage_t *)malloc(sizeof(rs_stage_t));
		stage = rs->stage;
	}
	else
	{
		stage = rs->stage;
		while (stage->next != NULL)
			stage = stage->next;

		stage->next = (rs_stage_t *)malloc(sizeof(rs_stage_t));
		stage = stage->next;
	}

	strncpy (stage->name, "***r_notexture***", sizeof(stage->name));
	strncpy (stage->name2, "***r_notexture***", sizeof(stage->name2));
	strncpy (stage->name3, "***r_notexture***", sizeof(stage->name3));
	
	stage->rand_stage = NULL;
	stage->anim_stage = NULL;
	stage->next = NULL;
	stage->last_anim = NULL;
	stage->condv = NULL;

	RS_ClearStage (stage);

	return stage;
}

void RS_FreeAllScripts (void)
{
	rscript_t	*rs = rs_rootscript, *tmp_rs;

	while (rs != NULL)
	{
		tmp_rs = rs->next;
		RS_ResetScript(rs);
		free (rs);
		rs = tmp_rs;
	}
}

void RS_ReloadImageScriptLinks (void)
{
	image_t		*image;
	int			i;
	char		shortname[MAX_QPATH];

	for (i=0, image=gltextures ; i<numgltextures ; i++,image++)
	{
		COM_StripExtension (image->name, shortname);
		image->script = RS_FindScript (shortname);
	}

}

void RS_FreeScript(rscript_t *rs)
{
	rscript_t	*tmp_rs;

	if (!rs)
		return;

	if (rs_rootscript == rs)
	{
		rs_rootscript = rs_rootscript->next;
		RS_ResetScript(rs);
		free (rs);
		return;
	}

	tmp_rs = rs_rootscript;
	while (tmp_rs->next != rs)
		tmp_rs = tmp_rs->next;
	tmp_rs->next = rs->next;

	RS_ResetScript (rs);

	free (rs);
}

void RS_FreeUnmarked (void)
{
	image_t		*image;
	int			i;
	rscript_t	*rs = rs_rootscript, *tmp_rs;

	for (i=0, image=gltextures ; i<numgltextures ; i++,image++)
	{
		if (image->script && !((rscript_t *)image->script)->dontflush)
			image->script = NULL;
	}
	
	while (rs != NULL)
	{
		tmp_rs = rs->next;

		if (!rs->dontflush)
			RS_FreeScript(rs);

		rs = tmp_rs;
	}
}

rscript_t *RS_FindScript(char *name)
{
	rscript_t	*rs = rs_rootscript;
	unsigned int	i, hash_key;

	COMPUTE_HASH_KEY(hash_key, name, i);

	while (rs != NULL)
	{
		if (rs->hash_key == hash_key && !Q_strcasecmp(rs->name, name))
		{
			if (! rs->stage)
				rs = NULL;
			break;
		}

		rs = rs->next;
	}

	return rs;
}

void RS_ReadyScript (rscript_t *rs)
{
	rs_stage_t		*stage;
	anim_stage_t	*anim;
	random_stage_t	*randStage;
	char			mode;
	int				i;

	if (rs->ready)
		return;

	mode = (rs->dontflush) ? it_pic : it_wall;
	stage = rs->stage;

	while (stage != NULL)
	{
		//if normalmap, set mode
		if(stage->normalmap)
			mode = it_bump;

		//set anim
		anim = stage->anim_stage;
		while (anim != NULL)
		{
			anim->texture = GL_FindImage (anim->name, mode);
			if (!anim->texture)
				anim->texture = r_notexture;

			anim = anim->next;
		}

		//set tiling
		randStage = stage->rand_stage;
		while (randStage != NULL)
		{
			randStage->texture = GL_FindImage (randStage->name, mode);
			if (!randStage->texture)
				randStage->texture = r_notexture;

			randStage = randStage->next;
		}

		//set name
		if (stage->name[0])
			stage->texture = GL_FindImage (stage->name, mode);
		if (!stage->texture)
			stage->texture = r_notexture;
		if (stage->name2[0])
			stage->texture2 = GL_FindImage (stage->name2, mode);
		if (!stage->texture2)
			stage->texture2 = r_notexture;
		if (stage->name3[0])
			stage->texture3 = GL_FindImage (stage->name3, mode);
		if (!stage->texture3)
			stage->texture3 = r_notexture;
		for (i = 0; i < stage->num_blend_textures; i++)
		{
			stage->blend_textures[i] = GL_FindImage (stage->blend_names[i], mode);
			if (!stage->blend_textures[i])
				stage->blend_textures[i] = r_notexture;
		}
		
		for (i = 0; i < stage->num_blend_normalmaps; i++)
		{
			stage->blend_normalmaps[i] = GL_FindImage (stage->normalblend_names[i], mode);
			if (!stage->blend_normalmaps[i])
				stage->blend_normalmaps[i] = r_notexture;
		}

		stage = stage->next;
	}

	rs->ready = true;
}

static int RS_BlendID (char *blend)
{
	if (!blend[0])
		return 0;
	
#define check_blend(name) \
	if (!Q_strcasecmp (blend, #name)) \
		return name;
	
	check_blend (GL_ZERO)
	check_blend (GL_ONE)
	check_blend (GL_SRC_ALPHA)
	check_blend (GL_ONE_MINUS_SRC_ALPHA)
	check_blend (GL_SRC_COLOR)
	check_blend (GL_ONE_MINUS_SRC_COLOR)

#undef check_blend

	Com_Printf ("WARN: Unrecognized blend factor %s in RScript!\n", blend);
	return 0;
}

static int RS_FuncName (char *text)
{
	if (!Q_strcasecmp (text, "static"))			// static
		return 0;
	else if (!Q_strcasecmp (text, "sine"))		// sine wave
		return 1;
	else if (!Q_strcasecmp (text, "cosine"))	// cosine wave
		return 2;

	return 0;
}

/*
scriptname
{
	subdivide <size>
	vertexwarp <speed> <distance> <smoothness>
	safe
	{
		map <texturename>
		map2 <texturename> - specific to "fx"
		map3 <texturename> - specific to "cube"
		scroll <xtype> <xspeed> <ytype> <yspeed>
		blendfunc <source> <dest>
		alphashift <speed> <min> <max>
		anim <delay> <tex1> <tex2> <tex3> ... end
		envmap
		nolightmap
		alphamask
		lensflare
		flaretype
		normalmap
		grass
		grasstype
		beam
		beamtype
		xang
		yang
		rotating
		fx
		glow
		cube
	}
}
*/

#define RS_STAGE_STRING_ATTR(attrname) \
static void rs_stage_ ## attrname (rs_stage_t *stage, char **token) \
{ \
	*token = strtok (NULL, TOK_DELIMINATORS); \
	strncpy (stage->attrname, *token, sizeof(stage->attrname)); \
}

#define RS_STAGE_BOOL_ATTR(attrname) \
static void rs_stage_ ## attrname (rs_stage_t *stage, char **token) \
{ \
	(void)token; /* shut up compiler warnings */ \
	stage->attrname = true; \
}

#define RS_STAGE_FLOAT_ATTR(attrname) \
static void rs_stage_ ## attrname (rs_stage_t *stage, char **token) \
{ \
	*token = strtok (NULL, TOK_DELIMINATORS); \
	stage->attrname = (float)atof(*token); \
}

#define RS_STAGE_INT_ATTR(attrname) \
static void rs_stage_ ## attrname (rs_stage_t *stage, char **token) \
{ \
	*token = strtok (NULL, TOK_DELIMINATORS); \
	stage->attrname = atoi(*token); \
}

RS_STAGE_STRING_ATTR (name)
RS_STAGE_STRING_ATTR (name2)
RS_STAGE_STRING_ATTR (name3)
RS_STAGE_BOOL_ATTR (depthhack)
RS_STAGE_BOOL_ATTR (envmap)
RS_STAGE_BOOL_ATTR (alphamask)
RS_STAGE_BOOL_ATTR (lensflare)
RS_STAGE_INT_ATTR (flaretype)
RS_STAGE_BOOL_ATTR (normalmap)
RS_STAGE_BOOL_ATTR (grass)
RS_STAGE_INT_ATTR (grasstype)
RS_STAGE_BOOL_ATTR (beam)
RS_STAGE_INT_ATTR (beamtype)
RS_STAGE_BOOL_ATTR (fx)
RS_STAGE_BOOL_ATTR (glow)
RS_STAGE_BOOL_ATTR (cube)
RS_STAGE_FLOAT_ATTR (xang)
RS_STAGE_FLOAT_ATTR (yang)
RS_STAGE_FLOAT_ATTR (rot_speed)
RS_STAGE_INT_ATTR (rotating)


static void rs_stage_nolightmap (rs_stage_t *stage, char **token)
{
	stage->lightmap = false;
}

static void rs_stage_colormap (rs_stage_t *stage, char **token)
{
	stage->colormap.enabled = true;

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->colormap.red = atof(*token);

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->colormap.green = atof(*token);

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->colormap.blue = atof(*token);
}

static void rs_stage_scroll (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scroll.typeX = RS_FuncName(*token);
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scroll.speedX = atof(*token);

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scroll.typeY = RS_FuncName(*token);
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scroll.speedY = atof(*token);
}

static void rs_stage_blendfunc (rs_stage_t *stage, char **token)
{
	stage->blendfunc.blend = true;

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->blendfunc.source = RS_BlendID (*token);

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->blendfunc.dest = RS_BlendID (*token);
}

static void rs_stage_alphashift (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->alphashift.speed = (float)atof(*token);

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->alphashift.min = (float)atof(*token);

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->alphashift.max = (float)atof(*token);
}

static void rs_stage_random (rs_stage_t *stage, char **token)
{
	random_stage_t	*rand = (random_stage_t *)malloc(sizeof(random_stage_t));

	stage->rand_stage = rand;
	stage->rand_count = 0;

	*token = strtok(NULL, TOK_DELIMINATORS);

	while (Q_strcasecmp (*token, "end"))
	{
		stage->rand_count++;

		strncpy (stage->name, *token, sizeof(stage->name));

		stage->texture = NULL;

		*token = strtok(NULL, TOK_DELIMINATORS);

		if (!Q_strcasecmp (*token, "end"))
		{
			rand->next = NULL;
			break;
		}

		rand->next = (random_stage_t *)malloc(sizeof(random_stage_t));
		rand = rand->next;
	}
}

static void rs_stage_anim (rs_stage_t *stage, char **token)
{
	anim_stage_t	*anim = (anim_stage_t *)malloc(sizeof(anim_stage_t));

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->anim_delay = (float)atof(*token);

	stage->anim_stage = anim;
	stage->last_anim = anim;

	*token = strtok(NULL, TOK_DELIMINATORS);

	while (Q_strcasecmp (*token, "end"))
	{
		stage->anim_count++;

		strncpy (anim->name, *token, sizeof(anim->name));

		anim->texture = NULL;

		*token = strtok(NULL, TOK_DELIMINATORS);

		if (!Q_strcasecmp (*token, "end"))
		{
			anim->next = NULL;
			break;
		}

		anim->next = (anim_stage_t *)malloc(sizeof(anim_stage_t));
		anim = anim->next;
	}
}

static void rs_stage_scale (rs_stage_t *stage, char **token)
{
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scale.typeX = RS_FuncName(*token);
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scale.scaleX = atof(*token);

	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scale.typeY = RS_FuncName(*token);
	*token = strtok (NULL, TOK_DELIMINATORS);
	stage->scale.scaleY = atof(*token);
}

typedef struct 
{
	char			*opname;
	rs_cond_op_t	op;
} rs_cond_op_key_t;
static rs_cond_op_key_t rs_cond_op_keys[] = 
{
	{	"==",	rs_cond_eq		},
	{	"!=",	rs_cond_neq		},
	{	">",	rs_cond_gt		},
	{	"<=",	rs_cond_ngt		},
	{	"<",	rs_cond_lt		},
	{	">=",	rs_cond_nlt		},
	{	"&&",	rs_cond_and		},
	{	"||",	rs_cond_or		},
	
	{	NULL,	0				}
};
static rs_cond_val_t *rs_stage_if_subexpr (char **token)
{
	int i;
	rs_cond_val_t *res = Z_Malloc (sizeof(rs_cond_val_t));
	*token = strtok (NULL, TOK_DELIMINATORS);
	if (!(*token)) 
	{
		Z_Free (res);
		return NULL;
	}
	if (!Q_strcasecmp (*token, "(")) 
	{
		res->subexpr1 = rs_stage_if_subexpr (token);
		if (!res->subexpr1) //there was an error somewhere
		{
			Z_Free (res);
			return NULL;
		}
		*token = strtok (NULL, TOK_DELIMINATORS);
		if (!(*token)) {
			Com_Printf ("Ran out of tokens in stage conditional!\n");
			rs_free_if_subexpr (res->subexpr1);
			Z_Free (res);
			return NULL;
		}
		if (!Q_strcasecmp (*token, ")"))
		{
			res->optype = rs_cond_is;
			return res;
		}
		for (i = 0; rs_cond_op_keys[i].opname; i++)
			if (!Q_strcasecmp (*token, rs_cond_op_keys[i].opname))
				break;
		if (!rs_cond_op_keys[i].opname) 
		{
			Com_Printf ("Invalid stage conditional operation %s\n", *token);
			rs_free_if_subexpr (res->subexpr1);
			Z_Free (res);
			return NULL;
		}
		res->optype = rs_cond_op_keys[i].op;
		res->subexpr2 = rs_stage_if_subexpr (token);
		if (!res->subexpr2) //there was an error somewhere
		{
			rs_free_if_subexpr (res->subexpr1);
			Z_Free (res);
			return NULL;
		}
		*token = strtok (NULL, TOK_DELIMINATORS);
		if (!(*token) || Q_strcasecmp (*token, ")")) 
		{
			Com_Printf ("Missing ) in stage conditional!\n");
			if (*token)
				Com_Printf ("Instead got %s\n", *token);
			rs_free_if_subexpr (res->subexpr1);
			rs_free_if_subexpr (res->subexpr2);
			Z_Free (res);
			return NULL;
		}
		return res;
	} 
	else if (!Q_strcasecmp (*token, "!"))
	{
		res->optype = rs_cond_lnot;
		res->subexpr1 = rs_stage_if_subexpr (token);
		if (!res->subexpr1) // there was an error somewhere
		{
			Z_Free (res);
			return NULL;
		}
		return res;
	}
	else if (!Q_strcasecmp (*token, "$"))
	{
		res->optype = rs_cond_none;
		*token = strtok (NULL, TOK_DELIMINATORS);
		if (!(*token)) {
			Com_Printf ("Missing cvar name in stage conditional!\n");
			rs_free_if_subexpr (res->subexpr1);
			Z_Free (res);
			return NULL;
		}
		res->val = Cvar_Get (*token, "0", 0);
		return res;
	}
	else
	{
		res->optype = rs_cond_none;
		res->val = &(res->lval);
		Anon_Cvar_Set (res->val, *token);
		return res;
	}
}
static void rs_stage_if (rs_stage_t *stage, char **token)
{
	stage->condv = rs_stage_if_subexpr (token);
	if (!stage->condv)
		Com_Printf ("ERROR in stage conditional!\n");
}

static void rs_stage_blendmap (rs_stage_t *stage, char **token)
{
	int i, numtextures;
	
	*token = strtok (NULL, TOK_DELIMINATORS);
	numtextures = atoi (*token);
	
	stage->num_blend_textures = min (numtextures, 6);
	
	for (i = 0; i < numtextures; i++)
	{
		float scale[2];
		int j;
		
		*token = strtok (NULL, TOK_DELIMINATORS);
		scale[0] = 1.0f / atof (*token);
		
		*token = strtok (NULL, TOK_DELIMINATORS);
		for (j = 0; (*token)[j] != '\0' && isdigit ((*token)[j]); j++);
		if ((*token)[j] == '\0')
		{
			scale[1] = 1.0f / atof (*token);
			*token = strtok (NULL, TOK_DELIMINATORS);
		}
		else
		{
			scale[1] = scale[0];
		}
		
		if (i < 6) // Current maximum
		{
			for (j = 0; j < 2; j++)
				stage->blend_scales[2*i+j] = scale[j];
		
			strncpy (stage->blend_names[i], *token, sizeof (stage->blend_names[i]));
		}
		else
		{
			Com_Printf ("WARN: skip blendmap channel %d (max 6)\n", i+1);
		}
	}
}

static void rs_stage_blendnormalmap (rs_stage_t *stage, char **token)
{
	int i, numtextures;
	
	*token = strtok (NULL, TOK_DELIMINATORS);
	numtextures = atoi (*token);
	
	stage->num_blend_normalmaps = min (numtextures, 3);
	
	for (i = 0; i < numtextures; i++)
	{
		int idx;
		
		*token = strtok (NULL, TOK_DELIMINATORS);
		idx = atoi (*token);
		
		*token = strtok (NULL, TOK_DELIMINATORS);
		
		if (i < 3) // Current maximum
		{
			stage->normalblend_indices[i] = idx;
			strncpy (stage->normalblend_names[i], *token, sizeof (stage->normalblend_names[i]));
		}
		else
		{
			Com_Printf ("WARN: skip blendnormalmap channel %d (max 3)\n", i+1);
		}
	}
}

// For legacy origin and angle commands that aren't actually used in the code.
// Some old rscripts still have the origin command in them, so we should parse
// it anyway. Can't find any angle commands, but may as well handle those too.
static void rs_stage_consume3 (rs_stage_t *stage, char **token)
{
	Com_Printf ("WARN: deprecated Rscript command: %s\n", *token); 
	*token = strtok (NULL, TOK_DELIMINATORS);
	*token = strtok (NULL, TOK_DELIMINATORS);
	*token = strtok (NULL, TOK_DELIMINATORS);
}

// Used for the old "model" command, even though I can't find any rscripts 
// that have that command.
static void rs_stage_consume1 (rs_stage_t *stage, char **token)
{
	Com_Printf ("WARN: deprecated Rscript command: %s\n", *token); 
	*token = strtok (NULL, TOK_DELIMINATORS);
}

// Used for the old "dynamic" command, even though I can't find any rscripts
// that have that command.
static void rs_stage_consume0 (rs_stage_t *stage, char **token)
{
	Com_Printf ("WARN: deprecated Rscript command: %s\n", *token);
}

static struct 
{
	char *stage;
	void (*func)(rs_stage_t *shader, char **token);
} rs_stagekeys[] =
{
	{	"colormap",			&rs_stage_colormap		},
	{	"map",				&rs_stage_name			},
	{	"map2",				&rs_stage_name2			},
	{	"map3",				&rs_stage_name3			},
	{	"scroll",			&rs_stage_scroll		},
	{	"blendfunc",		&rs_stage_blendfunc		},
	{	"alphashift",		&rs_stage_alphashift	},
	{	"rand",				&rs_stage_random		},
	{	"anim",				&rs_stage_anim			},
	{	"envmap",			&rs_stage_envmap		},
	{	"depthhack",		&rs_stage_depthhack		},
	{	"nolightmap",		&rs_stage_nolightmap	},
	{	"alphamask",		&rs_stage_alphamask		},
	{	"rotate",			&rs_stage_rot_speed		},
	{	"scale",			&rs_stage_scale			},
	{	"lensflare",		&rs_stage_lensflare		},
	{	"flaretype",		&rs_stage_flaretype		},
	{	"normalmap",		&rs_stage_normalmap		},
	{	"blendmap",			&rs_stage_blendmap		},
	{	"blendnormalmap",	&rs_stage_blendnormalmap},
	{	"grass",			&rs_stage_grass			},
	{	"grasstype",		&rs_stage_grasstype		},
	{	"beam",				&rs_stage_beam			},
	{	"beamtype",			&rs_stage_beamtype		},
	{	"xang",				&rs_stage_xang			},
	{	"yang",				&rs_stage_yang			},
	{	"rotating",			&rs_stage_rotating		},
	{	"fx",				&rs_stage_fx			},
	{	"glow",				&rs_stage_glow			},
	{	"cube",				&rs_stage_cube			},
	{	"if",				&rs_stage_if			},
	
	// Deprecated stuff
	{	"model",			&rs_stage_consume1		},
	{	"frames",			&rs_stage_consume3		},
	{	"origin",			&rs_stage_consume3		},
	{	"angle",			&rs_stage_consume3		},
	{	"dynamic",			&rs_stage_consume0		},
	
	// Stuff we may bring back if anyone ever wants to use it
	{	"alphafunc",		&rs_stage_consume1		},

	{	NULL,				NULL					}
};

static int num_stagekeys = sizeof (rs_stagekeys) / sizeof(rs_stagekeys[0]) - 1;

// =====================================================

static void sanity_check_stage (rscript_t *rs, rs_stage_t *stage)
{
	int i;
	qboolean scroll_enabled, scale_enabled;
	
	scroll_enabled =	stage->scroll.typeX != 0 ||
						stage->scroll.speedX != 0 ||
						stage->scroll.typeY != 0 ||
						stage->scroll.speedY != 0;
	
	if (scroll_enabled && stage->envmap)
	{
		Com_Printf ("WARN: Incompatible combination: envmapping and scrolling"
					" in script %s!\nForcing envmap off.\n", rs->outname);
		stage->envmap = false;
	}
	
	scale_enabled = stage->scale.typeX != 0 || stage->scale.typeY != 0;
	
	if (scale_enabled && stage->envmap)
	{
		Com_Printf ("WARN: Incompatible combination: envmapping and"
					" non-static scaling in script %s!\n"
					"Forcing envmap off.\n", rs->outname);
		stage->envmap = false;
	}
	
	if (stage->rot_speed != 0.0 && stage->envmap)
	{
		Com_Printf ("WARN: Incompatible combination: envmapping and rotating"
					" in script %s!\nForcing envmap off.\n", rs->outname);
		stage->envmap = false;
	}
	
	for (i = 0; i < stage->num_blend_normalmaps; i++)
	{
		if (stage->normalblend_indices[i] >= stage->num_blend_textures)
		{
			Com_Printf ("WARN: Invalid blendnormalmap index %d"
						" (num blendmap channels %d)"
						" in script %s!\nForcing blendnormalmap off.\n",
						stage->normalblend_indices[i], 
						stage->num_blend_textures, rs->outname);
			stage->num_blend_normalmaps = 0;
		}
	}
	
	if (stage->envmap)
		rs->flags |= RS_CONTAINS_ENVMAP;
	
	if (stage->rot_speed != 0.0)
		rs->flags |= RS_CONTAINS_ROTATE;
	
	if (scroll_enabled)
		rs->flags |= RS_CONTAINS_SCROLL;
	
	if (scale_enabled)
		rs->flags |= RS_CONTAINS_SCALE;
	
	if (!(stage->lensflare || stage->grass || stage->beam || stage->cube))
		rs->flags |= RS_CONTAINS_DRAWN;
}

void RS_LoadScript (char *script)
{
	qboolean		inscript = false, instage = false;
	char			ignored = 0;
	char			*token, *fbuf;
	char			script_full_path[MAX_OSPATH];
	rscript_t		*rs = NULL;
	rs_stage_t		*stage = NULL;
	unsigned int	len, i;

	len = FS_LoadFile (script, (void **)&fbuf);

	if (!fbuf || len < 16)
	{
	//	Con_Printf (PRINT_ALL, "Could not load script %s\n", script);
		return;
	}
	
	FS_FullPath (script_full_path, sizeof(script_full_path), script);

	token = strtok (fbuf, TOK_DELIMINATORS);
	while (token != NULL)
	{
		if (!Q_strcasecmp (token, "/*") || !Q_strcasecmp (token, "["))
			ignored++;
		else if (!Q_strcasecmp (token, "*/") || !Q_strcasecmp (token, "]"))
			ignored--;

		if (!Q_strcasecmp (token, "//"))
		{
			//IGNORE
		}
		else if (!inscript && !ignored)
		{
			if (!Q_strcasecmp (token, "{"))
			{
				inscript = true;
			}
			else
			{
				rs = RS_FindScript (token);
				rs = RS_NewScript (token, rs);
#if defined WIN32_VARIANT
				sprintf_s (rs->outname, sizeof(rs->outname), "%s:%s", script_full_path, rs->name);
#else
				snprintf (rs->outname, sizeof(rs->outname), "%s:%s", script_full_path, rs->name);
#endif
			}
		}
		else if (inscript && !ignored)
		{
			if (!Q_strcasecmp(token, "}"))
			{
				if (instage)
				{
					sanity_check_stage (rs, stage);
					instage = false;
				}
				else
				{
					inscript = false;
				}
			}
			else if (!Q_strcasecmp(token, "{"))
			{
				if (!instage) {
					instage = true;
					stage = RS_NewStage(rs);
				}
			}
			else
			{
				if (instage && !ignored)
				{
					for (i = 0; i < num_stagekeys; i++) {
						if (!Q_strcasecmp (rs_stagekeys[i].stage, token))
						{
							rs_stagekeys[i].func (stage, &token);
							break;
						}
					}
				}
			}
		}

		token = strtok (NULL, TOK_DELIMINATORS);
	}

	FS_FreeFile(fbuf);
}

void RS_ScanPathForScripts (void)
{
	char	script[MAX_OSPATH];
	char	**script_list;
	const char *c;
	int	script_count, i;


	script_list = FS_ListFilesInFS("scripts/*.rscript", &script_count, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM);

	if(script_list) {
		for (i = 0; i < script_count; i++)
		{
			c = COM_SkipPath(script_list[i]);
			Com_sprintf(script, MAX_OSPATH, "scripts/%s", c);
			RS_LoadScript(script);
		}

		FS_FreeFileList(script_list, script_count);
	}
	
	script_count = 0;
	//TODO: gl_interactivescripts cvar
	script_list = FS_ListFilesInFS("scripts/interactive/*.rscript", &script_count, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM);

	if(script_list) {
		for (i = 0; i < script_count; i++)
		{
			c = COM_SkipPath(script_list[i]);
			Com_sprintf(script, MAX_OSPATH, "scripts/interactive/%s", c);
			RS_LoadScript(script);
		}

		FS_FreeFileList(script_list, script_count);
	}

	script_count = 0;
	script_list = FS_ListFilesInFS("scripts/normals/*.rscript", &script_count, 0, SFF_SUBDIR | SFF_HIDDEN | SFF_SYSTEM);

	if(script_list) {
		for (i = 0; i < script_count; i++)
		{
			c = COM_SkipPath(script_list[i]);
			Com_sprintf(script, MAX_OSPATH, "scripts/normals/%s", c);
			RS_LoadScript(script);
		}

		FS_FreeFileList(script_list, script_count);
	}
}

// scaling factor to convert from rotations per minute to radians per second
#define ROTFACTOR (M_PI * 2.0 / 60.0)

static float RS_ScrollFunc (char type, float speed)
{
	switch (type)
	{
	case 0: // static
		return rs_realtime*speed;
	case 1:	// sine
		return sin (rs_realtime * speed);
	case 2:	// cosine
		return cos (rs_realtime * speed);
	}

	return 0;
}

static float RS_ScaleFunc (char type, float scale)
{
	if (scale == 0)
		return 1;
	
	switch (type)
	{
	case 0: // static
		return scale;
	case 1: // sine
		return scale * sin (rs_realtime * 0.05);
	case 2: // cosine
		return scale * cos (rs_realtime * 0.05);
	}

	return 1;
}

static cvar_t *rs_eval_if_subexpr (rs_cond_val_t *expr)
{
	int resv;
	switch (expr->optype)
	{
		case rs_cond_none:
			return expr->val;
		case rs_cond_is:
			return rs_eval_if_subexpr (expr->subexpr1);
		case rs_cond_lnot:
			resv = (rs_eval_if_subexpr (expr->subexpr1)->value == 0);
			break;
		case rs_cond_eq:
			resv = Q_strcasecmp (
					rs_eval_if_subexpr (expr->subexpr1)->string,
					rs_eval_if_subexpr (expr->subexpr2)->string
				) == 0;
			break;
		case rs_cond_neq:
			resv = Q_strcasecmp (
					rs_eval_if_subexpr (expr->subexpr1)->string,
					rs_eval_if_subexpr (expr->subexpr2)->string
				) != 0;
			break;
		case rs_cond_gt:
			resv = (rs_eval_if_subexpr (expr->subexpr1)->value >
					rs_eval_if_subexpr (expr->subexpr2)->value
				);
			break;
		case rs_cond_ngt:
			resv = (rs_eval_if_subexpr (expr->subexpr1)->value <=
					rs_eval_if_subexpr (expr->subexpr2)->value
				);
			break;
		case rs_cond_lt:
			resv = (rs_eval_if_subexpr (expr->subexpr1)->value <
					rs_eval_if_subexpr (expr->subexpr2)->value
				);
			break;
		case rs_cond_nlt:
			resv = (rs_eval_if_subexpr (expr->subexpr1)->value >=
					rs_eval_if_subexpr (expr->subexpr2)->value
				);
			break;
		case rs_cond_and:
			resv = (rs_eval_if_subexpr (expr->subexpr1)->value &&
					rs_eval_if_subexpr (expr->subexpr2)->value
				);
			break;
		case rs_cond_or:
			resv = (rs_eval_if_subexpr (expr->subexpr1)->value ||
					rs_eval_if_subexpr (expr->subexpr2)->value
				);
			break;
		default:
			Com_Printf ("Unknown optype %d! (Can't happen)\n", expr->optype);
			resv = 0;
			break;
	}
	Anon_Cvar_Set (&(expr->lval), va ("%d", resv));
	return &(expr->lval);
}

static int rs_dlights_enabled = -1;
static qboolean rs_shadows_enabled = -1;
// uniforms that may change from stage to stage
#define RS_STAGE_UNIFORMS X(lightmap) X(envmap) X(numblendtextures) X(numblendnormalmaps)
static struct 
{
#define X(name) int name;
	RS_STAGE_UNIFORMS
#undef X
} rs_stage_uniform_vals;
static void RS_SetupGLState (const entity_t *ent, int dynamic, qboolean shadows)
{
	int i;
	
	glUseProgramObjectARB (g_rscriptprogramObj[dynamic]);
	
	glUniform1iARB (rscript_uniforms[dynamic].mainTexture, 0);
	glUniform1iARB (rscript_uniforms[dynamic].lightmapTexture, 1);
	glUniform1iARB (rscript_uniforms[dynamic].mainTexture2, 2);
	for (i = 0; i < 6; i++)
		glUniform1iARB (rscript_uniforms[dynamic].blendTexture[i], 3+i);
	for (i = 0; i < 3; i++)
		glUniform1iARB (rscript_uniforms[dynamic].blendNormalmap[i], 9+i);
	glUniform1iARB (rscript_uniforms[dynamic].fog, map_fog);
	
	glUniform1iARB (rscript_uniforms[dynamic].static_normalmaps, r_worldnormalmaps->integer);
	
	if (dynamic != 0)
		R_SetDlightUniforms (&rscript_uniforms[dynamic].dlight_uniforms);
	
	glUniform3fARB (rscript_uniforms[dynamic].staticLightPosition, r_worldLightVec[0], r_worldLightVec[1], r_worldLightVec[2]);

	R_SetShadowmapUniforms (&rscript_uniforms[dynamic].shadowmap_uniforms, 12, shadows);

	if (r_sunShadowsOn && shadows)
	{
		float rotationMatrix[3][3];

		// because we are translating our entities, we need to supply the shader with the actual position of this mesh
		glUniform3fvARB (rscript_uniforms[dynamic].meshPosition, 1, (const GLfloat *)ent->origin);

		AnglesToMatrix3x3 (ent->angles, rotationMatrix);
		glUniformMatrix3fvARB (rscript_uniforms[dynamic].meshRotation, 1, GL_TRUE, (const GLfloat *) rotationMatrix);
	}

	qglMatrixMode (GL_TEXTURE);
	
	rs_dlights_enabled = dynamic;
	rs_shadows_enabled = shadows;
	
	memset (&rs_stage_uniform_vals, -1, sizeof (rs_stage_uniform_vals));
}

static void RS_SetStageUniforms (int dynamic, int lightmap, int envmap, int numblendtextures, int numblendnormalmaps)
{
#define X(name) \
	if (rs_stage_uniform_vals.name != name) \
	{ \
		rs_stage_uniform_vals.name = name; \
		glUniform1iARB (rscript_uniforms[dynamic].name, name); \
	}

	RS_STAGE_UNIFORMS

#undef X
}

static void RS_CleanupGLState (void)
{
	qglMatrixMode (GL_MODELVIEW);
	
	qglColor4f(1,1,1,1);
	
	glUseProgramObjectARB (0);
	
	// restore the original blend mode
	GL_BlendFunction (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	GLSTATE_DISABLE_BLEND
	GLSTATE_DISABLE_ALPHATEST
}

qboolean rs_in_group = false;

// If you're about to draw a huge number of RScript surfaces in a row with
// *no other types of surfaces* being drawn, you can surround them with 
// RS_Begin_Group and RS_End_Group for some extra performance.

void RS_Begin_Group (const entity_t *ent)
{
	rs_in_group = true;
	RS_SetupGLState (ent, CUR_NUM_DLIGHTS, true);
}

void RS_End_Group (void)
{
	rs_in_group = false;
	RS_CleanupGLState ();
}

void RS_Draw (	rscript_t *rs, const entity_t *ent, int lmtex, vec2_t rotate_center,
				vec3_t normal, qboolean translucent, rs_lightmaptype_t lm,
				qboolean enable_dlights, qboolean enable_shadows,
				void (*draw_callback) (void))
{
	vec3_t		vectors[3];
	rs_stage_t	*stage;
	int			i;
	int			dynamic;
	
	if (!rs)
		return;

	stage = rs->stage;

	//for envmap by normals
	AngleVectors (r_newrefdef.viewangles, vectors[0], vectors[1], vectors[2]);
	
	dynamic = enable_dlights ? CUR_NUM_DLIGHTS : 0;

	if (!rs_in_group || rs_dlights_enabled != dynamic || rs_shadows_enabled != enable_shadows)
		RS_SetupGLState (ent, dynamic, enable_shadows);
	
	do
	{
		float red = 1.0, green = 1.0, blue = 1.0, alpha = 1.0;
		
		if (stage->lensflare || stage->grass || stage->beam || stage->cube)
			continue; //handled elsewhere
		if (stage->condv && !(rs_eval_if_subexpr(stage->condv)->value))
			continue; //stage should not execute
		
		if (stage->lightmap && lm != rs_lightmap_off)
			GL_MBind (1, lmtex);
		
	 	if (stage->num_blend_textures > 3)
	 		GL_MBind (2, stage->texture2->texnum);
		
		if (stage->anim_count)
			GL_MBind (0, RS_Animate(stage));
		else
	 		GL_MBind (0, stage->texture->texnum);
	 	
	 	GL_SelectTexture (0);
		qglPushMatrix ();
		
		if (stage->blendfunc.blend)
		{
			// FIXME: hack!
			if (stage->blendfunc.source != GL_ONE || stage->blendfunc.dest != GL_ONE_MINUS_SRC_ALPHA)
				GL_BlendFunction (stage->blendfunc.source, stage->blendfunc.dest);
			GLSTATE_ENABLE_BLEND
		}
		else if (translucent && !stage->alphamask)
		{
			GL_BlendFunction(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			GLSTATE_ENABLE_BLEND
		}
		else
		{
			GLSTATE_DISABLE_BLEND
		}

		if (stage->alphashift.min || stage->alphashift.speed)
		{
			if (!stage->alphashift.speed && stage->alphashift.min > 0)
			{
				alpha = stage->alphashift.min;
			}
			else if (stage->alphashift.speed)
			{
				alpha = sin (rs_realtime * stage->alphashift.speed);
				alpha = (alpha + 1)*0.5f;
				if (alpha > stage->alphashift.max)
					alpha = stage->alphashift.max;
				if (alpha < stage->alphashift.min)
					alpha = stage->alphashift.min;
			}
		}
		
		if (stage->rot_speed)
		{
			qglTranslatef (rotate_center[0], rotate_center[1], 0);
			qglRotatef (RAD2DEG (-stage->rot_speed * rs_realtime * ROTFACTOR), 0, 0, 1);
			qglTranslatef (-rotate_center[0], -rotate_center[1], 0);
		}
		
		qglTranslatef (	RS_ScrollFunc (stage->scroll.typeX, stage->scroll.speedX),
						RS_ScrollFunc (stage->scroll.typeY, stage->scroll.speedY),
						0 );
		
		qglScalef (	RS_ScaleFunc (stage->scale.typeX, stage->scale.scaleX),
					RS_ScaleFunc (stage->scale.typeY, stage->scale.scaleY),
					1 );
		
		if (stage->envmap)
		{
			//move by normal & position
			qglTranslatef (	-DotProduct (normal, vectors[1]) - (r_origin[0]-r_origin[1]+r_origin[2])*0.0025,
							DotProduct (normal, vectors[2]) - (r_origin[0]-r_origin[1]+r_origin[2])*0.0025,
							0 );
		}
		
		if (stage->num_blend_textures > 0)
		{
			for (i = 0; i < stage->num_blend_textures; i++)
				GL_MBind (3+i, stage->blend_textures[i]->texnum);
			
			glUniform2fvARB (rscript_uniforms[dynamic].blendscales, stage->num_blend_textures, (const GLfloat *) stage->blend_scales);
		}
		
		if (stage->num_blend_normalmaps > 0)
		{
			for (i = 0; i < stage->num_blend_normalmaps; i++)
				GL_MBind (9+i, stage->blend_normalmaps[i]->texnum);
			
			glUniform1ivARB (rscript_uniforms[dynamic].normalblendindices, stage->num_blend_normalmaps, (const GLint *) stage->normalblend_indices);
		}
		
		GL_SelectTexture (0);
		
		if (stage->colormap.enabled)
		{
			red = stage->colormap.red;
			green = stage->colormap.green;
			blue = stage->colormap.blue;
		}
		
		qglColor4f (red, green, blue, alpha);
		
		if (stage->alphamask)
		{
			GLSTATE_ENABLE_ALPHATEST
		}
		else
		{
			GLSTATE_DISABLE_ALPHATEST
		}
		
		RS_SetStageUniforms (dynamic, stage->lightmap?lm:0, stage->envmap != 0, stage->num_blend_textures, stage->num_blend_normalmaps);
		
		draw_callback ();
					
		qglPopMatrix ();

	} while ( (stage = stage->next) );	
	
	if (!rs_in_group)
		RS_CleanupGLState ();
}

void RS_DrawSurface (msurface_t *surf, rscript_t *rs)
{
	int			lmtex = gl_state.lightmap_textures + surf->lightmaptexturenum;
	vec2_t		rotate_center;
	qboolean	translucent;
	
	if ((rs->flags & RS_CONTAINS_DRAWN) == 0)
		return;
	
	BSP_AddSurfToVBOAccum (surf);
	
	rotate_center[0] = surf->c_s;
	rotate_center[1] = surf->c_t;
	
	translucent = (surf->texinfo->flags & (SURF_TRANS33 | SURF_TRANS66)) != 0;
	
	RS_Draw (	rs, currententity, lmtex, rotate_center, surf->plane->normal,
				translucent,
				translucent?rs_lightmap_off:rs_lightmap_separate_texcoords,
				true, !translucent, BSP_DrawVBOAccum );
	
	BSP_ClearVBOAccum ();
}

rscript_t *rs_caustics;

void RS_LoadSpecialScripts (void) //the special case of water caustics
{
	rs_caustics = RS_FindScript("caustics");
	if(rs_caustics)
		RS_ReadyScript(rs_caustics);
	assert ((rs_caustics->flags & RS_PREVENT_BATCH) == 0);
}
