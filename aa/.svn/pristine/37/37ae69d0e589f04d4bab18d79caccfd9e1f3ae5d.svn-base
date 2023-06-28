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
// cl_fx.c -- entity effects parsing and management

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "client.h"
#include "ref_gl/r_image.h"
#include "ref_gl/qgl.h"

extern particle_t	particles[MAX_PARTICLES];
extern int			cl_numparticles;
extern cvar_t		*vid_ref;
extern cvar_t *r_lefthand;
extern void MakeNormalVectors (vec3_t forward, vec3_t right, vec3_t up);

void CL_LogoutEffect (vec3_t org, int type);
void CL_ItemRespawnParticles (vec3_t org);
void CL_TeleportParticles (vec3_t start);

static vec3_t avelocities [NUMVERTEXNORMALS];

/*
==============================================================

LIGHT STYLE MANAGEMENT

==============================================================
*/

typedef struct
{
	int		length;
	float	value[3];
	float	map[MAX_QPATH];
} clightstyle_t;

clightstyle_t	cl_lightstyle[MAX_LIGHTSTYLES];
int			lastofs;

/*
================
CL_ClearLightStyles
================
*/
void CL_ClearLightStyles (void)
{
	memset (cl_lightstyle, 0, sizeof(cl_lightstyle));
	lastofs = -1;
}

/*
================
CL_RunLightStyles
================
*/
void CL_RunLightStyles (void)
{
	int		ofs;
	int		i;
	clightstyle_t	*ls;

	ofs = cl.time / 25;
	if (ofs == lastofs)
		return;
	lastofs = ofs;

	for (i=0,ls=cl_lightstyle ; i<MAX_LIGHTSTYLES ; i++, ls++)
	{
		if (!ls->length || !cl_flicker->integer)
		{
			ls->value[0] = ls->value[1] = ls->value[2] = 1.0;
			continue;
		}
		if (ls->length == 1)
			ls->value[0] = ls->value[1] = ls->value[2] = ls->map[0];
		else
		{
			// nonlinear interpolation of the lightstyles for smoothness while
			// still preserving some "jaggedness"
			float a, b, dist;
			a = ls->map[(ofs/4)%ls->length];
			b = ls->map[(ofs/4+1)%ls->length];
			dist = (float)(ofs%4)/4.0f;
			if (dist != 0.75)
				dist = 0;
			ls->value[0] = ls->value[1] = ls->value[2] = a+(b-a)*dist;
		}
	}
}


void CL_SetLightstyle (int i)
{
	char	*s;
	int		j, k;

	s = cl.configstrings[i+CS_LIGHTS];

	j = strlen (s);
	if (j >= MAX_QPATH)
		Com_Error (ERR_DROP, "svc_lightstyle length=%i", j);

	cl_lightstyle[i].length = j;

	for (k=0 ; k<j ; k++)
		cl_lightstyle[i].map[k] = (float)(s[k]-'a')/(float)('m'-'a');
}

/*
================
CL_AddLightStyles
================
*/
void CL_AddLightStyles (void)
{
	int		i;
	clightstyle_t	*ls;

	for (i=0,ls=cl_lightstyle ; i<MAX_LIGHTSTYLES ; i++, ls++)
		V_AddLightStyle (i, ls->value[0], ls->value[1], ls->value[2]);
}

/*
==============================================================

DLIGHT MANAGEMENT

==============================================================
*/

cdlight_t		cl_dlights[MAX_DLIGHTS];

/*
================
CL_ClearDlights
================
*/
void CL_ClearDlights (void)
{
	memset (cl_dlights, 0, sizeof(cl_dlights));
}

/*
===============
CL_AllocDlight
===============
*/
cdlight_t *CL_AllocDlight (int key)
{
	int		i;
	cdlight_t	*dl;

// first look for an exact key match
	if (key)
	{
		dl = cl_dlights;
		for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
		{
			if (dl->key == key)
			{
				memset (dl, 0, sizeof(*dl));
				dl->key = key;
				return dl;
			}
		}
	}

// then look for anything else
	dl = cl_dlights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
	{
		if (dl->die < cl.time)
		{
			memset (dl, 0, sizeof(*dl));
			dl->key = key;
			return dl;
		}
	}

	dl = &cl_dlights[0];
	memset (dl, 0, sizeof(*dl));
	dl->key = key;
	return dl;
}

/*
===============
CL_NewDlight
===============
*/
void CL_NewDlight (int key, float x, float y, float z, float radius, float time)
{
	cdlight_t	*dl;

	dl = CL_AllocDlight (key);
	dl->origin[0] = x;
	dl->origin[1] = y;
	dl->origin[2] = z;
	dl->radius = radius;
	dl->die = cl.time + time;
}


/*
===============
CL_RunDLights
===============
*/
void CL_RunDLights (void)
{
	int			i;
	cdlight_t	*dl;

	dl = cl_dlights;
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
	{
		if ( dl->radius == 0.0f )
			continue;

		if (dl->die < cl.time)
		{
			dl->radius = 0.0f;
			return;
		}
		dl->radius -= cls.frametime*dl->decay;
		if (dl->radius < 0.0f)
			dl->radius = 0.0f;
	}
}

void RotateForNormal(vec3_t normal, vec3_t result){
	float forward, pitch, yaw;

	forward = sqrtf(normal[0] * normal[0] + normal[1] * normal[1]);
//	pitch = (int)(atan2f(normal[2], forward) * 180 / M_PI);
//	yaw = (int)(atan2f(normal[1], normal[0]) * 180 / M_PI);
	pitch = atan2f( normal[2], forward  ) * 180.0f / (float)M_PI;
	yaw   = atan2f( normal[1], normal[0]) * 180.0f / (float)M_PI;

	if(pitch < 0.0f)
		pitch += 360.0f;

	result[PITCH] = -pitch;
	result[YAW] =  yaw;
}

/*
==============
CL_ParseMuzzleFlash
==============
*/
void CL_ParseMuzzleFlash (void)
{
	vec3_t		fv, rv, shell_brass, dir, up;
	cdlight_t	*dl;
	int			i, j, weapon;
	centity_t	*pl;
	int			silenced;
	float		volume;
	char		soundname[64];

	i = MSG_ReadShort (&net_message);
	if (i < 1 || i >= MAX_EDICTS)
		Com_Error (ERR_DROP, "CL_ParseMuzzleFlash: bad entity");

	weapon = MSG_ReadByte (&net_message);
	silenced = weapon & MZ_SILENCED;
	weapon &= ~MZ_SILENCED;

	pl = &cl_entities[i];

	dl = CL_AllocDlight (i);
	VectorCopy (pl->current.origin,  dl->origin);
	AngleVectors (pl->current.angles, fv, rv, up);
	VectorMA (dl->origin, 18, fv, dl->origin);
	VectorMA (dl->origin, 16, rv, dl->origin);
	if (silenced)
		dl->radius = 75 + (rand()&31);
	else
		dl->radius = 125 + (rand()&31);
	dl->minlight = 32;
	dl->die = cl.time; // + 0.1;

	if (silenced)
		volume = 0.2;
	else
		volume = 1;

	VectorMA(pl->current.origin, 40, fv, shell_brass);
	VectorMA(shell_brass, 15, rv, shell_brass);
	VectorMA(shell_brass, 21, up, shell_brass);

	for (j = 0; j < 3; j++)
		dir[j] = fv[j] + rv[j] + up[j] * 2;

	// NOTE: sounds started here are subject to PVS rather than PHS
	//  which results in these sounds not being heard when others
	//  in same location can be heard
	switch (weapon)
	{
	case MZ_BLASTER:
		dl->color[0] = 0;dl->color[1] = 1;dl->color[2] = 1;
		S_StartSound (NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/blastf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_HYPERBLASTER:
		dl->color[0] = 0.1;dl->color[1] = 1;dl->color[2] = 0.2;
		S_StartSound (NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/hyprbf1a.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_MACHINEGUN:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound (NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0);
		break;
	case MZ_SHOTGUN:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		S_StartSound (NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/shotgf1b.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_SSHOTGUN:
		dl->color[0] = 1;dl->color[1] = 1;dl->color[2] = 0;
		S_StartSound (NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/sshotf1b.wav"), volume, ATTN_NORM, 0);
		break;
	case MZ_CHAINGUN1:
		dl->radius = 75 + (rand()&31);
		dl->color[0] = 1;dl->color[1] = 0.25;dl->color[2] = 0;
		Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound (NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0);
		CL_BrassShells(shell_brass, dir, 1);
		break;
	case MZ_CHAINGUN2:
		dl->radius = 85 + (rand()&31);
		dl->color[0] = 1;dl->color[1] = 0.35;dl->color[2] = 0;
		Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound (NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0);
		Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound (NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0.05);
		CL_BrassShells(shell_brass, dir, 1);
		break;
	case MZ_CHAINGUN3:
		dl->radius = 100 + (rand()&31);
		dl->color[0] = 1;dl->color[1] = .5;dl->color[2] = 0;
		Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound (NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0);
		Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound (NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0.033);
		Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_StartSound (NULL, i, CHAN_WEAPON, S_RegisterSound(soundname), volume, ATTN_NORM, 0.066);
		CL_BrassShells(shell_brass, dir, 1);
		break;
	case MZ_RAILGUN:
		dl->color[0] = 0.5;dl->color[1] = 0.5;dl->color[2] = 1.0;
		break;
	case MZ_ROCKET:
		dl->color[0] = 1;dl->color[1] = 0.5;dl->color[2] = 0.2;
		S_StartSound (NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/rocklf1a.wav"), volume, ATTN_NORM, 0);
		S_StartSound (NULL, i, CHAN_AUTO,   S_RegisterSound("weapons/rocklr1b.wav"), volume, ATTN_NORM, 0.1);
		break;
	case MZ_GRENADE:
		dl->color[0] = 1;dl->color[1] = 0.5;dl->color[2] = 0;
		S_StartSound (NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/grenlf1a.wav"), volume, ATTN_NORM, 0);
		S_StartSound (NULL, i, CHAN_AUTO,   S_RegisterSound("weapons/grenlr1b.wav"), volume, ATTN_NORM, 0.1);
		break;
	case MZ_BFG:
		dl->color[0] = 0;dl->color[1] = 1;dl->color[2] = 0;
		S_StartSound (NULL, i, CHAN_WEAPON, S_RegisterSound("weapons/bfg__f1y.wav"), volume, ATTN_NORM, 0);
		break;

	case MZ_LOGIN:
		dl->color[0] = 0;dl->color[1] = 1; dl->color[2] = 0;
		dl->die = cl.time + 1.0;
		S_StartSound (NULL, i, CHAN_WEAPON, S_RegisterSound("misc/tele1.wav"), 1, ATTN_NORM, 0);
		CL_TeleportParticles (pl->current.origin);
		break;
	case MZ_LOGOUT:
		dl->color[0] = 1;dl->color[1] = 0; dl->color[2] = 0;
		dl->die = cl.time + 1.0;
		S_StartSound (NULL, i, CHAN_WEAPON, S_RegisterSound("misc/tele1.wav"), 1, ATTN_NORM, 0);
		CL_LogoutEffect (pl->current.origin, weapon);
		break;
	case MZ_RESPAWN:
		dl->color[0] = 1;dl->color[1] = 1; dl->color[2] = 0;
		dl->die = cl.time + 1.0;
		S_StartSound (NULL, i, CHAN_WEAPON, S_RegisterSound("misc/tele1.wav"), 1, ATTN_NORM, 0);
		CL_LogoutEffect (pl->current.origin, weapon);
		break;
	}
}


/*
===============
CL_AddDLights
===============
*/
void CL_AddDLights (void)
{
	int			i;
	cdlight_t	*dl;

	dl = cl_dlights;

//=====
//PGM
	for (i=0 ; i<MAX_DLIGHTS ; i++, dl++)
	{
		if (!dl->radius)
			continue;
		V_AddLight (dl->origin, dl->radius,
			dl->color[0], dl->color[1], dl->color[2]);
	}
//PGM
//=====
}



/*
==============================================================

PARTICLE MANAGEMENT

==============================================================
*/

particle_t	*active_particles, *free_particles;

particle_t	particles[MAX_PARTICLES];
int			cl_numparticles = MAX_PARTICLES;

void addParticleLight (particle_t *p, float light, float lightvel, float lcol0, float lcol1, float lcol2)
{
	int i;

	for (i=0; i<P_LIGHTS_MAX; i++)
	{
		cplight_t *plight = &p->lights[i];

		if (!plight->isactive)
		{
			plight->isactive = true;
			plight->light = light;
			plight->lightvel = lightvel;
			plight->lightcol[0] = lcol0;
			plight->lightcol[1] = lcol1;
			plight->lightcol[2] = lcol2;
			return;
		}
	}
}
/*
===============
CL_ClearParticles
===============
*/
/*
Cl_ClearParticle
*/

void CL_ClearParticles (void)
{
	int		i;


	free_particles = &particles[0];
	active_particles = NULL;

	for (i=0 ;i<cl_numparticles ; i++)
		particles[i].next = &particles[i+1];
	particles[cl_numparticles-1].next = NULL;
}

static inline particle_t *new_particle (void)
{
	particle_t	*p;
	//int j;

	if (!free_particles)
		return NULL;

	p = free_particles;
	free_particles = p->next;
	
	memset (p, 0, sizeof(*p));
	
	p->next = active_particles;
	p->time = cl.time;

	return (active_particles = p);
}

#define WEATHER_PARTICLES 2048
static int weather_particles;
extern unsigned r_weather;

static inline void free_particle (particle_t *p)
{
	if (p->type == PARTICLE_WEATHER || p->type == PARTICLE_FLUTTERWEATHER)
		weather_particles--;
	p->next = free_particles;
	free_particles = p;
	p->free = true;
}

float getColor(void)
{
	float color;

	switch(cl_disbeamclr->integer) {
	case 0:
	default:
		color = 0xd6; //bright green
		break;
	case 1:
		color = 0x74; //blue
		break;
	case 2:
		color = 0x40; //red
		break;
	case 3:
		color = 0xe0; //yellow
		break;
	case 4:
		color = 0xff; //purple
		break;
	}
	return color;
}

void getColorvec(vec3_t colorvec)
{
	switch(cl_disbeamclr->integer) {
	case 0:
	default:
		colorvec[0] = 0; //bright green
		colorvec[1] = 1;
		colorvec[2] = 0.6;
		break;
	case 1:
		colorvec[0] = 0; //blue
		colorvec[1] = 0.5;
		colorvec[2] = 0.7;
		break;
	case 2:
		colorvec[0] = 0.6; //red
		colorvec[1] = 0;
		colorvec[2] = 0;
		break;
	case 3:
		colorvec[0] = 0.7; //yellow
		colorvec[1] = 0.7;
		colorvec[2] = 0;
		break;
	case 4:
		colorvec[0] = 0.7; //purple
		colorvec[1] = 0;
		colorvec[2] = 0.7;
		break;
	}
}

/*
===============
CL_ParticleEffect

Wall impact puffs
===============
*/
void CL_ParticleEffect (vec3_t org, vec3_t dir, int color, int count)
{
	int			i, j;
	particle_t	*p;
	float		d;

	if((color == 450 || color == 550) && cl_noblood->value)
		return;

	for (i=0 ; i<count ; i++)
	{
		if (!(p = new_particle()))
			return;

		d = rand()&31;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()&7)-4) + d*dir[j];
			p->vel[j] = crand()*50;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		
		p->type = PARTICLE_STANDARD;
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE_MINUS_SRC_ALPHA;
		p->alpha = 0.2;
		p->scalevel = 0;

		if (color == 425)		// gunshots: FIXME should be using type
		{
			p->image = r_pufftexture;
			p->scale = 4 + (rand()&2);
			p->alphavel = -1.0f / (1.5f + frand()*0.3f);
			p->accel[2] = PARTICLE_GRAVITY;
		}
		else if (color == 450)
		{
			p->image = r_bloodtexture;
			p->alphavel = -1.0f / (4.5f + frand()*0.3f);
			p->color = 0xe8; //(155 31 0) (0.607843 0.121569 0)
			p->scale = 6;
		}
		else if (color == 550)
		{
			p->image = r_bloodtexture;
			p->alphavel = -1.0f / (4.5f + frand()*0.3f);
			p->color = 0xd0 + (rand()&3); // (0-83 255-187 0-39) (0-0.32549 1-0.7333 0-0.1529)
			p->scale = 6;
		}
		else
		{
			p->type = PARTICLE_NONE;
			p->image = r_particletexture;
			p->blenddst = GL_ONE;
			p->scale = 1;
			p->alpha = 1.0;
			p->color = color;
			p->alphavel = -1.0f / (0.5f + frand()*0.3f);
		}


	}
}


/*
===============
CL_ParticleEffect2
===============
*/
void CL_ParticleEffect2 (vec3_t org, vec3_t dir, int color, int count)
{
	int			i, j;
	particle_t	*p;
	float		d;

	for (i=0 ; i<count ; i++)
	{
		if (!(p = new_particle()))
			return;

		p->type = PARTICLE_STANDARD;
		p->image = r_particletexture;
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE;
		p->scale = 1;
		p->scalevel = 0;

		p->color = color;

		d = rand()&7;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()&7)-4) + d*dir[j];
			p->vel[j] = crand()*20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0f / (0.5f + frand()*0.3f);
	}
}

/*
===============
CL_BulletSparks
===============
*/
void CL_BulletSparks (vec3_t org, vec3_t dir)
{
	int			i, j, k;
	float		inc, scale, nudge;
	particle_t	*p;
	particle_t	*pr;
	particle_t	*center;

	for( i=0; i<3; i++) {
		nudge = frand();
		if(nudge < 0.5)
			dir[i] += frand();
		else
			dir[i] -= frand();
	}

	//draw a fatter glow at the impact point
	if(!(p = new_particle()))
		return;

	p->type = PARTICLE_STANDARD;
	p->image = r_particletexture;
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	p->scale = 1;
	p->scalevel = 0;

	p->color = 0xe0 + (rand()&2); // (255-239 171-127 7-0) (1.0-0.937255 0.670588-0.498039 0.027451-0.0)
	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = org[j];
		p->vel[j] = 0;
	}
	p->accel[0] = p->accel[1] = p->accel[2] = 0;
	p->alpha = .5;

	p->alphavel = -1.0f / (3.0f + frand()*0.3f);
	center = p;

	//shoot off sparks

	VectorNormalize(dir);

	for( k=0; k<2; k++) {

		scale = frand()*3.0f;
		pr = NULL;

		i = 0;
		for (inc=1.0f ; inc<2.0f ; inc+=0.25f, i++)
		{
			if (!(p = new_particle()))
				return;

			p->color = 0xe0 + (rand()&2); // (255-239 171-127 7-0) (1.0-0.937255 0.670588-0.498039 0.027451-0.0)
			p->type = PARTICLE_CHAINED;
			p->image = r_raintexture;
			p->blendsrc = GL_SRC_ALPHA;
			p->blenddst = GL_ONE;
			p->scale = 1.25f * scale/inc;
			p->scalevel = 0;

			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + i * (-1.25f*scale) * dir[j]; 
				p->vel[j] = 60.0f * dir[j];
			}

			p->accel[0] = 0;
			p->accel[1] = 0;
			p->accel[2] = -((float)PARTICLE_GRAVITY) / (0.5f*inc);
			p->alpha = .5;

			p->alphavel = -1.0f / (2.5f + frand()*0.3f);
			
			if (pr) 
			{
				p->chain_prev = pr;
			}
			pr = p;
		}
	}
}

/*
===============
CL_BloodSplatter - simple blood effects
===============
*/

void CL_BloodSplatter ( vec3_t pos, vec3_t pos2, int color, int blend )
{
	particle_t *p;
	vec3_t		v;
	int			j;
	trace_t	trace;
	static vec3_t mins = { -1.0f, -1.0f, -1.0f };
    static vec3_t maxs = { 1.0f, 1.0f, 1.0f };

	//trace to see if impact occurs with nearby brush
	trace = CL_Trace ( pos, mins, maxs, pos2, -1, MASK_SOLID, true, NULL);
	if(trace.contents) 
	{	
		//hit a brush
		if(!(p = new_particle()))
			return;

		p->image = r_bloodtexture;
		p->color = color + (rand() & 1);
		p->type = PARTICLE_DECAL;
		p->blendsrc = GL_SRC_ALPHA;
		if(blend) //glowing blood
			p->blenddst = GL_ONE;
		else
			p->blenddst = GL_ONE_MINUS_SRC_ALPHA;
		p->scale = 2;
		p->scalevel = 0;

		VectorScale(trace.plane.normal, -1, v);
		RotateForNormal(v, p->angle);
		p->angle[ROLL] = rand() % 360;
		VectorAdd(pos2, trace.plane.normal, p->org);

		p->alpha = 1.0;
		p->alphavel = -0.2f / (2.0f + frand() * 0.3f);
		for (j=0 ; j<3 ; j++)
		{
				p->accel[j] = 0;
				p->vel[j] = 0;
		}
	}
}

void CL_BigBloodSplatterGround(vec3_t org, int color, int blend)
{
	particle_t *p;
	vec3_t		v, end, dir;
	int			j;
	trace_t	trace;
	static vec3_t mins = { -1.0f, -1.0f, -1.0f };
    static vec3_t maxs = { 1.0f, 1.0f, 1.0f };

	//VectorAdd(org, dir, end);
	VectorCopy(org, end);
	end[2] -= 128;
	trace = CL_Trace ( org, mins, maxs, end, -1, MASK_SOLID, true, NULL);
	if(trace.contents) 
	{	
		if(!(p = new_particle()))
			return;

		p->image = r_bloodtexture;
		p->color = color + (rand() & 1);
		p->type = PARTICLE_DECAL;
		p->blendsrc = GL_SRC_ALPHA;
		if(blend) //glowing blood
			p->blenddst = GL_ONE;
		else
			p->blenddst = GL_ONE_MINUS_SRC_ALPHA;
		p->scale = 4;
		p->scalevel = 0;

		VectorSet(dir, 0, 0, 1);

		VectorScale(dir, -1, v);
		RotateForNormal(v, p->angle);
		p->angle[ROLL] = rand() % 360;

		VectorCopy(trace.endpos, p->org);

		p->alpha = 0.6;
		p->alphavel = -0.2f / (2.0f + frand() * 0.3f);
		for (j=0 ; j<3 ; j++)
		{
				p->accel[j] = 0;
				p->vel[j] = 0;
		}
	}
}

void CL_BloodEffect (vec3_t org, vec3_t dir, int color, int count)
{
	int			i, j, k;
	particle_t	*p;
	float		d;

	if((color == 450 || color == 550) && cl_noblood->value)
		return;

	for (i=0 ; i<count ; i++)
	{
		vec3_t porg, pvel;

		d = rand()&31;
		for (j=0 ; j<3 ; j++)
		{
			porg[j] = org[j] + ((rand()&7)-4) + d*dir[j];
			pvel[j] = crand()*40;
		}

		// add blood stains
		if (!cl_noblood->value)
		{	
			if (color == 450)
				CL_BigBloodSplatterGround(porg, 0xe8, 0);
			else if (color == 550)
				CL_BigBloodSplatterGround(porg, 0xd0, 1);
		}

		for(k = 0; k < 5; k++)
		{
			if (!(p = new_particle()))
				return;
			
			VectorCopy(porg, p->org);
			VectorCopy(pvel, p->vel);

			p->accel[0] = p->accel[1] = 0;
			p->accel[2] = -PARTICLE_GRAVITY/2.0f;
		
			p->type = PARTICLE_STANDARD;
			p->blendsrc = GL_SRC_ALPHA;
			
			p->alpha = 0.5f;
			p->scalevel = 20.0f;
		 
			p->scale = 10;
				
			if (color == 450)
			{
				p->color = 0xe8;
				p->blenddst = GL_ONE_MINUS_SRC_ALPHA;
			}
			else if (color == 550)
			{
				p->blenddst = GL_ONE;
				p->color = 0xd0 + (rand()&3); 
			}

			p->alphavel = -4.0f / ((float)k + 5.0f);

			switch(k) {
				case 0:
					p->image = r_bloodtexture;
					break;
				case 1:
					p->image = r_blood2texture;
					break;
				case 2:
					p->image = r_blood3texture;
					break;
				case 3:
					p->image = r_blood4texture;
					break;
				case 4:
					p->image = r_blood5texture;
					break;
				default:
					break;
			}
		}
	}
}

void CL_FlameThrower (vec3_t org, vec3_t angles, qboolean from_client)
{
	int			i, j;
	float		inc, scale, nudge, rightoffset, upoffset, sFactor;
	particle_t	*p;
	vec3_t mflashorg, vforward, vright, vup, vec, dir;

	if(from_client)
		sFactor = 200.0f;
	else
		sFactor = 80.0f;

	VectorCopy(org, mflashorg);
	for (j=0 ; j<3 ; j++)
	{
		mflashorg[j] = mflashorg[j] + ((rand()%2)-1);
	}

	AngleVectors (angles, vforward, vright, vup);
	
	if (r_lefthand->value == 1.0F)
		rightoffset = -2.0f;
	else
		rightoffset = 2.0f;

	if(from_client)
		upoffset = -2.0f;
	else
		upoffset = 4.0f;

	if(cl.refdef.fov_x > 90)
	{
		if (r_lefthand->value == 1.0F)
			rightoffset -= (cl.refdef.fov_x - 90.0)/4.0;
		else
			rightoffset -= (cl.refdef.fov_x - 90.0)/25.0;
		upoffset += (cl.refdef.fov_x - 90.0)/10.3;
	}
	
	VectorMA(mflashorg, 24, vforward, mflashorg);
	VectorMA(mflashorg, rightoffset, vright, mflashorg);
	VectorMA(mflashorg, upoffset, vup, mflashorg);
	
	VectorCopy(vforward, dir);

	for( i=0; i<3; i++) {
		nudge = frand();
		if(nudge < 0.5f)
			dir[i] += frand()/10.0f;
		else
			dir[i] -= frand()/10.0f;
	}		
		
	scale = frand()*2.0f;
	
	if (!(p = new_particle()))
		return;	

	p->color = 15 + (rand()&2);
	p->type = PARTICLE_STANDARD;
	p->image = r_fireballtexture;
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	p->scale = 1.0f + scale/3.0f;
	p->scalevel = sFactor/4.0f;	
	p->colorvel = 5.0f;
	VectorNormalize(dir);
	for (j=0 ; j<3 ; j++)
	{
		if(from_client)
			p->org[j] = mflashorg[j] + (15.0f*scale) * dir[j]; 
		else
			p->org[j] = mflashorg[j] + (45.0f*scale) * dir[j]; 
		p->vel[j] = sFactor * dir[j];
	}

	p->accel[0] = sFactor*dir[0];
	p->accel[1] = sFactor*dir[1];
	p->accel[2] = PARTICLE_GRAVITY/2.0f;
	if(from_client)
	{		
		p->alpha = 1.0f;
	}
	else
	{
		p->alpha = 0.75f;
	}

	p->alphavel = -1.0f / (1.0f + frand()*0.5f);

	if(!from_client)
	{
		// Add sprite for blast coming fowward
		if (!(p = new_particle()))
			return;

		p->type = PARTICLE_BEAM;
		p->image = r_firestream3ptexture;
		p->scale = 12 + (rand()&3);
		VectorMA(mflashorg, 8, vup, mflashorg);
		VectorMA(mflashorg, 60, vforward, mflashorg);
		VectorAdd(mflashorg, vforward, vec);
		VectorSet(p->angle, vec[0], vec[1], vec[2]);
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = mflashorg[j];
			p->vel[j] = 0.0f;
		}
		p->accel[0] = p->accel[1] = 0.0f;
		p->accel[2] = 0.0f;
		p->alpha = 1.0f;
		p->color = 0xd9 + (rand()&7);
		p->alphavel = -17.0f;	
	}

	//shoot off sparks
	scale = frand();

	for( i=0; i<3; i++) {
		nudge = frand();
		if(nudge < 0.5f)
			dir[i] += frand()/5.0f;
		else
			dir[i] -= frand()/5.0f;
	}	

	VectorNormalize(dir);

	if(nudge < 0.4f)
		nudge = 0.4f;

	if(from_client)
		VectorMA(mflashorg, 90.0f * nudge, vforward, mflashorg);
	else
		VectorMA(mflashorg, 60.0f * nudge, vforward, mflashorg);

	i = 0;
	for (inc=1.0f ; inc<2.0f ; inc+=0.1f, i++)
	{
		if (!(p = new_particle()))
			return;

		p->color = 15 + (rand()&2);
		p->type = PARTICLE_STANDARD;
		p->image = r_particletexture;
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE;
		p->scale = 1.5f*scale/inc;
		p->scalevel = 0;

		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = mflashorg[j] + i*(-1.5f*scale)*dir[j];
			if(from_client)
				p->vel[j] = 60.0f*dir[j];
			else
				p->vel[j] = 35.0f*dir[j];
		}

		p->accel[0] = 0;
		p->accel[1] = 0;
		p->accel[2] = -((float)PARTICLE_GRAVITY) / (0.5f*inc);
		p->alpha = .5;

		p->alphavel = -1.0f / (1.5f + frand()*0.3f);
	}	
}

particle_t *CL_FlameThrowerParticle (vec3_t org, vec3_t angles, particle_t *previous)
{
	int			j;
	particle_t	*p;
	vec3_t		mflashorg, vforward, vright, vup;
	float		rightoffset, upoffset;

	AngleVectors (angles, vforward, vright, vup);

	VectorCopy(org, mflashorg);

	if (r_lefthand->value == 1.0F)
		rightoffset = -1.1;
	else
		rightoffset = 1.1;

	upoffset = -0.75;

	if(cl.refdef.fov_x > 90)
	{
		if (r_lefthand->value == 1.0F)
			rightoffset -= (cl.refdef.fov_x - 90.0)/4.0;
		else
			rightoffset -= (cl.refdef.fov_x - 90.0)/25.0;
		upoffset += (cl.refdef.fov_x - 90.0)/10.3;
	}
	
	VectorMA(mflashorg, 24, vforward, mflashorg);
	VectorMA(mflashorg, rightoffset, vright, mflashorg);
	VectorMA(mflashorg, upoffset, vup, mflashorg);

	if (!(p = new_particle()))
		return NULL;

	p->type = PARTICLE_CHAINED;
	p->image = r_firestreamtexture;
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	p->scale = 6.0f;
	p->scalevel = 4.0f;

	p->color = 15;
	p->colorvel = 15.0f;

	for (j = 0; j < 3; j++)
	{
		p->org[j] = mflashorg[j];
		p->vel[j] = vforward[j] * 175.0f;
		p->accel[j] = p->vel[j];
	}
	
	p->vel[2] += PARTICLE_GRAVITY * 0.1;
	p->accel[2] = PARTICLE_GRAVITY/2.0f;
	p->alpha = 5.0;

	p->alphavel = -10.0f;// / (0.1f + frand()*0.15f);
	
	if (previous != NULL)
	{
		vec3_t movement;
		float dist;
		
		VectorSubtract (p->org, previous->org, movement);
		dist = 5.0f * VectorLength (movement) / (p->time - previous->time);
		if (dist < 1.0)
			dist = 1.0;
		p->alpha /= dist;
	}
	
	p->chain_prev = previous;
	
	return p;
}

void CL_JetExhaust (vec3_t org, vec3_t dir)
{
	int			i, j;
	float		scale, nudge;
	particle_t	*p;
	
	for( i=0; i<3; i++) {
		nudge = frand();
		if(nudge < 0.5)
			dir[i] += frand()/10.0;
		else
			dir[i] -= frand()/10.0;
	}
		
	scale = frand();
	
	if (!(p = new_particle()))
		return;

	p->color = 0xe0 + (rand()&2); 
	p->type = PARTICLE_STANDARD;
	p->image = r_fireballtexture;
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	p->scale = 3.0f + scale;
	p->scalevel = 25.0f;

	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = org[j];
		p->vel[j] = 50.0f * dir[j];
	}

	p->accel[0] = 35*dir[0];
	p->accel[1] = 35*dir[1];
	p->accel[2] = ((float)PARTICLE_GRAVITY) / (0.75f);
	p->alpha = .4;

	p->alphavel = -1.0f / (3.0f + frand()*0.3f);

	if (!(p = new_particle()))
		return;

	p->color = 0x74 + (rand()&7);
	p->type = PARTICLE_STANDARD;
	p->image = r_fireballtexture;
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	p->scale = 1.25f + scale/2.0;
	p->scalevel = 10.0f;

	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = org[j];
		p->vel[j] = 25.0f * dir[j];
	}

	p->accel[0] = 25*dir[0];
	p->accel[1] = 25*dir[1];
	p->accel[2] = ((float)PARTICLE_GRAVITY) / (4.5f);
	p->alpha = .5;

	p->alphavel = -1.0f / (3.0f + frand()*0.3f);

	if (!(p = new_particle()))
		return;

	p->color = 15 + (rand()&2);
	p->type = PARTICLE_STANDARD;
	p->image = r_fireballtexture;
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	p->scale = 0.5f + scale/3.0;
	p->scalevel = 5;

	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = org[j];
		p->vel[j] = 15.0f * dir[j];
	}

	p->accel[0] = 15*dir[0];
	p->accel[1] = 15*dir[1];
	p->accel[2] = ((float)PARTICLE_GRAVITY) / (10.0f);
	p->alpha = .5;

	p->alphavel = -1.0f / (2.5f + frand()*0.3f);

	if (!(p = new_particle()))
		return;

	p->color = 15 + (rand()&2);
	p->type = PARTICLE_STANDARD;
	p->image = r_fireballtexture;
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	p->scale = 0.5f + scale/3.0;
	p->scalevel = 5;

	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = org[j];
		p->vel[j] = 8.0f * dir[j];
	}

	p->accel[0] = 8*dir[0];
	p->accel[1] = 8*dir[1];
	p->accel[2] = ((float)PARTICLE_GRAVITY) / (10.0f);
	p->alpha = .5;

	p->alphavel = -1.0f / (2.5f + frand()*0.3f);
}

/*
===============
CL_SplashEffect
===============
*/

void CL_SplashEffect (vec3_t org, vec3_t dir, int color, int count, int type)
{
	int			i, j, k;
	float		scale, nudge, sHeight;
	particle_t	*p;
	vec3_t		angle;

	//draw rings that expand outward
	if(!(p = new_particle()))
		return;
	p->type = PARTICLE_RAISEDDECAL;
	p->image = r_splashtexture;
	p->blendsrc = GL_DST_COLOR;
	p->blenddst = GL_SRC_COLOR;
	p->scale = 0.5;
	p->scalevel = 8;
	p->color = 0 + (rand() & 1); //(0-15 0-15 0-15) (0-0.0588...)

	VectorScale(dir, -1, angle);
	RotateForNormal(angle, p->angle);
	p->angle[ROLL] = rand() % 360;
	VectorAdd(org, dir, p->org);

	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = org[j];
		p->vel[j] = 0;
	}
	p->accel[0] = p->accel[1] = p->accel[2] = 0;
	p->alpha = .1;

	p->alphavel = -0.1f / (1.0f + frand()*0.3f);

	//if(type == SPLASH_BLOOD)
	//	count = 24;

	for( k=0; k<count/4; k++) {

		for( i=0; i<3; i++) {
			nudge = frand();
			if(nudge < 0.5)
				dir[i] += frand()/2;
			else
				dir[i] -= frand()/2;
		}

		VectorNormalize(dir);

		//shoot off small plume
		i = 0;
		for (i=0; i<count/2; i++)
		{
			if (!(p = new_particle()))
				return;
			
			switch(type)
			{
			case SPLASH_LAVA:
				p->image = r_lavasplashtexture;
				p->color = color - (rand()&2);
				p->blendsrc = GL_SRC_ALPHA;
				p->blenddst = GL_ONE;
				p->alpha = 0.5;
				scale = 15.0;
				sHeight = 10.0;
				break;
			case SPLASH_BLOOD:
				p->image = r_splash2texture;
				p->color = 0xe8;
				p->blendsrc = GL_SRC_ALPHA;
				p->blenddst = GL_ONE_MINUS_SRC_ALPHA;
				p->alpha = 0.666;
				scale = 1.0;
				sHeight = 8.0;
				break;
			default:
				p->image = r_splash2texture;
				p->color = color - (rand()&2);
				p->blendsrc = GL_SRC_ALPHA;
				p->blenddst = GL_ONE;
				p->alpha = 0.35;
				scale = 1.0;
				sHeight = 10.0;
				break;
			}
						
			p->type = PARTICLE_VERT;			
			p->scale = scale * 4 * (rand()&8);
			p->scalevel = 2;

			for (j=0; j<3; j++)
			{
				p->org[j] = org[j] + sHeight*dir[j];
				p->vel[j] = 60*dir[j];
			}
			p->org[2]+=sHeight*3.2;

			p->accel[0] = 0;
			p->accel[1] = 0;
			p->accel[2] = -((float)PARTICLE_GRAVITY) / 0.5f;			

			p->alphavel = -1.0f / (1.5f + frand()*0.3f);
		}
	}
}
/*
===============
CL_LaserSparks
===============
*/
void CL_LaserSparks (vec3_t org, vec3_t dir, int color, int count)
{
	int			i, j, k;
	float		inc, scale, nudge;
	particle_t	*p;

	for( i=0; i<3; i++) {
		nudge = frand();
		if(nudge < 0.5)
			dir[i] += frand();
		else
			dir[i] -= frand();
	}

	//shoot off sparks
	for( k=0; k<2; k++) {

		VectorNormalize(dir);

		scale = frand();

		i = 0;
		for (inc=1.0f ; inc<2.0f ; inc+=0.1f, i++)
		{
			if (!(p = new_particle()))
				return;

			p->color = color + (rand()&2);
			p->type = PARTICLE_STANDARD;
			p->image = r_particletexture;
			p->blendsrc = GL_SRC_ALPHA;
			p->blenddst = GL_ONE;
			p->scale = 1.5f*scale/inc;
			p->scalevel = 0;

			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + i*(-1.5f*scale)*dir[j];
				p->vel[j] = 60.0f*dir[j];
			}

			p->accel[0] = 0;
			p->accel[1] = 0;
			p->accel[2] = -((float)PARTICLE_GRAVITY) / (0.5f*inc);
			p->alpha = .5;

			p->alphavel = -1.0f / (1.5f + frand()*0.3f);
		}
	}
}

/*
===============
CL_LogoutEffect
===============
*/
void CL_LogoutEffect (vec3_t org, int type)
{
	int			i, j;
	particle_t	*p;

	for (i=0 ; i<500 ; i++)
	{
		if (!(p = new_particle()))
			return;
		
		p->type = PARTICLE_STANDARD;
		p->scale = 1;
		p->image = r_particletexture;
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE;

		if (type == MZ_LOGIN)
			p->color = 0xd0 + (rand()&7);	// green (0-95 255-123 0-51)
		else if (type == MZ_LOGOUT)
			p->color = 0x40 + (rand()&7);	// red (167-87 59-19 43-0)
		else
			p->color = 0xe0 + (rand()&7);	// yellow (255-171 171-43 0)

		p->org[0] = org[0] - 16 + frand()*32;
		p->org[1] = org[1] - 16 + frand()*32;
		p->org[2] = org[2] - 24 + frand()*56;

		for (j=0 ; j<3 ; j++)
			p->vel[j] = crand()*20;

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0f / (1.0f + frand()*0.3f);
	}
}


/*
===============
CL_ItemRespawnParticles
===============
*/
void CL_ItemRespawnParticles (vec3_t org)
{
	int			i, j;
	particle_t	*p;

	for (i=0 ; i<64 ; i++)
	{
		if (!(p = new_particle()))
			return;

		p->color = 0x74 + (rand()&7); //(23-0 83-31 111-43)
		p->scale = 12 + (rand()&7) ;
		p->scalevel = 0;
		p->type = PARTICLE_STANDARD;
		p->image = r_explosiontexture;
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE;
		p->org[0] = org[0] + crand()*16;
		p->org[1] = org[1] + crand()*16;
		p->org[2] = org[2] + crand()*16;

		for (j=0 ; j<3 ; j++)
			p->vel[j] = crand()*8;

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -((float)PARTICLE_GRAVITY) * 0.2f;
		p->alpha = .3;

		p->alphavel = -1.0f / (1.0f + frand()*0.3f);

		if (i < 4)
			addParticleLight (p,
						p->scale*30.0f, 10,
					0, 1, 1);

	}
}

/*
===============
CL_ExplosionParticles - new version -
===============
*/
extern int r_drawing_fbeffect;
extern int	r_fbFxType;
extern float r_fbeffectTime;
extern vec3_t r_explosionOrigin;
extern float rs_realtime;
void CL_ExplosionParticles (vec3_t org)
{
	int			i, j, k;
	particle_t	*p;

	//for glsl framebuffer distortion effects
	if(!r_drawing_fbeffect && cl_explosiondist->value) {
		r_fbFxType = 1; //EXPLOSION
		r_drawing_fbeffect = true;
		VectorCopy(org, r_explosionOrigin);
		r_fbeffectTime = rs_realtime;
	}

	for (i=0 ; i<7; i++)
	{
		float ifl = (float)i;

		for (k = 0; k<(12-i); k++) {

			float kfl = (float)k;
	
			if (!(p = new_particle()))
				return;

			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = org[j] + ((rand()%16)-8);
				p->vel[j] = 0;
			}
			p->type = PARTICLE_STANDARD;
			p->accel[0] = p->accel[1] = p->accel[2] = 0;
			p->alpha = 0.2;
			p->scale = 10;
			p->scalevel = 52;
			p->color = 0xd9 + (rand()&7); //(255 255-171 167-7)

			p->alphavel = -1.0f / (1.0f + ifl + kfl/5.0f);
			/* expression note: was k/5 ?==? kfl/5.0f
			 * k=[0,12],[0,11],...
			 * so k/5 might be 0,0,0,0,0,1,1,1,1,1,...
			 * if integer division is done before conversion to float
			 * What was intended?
			 */

			p->blendsrc = GL_SRC_ALPHA;
			p->blenddst = GL_ONE;
			switch(i) {
				case 0:
					p->image = r_explosion1texture;
					break;
				case 1:
					p->image = r_explosion2texture;
					break;
				case 2:
					p->image = r_explosion3texture;
					break;
				case 3:
					p->image = r_explosion4texture;
					p->blendsrc = GL_SRC_ALPHA;
					p->blenddst = GL_ONE_MINUS_SRC_ALPHA;
					break;
				case 4:
					p->image = r_explosion5texture;
					break;
				case 5:
					p->image = r_explosion6texture;
					p->blendsrc = GL_SRC_ALPHA;
					p->blenddst = GL_ONE;
					break;
				case 6:
					p->image = r_explosion7texture;
					break;
				default:
					p->image = r_explosion1texture;
					break;
			}
			if (p && i < 3)
				addParticleLight (p,
						p->scale*50.0f*ifl, 0,
					.1, .1, 0.025);
		}
	}

	//place a big shock wave effect
	if (!(p = new_particle()))
			return;
	p->alpha = 1.0;
	p->alphavel = -2.0f;
	p->type = PARTICLE_FLAT;
	p->image = r_explosion5texture;
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	p->color = 0xd9 + (rand()&7); //(255 255-171 167-7)
	p->scale = 12 + (rand()&4) ;
	p->scalevel = 100;
	for(j = 0; j < 3; j++) {
		p->org[j] = org[j];
		p->vel[j] = 0;
		p->accel[j] = 0;
	}

	//add smoke
	for (i=0 ; i<7; i++)
	{

		if (!(p = new_particle()))
			return;

		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()%32)-16);
			p->vel[j] = 0;
		}
		p->type = PARTICLE_STANDARD;
		p->accel[0] = p->accel[1] = p->accel[2] = 0;
		p->alpha = 0.2;
		p->alphavel = -2.0f / (30.0f + frand()*1.4f); //smoke lingers longer

		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE_MINUS_SRC_ALPHA;
		p->scale = 1 + (rand()&4);
		p->scalevel = 12.0;
		p->color = 1 + (rand()&10); // (15 15 15), (47 47 47), (139 139 139), (171 171 171)
		p->accel[2] = 10;

		p->image = r_smoketexture;

	}
}

void CL_FireParticles (vec3_t org)
{
	int			i, j;
	particle_t	*p;

	//for glsl framebuffer distortion effects
	if(!r_drawing_fbeffect && cl_explosiondist->value) {
		r_fbFxType = 1; //EXPLOSION
		r_drawing_fbeffect = true;
		VectorCopy(org, r_explosionOrigin);
		r_fbeffectTime = rs_realtime;
	}	
	
	if (!(p = new_particle()))
		return;

	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = org[j] + ((rand()%16)-8);
		p->vel[j] = 0;
	}
	p->vel[2] = PARTICLE_GRAVITY*1.5f;
	p->type = PARTICLE_STANDARD;
	p->accel[0] = p->accel[1] = p->accel[2] = 0;
	p->alpha = 0.3f;
	p->scale = 10.0f;
	p->scalevel = 30.0f;
	p->color = 0xd9 + (rand()&7); //(255 255-171 167-7)
	p->alphavel = -1.0f;

	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	p->image = r_explosiontexture;
			
	addParticleLight (p,
				p->scale*10.0f, 0,
			.1, .1, 0.025);	
	
	// add smoke
	for (i=0 ; i<4; i++)
	{

		if (!(p = new_particle()))
			return;

		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()%32)-16);
			p->vel[j] = 0;
		}
		p->type = PARTICLE_STANDARD;
		p->accel[0] = p->accel[1] = p->accel[2] = 0;
		p->alpha = 0.2;
		p->alphavel = -2.0f / (20.0f + frand()*1.4f); //smoke lingers longer

		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE_MINUS_SRC_ALPHA;
		p->scale = 1 + (rand()&4);
		p->scalevel = 12.0;
		p->color = 1 + (rand()&10); // (15 15 15), (47 47 47), (139 139 139), (171 171 171)
		p->accel[2] = 10;

		p->image = r_smoketexture;
	}
}

/*
===============
CL_MuzzleParticles - changed this to a smoke effect in 6.06
===============
*/
void CL_MuzzleParticles (vec3_t org)
{
	int			i, j;
	particle_t	*p;

	for ( i = 0; i < 4; i++)
	{
		if (!(p = new_particle()))
			return;

		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + crand()*1;
			p->vel[j] = crand()*5;
		}

		p->alphavel = -1.0f / (30.0f + frand()*1.4f); //smoke lingers longer
		p->alpha = .07;
		p->type = PARTICLE_STANDARD;
		p->image = r_smoketexture;
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE_MINUS_SRC_ALPHA;
		p->scale = 1 + (rand()&4);
		p->scalevel = 12.0;
		p->color = 15; // (255 255 255)
		p->accel[2] = 20;
	}

}
/*
===============
CL_MuzzleParticles
===============
*/

void CL_BlasterMuzzleParticles (vec3_t org, vec3_t angles, float color, float alpha, qboolean from_client)
{
	int			j;
	particle_t	*p;
	vec3_t		mflashorg, vforward, vright, vup, vec;
	float		rightoffset, upoffset;

	VectorCopy(org, mflashorg);
	for (j=0 ; j<3 ; j++)
	{
		mflashorg[j] = mflashorg[j] + ((rand()%2)-1);

	}

	AngleVectors (angles, vforward, vright, vup);

	if(from_client) 
	{
		if (r_lefthand->value == 1.0F)
			rightoffset = -2.0;
		else
			rightoffset = 2.0;

		upoffset = -1.0;

		if(cl.refdef.fov_x > 90)
		{
			if (r_lefthand->value == 1.0F)
				rightoffset -= (cl.refdef.fov_x - 90.0)/4.0;
			else
				rightoffset -= (cl.refdef.fov_x - 90.0)/25.0;
			upoffset += (cl.refdef.fov_x - 90.0)/10.3;
		}
	
		VectorMA(mflashorg, 24, vforward, mflashorg);
		VectorMA(mflashorg, rightoffset, vright, mflashorg);
		VectorMA(mflashorg, upoffset, vup, mflashorg);
	}
	else
	{
		VectorMA(mflashorg, 42, vforward, mflashorg);
		VectorMA(mflashorg, 14, vup, mflashorg);
	}
	
	if (!(p = new_particle()))
		return;

	p->type = PARTICLE_STANDARD;
	p->image = r_cflashtexture;
	p->scale = 10 + (rand()&3);
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = mflashorg[j];
		p->vel[j] = 0;
	}
	p->accel[0] = p->accel[1] = 0;
	p->accel[2] = 0;
	p->alpha = alpha;
	p->color = color;
	p->alphavel = -100.0f;

	// Add second sprite for blast coming fowward
	if (!(p = new_particle()))
		return;

	p->type = PARTICLE_BEAM;
	p->image = r_fflashtexture;
	p->scale = 7 + (rand()&3);
	//project a point forward
	VectorNormalize(vforward);
	VectorScale(vforward, 12.0f, vforward);
	VectorAdd(mflashorg, vforward, mflashorg);
	VectorAdd(mflashorg, vforward, vec);
	VectorSet(p->angle, vec[0], vec[1], vec[2]);
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = mflashorg[j];
		p->vel[j] = 0;
	}
	p->accel[0] = p->accel[1] = 0;
	p->accel[2] = 0;
	p->alpha = alpha;
	p->color = color;
	p->alphavel = -100.0f;	
}

void CL_LightningBall (vec3_t org, vec3_t angles, float color, qboolean from_client)
{
	int			j;
	particle_t	*p;
	vec3_t		mflashorg, vforward, vright, vup, vec;
	float		rightoffset, upoffset;

	VectorCopy(org, mflashorg);
	for (j=0 ; j<3 ; j++)
	{
		mflashorg[j] = mflashorg[j] + ((rand()%2)-1);

	}

	AngleVectors (angles, vforward, vright, vup);

	if(from_client) 
	{
		if (r_lefthand->value == 1.0F)
			rightoffset = -2.0;
		else
			rightoffset = 2.0;

		upoffset = -1.0;

		if(cl.refdef.fov_x > 90)
		{
			if (r_lefthand->value == 1.0F)
				rightoffset -= (cl.refdef.fov_x - 90.0)/4.0;
			else
				rightoffset -= (cl.refdef.fov_x - 90.0)/25.0;
			upoffset += (cl.refdef.fov_x - 90.0)/10.3;
		}
	
		VectorMA(mflashorg, 24, vforward, mflashorg);
		VectorMA(mflashorg, rightoffset, vright, mflashorg);
		VectorMA(mflashorg, upoffset, vup, mflashorg);
	}
	else
	{
		VectorMA(mflashorg, 42, vforward, mflashorg);
		VectorMA(mflashorg, 14, vup, mflashorg);
	}
	
	if (!(p = new_particle()))
		return;

	p->type = PARTICLE_ROTATINGROLL;
	p->image = r_shottexture;
	p->scale = 10 + (rand()&3);
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = mflashorg[j];
		p->vel[j] = 0;
	}
	p->accel[0] = p->accel[1] = 0;
	p->accel[2] = 0;
	p->alpha = 1.0;
	p->color = color;
	p->alphavel = -100.0f;

	// Add second sprite for blast coming fowward
	if (!(p = new_particle()))
		return;

	p->type = PARTICLE_BEAM;
	p->image = r_fflashtexture;
	p->scale = 7 + (rand()&3);
	//project a point forward
	VectorNormalize(vforward);
	VectorScale(vforward, 12.0f, vforward);
	VectorAdd(mflashorg, vforward, mflashorg);
	VectorAdd(mflashorg, vforward, vec);
	VectorSet(p->angle, vec[0], vec[1], vec[2]);
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = mflashorg[j];
		p->vel[j] = 0;
	}
	p->accel[0] = p->accel[1] = 0;
	p->accel[2] = 0;
	p->alpha = 0.7;
	p->color = color;
	p->alphavel = -100.0f;	
}

/*
===============
CL_MuzzleFlashParticle
===============
*/

void CL_MuzzleFlashParticle (vec3_t org, vec3_t angles, qboolean from_client)
{
	int			j;
	particle_t	*p;
	vec3_t		mflashorg, vforward, vright, vup, vec;
	float		rightoffset, upoffset;

	VectorCopy(org, mflashorg);
	for (j=0 ; j<3 ; j++)
	{
		mflashorg[j] = mflashorg[j] + ((rand()%2)-1);

	}

	AngleVectors (angles, vforward, vright, vup);

	if(from_client) 
	{
		if (r_lefthand->value == 1.0F)
			rightoffset = -1.2;
		else
			rightoffset = 1.2;

		upoffset = -1.0;

		if(cl.refdef.fov_x > 90)
		{
			if (r_lefthand->value == 1.0F)
				rightoffset -= (cl.refdef.fov_x - 90.0)/4.0;
			else
				rightoffset -= (cl.refdef.fov_x - 90.0)/25.0;
			upoffset += (cl.refdef.fov_x - 90.0)/10.3;
		}
	
		VectorMA(mflashorg, 24, vforward, mflashorg);
		VectorMA(mflashorg, rightoffset, vright, mflashorg);
		VectorMA(mflashorg, upoffset, vup, mflashorg);
	}
	else
	{
		VectorMA(mflashorg, 60, vforward, mflashorg);
		VectorMA(mflashorg, 14, vup, mflashorg);
	}
	
	if (!(p = new_particle()))
		return;

	p->type = PARTICLE_STANDARD;
	p->image = r_bflashtexture;
	p->scale = 7 + (rand()&3);
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = mflashorg[j];
		p->vel[j] = 0;
	}
	p->accel[0] = p->accel[1] = 0;
	p->accel[2] = 0;
	p->alpha = 0.7;
	p->color = 0xd9; //(255 255 167)
	p->alphavel = -100.0f;

	// Add second sprite for blast coming fowward
	if (!(p = new_particle()))
		return;

	p->type = PARTICLE_BEAM;
	p->image = r_fflashtexture;
	p->scale = 7 + (rand()&3);
	//project a point forward
	VectorNormalize(vforward);
	VectorScale(vforward, 12.0f, vforward);
	VectorAdd(mflashorg, vforward, mflashorg);
	VectorAdd(mflashorg, vforward, vec);
	VectorSet(p->angle, vec[0], vec[1], vec[2]);
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = mflashorg[j];
		p->vel[j] = 0;
	}
	p->accel[0] = p->accel[1] = 0;
	p->accel[2] = 0;
	p->alpha = 0.7;
	p->color = 0xd9; //(255 255 167)
	p->alphavel = -100.0f;	
}

void CL_PlasmaFlashParticle (vec3_t org, vec3_t angles, qboolean from_client)
{
	int			i, j;
	particle_t	*p;
	vec3_t		mflashorg, vforward, vright, vup, vec;
	float		rightoffset, upoffset, color;

	VectorCopy(org, mflashorg);
	for (j=0 ; j<3 ; j++)
	{
		mflashorg[j] = mflashorg[j] + ((rand()%2)-1);

	}

	AngleVectors (angles, vforward, vright, vup);

	if(from_client) 
	{
		if (r_lefthand->value == 1.0F)
			rightoffset = -2.0;
		else
			rightoffset = 2.0;

		upoffset = -0.0;

		if(cl.refdef.fov_x > 90)
		{
			if (r_lefthand->value == 1.0F)
				rightoffset -= (cl.refdef.fov_x - 90.0)/4.0;
			else
				rightoffset -= (cl.refdef.fov_x - 90.0)/25.0;
			upoffset += (cl.refdef.fov_x - 90.0)/10.3;
		}
	
		VectorMA(mflashorg, 24, vforward, mflashorg);
		VectorMA(mflashorg, rightoffset, vright, mflashorg);
		VectorMA(mflashorg, upoffset, vup, mflashorg);
	}
	else
	{
		VectorMA(mflashorg, 60, vforward, mflashorg);
		VectorMA(mflashorg, 14, vup, mflashorg);
	}

	//muzzleflash
	color = getColor();
    for(i = 0; i < 3; i++) {

        if (!(p = new_particle()))
            return;
        p->alpha = 0.4;
        p->alphavel = -100.0f;
        p->blenddst = GL_ONE;
        p->blendsrc = GL_SRC_ALPHA;
        p->image = r_cflashtexture;
        p->scale = 20/(i+1);
        p->type = PARTICLE_STANDARD;
        p->scalevel = 12;
        p->color = color;
        for (j=0 ; j<3 ; j++)
		{
			p->org[j] = mflashorg[j];
			p->vel[j] = 0;
			p->accel[j] = 0;
		}
    }

	// Add second sprite for blast coming fowward
	if (!(p = new_particle()))
		return;

	p->type = PARTICLE_BEAM;
	p->image = r_pflashtexture;
	p->scale = 7 + (rand()&3);
	//project a point forward
	VectorNormalize(vforward);
	VectorScale(vforward, 32.0f, vforward);
	VectorAdd(mflashorg, vforward, mflashorg);
	VectorAdd(mflashorg, vforward, vec);
	VectorSet(p->angle, vec[0], vec[1], vec[2]);
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = mflashorg[j];
		p->vel[j] = 0;
	}
	p->accel[0] = p->accel[1] = 0;
	p->accel[2] = 0;
	p->alpha = 0.7;
	p->color = color; //(255 255 167)
	p->alphavel = -100.0f;	

}

/*
===============
CL_SmartMuzzle
===============
*/
void CL_SmartMuzzle (vec3_t org, vec3_t angles, qboolean from_client)
{
	int			j;
	particle_t	*p;
	vec3_t		mflashorg, vforward, vright, vup;
	float		rightoffset, upoffset;

	VectorCopy(org, mflashorg);
	for (j=0 ; j<3 ; j++)
	{
		mflashorg[j] = mflashorg[j] + ((rand()%2)-1);

	}

	AngleVectors (angles, vforward, vright, vup);

	if(from_client) 
	{
		if (r_lefthand->value == 1.0F)
			rightoffset = -3;
		else
			rightoffset = 3;

		upoffset = -1.0;

		if(cl.refdef.fov_x > 90)
		{
			if (r_lefthand->value == 1.0F)
				rightoffset -= (cl.refdef.fov_x - 90.0)/4.0;
			else
				rightoffset -= (cl.refdef.fov_x - 90.0)/25.0;
			upoffset += (cl.refdef.fov_x - 90.0)/10.3;
		}
	
		VectorMA(mflashorg, 24, vforward, mflashorg);
		VectorMA(mflashorg, rightoffset, vright, mflashorg);
		VectorMA(mflashorg, upoffset, vup, mflashorg);
	}
	else
	{
		VectorMA(mflashorg, 40, vforward, mflashorg);
		VectorMA(mflashorg, 14, vup, mflashorg);
	}
	
	if (!(p = new_particle()))
		return;

	p->type = PARTICLE_STANDARD;
	p->image = r_leaderfieldtexture;
	p->scale = 12 + (rand()&2);
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = mflashorg[j];
		p->vel[j] = 0;
	}
	p->accel[0] = p->accel[1] = 0;
	p->accel[2] = 0;
	p->alpha = 1.0;
	p->color = 0xff; 
	p->alphavel = -100.0f;
}

/*
===============
CL_RocketMuzzle
===============
*/
void CL_RocketMuzzle (vec3_t org, vec3_t angles, qboolean from_client)
{
	int			i, j;
	particle_t	*p;
	vec3_t		mflashorg, vforward, vright, vup, vec;
	float		rightoffset, upoffset;

	VectorCopy(org, mflashorg);
	for (j=0 ; j<3 ; j++)
	{
		mflashorg[j] = mflashorg[j] + ((rand()%2)-1);

	}

	AngleVectors (angles, vforward, vright, vup);

	if(from_client) 
	{
		if (r_lefthand->value == 1.0F)
			rightoffset = -3;
		else
			rightoffset = 3;

		upoffset = -2.0;

		if(cl.refdef.fov_x > 90)
		{
			if (r_lefthand->value == 1.0F)
				rightoffset -= (cl.refdef.fov_x - 90.0)/4.0;
			else
				rightoffset -= (cl.refdef.fov_x - 90.0)/25.0;
			upoffset += (cl.refdef.fov_x - 90.0)/10.3;
		}
	
		VectorMA(mflashorg, 24, vforward, mflashorg);
		VectorMA(mflashorg, rightoffset, vright, mflashorg);
		VectorMA(mflashorg, upoffset, vup, mflashorg);
	}
	else
	{
		VectorMA(mflashorg, 50, vforward, mflashorg);
		VectorMA(mflashorg, 14, vup, mflashorg);
	}
	
	if (!(p = new_particle()))
		return;

	p->type = PARTICLE_STANDARD;
	p->image = r_explosion1texture;
	p->scale = 12 + (rand()&2);
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = mflashorg[j];
		p->vel[j] = 0;
	}
	p->accel[0] = p->accel[1] = 0;
	p->accel[2] = 0;
	p->alpha = 0.6;
	p->color = 0xd9 + (rand()&7);
	p->alphavel = -100.0f;

	//puff of smoke
	for(i = 0; i < 5; i++)
	{
		if (!(p = new_particle()))
		return;

		p->type = PARTICLE_STANDARD;
		p->image = r_smoketexture;
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE_MINUS_SRC_ALPHA;
		p->scale = 1.0f;
		p->color = 15; //(235 235 235)
		p->scalevel = 20.0f;
		VectorCopy(mflashorg, p->org);
		VectorScale(vforward, 5, p->vel);
		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = 5;
		p->alpha = 0.15;
		p->alphavel = -0.2f;
	}

	if (!(p = new_particle()))
		return;

	if(from_client)
	{
		//shoot some flame out of vent
		VectorCopy(org, mflashorg);

		VectorMA(mflashorg, 5, vforward, mflashorg);
		VectorMA(mflashorg, rightoffset * -0.125f, vright, mflashorg);
		VectorMA(mflashorg, upoffset+1.0f, vup, mflashorg);

		p->type = PARTICLE_BEAM;
		p->image = r_fflashtexture;
		p->scale = 0.3f;
		p->scalevel = 0;
		//project a point forward
		VectorScale(vforward, -1.0f, vforward);
		VectorMA(vforward, -0.3, vright, vforward);
		VectorAdd(mflashorg, vforward, vec);
		VectorSet(p->angle, vec[0], vec[1], vec[2]);
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = mflashorg[j];
			p->vel[j] = 0;
		}
		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = 0;
		p->alpha = 0.7;
		p->color = 0xd9; //(255 255 167)
		p->alphavel = -10.0f;	
	}
}

void CL_MEMuzzle (vec3_t org, vec3_t angles, qboolean from_client)
{
	int			i, j;
	particle_t	*p;
	vec3_t		mflashorg, vforward, vright, vup, vec;
	float		rightoffset, upoffset;

	VectorCopy(org, mflashorg);
	for (j=0 ; j<3 ; j++)
	{
		mflashorg[j] = mflashorg[j] + ((rand()%2)-1);

	}

	AngleVectors (angles, vforward, vright, vup);

	if(from_client) 
	{
		if (r_lefthand->value == 1.0F)
			rightoffset = -2.0;
		else
			rightoffset = 2.0;

		upoffset = -0.0;

		if(cl.refdef.fov_x > 90)
		{
			if (r_lefthand->value == 1.0F)
				rightoffset -= (cl.refdef.fov_x - 90.0)/4.0;
			else
				rightoffset -= (cl.refdef.fov_x - 90.0)/25.0;
			upoffset += (cl.refdef.fov_x - 90.0)/10.3;
		}
	
		VectorMA(mflashorg, 24, vforward, mflashorg);
		VectorMA(mflashorg, rightoffset, vright, mflashorg);
		VectorMA(mflashorg, upoffset, vup, mflashorg);
	}
	else
	{
		VectorMA(mflashorg, 70, vforward, mflashorg);
		VectorMA(mflashorg, 14, vup, mflashorg);
	}

	//muzzleflash
    for(i = 0; i < 3; i++) 
	{
        if (!(p = new_particle()))
            return;
        p->alpha = 0.7;
        p->alphavel = -1.0f;
        p->blenddst = GL_ONE;
        p->blendsrc = GL_SRC_ALPHA;
        p->image = r_voltagetexture;
        p->scale = 20/(i+1);
        p->type = PARTICLE_ROTATINGROLL;
        p->scalevel = 18;
        p->color = 0xff;
        for (j=0 ; j<3 ; j++)
		{
			p->org[j] = mflashorg[j];
			p->vel[j] = 0;
			p->accel[j] = 0;
		}
    }

	// Add second sprite for blast coming fowward
	if (!(p = new_particle()))
		return;

	p->type = PARTICLE_BEAM;
	p->image = r_pflashtexture;
	p->scale = 9 + (rand()&3);
	//project a point forward
	VectorNormalize(vforward);
	VectorScale(vforward, 32.0f, vforward);
	VectorAdd(mflashorg, vforward, mflashorg);
	VectorAdd(mflashorg, vforward, vec);
	VectorSet(p->angle, vec[0], vec[1], vec[2]);
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = mflashorg[j];
		p->vel[j] = 0;
	}
	p->accel[0] = p->accel[1] = 0;
	p->accel[2] = 0;
	p->alpha = 0.7;
	p->color = 0xff; 
	p->alphavel = -1.0f;	
}

particle_t *CL_BlueFlameParticle (vec3_t org, vec3_t angles, particle_t *previous)
{
	int			j;
	particle_t	*p;
	vec3_t		mflashorg, vforward, vright, vup;
	float		rightoffset, upoffset;

	VectorCopy(org, mflashorg);
	
	AngleVectors (angles, vforward, vright, vup);

	if (r_lefthand->value == 1.0F)
		rightoffset = -1.1;
	else
		rightoffset = 1.1;

	upoffset = -1.3;

	if(cl.refdef.fov_x > 90)
	{
		if (r_lefthand->value == 1.0F)
			rightoffset -= (cl.refdef.fov_x - 90.0)/4.0;
		else
			rightoffset -= (cl.refdef.fov_x - 90.0)/25.0;
		upoffset += (cl.refdef.fov_x - 90.0)/10.3;
	}
	
	VectorMA(mflashorg, 24, vforward, mflashorg);
	VectorMA(mflashorg, rightoffset, vright, mflashorg);
	VectorMA(mflashorg, upoffset, vup, mflashorg);

	if (!(p = new_particle()))
		return NULL;

	p->type = PARTICLE_CHAINED;
	p->image = r_particletexture;
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	p->scale = .2;
	p->scalevel = 2;

	p->color = 0x74;
	p->colorvel = 40;

	for (j = 0; j < 3; j++)
	{
		p->org[j] = mflashorg[j];
		p->vel[j] = crand();
		p->accel[j] = -p->vel[j];
	}
	
	p->vel[2] += PARTICLE_GRAVITY * 0.1;
	p->accel[2] = PARTICLE_GRAVITY / 2.5f;
	p->alpha = 3.0;

	p->alphavel = -1.0f / (0.1f + frand()*0.15f);
	
	if (previous != NULL)
	{
		vec3_t movement;
		float dist;
		
		VectorSubtract (p->org, previous->org, movement);
		dist = 25.0f * VectorLength (movement) / (p->time - previous->time);
		if (dist < 1.0)
			dist = 1.0;
		p->alpha /= dist;
	}
	
	p->chain_prev = previous;
	
	return p;
}
/*
===============
CL_Deathfield
===============
*/
void CL_Deathfield (vec3_t org, int type)
{
	int			j;
	particle_t	*p;

	if (!(p = new_particle()))
		return;

	p->type = PARTICLE_STANDARD;
	if(type)
		p->image = r_deathfieldtexture2;
	else
		p->image = r_deathfieldtexture;
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = org[j] + ((rand()%2)-1);
		p->vel[j] = 0;
	}
	p->accel[0] = p->accel[1] = 0;
	p->accel[2] = 0;
	p->alpha = 1.5;
	p->scale = 10.0f + (rand()&2);
	p->scalevel = 12;
	p->color = 0x72; // (71 119 139)
	p->accel[0] = p->accel[1] = 0;
	p->accel[2] = PARTICLE_GRAVITY;
	p->alphavel = -1.28f / (2.0f + frand()*0.3f);
}

/*
===============
CL_SayIcon

Displays an icon above a player when talking
===============
*/

void CL_SayIcon(vec3_t org)
{
	particle_t *p;
	int j;

	if(!(p = new_particle()))
		return;

	p->type = PARTICLE_STANDARD;
	p->image = r_sayicontexture;
	p->scale = 5;
	p->scalevel = 0;
	p->color = 15; //(235 235 235)
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE_MINUS_SRC_ALPHA;
	for(j=0; j<3; j++)
	{
		p->org[j] = org[j];
		p->vel[j] = 0;
		p->accel[j] = 0;
	}
	p->org[2]+=40;
	p->alpha = 0.9;

	p->alphavel = -0.5f;
}


/*
===============
CL_DustParticles
===============
*/
void CL_DustParticles (vec3_t org)
{
	int			i, j;
	particle_t	*p;

	for (i=0 ; i<64 ; i++)
	{
		if (!(p = new_particle()))
			return;

		p->type = PARTICLE_STANDARD;
		p->image = r_smoketexture;
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE_MINUS_SRC_ALPHA;
		p->scale = 4 + (rand()&2);
		p->color = 15; //(235 235 235)
		p->scalevel = 1.5;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()%4)-2);
			p->vel[j] = (rand()%88)-44;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 0.5;

		p->alphavel = -0.8f / (0.5f + frand()*0.3f);
	}
}

/*
===============
CL_BigTeleportParticles
===============
*/
void CL_BigTeleportParticles (vec3_t org)
{
	int			i, j;
	particle_t	*p;

	for (i=1 ; i<3 ; i++)
	{
		if (!(p = new_particle()))
			return;

		if(i == 1)
			p->type = PARTICLE_ROTATINGROLL;
		else
			p->type = PARTICLE_STANDARD;
		p->image = r_leaderfieldtexture;
		p->scale = 16*i + (rand()&7);
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE;
		if(i>1)
			p->color = 0xd4; // (95 167 47)
		else
			p->color = 0x74; // (23 83 111)
		p->scalevel = 30;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j];
			p->vel[j] = 0;
		}
		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = 0;
		p->alpha = 0.5;

		p->alphavel = -0.9f / (0.5f + frand()*0.3f);

		addParticleLight (p,
						p->scale*(2+(rand()&5)), 0,
					0, .6, 0.4);
	}
}

/*
===============
CL_BlasterParticles

Wall impact puffs for standard blaster.
===============
*/
void CL_BlasterParticles (vec3_t org, vec3_t dir)
{
	int			i, j;
	particle_t	*p;
	float d;
	for (i=0 ; i<16 ; i++)
	{
		float ifl = (float)i;
		
		if (!(p = new_particle()))
			return;

		p->type = PARTICLE_STANDARD;
		p->image = r_cflashtexture;
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE;
		p->color = 0x74; //(23 83 111)
		p->scale = (0.75f * (ifl + 1.0f)) + (rand()&2);
		p->scalevel = 12;
		d = rand()&7;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j]  +  d*dir[j];
			p->vel[j] = 0;
			p->accel[j] = 0;
		}
		p->alpha = 0.3;

		p->alphavel = -8.8f / ((ifl*2.0f) + frand()*0.3f);
	}

	// add light for the last (most intense) particle
	addParticleLight (p,
			p->scale*25.0f, 50,
		0, .8, 1);
}

/*
===============
Player Lights
===============
*/

void CL_PlayerLight(vec3_t pos, int color)
{
	int			j,i;
	particle_t	*p;

	for(i=1; i<3; i++) {
		if (!(p = new_particle()))
			return;

		VectorClear (p->accel);

		p->alpha = .7;
		p->type = PARTICLE_STANDARD;
		p->image = r_flaretexture;
		p->blendsrc = GL_ONE;
		p->blenddst = GL_ONE;
		p->color = color;
		p->scale = 10*i;
		p->alphavel = INSTANT_PARTICLE;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = pos[j];
			p->vel[j] = 0;
			p->accel[j] = 0;
		}
	}
}

void CL_BlueTeamLight(vec3_t pos)
{
	if (server_is_team)
		CL_PlayerLight (pos, 0x74);
	else
		CL_PlayerLight (pos, 0xd3);
}

void CL_RedTeamLight(vec3_t pos)
{
	CL_PlayerLight (pos, 0xe8);
}

void CL_FlagEffects(vec3_t pos, qboolean team)
{

	int			i;
	particle_t	*p;

	if (!(p = new_particle()))
		return;

	VectorClear (p->accel);

	p->alpha = 1.0;
	p->type = PARTICLE_ROTATINGYAW;
	p->image = r_flagtexture;
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	if(team)
		p->color = 0x74; //(23 83 111)
	else
		p->color = 0xe8; //(155 31 0)
	p->scale = 15;
	p->alphavel = INSTANT_PARTICLE;
	for (i=0 ; i<3 ; i++)
	{
		p->org[i] = pos[i];
		p->vel[i] = 0.0f;
		p->accel[i] = 0.0f;
	}
	p->org[2] += 64.0f;

	if (!(p = new_particle()))
			return;

	p->alpha = 1.0;
	p->type = PARTICLE_ROTATINGROLL;
	p->image = r_leaderfieldtexture;
	p->scale = 24.0f;
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	if(team)
		p->color = 0x74 + 2*crand(); //(23 83 111)
	else
		p->color = 0xe8 + 2*crand(); //(155 31 0)

	p->scale = 20 + crand();
	p->alphavel = INSTANT_PARTICLE;
	for (i=0 ; i<3 ; i++)
	{
		p->org[i] = pos[i];
		p->vel[i] = 0.0f;
		p->accel[i] = 0.0f;
	}
	p->org[2] += 64.0f;
}

/*
===============
CL_DiminishingTrail
===============
*/
void CL_DiminishingTrail (vec3_t start, vec3_t end, centity_t *old, int flags)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	particle_t	*p;
	float		dec;
	float		orgscale;
	float		velscale;

	if (((flags & EF_GIB) || (flags & EF_GREENGIB)) && cl_noblood->value)
		return;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 2.5;
	VectorScale (vec, dec, vec);

	if (old->trailcount > 900)
	{
		orgscale = 4;
		velscale = 15;
	}
	else if (old->trailcount > 800)
	{
		orgscale = 2;
		velscale = 10;
	}
	else
	{
		orgscale = 1;
		velscale = 5;
	}

	// add stains if moving
	if ( len && !cl_noblood->value ) {
		if ( flags & EF_GIB ) {
			CL_BloodSplatter(start, end, 0xe8, 0);
		} else if ( flags & EF_GREENGIB ) {
			CL_BloodSplatter(start, end, 0xd0, 1);
		}
	}

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
			return;

		// drop less particles as it flies
		if ((rand()&1023) < old->trailcount)
		{
			if (!(p = new_particle()))
				return;

			if (flags & EF_GIB)
			{
				p->alpha = .3;//1.0;
				p->alphavel = -1.0f / (2.0f + frand()*0.4f);
				p->type = PARTICLE_STANDARD;
				p->image = r_blood3texture;
				p->blendsrc = GL_SRC_ALPHA;
				p->blenddst = GL_ONE_MINUS_SRC_ALPHA;
				p->color = 0xe8; //(155 31 0)
				p->scale = 10;
				p->scalevel = 1.5;
				for (j=0 ; j<3 ; j++)
				{
					p->org[j] = move[j] + crand()*orgscale;
					p->vel[j] = crand()*velscale;
					p->accel[j] = 0;
				}
				p->vel[2] -= (float)PARTICLE_GRAVITY;
			}
			else if (flags & EF_GREENGIB)
			{
				p->alpha = .3;
				p->alphavel = -1.0f / (2.0f + frand()*0.4f);
				p->type = PARTICLE_STANDARD;
				p->image = r_blood3texture;
				p->blendsrc = GL_SRC_ALPHA;
				p->blenddst = GL_ONE;
				p->color = 0xd0+ (rand()&3); //(0-83 255-187 0-39)
				p->scale = 10;
				p->scalevel = 1.5;
				for (j=0; j< 3; j++)
				{
					p->org[j] = move[j] + crand()*orgscale;
					p->vel[j] = crand()*velscale;
					p->accel[j] = 0;
				}
				p->vel[2] -= (float)PARTICLE_GRAVITY;
			}
			else
			{
				dec = 10; //less smoke
				for (j=0 ; j<3 ; j++)
				{
					p->org[j] = move[j] + crand()*orgscale;
					p->vel[j] = crand()*velscale;
				}

				p->alphavel = -1.0f / (10.0f + frand()*1.4f); //smoke lingers longer
				p->alpha = .07;
				p->type = PARTICLE_STANDARD;
				p->image = r_smoketexture;
				p->blendsrc = GL_SRC_ALPHA;
				p->blenddst = GL_ONE_MINUS_SRC_ALPHA;
				p->scale = 1 + (rand()&4);
				p->scalevel = 15.0;
				p->color = 15; //(235 235 235)
				p->accel[2] = 20;
			}
		}

		old->trailcount -= 5;
		if (old->trailcount < 100)
			old->trailcount = 100;
		VectorAdd (move, vec, move);
	}
}

void MakeNormalVectors (vec3_t forward, vec3_t right, vec3_t up)
{
	float		d;

	// this rotate and negat guarantees a vector
	// not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	d = DotProduct (right, forward);
	VectorMA (right, -d, forward, right);
	VectorNormalize (right);
	CrossProduct (right, forward, up);
}

/*
===============
CL_RocketTrail
===============
*/
void CL_RocketTrail (vec3_t start, vec3_t end, centity_t *old)
{
	// smoke
	CL_DiminishingTrail (start, end, old, EF_ROCKET);

}

void CL_BlasterTrail (vec3_t start, vec3_t end, float color)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	particle_t	*p;
	float		dec;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	//rotating more solid leading particle at start
	if (!(p = new_particle()))
		return;

	p->alpha = 0.9;
	p->alphavel = INSTANT_PARTICLE;
	p->type = PARTICLE_ROTATINGROLL;
	p->image = r_shottexture;
	p->scale = 8;
	p->angle[1] = cl.refdef.viewangles[0];
	p->angle[0] = sinf(len);
	p->angle[2] = cl.refdef.viewangles[2];
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	p->color = color + (rand()&2);

	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = move[j];
		p->vel[j] = 0;
		p->accel[j] = 0;
	}
	
	//now make the fading trail
	dec = 20;
	VectorScale (vec, dec, vec);

	while (len > 0)
	{
		len -= dec;

		if ( (rand()&6) == 0)
		{
			if (!(p = new_particle()))
				return;

			VectorClear (p->accel);
			p->color = color + (rand()&2); // (0 255 0), (63 211 27)
			p->scale = 2 + (rand()&7);
			p->scalevel = 5;
			p->alpha = .3;
			p->alphavel = -1.0f / (3.0f + frand()*0.2f);
			p->type = PARTICLE_STANDARD;
			p->image = r_cflashtexture;
			p->blendsrc = GL_SRC_ALPHA;
			p->blenddst = GL_ONE;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = move[j] + crand()*2;
				p->vel[j] = crand()*2;
			}
			p->accel[2] = 0;
		}
		VectorAdd (move, vec, move);

	}
}

void CL_ShipExhaust (vec3_t start, vec3_t end, centity_t *old)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	particle_t	*p;
	float		dec;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 3;
	VectorScale (vec, dec, vec);

	while (len > 0)
	{
		len -= dec;

		if ( (rand()&6) == 0)
		{
			if (!(p = new_particle()))
				return;

			VectorClear (p->accel);
			p->color = 0xc0 + (rand()&2);
			p->scale = 2 + (rand()&7);
			p->scalevel = 5;
			p->alpha = .3;
			p->alphavel = -1.0f / (3.0f + frand()*0.2f);
			p->type = PARTICLE_STANDARD;
			p->image = r_explosiontexture;
			p->blendsrc = GL_SRC_ALPHA;
			p->blenddst = GL_ONE;
			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = move[j] + crand()*2;
				p->vel[j] = crand()*2;
			}
			p->accel[2] = 0;
		}
		VectorAdd (move, vec, move);

	}
}

void CL_RocketExhaust (vec3_t start, vec3_t end, centity_t *old)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	particle_t	*p;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	VectorScale (vec, 5, vec);

	if (!(p = new_particle()))
		return;

	VectorClear (p->accel);

	p->alpha = .3;
	p->alphavel = INSTANT_PARTICLE;
	p->type = PARTICLE_ROTATINGROLL;
	p->image = r_bflashtexture;
	p->scale = 5;
	p->angle[0] = cl.refdef.viewangles[0];
	p->angle[1] = cl.refdef.viewangles[1];
	p->angle[2] = cl.refdef.viewangles[2];

	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	p->color = 0xc0 + (rand()&2);
	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = move[j];
		p->vel[j] = 0;
		p->accel[j] = 0;
	}

	if (!(p = new_particle()))
		return;

	VectorClear (p->accel);

	p->alpha = .7;
	p->alphavel = INSTANT_PARTICLE;
	p->type = PARTICLE_STANDARD;
	p->image = r_bflashtexture;
	p->scale = 5;
	p->angle[0] = cl.refdef.viewangles[0];
	p->angle[1] = cl.refdef.viewangles[1];
	p->angle[2] = cl.refdef.viewangles[2];

	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	p->color = 0xc0 + (rand()&2);
	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = move[j];
		p->vel[j] = 0;
		p->accel[j] = 0;
	}

	if (!(p = new_particle()))
		return;

	VectorClear (p->accel);

	p->alpha = .7;
	p->alphavel = INSTANT_PARTICLE;
	p->type = PARTICLE_STANDARD;
	p->image = r_flaretexture;
	p->scale = 10;
	p->angle[0] = cl.refdef.viewangles[0];
	p->angle[1] = cl.refdef.viewangles[1];
	p->angle[2] = cl.refdef.viewangles[2];

	p->blendsrc = GL_ONE;
	p->blenddst = GL_ONE;
	p->color = 0xc0 + (rand()&2);
	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = move[j];
		p->vel[j] = 0;
		p->accel[j] = 0;
	}

	VectorAdd (move, vec, move);
}

/*
===============
Wall Impacts
===============
*/

void CL_BulletMarks(vec3_t org, vec3_t dir){
	particle_t *p;
	vec3_t		v;
	int			j;

	if(!(p = new_particle()))
		return;

	p->image = r_bulletnormal;
	p->color = 0 + (rand() & 1);
	p->type = PARTICLE_RAISEDDECAL;
	p->blendsrc = GL_DST_COLOR;
	p->blenddst = GL_SRC_COLOR;

	VectorScale(dir, -1, v);
	RotateForNormal(v, p->angle);
	p->angle[ROLL] = rand() % 360;
	VectorAdd(org, dir, p->org);

	p->scale = .5;
	p->alpha = 0.5;
	p->alphavel = -0.2f / (2.0f + frand() * 0.3f);
	for (j=0 ; j<3 ; j++)
	{
		p->accel[j] = 0;
		p->vel[j] = 0;
	}
}
void CL_BeamgunMark(vec3_t org, vec3_t dir, float dur, qboolean isDis){
	particle_t *p;
	vec3_t		v;
	int			j;
	float		color;
	vec3_t		colorvec;

	if(isDis)
		color = getColor();
	else
		color = 0xd4;

	if(!(p = new_particle()))
		return;

	p->image = r_bullettexture;
	p->color = color + (rand() & 1);
	p->type = PARTICLE_DECAL;
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	p->scale = .75;
	p->scalevel = 0;

	VectorScale(dir, -1, v);
	RotateForNormal(v, p->angle);
	p->angle[ROLL] = rand() % 360;
	VectorAdd(org, dir, p->org);

	p->alpha = 0.5;
	p->alphavel = -dur / (2.0f + frand() * 0.3f);
	for (j=0 ; j<3 ; j++)
	{
			p->accel[j] = 0;
			p->vel[j] = 0;
	}
	if(!(p = new_particle()))
		return;

	p->image = r_bullettexture;
	p->color = color + (rand() & 1);
	p->type = PARTICLE_DECAL;
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	p->scale = 1.5;
	p->scalevel = 0;

	VectorScale(dir, -1, v);
	RotateForNormal(v, p->angle);
	p->angle[ROLL] = rand() % 360;
	VectorAdd(org, dir, p->org);

	p->alpha = 0.5;
	p->alphavel = -dur / (2.0f + frand() * 0.3f);
	for (j=0 ; j<3 ; j++)
	{
			p->accel[j] = 0;
			p->vel[j] = 0;
	}

	//add small shockwave effect
	if(!(p = new_particle()))
		return;

	p->image = r_hittexture;
	p->color = color + (rand() & 1);
	p->type = PARTICLE_DECAL;
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	p->scale = .1;
	p->scalevel = 10;

	VectorScale(dir, -1, v);
	RotateForNormal(v, p->angle);
	p->angle[ROLL] = rand() % 360;
	VectorAdd(org, dir, p->org);

	p->alpha = 1.0;
	p->alphavel = -2.0f;
	for (j=0 ; j<3 ; j++)
	{
			p->accel[j] = 0;
			p->vel[j] = 0;
	}

	if(isDis)
		getColorvec(colorvec);
	else {
		colorvec[0] = 0; //bright green
		colorvec[1] = 1;
		colorvec[2] = 0.2;
	}

	addParticleLight (p,
			isDis ? 120:80, 5,
			colorvec[0], colorvec[1], colorvec[2]);
}

void CL_BlasterMark(vec3_t org, vec3_t dir){
	particle_t *p;
	vec3_t		v;
	int			j;

	if(!(p = new_particle()))
		return;

	p->image = r_bullettexture;
	p->color = 0x74 + (rand() & 1);
	p->type = PARTICLE_DECAL;
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	p->scale = .75;
	p->scalevel = 0;

	VectorScale(dir, -1, v);
	RotateForNormal(v, p->angle);
	p->angle[ROLL] = rand() % 360;
	VectorAdd(org, dir, p->org);

	p->alpha = 0.7;
	p->alphavel = -0.4f / (2.0f + frand() * 0.3f);
	for (j=0 ; j<3 ; j++)
	{
			p->accel[j] = 0;
			p->vel[j] = 0;
	}
	if(!(p = new_particle()))
		return;

	p->image = r_bullettexture;
	p->color = 0x74 + (rand() & 1);
	p->type = PARTICLE_DECAL;
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	p->scale = 1.5;
	p->scalevel = 0;

	VectorScale(dir, -1, v);
	RotateForNormal(v, p->angle);
	p->angle[ROLL] = rand() % 360;
	VectorAdd(org, dir, p->org);

	p->alpha = 0.5;
	p->alphavel = -0.4f / (2.0f + frand() * 0.3f);
	for (j=0 ; j<3 ; j++)
	{
			p->accel[j] = 0;
			p->vel[j] = 0;
	}
}

void CL_VaporizerMarks(vec3_t org, vec3_t dir){
	particle_t *p;
	vec3_t		v, forward, right, up;
	int			i,j;
	float		scatter;

	for(i = 0; i < 6; i ++) {
		if(!(p = new_particle()))
			return;

		p->image = r_bullettexture;
		p->color = 0xd4 + (rand()&7);
		p->type = PARTICLE_DECAL;
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE;
		p->scale = .75;
		p->scalevel = 0;

		VectorScale(dir, -1, v);
		RotateForNormal(v, p->angle);
		p->angle[ROLL] = rand() % 360;
		VectorAdd(org, dir, p->org);

		AngleVectors(p->angle, forward, right, up);

		scatter = ((rand()%8)-4);
		VectorMA(p->org, scatter, up, p->org);

		p->alpha = 0.7;
		p->alphavel = -0.4f / (2.0f + frand() * 0.3f);
		for (j=0 ; j<3 ; j++)
		{
				p->accel[j] = 0;
				p->vel[j] = 0;
		}
		if(!(p = new_particle()))
			return;

		p->image = r_bullettexture;
		p->color = 0x74 + (rand()&7);
		p->type = PARTICLE_DECAL;
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE;
		p->scale = 1.5;
		p->scalevel = 0;

		VectorScale(dir, -1, v);
		RotateForNormal(v, p->angle);
		p->angle[ROLL] = rand() % 360;
		VectorAdd(org, dir, p->org);

		VectorMA(p->org, scatter, up, p->org);

		p->alpha = 0.5;
		p->alphavel = -0.4f / (2.0f + frand() * 0.3f);
		for (j=0 ; j<3 ; j++)
		{
				p->accel[j] = 0;
				p->vel[j] = 0;
		}
	}
}

/*
===============
Particle Beams
===============
*/

//this is the length of each piece...
#define RAILTRAILSPACE 20
#define LASERTRAILSPACE 10

void CL_DisruptorBeam (vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec, point, last, vec2;
	float		len, color;
	vec3_t		right, up;
	particle_t	*p;
	int			i,j;
	float		v;

	color = getColor();

	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);
	VectorCopy (vec, point);

	MakeNormalVectors (vec, right, up);
	VectorCopy (vec, vec2);
	VectorScale (vec, RAILTRAILSPACE, vec);
	VectorCopy (start, move);

	for (i = 0; len>0; len -= RAILTRAILSPACE, i+=100)
	{
		VectorCopy (move, last);
		VectorAdd (move, vec, move);

		if (!(p = new_particle()))
				return;

		p->alpha = 1;
		p->alphavel = -1.0f - (len/(float)i);
		p->blenddst = GL_ONE;
		p->blendsrc = GL_SRC_ALPHA;

		v = frand();
		if(v < 0.2)
			p->image = r_dis3texture;
		else if(v < 0.5)
			p->image = r_dis2texture;
		else
			p->image = r_dis1texture;

		p->scale = 4;
		VectorCopy(move, p->angle);
		p->type = PARTICLE_BEAM;
		p->scalevel = 0;

		p->color = color;

		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = last[j];
			p->vel[j] = 0;
			p->accel[j] = 0;
		}
	}
}

void CL_LaserBeam (vec3_t start, vec3_t end, float color, qboolean use_start)
{
	vec3_t		move;
	vec3_t		vec, last;
	float		len;
	particle_t	*p;
	int			i,j;

	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	VectorScale (vec, LASERTRAILSPACE, vec);
	VectorCopy (start, move);

	for (i = 0; len>0; len -= LASERTRAILSPACE, i++)
	{
		VectorCopy (move, last);
		VectorAdd (move, vec, move);

		if (!(p = new_particle()))
				return;

		p->alpha = 1;
		p->alphavel = -2.0f;
		p->blenddst = GL_ONE;
		p->blendsrc = GL_SRC_ALPHA;

		if(i == 0 && use_start)
			p->image = r_beamstart;
		else
			p->image = r_beam2texture;
		p->scale = 2;

		VectorCopy(move, p->angle);
		p->type = PARTICLE_BEAM;
		p->scalevel = 0;

		p->color = color;

		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = last[j];
			p->vel[j] = 0;
			p->accel[j] = 0;
		}
	}
}

#define VAPORIZERTRAILSPACE 20

void CL_VaporizerBeam (vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec, point, last, vec2;
	float		len;
	vec3_t		right, up;
	particle_t	*p;
	int			i,j;

	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);
	VectorCopy (vec, point);

	MakeNormalVectors (vec, right, up);
	VectorCopy (vec, vec2);
	VectorScale (vec, VAPORIZERTRAILSPACE, vec);
	VectorCopy (start, move);

	//muzzleflash
	VectorScale (vec2, -VAPORIZERTRAILSPACE/2, vec2);
	VectorAdd(start, vec2, start);
	for(i = 0; i < 8; i++) {

		if (!(p = new_particle()))
			return;
		p->alpha = 0.9;
		p->alphavel = -0.8f / (0.6f + frand()*0.2f);
		p->blenddst = GL_ONE;
		p->blendsrc = GL_SRC_ALPHA;
		p->image = r_leaderfieldtexture;
		p->scale = 24/(i+1);
		for(j=0; j< 3; j++)
			p->angle[j] = 0;
		p->type = PARTICLE_STANDARD;
		p->scalevel = 12;
		p->color = 0x74 + (rand()&12);;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = start[j];
			p->vel[j] = 0;
			p->accel[j] = 0;
		}
	}

	for (; len>0; len -= VAPORIZERTRAILSPACE)
	{
		VectorCopy (move, last);
		VectorAdd (move, vec, move);

		for(i = 0; i < 3; i++) {
			if (!(p = new_particle()))
					return;

			p->alpha = 1;
			p->alphavel = -0.8f / (0.6f + frand()*0.2f);
			p->blenddst = GL_ONE;
			p->blendsrc = GL_SRC_ALPHA;
			p->image = r_beam2texture;
			p->scale = 4;
			VectorCopy(move, p->angle);
			p->type = PARTICLE_BEAM;
			p->scalevel = 0;

			p->color = 0x74 + (rand()&7);

			for (j=0 ; j<3 ; j++)
			{
				p->org[j] = last[j];
				p->vel[j] = 0;
				p->accel[j] = 0;
			}
		}
		//do lightning effects
		if (!(p = new_particle()))
				return;

		p->alpha = 1;
		p->alphavel = -0.8f / (0.6f + frand()*0.2f);
		p->blenddst = GL_ONE;
		p->blendsrc = GL_SRC_ALPHA;
		p->image = r_beam3texture;
		p->scale = 4;
		VectorCopy(move, p->angle);
		p->type = PARTICLE_BEAM;
		p->scalevel = 0;

		p->color = 0x74;

		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = last[j];
			p->vel[j] = 0;
			p->accel[j] = 0;
		}

	}

}

void CL_NewLightning (vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	particle_t	*p;
	particle_t	*pr = NULL;
	float		dec;
	vec3_t		right, up;
	int			i;
	float		skewx, skewy, skewz;
	float		x, y, z;
	// byte		clr = 0xd4;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	MakeNormalVectors (vec, right, up);

	dec = .75;
	VectorScale (vec, dec, vec);
	VectorCopy (start, move);
	skewx = skewy = skewz = .1;
	i = 0;
	x = y = z = 0;

	while (len > 0)
	{
		len -= dec;
		i++;

		if (!(p = new_particle()))
			return;

		VectorClear (p->accel);

		x++;
		y++;
		z++;

		// somehow increment x,y,z in random direction
		if (i > 10)//reset
		{
			i = 0;
			x = y = z = 0;
			if (frand() < 0.5)
				skewx = skewx * -1;
			if (frand() < 0.5)
				skewy = skewy * -1;
			if (frand() < 0.5)
				skewz = skewz * -1;
		}

		p->org[0] = move[0] + skewx * ++x;
		p->org[1] = move[1] + skewy * ++y;
		p->org[2] = move[2] + skewz * ++z;

		p->alpha = 1.2;
		p->alphavel = -3.33f;
		p->type = PARTICLE_CHAINED;
		p->image = r_raintexture;
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE;
		p->scale = 1 + (rand()&2);
		p->scalevel = 0;
		p->color = 0xff;
		for (j=0 ; j<3 ; j++)
		{
			p->vel[j] = 0;
			p->accel[j] = 0;
		}
		if (len < 4 && len + dec >= 4) // add only one, 4 units from end
			addParticleLight (p,
						p->scale*75.0f, 0,
					.25, 0, .3);
		VectorAdd (p->org, vec, move);
		if (pr) 
		{
			p->chain_prev = pr;
		}
		pr = p;
	}

}

/*
===============
CL_BubbleTrail
===============
*/
void CL_BubbleTrail (vec3_t start, vec3_t end)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			i, j;
	particle_t	*p;
	float		dec;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	dec = 32;
	VectorScale (vec, dec, vec);

	for (i=0 ; i<len ; i+=dec)
	{
		if (!(p = new_particle()))
			return;

		VectorClear (p->accel);

		p->alpha = 1.0;
		p->alphavel = -1.0f / (1.0f + frand()*0.2f);
		p->color = 4 + (rand()&7);
		p->type = PARTICLE_STANDARD;
		p->image = r_bubbletexture;
		p->scale = 2 + (rand()&2);
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE_MINUS_SRC_ALPHA;

		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = move[j] + crand()*2;
			p->vel[j] = crand()*5;
		}
		p->vel[2] += 6;

		VectorAdd (move, vec, move);
	}
}

/*
===============
CL_BFGExplosionParticles
===============
*/

void CL_BFGExplosionParticles (vec3_t org)
{
	int			i, j;
	particle_t	*p;

	//for glsl framebuffer distortion effects
	if(!r_drawing_fbeffect && cl_explosiondist->value) {
		r_fbFxType = 1; //EXPLOSION
		r_drawing_fbeffect = true;
		VectorCopy(org, r_explosionOrigin);
		r_fbeffectTime = rs_realtime;
	}

	for (i=0 ; i<32 ; i++)
	{
		if (!(p = new_particle()))
			return;

		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = org[j] + ((rand()%62)-32);
			p->vel[j] = (rand()%32)-16;
		}

		p->accel[0] = p->accel[1] = 5+(rand()&7);
		p->accel[2] = 5+(rand()&7);
		p->scale = (24 + (rand()&2));
		p->scalevel = 24;
		p->type = PARTICLE_STANDARD;
		p->image = r_explosiontexture;
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE;
		p->color = 0x74 + (rand()&7);
		p->alpha = 0.4;

		p->alphavel = -0.8f / (2.5f + frand()*0.3f);
		if (i > 28)
			addParticleLight (p,
						p->scale*(15+(rand()&5)), 10,
					0, 0.6, 0.5);
	}

	//place a big shock wave effect
	if (!(p = new_particle()))
			return;
	p->alpha = 1.0;
	p->alphavel = -1.0f;
	p->type = PARTICLE_FLAT;
	p->image = r_hittexture;
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	p->color = 0x74 + (rand()&7);
	p->scale = 24 + (rand()&4) ;
	p->scalevel = 100;
	for(j = 0; j < 3; j++) {
		p->org[j] = org[j];
		p->vel[j] = 0;
		p->accel[j] = 0;
	}
}

/*
===============
CL_TeleportParticles
===============
*/
void CL_TeleportParticles (vec3_t orig_start)
{
	vec3_t		move;
	vec3_t		vec;
	vec3_t		start;
	vec3_t		end;
	float		len;
	int			j;
	particle_t	*p;
	int			i;
	vec3_t		pos1, pos2, v;
	float		step = 16.0;
	// static vec3_t mins = { -1, -1, -1 };
    // static vec3_t maxs = { 1, 1, 1 };

	//make a copy to prevent modifying an important vector
	VectorCopy(orig_start, start); 
	VectorCopy(start, pos1);
	pos2[2] = 1;
	pos2[0] = pos2[1] = 0;
	pos1[2] -= 32;

	if(!(p = new_particle()))
		return;

	p->image = r_logotexture;
	p->color = 0x74 + (rand()&7);
	p->type = PARTICLE_DECAL;
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	p->scale = 4;
	p->scalevel = 0;

	VectorScale(pos2, -1, v);
	RotateForNormal(v, p->angle);
	p->angle[ROLL] = rand() % 360;
	VectorCopy( pos1, p->org);

	p->alpha = 0.7;
	p->alphavel = -0.15f;
	for (j=0 ; j<3 ; j++)
	{
		p->accel[j] = 0;
		p->vel[j] = 0;
	}

	//place a big shock wave effect
	if (!(p = new_particle()))
			return;

	p->alpha = 1.0;
	p->alphavel = -0.250f;
	p->type = PARTICLE_DECAL;
	p->image = r_explosion5texture;
	p->blendsrc = GL_SRC_ALPHA;
	p->blenddst = GL_ONE;
	p->color = 0x74 + (rand()&7);
	p->scale = 1 + (rand()&4) ;
	p->scalevel = 4;
	VectorScale(pos2, -1, v);
	RotateForNormal(v, p->angle);
	p->angle[ROLL] = rand() % 360;
	VectorCopy( pos1, p->org);
	for(j = 0; j < 3; j++) {
		p->vel[j] = 0;
		p->accel[j] = 0;
	}

	VectorCopy (start, move);
	VectorCopy (start, end);
	end[2] += 32;
	start[2] -= 48;
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	for (i=0 ; i<len ; i+=step)
	{

		if (!(p = new_particle()))
			return;

		VectorClear (p->accel);


		p->alpha = 1.0;
		p->alphavel = -2.5f / (2.0f + frand()*0.2f);
		p->type = PARTICLE_FLAT;
		p->image = r_hittexture;
		p->blendsrc = GL_ONE;
		p->blenddst = GL_ONE;
		p->color = 0x74 + (rand()&7);
		p->scale = 24 + (rand()&4) ;
		p->scalevel = 0;
		for(j = 0; j < 3; j++) {
			p->org[j] = start[j];
			p->vel[j] = 0;
		}
		p->vel[2] = (float)PARTICLE_GRAVITY * 2.0f;
		if (i < 1)
		addParticleLight (p,
					p->scale*10, 10,
				.3, .4, .7);

		start[2] += 16.0f;
	}
}

/*
Cl_WeatherEffects - originally adopted from Jay Dolan's Q2W
*/
void Cl_WeatherEffects()
{
	int i, j, k;
	vec3_t start, end;
	trace_t tr;
	float ceiling;
	particle_t *p;

	if(!r_weather)
		return;

	i = weather_particles;  // we count up from current particles
	p = NULL;  // so that we add the right amount

	k = 0;

	if(r_weather == 5) 
	{
		while(i++ < WEATHER_PARTICLES && k++ < 1)
		{
			VectorCopy(cl.refdef.vieworg, start);
			start[0] = start[0] + (rand() % 5096) - 2048;
			start[1] = start[1] + (rand() % 5096) - 2048;

			VectorCopy(start, end);
			end[2] += 8192;

			// trace up looking for sky
			tr = CM_BoxTrace(start, end, vec3_origin, vec3_origin, 0, MASK_SHOT);

			if(!(tr.surface->flags & SURF_SKY))
				continue;

			// drop down somewhere between sky and player
			ceiling = tr.endpos[2] > start[2] + 1024 ? start[2] + 1024 : tr.endpos[2];
			tr.endpos[2] = tr.endpos[2] - ((ceiling - start[2]) * frand());

			if (!(p = new_particle()))
				return;

			VectorCopy(tr.endpos, p->org);
			p->org[2] -= 1;

			VectorCopy(start, end);
			end[2] -= 8192;

			tr = CM_BoxTrace(p->org, end, vec3_origin, vec3_origin, 0, MASK_ALL);

			if(!tr.surface)  // this shouldn't happen
				VectorCopy(start, p->end);
			else
				VectorCopy(tr.endpos, p->end);

			p->type = PARTICLE_FLUTTERWEATHER;
			// setup the particles
			//trash
			p->image = r_trashtexture;
			p->vel[2] = -80;
			p->accel[2] = 0;
			p->alpha = 0.9;
			p->alphavel = 0.0f;
			p->color = 8;
			p->scale = 12;
			p->scalevel = 0;
			p->blendsrc = GL_SRC_ALPHA;
			p->blenddst = GL_ONE_MINUS_SRC_ALPHA;
			
			for(j = 0; j < 2; j++)
			{
				p->vel[j] = crand() * 40;
				p->accel[j] = crand() * 50;
			}
			weather_particles++;
		}		
	}
	else if(r_weather == 3 || r_weather == 4) 
	{
		while(i++ < WEATHER_PARTICLES && k++ < 2)
		{
			VectorCopy(cl.refdef.vieworg, start);
			start[0] = start[0] + (rand() % 2048) - 1024;
			start[1] = start[1] + (rand() % 2048) - 1024;

			VectorCopy(start, end);
			end[2] += 8192;

			// trace up looking for sky
			tr = CM_BoxTrace(start, end, vec3_origin, vec3_origin, 0, MASK_SHOT);

			if(!(tr.surface->flags & SURF_SKY))
				continue;

			// drop down somewhere between sky and player
			ceiling = tr.endpos[2] > start[2] + 1024 ? start[2] + 1024 : tr.endpos[2];
			tr.endpos[2] = tr.endpos[2] - ((ceiling - start[2]) * frand());

			if (!(p = new_particle()))
				return;

			VectorCopy(tr.endpos, p->org);
			p->org[2] -= 1;

			VectorCopy(start, end);
			end[2] -= 8192;

			tr = CM_BoxTrace(p->org, end, vec3_origin, vec3_origin, 0, MASK_ALL);

			if(!tr.surface)  // this shouldn't happen
				VectorCopy(start, p->end);
			else
				VectorCopy(tr.endpos, p->end);

			// setup the particles
			if(r_weather == 3) //leaves
			{
				p->type = PARTICLE_FLUTTERWEATHER;
				p->image = r_leaftexture;
				p->vel[2] = -80;
				p->accel[2] = 0;
				p->alpha = 0.9;
				p->alphavel = 0.0f;
				p->color = 8;
				p->scale = 2;
				p->scalevel = 0;
				p->blendsrc = GL_SRC_ALPHA;
				p->blenddst = GL_ONE_MINUS_SRC_ALPHA;
			}
			else if(r_weather == 4) //embers
			{
				p->type = PARTICLE_WEATHER;
				p->image = r_particletexture;
				p->vel[2] = -80;
				p->accel[2] = 0;
				p->alpha = 0.9;
				p->alphavel = frand() * -1.0f;
				p->color = 0xe8;
				p->scale = 2;
				p->scalevel = 0;
				p->blendsrc = GL_SRC_ALPHA;
				p->blenddst = GL_ONE;
			}	
			for(j = 0; j < 2; j++)
			{
				p->vel[j] = crand() * 25;
				p->accel[j] = crand() * 50;
			}
			weather_particles++;
		}
	}
	else 
	{
		while(i++ < WEATHER_PARTICLES && k++ < 25)
		{
			VectorCopy(cl.refdef.vieworg, start);
			start[0] = start[0] + (rand() % 2048) - 1024;
			start[1] = start[1] + (rand() % 2048) - 1024;

			VectorCopy(start, end);
			end[2] += 8192;

			// trace up looking for sky
			tr = CM_BoxTrace(start, end, vec3_origin, vec3_origin, 0, MASK_SHOT);

			if(!(tr.surface->flags & SURF_SKY))
				continue;

			// drop down somewhere between sky and player
			ceiling = tr.endpos[2] > start[2] + 1024 ? start[2] + 1024 : tr.endpos[2];
			tr.endpos[2] = tr.endpos[2] - ((ceiling - start[2]) * frand());

			if (!(p = new_particle()))
				return;

			VectorCopy(tr.endpos, p->org);
			p->org[2] -= 1;

			VectorCopy(start, end);
			end[2] -= 8192;

			tr = CM_BoxTrace(p->org, end, vec3_origin, vec3_origin, 0, MASK_ALL);

			if(!tr.surface)  // this shouldn't happen
				VectorCopy(start, p->end);
			else
				VectorCopy(tr.endpos, p->end);

			p->type = PARTICLE_WEATHER;
			// setup the particles
			if(r_weather == 1) //rain
			{
				p->image = r_raintexture;
				p->vel[2] = -800;
				p->accel[2] = 0;
				p->alpha = 0.3;
				p->alphavel = frand() * -1.0f;
				p->color = 8;
				p->scale = 6;
				p->scalevel = 0;
			}
			else if(r_weather == 2) //snow
			{
				p->image = r_particletexture;
				p->vel[2] = -120;
				p->accel[2] = 0;
				p->alpha = 0.8;
				p->alphavel = frand() * -1.0f;
				p->color = 8;
				p->scale = 1;
				p->scalevel = 0;
			}
			p->blendsrc = GL_SRC_ALPHA;
			p->blenddst = GL_ONE;

			for(j = 0; j < 2; j++)
			{
				p->vel[j] = crand() * 2;
				p->accel[j] = crand() * 2;
			}
			weather_particles++;
		}
	}
}

/*
Powered up effects
*/

#define	BEAMLENGTH			16.0f
void CL_PoweredEffects (vec3_t origin, unsigned int nEffect)
{
	int			i,j;
	particle_t	*p;
	float		angle;
	float		sr, sp, sy, cr, cp, cy;
	vec3_t		forward;
	float		dist = 64;
	float		ltime;
	
	if (!avelocities[0][0])
	{
		for (i=0 ; i<NUMVERTEXNORMALS ; ++i)
			for (j=0; j<3; ++j)
				avelocities[i][j] = (rand()&255) * 0.01f;
	}

	ltime = (float)cl.time / 1000.0f;
	for (i=0 ; i<32 ; i+=2)
	{
		angle = ltime * avelocities[i][0];
		sy = sinf(angle);
		cy = cosf(angle);
		angle = ltime * avelocities[i][1];
		sp = sinf(angle);
		cp = cosf(angle);
		angle = ltime * avelocities[i][2];
		sr = sinf(angle);
		cr = cosf(angle);

		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;

		if (!(p = new_particle()))
			return;

		dist = sinf(ltime + i) * 32.0f;
		p->org[0] = origin[0] + bytedirs[i][0]*dist + forward[0]*BEAMLENGTH;
		p->org[1] = origin[1] + bytedirs[i][1]*dist + forward[1]*BEAMLENGTH;
		p->org[2] = origin[2] + bytedirs[i][2]*dist + forward[2]*BEAMLENGTH;

		VectorClear (p->vel);
		VectorClear (p->accel);

		p->type = PARTICLE_STANDARD;		
		p->blendsrc = GL_ONE;
		p->blenddst = GL_ONE;
		p->scale = 2;
		p->scalevel = 2;
		if(nEffect & EF_QUAD)
		{
			p->image = r_doubledamage;
			p->color = 0x74;
		}
		else if(nEffect & EF_PENT)
		{
			p->image = r_invulnerability;
			p->color = 0x40;
		}
		else
		{
			p->image = r_logotexture;
			p->color = 0xd4;
		}

		p->alpha = .7;
		p->alphavel = -50.0f / (0.5f + frand()*0.3f);
	}
}

/*
======
vectoangles2 - this is duplicated in the game DLL, but I need it here.
======
*/
void vectoangles2 (vec3_t value1, vec3_t angles)
{
	float	forward;
	float	yaw, pitch;

	if (value1[1] == 0 && value1[0] == 0)
	{
		yaw = 0;
		if (value1[2] > 0)
			pitch = 90;
		else
			pitch = 270;
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

		forward = sqrtf( value1[0]*value1[0] + value1[1]*value1[1] );
		pitch = (atan2f(value1[2], forward) * 180.0f / (float)M_PI);
		if (pitch < 0.0f)
			pitch += 360.0f;
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

/*
===============
CL_SmokeTrail
===============
*/
void CL_SmokeTrail (vec3_t start, vec3_t end, int colorStart, int colorRun, int spacing)
{
	vec3_t		move;
	vec3_t		vec;
	float		len;
	int			j;
	particle_t	*p;

	VectorCopy (start, move);
	VectorSubtract (end, start, vec);
	len = VectorNormalize (vec);

	VectorScale (vec, spacing, vec);

	// FIXME: this is a really silly way to have a loop
	while (len > 0)
	{
		len -= spacing;

		if (!(p = new_particle()))
			return;
		VectorClear (p->accel);

		p->alpha = 1.0;
		p->alphavel = -1.0f / (1.0f + frand()*0.5f);
		p->color = colorStart + (rand() % colorRun);
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = move[j] + crand()*3;
			p->accel[j] = 0;
		}
		p->vel[2] = 20 + crand()*5;

		VectorAdd (move, vec, move);
	}
}

/*
===============
CL_ParticleSteamEffect

Puffs with velocity along direction, with some randomness thrown in
===============
*/

void CL_ParticleSteamEffect (cl_sustain_t *self)
{
	int			i, j;
	particle_t	*p;
	float		d;
	vec3_t		r, u;
	vec3_t		dir;

	VectorCopy (self->dir, dir);
	MakeNormalVectors (dir, r, u);

	for (i=0 ; i<2 ; i++)
	{
		if (!(p = new_particle()))
			return;

		p->type = PARTICLE_STANDARD;
		p->image = r_pufftexture;
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE_MINUS_SRC_ALPHA;
		p->scale = 4 + (rand()&2);
		p->scalevel = 10;
		p->color = self->color;

		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = self->org[j] + self->magnitude*0.1*crand();
		}
		VectorScale (dir, self->magnitude, p->vel);
		d = crand()*self->magnitude/3;
		VectorMA (p->vel, d, r, p->vel);
		d = crand()*self->magnitude/3;
		VectorMA (p->vel, d, u, p->vel);

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -((float)PARTICLE_GRAVITY) / 2.0f;
		p->alpha = 0.1;

		p->alphavel = -1.0f / (6.5f + frand()*0.3f);

		p->fromsustainedeffect = true;
	}
	self->nextthink += self->thinkinterval;
}
void CL_ParticleFireEffect2 (cl_sustain_t *self)
{
	int			i, j;
	particle_t	*p;
	float		d;
	vec3_t		r, u;
	vec3_t		dir;

	VectorCopy (self->dir, dir);
	MakeNormalVectors (dir, r, u);

	for (i=0 ; i<1 ; i++)
	{
		if (!(p = new_particle()))
			return;

		p->type = PARTICLE_STANDARD;
		p->image = r_explosiontexture;
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE;
		p->color = 0xe0;
		p->scale = 24 + (rand()&7);
		p->scalevel = 4;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = self->org[j] + self->magnitude*0.1*crand();

		}
		p->vel[2] = 100;
		VectorScale (dir, self->magnitude, p->vel);
		d = crand()*self->magnitude/3;
		VectorMA (p->vel, d, r, p->vel);
		d = crand()*self->magnitude/3;
		VectorMA (p->vel, d, u, p->vel);

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY/3;
		p->alpha = 0.03;

		p->alphavel = -.015f / (0.8f + frand()*0.3f);

		p->fromsustainedeffect = true;
	}
	self->nextthink += self->thinkinterval;
}
void CL_ParticleSmokeEffect2 (cl_sustain_t *self)
{
	int			i, j;
	particle_t	*p;
	float		d;
	vec3_t		r, u;
	vec3_t		dir;

	VectorCopy (self->dir, dir);
	MakeNormalVectors (dir, r, u);

	for (i=0 ; i<self->count ; i++)
	{
		if (!(p = new_particle()))
			return;

		p->type = PARTICLE_STANDARD;
		p->image = r_smoketexture;
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE_MINUS_SRC_ALPHA;
		p->scale = 6 + (rand()&7);
		p->color = 14;
		p->scalevel = 4.5;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = self->org[j] + self->magnitude*0.1*crand();
		}
		p->vel[2] = 10;
		VectorScale (dir, self->magnitude, p->vel);
		d = crand()*self->magnitude/3;
		VectorMA (p->vel, d, r, p->vel);
		d = crand()*self->magnitude/3;
		VectorMA (p->vel, d, u, p->vel);

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -((float)PARTICLE_GRAVITY) / 3.0f;
		p->alpha = 0.03;

		p->alphavel = -0.015f / (7.8f + frand()*0.3f);

		p->fromsustainedeffect = true;
	}
	self->nextthink += self->thinkinterval;
}

void CL_ParticleDustEffect (cl_sustain_t *self)
{
	int			i, j;
	particle_t	*p;
	float		d;
	vec3_t		r, u;
	vec3_t		dir;

	VectorCopy (self->dir, dir);
	MakeNormalVectors (dir, r, u);

	for (i=0 ; i<1 ; i++)
	{
		if (!(p = new_particle()))
			return;

		p->type = PARTICLE_STANDARD;
		p->image = r_fireballtexture;
		p->blendsrc = GL_SRC_ALPHA;
		p->blenddst = GL_ONE_MINUS_SRC_ALPHA;
		p->color = 14;
		p->scale = 256 + (rand()&7);
		p->scalevel = 100;
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = self->org[j] + self->magnitude*0.1*crand();

		}
		p->vel[2] = 100;
		VectorScale (dir, self->magnitude, p->vel);
		d = crand()*self->magnitude/3;
		VectorMA (p->vel, d, r, p->vel);
		d = crand()*self->magnitude/3;
		VectorMA (p->vel, d, u, p->vel);

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY/2;
		p->alpha = 0.02;

		p->alphavel = -.01f / (0.8f + frand()*0.3f);

		p->fromsustainedeffect = true;
	}
	self->nextthink += self->thinkinterval;
}

/*
===============
CL_AddParticles
===============
*/
void CL_AddParticles (void)
{
	particle_t		*p, *next;
	float			light;
	float			time = 0.0f;
	particle_t		*active, *tail;
	int				i;
	
	Cl_WeatherEffects();

	active = NULL;
	tail = NULL;

	for (p = active_particles; p; p = next)
	{
		next = p->next;

		// PMM - added INSTANT_PARTICLE handling for heat beam
		if (p->alphavel != INSTANT_PARTICLE)
		{
			time = (cl.time - p->time)*0.001; 
		
			p->current_alpha = p->alpha + time*p->alphavel;
			if (p->current_alpha <= 0)
			{	// faded out
				free_particle (p);
				continue;
			}
		}
		else
		{
			p->current_alpha = p->alpha;
			p->alpha = p->alphavel = 0.0f;
		}
		if(p->scalevel > 1) {
			time = (cl.time - p->time)*0.001;
			p->current_scale = p->scale + time * p->scalevel;
		}
		else
			p->current_scale = p->scale;
			
        if (p->colorvel != 0) {
            time = (cl.time - p->time)*0.001;
            p->current_color = (p->color + time * p->colorvel);
        } else
            p->current_color = p->color;

		if(p->current_scale > 64)
			p->current_scale = 64; //don't want them getting too large.
		if(p->current_scale < 0)
		    p->current_scale = 0;
		
		if(p->current_color < 0)
		    p->current_color = 0;

		// update origin
		for(i = 0; i < 3; i++) {
			p->current_origin[i] = p->org[i] + p->vel[i]*time + p->accel[i]*(time*time);
		}
		
		// free up weather particles that have hit the ground
		if(p->type == PARTICLE_WEATHER || p->type == PARTICLE_FLUTTERWEATHER)
		{
			if (p->current_origin[2] <= p->end[2]){
				free_particle (p);
				continue;
			}
		}
		
		// some weather particles flutter around as they fall
		if (p->type == PARTICLE_FLUTTERWEATHER)
		{
			// An accurate-looking model of particle rotation is to model the
			// particle as a sphere rotating on a surface (or a circle on each
			// axis.) If the particle was a marble moving on some hypothetical
			// 3D tabletop, how many degrees in each direction has it rotated 
			// since it was spawned? 
			float circumference;
			vec3_t distance;
			
			// Use a slightly larger radius so it rotates slower.
			circumference = 2.0f * (float)M_PI * (3.5f * p->current_scale);
			
			VectorSubtract (p->current_origin, p->org, distance);
			VectorScale (distance, 360.0f/circumference, p->angle);
		}

		p->next = NULL;
		if (!tail)
			active = tail = p;
		else
		{
			tail->next = p;
			tail = p;
		}

		if (p->current_alpha > 1.0)
			p->current_alpha = 1;

		for (i=0;i<P_LIGHTS_MAX;i++)
		{
			const cplight_t *plight = &p->lights[i];
			if (plight->isactive && !p->fromsustainedeffect)
			{
				light = plight->light*p->current_alpha*2.0 + plight->lightvel*time;
				V_AddLight (p->current_origin, light, plight->lightcol[0], plight->lightcol[1], plight->lightcol[2]);
			}
		}
	}

	active_particles = active;
	
	// second pass: clean up chains that have had some of their members expire
	for (p = active_particles; p; p = p->next)
	{
		if (p->type == PARTICLE_CHAINED && p->chain_prev != NULL && p->chain_prev->free)
			p->chain_prev = NULL;
		
		// hack a scale up to keep particles from disapearing
        p->dist = ( p->current_origin[0] - r_origin[0] ) * vpn[0] +
                ( p->current_origin[1] - r_origin[1] ) * vpn[1] +
                ( p->current_origin[2] - r_origin[2] ) * vpn[2];

        if (p->type != PARTICLE_CHAINED && p->dist < 0)
            continue;
        else if (p->dist >= 40)
            p->dist = 2 + p->dist * 0.004;
        else
            p->dist = 2;
        
        if ((p->type == PARTICLE_CHAINED) != (p->chain_prev != NULL))
        	continue;
        
        if (p->type == PARTICLE_CHAINED && p->chain_prev) {
        	vec3_t span, delta;
        	VectorSubtract (p->current_origin, p->chain_prev->current_origin, span);
        	VectorSubtract (r_origin, p->current_origin, delta);
        	CrossProduct (span, delta, p->current_pspan);
        	VectorNormalize (p->current_pspan);
        	VectorScale (p->current_pspan, p->dist*p->current_scale, p->current_pspan);
        }
        
		V_AddParticle (p);
	}
}


/*
==============
CL_EntityEvent

An entity has just been parsed that has an event value

the female events are there for backwards compatability
==============
*/
extern struct sfx_s	*cl_sfx_footsteps[4];
extern struct sfx_s *cl_sfx_metal_footsteps[4];

//Knightmare- modified by Irritant
/*
===============
CL_Footsteps
Plays appropriate footstep sound depending on surface flags of the ground surface.
Since this is a replacement for plain Jane EV_FOOTSTEP, we already know
the player is definitely on the ground when this is called.
===============
*/

void CL_FootSteps (entity_state_t *ent, qboolean loud)
{
   trace_t   tr;
   vec3_t   end;
   int      r;
   int      surface;
   struct sfx_s   *stepsound;
   float volume = 0.5;

   if(ent->effects & EF_SILENT)
	   return;

   r = (rand()&3);

   VectorCopy(ent->origin,end);
   end[2] -= 64;
   tr = CL_PMSurfaceTrace (ent->origin,NULL,NULL,end,MASK_SOLID | MASK_WATER);
   if (!tr.surface)
      return;
   surface = tr.surface->flags;
   switch (surface)
   {
	  case SURF_TRANS66:
      case SURF_TRANS33|SURF_TRANS66: //all metal grates in AA have these flags set
         stepsound = cl_sfx_metal_footsteps[r];
	     break;
      default:
         stepsound = cl_sfx_footsteps[r];
         break;
   }

   volume = 1.0;
   S_StartSound (NULL, ent->number, CHAN_BODY, stepsound, volume, ATTN_NORM, 0);
}

void CL_WaterWade (entity_state_t *ent)
{
	particle_t	*p;
	trace_t   tr;
    vec3_t   end, angle;
	int		j;

	S_StartSound (NULL, ent->number, CHAN_BODY, S_RegisterSound("player/wade1.wav"), 1, ATTN_NORM, 0);

	VectorCopy(ent->origin,end);
    end[2] -= 64;
    tr = CL_PMSurfaceTrace (ent->origin,NULL,NULL,end,MASK_SOLID | MASK_WATER);
    if (!tr.surface)
       return;

	//draw rings that expand outward
	if(!(p = new_particle()))
		return;
	p->type = PARTICLE_RAISEDDECAL;
	p->image = r_splashtexture;
	p->blendsrc = GL_DST_COLOR;
	p->blenddst = GL_SRC_COLOR;
	p->scale = 1;
	p->scalevel = 8;
	p->color = 0 + (rand() & 1);

	VectorScale(tr.plane.normal, -1, angle);
	RotateForNormal(angle, p->angle);
	p->angle[ROLL] = rand() % 360;
	VectorAdd(tr.endpos, tr.plane.normal, p->org);

	for (j=0 ; j<3 ; j++)
	{
		p->org[j] = tr.endpos[j];
		p->vel[j] = 0;
	}
	p->accel[0] = p->accel[1] = p->accel[2] = 0;
	p->alpha = .1;

	p->alphavel = -0.1f / (1.0f + frand()*0.3f);
}

void CL_EntityEvent (entity_state_t *ent)
{
	switch (ent->event)
	{
	case EV_ITEM_RESPAWN:
		S_StartSound (NULL, ent->number, CHAN_WEAPON, S_RegisterSound("items/respawn1.wav"), 1, ATTN_IDLE, 0);
		CL_ItemRespawnParticles (ent->origin);
		break;
	case EV_PLAYER_TELEPORT:
		S_StartSound (NULL, ent->number, CHAN_WEAPON, S_RegisterSound("misc/tele1.wav"), 1, ATTN_IDLE, 0);
		CL_TeleportParticles (ent->origin);
		break;
	case EV_FOOTSTEP:
		if (cl_footsteps->value)
			CL_FootSteps (ent, false);
		break;
	case EV_WADE:
		if (cl_footsteps->value)
			CL_WaterWade ( ent );
		break;
	case EV_FALLSHORT:
		S_StartSound (NULL, ent->number, CHAN_AUTO, S_RegisterSound ("player/land1.wav"), 1, ATTN_NORM, 0);
		break;
	case EV_FALL:
		S_StartSound (NULL, ent->number, CHAN_AUTO, S_RegisterSound ("*fall2.wav"), 1, ATTN_NORM, 0);
		break;
	case EV_FALLFAR:
		S_StartSound (NULL, ent->number, CHAN_AUTO, S_RegisterSound ("*fall1.wav"), 1, ATTN_NORM, 0);
		break;
	}
}


/*
==============
CL_ClearEffects

==============
*/
void CL_ClearEffects (void)
{
	CL_ClearParticles ();
	CL_ClearDlights ();
	CL_ClearLightStyles ();
	CL_ClearClEntities();
}
