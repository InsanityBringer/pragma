/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

//
// qfiles.h: quake file formats
// This file must be identical in the quake and utils directories
//

/*
========================================================================

The .pak files are just a linear collapse of a directory tree

========================================================================
*/

#define IDPAKHEADER		(('K'<<24)+('C'<<16)+('A'<<8)+'P')

typedef struct
{
	char	name[56];
	int		filepos, filelen;
} dpackfile_t;

typedef struct
{
	int		ident;		// == IDPAKHEADER
	int		dirofs;
	int		dirlen;
} dpackheader_t;

#define	MAX_FILES_IN_PACK	4096


/*
========================================================================

.MD3 triangle model file format

========================================================================
*/

typedef enum {LOD_HIGH, LOD_MEDIUM, LOD_LOW, NUM_LODS} lod_t;
//float lodDist[NUM_LODS] = { 0.0f, 512.0f, 1024.0f };

#define MD3_IDENT			(('3'<<24)+('P'<<16)+('D'<<8)+'I')
#define MD3_VERSION			15

// limits
#define MD3_MAX_LODS		NUM_LODS // was 4, but we want NUM_LODS in pragma
#define	MD3_MAX_TRIANGLES	8192	// per surface
#define MD3_MAX_VERTS		4096	// per surface
#define MD3_MAX_SHADERS		256		// per surface (???)
#define MD3_MAX_FRAMES		1024	// per model
#define	MD3_MAX_SURFACES	8		// per model, braxi -- was 32, changed to 8 to fit into network's hidePartBits
#define MD3_MAX_TAGS		16		// per frame
#define MD3_MAX_NAME		64		// max tag and skin names len, this was MAX_QPATH

#define	MD3_XYZ_SCALE		(1.0/64) // vertex scales

typedef struct md3Frame_s 
{
	vec3_t		bounds[2];
	vec3_t		localOrigin;
	float		radius;
	char		name[16];
} md3Frame_t;

typedef struct md3Tag_s 
{
	char		name[MD3_MAX_NAME];	// tag name
	vec3_t		origin;
	vec3_t		axis[3];
} md3Tag_t;

/*
** md3Surface_t
**
** CHUNK			SIZE
** header			sizeof( md3Surface_t )
** shaders			sizeof( md3Shader_t ) * numShaders
** triangles[0]		sizeof( md3Triangle_t ) * numTriangles
** st				sizeof( md3St_t ) * numVerts
** XyzNormals		sizeof( md3XyzNormal_t ) * numVerts * numFrames
*/
typedef struct 
{
	int		ident;				

	char	name[MD3_MAX_NAME];	// polyset name

	int		flags;

	int		numFrames;			// all surfaces in a model should have the same
	int		numShaders;			// all surfaces in a model should have the same
	int		numVerts;
	int		numTriangles;

	int		ofsTriangles;

	int		ofsShaders;			// offset from start of md3Surface_t
	int		ofsSt;				// texture coords are common for all frames
	int		ofsXyzNormals;		// numVerts * numFrames

	int		ofsEnd;				// next surface follows
} md3Surface_t;

typedef struct 
{
	char			name[MAX_QPATH];
	int				shaderIndex;	// for in-game use
} md3Shader_t;

typedef struct
{
	int			indexes[3];
} md3Triangle_t;

typedef struct 
{
	float		st[2];
} md3St_t;

typedef struct 
{
	short		xyz[3];
	short		normal;
} md3XyzNormal_t;

typedef struct 
{
	int			ident;				// == MD3_IDENT
	int			version;			// == MD3_VERSION

	char		name[MAX_QPATH];	// model name

	int			flags;

	int			numFrames;
	int			numTags;
	int			numSurfaces;

	int			numSkins;

	int			ofsFrames;			// offset for first frame
	int			ofsTags;			// numFrames * numTags
	int			ofsSurfaces;		// first surface, others follow

	int			ofsEnd;				// end of file
} md3Header_t;


/*
========================================================================

.SP2 sprite file format

========================================================================
*/

#define SP2_IDENT			(('2'<<24)+('S'<<16)+('D'<<8)+'I') // little-endian "IDS2"
#define SP2_VERSION			2
#define SP2_MAX_PICNAME		64

typedef struct
{
	int		width, height;
	int		origin_x, origin_y;		// raster coordinates inside pic
	char	name[SP2_MAX_PICNAME];	// name of TGA file
} sp2Frame_t;

typedef struct 
{
	int			ident;
	int			version;
	int			numframes;
	sp2Frame_t	frames[1];			// variable sized
} sp2Header_t;

// key / value pair sizes
#define	MAX_KEY		32
#define	MAX_VALUE	1024

/*
==============================================================================

  .BSP file format

==============================================================================
*/

typedef enum
{
	BSP_MODELS, BSP_BRUSHES, BSP_ENTITIES, BSP_ENTSTRING, BSP_TEXINFO,
	BSP_AREAS, BSP_AREAPORTALS, BSP_PLANES, BSP_NODES, BSP_BRUSHSIDES,
	BSP_LEAFS, BSP_VERTS, BSP_FACES, BSP_LEAFFACES, BSP_LEAFBRUSHES,
	BSP_PORTALS, BSP_EDGES, BSP_SURFEDGES, BSP_LIGHTING, BSP_VISIBILITY, BSP_NUM_DATATYPES
} bspDataType;

//
// REGULAR QUAKE2 BSP FORMAT
//
#define BSP_IDENT		(('P'<<24)+('S'<<16)+('B'<<8)+'I') // little-endian "IBSP"
#define BSP_VERSION	38

// upper design bounds
// leaffaces, leafbrushes, planes, and verts are still bounded by 16 bit short limits
#define	MAX_MAP_MODELS		1024
#define	MAX_MAP_BRUSHES		8192
#define	MAX_MAP_ENTITIES	2048
#define	MAX_MAP_ENTSTRING	0x40000
#define	MAX_MAP_TEXINFO		8192

#define	MAX_MAP_AREAS		256		// the same in qbism
#define	MAX_MAP_AREAPORTALS	1024	// the same in qbism
#define	MAX_MAP_PLANES		65536
#define	MAX_MAP_NODES		65536
#define	MAX_MAP_BRUSHSIDES	65536
#define	MAX_MAP_LEAFS		65536
#define	MAX_MAP_VERTS		65536
#define	MAX_MAP_FACES		65536
#define	MAX_MAP_LEAFFACES	65536
#define	MAX_MAP_LEAFBRUSHES 65536
#define	MAX_MAP_PORTALS		65536
#define	MAX_MAP_EDGES		128000
#define	MAX_MAP_SURFEDGES	256000
#define	MAX_MAP_LIGHTING	0x200000
#define	MAX_MAP_VISIBILITY	0x100000


//
// QBISM EXTENDED QUAKE2 BSP FORMAT
//
#define QBISM_IDENT				 ('Q' | ('B' << 8) | ('S' << 16) | ('P' << 24))

#define MAX_MAP_MODELS_QBSP      131072
#define MAX_MAP_BRUSHES_QBSP     1048576
#define MAX_MAP_ENTITIES_QBSP    131072
#define MAX_MAP_ENTSTRING_QBSP   13631488
#define MAX_MAP_TEXINFO_QBSP     1048576
#define MAX_MAP_PLANES_QBSP      1048576
#define MAX_MAP_NODES_QBSP       1048576
#define MAX_MAP_LEAFS_QBSP       1048576
#define MAX_MAP_VERTS_QBSP       4194304
#define MAX_MAP_FACES_QBSP       1048576
#define MAX_MAP_LEAFFACES_QBSP   1048576
#define MAX_MAP_LEAFBRUSHES_QBSP 1048576
#define MAX_MAP_EDGES_QBSP       1048576
#define MAX_MAP_BRUSHSIDES_QBSP  4194304
#define MAX_MAP_PORTALS_QBSP     1048576
#define MAX_MAP_SURFEDGES_QBSP   4194304
#define MAX_MAP_LIGHTING_QBSP    54525952
#define MAX_MAP_VISIBILITY_QBSP  0x8000000

//=============================================================================

// BSPX extensions to BSP
#define BSPX_IDENT		(('X'<<24)+('P'<<16)+('S'<<8)+'B') // little-endian "BSPX"

typedef struct
{
	unsigned short	width;
	unsigned short	height;
	int		lightofs;		// start of numstyles (from face struct) * (width * height) samples
	float	vecs[2][4];		// this is a world -> lightmap space transformation matrix
} bspx_decoupledlm_t;

typedef struct
{
	char	name[24];	// up to 23 chars, zero-padded
	int		fileofs;	// from file start
	int		filelen;
} bspx_lump_t;

typedef struct
{
	int ident;		// BSPX_IDENT
	int numlumps;	// bspx_lump_t[numlumps]
} bspx_header_t;

//=============================================================================

typedef struct
{
	int		fileofs, filelen;
} lump_t;

// standard lumps
#define	LUMP_ENTITIES		0
#define	LUMP_PLANES			1
#define	LUMP_VERTEXES		2
#define	LUMP_VISIBILITY		3
#define	LUMP_NODES			4
#define	LUMP_TEXINFO		5
#define	LUMP_FACES			6
#define	LUMP_LIGHTING		7
#define	LUMP_LEAFS			8
#define	LUMP_LEAFFACES		9
#define	LUMP_LEAFBRUSHES	10
#define	LUMP_EDGES			11
#define	LUMP_SURFEDGES		12
#define	LUMP_MODELS			13
#define	LUMP_BRUSHES		14
#define	LUMP_BRUSHSIDES		15
#define	LUMP_POP			16
#define	LUMP_AREAS			17
#define	LUMP_AREAPORTALS	18
#define	HEADER_LUMPS		19

typedef struct
{
	int			ident;
	int			version;	
	lump_t		lumps[HEADER_LUMPS];
} dbsp_header_t;

typedef struct
{
	float		mins[3], maxs[3];
	float		origin[3];		// for sounds or lights
	int			headnode;
	int			firstface, numfaces;	// inlineModels just draw faces
										// without walking the bsp tree
} dbsp_model_t;


typedef struct
{
	float	point[3];
} dbsp_vertex_t;


// 0-2 are axial planes
#define	PLANE_X			0
#define	PLANE_Y			1
#define	PLANE_Z			2

// 3-5 are non-axial planes snapped to the nearest
#define	PLANE_ANYX		3
#define	PLANE_ANYY		4
#define	PLANE_ANYZ		5

// planes (x&~1) and (x&~1)+1 are always opposites

typedef struct
{
	float	normal[3];
	float	dist;
	int		type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dbsp_plane_t;


// contents flags are seperate bits
// a given brush can contribute multiple content bits
// multiple brushes can be in a single leaf

// these definitions also need to be in shared.h!

// lower bits are stronger, and will eat weaker brushes completely
#define	CONTENTS_NONE			-1
#define	CONTENTS_EMPTY			0		// air
#define	CONTENTS_SOLID			1		// an eye is never valid in a solid
#define	CONTENTS_WINDOW			2		// translucent, but not watery
#define	CONTENTS_AUX			4		// drawn but not solid
#define	CONTENTS_LAVA			8		// liquid volume
#define	CONTENTS_SLIME			16		// liquid volume
#define	CONTENTS_WATER			32		// liquid volume
#define	CONTENTS_MIST			64		// drawn but not solid
#define	LAST_VISIBLE_CONTENTS	64

// remaining contents are non-visible, and don't eat brushes
#define	CONTENTS_AREAPORTAL		0x8000	// func_areaportal

#define	CONTENTS_PLAYERCLIP		0x10000 // everything but players can pass through
#define	CONTENTS_MONSTERCLIP	0x20000 // everything but actors can pass through

// currents can be added to any other contents, and may be mixed
#define	CONTENTS_CURRENT_0		0x40000
#define	CONTENTS_CURRENT_90		0x80000
#define	CONTENTS_CURRENT_180	0x100000
#define	CONTENTS_CURRENT_270	0x200000
#define	CONTENTS_CURRENT_UP		0x400000
#define	CONTENTS_CURRENT_DOWN	0x800000

#define	CONTENTS_ORIGIN			0x1000000	// removed before bsping an entity

#define	CONTENTS_MONSTER		0x2000000	// should never be on a brush, only in game
#define	CONTENTS_DEADMONSTER	0x4000000	// can walk through but shots are blocked
#define	CONTENTS_DETAIL			0x8000000	// brushes to be added after vis leafs
#define	CONTENTS_TRANSLUCENT	0x10000000	// auto set if any surface has trans
#define	CONTENTS_LADDER			0x20000000	// player can climb it

#define	SURF_LIGHT		0x1		// value will hold the light strength
#define	SURF_SLICK		0x2		// effects game physics
#define	SURF_SKY		0x4		// don't draw, but add to skybox
#define	SURF_WARP		0x8		// turbulent water warp
#define	SURF_TRANS33	0x10	// 33% alpha
#define	SURF_TRANS66	0x20	// 66% alpha
#define	SURF_FLOWING	0x40	// scroll towards S coord
#define	SURF_NODRAW		0x80	// don't bother referencing the texture
#define	SURF_HINT		0x100	// make a primary bsp splitter
#define	SURF_SKIP		0x200	// completely ignore, allowing non-closed brushes

// ericw_tools additional surface flags
#define SURF_ALPHATEST	(1 << 25) // alpha test flag
#define SURF_N64_UV		(1 << 28) // N64 UV and surface flag hack
#define SURF_SCROLLX	(1 << 29) // slow x scroll
#define SURF_SCROLLY	(1 << 30) // slow y scroll
#define SURF_SCROLLFLIP	(1 << 31) // flip scroll directon


typedef struct
{
	int			planenum;
	int			children[2];	// negative numbers are -(leafs+1), not nodes
	short		mins[3];		// for frustom culling
	short		maxs[3];
	unsigned short	firstface;
	unsigned short	numfaces;	// counting both sides
} dbsp_node_t;

typedef struct
{
	int			planenum;
	int			children[2];	// negative numbers are -(leafs+1), not nodes
	float		mins[3];		// for frustom culling
	float		maxs[3];
	unsigned int firstface;
	unsigned int numfaces; // counting both sides
} dbsp_node_ext_t;	// QBISM BSP


typedef struct texinfo_s
{
	float		vecs[2][4];		// [s/t][xyz offset]
	int			flags;			// miptex flags + overrides
	int			value;			// light emission, etc
	char		texture[32];	// texture name (textures/*.tga)
	int			nexttexinfo;	// for animations, -1 = end of chain
} dbsp_texinfo_t;


// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct
{
	unsigned short	v[2];		// vertex numbers
} dbsp_edge_t;

typedef struct
{
	unsigned int v[2]; // vertex numbers
} dbsp_edge_ext_t; // QBISM BSP


#define	MAX_LIGHTMAPS_PER_SURFACE	4 // this is also max lightstyles for surf
typedef struct
{
	unsigned short	planenum;
	short		side;

	int			firstedge;		// we must support > 64k edges
	short		numedges;	
	short		texinfo;

// lighting info
	byte		styles[MAX_LIGHTMAPS_PER_SURFACE];
	int			lightofs;		// start of [numstyles*surfsize] samples
} dbsp_face_t;

typedef struct
{
	unsigned int planenum;
	int			side;

	int			firstedge; // we must support > 64k edges
	int			numedges;
	int			texinfo;

	// lighting info
	byte		styles[MAX_LIGHTMAPS_PER_SURFACE];
	int			lightofs; // start of [numstyles*surfsize] samples
} dbsp_face_ext_t; // QBISM BSP


typedef struct
{
	int				contents;			// OR of all brushes (not needed?)

	short			cluster;
	short			area;

	short			mins[3];			// for frustum culling
	short			maxs[3];

	unsigned short	firstleafface;
	unsigned short	numleaffaces;

	unsigned short	firstleafbrush;
	unsigned short	numleafbrushes;
} dbsp_leaf_t;

typedef struct
{
	int				contents; // OR of all brushes (not needed?)

	int				cluster;
	int				area;

	float			mins[3]; // for frustum culling
	float			maxs[3];

	unsigned int	firstleafface;
	unsigned int	numleaffaces;

	unsigned int	firstleafbrush;
	unsigned int	numleafbrushes;
} dbsp_leaf_ext_t; // QBISM BSP


typedef struct
{
	unsigned short	planenum;		// facing out of the leaf
	short			texinfo;
} dbsp_brushside_t;

typedef struct
{
	unsigned int	planenum; // facing out of the leaf
	int				texinfo;
} dbsp_brushside_ext_t; // QBISM BSP

typedef struct
{
	int			firstside;
	int			numsides;
	int			contents;
} dbrush_t;

//#define	ANGLE_UP	-1 // braxi -- unused
//#define	ANGLE_DOWN	-2 // braxi -- unused


// the visibility lump consists of a header with a count, then
// byte offsets for the PVS and PHS of each cluster, then the raw
// compressed bit vectors
#define	DVIS_PVS	0
#define	DVIS_PHS	1
#define	DVIS_NUM	2

typedef struct
{
	int			numclusters;
	int			bitofs[8][DVIS_NUM];	// bitofs[numclusters][DVIS_NUM]
} dbsp_vis_t;

// each area has a list of portals that lead into other areas
// when portals are closed, other areas may not be visible or
// hearable even if the vis info says that it should be
typedef struct
{
	int		portalnum;
	int		otherarea;
} dbsp_areaportal_t;

typedef struct
{
	int		numareaportals;
	int		firstareaportal;
} dbsp_area_t;
