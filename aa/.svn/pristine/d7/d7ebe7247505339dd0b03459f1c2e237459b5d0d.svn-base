#ifndef __GL_SCRIPT__
#define __GL_SCRIPT__

//CVARS
extern cvar_t *rs_dynamic_time;

// Animation loop
typedef struct anim_stage_s 
{
	image_t					*texture;	// texture
	char					name[MAX_OSPATH];	// texture name
	struct anim_stage_s		*next;		// next anim stage
} anim_stage_t;


typedef struct random_stage_s 
{
	image_t					*texture;	// texture
	char					name[MAX_OSPATH];	// texture name
	struct random_stage_s	*next;		// next anim stage
} random_stage_t;

// Blending
typedef struct 
{
	int			source;		// source blend value
	int			dest;		// dest blend value
	qboolean	blend;		// are we going to blend?
} blendfunc_t;

// Alpha shifting
typedef struct {
	float		min, max;	// min/max alpha values
	float		speed;		// shifting speed
} alphashift_t;

// scaling
typedef struct
{
	char	typeX, typeY;	// scale types
	float	scaleX, scaleY;	// scaling factors
} rs_scale_t;

// scrolling
typedef struct
{
	char	typeX, typeY;	// scroll types
	float	speedX, speedY;	// speed of scroll
} rs_scroll_t;

// colormap
typedef struct
{
	qboolean	enabled;
	float		red, green, blue;	// colors - duh!
} rs_colormap_t;

//conditional operation types
typedef enum
{
	rs_cond_none,	// no comparison, no subexpressions
	rs_cond_is,		// no comparison, use first subexpresson
	rs_cond_eq,		// ==
	rs_cond_neq,	// !=
	rs_cond_gt,		// >
	rs_cond_ngt,	// <=
	rs_cond_lt,		// <
	rs_cond_nlt,	// >=
	rs_cond_and,	// &&
	rs_cond_or,		// ||
	rs_cond_lnot,   // !, use first subexpression
} rs_cond_op_t;
//conditional operation value
typedef struct rs_cond_val_s
{
	rs_cond_op_t			optype;					//what type of operation
	struct rs_cond_val_s	*subexpr1, *subexpr2;	//sub expressions, if applicable
	cvar_t					*val;					//value, if applicable
	cvar_t					lval;					//for use storing literal values
} rs_cond_val_t;

// Script stage
typedef struct rs_stage_s 
{
	image_t					*texture;				// texture
	char					name[MAX_OSPATH];		// texture name
	
	// These are for the mesh rendering single-stage "fast path."
	image_t					*texture2;				// texture for combining(GLSL)
	char					name2[MAX_OSPATH];		// texture name
	image_t					*texture3;				// texture for combining(GLSL)
	char					name3[MAX_OSPATH];		// texture name

	rs_cond_val_t			*condv;			// conditional expression
	
	anim_stage_t			*anim_stage;	// first animation stage
	float					anim_delay;		// Delay between anim frames
	float					last_anim_time; // gametime of last frame change
	char					anim_count;		// number of animation frames
	anim_stage_t			*last_anim;		// pointer to last anim

	random_stage_t			*rand_stage;	// random linked list
	int						rand_count;		// number of random pics

	rs_colormap_t			colormap;		// color mapping
	blendfunc_t				blendfunc;		// image blending
	alphashift_t			alphashift;		// alpha shifting
	rs_scroll_t				scroll;			// tcmod
	rs_scale_t				scale;			// tcmod
	
	float					rot_speed;		// rotate speed (0 for no rotate);

	qboolean				depthhack;		// fake zdepth

	qboolean				envmap;			// fake envmapping - spheremapping
	qboolean				lightmap;		// lightmap this stage?
	qboolean				alphamask;		// alpha masking?

	qboolean				normalmap;		// use mesh rendering GLSL "fast path?"
	
	int						num_blend_textures;
	float					blend_scales[12];
	char					blend_names[6][MAX_OSPATH];
	image_t					*blend_textures[6];
	
	// For the nth blended normalmap, borrow the blend coefficient channel and
	// texture scale from the normalblend_indices[n]th blended texture.
	int						num_blend_normalmaps;
	int						normalblend_indices[3];
	char					normalblend_names[3][MAX_OSPATH];
	image_t					*blend_normalmaps[3];
	
	qboolean				lensflare;		// for adding lensflares
	int						flaretype;		// type of flare

	qboolean				grass;			// grass and vegetation
	int						grasstype;		// the type of vegetation
	
	qboolean				beam;			// for adding light beams
	int						beamtype;		// the type of beam(up vs down)
	float					xang;			// beam pitch
	float					yang;			// beam roll
	qboolean				rotating;		//rotating beams(moving spotlights, etc).
	
	qboolean				fx;				// for glsl effect layer
	qboolean				glow;			// for glsl effect layer
	qboolean				cube;			// for glsl effect layer

	struct rs_stage_s		*next;			// next stage
} rs_stage_t;



// Base script
typedef struct rscript_s 
{
	char				name[MAX_OSPATH];	// texture the script applies to
	unsigned int		hash_key;			// hash key for the script's name
	char				outname[2*MAX_OSPATH+1]; // for output
	
	qboolean			dontflush;	// dont flush from memory on map change
	qboolean			ready;		// readied by the engine?
	rs_stage_t			*stage;		// first rendering stage
	struct rscript_s	*next;		// next script in linked list
	
	// Bits are set if the script contains certain types of stages. Some kinds
	// of stages require rscript surfaces to be drawn one at a time.
	#define RS_CONTAINS_ROTATE 1
	#define RS_CONTAINS_ENVMAP 2
	#define RS_CONTAINS_SCROLL 4
	#define RS_CONTAINS_SCALE 8
	#define RS_PREVENT_BATCH (RS_CONTAINS_ROTATE|RS_CONTAINS_ENVMAP)
	// Some kinds of stages are only processed at load-time and then ignored
	// (such as lens flare, grass, beam, etc.) If a script contains nothing 
	// but these stage types, we can save some time by not drawing it at all.
	// TODO: filter these stages out at load time, then filter out empty 
	// scripts.
	#define RS_CONTAINS_DRAWN 16
	#define RS_FASTPATH_ALLOWED (RS_CONTAINS_DRAWN) // allowed in fast path
	#define RS_PREVENT_FASTPATH (~RS_FASTPATH_ALLOWED)
	int					flags;

} rscript_t;

void RS_LoadScript(char *script);
void RS_FreeAllScripts(void);
void RS_ReloadImageScriptLinks (void);
void RS_FreeScript(rscript_t *rs);
void RS_FreeUnmarked(void);
rscript_t *RS_FindScript(char *name);
void RS_ReadyScript(rscript_t *rs);
void RS_ScanPathForScripts(void);
void RS_UpdateRegistration(void);
typedef enum 
{
	rs_lightmap_off,
	rs_lightmap_on,
	rs_lightmap_separate_texcoords
} rs_lightmaptype_t;
void RS_Begin_Group (const entity_t *ent);
void RS_End_Group (void);
void RS_Draw (	rscript_t *rs, const entity_t *ent, int lmtex, vec2_t rotate_center,
				vec3_t normal, qboolean translucent, rs_lightmaptype_t lm,
				qboolean enable_dlights, qboolean enable_shadows,
				void (*draw_callback) (void));
void RS_DrawSurface (msurface_t *surf, rscript_t *rs);
void RS_LoadSpecialScripts(void);

extern float rs_realtime;

#endif // __GL_SCRIPT__
