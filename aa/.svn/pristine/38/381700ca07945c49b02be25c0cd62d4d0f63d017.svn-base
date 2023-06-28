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

/*

  Mipmaps are generated for all texture types. If type <= it_wall, brightness
  and contrast settings are applied to it at load time. If type >= it_bump, 
  the standard mipmapping/anisotropic settings are always used, otherwise they
  are only used if the entire texture has the same alpha value. If type !=
  it_pic and type != it_particle, picmip settings are applied at load time.

*/

typedef enum 
{
	it_skin,
	it_sprite,
	it_lightmap,
	it_wall,
	it_pic,
	it_sky,
	it_bump,
	it_particle
} imagetype_t;

typedef struct image_s
{
	char			name[MAX_QPATH];		// game path, including extension
	char			bare_name[MAX_QPATH];		// filename only, as called when searching
	unsigned int		hash_key;			// hash key used in GL_FindImage
	imagetype_t		type;
	int			width, height;			// source image
	int			upload_width, upload_height;	// after power of two and picmip
	int			registration_sequence;		// 0 = free
	int			texnum;				// gl texture binding
	float			sl, tl, sh, th;			// 0,0 - 1,1 unless part of the scrap
	int				crop_left, crop_top, crop_width, crop_height; //for cropped 2D drawing
	float			crop_sl, crop_tl, crop_sh, crop_th;	// texcoords of cropped corners
	qboolean		scrap;
	qboolean		has_alpha;
	qboolean		paletted;
	qboolean		is_cin;				// Heffo - To identify a cin texture's image_t
	void			*script;

} image_t;

//Pretty safe bet most cards support this
#define	LIGHTMAP_SIZE	2048 
#define	MAX_LIGHTMAPS	12

#define	TEXNUM_LIGHTMAPS	1024
#define	TEXNUM_SCRAPS		1152
#define	TEXNUM_IMAGES		1153

#define		MAX_GLTEXTURES	4096 //was 1024

//particles
extern image_t		*r_notexture;
extern image_t		*r_particletexture;
extern image_t		*r_smoketexture; 
extern image_t		*r_fireballtexture;
extern image_t		*r_firestreamtexture;
extern image_t		*r_firestream3ptexture;
extern image_t		*r_explosiontexture;
extern image_t		*r_explosion1texture;
extern image_t		*r_explosion2texture;
extern image_t		*r_explosion3texture;
extern image_t		*r_explosion4texture;
extern image_t		*r_explosion5texture;
extern image_t		*r_explosion6texture;
extern image_t		*r_explosion7texture;
extern image_t		*r_bloodtexture;
extern image_t		*r_blood2texture;
extern image_t		*r_blood3texture;
extern image_t		*r_blood4texture;
extern image_t		*r_blood5texture;
extern image_t		*r_pufftexture;
extern image_t		*r_bflashtexture;
extern image_t		*r_cflashtexture;
extern image_t		*r_fflashtexture;
extern image_t		*r_pflashtexture;
extern image_t		*r_leaderfieldtexture;
extern image_t		*r_deathfieldtexture;
extern image_t		*r_deathfieldtexture2;
extern image_t      *r_shelltexture; 
extern image_t		*r_shelltexture2;
extern image_t		*r_shellnormal;
extern image_t		*r_shellnormal2;
extern image_t		*r_hittexture;
extern image_t		*r_bubbletexture;
extern image_t		*r_reflecttexture;
extern image_t		*r_mirrorspec;
extern image_t		*r_shottexture;
extern image_t		*r_bullettexture;
extern image_t		*r_bulletnormal;
extern image_t		*r_sayicontexture;
extern image_t		*r_flaretexture;
extern image_t		*r_beamstart;
extern image_t		*r_beamtexture;
extern image_t		*r_beam2texture;
extern image_t		*r_beam3texture;
extern image_t		*r_dis1texture;
extern image_t		*r_dis2texture;
extern image_t		*r_dis3texture;
extern image_t		*r_voltagetexture;
extern image_t		*r_raintexture;
extern image_t		*r_leaftexture;
extern image_t		*r_trashtexture;
extern image_t		*r_splashtexture;
extern image_t		*r_splash2texture;
extern image_t		*r_lavasplashtexture;
extern image_t		*r_flagtexture;
extern image_t		*r_logotexture;
extern image_t		*r_distort;
extern image_t		*r_mirrortexture;
extern image_t		*r_doubledamage;
extern image_t		*r_invulnerability;

extern	image_t		gltextures[MAX_GLTEXTURES];
extern	int			numgltextures;

extern image_t		*r_flare;
extern image_t		*r_flare1;
extern image_t		*sun_object;
extern image_t		*sun1_object;
extern image_t		*sun2_object;

extern	image_t		*r_around;
