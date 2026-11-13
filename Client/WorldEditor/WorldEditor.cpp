// WorldEditor.cpp : Defines the class behaviors for the application.
//

#include "StdAfx.h"
#include "WorldEditor.h"

#include "MainFrm.h"
#include "WorldEditorDoc.h"
#include "WorldEditorView.h"

#include "DataCtrl/NonPlayerCharacterInfo.h"
#include "../EterLib/Camera.h"
#include "Util.h"
#include <sys/stat.h>

#include <SpeedTreeRT.h>
#include "../GameLib/MapOutdoor.h"
#include "../SpeedTreeLib/SpeedTreeWrapper.h"
#include "DataCtrl/ShadowRenderHelper.h"

#pragma comment(lib, "dbghelp.lib")
#pragma comment(linker, "/NODEFAULTLIB:libci.lib")

#pragma comment( lib, "python27.lib" )
#pragma comment( lib, "granny2.11.8.0.lib" )
#pragma comment( lib, "mss32.lib" )

#pragma comment( lib, "DevIL.lib" )
#pragma comment( lib, "ILU.lib" )

#pragma comment( lib, "oldnames.lib" )
#pragma comment( lib, "dinput8.lib" )
#pragma comment( lib, "dxguid.lib" )

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

#pragma comment( lib, "version.lib" )
#pragma comment( lib, "imagehlp.lib" )
#pragma comment( lib, "winmm.lib" )
#pragma comment( lib, "imm32.lib" )

#pragma comment( lib, "SpeedTreeRT.lib" )

//#pragma comment( lib, "ws2_32.lib" )
//#pragma comment( lib, "wsock32.lib" )

#include <cryptopp/cryptoppLibLink.h>

#ifdef _DEBUG
	#define new DEBUG_NEW
	#undef THIS_FILE
	static char THIS_FILE[] = __FILE__;
#endif

bool __IS_TEST_SERVER_MODE__ = false;

float CCamera::CAMERA_MIN_DISTANCE = 200.0f;
float CCamera::CAMERA_MAX_DISTANCE = 2500.0f;

//#define USE_PACK

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorApp

BEGIN_MESSAGE_MAP (CWorldEditorApp, CWinApp)
	//{{AFX_MSG_MAP(CWorldEditorApp)
	ON_COMMAND (ID_APP_ABOUT, OnAppAbout)
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND (ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND (ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND (ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorApp construction

CWorldEditorApp::CWorldEditorApp()
{
	timeBeginPeriod (1);
}

CWorldEditorApp::~CWorldEditorApp()
{
	timeEndPeriod (1);
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CWorldEditorApp object

CWorldEditorApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorApp initialization

void PackInitialize (const char* c_pszFolder)
{
	if (access (c_pszFolder, 0) != 0)
	{
		return;
	}

	std::string stFolder (c_pszFolder);
	stFolder += "/";

	std::string stFileName (stFolder);
	stFileName += "Index";

	CMappedFile file;
	LPCVOID pvData;

	if (!file.Create (stFileName.c_str(), &pvData, 0, 0))
	{
		LogBoxf ("FATAL ERROR! File not exist: %s", stFileName.c_str());
		TraceError ("FATAL ERROR! File not exist: %s", stFileName.c_str());
		return;
	}

	CMemoryTextFileLoader TextLoader;
	TextLoader.Bind (file.Size(), pvData);

	for (DWORD i = 1; i < TextLoader.GetLineCount() - 1; i += 2)
	{
		const std::string & c_rstFolder = TextLoader.GetLineString (i);
		const std::string & c_rstName = TextLoader.GetLineString (i + 1);

		CEterPackManager::Instance().RegisterPack ((stFolder + c_rstName).c_str(), c_rstFolder.c_str());
	}

	CEterPackManager::Instance().RegisterRootPack ((stFolder + std::string ("root")).c_str());
	CEterPackManager::Instance().SetSearchMode (CEterPackManager::SEARCH_FILE_FIRST);
	//CEterPackManager::Instance().SetRelativePathMode();
	CSoundData::SetPackMode();	// Miles 파일 콜백을 셋팅해 줘야 한다.
}

static LONG WINAPI WorldEditorWriteCrashDump (EXCEPTION_POINTERS * pInfo)
{
	char szExe[MAX_PATH];
	std::string strDir;
	std::string strName = "WorldEditor";
	if (GetModuleFileNameA (NULL, szExe, MAX_PATH))
	{
		strDir = szExe;
		const size_t nSlash = strDir.find_last_of ("\\/");
		if (nSlash != std::string::npos)
		{
			strName = strDir.substr (nSlash + 1);
			strDir.erase (nSlash);
			const size_t nDot = strName.rfind ('.');
			if (nDot != std::string::npos)
			{
				strName.erase (nDot);
			}
		}
	}

	const std::string strLogs = strDir.empty() ? std::string ("logs") : strDir + "\\logs";
	CreateDirectoryA (strLogs.c_str(), NULL);

	SYSTEMTIME st;
	GetLocalTime (&st);
	char szDump[MAX_PATH];
	_snprintf (szDump, MAX_PATH, "%s\\WorldEditorRemix_%s_%04u%02u%02u_%02u%02u%02u.dmp",
		strLogs.c_str(), strName.c_str(),
		st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

	HANDLE hFile = CreateFileA (szDump, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		MINIDUMP_EXCEPTION_INFORMATION kInfo;
		kInfo.ThreadId = GetCurrentThreadId();
		kInfo.ExceptionPointers = pInfo;
		kInfo.ClientPointers = FALSE;
		MiniDumpWriteDump (GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &kInfo, NULL, NULL);
		CloseHandle (hFile);
	}

	return EXCEPTION_EXECUTE_HANDLER;
}

BOOL CWorldEditorApp::InitInstance()
{
	SetUnhandledExceptionFilter (WorldEditorWriteCrashDump);

	#ifdef _DEBUG
	OpenConsoleWindow();
	OpenLogFile();
	#endif

	PackInitialize ("pack");

	AfxEnableControlContainer();

	// Standard initialization

	#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
	#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
	#endif

	// Change the registry key under which our settings are stored.
	SetRegistryKey (_T ("WorldEditor Renewal by Kaptan Yosun"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register document templates
	CSingleDocTemplate * pDocTemplate;
	pDocTemplate = new CSingleDocTemplate (IDR_MAINFRAME,
										   RUNTIME_CLASS (CWorldEditorDoc),
										   RUNTIME_CLASS (CMainFrame),      // main SDI frame window
										   RUNTIME_CLASS (CWorldEditorView));
	AddDocTemplate (pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine (cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand (cmdInfo))
	{
		return FALSE;
	}

	////////////////////////////////////////////////////////////////////
	//// Specialize Code Start
	// NOTE : PropertyManager에 USE_PACK 모드인지 설정한다. - [levites]
	getcwd (g_szProgramPath, PROGRAM_PATH_LENGTH);
	getcwd (g_szProgramWindowPath, PROGRAM_PATH_LENGTH);
	StringPath (g_szProgramPath);
	CWorldEditorConfig::Instance().Load();
	CMapOutdoor::SetViewObjectLighting (CWorldEditorConfig::Instance().ViewObjectLighting());
	CSpeedTreeWrapper::SetRotationEnabled (CWorldEditorConfig::Instance().EnableTreeRotation());
	{
		int iShadowSize = CWorldEditorConfig::Instance().CharacterShadowMapSize();
		int iPower = 0;
		while (iShadowSize > 1)
		{
			iShadowSize >>= 1;
			++iPower;
		}
		CShadowRenderHelper::Instance().SetShadowMapPower ((BYTE) iPower);
	}
	{
		const std::string strLocale = CWorldEditorConfig::Instance().Locale();
		if (!strLocale.empty() && _stricmp (strLocale.c_str(), "EN") != 0)
		{
			char szExe[MAX_PATH];
			if (GetModuleFileNameA (NULL, szExe, MAX_PATH))
			{
				std::string strDll = szExe;
				const size_t nSlash = strDll.find_last_of ("\\/");
				if (nSlash != std::string::npos)
					strDll.erase (nSlash);
				strDll += "\\WorldEditorRemix_" + strLocale + ".dll";
				HMODULE hLocale = LoadLibraryA (strDll.c_str());
				if (!hLocale)
					Tracef ("Language Pack %s not loaded\n", strLocale.c_str());
				else
					AfxSetResourceHandle (hLocale);
			}
		}
	}
	{
		CFrameWnd * pFrame = (CFrameWnd*) AfxGetMainWnd();
		if (pFrame)
		{
			CWorldEditorDoc * pDoc = (CWorldEditorDoc*) pFrame->GetActiveDocument();
			if (pDoc)
				pDoc->GetMapManagerAccessor()->SetAutoSave (CWorldEditorConfig::Instance().AutoSave());
		}
	}

	struct _stat kPackPropertyStat;
	if (0 == _stat ("pack/property", &kPackPropertyStat) ||
		0 == _stat ("pack/property.epk", &kPackPropertyStat) ||
		0 == _stat ("pack/property.eix", &kPackPropertyStat))
	{
		CPropertyManager::Instance().Initialize ("pack/property");
		Tracef ("Property loaded from %s\n", "pack/property");
	}
	else
	{
		CPropertyManager::Instance().Initialize (NULL);
	}

	const std::string strMobProto = CWorldEditorConfig::Instance().MobProtoPath();
	if (!CNonPlayerCharacterInfo::Instance().LoadNonPlayerData (strMobProto.c_str()))
	{
		TraceError ("The file [%s] doesn't exist", strMobProto.c_str());
		if (strMobProto != "locale/en/mob_proto")
		{
			CNonPlayerCharacterInfo::Instance().LoadNonPlayerData ("locale/en/mob_proto");
		}
	}
	CNonPlayerCharacterInfo::Instance().LoadNPCGroupData ("group.txt");

	m_GraphicDevice.Create (m_pMainWnd->GetSafeHwnd(), 1024, 768);
	CreateUtilData();

	g_PopupHwnd = m_pMainWnd->GetSafeHwnd();

	// Initialize
	CMainFrame * pFrame = (CMainFrame*) m_pMainWnd;
	pFrame->Initialize();

	srandom (time (0));

	m_SoundManager.Create();

	m_LightManager.Initialize();
	//// Specialize Code End
	////////////////////////////////////////////////////////////////////

	m_pMainWnd->ShowWindow (SW_SHOW);
	if (CWorldEditorConfig::Instance().CenteredWindow())
	{
		CRect rcWindow;
		m_pMainWnd->GetWindowRect (&rcWindow);
		RECT rcWork;
		if (!SystemParametersInfo (SPI_GETWORKAREA, 0, &rcWork, 0))
		{
			rcWork.left = 0;
			rcWork.top = 0;
			rcWork.right = GetSystemMetrics (SM_CXSCREEN);
			rcWork.bottom = GetSystemMetrics (SM_CYSCREEN);
		}
		const int iX = rcWork.left + ((rcWork.right - rcWork.left) - rcWindow.Width()) / 2;
		const int iY = rcWork.top + ((rcWork.bottom - rcWork.top) - rcWindow.Height()) / 2;
		m_pMainWnd->SetWindowPos (NULL, iX, iY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
	m_pMainWnd->UpdateWindow();

	/////
	CWorldEditorApp * pApplication = (CWorldEditorApp*) AfxGetApp();
	CWorldEditorView * pView = (CWorldEditorView*)pFrame->GetActiveView();
	CRect Rect;
	pView->GetClientRect (&Rect);
	pApplication->GetGraphicDevice().ResizeBackBuffer (Rect.Width(), Rect.Height());

	return TRUE;
}

int CWorldEditorApp::ExitInstance()
{
	DestroyUtilData();
	m_GraphicDevice.Destroy();
	m_SoundManager.Destroy();
	m_EffectManager.Destroy();

	return CWinApp::ExitInstance();
}

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
	public:
		CAboutDlg();

		// Dialog Data
		//{{AFX_DATA(CAboutDlg)
		enum { IDD = IDD_ABOUTBOX };
		//}}AFX_DATA

		// ClassWizard generated virtual function overrides
		//{{AFX_VIRTUAL(CAboutDlg)
	protected:
		virtual void DoDataExchange (CDataExchange* pDX);   // DDX/DDV support
		//}}AFX_VIRTUAL

		// Implementation
	protected:
		//{{AFX_MSG(CAboutDlg)
		// No message handlers
		//}}AFX_MSG
		DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog (CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange (CDataExchange* pDX)
{
	CDialog::DoDataExchange (pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP (CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CWorldEditorApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorApp normal functions

CObjectData* CWorldEditorApp::GetObjectData()
{
	CMainFrame * pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc*)pFrame->GetActiveDocument();

	if (!pDocument)
	{
		return NULL;
	}

	return pDocument->GetObjectData();
}

CEffectAccessor* CWorldEditorApp::GetEffectAccessor()
{
	CMainFrame * pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc*)pFrame->GetActiveDocument();

	if (!pDocument)
	{
		return NULL;
	}

	return pDocument->GetEffectAccessor();
}

CMapManagerAccessor* CWorldEditorApp::GetMapManagerAccessor()
{
	CMainFrame * pFrame = (CMainFrame*)AfxGetMainWnd();

	// Temporary
	if (!pFrame->IsWindowVisible())
	{
		return NULL;
	}
	// Temporary

	CWorldEditorDoc * pDocument = (CWorldEditorDoc*)pFrame->GetActiveDocument();

	if (!pDocument)
	{
		return NULL;
	}

	return pDocument->GetMapManagerAccessor();
}

CSceneObject* CWorldEditorApp::GetSceneObject()
{
	CMainFrame * pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc*)pFrame->GetActiveDocument();

	if (!pDocument)
	{
		return NULL;
	}

	return pDocument->GetSceneObject();
}

CSceneEffect* CWorldEditorApp::GetSceneEffect()
{
	CMainFrame * pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc*)pFrame->GetActiveDocument();

	if (!pDocument)
	{
		return NULL;
	}

	return pDocument->GetSceneEffect();
}

CSceneMap* CWorldEditorApp::GetSceneMap()
{
	CMainFrame * pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc*)pFrame->GetActiveDocument();

	if (!pDocument)
	{
		return NULL;
	}

	return pDocument->GetSceneMap();
}

CSceneFly* CWorldEditorApp::GetSceneFly()
{
	CMainFrame * pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorDoc * pDocument = (CWorldEditorDoc*)pFrame->GetActiveDocument();

	if (!pDocument)
	{
		return NULL;
	}

	return pDocument->GetSceneFly();
}

/////////////////////////////////////////////////////////////////////////////
// CWorldEditorApp message handlers

BOOL CWorldEditorApp::OnIdle (LONG lCount)
{
	CMainFrame * pFrame = (CMainFrame*)AfxGetMainWnd();
	CWorldEditorView * pView = (CWorldEditorView*)pFrame->GetActiveView();

	pView->Process();

	return CWinApp::OnIdle (lCount);
}
