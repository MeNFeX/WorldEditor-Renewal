#ifndef __TERRAIn_TYPES__
#define __TERRAIn_TYPES__

#include "../eterlib/GrpVertexBuffer.h"
#include "../eterlib/GrpIndexBuffer.h"
#include <cmath>

#define TERRAIN_PATCHSIZE	16
#define TERRAIN_SIZE		128
#define TERRAIN_PATCHCOUNT	TERRAIN_SIZE/TERRAIN_PATCHSIZE
#define MAXTERRAINTEXTURES	256

typedef struct STerainSplat
{
	long					Active;
	long					NeedsUpdate;
	LPDIRECT3DTEXTURE9		pd3dTexture;
} TTerainSplat;

typedef struct STerrainSplatPatch
{
 	DWORD			TileCount[MAXTERRAINTEXTURES];
	DWORD			PatchTileCount[TERRAIN_PATCHCOUNT*TERRAIN_PATCHCOUNT][MAXTERRAINTEXTURES];
	TTerainSplat 	Splats[MAXTERRAINTEXTURES];
	bool			m_bNeedsUpdate;
} TTerrainSplatPatch;

typedef struct STERRAIN_VBUFFER
{
	char					used;
	short					mat;

	CGraphicVertexBuffer	vb;
	CGraphicIndexBuffer		ib;
	long					VertexSize;

	short					NumIndices;

	float					minx, maxx;
	float					miny, maxy;
	float					minz, maxz;
} TERRAIN_VBUFFER;

typedef struct SPR_MATERIAL
{
	char name[19];
	float ambi_r, ambi_g, ambi_b, ambi_a;		/* Ambient Color */
	float diff_r, diff_g, diff_b, diff_a;		/* Diffuse Color */
	float spec_r, spec_g, spec_b, spec_a;		/* Specular Color */
	float spec_power;							/* Specular power */
} PR_MATERIAL;

typedef struct STerrainGlobals
{
	/* Public Settings */
	float			PageUVLength;
	long			SquaresPerTexture;              /* Heightfield squares per texture (128 texels) */
	long			SplatTilesX;					/* Number of splat textures across map */
	long			SplatTilesY;					/* Number of splat textures down map */
	long			DisableWrapping;
	long			DisableShadow;
	long			ShadowMode;
	long			OutsideVisible;
	D3DXVECTOR3		SunLocation;
} TTerrainGlobals;

/* x87 FISTP is 32-bit only. On x64 the same rounding is done in C++:
   PR_FLOAT_TO_INT floors, the other two round to nearest. */
#if defined(_WIN64)
#define PR_FLOAT_TO_FIXED(inreal, outint) \
	(outint) = static_cast<long>(std::lround(static_cast<double>(inreal) * 65536.0))

#define PR_FLOAT_TO_INT(inreal, outint) \
	(outint) = static_cast<long>(std::floor(static_cast<double>(inreal)))

#define PR_FLOAT_ADD_TO_INT(inreal, outint) \
	(outint) += static_cast<long>(std::lround(static_cast<double>(inreal)))
#else
#define PR_FLOAT_TO_INTASM __asm	\
{									\
	__asm fld PR_FCNV				\
	__asm fistp PR_ICNV				\
}

#define PR_FLOAT_TO_FIXED(inreal, outint)	\
{											\
	PR_FCNV = (inreal) * 65536.0f;			\
    PR_FLOAT_TO_INTASM;						\
	(outint) = PR_ICNV;						\
}

#define PR_FLOAT_TO_INT(inreal, outint)							\
{																\
	PR_FCNV = (inreal);											\
    PR_FLOAT_TO_INTASM;											\
	(outint) = PR_ICNV > PR_FCNV ? PR_ICNV - 1 : PR_ICNV;		\
}

#define PR_FLOAT_ADD_TO_INT(inreal, outint)	\
{											\
	PR_FCNV = (inreal);						\
    PR_FLOAT_TO_INTASM;						\
	(outint) += PR_ICNV;					\
}
#endif

#endif //__TERRAIn_TYPES__

