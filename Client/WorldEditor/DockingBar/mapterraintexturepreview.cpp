// mapterraintexturepreview.cpp : implementation file
//

#include "StdAfx.h"
#include "../WorldEditor.h"
#include "mapterraintexturepreview.h"
#include "../DataCtrl/MapAccessorTerrain.h"
#include "../DataCtrl/ImageD3D.h"
#include "../../EterLib/ResourceManager.h"
#include "../../EterPack/EterPackManager.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMapTerrainTexturePreview

CMapTerrainTexturePreview::CMapTerrainTexturePreview()
{
}

CMapTerrainTexturePreview::~CMapTerrainTexturePreview()
{
}

BEGIN_MESSAGE_MAP (CMapTerrainTexturePreview, CWnd)
	//{{AFX_MSG_MAP(CMapTerrainTexturePreview)
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapTerrainTexturePreview message handlers

BOOL CMapTerrainTexturePreview::Create()
{
	CRect rect;
	GetClientRect (&rect);
	if (!m_DIB.Create (NULL, rect.Width(), rect.Height()))
		return FALSE;

	DWORD * pdwDst = (DWORD *) m_DIB.GetPointer();
	if (pdwDst && rect.Width() > 0 && rect.Height() > 0)
		memset (pdwDst, 0, rect.Width() * rect.Height() * 4);
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMapTerrainTexturePreview message handlers

void CMapTerrainTexturePreview::ClearPreview()
{
	DWORD * pdwDst = (DWORD *) m_DIB.GetPointer();
	const int dibW = m_DIB.GetWidth();
	const int dibH = m_DIB.GetHeight();
	if (pdwDst && dibW > 0 && dibH > 0)
		memset (pdwDst, 0, dibW * dibH * 4);
}

void CMapTerrainTexturePreview::UpdatePreview (int nTexNum)
{
	CTextureSet * pTextureSet = CTerrain::GetTextureSet();
	const int dibW = m_DIB.GetWidth();
	const int dibH = m_DIB.GetHeight();
	DWORD * pdwDst = (DWORD *) m_DIB.GetPointer();

	if (nTexNum == 0 || !pdwDst || dibW <= 0 || dibH <= 0 || !ms_lpd3dDevice)
	{
		ClearPreview();
		Invalidate (TRUE);
		return;
	}

	CGraphicImageInstance & rImageInstance = pTextureSet->GetTexture (nTexNum).ImageInstance;
	CGraphicImage * pImage = rImageInstance.GetGraphicImagePointer();
	if (!pImage || !pImage->GetFileName() || !pImage->GetFileName()[0])
	{
		ClearPreview();
		Invalidate (TRUE);
		return;
	}
	CMappedFile kFile;
	LPCVOID pvData = NULL;
	if (!CEterPackManager::Instance().Get (kFile, pImage->GetFileName(), &pvData) || !pvData || kFile.Size() <= 0)
	{
		TraceError ("The file [%s] doesn't exist", pImage->GetFileName());
		ClearPreview();
		Invalidate (TRUE);
		return;
	}

	LPDIRECT3DSURFACE9 surface = NULL;
	if (!ImageLoadMemory (ms_lpd3dDevice, pvData, (UINT) kFile.Size(), dibW, dibH, &surface))
	{
		TraceError ("The file [%s] could not be decoded", pImage->GetFileName());
		ClearPreview();
		Invalidate (TRUE);
		return;
	}

	D3DLOCKED_RECT lock;
	if (SUCCEEDED (surface->LockRect (&lock, NULL, D3DLOCK_READONLY)) && lock.pBits)
	{
		for (int y = 0; y < dibH; ++y)
		{
			const DWORD * row = (const DWORD *) ((const BYTE *) lock.pBits + y * lock.Pitch);
			for (int x = 0; x < dibW; ++x)
				pdwDst[x + y * dibW] = (row[x] & 0x00ffffff) | 0xff000000;
		}
		surface->UnlockRect();
	}
	else
	{
		ClearPreview();
	}
	surface->Release();
	Invalidate (TRUE);
}

void CMapTerrainTexturePreview::OnPaint()
{
	CPaintDC dc (this);

	if (m_DIB.GetPointer())
		m_DIB.Put (dc.GetSafeHdc(), 0, 0);
}
