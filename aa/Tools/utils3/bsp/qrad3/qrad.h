
#include "cmdlib.h"
#include "mathlib.h"
#include "bspfile.h"
#include "polylib.h"
#include "threads.h"
#include "lbmlib.h"

#ifdef WIN32
#include <windows.h>
#endif

#ifdef USE_SETRLIMIT
#include <sys/resource.h>
#endif

#if defined WIN32 || (!defined USE_PTHREADS && !defined USE_SETRLIMIT)
#define STACK_CONSTRAINED
#endif

typedef enum
{
	emit_surface,
	emit_point,
	emit_spotlight,
    emit_sky
} emittype_t;

typedef struct directlight_s
{
	struct directlight_s *next;
	emittype_t	type;

	float		intensity;
	int			style;
    float       wait;
    float       adjangle;
	vec3_t		origin;
	vec3_t		color;
	vec3_t		normal;		// for surfaces and spotlights
	float		stopdot;		// for spotlights
    dplane_t    *plane;
    dleaf_t     *leaf;
    int			nodenum;
	float		proportion_direct, proportion_directsun, proportion_indirect;

} directlight_t;


// the sum of all tranfer->transfer values for a given patch
// should equal exactly 0x10000, showing that all radiance
// reaches other patches
typedef struct
{
	unsigned short	patch;
	unsigned short	transfer;
} transfer_t;


#define	MAX_PATCHES	65000			// larger will cause 32 bit overflows

typedef struct patch_s
{
	winding_t	*winding;
	struct patch_s		*next;		// next in face
	int			numtransfers;
	transfer_t	*transfers;
    byte *trace_hit;
    
    int			nodenum;

	int			cluster;			// for pvs checking
	vec3_t		origin;
	dplane_t	*plane;

	qboolean	sky;

	vec3_t		totallight;			// accumulated by radiosity
									// does NOT include light
									// accounted for by direct lighting
	float		area;

	// illuminance * reflectivity = radiosity
	vec3_t		reflectivity;
	vec3_t		baselight;			// emissivity only

	// each style 0 lightmap sample in the patch will be
	// added up to get the average illuminance of the entire patch
	vec3_t		samplelight;
	int			samples;		// for averaging direct light
} patch_t;

extern	patch_t		*face_patches[MAX_MAP_FACES];
extern	entity_t	*face_entity[MAX_MAP_FACES];
extern	vec3_t		face_offset[MAX_MAP_FACES];		// for rotating bmodels
extern	patch_t		patches[MAX_PATCHES];
extern	unsigned	num_patches;

extern	int		leafparents[MAX_MAP_LEAFS];
extern	int		nodeparents[MAX_MAP_NODES];

extern	float	lightscale;


void MakeShadowSplits (void);

float			*texture_data[MAX_MAP_TEXINFO];
int				texture_sizes[MAX_MAP_TEXINFO][2];


qboolean		doing_texcheck;
qboolean		doing_blur;

//==============================================


void BuildVisMatrix (void);
qboolean CheckVisBit (unsigned p1, unsigned p2);

//==============================================

extern	float ambient, maxlight;

void LinkPlaneFaces (void);

// extern qboolean    nocolor;
extern float grayscale;
extern float desaturate;
extern	qboolean	extrasamples;
extern int numbounce;
extern int noblock;

extern	directlight_t	*directlights[MAX_MAP_LEAFS];

extern	byte	nodehit[MAX_MAP_NODES];

typedef struct
{
	vec3_t	direct; // occluded by shadow casters in the engine
	vec3_t	directsun; // occluded by shadow casters in the engine
	vec3_t	indirect; // not shadowed
} sample_t;
//for each sample, we keep track of how much blurring we should do
typedef struct {
	//divide these for a weighted blur average for a single sample
	double	total_blur;
	int		num_lights;
} sample_blur_t[4];
typedef struct
{
	struct cterraintri_s *mru, *lru;
} occlusioncache_t;
void LightContributionToPoint	(	directlight_t *l, vec3_t pos, int nodenum,
									vec3_t normal,
									sample_t *out_color,
									float lightscale2, // adjust for multisamples, -extra cmd line arg
									qboolean *sun_main_once, 
									qboolean *sun_ambient_once,
									sample_blur_t blur,
									occlusioncache_t *cache
								);
void PostProcessLightSample (const sample_t *in, const vec3_t radiosity_add, sample_t *out, vec3_t out_combined_color);

void BuildLightmaps (void);

void BuildFacelights (int facenum);

void FinalLightFace_Worker (int facenum);
void DetectUniformColor (int facenum);

qboolean PvsForOrigin (vec3_t org, byte *pvs);

int	PointInNodenum (vec3_t point);
int TestLine (vec3_t start, vec3_t stop);
int TestLine_color (int node, vec3_t start, vec3_t stop, vec3_t occluded, occlusioncache_t *cache);
int TestLine_r (int node, vec3_t start, vec3_t stop);

void CreateDirectLights (void);

dleaf_t		*PointInLeaf (vec3_t point);


extern	dplane_t	backplanes[MAX_MAP_PLANES];
extern	int			fakeplanes;					// created planes for origin offset

extern	float	subdiv;

extern	float	direct_scale;
extern	float	entity_scale;

extern qboolean sun;
extern qboolean sun_alt_color;
extern vec3_t sun_pos;
extern float sun_main;
extern float sun_ambient;
extern vec3_t sun_color;

int	refine_amt, refine_setting;
int terrain_refine;

int	PointInLeafnum (vec3_t point);
void MakeTnodes (dmodel_t *bm);
void MakePatches (void);
void SubdividePatches (void);
void PairEdges (void);
void CalcTextureReflectivity (void);

void bilinear_sample (const byte *texture, int tex_w, int tex_h, float u, float v, vec4_t out);


#define Z_Free free
void *Z_Malloc (size_t sz);
char *CopyString (const char *in);

#define M_PI       3.14159265358979323846

typedef vec_t vec2_t[2];
#define VectorSet(v, x, y, z)		((v)[0]=(x), (v)[1]=(y), (v)[2]=(z))
#define	PITCH				0		// up / down
#define	YAW					1		// left / right
#define	ROLL				2		// fall over
#define DEG2RAD( a ) (( (a) * M_PI ) / 180.0F)
#define RAD2DEG( a ) (( (a) * 180.0F ) / M_PI)

typedef struct
{
	char			*texture_path;
	char			*lightmap_path;
	int				num_vertices;
	float			*vert_positions;
	float			*vert_texcoords;
	int				num_triangles;
	unsigned int	*tri_indices;
	vec3_t			mins, maxs;
	int             heightmap_w, heightmap_h;
} terraindata_t;

// out will be populated with a simplified version of the mesh. 
// name is just the path of the .terrain file, only used for error messages.
// oversampling_factor indicates how much detail to sample the heightmap 
// at before simplification. 2.0 means 4x as many samples as there are pixels,
// 0.5 means 0.25x as many.
// reduction_amt indicates how many times fewer triangles the simplified mesh
// should have.
// buf is a string containing the text of a .terrain file.
void LoadTerrainFile (terraindata_t *out, const char *name, float oversampling_factor, int reduction_amt, char *buf);

// Frees any allocated buffers in dat.
void CleanupTerrainData (terraindata_t *dat);

qboolean Terrain_Trace (vec3_t start, vec3_t end, vec3_t out_end, vec3_t out_normal);
qboolean Fast_Terrain_Trace_Try_Cache (vec3_t start, vec3_t end, occlusioncache_t *cache);
qboolean Fast_Terrain_Trace_Cache_Miss (vec3_t start, vec3_t end, occlusioncache_t *cache);
void LoadAllTerrain (void);
void GenerateAllTerrainLightmaps (const char *mapname);

void SaveTGA (const byte *texture, int tex_w, int tex_h, const char *name);
