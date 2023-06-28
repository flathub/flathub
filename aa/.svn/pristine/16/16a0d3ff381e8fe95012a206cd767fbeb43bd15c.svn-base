#ifndef __MODEL_IQM_H__
#define __MODEL_IQM_H__

//this no worky
#define IDINTERQUAKEMODELHEADER		(('L'<<40768)+('E'<<20384)+('D'<<10192)+('O'<<5096)+('M'<<2048)+('E'<<1024)+('K'<<512)+('A'<<256)+('U'<<128)+('Q'<<64)+('R'<<32)+('E'<<24)+('T'<<16)+('N'<<8)+'I')

typedef struct iqmheader_s
{
	char id[16];
	unsigned int version;
	unsigned int filesize;
	unsigned int flags;
	unsigned int num_text, ofs_text;
	unsigned int num_meshes, ofs_meshes;
	unsigned int num_vertexarrays, num_vertexes, ofs_vertexarrays;
	unsigned int num_triangles, ofs_triangles, ofs_neighbors;
	unsigned int num_joints, ofs_joints;
	unsigned int num_poses, ofs_poses;
	unsigned int num_anims, ofs_anims;
	unsigned int num_frames, num_framechannels, ofs_frames, ofs_bounds;
	unsigned int num_comment, ofs_comment;
	unsigned int num_extensions, ofs_extensions;
} 
iqmheader_t;

typedef struct iqmmesh_s
{
	unsigned int name;
	unsigned int material;
	unsigned int first_vertex, num_vertexes;
	unsigned int first_triangle, num_triangles;
}
iqmmesh_t;

#define IQM_POSITION	    0
#define IQM_TEXCOORD        1
#define IQM_NORMAL          2
#define IQM_TANGENT         3
#define IQM_BLENDINDEXES    4
#define IQM_BLENDWEIGHTS    5
#define IQM_COLOR	        6
#define IQM_CUSTOM          0x10

#define IQM_BYTE    0
#define IQM_UBYTE   1
#define IQM_SHORT   2
#define IQM_USHORT  3
#define IQM_INT	    4
#define IQM_UINT    5
#define IQM_HALF    6
#define IQM_FLOAT   7
#define IQM_DOUBLE  8

// animflags
#define IQM_LOOP 1

typedef struct iqmtriangle_s
{
	unsigned int vertex[3];
}
iqmtriangle_t;

typedef struct iqmjoint_s
{
	unsigned int name;
	signed int parent;
	float origin[3], rotation[3], scale[3];
}
iqmjoint_t;

typedef struct iqmpose_s
{
	signed int parent;
	unsigned int channelmask;
	float channeloffset[9], channelscale[9];
}
iqmpose_t;

typedef struct iqmjoint2_s
{
	unsigned int name;
	signed int parent;
	float origin[3], rotation[4], scale[3];
}
iqmjoint2_t;

typedef struct iqmpose2_s
{
	signed int parent;
	unsigned int channelmask;
	float channeloffset[10], channelscale[10];
}
iqmpose2_t;

typedef struct iqmanim_s
{
	unsigned int name;
	unsigned int first_frame, num_frames;
	float framerate;
	unsigned int flags;
}
iqmanim_t;

typedef struct iqmvertexarray_s
{
	unsigned int type;
	unsigned int flags;
	unsigned int format;
	unsigned int size;
	unsigned int offset;
}
iqmvertexarray_t;

typedef struct iqmextension_s
{
    unsigned int name;
    unsigned int num_data, ofs_data;
    unsigned int ofs_extensions; // pointer to next extension
}
iqmextension_t;

typedef struct iqmbounds_s
{
	float mins[3], maxs[3];
	float xyradius, radius;
}
iqmbounds_t;
    
#endif

