#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <math.h>
#include <float.h>
#include <assert.h>
#include <stdlib.h>

#include "binheap.h"
#include "libgarland.h"

// Implementation of Michael Garland's polygonal surface simplification
// algorithm. You can read his PhD thesis on the algorithm here:
// http://mgarland.org/research/thesis.html

// NOTE: I am intentionally not using CRX's custom types here because I want
// to be able to use this code in other codebases, such as aaradiant. -Max

// FIXME: there are still some bugs in here. We occasionally end up with 
// "balcony-like" features and walls that "tuck in" under the polygons that
// are supposed to be capping them.


// The algorithm simplifies meshes in any number of dimensions. For example,
// a 3D mesh would obviously use up 3 dimensions. However, adding texcoords
// brings the total up to 5 dimensions (i.e. manipulating a mesh in 5D space.)
// If you were doing a skeletal model with 10 bones and blend weights for each
// bone, that would bring the total up to 15 dimensions. For now, we just do
// spacial and texture coordinates for a total of 5 dimensions.

#define AXES			5
#define _SCOORDS(s,x,y)	(y*s+x - y*(y+1)/2)
#define SCOORDS(s,x,y)	((y)>(x) ? _SCOORDS((s),(y),(x)) : _SCOORDS((s),(x),(y)))
#define ACOORDS(x,y)	SCOORDS(AXES,x,y)
#define MATSIZE			(1 + ACOORDS (AXES-1, AXES-1))
#define M_PI		3.14159265358979323846

// The quadric of a plane is effectively a function that computes the squared
// distance of a given point from that plane. The values in this structure
// determines how it will multiply and add the elements of a point vector,
// similar to setting up the values in a matrix to get a desired
// transformation function. There are simpler functions that can also get the
// distance from a point to a plane, but quadrices have some nifty
// mathematical properties that are desirable here.
typedef struct
{
	// Symmetric nxn matrix; redundant values on one side of the diagonal are
	// ommitted, so we store fewer than nxn values. If n == 3, then matrix is
	// layed out like this:
	//	A[0]	A[1]	A[2]
	//	A[1]	A[3]	A[4]
	//	A[2]	A[4]	A[5]
	// You don't really need to worry about this, just use the ACOORDS macro.
	double	A[MATSIZE];
	
	// n-vector
	double	b[AXES];
	
	// constant
	double	c;
} quadric_t;

typedef struct edge_s
{
	struct vert_s	*vtx_a, *vtx_b;
	
	char		border;				// 1 if the edge is only used once
	char		cull;				// 1 if we're not keeping it
	double		contract_pos[AXES];	// calculated optimum position for contraction
	double		contract_error;		// cost of deleting this edge
	char		contract_flipvtx;	// if this is 1, contract into vertex b
	char		moved;
	
	// for the binheap code
	unsigned int	sorted_pos;
} edge_t;

typedef struct edgelist_s
{
	edge_t				*edge;
	struct edgelist_s	*next;
} edgelist_t;

typedef struct trilist_s
{
	struct tri_s		*tri;
	struct trilist_s	*next;
	char				idx; // Which vertex is this within the triangle?
} trilist_t;

typedef struct vert_s
{
	char		cull;	// 1 if we're not keeping it
	idx_t		idx;	// Used to assign the vertex index when creating the
						// new simplified geometry.
	double		pos[AXES];
	quadric_t	quadric;
	edgelist_t	*edges;
	trilist_t	*tris;
} vert_t;

typedef struct tri_s
{
	char	cull; // 1 if we're not keeping it
	vert_t	*verts[3];
} tri_t;


static edge_t *find_edge (const vert_t *a, const vert_t *b)
{
	edgelist_t *list;
	
	if (b < a)
		return find_edge (b, a);
	
	for (list = a->edges; list != NULL; list = list->next)
	{
		if (list->edge->vtx_b == b)
			return list->edge;
	}
	
	return NULL;
}

static edge_t *find_or_create_edge (mesh_t *mesh, vert_t *a, vert_t *b)
{
	edgelist_t	*link;
	edge_t		*ret;
	
	if (b < a)
		return find_or_create_edge (mesh, b, a);
	
	ret = find_edge (a, b);
	if (ret != NULL)
	{
		ret->border = 0;
		return ret;
	}
	
	mesh->edgeheap.items[mesh->num_edges] = ret = &mesh->edges[mesh->num_edges];
	memset (ret, 0, sizeof(*ret));
	ret->vtx_a = a;
	ret->vtx_b = b;
	ret->border = 1;
	
	link = &mesh->edgelinks[2*mesh->num_edges];
	link->edge = ret;
	link->next = a->edges;
	a->edges = link;
	
	link = &mesh->edgelinks[2*mesh->num_edges+1];
	link->edge = ret;
	link->next = b->edges;
	b->edges = link;
	
	mesh->num_edges++;
	
	return ret;
}

static void vec_sub (const double a[AXES], const double b[AXES], double diff[AXES])
{
	int i;
	
	for (i = 0; i < AXES; i++)
		diff[i] = a[i]-b[i];
}

// Assumes vertices in the input data are already unique
static void generate_vertices_for_mesh (mesh_t *mesh)
{
	idx_t i;
	double biggest_scale;
	
	mesh->everts = malloc (mesh->num_verts*sizeof(*mesh->everts));
	memset (mesh->everts, 0, mesh->num_verts*sizeof(*mesh->everts));
	
	memset (mesh->mins, 0, AXES*sizeof(*mesh->mins));
	memset (mesh->maxs, 0, AXES*sizeof(*mesh->maxs));
	
	for (i = 0; i < mesh->num_verts; i++)
	{
		int j;
		
		for (j = 0; j < 3; j++)
			mesh->everts[i].pos[j] = mesh->vcoords[3*i+j];
		
		for (j = 0; j < 2; j++)
			mesh->everts[i].pos[3+j] = mesh->vtexcoords[2*i+j];
		
		for (j = 0; j < AXES; j++)
		{
			if (mesh->everts[i].pos[j] < mesh->mins[j])
				mesh->mins[j] = mesh->everts[i].pos[j];
			else if (mesh->everts[i].pos[j] > mesh->maxs[j])
				mesh->maxs[j] = mesh->everts[i].pos[j];
		}
		
		mesh->everts[i].idx = i;
	}
	
	vec_sub (mesh->maxs, mesh->mins, mesh->scale);
	
	biggest_scale = mesh->scale[0];
	for (i = 1; i < 3; i++)
	{
		if (mesh->scale[i] > biggest_scale)
			biggest_scale = mesh->scale[i];
	}
	
	// Transform the mesh into the range -1 to 1. This helps avoid FP issues
	// which result in nasty visual artefacts. We keep the mesh proportional
	// for accurate error estimation.
	for (i = 0; i < mesh->num_verts; i++)
	{
		int j;
		
		for (j = 0; j < 3; j++)
			mesh->everts[i].pos[j] = (mesh->everts[i].pos[j] - mesh->mins[j]) * 2.0 / biggest_scale - 1.0;
	}
}

static void generate_edges_for_mesh (mesh_t *mesh)
{
	idx_t i;
	
	mesh->num_edges = 0;
	mesh->edges = malloc (mesh->num_tris*3*sizeof(*mesh->edges));
	mesh->edgeheap.items = malloc (mesh->num_tris*3*sizeof(*mesh->edgeheap.items));
	mesh->edgelinks = malloc (mesh->num_tris*6*sizeof(*mesh->edgelinks));
	mesh->trilinks = malloc (mesh->num_tris*6*sizeof(*mesh->trilinks));
	mesh->etris = malloc (mesh->num_tris*sizeof(*mesh->etris));
	
	for (i = 0; i < mesh->num_tris; i++)
	{
		int j;
		
		mesh->etris[i].cull = 0;
		for (j = 0; j < 3; j++)
		{
			trilist_t	*link;
			idx_t		a_idx, b_idx;
			
			a_idx = mesh->tris[i*3+j];
			b_idx = mesh->tris[i*3+((j+1)%3)];
			
			mesh->etris[i].verts[j] = &mesh->everts[a_idx];
			find_or_create_edge (mesh, &mesh->everts[a_idx], &mesh->everts[b_idx]);
			
			link = &mesh->trilinks[3*i+j];
			link->tri = &mesh->etris[i];
			link->next = mesh->etris[i].verts[j]->tris;
			link->idx = j;
			mesh->etris[i].verts[j]->tris = link;
		}
	}
	
	mesh->edgeheap.nitems = mesh->num_edges;
}

static void vec_copy (const double in[AXES], double out[AXES])
{
	int i;
	
	for (i = 0; i < AXES; i++)
		out[i] = in[i];
}

static void vec_scale (const double in[AXES], double scale, double out[AXES])
{
	int i;
	
	for (i = 0; i < AXES; i++)
		out[i] = scale*in[i];
}

static double vec_dot (const double a[AXES], const double b[AXES])
{
	int i;
	double ret = 0;
	
	for (i = 0; i < AXES; i++)
		ret += a[i]*b[i];
	
	return ret;
}

static double vec_length (const double vec[AXES])
{
	return sqrt (vec_dot (vec, vec));
}

// also returns length
static double vec_normalize (const double in[AXES], double out[AXES])
{
	double ret = vec_length (in);
	
	vec_scale (in, 1.0/ret, out);
	
	return ret;
}

static void vec_add (const double a[AXES], const double b[AXES], double diff[AXES])
{
	int i;
	
	for (i = 0; i < AXES; i++)
		diff[i] = a[i]+b[i];
}


static double vec_angle (const double a[AXES], const double b[AXES])
{
	return acos (vec_dot (a, b) / (vec_length (a) * vec_length (b)));
}

// Note that the cross product only uses the first three axes. In all places
// it's used, this ends up being approprate-- the math works out anyway.
static void vec3_cross (const double a[3], const double b[3], double cross[3])
{
	cross[0] = a[1]*b[2] - a[2]*b[1];
	cross[1] = a[2]*b[0] - a[0]*b[2];
	cross[2] = a[0]*b[1] - a[1]*b[0];
}

// Takes two vectors that are in a plane and makes them orthoganal, while 
// keeping them both in the same plane. a_out will remain parallel to a, but
// b_out will not be parallel to b unless b was already orthoganal to a.
static void vec_orthoganalize (const double a[AXES], const double b[AXES], double a_out[AXES], double b_out[AXES])
{
	vec_normalize (a, a_out);
	vec_scale (a_out, vec_dot (a_out, b), b_out);
	vec_sub (b, b_out, b_out);
	vec_normalize (b_out, b_out);
}

// Here, we define a plane using two orthganal unit vectors that are parallel 
// to the plane and a single point that lies on the plane. This ends up being
// more useful here than the usual normal-plus-distance system.
static double get_quadric_error (const quadric_t *q, const double v[AXES]);
static void fundamental_quadric_for_plane (const double tangent1[AXES], const double tangent2[AXES], const double point[AXES], quadric_t *out)
{
	int		i, j;
	double	b_tmp[AXES];
	double	dot_point_tangent1;
	double	dot_point_tangent2;
	
	for (i = 0; i < AXES; i++)
	{
		for (j = i; j < AXES; j++)
		{
			out->A[ACOORDS(i,j)] =	(i == j) -
									tangent1[i]*tangent1[j] -
									tangent2[i]*tangent2[j];
		}
	}
	
	dot_point_tangent1 = vec_dot (point, tangent1);
	dot_point_tangent2 = vec_dot (point, tangent2);
	
	vec_scale (tangent1, dot_point_tangent1, out->b);
	vec_scale (tangent2, dot_point_tangent2, b_tmp);
	vec_add (out->b, b_tmp, out->b);
	vec_sub (out->b, point, out->b);
	
	out->c =	vec_dot (point, point) -
				dot_point_tangent1*dot_point_tangent1 -
				dot_point_tangent2*dot_point_tangent2;
}

static void add_quadrices (const quadric_t *a, const quadric_t *b, quadric_t *sum)
{
	int i;
	
	for (i = 0; i < MATSIZE; i++)
		sum->A[i] = a->A[i] + b->A[i];
	
	for (i = 0; i < AXES; i++)
		sum->b[i] = a->b[i] + b->b[i];
	
	sum->c = a->c + b->c;
}

static void scale_quadric (const quadric_t *in, double scale, quadric_t *out)
{
	int i;
	
	for (i = 0; i < MATSIZE; i++)
		out->A[i] = scale * in->A[i];
	
	for (i = 0; i < AXES; i++)
		out->b[i] = scale * in->b[i];
	
	out->c = scale * in->c;
}

// Q(v) = dot(rowvector(v)*A, v) + 2*dot(b, v) + c
static double get_quadric_error (const quadric_t *q, const double v[AXES])
{
	int		i, j;
	double	matproduct[AXES];
	
	for (i = 0; i < AXES; i++)
	{
		matproduct[i] = 0;
		
		// Loop unrolling- profiling indicates this area is a bottleneck
		j = 0;
		
#if AXES % 2
		matproduct[i] += v[j] * q->A[ACOORDS(i,j)];
		j++;
#endif
		
		for (; j < AXES; j++)
		{
			matproduct[i] += v[j] * q->A[ACOORDS(i,j)];
			j++;
			matproduct[i] += v[j] * q->A[ACOORDS(i,j)];
		}
	}
	
	return vec_dot (matproduct, v) + 2 * vec_dot (q->b, v) + q->c;
}

static void vec3_sub (const double a[3], const double b[3], double diff[3])
{
	diff[0] = a[0] - b[0];
	diff[1] = a[1] - b[1];
	diff[2] = a[2] - b[2];
}

static double vec3_dot (const double a[3], const double b[3])
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// check if a hypothetical triangle comprised of these three verts in the given
// order would be inverted
static int triangle_would_invert (const vert_t *v0, const vert_t *v1, const vert_t *v2)
{
	int i;
	double	temp[3], bitangent[3], tangent[3], normal[3];
	double	posdiff1[3], posdiff2[3];
	double	stdiff1[2], stdiff2[2];
	
	vec3_sub (v1->pos, v0->pos, posdiff1);
	vec3_sub (v2->pos, v0->pos, posdiff2);
	
#if AXES == 5 // take texcoords into account for inversion detection
#define TEXIDX(i) (i+3)
#elif AXES == 3 // just look at vertex ordering alone
#define TEXIDX(i) (i)
#else
#error must customize triangle_is_inverted for the current number of axes!
#endif
	for (i = 0; i < 2; i++)
	{
		stdiff1[i] = v1->pos[TEXIDX(i)] - v0->pos[TEXIDX(i)];
		stdiff2[i] = v2->pos[TEXIDX(i)] - v0->pos[TEXIDX(i)];
	}
#undef TEXIDX
	
	// get the tangent & bitangent (though not scaled or normalized correctly)
	for (i = 0; i < 3; i++)
	{
		tangent[i] = posdiff1[i] * stdiff2[1] - posdiff2[i] * stdiff1[1];
		bitangent[i] = posdiff2[i] * stdiff1[0] - posdiff1[i] * stdiff2[0];
	}
	
	// get the normal vector
	vec3_cross (posdiff2, posdiff1, normal);

	// handedness
	vec3_cross (normal, tangent, temp);
	return vec3_dot (temp, bitangent) >= 0.0;
}

// would any triangles get inverted if we contracted a and b onto a?
// This helps, but FIXME: why isn't this catching everything?
static double penalize_inversions (const vert_t *a, const vert_t *b)
{
	const trilist_t *link;
	double penalty = 0.0;
	
	for (link = b->tris; link != NULL; link = link->next)
	{
		const vert_t *e2_a, *e2_b;
		
		e2_a = link->tri->verts[(link->idx + 1) % 3];
		e2_b = link->tri->verts[(link->idx + 2) % 3];
		
		if (e2_a == a || e2_b == a)
			continue;
		
		if (triangle_would_invert (a, e2_a, e2_b))
			penalty += 1000.0;
	}
	
	return penalty;
}

static void generate_contraction (edge_t *edge)
{
	quadric_t		q;
	double			a_error, b_error;
	
	add_quadrices (&edge->vtx_a->quadric, &edge->vtx_b->quadric, &q);
	
	// what would happen if we contracted a and b into a?
	a_error =	get_quadric_error (&q, edge->vtx_a->pos) + 
				penalize_inversions (edge->vtx_a, edge->vtx_b);
	
	// If a and b are on the same plane, it doesn't matter which we pick.
	if (fabs (a_error) < DBL_EPSILON)
	{
		edge->contract_error = a_error;
		edge->contract_flipvtx = 0;
		vec_copy (edge->vtx_a->pos, edge->contract_pos);
		return;
	}
	
	// what would happen if we contracted a and b into b?
	b_error =	get_quadric_error (&q, edge->vtx_b->pos) + 
				penalize_inversions (edge->vtx_b, edge->vtx_a);
	
	if (b_error < a_error)
	{
		edge->contract_error = b_error;
		edge->contract_flipvtx = 1;
		vec_copy (edge->vtx_b->pos, edge->contract_pos);
	}
	else
	{
		edge->contract_error = a_error;
		edge->contract_flipvtx = 0;
		vec_copy (edge->vtx_a->pos, edge->contract_pos);
	}
}

static void add_edge_quadrices (vert_t *vtx_a, vert_t *vtx_b, vert_t *vtx_c, double normal[AXES], double tri_area, quadric_t *tri_q)
{
	double		edgevec1[AXES], edgevec2[AXES];
	edge_t		*edge;
	quadric_t	q2;
	
	vec_sub (vtx_c->pos, vtx_a->pos, edgevec1);
	vec_sub (vtx_b->pos, vtx_a->pos, edgevec2);
	
	scale_quadric (tri_q, tri_area * vec_angle (edgevec1, edgevec2) / M_PI, &q2);
	
	add_quadrices (&vtx_a->quadric, &q2, &vtx_a->quadric);
	
	// If this is the only triangle that uses this edge, then the edge is part
    // of the "border" of the mesh. Add a special extra quadric to prevent the
    // edge from being eroded-- add the cost of moving either of its vertices
    // from a hypothetical plane perpendicular to this triangle.
	if ((edge = find_edge (vtx_a, vtx_b))->border)
	{
		double tangent[AXES];
		double tmp;
		
		tmp = vec_normalize (edgevec2, tangent);
		
		fundamental_quadric_for_plane (tangent, normal, vtx_a->pos, &q2);
		
		scale_quadric (&q2, tmp*tmp, &q2);
		add_quadrices (&vtx_a->quadric, &q2, &vtx_a->quadric);
		add_quadrices (&vtx_b->quadric, &q2, &vtx_b->quadric);
	}
}

static void generate_quadrices_for_tri (tri_t *tri)
{
	int			i;
	vert_t		*vtx_a, *vtx_b, *vtx_c;
	double		normal[AXES];
	double		edgevec1[AXES], edgevec2[AXES], tangent1[AXES], tangent2[AXES];
	double		area;
	quadric_t	q;
	
	vtx_a = tri->verts[0];
	vtx_b = tri->verts[1];
	vtx_c = tri->verts[2];

	vec_sub (vtx_c->pos, vtx_a->pos, edgevec1);
	vec_sub (vtx_b->pos, vtx_a->pos, edgevec2);
	
	vec3_cross (edgevec1, edgevec2, normal);
	for (i = 3; i < AXES; i++)
		normal[i] = 0;
	area = vec_normalize (normal, normal)/2;
	
	vec_orthoganalize (edgevec1, edgevec2, tangent1, tangent2);
	
	fundamental_quadric_for_plane (tangent1, tangent2, vtx_a->pos, &q);
	add_edge_quadrices (vtx_a, vtx_b, vtx_c, normal, area, &q);
	add_edge_quadrices (vtx_b, vtx_c, vtx_a, normal, area, &q);
	add_edge_quadrices (vtx_c, vtx_a, vtx_b, normal, area, &q);
}

static void generate_quadrices_for_mesh (mesh_t *mesh)
{
	idx_t i;
	
	for (i = 0; i < mesh->num_tris; i++)
		generate_quadrices_for_tri (&mesh->etris[i]);
}

static int edge_compare (const void *_a, const void *_b)
{
	const edge_t *a, *b;
	
	a = (const edge_t *)_a;
	b = (const edge_t *)_b;
	
	if (a->contract_error < b->contract_error) return -1;
	if (a->contract_error > b->contract_error) return 1;
	if (a < b) return -1;
	if (a > b) return 1;
	return 0;
}

static void edge_setidx (void *a, unsigned int idx)
{
	((edge_t *)a)->sorted_pos = idx;
}

static unsigned int edge_getidx (const void *a)
{
	return ((edge_t *)a)->sorted_pos;
}

static void generate_all_contractions (mesh_t *mesh)
{
	idx_t i;
	
	for (i = 0; i < mesh->num_edges; i++)
		generate_contraction (&mesh->edges[i]);
	
	binheap_heapify (&mesh->edgeheap);
}

static void delete_triangle (mesh_t *mesh, tri_t *tri)
{
	int i;
	trilist_t **list;
	
	if (tri == NULL || tri->cull)
		return;
	
	for (i = 0; i < 3; i++)
	{
		for (list = &tri->verts[i]->tris; *list != NULL; list = &(*list)->next)
		{
			if ((*list)->tri == tri)
			{
				*list = (*list)->next;
				break;
			}
		}
	}
	
	tri->cull = 1;
	mesh->simplified_num_tris--;
}

static void delete_edge (mesh_t *mesh, edge_t *edge)
{
	edgelist_t **list;
	
	if (edge == NULL)
		return;
	
	for (list = &edge->vtx_a->edges; *list != NULL; list = &(*list)->next)
	{
		if ((*list)->edge == edge)
		{
			*list = (*list)->next;
			break;
		}
	}
	
	for (list = &edge->vtx_b->edges; *list != NULL; list = &(*list)->next)
	{
		if ((*list)->edge == edge)
		{
			*list = (*list)->next;
			break;
		}
	}
	
	binheap_remove (&mesh->edgeheap, edge);
	
	edge->cull = 1;
}

// Replace all instances of vertex b with vertex a. 
static void replace_vertex (mesh_t *mesh, vert_t *a, vert_t *b)
{
	while (b->edges != NULL)
	{
		edgelist_t		*link;
		vert_t			*c;
		edge_t			*tmp;
		
		link = b->edges;
		b->edges = link->next;
		if (link->edge->vtx_a == b)
			c = link->edge->vtx_b;
		else
			c = link->edge->vtx_a;
		
		assert (!link->edge->cull);
		
		if ((tmp = find_edge (a, c)) != NULL)
		{
			// We've found an edge from b->c which will be merged into an
			// existing	edge from a->c when vertex b is merged into vertex a.
			// Delete the b->c edge, as it is now redundant.
			delete_edge (mesh, link->edge);
		}
		else
		{
			if (a > c)
			{
				link->edge->vtx_a = c;
				link->edge->vtx_b = a;
			}
			else
			{
				link->edge->vtx_a = a;
				link->edge->vtx_b = c;
			}
			link->edge->moved = 1;
			link->next = a->edges;
			a->edges = link;
		}
	}
	
	while (b->tris != NULL)
	{
		trilist_t		*link;
		int				i;
		
		link = b->tris;
		b->tris = link->next;
		
		for (i = 0; i < 3; i++)
		{
			if (link->tri->verts[i] == a)
			{
				delete_triangle (mesh, link->tri);
				break;
			}
			if (link->tri->verts[i] == b)
				link->tri->verts[i] = a;
		}
		
		if (!link->tri->cull)
		{
			link->next = a->tris;
			a->tris = link;
		}
	}
}

// Recalculate the cost of contracting any edge that is incident on the given
// vertex
static void recalculate_vert_neighborhood (mesh_t *mesh, vert_t *vtx)
{
	edgelist_t *edges;
	
	for (edges = vtx->edges; edges != NULL; edges = edges->next)
	{
		vert_t *vtx2;
		edgelist_t *edges2;
		edge_t *edge = edges->edge;
		
		assert (!edge->cull);
		
		binheap_remove (&mesh->edgeheap, edge);
		generate_contraction (edge);
		binheap_insert (&mesh->edgeheap, edge);
		
		if (!edge->moved)
			continue;
		
		edge->moved = 0;
		
		if (edge->vtx_a == vtx)
			vtx2 = edge->vtx_b;
		else
			vtx2 = edge->vtx_a;
		
		for (edges2 = vtx2->edges; edges2 != NULL; edges2 = edges2->next)
		{
			edge_t *edge2 = edges2->edge;
			
			if (edge2 == edge)
				continue;
			
			binheap_remove (&mesh->edgeheap, edge2);
			generate_contraction (edge2);
			binheap_insert (&mesh->edgeheap, edge2);
		}
	}
}

// Modifies the mesh by contracting the vertex pair that introduces the least
// error by being contracted.
static void contract (mesh_t *mesh, edge_t *edge)
{
	vert_t			*a, *b;
	
	assert (!edge->cull);
	
	if (edge->contract_flipvtx)
	{
		a = edge->vtx_b;
		b = edge->vtx_a;
	}
	else
	{
		a = edge->vtx_a;
		b = edge->vtx_b;
	}
	
	b->cull = 1;
	
	// Do the contraction: update a's quadric
	add_quadrices (&a->quadric, &b->quadric, &a->quadric);
	
	// Do the contraction: remove the edge that was just contracted
	delete_edge (mesh, edge);
	
	// Do the contraction: change all references to b into references to a
	replace_vertex (mesh, a, b);
	
	// Do the contraction: move a to the optimum place
	vec_copy (edge->contract_pos, a->pos);
	
	// Do the contraction: recalculate contraction errors as needed
	recalculate_vert_neighborhood (mesh, a);
}

// Reencode the simplified geometry back into the format given as input
// * culls vertices
// * simplification cannot be continued after this is called
// * revises mesh->num_tris to match the reduced number of triangles
static void reconstruct_mesh (mesh_t *mesh)
{
	idx_t	old_num_verts, old_num_tris;
	idx_t	i;
	
	old_num_verts = mesh->num_verts;
	mesh->num_verts = 0;
	for (i = 0; i < old_num_verts; i++)
	{
		int j;
		
		if (mesh->everts[i].cull)
			continue;
		
		mesh->everts[i].idx = mesh->num_verts++;
		
		for (j = 0; j < 3; j++)
			mesh->vcoords[3*mesh->everts[i].idx+j] = mesh->vcoords[3*i+j];
		
		for (j = 0; j < 2; j++)
			mesh->vtexcoords[2*mesh->everts[i].idx+j] = mesh->vtexcoords[2*i+j];
	}
	
	old_num_tris = mesh->num_tris;
	mesh->num_tris = 0;
	for (i = 0; i < old_num_tris; i++)
	{
		int		j;
		tri_t	*tri;
		
		tri = &mesh->etris[i];
		
		if (tri->cull)
			continue;
		
		for (j = 0; j < 3; j++)
		{
			assert (!tri->verts[j]->cull);
			mesh->tris[3*mesh->num_tris+j] = tri->verts[j]->idx;
		}
		
		mesh->num_tris++;
	}
	
	assert (mesh->num_tris == mesh->simplified_num_tris);
}

// Reencode the simplified geometry back into the format given as input
// * leaves culled vertices
// * simplification may be continued after this is called
// * must look in mesh->simplified_num_tris to get the reduced number of
//   triangles
void export_mesh (mesh_t *mesh)
{
	idx_t	new_idx = 0;
	idx_t	i;
	
	for (i = 0; i < mesh->num_tris; i++)
	{
		int		j;
		tri_t	*tri;
		
		tri = &mesh->etris[i];
		
		if (tri->cull)
			continue;
		
		for (j = 0; j < 3; j++)
		{
			assert (!tri->verts[j]->cull);
			mesh->tris[3*new_idx+j] = tri->verts[j]->idx;
		}
		
		new_idx++;
	}
	
	assert (new_idx == mesh->simplified_num_tris);
}

void simplify_init (mesh_t *mesh)
{
	static double mins[AXES], maxs[AXES], scale[AXES];
	
	mesh->mins = mins;
	mesh->maxs = maxs;
	mesh->scale = scale;
	mesh->edgeheap.cmp = edge_compare;
	mesh->edgeheap.setidx = edge_setidx;
	mesh->edgeheap.getidx = edge_getidx;
	
	generate_vertices_for_mesh (mesh);
	generate_edges_for_mesh (mesh);
	generate_quadrices_for_mesh (mesh);
	generate_all_contractions (mesh);
	
	mesh->simplified_num_tris = mesh->num_tris;
}

void simplify_teardown (mesh_t *mesh)
{
	free (mesh->everts);
	free (mesh->etris);
	free (mesh->edges);
	free (mesh->edgelinks);
	free (mesh->edgeheap.items);
}

int simplify_step (mesh_t *mesh, idx_t target_polycount)
{
	edge_t *next_contraction = binheap_find_root (&mesh->edgeheap);
	if (fabs (next_contraction->contract_error) < DBL_EPSILON || mesh->simplified_num_tris > target_polycount)
	{
		contract (mesh, next_contraction);
		return 1;
	}
	
	return 0;
}

// Continue to remove edges from the mesh until it reaches a target polygon
// count.
void simplify_mesh (mesh_t *mesh, idx_t target_polycount)
{
	simplify_init (mesh);
	while (simplify_step (mesh, target_polycount));
	reconstruct_mesh (mesh);
	simplify_teardown (mesh);
}
