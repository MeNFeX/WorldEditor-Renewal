#include "StdAfx.h"
#include "Util.h"
#include <set>

char					g_szProgramPath[PROGRAM_PATH_LENGTH + 1];
char					g_szProgramWindowPath[PROGRAM_PATH_LENGTH + 1];

BOOL					g_isTileData = FALSE;
BOOL					g_isTileFile = TRUE;
BOOL					g_isCharacterData = FALSE;
BOOL					g_isCharacterFile = TRUE;
LPD3DXMESH				g_pSphereMesh;
LPD3DXMESH				g_pCylinderMesh;
CGraphicThing* 			g_pTileThing = NULL;
CTileInstance			g_TileInstance;
CGraphicThingInstance	g_CharacterInstance;

void CreateBitmapButton (CButton * pButton, int iBitmapID, CBitmap & rBitmap)
{
	rBitmap.DeleteObject();
	rBitmap.LoadBitmap (iBitmapID);
	pButton->SetBitmap (rBitmap);
}

void RepositioningWindow (CPageCtrl & rPageCtrl, int ixTemporarySpace, int iyTemporarySpace)
{
	CRect Rect;
	rPageCtrl.GetClientRect (Rect);
	rPageCtrl.MoveWindow (Rect.left + ixTemporarySpace, Rect.top + iyTemporarySpace, Rect.Width(), Rect.Height());
}

void CreateHighColorImageList (int iBitmapID, CImageList * pImageList)
{
	CBitmap Bitmap;

	HINSTANCE hInstance = ::AfxFindResourceHandle (MAKEINTRESOURCE (iBitmapID), RT_BITMAP);
	HANDLE hImage = ::LoadImage (hInstance, MAKEINTRESOURCE (iBitmapID), IMAGE_BITMAP, 0, 0, (LR_DEFAULTSIZE | LR_CREATEDIBSECTION));
	Bitmap.Attach (hImage);

	// Replace
	BITMAP bmInfo;
	const RGBTRIPLE	kBackgroundColor = {192, 192, 192};

	VERIFY (Bitmap.GetBitmap (&bmInfo));

	const UINT numPixels (bmInfo.bmHeight * bmInfo.bmWidth);
	DIBSECTION ds;

	VERIFY (Bitmap.GetObject (sizeof (DIBSECTION), &ds) == sizeof (DIBSECTION));

	RGBTRIPLE * pixels = reinterpret_cast<RGBTRIPLE*> (ds.dsBm.bmBits);
	VERIFY (pixels != NULL);

	const COLORREF buttonColor (::GetSysColor (COLOR_BTNFACE));
	const RGBTRIPLE userBackgroundColor = { GetBValue (buttonColor), GetGValue (buttonColor), GetRValue (buttonColor) };

	for (UINT i = 0; i < numPixels; ++i)
	{
		if (pixels[i].rgbtBlue == kBackgroundColor.rgbtBlue &&
		pixels[i].rgbtGreen == kBackgroundColor.rgbtGreen &&
		pixels[i].rgbtRed == kBackgroundColor.rgbtRed)
		{
			pixels[i] = userBackgroundColor;
		}
	}

	// Create
	pImageList->Create (16, 16, ILC_COLOR24, 5, 0);
	pImageList->Add (&Bitmap, RGB (0, 0, 0));
}

void SetDialogFloatText (HWND hWnd, int iID, float fData)
{
	char szString[64 + 1];
	_snprintf (szString, 64, "%.3f", fData);
	SetDlgItemText (hWnd, iID, szString);
}

void SetDialogIntegerText (HWND hWnd, int iID, int iData)
{
	char szString[64 + 1];
	_snprintf (szString, 64, "%i", iData);
	SetDlgItemText (hWnd, iID, szString);
}

float GetDialogFloatText (HWND hWnd, int iID)
{
	char szString[64 + 1];
	GetDlgItemText (hWnd, iID, szString, 64);
	return atof (szString);
}

int GetDialogIntegerText (HWND hWnd, int iID)
{
	char szString[64 + 1];
	GetDlgItemText (hWnd, iID, szString, 64);
	return atoi (szString);
}

void SelectComboBoxItem (CComboBox & rComboBox, DWORD dwIndex)
{
	CString strItem;
	rComboBox.GetLBText (dwIndex, strItem);
	rComboBox.SelectString (0, strItem);
}

const char* GetProgramExcutingPath (const char* c_szFullFileName)
{
	if (strlen (c_szFullFileName) < strlen (g_szProgramPath) + 1)
	{
		return c_szFullFileName;
	}

	for (DWORD i = 0; i < strlen (g_szProgramPath); ++i)
	{
		char src, dst;

		src = tolower (c_szFullFileName[i]);
		dst = tolower (g_szProgramPath[i]);

		if (src == '/')
		{
			src = '\\';
		}

		if (dst == '/')
		{
			dst = '\\';
		}

		if (src != dst)
		{
			return &c_szFullFileName[i];
		}
	}

	return &c_szFullFileName[strlen (g_szProgramPath) + 1];
}

void CreateUtilData()
{
	class CUtilScreen : public CScreen
	{
		public:
			CUtilScreen()
			{
				if (FAILED (D3DXCreateSphere (ms_lpd3dDevice, 1.0f, 8, 8, &g_pSphereMesh, NULL)))
				{
					LogBox ("CreateUtilData: Cannot create sphere primitive");
					return;
				}

				if (FAILED (D3DXCreateCylinder (ms_lpd3dDevice, 1.0f, 1.0f, 1.0f, 8, 8, &g_pCylinderMesh, NULL)))
				{
					LogBox ("CreateUtilData: Cannot create cylinder primitive");
					return;
				}
			}
	} UtilScreen;
}

void DestroyUtilData()
{
	SAFE_RELEASE (g_pSphereMesh);
	SAFE_RELEASE (g_pCylinderMesh);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

class CSetRenderingOption : public CScreen
{
	public:
		BOOL	m_isFilled;
		DWORD	m_dwVS;

		CSetRenderingOption (BOOL isFilled, const D3DXMATRIX & c_rmatWorld)
		{
			ms_lpd3dDevice->GetFVF (&m_dwVS);

			STATEMANAGER.SaveTextureStageState (0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
			STATEMANAGER.SaveTextureStageState (0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
			STATEMANAGER.SaveTextureStageState (0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

			m_isFilled = isFilled;

			if (m_isFilled)
			{
				STATEMANAGER.SaveRenderState (D3DRS_ALPHABLENDENABLE, TRUE);
				STATEMANAGER.SaveRenderState (D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
				STATEMANAGER.SaveRenderState (D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
			}
			else
			{
				STATEMANAGER.SetRenderState (D3DRS_FILLMODE, D3DFILL_WIREFRAME);
			}

			STATEMANAGER.SetTransform (D3DTS_WORLD, &c_rmatWorld);

			STATEMANAGER.SetTexture (0, NULL);
			STATEMANAGER.SetTexture (1, NULL);
		}

		~CSetRenderingOption()
		{
			ms_lpd3dDevice->SetFVF (m_dwVS);

			STATEMANAGER.RestoreTextureStageState (0, D3DTSS_COLORARG1);
			STATEMANAGER.RestoreTextureStageState (0, D3DTSS_COLOROP);
			STATEMANAGER.RestoreTextureStageState (0, D3DTSS_ALPHAOP);

			if (m_isFilled)
			{
				STATEMANAGER.RestoreRenderState (D3DRS_ALPHABLENDENABLE);
				STATEMANAGER.RestoreRenderState (D3DRS_SRCBLEND);
				STATEMANAGER.RestoreRenderState (D3DRS_DESTBLEND);
			}
			else
			{
				STATEMANAGER.SetRenderState (D3DRS_FILLMODE, D3DFILL_SOLID);
			}
		}
};

void RenderSphere (const D3DXMATRIX * c_pmatWorld, float fx, float fy, float fz, float fRadius, BOOL isFilled /* = FALSE */)
{
	D3DXMATRIX matTranslation;
	D3DXMATRIX matScaling;

	D3DXMatrixTranslation (&matTranslation, fx, fy, fz);
	D3DXMatrixScaling (&matScaling, fRadius, fRadius, fRadius);

	D3DXMATRIX matWorld;
	matWorld = matScaling * matTranslation;

	if (c_pmatWorld)
	{
		matWorld *= *c_pmatWorld;
	}

	CSetRenderingOption SetRenderingOption (isFilled, matWorld);
	g_pSphereMesh->DrawSubset (0);
}

void RenderCylinder (const D3DXMATRIX * c_pmatWorld, float fx, float fy, float fz, float fRadius, float fLength, BOOL isFilled)
{
	D3DXMATRIX matTranslation;
	D3DXMATRIX matScaling;

	D3DXMatrixTranslation (&matTranslation, fx, fy, fz);
	D3DXMatrixScaling (&matScaling, fRadius, fRadius, fLength);

	D3DXMATRIX matWorld;
	matWorld = matScaling * matTranslation;

	if (c_pmatWorld)
	{
		matWorld *= *c_pmatWorld;
	}

	CSetRenderingOption SetRenderingOption (isFilled, matWorld);
	g_pCylinderMesh->DrawSubset (0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void RenderBackGroundCharacter (float fx, float fy, float fz)
{
	if (!g_isCharacterFile)
	{
		return;
	}

	if (!g_isCharacterData)
	{
		//CGraphicThing * pCharacter = (CGraphicThing *)CResourceManager::Instance().GetResourcePointer("D:\\Ymir Work\\pc\\warrior\\warrior_novice.GR2");
		//CGraphicThing * pMotion = (CGraphicThing *)CResourceManager::Instance().GetResourcePointer("D:\\Ymir Work\\pc\\warrior\\general\\wait.GR2");
		CGraphicThing * pCharacter = (CGraphicThing*)CResourceManager::Instance().GetResourcePointer ("D:\\Ymir Work\\pc\\sura\\sura_novice.GR2");
		CGraphicThing * pMotion = (CGraphicThing*)CResourceManager::Instance().GetResourcePointer ("D:\\Ymir Work\\pc\\sura\\onehand_sword\\wait.GR2");

		g_CharacterInstance.Clear();
		g_CharacterInstance.ReserveModelThing (1);
		g_CharacterInstance.ReserveModelInstance (1);
		g_CharacterInstance.RegisterModelThing (0, pCharacter);
		g_CharacterInstance.RegisterMotionThing (1, pMotion);

		if (pCharacter->IsEmpty())
		{
			g_isCharacterFile = FALSE;
			LogBoxf ("%s is not exist", pCharacter->GetFileName());
			return;
		}

		if (pMotion->IsEmpty())
		{
			g_isCharacterFile = FALSE;
			LogBoxf ("%s is not exist", pMotion->GetFileName());
			return;
		}

		g_CharacterInstance.SetModelInstance (0, 0, 0);
		g_CharacterInstance.SetMotion (1);
		g_isCharacterData = TRUE;
	}

	g_CharacterInstance.SetPosition (fx, fy, fz);
	g_CharacterInstance.SetRotation (45.0f);
	g_CharacterInstance.Update();
	g_CharacterInstance.Transform();
	g_CharacterInstance.Deform();
	g_CharacterInstance.Render();
}

void RenderBackGroundTile()
{
	if (!g_isTileFile)
	{
		return;
	}

	if (!g_isTileData)
	{
		if (g_pTileThing)
		{
			return;
		}

		g_pTileThing = (CGraphicThing*)CResourceManager::Instance().GetResourcePointer ("background_stone.GR2");

		g_pTileThing->AddReference();
		if (g_pTileThing->IsEmpty())
		{
			LogBox ("background_stone.gr2 is not exist");
			return;
		}
		g_TileInstance.Clear();

		if (g_TileInstance.GetDeformableVertexCount() == false)
		{
			return;
		}
		CGraphicVertexBuffer m_kSharedDeformableVertexBuffer;
		m_kSharedDeformableVertexBuffer.Destroy();
		m_kSharedDeformableVertexBuffer.Create (
			g_TileInstance.GetDeformableVertexCount(),
			D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1,
			D3DUSAGE_WRITEONLY,
			D3DPOOL_DEFAULT);
		g_TileInstance.SetMainModelPointer (g_pTileThing->GetModelPointer (0), &m_kSharedDeformableVertexBuffer);
		//g_TileInstance.SetModelPointer(g_pTileThing->GetModelPointer(0));

		g_TileInstance.LoadTexture();
		g_isTileData = TRUE;
	}

	g_TileInstance.RenderTile();
}

void DestroyBackGroundData()
{
	if (g_isCharacterData)
	{
		g_CharacterInstance.Clear();
	}

	if (g_isTileData)
	{
		g_TileInstance.Clear();
		g_pTileThing->Release();
	}
}

void CTileInstance::LoadTexture()
{
	CGraphicImage * pImage = (CGraphicImage*)CResourceManager::Instance().GetResourcePointer ("D:\\Ymir Work\\terrainmaps/b/field/field 01.dds");
	m_ImageInstance.SetImagePointer (pImage);
}

void CTileInstance::RenderTile()
{
	LPDIRECT3DVERTEXBUFFER9 lpd3dRigidPNTVtxBuf = m_pModel->GetPNTD3DVertexBuffer();
	LPDIRECT3DINDEXBUFFER9 lpd3dIdxBuf = m_pModel->GetD3DIndexBuffer();

	const CGrannyModel::TMeshNode * pMeshNode = m_pModel->GetMeshNodeList (CGrannyMesh::TYPE_RIGID, CGrannyMaterial::TYPE_DIFFUSE_PNT);

	while (pMeshNode)
	{
		const CGrannyMesh * pMesh = pMeshNode->pMesh;
		int vtxMeshBasePos = pMesh->GetVertexBasePosition();

		STATEMANAGER.SetIndices (lpd3dIdxBuf, vtxMeshBasePos);
		STATEMANAGER.SetFVF (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1);
		STATEMANAGER.SetStreamSource (0, lpd3dRigidPNTVtxBuf, sizeof (TPNTVertex));
		STATEMANAGER.SetTransform (D3DTS_WORLD, &CScreen::GetIdentityMatrix());

		const CGrannyMesh::TTriGroupNode* pTriGroupNode = pMesh->GetTriGroupNodeList (CGrannyMaterial::TYPE_DIFFUSE_PNT);

		while (pTriGroupNode)
		{
			ms_faceCount += pTriGroupNode->triCount;

			STATEMANAGER.SetTexture (0, m_ImageInstance.GetTexturePointer()->GetD3DTexture());
			STATEMANAGER.DrawIndexedPrimitive (D3DPT_TRIANGLELIST, 0, pMesh->GetVertexCount(), pTriGroupNode->idxPos, pTriGroupNode->triCount, pMesh->GetVertexBasePosition());

			pTriGroupNode = pTriGroupNode->pNextTriGroupNode;
		}

		pMeshNode = pMeshNode->pNextMeshNode;
	}
}


std::string RemoveStartString (const std::string & strOriginal, const std::string & strBegin)
{
	if (strOriginal.substr (0, strBegin.length()) == strBegin)
	{
		return strOriginal.substr (strBegin.length(), strOriginal.npos);
	}
	return strOriginal;
}

namespace
{
	void TrimConfig (std::string & rstr)
	{
		while (!rstr.empty() && (rstr.back() == ' ' || rstr.back() == '\t' || rstr.back() == '\r' || rstr.back() == '\n'))
		{
			rstr.pop_back();
		}

		size_t i = 0;
		while (i < rstr.size() && (rstr[i] == ' ' || rstr[i] == '\t'))
		{
			++i;
		}
		if (i > 0)
		{
			rstr.erase (0, i);
		}
	}

	void UnquoteConfig (std::string & rstr)
	{
		if (rstr.size() >= 2 && rstr.front() == '"' && rstr.back() == '"')
		{
			rstr = rstr.substr (1, rstr.size() - 2);
		}
	}

	int ConfigToInt (const std::string & c_rstrValue)
	{
		if (_stricmp (c_rstrValue.c_str(), "true") == 0 || _stricmp (c_rstrValue.c_str(), "yes") == 0 || _stricmp (c_rstrValue.c_str(), "on") == 0)
			return 1;
		if (_stricmp (c_rstrValue.c_str(), "false") == 0 || _stricmp (c_rstrValue.c_str(), "no") == 0 || _stricmp (c_rstrValue.c_str(), "off") == 0)
			return 0;
		return atoi (c_rstrValue.c_str());
	}
}

CWorldEditorConfig::CWorldEditorConfig()
{
	ApplyDefaults();
}

CWorldEditorConfig & CWorldEditorConfig::Instance()
{
	static CWorldEditorConfig s_kConfig;
	return s_kConfig;
}

void CWorldEditorConfig::ApplyDefaults()
{
	m_iViewChar = 1;
	m_iViewShadow = 1;
	m_iViewWater = 1;
	m_fWindowFov = 45.0f;
	m_iWasdMinimalMove = 100;
	m_iNoGotoAfterInsert = 1;
	m_iNoMaiAtlasDump = 1;
	m_iNoMinimapRawAlpha = 1;
	m_iDetectMdatrHeight = 1;
	m_iNoFogOnMapLoad = 1;
	m_iRefreshAllOnUpdateUI = 0;
	m_iTextureSetSaveAsMapName = 1;
	m_iRemoveWeirdAttrFlags = 1;
	m_iViewObjectLighting = 1;
	m_iViewMonsterAreaInfo = 0;
	m_fCursorColorR = 0.0f;
	m_fCursorColorG = 1.0f;
	m_fCursorColorB = 0.0f;
	m_fObjectHeightMax = 5000.0f;
	m_iObjectHeightSliderMax = 3000;
	m_iAttrSliderRemoval = 1;
	m_iCenteredWindow = 1;
	m_iAutoSaveTime = 60;
	m_iAutoSave = 0;
	m_iEnablePython = 0;
	m_fMouseWheelMin = 200.0f;
	m_fMouseWheelMax = 80000.0f;
	m_iMaiWithoutMonsterArea = 1;
	m_iMinimapEffectSnapshot = 0;
	m_iEnableGr2Animation = 1;
	m_iCharacterShadowMapSize = 512;
	m_iEnableNewCompass = 0;
	m_iEnableObjectHierarchy = 1;
	m_iIntersectObjectPicking = 0;
	m_iAutoBackup = 1;
	m_iAutoBackupSubfolderMode = 0;
	m_iExportSubfolderMode = 0;
	m_iEnableAutogenProperty = 1;
	m_iAutogenPropertyEffect = 1;
	m_iNewObjectIdFromCrc = 1;
	m_iEnableTreeRotation = 0;
	m_iExportMaiAtlasAsPng = 1;
	m_iPreloadAllTerrains = 1;
	m_iEnableMapAttrColored = 1;
	m_strLocale = "EN";
	m_strAutoBackupFolder = "_backup";
	m_strExportFolder = "_export";
	m_strMapNamePrefix = "metin2_map_";
	m_strTextureSetLoadPath = "textureset\\metin2_a1.txt";
	m_strMobProtoPath = "locale/ymir/mob_proto";
}

void CWorldEditorConfig::Load()
{
	ApplyDefaults();

	char szExe[MAX_PATH];
	if (GetModuleFileNameA (NULL, szExe, MAX_PATH))
	{
		std::string strDir = szExe;
		const size_t nSlash = strDir.find_last_of ("\\/");
		if (nSlash != std::string::npos)
		{
			strDir.erase (nSlash);
			if (LoadFile ((strDir + "\\WorldEditorRemix.ini").c_str()))
			{
				return;
			}
			if (LoadFile ((strDir + "\\WorldEditor.txt").c_str()))
			{
				return;
			}
		}
	}

	if (LoadFile ("WorldEditorRemix.ini"))
	{
		return;
	}

	LoadFile ("WorldEditor.txt");
}

bool CWorldEditorConfig::LoadFile (const char * c_szPath)
{
	FILE * pFile = fopen (c_szPath, "r");
	if (!pFile)
	{
		return false;
	}

	char szLine[1024];
	std::set<std::string> kSeenKeys;
	bool bDuplicate = false;
	while (fgets (szLine, sizeof (szLine), pFile))
	{
		std::string strLine = szLine;
		bool bQuote = false;
		size_t nComment = std::string::npos;
		for (size_t i = 0; i < strLine.size(); ++i)
		{
			if (strLine[i] == '"')
			{
				bQuote = !bQuote;
			}
			else if (!bQuote && (strLine[i] == '#' || strLine[i] == ';'))
			{
				nComment = i;
				break;
			}
		}
		if (nComment != std::string::npos)
		{
			strLine.erase (nComment);
		}

		const size_t nEqual = strLine.find ('=');
		if (nEqual == std::string::npos)
		{
			continue;
		}

		std::string strKey = strLine.substr (0, nEqual);
		std::string strValue = strLine.substr (nEqual + 1);
		TrimConfig (strKey);
		TrimConfig (strValue);
		UnquoteConfig (strValue);

		for (size_t i = 0; i < strKey.size(); ++i)
		{
			if (strKey[i] >= 'a' && strKey[i] <= 'z')
			{
				strKey[i] = (char) (strKey[i] - 'a' + 'A');
			}
		}

		if (!kSeenKeys.insert (strKey).second)
		{
			if (!bDuplicate)
			{
				Tracef ("WorldEditorConfig: Duplicate keys found in %s:\n", c_szPath);
				bDuplicate = true;
			}
			Tracef ("WorldEditorConfig: Warning - duplicate keys detected in %s\n", c_szPath);
		}

		ApplyKey (strKey, strValue);
	}

	fclose (pFile);
	return true;
}

void CWorldEditorConfig::ApplyKey (const std::string & c_rstrKey, const std::string & c_rstrValue)
{
	if (c_rstrKey == "VIEW_CHAR_OUTPUT_BY_DEFAULT")
	{
		m_iViewChar = ConfigToInt (c_rstrValue);
	}
	else if (c_rstrKey == "VIEW_SHADOW_OUTPUT_BY_DEFAULT")
	{
		m_iViewShadow = ConfigToInt (c_rstrValue);
	}
	else if (c_rstrKey == "VIEW_WATER_OUTPUT_BY_DEFAULT")
	{
		m_iViewWater = ConfigToInt (c_rstrValue);
	}
	else if (c_rstrKey == "WINDOW_FOV_SIZE")
	{
		m_fWindowFov = (float) atof (c_rstrValue.c_str());
	}
	else if (c_rstrKey == "WASD_MINIMAL_MOVE")
	{
		m_iWasdMinimalMove = ConfigToInt (c_rstrValue);
		if (m_iWasdMinimalMove < 1)
		{
			m_iWasdMinimalMove = 1;
		}
	}
	else if (c_rstrKey == "NO_GOTO_AFTER_INSERT")
	{
		m_iNoGotoAfterInsert = ConfigToInt (c_rstrValue);
	}
	else if (c_rstrKey == "NOMAI_ATLAS_DUMP")
	{
		m_iNoMaiAtlasDump = ConfigToInt (c_rstrValue);
	}
	else if (c_rstrKey == "NOMINIMAP_RAWALPHA")
	{
		m_iNoMinimapRawAlpha = ConfigToInt (c_rstrValue);
	}
	else if (c_rstrKey == "DETECT_MDATR_HEIGHT")
	{
		m_iDetectMdatrHeight = ConfigToInt (c_rstrValue);
	}
	else if (c_rstrKey == "NOFOG_ONMAPLOAD")
	{
		m_iNoFogOnMapLoad = ConfigToInt (c_rstrValue);
	}
	else if (c_rstrKey == "REFRESHALL_ONUPDATEUI")
	{
		m_iRefreshAllOnUpdateUI = ConfigToInt (c_rstrValue);
	}
	else if (c_rstrKey == "NEW_MAP_MAPNAME_PREFIX")
	{
		m_strMapNamePrefix = c_rstrValue;
	}
	else if (c_rstrKey == "NEWMAP_TEXTURESETLOADPATH")
	{
		m_strTextureSetLoadPath = c_rstrValue;
	}
	else if (c_rstrKey == "NEWMAP_TEXTURESETSAVEASMAPNAME")
	{
		m_iTextureSetSaveAsMapName = ConfigToInt (c_rstrValue);
	}
	else if (c_rstrKey == "SERVERATTR_REMOVE_WEIRD_FLAGS")
	{
		m_iRemoveWeirdAttrFlags = ConfigToInt (c_rstrValue);
	}
	else if (c_rstrKey == "VIEW_OBJECT_LIGHTING")
	{
		m_iViewObjectLighting = ConfigToInt (c_rstrValue);
	}
	else if (c_rstrKey == "VIEW_MONSTER_AREA_INFO")
	{
		m_iViewMonsterAreaInfo = ConfigToInt (c_rstrValue);
	}
	else if (c_rstrKey == "MOB_PROTO_PATH")
	{
		if (!c_rstrValue.empty())
		{
			m_strMobProtoPath = c_rstrValue;
		}
	}
	else if (c_rstrKey == "RENDER_CURSOR_COLOR_R")
	{
		m_fCursorColorR = (float) atof (c_rstrValue.c_str());
		if (m_fCursorColorR < 0.0f)
			m_fCursorColorR = 0.0f;
		if (m_fCursorColorR > 1.0f)
			m_fCursorColorR = 1.0f;
	}
	else if (c_rstrKey == "RENDER_CURSOR_COLOR_G")
	{
		m_fCursorColorG = (float) atof (c_rstrValue.c_str());
		if (m_fCursorColorG < 0.0f)
			m_fCursorColorG = 0.0f;
		if (m_fCursorColorG > 1.0f)
			m_fCursorColorG = 1.0f;
	}
	else if (c_rstrKey == "RENDER_CURSOR_COLOR_B")
	{
		m_fCursorColorB = (float) atof (c_rstrValue.c_str());
		if (m_fCursorColorB < 0.0f)
			m_fCursorColorB = 0.0f;
		if (m_fCursorColorB > 1.0f)
			m_fCursorColorB = 1.0f;
	}
	else if (c_rstrKey == "OBJECT_HEIGHT_MAX")
	{
		m_fObjectHeightMax = (float) atof (c_rstrValue.c_str());
		if (m_fObjectHeightMax < (float) m_iObjectHeightSliderMax)
			m_fObjectHeightMax = (float) m_iObjectHeightSliderMax;
	}
	else if (c_rstrKey == "OBJECT_HEIGHT_SLIDER_MAX")
	{
		m_iObjectHeightSliderMax = ConfigToInt (c_rstrValue);
		if (m_iObjectHeightSliderMax < 1)
			m_iObjectHeightSliderMax = 1;
		if (m_fObjectHeightMax < (float) m_iObjectHeightSliderMax)
			m_fObjectHeightMax = (float) m_iObjectHeightSliderMax;
	}
	else if (c_rstrKey == "ATTR_SLIDER_REMOVAL")
	{
		m_iAttrSliderRemoval = ConfigToInt (c_rstrValue);
	}
	else if (c_rstrKey == "CENTERED_WINDOW")
	{
		m_iCenteredWindow = ConfigToInt (c_rstrValue);
	}
	else if (c_rstrKey == "AUTO_SAVE_TIME")
	{
		m_iAutoSaveTime = ConfigToInt (c_rstrValue);
		if (m_iAutoSaveTime < 1)
			m_iAutoSaveTime = 1;
		Tracef ("WorldEditorConfig: AUTO_SAVE_TIME %d\n", m_iAutoSaveTime);
	}
	else if (c_rstrKey == "AUTO_SAVE")
	{
		m_iAutoSave = ConfigToInt (c_rstrValue);
		Tracef ("WorldEditorConfig: AUTO_SAVE %d\n", m_iAutoSave);
	}
	else if (c_rstrKey == "LOCALE")
	{
		if (!c_rstrValue.empty())
			m_strLocale = c_rstrValue;
		Tracef ("WorldEditorConfig: LOCALE %s\n", m_strLocale.c_str());
	}
	else if (c_rstrKey == "ENABLE_PYTHON")
	{
		m_iEnablePython = ConfigToInt (c_rstrValue);
		Tracef ("WorldEditorConfig: ENABLE_PYTHON %d\n", m_iEnablePython);
	}
	else if (c_rstrKey == "MOUSE_WHEEL_MIN")
	{
		m_fMouseWheelMin = (float) atof (c_rstrValue.c_str());
		if (m_fMouseWheelMin < 1.0f)
			m_fMouseWheelMin = 1.0f;
		Tracef ("WorldEditorConfig: MOUSE_WHEEL_MIN %.f\n", m_fMouseWheelMin);
	}
	else if (c_rstrKey == "MOUSE_WHEEL_MAX")
	{
		m_fMouseWheelMax = (float) atof (c_rstrValue.c_str());
		if (m_fMouseWheelMax < m_fMouseWheelMin)
			m_fMouseWheelMax = m_fMouseWheelMin;
		Tracef ("WorldEditorConfig: MOUSE_WHEEL_MAX %.f\n", m_fMouseWheelMax);
	}
	else if (c_rstrKey == "MAI_IMAGE_WITHOUT_MONSTER_AREA_INFO")
	{
		m_iMaiWithoutMonsterArea = ConfigToInt (c_rstrValue);
		Tracef ("WorldEditorConfig: MAI_IMAGE_WITHOUT_MONSTER_AREA_INFO %d\n", m_iMaiWithoutMonsterArea);
	}
	else if (c_rstrKey == "MINIMAP_EFFECT_SNAPSHOT")
	{
		m_iMinimapEffectSnapshot = ConfigToInt (c_rstrValue);
		Tracef ("WorldEditorConfig: MINIMAP_EFFECT_SNAPSHOT %d\n", m_iMinimapEffectSnapshot);
	}
	else if (c_rstrKey == "ENABLE_GR2_ANIMATION")
	{
		m_iEnableGr2Animation = ConfigToInt (c_rstrValue);
		Tracef ("WorldEditorConfig: ENABLE_GR2_ANIMATION %d\n", m_iEnableGr2Animation);
	}
	else if (c_rstrKey == "CHARACTER_SHADOW_MAP_SIZE")
	{
		m_iCharacterShadowMapSize = ConfigToInt (c_rstrValue);
		if (m_iCharacterShadowMapSize < 64)
			m_iCharacterShadowMapSize = 64;
		Tracef ("WorldEditorConfig: CHARACTER_SHADOW_MAP_SIZE %d\n", m_iCharacterShadowMapSize);
	}
	else if (c_rstrKey == "ENABLE_NEW_COMPASS")
	{
		m_iEnableNewCompass = ConfigToInt (c_rstrValue);
		Tracef ("WorldEditorConfig: ENABLE_NEW_COMPASS %d\n", m_iEnableNewCompass);
	}
	else if (c_rstrKey == "ENABLE_OBJECT_HIERARCHY")
	{
		m_iEnableObjectHierarchy = ConfigToInt (c_rstrValue);
		Tracef ("WorldEditorConfig: ENABLE_OBJECT_HIERARCHY %d\n", m_iEnableObjectHierarchy);
	}
	else if (c_rstrKey == "INTERSECT_OBJECT_PICKING")
	{
		m_iIntersectObjectPicking = ConfigToInt (c_rstrValue);
		Tracef ("WorldEditorConfig: INTERSECT_OBJECT_PICKING %d\n", m_iIntersectObjectPicking);
	}
	else if (c_rstrKey == "AUTO_BACKUP")
	{
		m_iAutoBackup = ConfigToInt (c_rstrValue);
		Tracef ("WorldEditorConfig: AUTO_BACKUP %d\n", m_iAutoBackup);
	}
	else if (c_rstrKey == "AUTO_BACKUP_FOLDER")
	{
		if (!c_rstrValue.empty())
			m_strAutoBackupFolder = c_rstrValue;
		Tracef ("WorldEditorConfig: AUTO_BACKUP_FOLDER %s\n", m_strAutoBackupFolder.c_str());
	}
	else if (c_rstrKey == "AUTO_BACKUP_SUBFOLDER_MODE")
	{
		m_iAutoBackupSubfolderMode = ConfigToInt (c_rstrValue);
		if (m_iAutoBackupSubfolderMode < 0 || m_iAutoBackupSubfolderMode > 3)
			m_iAutoBackupSubfolderMode = 0;
		Tracef ("WorldEditorConfig: AUTO_BACKUP_SUBFOLDER_MODE %d\n", m_iAutoBackupSubfolderMode);
	}
	else if (c_rstrKey == "EXPORT_FOLDER")
	{
		if (!c_rstrValue.empty())
			m_strExportFolder = c_rstrValue;
		Tracef ("WorldEditorConfig: EXPORT_FOLDER %s\n", m_strExportFolder.c_str());
	}
	else if (c_rstrKey == "EXPORT_SUBFOLDER_MODE")
	{
		m_iExportSubfolderMode = ConfigToInt (c_rstrValue);
		if (m_iExportSubfolderMode < 0 || m_iExportSubfolderMode > 3)
			m_iExportSubfolderMode = 0;
		Tracef ("WorldEditorConfig: EXPORT_SUBFOLDER_MODE %d\n", m_iExportSubfolderMode);
	}
	else if (c_rstrKey == "ENABLE_AUTOGEN_PROPERTY")
	{
		m_iEnableAutogenProperty = ConfigToInt (c_rstrValue);
		Tracef ("WorldEditorConfig: ENABLE_AUTOGEN_PROPERTY %d\n", m_iEnableAutogenProperty);
	}
	else if (c_rstrKey == "AUTOGEN_PROPERTY_EFFECT")
	{
		m_iAutogenPropertyEffect = ConfigToInt (c_rstrValue);
		Tracef ("WorldEditorConfig: AUTOGEN_PROPERTY_EFFECT %d\n", m_iAutogenPropertyEffect);
	}
	else if (c_rstrKey == "NEW_OBJECT_ID_FROM_CRC")
	{
		m_iNewObjectIdFromCrc = ConfigToInt (c_rstrValue);
		Tracef ("WorldEditorConfig: NEW_OBJECT_ID_FROM_CRC %d\n", m_iNewObjectIdFromCrc);
	}
	else if (c_rstrKey == "ENABLE_TREE_ROTATION")
	{
		m_iEnableTreeRotation = ConfigToInt (c_rstrValue);
		Tracef ("WorldEditorConfig: ENABLE_TREE_ROTATION %d\n", m_iEnableTreeRotation);
	}
	else if (c_rstrKey == "EXPORT_MAI_ATLAS_AS_PNG")
	{
		m_iExportMaiAtlasAsPng = ConfigToInt (c_rstrValue);
		Tracef ("WorldEditorConfig: EXPORT_MAI_ATLAS_AS_PNG %d\n", m_iExportMaiAtlasAsPng);
	}
	else if (c_rstrKey == "PRELOAD_ALL_TERRAINS")
	{
		m_iPreloadAllTerrains = ConfigToInt (c_rstrValue);
		Tracef ("WorldEditorConfig: PRELOAD_ALL_TERRAINS %d\n", m_iPreloadAllTerrains);
	}
	else if (c_rstrKey == "ENABLE_MAP_ATTR_COLORED")
	{
		m_iEnableMapAttrColored = ConfigToInt (c_rstrValue);
		Tracef ("WorldEditorConfig: ENABLE_MAP_ATTR_COLORED %d\n", m_iEnableMapAttrColored);
	}
}