#pragma once
#include <map>
#include "SpeedTreeForest.h"
#include "SpeedTreeMaterial.h"

class CSpeedTreeDirectX : public CSpeedTreeForest, public CGraphicBase
{
public:
	CSpeedTreeDirectX();
	~CSpeedTreeDirectX();

	void                        UploadWindMatrix(unsigned int uiLocation, const float* pMatrix) const;
	bool						SetRenderingDevice();
	void                        Render(unsigned long ulRenderBitVector = Forest_RenderAll);
	void						UpdateCompundMatrix(const D3DXVECTOR3& c_rEyeVec, const D3DXMATRIX& c_rmatView, const D3DXMATRIX& c_rmatProj);
#ifdef WORLD_EDITOR
	static void					SetVisibleTreeCount(unsigned int uiCount);
	static unsigned int			GetVisibleTreeCount();
	static bool					IsSphereInCamera(float x, float y, float z, float fRadius);
#endif

private:
	bool                        InitVertexShaders();

private:
	LPDIRECT3DVERTEXSHADER9     m_dwBranchVertexShader;
	LPDIRECT3DVERTEXSHADER9     m_dwLeafVertexShader;
};

