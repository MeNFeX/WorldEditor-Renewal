#pragma once

#include "../eterGrnLib/ModelInstance.h"
#include "./DockingBar/PageCtrl.h"

void CreateBitmapButton (CButton * pButton, int iBitmapID, CBitmap & rBitmap);
void RepositioningWindow (CPageCtrl & rPageCtrl, int ixTemporarySpace, int iyTemporarySpace);
void CreateHighColorImageList (int iBitmapID, CImageList * pImageList);

void SetDialogFloatText (HWND hWnd, int iID, float fData);
void SetDialogIntegerText (HWND hWnd, int iID, int iData);
float GetDialogFloatText (HWND hWnd, int iID);
int GetDialogIntegerText (HWND hWnd, int iID);
void SelectComboBoxItem (CComboBox & rComboBox, DWORD dwIndex);

extern char g_szProgramPath[PROGRAM_PATH_LENGTH + 1];
extern char g_szProgramWindowPath[PROGRAM_PATH_LENGTH + 1];

extern const char* GetProgramExcutingPath (const char* c_szFullFileName);

void CreateUtilData();
void DestroyUtilData();

///////////////////////////////////////////////////////////////////////////////////////////////////

void RenderSphere (const D3DXMATRIX * c_pmatWorld, float fx, float fy, float fz, float fRadius, BOOL isFilled = FALSE);
void RenderCylinder (const D3DXMATRIX * c_pmatWorld, float fx, float fy, float fz, float fRadius, float fLength, BOOL isFilled = FALSE);

///////////////////////////////////////////////////////////////////////////////////////////////////

class CTileInstance : public CGrannyModelInstance
{
	public:
		CTileInstance() {}
		~CTileInstance() {}

		void LoadTexture();
		void RenderTile();

	protected:
		CGraphicImageInstance m_ImageInstance;
};

class COpenFileDialog
{
	public:
		COpenFileDialog (CWnd * pParentWnd = NULL, DWORD dwFlag = 0, const char* c_pszTitle = NULL, const char* c_pszFilter = NULL, const char* c_pszDefaultPath = NULL,
						 const char* c_pfirstName = NULL) :
			m_pWnd (pParentWnd),
			m_dwFlag (dwFlag),
			mc_pszTitle (c_pszTitle),
			mc_pszDefaultPath (c_pszDefaultPath)
		{
			strncpy (m_szFilter, c_pszFilter, MAX_PATH);

			char* p = m_szFilter;
			while ((p = strchr (p, '|')))
			{
				* (p++) = '\0';
			}

			CWinApp * pApplication = (CWinApp*) AfxGetApp();
			assert (pApplication);

			memset (m_szFileName, 0, sizeof (m_szFileName));
			memset (m_szPathName, 0, sizeof (m_szPathName));
			if (c_pfirstName)
			{
				strcpy (m_szPathName, c_pfirstName);
			}

			memset (&m_ofn, 0, sizeof (OPENFILENAME));

			m_ofn.lStructSize		= sizeof (OPENFILENAME);
			m_ofn.hwndOwner			= m_pWnd->GetSafeHwnd();
			m_ofn.hInstance			= pApplication->m_hInstance;
			m_ofn.lpstrFilter		= m_szFilter;
			m_ofn.nFilterIndex		= 1L;

			m_ofn.lpstrFile			= m_szPathName;
			m_ofn.nMaxFile			= sizeof (m_szPathName);
			m_ofn.lpstrFileTitle	= m_szFileName;
			m_ofn.nMaxFileTitle		= sizeof (m_szFileName);

			m_ofn.Flags				= OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_ENABLESIZING | m_dwFlag;
			m_ofn.lpstrTitle		= mc_pszTitle;
			m_ofn.lpstrInitialDir	= mc_pszDefaultPath;
			m_ofn.lpstrDefExt		= NULL;
		}

		~COpenFileDialog() {};

		virtual bool DoModal()
		{
			if (!GetOpenFileName (&m_ofn))
			{
				return FALSE;
			}

			return TRUE;
		}

		const char* GetPathName()
		{
			return m_szPathName;
		}

	protected:
		char			m_szPathName[MAX_PATH + 1];		// Filename including path and extension
		char			m_szFileName[MAX_PATH + 1];		// Filename and extension without path
		char			m_szFilter[MAX_PATH + 1];			// File Filter

		CWnd* 			m_pWnd;
		DWORD			m_dwFlag;
		const char* 	mc_pszTitle;
		const char* 	mc_pszDefaultPath;
		OPENFILENAME	m_ofn;
};

class CSaveFileDialog : public COpenFileDialog
{
	public:
		CSaveFileDialog (CWnd * pParentWnd = NULL, DWORD dwFlag = 0, const char* c_pszTitle = NULL, const char* c_pszFilter = NULL, const char* c_pszDefaultPath = NULL,
						 const char* c_pfirstName = NULL) :
			COpenFileDialog (pParentWnd, dwFlag, c_pszTitle, c_pszFilter, c_pszDefaultPath,  c_pfirstName)
		{
		}

		virtual bool DoModal()
		{
			if (!GetSaveFileName (&m_ofn))
			{
				return FALSE;
			}

			return TRUE;
		}
};

extern void RenderBackGroundCharacter (float fx, float fy, float fz);
extern void RenderBackGroundTile();
extern void DestroyBackGroundData();

extern std::string RemoveStartString (const std::string & strOriginal, const std::string & strBegin);

enum
{
	IDC_WE_WATER_SET0 = 45110,
	IDC_WE_WATER_PLUS = 45111,
	IDC_WE_WATER_MINUS = 45112,
	IDC_WE_TEXTURE_CHANGE = 45113,
	IDC_WE_TEXTURE_REMOVE = 45114,
	IDC_WE_NEWMAP_TEXTURESET = 45115,
	IDC_WE_UNK11_SPEED = 45116,
};

class CWorldEditorConfig
{
	public:
		static CWorldEditorConfig & Instance();

		void Load();

		int ViewChar() const { return m_iViewChar; }
		int ViewShadow() const { return m_iViewShadow; }
		int ViewWater() const { return m_iViewWater; }
		float WindowFov() const { return m_fWindowFov; }
		int WasdMinimalMove() const { return m_iWasdMinimalMove; }
		bool NoGotoAfterInsert() const { return m_iNoGotoAfterInsert != 0; }
		bool NoMaiAtlasDump() const { return m_iNoMaiAtlasDump != 0; }
		bool NoMinimapRawAlpha() const { return m_iNoMinimapRawAlpha != 0; }
		bool DetectMdatrHeight() const { return m_iDetectMdatrHeight != 0; }
		bool NoFogOnMapLoad() const { return m_iNoFogOnMapLoad != 0; }
		bool RefreshAllOnUpdateUI() const { return m_iRefreshAllOnUpdateUI != 0; }
		const std::string & MapNamePrefix() const { return m_strMapNamePrefix; }
		const std::string & TextureSetLoadPath() const { return m_strTextureSetLoadPath; }
		bool TextureSetSaveAsMapName() const { return m_iTextureSetSaveAsMapName != 0; }
		bool RemoveWeirdAttrFlags() const { return m_iRemoveWeirdAttrFlags != 0; }
		bool ViewObjectLighting() const { return m_iViewObjectLighting != 0; }
		bool ViewMonsterAreaInfo() const { return m_iViewMonsterAreaInfo != 0; }
		const std::string & MobProtoPath() const { return m_strMobProtoPath; }
		float CursorColorR() const { return m_fCursorColorR; }
		float CursorColorG() const { return m_fCursorColorG; }
		float CursorColorB() const { return m_fCursorColorB; }
		float ObjectHeightMax() const { return m_fObjectHeightMax; }
		int ObjectHeightSliderMax() const { return m_iObjectHeightSliderMax; }
		bool AttrSliderRemoval() const { return m_iAttrSliderRemoval != 0; }
		bool CenteredWindow() const { return m_iCenteredWindow != 0; }
		int AutoSaveTime() const { return m_iAutoSaveTime; }
		bool AutoSave() const { return m_iAutoSave != 0; }
		const std::string & Locale() const { return m_strLocale; }
		bool EnablePython() const { return m_iEnablePython != 0; }
		float MouseWheelMin() const { return m_fMouseWheelMin; }
		float MouseWheelMax() const { return m_fMouseWheelMax; }
		bool MaiImageWithoutMonsterAreaInfo() const { return m_iMaiWithoutMonsterArea != 0; }
		bool MinimapEffectSnapshot() const { return m_iMinimapEffectSnapshot != 0; }
		bool EnableGr2Animation() const { return m_iEnableGr2Animation != 0; }
		int CharacterShadowMapSize() const { return m_iCharacterShadowMapSize; }
		bool EnableNewCompass() const { return m_iEnableNewCompass != 0; }
		bool EnableObjectHierarchy() const { return m_iEnableObjectHierarchy != 0; }
		bool IntersectObjectPicking() const { return m_iIntersectObjectPicking != 0; }
		bool AutoBackup() const { return m_iAutoBackup != 0; }
		void SetAutoBackup (bool on) { m_iAutoBackup = on ? 1 : 0; }
		const std::string & AutoBackupFolder() const { return m_strAutoBackupFolder; }
		int AutoBackupSubfolderMode() const { return m_iAutoBackupSubfolderMode; }
		const std::string & ExportFolder() const { return m_strExportFolder; }
		int ExportSubfolderMode() const { return m_iExportSubfolderMode; }
		bool EnableAutogenProperty() const { return m_iEnableAutogenProperty != 0; }
		bool AutogenPropertyEffect() const { return m_iAutogenPropertyEffect != 0; }
		bool NewObjectIdFromCrc() const { return m_iNewObjectIdFromCrc != 0; }
		bool EnableTreeRotation() const { return m_iEnableTreeRotation != 0; }
		bool ExportMaiAtlasAsPng() const { return m_iExportMaiAtlasAsPng != 0; }
		bool PreloadAllTerrains() const { return m_iPreloadAllTerrains != 0; }
		bool EnableMapAttrColored() const { return m_iEnableMapAttrColored != 0; }

	private:
		CWorldEditorConfig();
		void ApplyDefaults();
		bool LoadFile (const char * c_szPath);
		void ApplyKey (const std::string & c_rstrKey, const std::string & c_rstrValue);

		int m_iViewChar;
		int m_iViewShadow;
		int m_iViewWater;
		float m_fWindowFov;
		int m_iWasdMinimalMove;
		int m_iNoGotoAfterInsert;
		int m_iNoMaiAtlasDump;
		int m_iNoMinimapRawAlpha;
		int m_iDetectMdatrHeight;
		int m_iNoFogOnMapLoad;
		int m_iRefreshAllOnUpdateUI;
		int m_iTextureSetSaveAsMapName;
		int m_iRemoveWeirdAttrFlags;
		int m_iViewObjectLighting;
		int m_iViewMonsterAreaInfo;
		float m_fCursorColorR;
		float m_fCursorColorG;
		float m_fCursorColorB;
		float m_fObjectHeightMax;
		int m_iObjectHeightSliderMax;
		int m_iAttrSliderRemoval;
		int m_iCenteredWindow;
		int m_iAutoSaveTime;
		int m_iAutoSave;
		int m_iEnablePython;
		int m_iMaiWithoutMonsterArea;
		int m_iMinimapEffectSnapshot;
		int m_iEnableGr2Animation;
		int m_iCharacterShadowMapSize;
		int m_iEnableNewCompass;
		int m_iEnableObjectHierarchy;
		int m_iIntersectObjectPicking;
		int m_iAutoBackup;
		int m_iAutoBackupSubfolderMode;
		int m_iExportSubfolderMode;
		int m_iEnableAutogenProperty;
		int m_iAutogenPropertyEffect;
		int m_iNewObjectIdFromCrc;
		int m_iEnableTreeRotation;
		int m_iExportMaiAtlasAsPng;
		int m_iPreloadAllTerrains;
		int m_iEnableMapAttrColored;
		float m_fMouseWheelMin;
		float m_fMouseWheelMax;
		std::string m_strLocale;
		std::string m_strAutoBackupFolder;
		std::string m_strExportFolder;
		std::string m_strMapNamePrefix;
		std::string m_strTextureSetLoadPath;
		std::string m_strMobProtoPath;
};
