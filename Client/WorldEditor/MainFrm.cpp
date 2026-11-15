// MainFrm.cpp : implementation of the CMainFrame class
//

#include "StdAfx.h"
#include <math.h>
#include "WorldEditor.h"
#include "WorldEditorDoc.h"
#include "WorldEditorView.h"
#include "MainFrm.h"
#include "DataCtrl/MapAccessorOutdoor.h"
#include "Util.h"
#include "InterfaceTheme.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

int iStartUpPage = ID_VIEW_MAP;
//int iStartUpPage = ID_VIEW_OBJECT;
//int iStartUpPage = ID_VIEW_EFFECT;

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE (CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP (CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND (ID_EDIT_UNDO, OnUndo)
	ON_COMMAND (ID_EDIT_REDO, OnRedo)
	ON_COMMAND_RANGE (ID_INTERFACE_COLOR_LIGHT, ID_INTERFACE_COLOR_SLATE, OnInterfaceColor)
	ON_UPDATE_COMMAND_UI_RANGE (ID_INTERFACE_COLOR_LIGHT, ID_INTERFACE_COLOR_SLATE, OnUpdateInterfaceColor)
	ON_COMMAND (ID_INTERFACE_CUSTOM_PANEL, OnInterfaceCustomPanel)
	ON_COMMAND (ID_INTERFACE_CUSTOM_TEXT, OnInterfaceCustomText)
	ON_COMMAND_RANGE (ID_INTERFACE_FONT_DEFAULT, ID_INTERFACE_FONT_GEORGIA, OnInterfaceFont)
	ON_UPDATE_COMMAND_UI_RANGE (ID_INTERFACE_FONT_DEFAULT, ID_INTERFACE_FONT_GEORGIA, OnUpdateInterfaceFont)
	ON_COMMAND_RANGE (ID_INTERFACE_SIZE_DEFAULT, ID_INTERFACE_SIZE_12, OnInterfaceFontSize)
	ON_UPDATE_COMMAND_UI_RANGE (ID_INTERFACE_SIZE_DEFAULT, ID_INTERFACE_SIZE_12, OnUpdateInterfaceFontSize)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

static UINT auiStatusIndicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_OVR,       // sector folder, 000000
	ID_INDICATOR_OVR,       // auto-save, starts as N/A
	ID_INDICATOR_OVR,       // meters
	ID_INDICATOR_OVR,       // sector X / Y
	ID_INDICATOR_OVR,       // frames per second
	ID_INDICATOR_OVR,       // frame time in milliseconds
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	m_strSaveStatus = "N/A";
	m_fFrameMs = 0.0f;
	timeBeginPeriod (1);
}

CMainFrame::~CMainFrame()
{
	DestroyBackGroundData();
	timeEndPeriod (1);
}

int CMainFrame::OnCreate (LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate (lpCreateStruct) == -1)
	{
		return -1;
	}

	if (!CreateStatusBar())
	{
		TRACE0 ("Failed to create toolbar\n");
		return -1;
	}

	if (!CreateToolBar())
	{
		TRACE0 ("Failed to create toolbar\n");
		return -1;
	}

	if (!CreateDockingBar())
	{
		TRACE0 ("Failed to create docking bar.\n");
		return -1;
	}

#if defined(_WIN64)
#define WORLDEDITOR_ARCH "x64"
#else
#define WORLDEDITOR_ARCH "Win32"
#endif

#ifdef _DEBUG
	m_strTitle = "World Editor Renewal 1.0 [WE-Debug | " WORLDEDITOR_ARCH " | " __DATE__ " " __TIME__ "]";
#else
	m_strTitle = "World Editor Renewal 1.0 [WE-Release | " WORLDEDITOR_ARCH " | " __DATE__ " " __TIME__ "]";
#endif

	InterfaceThemeApplyStartup();
	return 0;
}

void CMainFrame::OnInterfaceColor (UINT nId)
{
	InterfaceThemeSetColor (nId);
}

void CMainFrame::OnInterfaceCustomPanel()
{
	InterfaceThemePickCustom (false);
}

void CMainFrame::OnInterfaceCustomText()
{
	InterfaceThemePickCustom (true);
}

void CMainFrame::OnUpdateInterfaceColor (CCmdUI * pCmdUI)
{
	pCmdUI->SetCheck (pCmdUI->m_nID == InterfaceThemeColor());
}

void CMainFrame::OnInterfaceFont (UINT nId)
{
	InterfaceThemeSetFont (nId);
}

void CMainFrame::OnUpdateInterfaceFont (CCmdUI * pCmdUI)
{
	pCmdUI->SetCheck (pCmdUI->m_nID == InterfaceThemeFont());
}

void CMainFrame::OnInterfaceFontSize (UINT nId)
{
	InterfaceThemeSetSize (nId);
}

void CMainFrame::OnUpdateInterfaceFontSize (CCmdUI * pCmdUI)
{
	pCmdUI->SetCheck (pCmdUI->m_nID == InterfaceThemeSize());
}

void CMainFrame::PlayEffect()
{
	m_wndEffectCtrlBar.m_pdlgTimeBar->Play();
}

BOOL CMainFrame::PreCreateWindow (CREATESTRUCT& cs)
{
	if (!CFrameWnd::PreCreateWindow (cs))
	{
		return FALSE;
	}
	/*
	cs.x = 0;
	cs.y = 0;
	cs.cx = 1032;
	cs.cy = 877;
	*/
	cs.x = 0;
	cs.y = 0;
	cs.cx = 1248;
	cs.cy = 877;
	// viewport 1024x768 가 되려면 위의 설정
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump (CDumpContext& dc) const
{
	CFrameWnd::Dump (dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame normal functions

BOOL CMainFrame::CreateStatusBar()
{
	if (!m_wndStatusBar.Create (this) ||
		!m_wndStatusBar.SetIndicators (auiStatusIndicators, sizeof (auiStatusIndicators) / sizeof (UINT)))
	{
		return FALSE;
	}

	m_wndStatusBar.SetPaneText (1, "", TRUE);
	m_wndStatusBar.SetPaneText (2, m_strSaveStatus, TRUE);
	m_wndStatusBar.SetPaneText (3, "", TRUE);
	m_wndStatusBar.SetPaneText (4, "", TRUE);
	m_wndStatusBar.SetPaneText (5, "0 fps", TRUE);
	m_wndStatusBar.SetPaneText (6, "0.0 ms", TRUE);

	CRect rc;
	m_wndStatusBar.GetClientRect (&rc);
	if (rc.Width() > 0)
		m_wndStatusBar.FitToWidth (rc.Width());
	return TRUE;
}

BEGIN_MESSAGE_MAP (CEditorStatusBar, CStatusBar)
	ON_WM_SIZE()
END_MESSAGE_MAP()

void CEditorStatusBar::OnSize (UINT nType, int cx, int cy)
{
	if (nType != SIZE_MINIMIZED && cx > 0 && !m_bFitting)
	{
		m_bFitting = true;
		FitToWidth (cx);
		m_bFitting = false;
	}
	CStatusBar::OnSize (nType, cx, cy);
}

void CEditorStatusBar::FitToWidth (int cx)
{
	if (m_nCount < 7)
		return;

	int rgBorders[3] = { 0, 0, 2 };
	DefWindowProc (SB_GETBORDERS, 0, (LPARAM) rgBorders);
	const int iPanePad = 6 + rgBorders[2];
	CWnd * pParent = GetParent();
	const int iGrip = (pParent && (pParent->GetStyle() & WS_THICKFRAME)) ? ::GetSystemMetrics (SM_CXVSCROLL) : 0;
	int iBudget = cx - rgBorders[0] - iGrip - (7 * iPanePad) - 40;
	if (iBudget < 48)
		iBudget = 48;

	const int kPrefer[6] = { 72, 200, 176, 52, 72, 78 };
	const int kMin[6] = { 52, 28, 110, 36, 54, 58 };
	int aiWidth[6];
	int iPreferSum = 0;
	int iMinSum = 0;
	for (int i = 0; i < 6; ++i)
	{
		aiWidth[i] = kPrefer[i];
		iPreferSum += kPrefer[i];
		iMinSum += kMin[i];
	}

	if (iBudget >= iPreferSum)
	{
		int iExtra = iBudget - iPreferSum;
		if (iExtra > 120)
			iExtra = 120;
		aiWidth[1] += iExtra;
	}
	else if (iBudget >= iMinSum)
	{
		int iCut = iPreferSum - iBudget;
		const int iSaveSlack = kPrefer[1] - kMin[1];
		if (iCut > 0)
		{
			const int iTake = (iCut < iSaveSlack) ? iCut : iSaveSlack;
			aiWidth[1] -= iTake;
			iCut -= iTake;
		}
		const int iFolderSlack = kPrefer[0] - kMin[0];
		if (iCut > 0 && iFolderSlack > 0)
		{
			const int iTake = (iCut < iFolderSlack) ? iCut : iFolderSlack;
			aiWidth[0] -= iTake;
			iCut -= iTake;
		}
		if (iCut > 0)
		{
			int iRestPrefer = 0;
			int iRestMin = 0;
			for (int i = 2; i < 6; ++i)
			{
				iRestPrefer += kPrefer[i];
				iRestMin += kMin[i];
			}
			const int iRestBudget = (iRestPrefer - iCut > iRestMin) ? (iRestPrefer - iCut) : iRestMin;
			for (int i = 2; i < 6; ++i)
				aiWidth[i] = (iRestPrefer > 0) ? (kPrefer[i] * iRestBudget / iRestPrefer) : kMin[i];
		}
	}
	else
	{
		for (int i = 0; i < 6; ++i)
			aiWidth[i] = (iPreferSum > 0) ? (kPrefer[i] * iBudget / iPreferSum) : 8;
	}

	int iSum = 0;
	for (int i = 0; i < 6; ++i)
	{
		if (aiWidth[i] < 8)
			aiWidth[i] = 8;
		iSum += aiWidth[i];
	}
	if (iSum > iBudget && aiWidth[1] > 8)
	{
		aiWidth[1] -= iSum - iBudget;
		if (aiWidth[1] < 8)
			aiWidth[1] = 8;
	}

	for (int i = 0; i < 6; ++i)
	{
		UINT uiID = 0;
		UINT uiStyle = 0;
		int iWidth = 0;
		GetPaneInfo (i + 1, uiID, uiStyle, iWidth);
		if (iWidth != aiWidth[i])
			SetPaneInfo (i + 1, uiID, uiStyle, aiWidth[i]);
	}
}

BOOL CMainFrame::CreateToolBar()
{
	if (!m_wndFileToolBar.CreateEx (this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
									| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC))
	{
		return FALSE;
	}
	if (!m_wndFileToolBar.LoadToolBar (IDR_MAINFRAME))
	{
		return FALSE;
	}

	m_wndFileToolBar.ShowButtonText (FALSE);
	m_wndFileToolBar.EnableDocking (CBRS_ALIGN_ANY);
	EnableDocking (CBRS_ALIGN_ANY);
	DockControlBar (&m_wndFileToolBar);

	return TRUE;
}

BOOL CMainFrame::CreateDockingBar()
{
	if (!m_wndMapCtrlBar.Create (this))
	{
		return FALSE;
	}
	if (!m_wndObjectCtrlBar.Create (this))
	{
		return FALSE;
	}
	if (!m_wndEffectCtrlBar.Create (this))
	{
		return FALSE;
	}
	if (!m_wndFlyCtrlBar.Create (this))
	{
		return FALSE;
	}

	m_wndMapCtrlBar.EnableDocking (CBRS_ALIGN_LEFT | CBRS_ALIGN_RIGHT);
	m_wndObjectCtrlBar.EnableDocking (CBRS_ALIGN_LEFT | CBRS_ALIGN_RIGHT);
	m_wndEffectCtrlBar.EnableDocking (CBRS_ALIGN_LEFT | CBRS_ALIGN_RIGHT);
	m_wndFlyCtrlBar.EnableDocking (CBRS_ALIGN_LEFT | CBRS_ALIGN_RIGHT);

	DockControlBar (&m_wndMapCtrlBar, AFX_IDW_DOCKBAR_LEFT);
	RecalcLayout();
	DockControlBar (&m_wndObjectCtrlBar, AFX_IDW_DOCKBAR_RIGHT);
	RecalcLayout();
	DockControlBar (&m_wndEffectCtrlBar, AFX_IDW_DOCKBAR_RIGHT);
	RecalcLayout();
	DockControlBar (&m_wndFlyCtrlBar, AFX_IDW_DOCKBAR_RIGHT);
	RecalcLayout();

	m_wndMapCtrlBar.SetFloatFixedWidth (TRUE);
	m_wndMapCtrlBar.SetMinSize (CSize (220, 10));
	m_wndMapCtrlBar.SetDockingFixedWidth (TRUE);
	ShowControlBar (&m_wndMapCtrlBar, TRUE, FALSE);

	m_wndObjectCtrlBar.SetFloatFixedWidth (TRUE);
	m_wndObjectCtrlBar.SetMinSize (CSize (220, 10));
	m_wndObjectCtrlBar.SetDockingFixedWidth (TRUE);
	ShowControlBar (&m_wndObjectCtrlBar, FALSE, FALSE);

	m_wndEffectCtrlBar.SetFloatFixedWidth (TRUE);
	m_wndEffectCtrlBar.SetMinSize (CSize (220, 50));
	m_wndEffectCtrlBar.SetDockingFixedWidth (TRUE);
	ShowControlBar (&m_wndEffectCtrlBar, FALSE, FALSE);

	m_wndFlyCtrlBar.SetFloatFixedWidth (TRUE);
	m_wndFlyCtrlBar.SetMinSize (CSize (220, 50));
	m_wndFlyCtrlBar.SetDockingFixedWidth (TRUE);
	ShowControlBar (&m_wndFlyCtrlBar, FALSE, FALSE);

	return TRUE;
}

void CMainFrame::Initialize()
{
	// NOTE : If you use next line, it can affect negative side to effect usage which is used on playing
	//        with animation. - [levites]
	//CResource::SetDeleteImmediately(true);

	// NOTE : This initialize proccessing's order is very important!
	//        If you change this order, program maybe not run correctly. - [ levites ]
	CWorldEditorDoc * pDocument = (CWorldEditorDoc*) GetActiveDocument();

	CMapManagerAccessor * pMapManagerAccessor = pDocument->GetMapManagerAccessor();
	pMapManagerAccessor->Create();

	CSceneMap * pSceneMap = pDocument->GetSceneMap();
	pSceneMap->Initialize();
	pSceneMap->SetMapManagerAccessor (pMapManagerAccessor);

	CObjectData * pObjectData = pDocument->GetObjectData();
	pObjectData->Initialize();

	CSceneEffect * pSceneEffect = pDocument->GetSceneEffect();
	pSceneEffect->Initialize();

	CSceneObject * pSceneObject = pDocument->GetSceneObject();
	pSceneObject->Initialize();

	CSceneFly * pSceneFly = pDocument->GetSceneFly();
	pSceneFly->Initialize();

	m_wndObjectCtrlBar.Initialize();
	m_wndEffectCtrlBar.Initialize();
	m_wndMapCtrlBar.Initialize();
	m_wndFlyCtrlBar.Initialize();

	ShowControlBar (&m_wndMapCtrlBar, FALSE, FALSE);
	ShowControlBar (&m_wndObjectCtrlBar, FALSE, FALSE);
	ShowControlBar (&m_wndEffectCtrlBar, FALSE, FALSE);
	ShowControlBar (&m_wndFlyCtrlBar, FALSE, FALSE);

	switch (iStartUpPage)
	{
		case ID_VIEW_MAP:
			pDocument->SetActiveMode (ID_VIEW_MAP);
			ShowControlBar (&m_wndMapCtrlBar, TRUE, FALSE);
			break;

		case ID_VIEW_OBJECT:
			pDocument->SetActiveMode (ID_VIEW_OBJECT);
			ShowControlBar (&m_wndObjectCtrlBar, TRUE, FALSE);
			break;

		case ID_VIEW_EFFECT:
			pDocument->SetActiveMode (ID_VIEW_EFFECT);
			ShowControlBar (&m_wndEffectCtrlBar, TRUE, FALSE);
			break;

		case ID_VIEW_FLY:
			pDocument->SetActiveMode (ID_VIEW_FLY);
			ShowControlBar (&m_wndFlyCtrlBar, TRUE, FALSE);
			break;
	}

	CWorldEditorView * pView = (CWorldEditorView*) GetActiveView();
	pView->Initialize();
	m_wndMapCtrlBar.m_pageFile.InstallFolderDrop (GetSafeHwnd());

	pSceneObject->FitCamera();
}

void CMainFrame::UpdateStatusBar (float fx, float fy, float fz)
{
	CString strMeter;
	CString strSector;
	CString strFolder;
	unsigned uSectorX = 0;
	unsigned uSectorY = 0;
	bool bHaveSector = false;

	strMeter.Format ("%.2f / %.2f / %.2f", fx / 100.0f, fy / 100.0f, fz / 100.0f);

	CWorldEditorDoc * pDocument = (CWorldEditorDoc *) GetActiveDocument();
	if (pDocument)
	{
		CMapManagerAccessor * pManager = pDocument->GetMapManagerAccessor();
		CMapOutdoorAccessor * pMap = pManager->GetMapOutdoorPtr();
		if (pMap && pManager->IsMapReady())
		{
			const TOutdoorMapCoordinate & rCoord = pMap->GetCurCoordinate();
			int iSectorX = rCoord.m_sTerrainCoordX;
			int iSectorY = rCoord.m_sTerrainCoordY;
			if (iSectorX < 0 || iSectorY < 0)
			{
				const float fTerrainX = (float) CTerrainImpl::TERRAIN_XSIZE;
				const float fTerrainY = (float) CTerrainImpl::TERRAIN_YSIZE;
				iSectorX = (fTerrainX > 0.0f) ? (int) floorf (fx / fTerrainX) : 0;
				iSectorY = (fTerrainY > 0.0f) ? (int) floorf (fy / fTerrainY) : 0;
				if (iSectorX < 0)
					iSectorX = 0;
				if (iSectorY < 0)
					iSectorY = 0;
			}
			bHaveSector = true;
			uSectorX = (unsigned) iSectorX;
			uSectorY = (unsigned) iSectorY;
			strFolder.Format ("%06u", uSectorX * 1000u + uSectorY);
		}
	}
	if (bHaveSector)
		strSector.Format ("%u / %u", uSectorX, uSectorY);

	m_wndStatusBar.SetPaneText (1, strFolder, TRUE);
	m_wndStatusBar.SetPaneText (2, m_strSaveStatus, TRUE);
	m_wndStatusBar.SetPaneText (3, strMeter, TRUE);
	m_wndStatusBar.SetPaneText (4, strSector, TRUE);
}

void CMainFrame::SetFrameTime (float fFrameMs)
{
	if (fFrameMs < 0.0f)
		fFrameMs = 0.0f;

	if (m_fFrameMs <= 0.0f || fFrameMs > 200.0f)
		m_fFrameMs = fFrameMs;
	else
		m_fFrameMs += (fFrameMs - m_fFrameMs) * 0.12f;

	const float fFps = (m_fFrameMs > 0.05f) ? (1000.0f / m_fFrameMs) : 0.0f;
	CString strFps;
	CString strMs;
	strFps.Format ("%.0f fps", fFps);
	strMs.Format ("%.1f ms", m_fFrameMs);
	if (m_wndStatusBar.GetSafeHwnd())
	{
		m_wndStatusBar.SetPaneText (5, strFps, TRUE);
		m_wndStatusBar.SetPaneText (6, strMs, TRUE);
	}
}

void CMainFrame::SetAutoSaveStatus (const char * c_szMessage)
{
	SYSTEMTIME kTime;
	::GetLocalTime (&kTime);
	m_strSaveStatus.Format ("[%02d:%02d:%02d] %s", kTime.wHour, kTime.wMinute, kTime.wSecond, c_szMessage);
	if (m_wndStatusBar.GetSafeHwnd())
		m_wndStatusBar.SetPaneText (2, m_strSaveStatus, TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

BOOL CMainFrame::OnCmdMsg (UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	if (pHandlerInfo == NULL)
	{
		CCmdUI *pCmdUI = (CCmdUI*)pExtra;

		switch (nCode)
		{
			case CN_COMMAND:
				if (CN_COMMAND_HANDLER (nID, pCmdUI) == TRUE)
				{
					return TRUE;
				}
				break;
			case CN_UPDATE_COMMAND_UI:
				if (CN_UPDATE_HANDLER (nID, pCmdUI) == TRUE)
				{
					return TRUE;
				}
				break;
		}
	}

	return CFrameWnd::OnCmdMsg (nID, nCode, pExtra, pHandlerInfo);
}

void CMainFrame::UpdateObjectControlBar()
{
	m_wndObjectCtrlBar.UpdatePage();
}

void CMainFrame::UpdateAnimationPage()
{
	m_wndObjectCtrlBar.UpdateAnimationPage();
}

void CMainFrame::UpdateEffectControlBar()
{
	m_wndEffectCtrlBar.UpdatePage();
}

void CMainFrame::UpdateFlyControlBar()
{
	m_wndFlyCtrlBar.UpdatePage();
}

void CMainFrame::AcceptMapFolderDrop (HDROP hDrop)
{
	m_wndMapCtrlBar.m_pageFile.HandleFolderDrop (hDrop);
}

void CMainFrame::RunLoadMapEvent()
{
	m_wndMapCtrlBar.RunLoadMapEvent();
}

void CMainFrame::UpdateMapControlBar()
{
	CWorldEditorDoc * pDocument = (CWorldEditorDoc*)GetActiveDocument();

	if (!pDocument)
	{
		return;
	}

	int iMode = m_wndMapCtrlBar.UpdatePage();
	pDocument->m_SceneMap.SetEditingMode (iMode);

	if (CWorldEditorConfig::Instance().RefreshAllOnUpdateUI())
	{
		CMapManagerAccessor * pMapManagerAccessor = pDocument->GetMapManagerAccessor();
		if (pMapManagerAccessor && pMapManagerAccessor->IsMapReady())
		{
			pMapManagerAccessor->RefreshArea();
		}
	}
}

void CMainFrame::OnUndo()
{
	CWorldEditorDoc * pDocument = (CWorldEditorDoc*)GetActiveDocument();
	CUndoBuffer * pUndoBuffer = pDocument->GetUndoBuffer();

	pUndoBuffer->Undo();
}

void CMainFrame::OnRedo()
{
	CWorldEditorDoc * pDocument = (CWorldEditorDoc*)GetActiveDocument();
	CUndoBuffer * pUndoBuffer = pDocument->GetUndoBuffer();

	pUndoBuffer->Redo();
}

BOOL CMainFrame::CN_COMMAND_HANDLER (UINT nID, CCmdUI *pCmdUI)
{
	if (m_wndFileToolBar.CN_COMMAND_HANDLER (nID, pCmdUI))
	{
		return TRUE;
	}

	CWorldEditorDoc * pDocument = (CWorldEditorDoc*)GetActiveDocument();
	CWorldEditorView * pView = (CWorldEditorView*)GetActiveView();
	pDocument->SetActiveMode (nID);

	switch (nID)
	{
		case ID_VIEW_MAP:
			ShowControlBar (&m_wndMapCtrlBar, !m_wndMapCtrlBar.IsWindowVisible(), FALSE);
			ShowControlBar (&m_wndObjectCtrlBar, FALSE, FALSE);
			ShowControlBar (&m_wndEffectCtrlBar, FALSE, FALSE);
			ShowControlBar (&m_wndFlyCtrlBar, FALSE, FALSE);
			return TRUE;
			break;

		case ID_VIEW_OBJECT:
			ShowControlBar (&m_wndObjectCtrlBar, !m_wndObjectCtrlBar.IsWindowVisible(), FALSE);
			ShowControlBar (&m_wndMapCtrlBar, FALSE, FALSE);
			ShowControlBar (&m_wndEffectCtrlBar, FALSE, FALSE);
			ShowControlBar (&m_wndFlyCtrlBar, FALSE, FALSE);
			return TRUE;
			break;

		case ID_VIEW_EFFECT:
			ShowControlBar (&m_wndEffectCtrlBar, !m_wndEffectCtrlBar.IsWindowVisible(), FALSE);
			ShowControlBar (&m_wndMapCtrlBar, FALSE, FALSE);
			ShowControlBar (&m_wndObjectCtrlBar, FALSE, FALSE);
			ShowControlBar (&m_wndFlyCtrlBar, FALSE, FALSE);
			return TRUE;
			break;

		case ID_VIEW_FLY:
			ShowControlBar (&m_wndFlyCtrlBar, !m_wndFlyCtrlBar.IsWindowVisible(), FALSE);
			ShowControlBar (&m_wndEffectCtrlBar, FALSE, FALSE);
			ShowControlBar (&m_wndMapCtrlBar, FALSE, FALSE);
			ShowControlBar (&m_wndObjectCtrlBar, FALSE, FALSE);
			return TRUE;
			break;

		case ID_VIEW_INFO_BOX:
		{
			CWorldEditorView * pView = (CWorldEditorView*)GetActiveView();
			pView->ToggleInfoBoxVisibleState();
		}
		return TRUE;
		break;

		case ID_VIEW_RENDERING_MODE_SOLID:
			CSceneBase::SetRenderingMode (CSceneBase::RENDERING_MODE_SOLID);
			return TRUE;
			break;

		case ID_VIEW_RENDERING_MODE_WIRE_FRAME:
			CSceneBase::SetRenderingMode (CSceneBase::RENDERING_MODE_WIRE_FRAME);
			return TRUE;
			break;

		case ID_VIEW_DLG_PERSPECTIVE:
		{
			CWorldEditorView * pView = (CWorldEditorView*)GetActiveView();
			pView->TogglePerspectiveDialog();
		}
		return TRUE;
		break;
	}

	return FALSE;
}

BOOL CMainFrame::CN_UPDATE_HANDLER (UINT nID, CCmdUI *pCmdUI)
{
	if (m_wndFileToolBar.CN_UPDATE_HANDLER (nID, pCmdUI))
	{
		return TRUE;
	}

	switch (nID)
	{
		case ID_VIEW_MAP:
			pCmdUI->Enable();
			pCmdUI->SetCheck (m_wndMapCtrlBar.IsWindowVisible());
			return TRUE;
			break;

		case ID_VIEW_OBJECT:
			pCmdUI->Enable();
			pCmdUI->SetCheck (m_wndObjectCtrlBar.IsWindowVisible());
			return TRUE;
			break;

		case ID_VIEW_EFFECT:
			pCmdUI->Enable();
			pCmdUI->SetCheck (m_wndEffectCtrlBar.IsWindowVisible());
			return TRUE;
			break;

		case ID_VIEW_FLY:
			pCmdUI->Enable();
			pCmdUI->SetCheck (m_wndFlyCtrlBar.IsWindowVisible());
			return TRUE;
			break;

		case ID_VIEW_INFO_BOX:
		{
			pCmdUI->Enable();
			CWorldEditorView * pView = (CWorldEditorView*)GetActiveView();
			if (pView->IsInfoBoxVisible())
			{
				pCmdUI->SetCheck (TRUE);
			}
			else
			{
				pCmdUI->SetCheck (FALSE);
			}
			return TRUE;
			break;
		}

		case ID_VIEW_RENDERING_MODE_SOLID:
			pCmdUI->Enable();
			if (CSceneBase::RENDERING_MODE_SOLID == CSceneBase::GetRenderingMode())
			{
				pCmdUI->SetCheck (TRUE);
			}
			else
			{
				pCmdUI->SetCheck (FALSE);
			}
			return TRUE;
			break;

		case ID_VIEW_RENDERING_MODE_WIRE_FRAME:
			pCmdUI->Enable();
			if (CSceneBase::RENDERING_MODE_WIRE_FRAME == CSceneBase::GetRenderingMode())
			{
				pCmdUI->SetCheck (TRUE);
			}
			else
			{
				pCmdUI->SetCheck (FALSE);
			}
			return TRUE;
			break;

		case ID_VIEW_DLG_PERSPECTIVE:
		{
			CWorldEditorView * pView = (CWorldEditorView*)GetActiveView();
			pCmdUI->Enable();
			if (pView->isShowingPerspectiveDialog())
			{
				pCmdUI->SetCheck (TRUE);
			}
			else
			{
				pCmdUI->SetCheck (FALSE);
			}
		}
		return TRUE;
		break;
	}

	return FALSE;
}
