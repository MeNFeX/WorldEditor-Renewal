// ObjectAnimationNewEvent.cpp : implementation file
//

#include "StdAfx.h"
#include "../WorldEditor.h"
#include "ObjectAnimationEvent.h"
#include "../Util.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectAnimationEvent dialog


CObjectAnimationEvent::CObjectAnimationEvent (CWnd* pParent /*=NULL*/)
	: CDialog (CObjectAnimationEvent::IDD, pParent)
{
	m_dwEventType = CRaceMotionData::MOTION_EVENT_TYPE_SOUND;
	mc_pPreserveData = nullptr;

	//{{AFX_DATA_INIT(CObjectAnimationEvent)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CObjectAnimationEvent::DoDataExchange (CDataExchange* pDX)
{
	CDialog::DoDataExchange (pDX);
	//{{AFX_DATA_MAP(CObjectAnimationEvent)
	DDX_Control (pDX, IDD_OBJECT_ANIMATION_EVENT_TYPE, m_ctrlEventType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP (CObjectAnimationEvent, CDialog)
	//{{AFX_MSG_MAP(CObjectAnimationEvent)
	ON_BN_CLICKED (IDC_NEW_EVENT_ACCEPT, OnAcceptNewEvent)
	ON_BN_CLICKED (IDC_NEW_EVENT_CANCEL, OnCancelNewEvent)
	ON_CBN_SELCHANGE (IDD_OBJECT_ANIMATION_EVENT_TYPE, OnChangeEventType)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectAnimationEvent normal functions

int CObjectAnimationEvent::GetEventType()
{
	return m_dwEventType;
}

void CObjectAnimationEvent::SetData (const CRaceMotionDataAccessor::TMotionEventData * c_pData)
{
	m_dwEventType = c_pData->iType;
	mc_pPreserveData = c_pData;
}

void CObjectAnimationEvent::GetMotionEventData (CRaceMotionDataAccessor::TMotionEventData * pData)
{
	DWORD dwType = pData->iType;
	if (dwType >= m_EventPageVector.size())
	{
		return;
	}

	if (dwType != m_dwEventType)
	{
		assert (!"CObjectAnimationEvent::GetMotionEventData - Different Type & Pointer");
		return;
	}

	if (m_EventPageVector[dwType])
	{
		m_EventPageVector[dwType]->GetData (pData);
	}
}

void CObjectAnimationEvent::__SelectEvent (DWORD dwEventType)
{
	m_dwEventType = dwEventType;
	if (dwEventType < m_EventPageVector.size())
		if (m_EventPageVector[dwEventType])
		{
			m_EventPageVector[dwEventType]->ShowWindow (SW_SHOW);
		}

	CString strName;
	m_ctrlEventType.GetLBText (dwEventType, strName);
	m_ctrlEventType.SelectString (-1, strName);
}

void CObjectAnimationEvent::InitializePage()
{
	if (mc_pPreserveData)
	{
		__SelectEvent (mc_pPreserveData->iType);
		if (m_dwEventType < m_EventPageVector.size())
			if (m_EventPageVector[m_dwEventType])
			{
				m_EventPageVector[m_dwEventType]->SetData (mc_pPreserveData);
			}

		m_ctrlEventType.EnableWindow (FALSE);
	}
	else
	{
		__SelectEvent (0);
	}
}

void CObjectAnimationEvent::__BuildEventPageVector()
{
	m_EventPageVector.clear();
	m_EventPageVector.resize (CRaceMotionData::MOTION_EVENT_TYPE_MAX_NUM, NULL);
	m_EventPageVector[CRaceMotionData::MOTION_EVENT_TYPE_NONE] = NULL;
	m_EventPageVector[CRaceMotionData::MOTION_EVENT_TYPE_EFFECT] = &m_EventPageEffect;
	m_EventPageVector[CRaceMotionData::MOTION_EVENT_TYPE_SCREEN_WAVING] = &m_EventPageScreenWaving;
	m_EventPageVector[CRaceMotionData::MOTION_EVENT_TYPE_SCREEN_FLASHING] = &m_EventPageScreenFlashing;
	m_EventPageVector[CRaceMotionData::MOTION_EVENT_TYPE_SPECIAL_ATTACKING] = &m_EventPageAttack;
	m_EventPageVector[CRaceMotionData::MOTION_EVENT_TYPE_SOUND] = &m_EventPageSound;
	m_EventPageVector[CRaceMotionData::MOTION_EVENT_TYPE_FLY] = &m_EventPageFly;
	m_EventPageVector[CRaceMotionData::MOTION_EVENT_TYPE_CHARACTER_SHOW] = &m_EventPageCharacterShow;
	m_EventPageVector[CRaceMotionData::MOTION_EVENT_TYPE_CHARACTER_HIDE] = &m_EventPageCharacterHide;
	m_EventPageVector[CRaceMotionData::MOTION_EVENT_TYPE_WARP] = &m_EventPageWarp;
	m_EventPageVector[CRaceMotionData::MOTION_EVENT_TYPE_EFFECT_TO_TARGET] = &m_EventPageEffectToTarget;
#ifdef ENABLE_WOLFMAN_CHARACTER
	m_EventPageVector[CRaceMotionData::MOTION_EVENT_TYPE_UNK11] = &m_EventPageUnk11;
	m_EventPageVector[CRaceMotionData::MOTION_EVENT_TYPE_UNK12] = &m_EventPageUnk12;
#endif
}

void CObjectAnimationEvent::__BuildEventTypeList()
{
	std::string strEventTypeNames[CRaceMotionData::MOTION_EVENT_TYPE_MAX_NUM] =
	{
		"None",
		"Effect",
		"Screen Waving",
		"Screen Flashing",
		"Splash Attack",
		"Sound",
		"Fly",
		"Character Show",
		"Character Hide",
		"Warp",
		"Effect To Target",
#ifdef ENABLE_WOLFMAN_CHARACTER
		"Wolfman AniSpeed",
		"Wolfman Unk12",
#endif
	};
	for (int i = 0; i < CRaceMotionData::MOTION_EVENT_TYPE_MAX_NUM; ++i)
	{
		m_ctrlEventType.InsertString (i, strEventTypeNames[i].c_str());
	}
}

/////////////////////////////////////////////////////////////////////////////
// CObjectAnimationEvent message handlers

BOOL CObjectAnimationEvent::OnInitDialog()
{
	CDialog::OnInitDialog();

	CRect Rect;
	GetDlgItem (IDD_OBJECT_ANIMATION_NEW_EVENT_PAGE)->GetWindowRect (&Rect);
	ScreenToClient (&Rect);

	Rect.left -= 5;
	Rect.top -= 6;

	if (!m_EventPageScreenWaving.Create (this, Rect))
	{
		return FALSE;
	}
	if (!m_EventPageScreenFlashing.Create (this, Rect))
	{
		return FALSE;
	}
	if (!m_EventPageEffect.Create (this, Rect))
	{
		return FALSE;
	}
	if (!m_EventPageAttack.Create (this, Rect))
	{
		return FALSE;
	}
	if (!m_EventPageSound.Create (this, Rect))
	{
		return FALSE;
	}
	if (!m_EventPageFly.Create (this, Rect))
	{
		return FALSE;
	}
	if (!m_EventPageCharacterShow.Create (this, Rect))
	{
		return FALSE;
	}
	if (!m_EventPageCharacterHide.Create (this, Rect))
	{
		return FALSE;
	}
	if (!m_EventPageWarp.Create (this, Rect))
	{
		return FALSE;
	}
	if (!m_EventPageEffectToTarget.Create (this, Rect))
	{
		return FALSE;
	}
#ifdef ENABLE_WOLFMAN_CHARACTER
	if (!m_EventPageUnk11.Create (this, Rect))
	{
		return FALSE;
	}
	if (!m_EventPageUnk12.Create (this, Rect))
	{
		return FALSE;
	}
#endif

	__BuildEventPageVector();
	__BuildEventTypeList();
	InitializePage();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CObjectAnimationEvent::OnOK()
{
	if (CRaceMotionDataAccessor::MOTION_EVENT_TYPE_EFFECT == GetEventType())
	{
		if (!m_EventPageEffect.canClose())
		{
			return;
		}
	}
	else if (CRaceMotionDataAccessor::MOTION_EVENT_TYPE_FLY == GetEventType())
	{
		if (!m_EventPageFly.canClose())
		{
			return;
		}
	}

	for (DWORD i = 0; i < m_EventPageVector.size(); ++i)
	{
		if (m_EventPageVector[i])
		{
			m_EventPageVector[i]->Close();
		}
	}

	EndDialog (TRUE);
}
void CObjectAnimationEvent::OnCancel()
{
	EndDialog (FALSE);
}

void CObjectAnimationEvent::OnAcceptNewEvent()
{
	OnOK();
}

void CObjectAnimationEvent::OnCancelNewEvent()
{
	OnCancel();
}

void CObjectAnimationEvent_HideDialog (CDialog * pDialog)
{
	if (!pDialog)
	{
		return;
	}

	pDialog->ShowWindow (SW_HIDE);
}

void CObjectAnimationEvent::OnChangeEventType()
{
	std::ranges::for_each (m_EventPageVector, CObjectAnimationEvent_HideDialog);

	DWORD dwCurSel = DWORD (m_ctrlEventType.GetCurSel());
	if (dwCurSel >= m_EventPageVector.size())
	{
		return;
	}

	m_dwEventType = dwCurSel;
	if (m_EventPageVector[dwCurSel])
	{
		m_EventPageVector[dwCurSel]->ShowWindow (SW_SHOW);
	}
}

#ifdef ENABLE_WOLFMAN_CHARACTER
CObjectAnimationEventUnk11::CObjectAnimationEventUnk11 (CWnd * pParent)
	: CObjectAnimationEventBase (CObjectAnimationEventUnk11::IDD, pParent)
{
}

BEGIN_MESSAGE_MAP (CObjectAnimationEventUnk11, CDialog)
END_MESSAGE_MAP()

BOOL CObjectAnimationEventUnk11::Create (CWnd * pParent, const CRect & c_rRect)
{
	if (!CDialog::Create (CObjectAnimationEventUnk11::IDD, pParent))
	{
		return FALSE;
	}

	SetWindowPos (NULL, c_rRect.left, c_rRect.top, c_rRect.Width(), c_rRect.Height(), TRUE);
	m_staticSpeed.Create ("BaseVelocity", WS_CHILD | WS_VISIBLE, CRect (8, 8, 90, 24), this);
	m_editSpeed.Create (WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL, CRect (96, 6, 180, 26), this, IDC_WE_UNK11_SPEED);
	m_editSpeed.SetWindowText ("0");
	ShowWindow (SW_HIDE);
	return TRUE;
}

void CObjectAnimationEventUnk11::Close()
{
}

void CObjectAnimationEventUnk11::GetData (CRaceMotionData::TMotionEventData * pMotionEventData)
{
	CRaceMotionData::TMotionUnk11EventData * pData = (CRaceMotionData::TMotionUnk11EventData*) pMotionEventData;
	CString strSpeed;
	m_editSpeed.GetWindowText (strSpeed);
	pData->iAniSpeed = atoi (strSpeed);
}

void CObjectAnimationEventUnk11::SetData (const CRaceMotionData::TMotionEventData * c_pMotionEventData)
{
	const CRaceMotionData::TMotionUnk11EventData * pData = (const CRaceMotionData::TMotionUnk11EventData*) c_pMotionEventData;
	CString strSpeed;
	strSpeed.Format ("%d", pData->iAniSpeed);
	m_editSpeed.SetWindowText (strSpeed);
}

CObjectAnimationEventUnk12::CObjectAnimationEventUnk12 (CWnd * pParent)
	: CObjectAnimationEventBase (CObjectAnimationEventUnk12::IDD, pParent)
{
}

BEGIN_MESSAGE_MAP (CObjectAnimationEventUnk12, CDialog)
END_MESSAGE_MAP()

BOOL CObjectAnimationEventUnk12::Create (CWnd * pParent, const CRect & c_rRect)
{
	if (!CDialog::Create (CObjectAnimationEventUnk12::IDD, pParent))
	{
		return FALSE;
	}

	SetWindowPos (NULL, c_rRect.left, c_rRect.top, c_rRect.Width(), c_rRect.Height(), TRUE);
	ShowWindow (SW_HIDE);
	return TRUE;
}

void CObjectAnimationEventUnk12::Close()
{
}

void CObjectAnimationEventUnk12::GetData (CRaceMotionData::TMotionEventData * pMotionEventData)
{
	(void) pMotionEventData;
}

void CObjectAnimationEventUnk12::SetData (const CRaceMotionData::TMotionEventData * c_pMotionEventData)
{
	(void) c_pMotionEventData;
}
#endif
