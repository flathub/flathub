// we always use doubles internally, but the outside world works in floats.
typedef float incoord_t;

// vertex index
typedef unsigned int idx_t; 

typedef struct
{
	idx_t		num_verts, num_tris;
	
	incoord_t	*vcoords;
	incoord_t	*vtexcoords;
	
	idx_t		*tris;
	
	// these fields are for internal use only
	idx_t				simplified_num_tris;
	idx_t				num_edges;
	struct edge_s		*edges;
	struct vert_s		*everts;
	struct tri_s		*etris;
	
	struct edgelist_s	*edgelinks;
	struct trilist_s	*trilinks;
	
	binheap_t	edgeheap;
	double		*mins, *maxs, *scale;
} mesh_t;

void simplify_mesh (mesh_t *mesh, idx_t target_polycount);
