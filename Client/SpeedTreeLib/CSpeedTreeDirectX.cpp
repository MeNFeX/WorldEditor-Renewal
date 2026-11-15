#include "StdAfx.h"
#include "CSpeedTreeDirectX.h"
#include <d3d9.h>
#include <d3d9types.h>
#include <d3dx9.h>
#include "VertexShaders.h"
#include "../eterBase/Timer.h"
#include "../eterLib/StateManager.h"
#include "../eterLib/Camera.h"
#include "../eterLib/EditorFrameLog.h"

CSpeedTreeDirectX::CSpeedTreeDirectX() : m_dwBranchVertexShader(nullptr), m_dwLeafVertexShader(nullptr) {}

CSpeedTreeDirectX::~CSpeedTreeDirectX()
{
	SAFE_RELEASE(m_dwBranchVertexShader);
	SAFE_RELEASE(m_dwLeafVertexShader);
}

#ifdef WORLD_EDITOR
static unsigned int s_uiVisibleTrees = 0;

void CSpeedTreeDirectX::SetVisibleTreeCount(unsigned int uiCount)
{
	s_uiVisibleTrees = uiCount;
}

unsigned int CSpeedTreeDirectX::GetVisibleTreeCount()
{
	return s_uiVisibleTrees;
}

static bool s_bEditorCullTrees = false;
static float s_editorPlanes[6][4];

static void EditorNormalizePlane(float* p)
{
	const float fLen = sqrtf(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
	if (fLen <= 0.00001f)
		return;
	const float fInv = 1.0f / fLen;
	p[0] *= fInv;
	p[1] *= fInv;
	p[2] *= fInv;
	p[3] *= fInv;
}

static bool EditorPointInside(float x, float y, float z, float fPad)
{
	for (int i = 0; i < 6; ++i)
	{
		const float fDist = s_editorPlanes[i][0] * x + s_editorPlanes[i][1] * y + s_editorPlanes[i][2] * z + s_editorPlanes[i][3];
		if (fDist < -fPad)
			return false;
	}
	return true;
}

static void EditorBuildTreePlanes(const D3DXMATRIX& rView, const D3DXMATRIX& rProj)
{
	D3DXMATRIX clip;
	D3DXMatrixMultiply(&clip, &rView, &rProj);
	s_editorPlanes[0][0] = clip._14 + clip._11; s_editorPlanes[0][1] = clip._24 + clip._21; s_editorPlanes[0][2] = clip._34 + clip._31; s_editorPlanes[0][3] = clip._44 + clip._41;
	s_editorPlanes[1][0] = clip._14 - clip._11; s_editorPlanes[1][1] = clip._24 - clip._21; s_editorPlanes[1][2] = clip._34 - clip._31; s_editorPlanes[1][3] = clip._44 - clip._41;
	s_editorPlanes[2][0] = clip._14 + clip._12; s_editorPlanes[2][1] = clip._24 + clip._22; s_editorPlanes[2][2] = clip._34 + clip._32; s_editorPlanes[2][3] = clip._44 + clip._42;
	s_editorPlanes[3][0] = clip._14 - clip._12; s_editorPlanes[3][1] = clip._24 - clip._22; s_editorPlanes[3][2] = clip._34 - clip._32; s_editorPlanes[3][3] = clip._44 - clip._42;
	s_editorPlanes[4][0] = clip._13;            s_editorPlanes[4][1] = clip._23;            s_editorPlanes[4][2] = clip._33;            s_editorPlanes[4][3] = clip._43;
	s_editorPlanes[5][0] = clip._14 - clip._13; s_editorPlanes[5][1] = clip._24 - clip._23; s_editorPlanes[5][2] = clip._34 - clip._33; s_editorPlanes[5][3] = clip._44 - clip._43;
	for (int i = 0; i < 6; ++i)
		EditorNormalizePlane(s_editorPlanes[i]);

	s_bEditorCullTrees = true;
	const CCamera * pCamera = CCameraManager::Instance().GetCurrentCamera();
	if (!pCamera)
	{
		s_bEditorCullTrees = false;
		return;
	}
	const D3DXVECTOR3 vTarget = pCamera->GetTarget();
	if (!EditorPointInside(vTarget.x, vTarget.y, vTarget.z, 2000.0f))
		s_bEditorCullTrees = false;
}

static bool EditorTreeShown(CSpeedTreeWrapper* pTree)
{
	if (!pTree || !pTree->isShow())
		return false;
	if (!s_bEditorCullTrees)
		return true;

	D3DXVECTOR3 vCenter;
	float fRadius = 0.0f;
	if (!pTree->GetBoundingSphere(vCenter, fRadius))
		return true;
	fRadius = fRadius * 1.15f + 1000.0f;
	return EditorPointInside(vCenter.x, vCenter.y, vCenter.z, fRadius);
}

bool CSpeedTreeDirectX::IsSphereInCamera(float x, float y, float z, float fRadius)
{
	if (fRadius < 0.0f)
		fRadius = 0.0f;

	D3DXVECTOR3 vCenter (x, y, z);
	D3DXMATRIX matViewProj;
	D3DXMatrixMultiply (&matViewProj, &ms_matView, &ms_matProj);
	D3DXVECTOR4 vClip;
	D3DXVec3Transform (&vClip, &vCenter, &matViewProj);
	if (vClip.w <= 1.0f)
	{
		const CCamera * pCamera = CCameraManager::Instance().GetCurrentCamera();
		if (!pCamera)
			return true;
		const D3DXVECTOR3 vEye = pCamera->GetEye();
		const float fDx = x - vEye.x;
		const float fDy = y - vEye.y;
		const float fDz = z - vEye.z;
		const float fReach = fRadius + 300.0f;
		return (fDx * fDx + fDy * fDy + fDz * fDz) <= (fReach * fReach);
	}

	const float fInv = 1.0f / vClip.w;
	const float fSx = vClip.x * fInv;
	const float fSy = vClip.y * fInv;
	const float fSz = vClip.z * fInv;
	float fMargin = fRadius * fInv;
	if (fMargin < 0.0f)
		fMargin = -fMargin;
	if (fMargin > 1.5f)
		fMargin = 1.5f;

	return fSx >= -1.0f - fMargin && fSx <= 1.0f + fMargin && fSy >= -1.0f - fMargin && fSy <= 1.0f + fMargin && fSz >= -0.1f && fSz <= 1.05f + fMargin;
}
#endif

void CSpeedTreeDirectX::UploadWindMatrix(unsigned int uiLocation, const float* pMatrix) const
{
	STATEMANAGER.SetVertexShaderConstant(uiLocation, pMatrix, 4);
}

void CSpeedTreeDirectX::UpdateCompundMatrix(const D3DXVECTOR3& c_rEyeVec, const D3DXMATRIX& c_rmatView, const D3DXMATRIX& c_rmatProj)
{
	D3DXMATRIX matBlend;
	D3DXMatrixIdentity(&matBlend);

	D3DXMATRIX matBlendShader;
	D3DXMatrixMultiply(&matBlendShader, &c_rmatView, &c_rmatProj);

	float afDirection[3];
#ifdef WORLD_EDITOR
	const CCamera * pCamera = CCameraManager::Instance().GetCurrentCamera();
	D3DXVECTOR3 vDirection = pCamera ? pCamera->GetView() : D3DXVECTOR3(matBlendShader.m[0][2], matBlendShader.m[1][2], matBlendShader.m[2][2]);
	const float fLength = D3DXVec3Length(&vDirection);
	if (fLength > 0.0001f)
		vDirection /= fLength;
	if (vDirection.z > 1.0f)
		vDirection.z = 1.0f;
	else if (vDirection.z < -1.0f)
		vDirection.z = -1.0f;
	afDirection[0] = vDirection.x;
	afDirection[1] = vDirection.y;
	afDirection[2] = vDirection.z;
#else
	afDirection[0] = matBlendShader.m[0][2];
	afDirection[1] = matBlendShader.m[1][2];
	afDirection[2] = matBlendShader.m[2][2];
#endif
	CSpeedTreeRT::SetCamera(c_rEyeVec, afDirection);

	D3DXMatrixTranspose(&matBlendShader, &matBlendShader);
	STATEMANAGER.SetVertexShaderConstant(c_nVertexShader_CompoundMatrix, (const float*)&matBlendShader, 4);
}

bool CSpeedTreeDirectX::SetRenderingDevice()
{
	if (!InitVertexShaders())
		return false;

	const float c_afLightPosition[4] = { -0.707f, -0.300f, 0.707f, 0.0f };
	const float	c_afLightAmbient[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
	const float	c_afLightDiffuse[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	const float	c_afLightSpecular[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float afLight1[] =
	{
		c_afLightPosition[0], c_afLightPosition[1], c_afLightPosition[2],
		c_afLightDiffuse[0], c_afLightDiffuse[1], c_afLightDiffuse[2],
		c_afLightAmbient[0], c_afLightAmbient[1], c_afLightAmbient[2],
		c_afLightSpecular[0], c_afLightSpecular[1], c_afLightSpecular[2],
		c_afLightPosition[3],
		1.0f, 0.0f, 0.0f
	};

	CSpeedTreeRT::SetLightAttributes(0, afLight1);
	CSpeedTreeRT::SetLightState(0, true);
	return true;
}

void CSpeedTreeDirectX::Render(unsigned long ulRenderBitVector)
{
	if (m_pMainTreeMap.empty())
		return;

	if (!(ulRenderBitVector & Forest_RenderToShadow) && !(ulRenderBitVector & Forest_RenderToMiniMap))
		UpdateCompundMatrix(CCameraManager::Instance().GetCurrentCamera()->GetEye(), ms_matView, ms_matProj);
#ifdef WORLD_EDITOR
	s_bEditorCullTrees = false;
	if (!(ulRenderBitVector & Forest_RenderToShadow) && !(ulRenderBitVector & Forest_RenderToMiniMap))
		EditorBuildTreePlanes(ms_matView, ms_matProj);
#endif

	DWORD dwLightState = STATEMANAGER.GetRenderState(D3DRS_LIGHTING);
	DWORD dwColorVertexState = STATEMANAGER.GetRenderState(D3DRS_COLORVERTEX);
	DWORD dwFogVertexMode = STATEMANAGER.GetRenderState(D3DRS_FOGVERTEXMODE);

	STATEMANAGER.SetRenderState(D3DRS_LIGHTING, FALSE);
	STATEMANAGER.SetRenderState(D3DRS_COLORVERTEX, TRUE);
	EditorFrameMark("tree.prep");

	UINT uiCount;
	TTreeMap::const_iterator itor = m_pMainTreeMap.begin();

	while (itor != m_pMainTreeMap.end())
	{
		CSpeedTreeWrapper* pMainTree = (itor++)->second;
		CSpeedTreeWrapper** ppInstances = pMainTree->GetInstances(uiCount);

		for (UINT i = 0; i < uiCount; ++i)
		{
			EditorFrameCount(EDITOR_COUNT_TREE);
#ifdef WORLD_EDITOR
			if (!EditorTreeShown(ppInstances[i]))
				continue;
#endif
			if (ppInstances[i]->isShow())
				EditorFrameCount(EDITOR_COUNT_TREE_ON);
			ppInstances[i]->Advance();
		}
	}
	EditorFrameMark("tree.advance");

	STATEMANAGER.SetVertexShaderConstant(c_nVertexShader_Light, m_afLighting, 3);
	STATEMANAGER.SetVertexShaderConstant(c_nVertexShader_Fog, m_afFog, 1);

	if (ulRenderBitVector & Forest_RenderToShadow)
	{
		STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	}
	else
	{
		STATEMANAGER.SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		STATEMANAGER.SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		STATEMANAGER.SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		STATEMANAGER.SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		STATEMANAGER.SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		STATEMANAGER.SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
#ifdef WORLD_EDITOR
		STATEMANAGER.SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_POINT);
#else
		STATEMANAGER.SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_ANISOTROPIC);
#endif

		STATEMANAGER.SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		STATEMANAGER.SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
		STATEMANAGER.SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
		STATEMANAGER.SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
		STATEMANAGER.SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
		STATEMANAGER.SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
	}

	STATEMANAGER.SaveRenderState(D3DRS_ALPHATESTENABLE, TRUE);
	STATEMANAGER.SaveRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
	STATEMANAGER.SaveRenderState(D3DRS_CULLMODE, D3DCULL_CW);

	if (STATEMANAGER.GetRenderState(D3DRS_FOGENABLE)){}

	STATEMANAGER.SetFVF(D3DFVF_SPEEDTREE_BRANCH_VERTEX);
	STATEMANAGER.SetVertexShader(m_dwBranchVertexShader);

	if (ulRenderBitVector & Forest_RenderBranches)
	{
		itor = m_pMainTreeMap.begin();

		while (itor != m_pMainTreeMap.end())
		{
			CSpeedTreeWrapper* pMainTree = (itor++)->second;
			CSpeedTreeWrapper** ppInstances = pMainTree->GetInstances(uiCount);

			pMainTree->SetupBranchForTreeType();
#ifdef WORLD_EDITOR
			pMainTree->BeginEditorGeometry(0);
#endif

			for (UINT i = 0; i < uiCount; ++i)
#ifdef WORLD_EDITOR
				if (EditorTreeShown(ppInstances[i]))
#else
				if (ppInstances[i]->isShow())
#endif
					ppInstances[i]->RenderBranches();
		}
	}
	EditorFrameMark("tree.branch");

	STATEMANAGER.SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	if (ulRenderBitVector & Forest_RenderFronds)
	{
		itor = m_pMainTreeMap.begin();

		while (itor != m_pMainTreeMap.end())
		{
			CSpeedTreeWrapper* pMainTree = (itor++)->second;
			CSpeedTreeWrapper** ppInstances = pMainTree->GetInstances(uiCount);

			pMainTree->SetupFrondForTreeType();
#ifdef WORLD_EDITOR
			pMainTree->BeginEditorGeometry(1);
#endif

			for (UINT i = 0; i < uiCount; ++i)
#ifdef WORLD_EDITOR
				if (EditorTreeShown(ppInstances[i]))
#else
				if (ppInstances[i]->isShow())
#endif
					ppInstances[i]->RenderFronds();
		}
	}
	EditorFrameMark("tree.frond");

	if (ulRenderBitVector & Forest_RenderLeaves)
	{
		STATEMANAGER.SetFVF(D3DFVF_SPEEDTREE_LEAF_VERTEX);
		STATEMANAGER.SetVertexShader(m_dwLeafVertexShader);

		if (STATEMANAGER.GetRenderState(D3DRS_FOGENABLE)){}
		if (ulRenderBitVector & Forest_RenderToShadow || ulRenderBitVector & Forest_RenderToMiniMap)
		{
			STATEMANAGER.SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_NOTEQUAL);
			STATEMANAGER.SaveRenderState(D3DRS_ALPHAREF, 0x00000000);
		}

		itor = m_pMainTreeMap.begin();

		while (itor != m_pMainTreeMap.end())
		{
			CSpeedTreeWrapper* pMainTree = (itor++)->second;
			CSpeedTreeWrapper** ppInstances = pMainTree->GetInstances(uiCount);

			pMainTree->SetupLeafForTreeType();
			pMainTree->ResetCpuLeafUpdates();
#ifdef WORLD_EDITOR
			pMainTree->BeginEditorGeometry(2);
#endif

			for (UINT i = 0; i < uiCount; ++i)
#ifdef WORLD_EDITOR
				if (EditorTreeShown(ppInstances[i]))
#else
				if (ppInstances[i]->isShow())
#endif
					ppInstances[i]->RenderLeaves();
		}
		EditorFrameMark("tree.leaf");

		if (ulRenderBitVector & Forest_RenderToShadow || ulRenderBitVector & Forest_RenderToMiniMap)
		{
			STATEMANAGER.SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
			STATEMANAGER.RestoreRenderState(D3DRS_ALPHAREF);
		}
	}

	if (ulRenderBitVector & Forest_RenderBillboards)
	{
		STATEMANAGER.SetVertexShader(nullptr);
		STATEMANAGER.SetFVF(D3DFVF_SPEEDTREE_BILLBOARD_VERTEX);
		STATEMANAGER.SetRenderState(D3DRS_LIGHTING, FALSE);
		STATEMANAGER.SetRenderState(D3DRS_COLORVERTEX, FALSE);

		itor = m_pMainTreeMap.begin();

		while (itor != m_pMainTreeMap.end())
		{
			CSpeedTreeWrapper* pMainTree = (itor++)->second;
			CSpeedTreeWrapper** ppInstances = pMainTree->GetInstances(uiCount);

			pMainTree->SetupBranchForTreeType();
#ifdef WORLD_EDITOR
			pMainTree->BeginEditorGeometry(3);
#endif

			for (UINT i = 0; i < uiCount; ++i)
#ifdef WORLD_EDITOR
				if (EditorTreeShown(ppInstances[i]))
#else
				if (ppInstances[i]->isShow())
#endif
					ppInstances[i]->RenderBillboards();
		}
	}
	EditorFrameMark("tree.board");

	STATEMANAGER.SetRenderState(D3DRS_LIGHTING, dwLightState);
	STATEMANAGER.SetRenderState(D3DRS_COLORVERTEX, dwColorVertexState);
	STATEMANAGER.SetRenderState(D3DRS_FOGVERTEXMODE, dwFogVertexMode);

	if (!(ulRenderBitVector & Forest_RenderToShadow))
	{
		STATEMANAGER.SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
		STATEMANAGER.SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

	}

	STATEMANAGER.RestoreRenderState(D3DRS_ALPHATESTENABLE);
	STATEMANAGER.RestoreRenderState(D3DRS_ALPHAFUNC);
	STATEMANAGER.RestoreRenderState(D3DRS_CULLMODE);
}

bool CSpeedTreeDirectX::InitVertexShaders(void)
{
	m_dwBranchVertexShader = LoadBranchShader(ms_lpd3dDevice);
	m_dwLeafVertexShader = LoadLeafShader(ms_lpd3dDevice);

	return true;
}

