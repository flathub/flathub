
#include "qfiles.h"


extern	int			nummodels;
extern	dmodel_t	dmodels[MAX_MAP_MODELS];

extern	int			visdatasize;
extern	byte		dvisdata[MAX_MAP_VISIBILITY];
extern	dvis_t		*dvis;

extern	int			lightdatasize;
extern	byte		dlightdata[4][MAX_OVERRIDE_LIGHTING];

extern	int			entdatasize;
extern	char		dentdata[MAX_MAP_ENTSTRING];

extern	int			numleafs;
extern	dleaf_t		dleafs[MAX_MAP_LEAFS];

extern	int			numplanes;
extern	dplane_t	dplanes[MAX_MAP_PLANES];

extern	int			numvertexes;
extern	dvertex_t	dvertexes[MAX_MAP_VERTS];

extern	int			numnodes;
extern	dnode_t		dnodes[MAX_MAP_NODES];

extern	int			numtexinfo;
extern	texinfo_t	texinfo[MAX_MAP_TEXINFO];

extern	int			numfaces;
extern	dface_t		dfaces[MAX_MAP_FACES];

extern	int			numedges;
extern	dedge_t		dedges[MAX_MAP_EDGES];

extern	int			numleaffaces;
extern	unsigned short	dleaffaces[MAX_MAP_LEAFFACES];

extern	int			numleafbrushes;
extern	unsigned short	dleafbrushes[MAX_MAP_LEAFBRUSHES];

extern	int			numsurfedges;
extern	int			dsurfedges[MAX_MAP_SURFEDGES];

extern	int			numareas;
extern	darea_t		dareas[MAX_MAP_AREAS];

extern	int			numareaportals;
extern	dareaportal_t	dareaportals[MAX_MAP_AREAPORTALS];

extern	int			numbrushes;
extern	dbrush_t	dbrushes[MAX_MAP_BRUSHES];

extern	int			numbrushsides;
extern	dbrushside_t	dbrushsides[MAX_MAP_BRUSHSIDES];

extern	byte		dpop[256];
void DecompressVis (byte *in, byte *decompressed);
int CompressVis (byte *vis, byte *dest);

void	LoadBSPFile (char *filename);
void	LoadBSPFileTexinfo (char *filename);	// just for qdata
void	WriteBSPFile (char *filename);
void	PrintBSPFileSizes (void);


//FOR REFINE
ltmp_facelookup_t   lfacelookups[MAX_MAP_FACES];
//dlightdata and lightdatasize are reused
void    WriteLTMPFile (char *filename);

//===============


typedef struct epair_s
{
	struct epair_s	*next;
	char	*key;
	char	*value;
} epair_t;

typedef struct
{
	vec3_t		origin;
	int			firstbrush;
	int			numbrushes;
	epair_t		*epairs;

// only valid for func_areaportals
	int			areaportalnum;
	int			portalareas[2];
} entity_t;

extern	int			num_entities;
extern	entity_t	entities[MAX_MAP_ENTITIES];

void	ParseEntities (void);
void	UnparseEntities (void);

void 	SetKeyValue (entity_t *ent, char *key, char *value);
char 	*ValueForKey (entity_t *ent, char *key);
// will return "" if not present

vec_t	FloatForKey (entity_t *ent, char *key);
void 	GetVectorForKey (entity_t *ent, char *key, vec3_t vec);

epair_t *ParseEpair (void);

void PrintEntity (entity_t *ent);

