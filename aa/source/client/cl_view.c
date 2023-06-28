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
// cl_view.c -- player rendering positioning

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "client.h"

//=============

int r_numgrasses;
grass_t r_grasses[MAX_GRASSES];

int r_numbeams;
beam_t	r_beams[MAX_BEAMS];

extern cvar_t *cl_showPlayerNames;
extern cvar_t *name;
extern char map_music[260];
extern cvar_t *background_music;
extern qboolean IsVisible(vec3_t org1,vec3_t org2);
extern void R_TransformVectorToScreen( refdef_t *rd, vec3_t in, vec2_t out );

cvar_t		*crosshair;
cvar_t		*cl_testparticles;
cvar_t		*cl_testentities;
cvar_t		*cl_testlights;
cvar_t		*cl_testblend;

cvar_t		*cl_stats;

int			r_numdlights;
dlight_t	r_dlights[MAX_DLIGHTS];

int			r_numentities;
entity_t	r_entities[MAX_ENTITIES];

int			r_numviewentities;
entity_t	r_viewentities[MAX_ENTITIES];

int			r_numparticles;
particle_t	*r_particles[MAX_PARTICLES];

lightstyle_t	r_lightstyles[MAX_LIGHTSTYLES];

char cl_weaponmodels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
int num_cl_weaponmodels;

/*
====================
V_ClearScene

Specifies the model that will be used as the world
====================
*/
void V_ClearScene (void)
{
	r_numdlights = 0;
	r_numentities = 0;
	r_numviewentities = 0;
	r_numparticles = 0;
}


/*
=====================
V_AddEntity

=====================
*/
void V_AddEntity (entity_t *ent)
{
	if (r_numentities >= MAX_ENTITIES)
		return;
	r_entities[r_numentities++] = *ent;
}

/*
=====================
V_AddViewEntity

=====================
*/
void V_AddViewEntity (entity_t *ent)
{
	if (r_numviewentities >= MAX_ENTITIES)
		return;
	r_viewentities[r_numviewentities++] = *ent;
}


/*
=====================
V_AddParticle

=====================
*/
void V_AddParticle (particle_t	*p)
{

	if (r_numparticles >= MAX_PARTICLES)
		return;
	r_particles[r_numparticles++] = p;
}

/*
=====================
V_AddLight

=====================
*/
extern	cvar_t	*gl_dynamic;
void V_AddLight (vec3_t org, float intensity, float r, float g, float b)
{
	dlight_t	*dl;

	if (gl_dynamic->integer == 0)
		return;
	if (r_numdlights >= MAX_DLIGHTS)
		return;
	dl = &r_dlights[r_numdlights++];
	VectorCopy (org, dl->origin);
	dl->intensity = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
}

/*
=====================
V_AddLightStyle

=====================
*/
void V_AddLightStyle (int style, float r, float g, float b)
{
	lightstyle_t	*ls;

	if (style < 0 || style > MAX_LIGHTSTYLES)
		Com_Error (ERR_DROP, "Bad light style %i", style);
	ls = &r_lightstyles[style];

	ls->white = r+g+b;
	ls->rgb[0] = r;
	ls->rgb[1] = g;
	ls->rgb[2] = b;
}

/*
================
V_TestParticles

If cl_testparticles is set, create 4096 particles in the view
================
*/
void CL_LogoutEffect (vec3_t org, int type);
void V_TestParticles (void)
{
    vec3_t  org;
    
    VectorMA (cl.refdef.vieworg, 128, cl.v_forward, org);
    CL_LogoutEffect (org, MZ_LOGOUT);
}

/*
================
V_TestEntities

If cl_testentities is set, create 32 player models
================
*/
extern struct model_s *Mod_ForName (char *name, qboolean crash);
void V_TestEntities (void)
{
	int			i, j;
	float		f, r;
	entity_t	*ent;
	struct model_s		*mod;
	struct image_s		*skin;

	memset (r_entities, 0, sizeof(r_entities));
	
	if (cl_testentities->integer)
	{
		mod = cl.baseclientinfo.model;
		skin = cl.baseclientinfo.skin;
		r_numentities = cl_testentities->integer == 1 ? 32 : cl_testentities->integer;
	}
	else
	{
		mod = Mod_ForName (cl_testentities->string, false);
		skin = NULL;
		r_numentities = 1;
	}

	for (i=0 ; i<r_numentities ; i++)
	{
		ent = &r_entities[i];

		if (cl_testentities->integer)
		{
			r = 64 * ( (i%4) - 1.5 );
			f = 64 * (i/4) + 128;
		}
		else
		{
			r = 0;
			f = 128;
		}

		for (j=0 ; j<3 ; j++)
			ent->origin[j] = cl.refdef.vieworg[j] + cl.v_forward[j]*f +
			cl.v_right[j]*r;

		ent->model = mod;
		ent->skin = skin;
	}
}

/*
================
V_TestLights

If cl_testlights is set, create 32 lights models
================
*/
void V_TestLights (void)
{
	int			i, j;
	float		f, r;
	dlight_t	*dl;

	r_numdlights = 32;
	memset (r_dlights, 0, sizeof(r_dlights));

	for (i=0 ; i<r_numdlights ; i++)
	{
		dl = &r_dlights[i];

		r = 64 * ( (i%4) - 1.5 );
		f = 64 * (i/4) + 128;

		for (j=0 ; j<3 ; j++)
			dl->origin[j] = cl.refdef.vieworg[j] + cl.v_forward[j]*f +
			cl.v_right[j]*r;
		dl->color[0] = ((i%6)+1) & 1;
		dl->color[1] = (((i%6)+1) & 2)>>1;
		dl->color[2] = (((i%6)+1) & 4)>>2;
		dl->intensity = 200;
	}
}

//===================================================================

/*
=================
CL_PrepRefresh

Call before entering a new level, or after changing dlls
=================
*/
qboolean needLoadingPlaque (void);
extern int	seconds, minutes;
void CL_PrepRefresh ( void )
{
	char		mapname[32];
	int			i, max;
	char		name[MAX_QPATH];
	float		rotate;
	vec3_t		axis;
	qboolean	newPlaque = needLoadingPlaque();

	loadingPercent = 0;
	rocketlauncher = 0;
	rocketlauncher_drawn = 0;
	smartgun = 0;
	smartgun_drawn = 0;
	disruptor = 0;
	disruptor_drawn = 0;
	flamethrower = 0;
	flamethrower_drawn = 0;
	beamgun = 0;
	beamgun_drawn = 0;
	chaingun = 0;
	chaingun_drawn = 0;
	vaporizer = 0;
	vaporizer_drawn = 0;
	quad = 0;
	quad_drawn = 0;
	haste = 0;
	haste_drawn = 0;
	sproing = 0;
	sproing_drawn = 0;
	inv = 0;
	inv_drawn = 0;
	adren = 0;
	adren_drawn = 0;

	numitemicons = 0;

	//reset clock
	seconds = minutes = 0;

	if (!cl.configstrings[CS_MODELS+1][0])
		return;		// no map loaded

	if (newPlaque)
		SCR_BeginLoadingPlaque();

	loadingMessage = true;
	memset( loadingMessages , 0 , sizeof( loadingMessages ) );
	Com_sprintf (loadingMessages[0][0], CL_LOADMSG_LENGTH, "Loading map...");
	Com_sprintf (loadingMessages[1][0], CL_LOADMSG_LENGTH, "Loading models...");
	Com_sprintf (loadingMessages[2][0], CL_LOADMSG_LENGTH, "Loading pics...");
	Com_sprintf (loadingMessages[3][0], CL_LOADMSG_LENGTH, "Loading clients...");

	// let the render dll load the map
	strcpy (mapname, cl.configstrings[CS_MODELS+1] + 5);	// skip "maps/"
	mapname[strlen(mapname)-4] = 0;		// cut off ".bsp"

	// register models, pics, and skins

	//this was moved here, to prevent having a pic loaded over and over again, which was
	//totally killing performance after a dozen or so maps.
	// 2010-10 modified on suspicion of affecting possible compiler bug. see R_RegisterPic()
	map_pic_loaded = false;
	if ( R_RegisterPic( va("/levelshots/%s.pcx", mapname)) != NULL )
	{
		map_pic_loaded = true;
	}

	Com_Printf ("Map: %s\r", mapname);
	Com_sprintf( loadingMessages[0][1] , CL_LOADMSG_LENGTH , "^3in progress" );
	SCR_UpdateScreen ();
	R_BeginRegistration (mapname);
	Com_Printf ("                                     \r");
	Com_sprintf (loadingMessages[0][1], CL_LOADMSG_LENGTH, "^2done");
	loadingPercent += 20;

	// precache status bar pics
	Com_Printf ("pics\r");
	SCR_UpdateScreen ();
	SCR_TouchPics ();
	Com_Printf ("                                     \r");

	num_cl_weaponmodels = 1;
	strcpy(cl_weaponmodels[0], "weapon.iqm");

	for (i=1, max=0 ; i<MAX_MODELS && cl.configstrings[CS_MODELS+i][0] ; i++)
		max++;

	for (i=1 ; i<MAX_MODELS && cl.configstrings[CS_MODELS+i][0] ; i++)
	{
		strcpy (name, cl.configstrings[CS_MODELS+i]);
		// name[37] = 0; 2010-10 archaic line length truncates log lines

		if (name[0] != '*')
		{
			Com_Printf ("%s\r", name);

			//only make max of 20 chars long
			Com_sprintf( loadingMessages[1][1] , CL_LOADMSG_LENGTH , "^1%s" , name );

			//check for types
			if(!strcmp(name, "models/weapons/g_rocket/tris.iqm"))
				rocketlauncher = 1;
			if(!strcmp(name, "models/weapons/g_chaingun/tris.iqm"))
				chaingun = 1;
			if(!strcmp(name, "models/weapons/g_smartgun/tris.iqm"))
				smartgun = 1;
			if(!strcmp(name, "models/weapons/g_beamgun/tris.iqm"))
				beamgun = 1;
			if(!strcmp(name, "models/weapons/g_disruptor/tris.iqm"))
				disruptor = 1;
			if(!strcmp(name, "models/weapons/g_flamethrower/tris.iqm"))
				flamethrower = 1;
			if(!strcmp(name, "models/weapons/g_vaporizer/tris.iqm"))
				vaporizer = 1;
			if(!strcmp(name, "models/items/quaddama/tris.iqm"))
				quad = 1;
			if(!strcmp(name, "models/items/haste/tris.iqm"))
				haste = 1;
			if(!strcmp(name, "models/items/sproing/tris.iqm"))
				sproing = 1;
			if(!strcmp(name, "models/items/adrenaline/tris.iqm"))
				adren = 1;
			if(!strcmp(name, "models/items/invulner/tris.iqm"))
				inv = 1;
		}

		SCR_UpdateScreen ();

		Sys_SendKeyEvents ();	// pump message loop
		if (name[0] == '#')
		{
			// special player weapon model
			if (num_cl_weaponmodels < MAX_CLIENTWEAPONMODELS)
			{
				strncpy(cl_weaponmodels[num_cl_weaponmodels], cl.configstrings[CS_MODELS+i]+1,
					sizeof(cl_weaponmodels[num_cl_weaponmodels]) - 1);
				num_cl_weaponmodels++;

			}
		}
		else
		{
			cl.model_draw[i] = R_RegisterModel (cl.configstrings[CS_MODELS+i]);
			if (name[0] == '*')
				cl.model_clip[i] = CM_InlineModel (cl.configstrings[CS_MODELS+i]);
			else
				cl.model_clip[i] = NULL;
		}
		if (name[0] != '*')
			Com_Printf ("                                     \r");

		loadingPercent += 60.0f/(float)max;
	}

	Com_sprintf (loadingMessages[1][1], CL_LOADMSG_LENGTH, "^3precaching");

	SCR_UpdateScreen ();

	R_RegisterBasePlayerModels();
	if(cl_precachecustom->value)
		R_RegisterCustomPlayerModels();

	Com_sprintf (loadingMessages[1][1], CL_LOADMSG_LENGTH, "^2done");

	Com_Printf ("images\r", i);
	SCR_UpdateScreen ();

	for (i=1, max=0 ; i<MAX_IMAGES && cl.configstrings[CS_IMAGES+i][0] ; i++)
		max++;

	for (i=1 ; i<MAX_IMAGES && cl.configstrings[CS_IMAGES+i][0] ; i++)
	{
		cl.image_precache[i] = R_RegisterPic (cl.configstrings[CS_IMAGES+i]);
		Sys_SendKeyEvents ();	// pump message loop
		loadingPercent += 10.0f/(float)max;
	}
	Com_sprintf (loadingMessages[2][1], CL_LOADMSG_LENGTH, "^2done");

	Com_Printf ("                                     \r");

	//refresh the player model/skin info
	CL_LoadClientinfo (&cl.baseclientinfo, va("unnamed\\%s\\%s", DEFAULTMODEL, DEFAULTSKIN));

	for (i=1, max=0 ; i<MAX_CLIENTS ; i++)
		if (cl.configstrings[CS_PLAYERSKINS+i][0])
			max++;

	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		if (!cl.configstrings[CS_PLAYERSKINS+i][0])
			continue;

		Com_sprintf (loadingMessages[3][1], CL_LOADMSG_LENGTH, "^3%i" , i);
		Com_Printf ("client %i\r", i);
		SCR_UpdateScreen ();
		Sys_SendKeyEvents ();	// pump message loop

		CL_ParseClientinfo (i);
		Com_Printf ("                                     \r");
		loadingPercent += 10.0f/(float)max;
	}
	Com_sprintf (loadingMessages[3][1], CL_LOADMSG_LENGTH, "^2done");

	//hack hack hack - psychospaz
	loadingPercent = 100;

	// set sky textures and speed
	Com_Printf ("sky\r", i);
	SCR_UpdateScreen ();
	rotate = atof (cl.configstrings[CS_SKYROTATE]);
	sscanf(	cl.configstrings[CS_SKYAXIS], "%f %f %f",
		&axis[0], &axis[1], &axis[2]);
	R_SetSky( cl.configstrings[CS_SKY], rotate, axis );
	Com_Printf ("                                     \r");

	// the renderer can now free unneeded stuff
	R_EndRegistration ();

	// clear any lines of console text
	CON_ClearNotify( );

	SCR_UpdateScreen ();
	cl.refresh_prepped = true;
	cl.force_refdef = true;	// make sure we have a valid refdef

	// start background music
	background_music = Cvar_Get ("background_music", "1", CVAR_ARCHIVE);
	S_StartMapMusic();

	//loadingMessage = false;
	rocketlauncher = 0;
	rocketlauncher_drawn = 0;
	smartgun = 0;
	smartgun_drawn = 0;
	disruptor = 0;
	disruptor_drawn = 0;
	flamethrower = 0;
	flamethrower_drawn = 0;
	beamgun = 0;
	beamgun_drawn = 0;
	chaingun = 0;
	chaingun_drawn = 0;
	vaporizer = 0;
	vaporizer_drawn = 0;
	quad = 0;
	quad_drawn = 0;
	haste = 0;
	haste_drawn = 0;
	sproing = 0;
	sproing_drawn = 0;
	inv = 0;
	inv_drawn = 0;
	adren = 0;
	adren_drawn = 0;
	numitemicons = 0;

	if (newPlaque)
		SCR_EndLoadingPlaque();
	else
		Cvar_Set ("paused", "0");
}

/*
====================
CalcFov
====================
*/
float CalcFov (float fov_x, float width, float height)
{
	float	a;
	float	x;

	if (fov_x < 1 || fov_x > 179)
		Com_Error (ERR_DROP, "Bad fov: %f", fov_x);

	x = width/tanf(fov_x/360*M_PI);
    a = atan (height/x);
    a = a*360/M_PI;

	return a;
}

/*
=================
SCR_DrawCrosshair
=================
*/
extern cvar_t *hand;
void SCR_DrawCrosshair (refdef_t *fd)
{
	int x, y, crosshairposition;
	
	if (!strcmp(crosshair->string, "none"))
		return;

	if (crosshair->modified)
	{
		crosshair->modified = false;
		SCR_TouchPics ();
	}

	if (!crosshair_pic[0])
		return;

	x = fd->x + ((fd->width - crosshair_width)>>1);
	y = fd->y + ((fd->height - crosshair_height)>>1);
	
	// get rid of the old crosshair adjustment built into the texture
	x -= 4;
	y -= 4;
	
	// add a new crosshair adjustment offset
	crosshairposition = cl.frame.playerstate.stats[STAT_FLAGS] & STAT_FLAGS_CROSSHAIRPOSITION;
	if (crosshairposition != STAT_FLAGS_CROSSHAIRCENTER)
	{
		double x_offs, y_offs;
		switch (crosshairposition)
		{
			default: // other crosshair positions reserved for future use
			case STAT_FLAGS_CROSSHAIRPOS1: // use the original default
				x_offs = y_offs = 4.0;
				break;
			case STAT_FLAGS_CROSSHAIRPOS2:
				x_offs = 2.0;
				y_offs = 3.0;
				break;
		}
		y_offs *= (double)fd->height/480.0;
		x_offs *= (double)fd->width/640.0;
		if (y_offs-(int)y_offs >= 0.5)
			y_offs = (int)y_offs + 1;
		if (x_offs-(int)x_offs >= 0.5)
			x_offs = (int)x_offs + 1;
		if (hand->integer == 1)
			x_offs = -x_offs;
		else if (hand->integer == 2)
			x_offs = 0;
		x += x_offs;
		y += y_offs;
	}
	
	Draw_Pic (x, y, crosshair_pic);
}

qboolean InFront (vec3_t target)
{
	vec3_t	vec;
	float	dot;
	vec3_t	forward;

	AngleVectors (cl.refdef.viewangles, forward, NULL, NULL);
	VectorSubtract (target, cl.refdef.vieworg, vec);
	VectorNormalize (vec);
	dot = DotProduct (vec, forward);

	if (dot > 0.3)
		return true;
	return false;
}

static void SCR_DrawPlayerNames (void)
{
	static vec3_t		mins = { -8, -8, -8 };
	static vec3_t		maxs = { 8, 8, 8 };
	FNT_font_t		font;
	struct FNT_window_s	box;
	int			i;
	centity_t *		cent;
	float			dist, mindist;
	trace_t			trace;
	vec2_t			screen_pos;
	vec3_t			vecdist;
	int			closest;
	
	font = FNT_AutoGet (CL_gameFont);

	mindist = 1000;
	closest = 0;

	for (i = 0; i < MAX_CLIENTS; i++)
	{
		cent = cl_entities + i + 1;

		if (!cent->current.modelindex)
			continue;
		
		if (strlen (cl.clientinfo[i].name) <= 1)
			continue;

		if (!strcmp (cl.clientinfo[i].name, name->string))
			continue;
		
		VectorSubtract (cent->current.origin, cl.refdef.vieworg, vecdist);
		dist = VectorLength (vecdist);
		
		if (dist >= mindist)
			continue;
		
		if (!InFront (cent->current.origin))
			continue;

		// FIXME: CL_Trace is hot-spot here!
		trace = CL_Trace ( cl.refdef.vieworg, mins, maxs, cent->current.origin, -1, MASK_PLAYERSOLID, true, NULL);
		if (trace.fraction != 1.0)
			continue;

		if (cl_showPlayerNames->integer == 2)
		{
		    // Draw a label over each visible player
			R_TransformVectorToScreen(&cl.refdef, cent->current.origin, screen_pos);
			box.x = (int)screen_pos[0];
			box.y = cl.refdef.height-(int)screen_pos[1]-cl.refdef.height/6;
			box.width = box.height = 0;
			FNT_BoundedPrint (font, cl.clientinfo[i].name, FNT_CMODE_QUAKE_SRS,
				FNT_ALIGN_LEFT, &box, FNT_colors[2]);
		}
		else
		{
		    mindist = dist;
			closest = i;
		}
	}

	if (closest == 0)
		return;

	// Draw a label of the closest visible player in the center of the screen
	box.x = (int)( ( cl.refdef.width - 200 ) / 2 );
	box.y = (int)( cl.refdef.height / 1.8 );
	box.width = 400;
	box.height = 0;
	FNT_BoundedPrint (font, cl.clientinfo[closest].name, FNT_CMODE_QUAKE_SRS,
		FNT_ALIGN_CENTER, &box, FNT_colors[2]);

}

void SCR_DrawBases (void)
{
	FNT_font_t	font;
	int		i;
	entity_t *	ent;
	vec2_t		screen_pos;

	font = FNT_AutoGet( CL_gameFont );
	for (i=0 ; i<cl.refdef.num_entities; i++)
	{
		struct FNT_window_s	box;
		const char *		str;

		ent = &r_entities[i];
		if(!ent->flag)
			continue;
		if (!InFront(ent->origin))
			continue;

		R_TransformVectorToScreen(&cl.refdef, ent->origin, screen_pos);
		box.x = (int)screen_pos[0];
		box.y = cl.refdef.height-(int)screen_pos[1]-cl.refdef.height/6;
		box.width = box.height = 0;

		if(ent->flag == 2)
			str = "^4Blue Flag";
		else if(ent->flag == 1)
			str = "^1Red Flag";
		else
			str = "Flag";
		FNT_BoundedPrint( font , str , FNT_CMODE_QUAKE_SRS , FNT_ALIGN_LEFT , &box , FNT_colors[ 2 ] );
	}
}

/*
==================
V_RenderView

==================
*/
extern cvar_t		*scr_netgraph;
extern cvar_t		*scr_timegraph;
extern cvar_t		*scr_debuggraph;
extern cvar_t		*scr_graphheight;
void V_RenderView( float stereo_separation )
{
	extern int entitycmpfnc( const entity_t *, const entity_t * );

	if (cls.state != ca_active)
		return;

	if (!cl.refresh_prepped)
		return;			// still loading

	if ( cl_timedemo && cl_timedemo->integer )
	{
		if ( cl.timedemo_start > 0 )
		{ /* frame counter increment for timedemo benchmark */
			cl.timedemo_frames++;
		}
		else
		{ /* time demo start trigger */
			cl.timedemo_start = Sys_Milliseconds ();
			cl.timedemo_frames = 0;
		}
	}

	// an invalid frame will just use the exact previous refdef
	// we can't use the old frame if the video mode has changed, though...
	if ( cl.frame.valid && (cl.force_refdef || !cl_paused->value) )
	{
		cl.force_refdef = false;

		// build a refresh entity list and calc cl.sim*
		// this also calls CL_CalcViewValues which loads
		// v_forward, etc.
		CL_AddEntities ();

		if (cl_testparticles->value)
			V_TestParticles ();
		if (strcmp (cl_testentities->string, "0"))
			V_TestEntities ();
		if (cl_testlights->value)
			V_TestLights ();
		if (cl_testblend->value)
		{
			cl.refdef.blend[0] = 1;
			cl.refdef.blend[1] = 0.5;
			cl.refdef.blend[2] = 0.25;
			cl.refdef.blend[3] = 0.5;
		}

		// offset vieworg appropriately if we're doing stereo separation
		if ( stereo_separation != 0 )
		{
			vec3_t tmp;

			VectorScale( cl.v_right, stereo_separation, tmp );
			VectorAdd( cl.refdef.vieworg, tmp, cl.refdef.vieworg );
		}

		// never let it sit exactly on a node line, because a water plane can
		// dissapear when viewed with the eye exactly on it.
		// the server protocol only specifies to 1/8 pixel, so add 1/16 in each axis
		cl.refdef.vieworg[0] += 1.0/16;
		cl.refdef.vieworg[1] += 1.0/16;
		cl.refdef.vieworg[2] += 1.0/16;

		cl.refdef.x = scr_vrect.x;
		cl.refdef.y = scr_vrect.y;
		cl.refdef.width = scr_vrect.width;
		cl.refdef.height = scr_vrect.height;
		
		if (scr_debuggraph->integer || scr_timegraph->integer || scr_netgraph->integer)
			cl.refdef.height -= scr_graphheight->integer;
		
		cl.refdef.fov_y = CalcFov (cl.refdef.fov_x, cl.refdef.width, cl.refdef.height);
		cl.refdef.time = cl.time*0.001;

		cl.refdef.areabits = cl.frame.areabits;

		if (!cl_add_entities->value) {
			r_numentities = 0;
			r_numviewentities = 0;
		}
		if (!cl_add_particles->value)
			r_numparticles = 0;
		if (!cl_add_lights->value)
			r_numdlights = 0;
		if (!cl_add_blend->value)
		{
			Vector4Clear (cl.refdef.blend);
		}

		cl.refdef.num_entities = r_numentities;
		cl.refdef.entities = r_entities;
		cl.refdef.num_viewentities = r_numviewentities;
		cl.refdef.viewentities = r_viewentities;
		cl.refdef.num_particles = r_numparticles;
		cl.refdef.particles = r_particles;
		cl.refdef.num_dlights = r_numdlights;
		cl.refdef.dlights = r_dlights;
		cl.refdef.lightstyles = r_lightstyles;
		cl.refdef.num_grasses = r_numgrasses;
		cl.refdef.grasses = r_grasses;
		cl.refdef.num_beams = r_numbeams;
		cl.refdef.beams = r_beams;

		cl.refdef.rdflags = cl.frame.playerstate.rdflags;

		// sort entities for better cache locality
        qsort( cl.refdef.entities, cl.refdef.num_entities, sizeof( cl.refdef.entities[0] ), (int (*)(const void *, const void *))entitycmpfnc );

		V_ClearScene ();
	}

	cl.refdef.rdflags |= RDF_BLOOM;   //BLOOMS

	R_RenderFrame (&cl.refdef);
	if (cl_stats->value)
		Com_Printf ("ent:%i  lt:%i  part:%i\n", r_numentities, r_numdlights, r_numparticles);
	if ( log_stats->value && ( log_stats_file != 0 ) )
		fprintf( log_stats_file, "%i,%i,%i,",r_numentities, r_numdlights, r_numparticles);


	SCR_DrawCrosshair (&cl.refdef);

	if (cl_showPlayerNames->integer)
	{
		SCR_DrawPlayerNames ();
		SCR_DrawBases ();
	}

}

/**
 * @brief Console output of position and orientation
 *
 * Target of 'viewpos' command. Modified 2011-02. Added pitch.
 * Helps with repeatable positioning for timerefresh command
 * and other performance testing.
 */
void V_Viewpos_f (void)
{
	Com_Printf ("x:%#1.0f y:%#1.0f z:%#1.0f yaw:%#1.0f pitch:%#1.0f\n",
		cl.refdef.vieworg[0], cl.refdef.vieworg[1], cl.refdef.vieworg[2],
		cl.refdef.viewangles[YAW], cl.refdef.viewangles[PITCH] );
}

/*
=============
V_Init
=============
*/
void V_Init (void)
{
	Cmd_AddCommand ("viewpos", V_Viewpos_f);

	crosshair = Cvar_Get ("crosshair", "0", CVAR_ARCHIVE);

	cl_testblend = Cvar_Get ("cl_testblend", "0", 0);
	cl_testparticles = Cvar_Get ("cl_testparticles", "0", 0);
	cl_testentities = Cvar_Get ("cl_testentities", "0", 0);
	cl_testlights = Cvar_Get ("cl_testlights", "0", 0);

	cl_stats = Cvar_Get ("cl_stats", "0", 0);
}
