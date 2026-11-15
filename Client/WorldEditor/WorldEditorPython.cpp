#include "StdAfx.h"
#include "WorldEditorPython.h"
#include "WorldEditor.h"
#include "MainFrm.h"
#include "WorldEditorDoc.h"
#include "DataCtrl/MapManagerAccessor.h"
#include "DataCtrl/MapAccessorTerrain.h"

#if defined(_WIN64)
typedef long long WePy_ssize;
#else
typedef int WePy_ssize;
#endif

struct WePyObject
{
	WePy_ssize ob_refcnt;
	void * ob_type;
};

struct WePyMethodDef
{
	const char * ml_name;
	WePyObject * (__cdecl * ml_meth) (WePyObject *, WePyObject *);
	int ml_flags;
	const char * ml_doc;
};

struct WePyModuleDef
{
	WePyObject ob_base;
	WePyObject * (__cdecl * m_init)();
	WePy_ssize m_index;
	WePyObject * m_copy;
	const char * m_name;
	const char * m_doc;
	WePy_ssize m_size;
	WePyMethodDef * m_methods;
	void * m_slots;
	void * m_traverse;
	void * m_clear;
	void * m_free;
};

static HMODULE s_python = NULL;
static bool s_ready = false;

static WePyObject * (__cdecl * s_PyModule_Create2) (WePyModuleDef *, int) = NULL;
static int (__cdecl * s_PyImport_AppendInittab) (const char *, WePyObject * (__cdecl *)()) = NULL;
static void (__cdecl * s_Py_SetPythonHome) (const wchar_t *) = NULL;
static void (__cdecl * s_Py_InitializeEx) (int) = NULL;
static int (__cdecl * s_PyRun_SimpleString) (const char *) = NULL;
static void (__cdecl * s_PyErr_Print)() = NULL;
static WePyObject * (__cdecl * s_Py_BuildValue) (const char *, ...) = NULL;
static int (__cdecl * s_PyArg_ParseTuple) (WePyObject *, const char *, ...) = NULL;
static int (__cdecl * s_PyModule_AddIntConstant) (WePyObject *, const char *, long) = NULL;

static const int WE_METH_VARARGS = 0x0001;
static const int WE_PYTHON_API_VERSION = 1013;

static CMapManagerAccessor * EditorMap()
{
	CMainFrame * pFrame = (CMainFrame *) AfxGetMainWnd();
	if (!pFrame)
		return NULL;
	CWorldEditorDoc * pDoc = (CWorldEditorDoc *) pFrame->GetActiveDocument();
	if (!pDoc)
		return NULL;
	return pDoc->GetMapManagerAccessor();
}

static WePyObject * we_none()
{
	return s_Py_BuildValue ("");
}

static WePyObject * dbg_tracen (WePyObject *, WePyObject * args)
{
	const char * text = NULL;
	if (!s_PyArg_ParseTuple (args, "s", &text))
		return NULL;
	Tracef ("%s\n", text ? text : "");
	return we_none();
}

static WePyObject * dbg_logbox (WePyObject *, WePyObject * args)
{
	const char * text = NULL;
	if (!s_PyArg_ParseTuple (args, "s", &text))
		return NULL;
	LogBox (text ? text : "");
	return we_none();
}

static WePyObject * we_is_map_ready (WePyObject *, WePyObject * args)
{
	if (!s_PyArg_ParseTuple (args, ""))
		return NULL;
	CMapManagerAccessor * pMap = EditorMap();
	return s_Py_BuildValue ("i", (pMap && pMap->IsMapReady()) ? 1 : 0);
}

static WePyObject * we_get_terrain_count (WePyObject *, WePyObject * args)
{
	if (!s_PyArg_ParseTuple (args, ""))
		return NULL;
	CMapManagerAccessor * pMap = EditorMap();
	short sCountX = 0;
	short sCountY = 0;
	if (pMap)
		pMap->GetTerrainCount (&sCountX, &sCountY);
	return s_Py_BuildValue ("(ii)", (int) sCountX, (int) sCountY);
}

static WePyObject * we_get_base_xy (WePyObject *, WePyObject * args)
{
	if (!s_PyArg_ParseTuple (args, ""))
		return NULL;
	CMapManagerAccessor * pMap = EditorMap();
	DWORD dwX = 0;
	DWORD dwY = 0;
	if (pMap)
		pMap->GetBaseXY (&dwX, &dwY);
	return s_Py_BuildValue ("(ii)", (int) dwX, (int) dwY);
}

static WePyObject * we_get_environment (WePyObject *, WePyObject * args)
{
	if (!s_PyArg_ParseTuple (args, ""))
		return NULL;
	CMapManagerAccessor * pMap = EditorMap();
	const std::string name = pMap ? pMap->GetEnvironmentDataName() : std::string();
	return s_Py_BuildValue ("s", name.c_str());
}

static WePyObject * we_trim_memory (WePyObject *, WePyObject * args)
{
	if (!s_PyArg_ParseTuple (args, ""))
		return NULL;
	CMapManagerAccessor * pMap = EditorMap();
	if (pMap)
		pMap->TrimMemory();
	return we_none();
}

static WePyObject * we_set_all_water_height (WePyObject *, WePyObject * args)
{
	int iHeight = 0;
	if (!s_PyArg_ParseTuple (args, "i", &iHeight))
		return NULL;
	CMapManagerAccessor * pMap = EditorMap();
	if (pMap)
		pMap->SetAllWaterHeight ((long) iHeight);
	return we_none();
}

static WePyObject * we_generate_server_attr (WePyObject *, WePyObject * args)
{
	const char * path = NULL;
	if (!s_PyArg_ParseTuple (args, "s", &path))
		return NULL;
	CMapManagerAccessor * pMap = EditorMap();
	const bool bOk = pMap && path && pMap->GenerateServerAttr (path);
	return s_Py_BuildValue ("i", bOk ? 1 : 0);
}

static WePyObject * we_draw_height_brush (WePyObject *, WePyObject * args)
{
	float fX = 0.0f;
	float fY = 0.0f;
	int iShape = 0;
	int iType = 0;
	int iSize = 0;
	int iStrength = 0;
	int iRefresh = 0;
	if (!s_PyArg_ParseTuple (args, "ffiiii|i", &fX, &fY, &iShape, &iType, &iSize, &iStrength, &iRefresh))
		return NULL;
	CMapManagerAccessor * pMap = EditorMap();
	if (pMap)
		pMap->DrawHeightBrush (fX, fY, (DWORD) iShape, (DWORD) iType, (BYTE) iSize, (BYTE) iStrength, iRefresh != 0);
	return we_none();
}

static WePyObject * we_get_object_height_bias (WePyObject *, WePyObject * args)
{
	int iIndex = 0;
	if (!s_PyArg_ParseTuple (args, "i", &iIndex))
		return NULL;
	float fBias = 0.0f;
	CMapManagerAccessor * pMap = EditorMap();
	if (!pMap || !pMap->GetObjectHeightBias (iIndex, &fBias))
		return s_Py_BuildValue ("f", 0.0f);
	return s_Py_BuildValue ("f", fBias);
}

static WePyObject * we_set_object_height_bias (WePyObject *, WePyObject * args)
{
	int iIndex = 0;
	float fBias = 0.0f;
	if (!s_PyArg_ParseTuple (args, "if", &iIndex, &fBias))
		return NULL;
	CMapManagerAccessor * pMap = EditorMap();
	if (pMap)
		pMap->SetObjectHeightBias (iIndex, fBias);
	return we_none();
}

static WePyObject * we_get_object_rotation (WePyObject *, WePyObject * args)
{
	int iIndex = 0;
	if (!s_PyArg_ParseTuple (args, "i", &iIndex))
		return NULL;
	float fYaw = 0.0f;
	float fPitch = 0.0f;
	float fRoll = 0.0f;
	CMapManagerAccessor * pMap = EditorMap();
	if (pMap)
		pMap->GetObjectRotation (iIndex, &fYaw, &fPitch, &fRoll);
	return s_Py_BuildValue ("(fff)", fYaw, fPitch, fRoll);
}

static WePyObject * we_set_object_rotation (WePyObject *, WePyObject * args)
{
	int iIndex = 0;
	float fYaw = 0.0f;
	float fPitch = 0.0f;
	float fRoll = 0.0f;
	if (!s_PyArg_ParseTuple (args, "ifff", &iIndex, &fYaw, &fPitch, &fRoll))
		return NULL;
	CMapManagerAccessor * pMap = EditorMap();
	if (pMap)
		pMap->SetObjectRotation (iIndex, fYaw, fPitch, fRoll);
	return we_none();
}

static WePyMethodDef s_dbgMethods[] =
{
	{ "Tracen", dbg_tracen, WE_METH_VARARGS, NULL },
	{ "Trace", dbg_tracen, WE_METH_VARARGS, NULL },
	{ "LogBox", dbg_logbox, WE_METH_VARARGS, NULL },
	{ NULL, NULL, 0, NULL }
};

static WePyMethodDef s_worldMethods[] =
{
	{ "IsMapReady", we_is_map_ready, WE_METH_VARARGS, NULL },
	{ "GetTerrainCount", we_get_terrain_count, WE_METH_VARARGS, NULL },
	{ "GetBaseXY", we_get_base_xy, WE_METH_VARARGS, NULL },
	{ "GetEnvironmentDataName", we_get_environment, WE_METH_VARARGS, NULL },
	{ "TrimMemory", we_trim_memory, WE_METH_VARARGS, NULL },
	{ "SetAllWaterHeight", we_set_all_water_height, WE_METH_VARARGS, NULL },
	{ "GenerateServerAttr", we_generate_server_attr, WE_METH_VARARGS, NULL },
	{ "DrawHeightBrush", we_draw_height_brush, WE_METH_VARARGS, NULL },
	{ "GetObjectHeightBias", we_get_object_height_bias, WE_METH_VARARGS, NULL },
	{ "SetObjectHeightBias", we_set_object_height_bias, WE_METH_VARARGS, NULL },
	{ "GetObjectRotation", we_get_object_rotation, WE_METH_VARARGS, NULL },
	{ "SetObjectRotation", we_set_object_rotation, WE_METH_VARARGS, NULL },
	{ NULL, NULL, 0, NULL }
};

static WePyModuleDef s_dbgModule;
static WePyModuleDef s_worldModule;

static void PrepareModule (WePyModuleDef * pModule, const char * c_szName, WePyMethodDef * pMethods)
{
	memset (pModule, 0, sizeof (WePyModuleDef));
	pModule->ob_base.ob_refcnt = 1;
	pModule->m_name = c_szName;
	pModule->m_methods = pMethods;
	pModule->m_size = -1;
}

static WePyObject * PyInit_dbg()
{
	PrepareModule (&s_dbgModule, "dbg", s_dbgMethods);
	return s_PyModule_Create2 (&s_dbgModule, WE_PYTHON_API_VERSION);
}

static WePyObject * PyInit_WorldEditor()
{
	PrepareModule (&s_worldModule, "WorldEditor", s_worldMethods);
	WePyObject * pModule = s_PyModule_Create2 (&s_worldModule, WE_PYTHON_API_VERSION);
	if (!pModule)
		return NULL;

	s_PyModule_AddIntConstant (pModule, "TERRAIN_SIZE", TERRAIN_SIZE);
	s_PyModule_AddIntConstant (pModule, "TERRAIN_PATCHSIZE", TERRAIN_PATCHSIZE);
	s_PyModule_AddIntConstant (pModule, "TERRAIN_PATCHCOUNT", TERRAIN_PATCHCOUNT);
	s_PyModule_AddIntConstant (pModule, "MAXTERRAINTEXTURES", MAXTERRAINTEXTURES);
	s_PyModule_AddIntConstant (pModule, "CELLSCALE_IN_METER", CTerrainImpl::CELLSCALE_IN_METER);
	s_PyModule_AddIntConstant (pModule, "HALF_CELLSCALE_IN_METER", CTerrainImpl::HALF_CELLSCALE_IN_METER);
	s_PyModule_AddIntConstant (pModule, "ATTRMAP_XSIZE", CTerrainImpl::ATTRMAP_XSIZE);
	s_PyModule_AddIntConstant (pModule, "ATTRMAP_YSIZE", CTerrainImpl::ATTRMAP_YSIZE);
	s_PyModule_AddIntConstant (pModule, "TILEMAP_RAW_XSIZE", CTerrainImpl::TILEMAP_RAW_XSIZE);
	s_PyModule_AddIntConstant (pModule, "TILEMAP_RAW_YSIZE", CTerrainImpl::TILEMAP_RAW_YSIZE);
	s_PyModule_AddIntConstant (pModule, "MAX_WATER_NUM", CTerrainImpl::MAX_WATER_NUM);
	s_PyModule_AddIntConstant (pModule, "AROUND_AREA_NUM", AROUND_AREA_NUM);
	s_PyModule_AddIntConstant (pModule, "ATTRIBUTE_BLOCK", CTerrainImpl::ATTRIBUTE_BLOCK);
	s_PyModule_AddIntConstant (pModule, "ATTRIBUTE_WATER", CTerrainImpl::ATTRIBUTE_WATER);
	s_PyModule_AddIntConstant (pModule, "ATTRIBUTE_BANPK", CTerrainImpl::ATTRIBUTE_BANPK);
	s_PyModule_AddIntConstant (pModule, "BRUSH_SHAPE_NONE", CTerrainAccessor::BRUSH_SHAPE_NONE);
	s_PyModule_AddIntConstant (pModule, "BRUSH_SHAPE_CIRCLE", CTerrainAccessor::BRUSH_SHAPE_CIRCLE);
	s_PyModule_AddIntConstant (pModule, "BRUSH_SHAPE_SQUARE", CTerrainAccessor::BRUSH_SHAPE_SQUARE);
	s_PyModule_AddIntConstant (pModule, "BRUSH_SHAPE_TRIANGLE", CTerrainAccessor::BRUSH_SHAPE_TRIANGLE);
	s_PyModule_AddIntConstant (pModule, "BRUSH_SHAPE_MAX", CTerrainAccessor::BRUSH_SHAPE_MAX);
	s_PyModule_AddIntConstant (pModule, "BRUSH_TYPE_NONE", CTerrainAccessor::BRUSH_TYPE_NONE);
	s_PyModule_AddIntConstant (pModule, "BRUSH_TYPE_UP", CTerrainAccessor::BRUSH_TYPE_UP);
	s_PyModule_AddIntConstant (pModule, "BRUSH_TYPE_DOWN", CTerrainAccessor::BRUSH_TYPE_DOWN);
	s_PyModule_AddIntConstant (pModule, "BRUSH_TYPE_PLATEAU", CTerrainAccessor::BRUSH_TYPE_PLATEAU);
	s_PyModule_AddIntConstant (pModule, "BRUSH_TYPE_NOISE", CTerrainAccessor::BRUSH_TYPE_NOISE);
	s_PyModule_AddIntConstant (pModule, "BRUSH_TYPE_SMOOTH", CTerrainAccessor::BRUSH_TYPE_SMOOTH);
	return pModule;
}

static bool LoadPythonSymbol (const char * c_szName, void ** ppSymbol)
{
	*ppSymbol = (void *) GetProcAddress (s_python, c_szName);
	if (!*ppSymbol)
	{
		Tracef ("Python symbol missing: %s\n", c_szName);
		return false;
	}
	return true;
}

static bool EnsurePython()
{
	if (s_ready)
		return true;

	char szExe[MAX_PATH];
	if (!GetModuleFileNameA (NULL, szExe, MAX_PATH))
		return false;
	char * pSlash = strrchr (szExe, '\\');
	if (!pSlash)
		return false;
	*pSlash = '\0';

	char szHome[MAX_PATH];
#if defined(_WIN64)
	_snprintf (szHome, MAX_PATH, "%s\\lib38-x64", szExe);
#else
	_snprintf (szHome, MAX_PATH, "%s\\lib38", szExe);
#endif

	char szDll[MAX_PATH];
	_snprintf (szDll, MAX_PATH, "%s\\python38.dll", szHome);
	if (GetFileAttributesA (szDll) == INVALID_FILE_ATTRIBUTES)
	{
		Tracef ("Python runtime missing: %s\n", szDll);
		return false;
	}

	SetDllDirectoryA (szHome);
	s_python = LoadLibraryA (szDll);
	SetDllDirectoryA (NULL);
	if (!s_python)
	{
		Tracef ("python38.dll failed to load (%lu)\n", GetLastError());
		return false;
	}

	if (!LoadPythonSymbol ("PyModule_Create2", (void **) &s_PyModule_Create2) ||
		!LoadPythonSymbol ("PyImport_AppendInittab", (void **) &s_PyImport_AppendInittab) ||
		!LoadPythonSymbol ("Py_SetPythonHome", (void **) &s_Py_SetPythonHome) ||
		!LoadPythonSymbol ("Py_InitializeEx", (void **) &s_Py_InitializeEx) ||
		!LoadPythonSymbol ("PyRun_SimpleString", (void **) &s_PyRun_SimpleString) ||
		!LoadPythonSymbol ("PyErr_Print", (void **) &s_PyErr_Print) ||
		!LoadPythonSymbol ("Py_BuildValue", (void **) &s_Py_BuildValue) ||
		!LoadPythonSymbol ("PyArg_ParseTuple", (void **) &s_PyArg_ParseTuple) ||
		!LoadPythonSymbol ("PyModule_AddIntConstant", (void **) &s_PyModule_AddIntConstant))
	{
		return false;
	}

	if (s_PyImport_AppendInittab ("dbg", PyInit_dbg) != 0 ||
		s_PyImport_AppendInittab ("WorldEditor", PyInit_WorldEditor) != 0)
	{
		Tracef ("Python module table is full.\n");
		return false;
	}

	static wchar_t s_home[MAX_PATH];
	MultiByteToWideChar (CP_ACP, 0, szHome, -1, s_home, MAX_PATH);
	s_Py_SetPythonHome (s_home);
	s_Py_InitializeEx (0);
	s_ready = true;
	Tracef ("Python 3.8 loaded from %s\n", szHome);
	return true;
}

bool RunWorldEditorPython()
{
	if (!EnsurePython())
		return false;

	char szExe[MAX_PATH];
	if (!GetModuleFileNameA (NULL, szExe, MAX_PATH))
		return false;
	char * pSlash = strrchr (szExe, '\\');
	if (!pSlash)
		return false;
	*pSlash = '\0';

	char szScript[MAX_PATH];
	_snprintf (szScript, MAX_PATH, "%s\\lib38\\WorldEditorRemix.py", szExe);
	if (GetFileAttributesA (szScript) == INVALID_FILE_ATTRIBUTES)
	{
		Tracef ("Python script missing: %s\n", szScript);
		return false;
	}

	std::string strCode;
	strCode += "import sys, traceback\n";
	strCode += "def _we_hook(t, v, tb):\n";
	strCode += "    dbg.Tracen(''.join(traceback.format_exception(t, v, tb)))\n";
	strCode += "sys.excepthook = _we_hook\n";
	strCode += "sys.path.insert(0, r'";
	strCode += szExe;
	strCode += "\\lib38')\n";
	strCode += "exec(compile(open(r'";
	strCode += szScript;
	strCode += "', encoding='utf-8').read(), r'";
	strCode += szScript;
	strCode += "', 'exec'), {'__name__':'__main__', '__file__': r'";
	strCode += szScript;
	strCode += "'})\n";

	if (s_PyRun_SimpleString (strCode.c_str()) != 0)
	{
		s_PyErr_Print();
		Tracef ("Python script failed.\n");
	}
	return true;
}
