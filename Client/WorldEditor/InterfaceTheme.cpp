#include "StdAfx.h"
#include "InterfaceTheme.h"
#include "Resource.h"
#include "UI/GTColorDialogDlg.h"

#include <dwmapi.h>
#include <uxtheme.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "msimg32.lib")

namespace
{
	enum PreferredAppMode
	{
		APPMODE_DEFAULT = 0,
		APPMODE_ALLOW_DARK = 1,
		APPMODE_FORCE_DARK = 2,
		APPMODE_FORCE_LIGHT = 3
	};

	struct ThemePalette
	{
		COLORREF bg;
		COLORREF edit;
		COLORREF text;
		COLORREF caption;
	};

	const ThemePalette kDark = { RGB (32, 32, 32), RGB (45, 45, 48), RGB (230, 230, 230), RGB (28, 28, 28) };
	const ThemePalette kBlue = { RGB (16, 36, 64), RGB (10, 28, 52), RGB (214, 228, 245), RGB (12, 32, 58) };
	const ThemePalette kWarm = { RGB (54, 38, 28), RGB (42, 28, 20), RGB (255, 236, 214), RGB (48, 32, 22) };
	const ThemePalette kForest = { RGB (22, 42, 30), RGB (14, 32, 22), RGB (214, 236, 214), RGB (16, 36, 24) };
	const ThemePalette kSlate = { RGB (44, 48, 58), RGB (32, 36, 46), RGB (226, 230, 238), RGB (36, 40, 50) };

	UINT s_color = ID_INTERFACE_COLOR_LIGHT;
	UINT s_font = ID_INTERFACE_FONT_DEFAULT;
	UINT s_size = ID_INTERFACE_SIZE_DEFAULT;
	COLORREF s_customBg = RGB (32, 32, 32);
	COLORREF s_customText = RGB (230, 230, 230);
	ThemePalette s_custom = { RGB (32, 32, 32), RGB (24, 24, 24), RGB (230, 230, 230), RGB (28, 28, 28) };
	bool s_pickText = false;
	CGTColorDialogDlg * s_pickerDlg = NULL;
	HWND s_picker = NULL;
	HFONT s_fontHandle = NULL;
	HBRUSH s_bgBrush = NULL;
	HBRUSH s_editBrush = NULL;
	COLORREF s_text = RGB (0, 0, 0);
	COLORREF s_bg = RGB (255, 255, 255);
	COLORREF s_edit = RGB (255, 255, 255);
	COLORREF s_caption = RGB (255, 255, 255);
	HHOOK s_hook = NULL;
	int s_applying = 0;
	bool s_loaded = false;

	bool Colored()
	{
		return s_color != ID_INTERFACE_COLOR_LIGHT;
	}

	COLORREF Mix (COLORREF a, COLORREF b, int percentB)
	{
		if (percentB < 0)
			percentB = 0;
		if (percentB > 100)
			percentB = 100;
		const int percentA = 100 - percentB;
		return RGB (
			(GetRValue (a) * percentA + GetRValue (b) * percentB) / 100,
			(GetGValue (a) * percentA + GetGValue (b) * percentB) / 100,
			(GetBValue (a) * percentA + GetBValue (b) * percentB) / 100);
	}

	bool CustomFont()
	{
		return s_font != ID_INTERFACE_FONT_DEFAULT || s_size != ID_INTERFACE_SIZE_DEFAULT;
	}

	void UpdateCustomPalette()
	{
		s_custom.bg = s_customBg;
		s_custom.text = s_customText;
		s_custom.edit = Mix (s_customBg, RGB (0, 0, 0), 18);
		s_custom.caption = Mix (s_customBg, RGB (0, 0, 0), 12);
	}

	const ThemePalette * Palette()
	{
		switch (s_color)
		{
			case ID_INTERFACE_COLOR_DARK: return &kDark;
			case ID_INTERFACE_COLOR_BLUE: return &kBlue;
			case ID_INTERFACE_COLOR_WARM: return &kWarm;
			case ID_INTERFACE_COLOR_FOREST: return &kForest;
			case ID_INTERFACE_COLOR_SLATE: return &kSlate;
			case ID_INTERFACE_COLOR_CUSTOM:
				UpdateCustomPalette();
				return &s_custom;
			default: return NULL;
		}
	}

	const char * ColorName (UINT nId)
	{
		switch (nId)
		{
			case ID_INTERFACE_COLOR_DARK: return "dark";
			case ID_INTERFACE_COLOR_BLUE: return "blue";
			case ID_INTERFACE_COLOR_WARM: return "warm";
			case ID_INTERFACE_COLOR_FOREST: return "forest";
			case ID_INTERFACE_COLOR_SLATE: return "slate";
			case ID_INTERFACE_COLOR_CUSTOM: return "custom";
			default: return "default";
		}
	}

	const char * FontName (UINT nId)
	{
		switch (nId)
		{
			case ID_INTERFACE_FONT_SEGOE: return "segoe";
			case ID_INTERFACE_FONT_TAHOMA: return "tahoma";
			case ID_INTERFACE_FONT_VERDANA: return "verdana";
			case ID_INTERFACE_FONT_CONSOLAS: return "consolas";
			case ID_INTERFACE_FONT_GEORGIA: return "georgia";
			default: return "default";
		}
	}

	const char * FontFace (UINT nId)
	{
		switch (nId)
		{
			case ID_INTERFACE_FONT_SEGOE: return "Segoe UI";
			case ID_INTERFACE_FONT_TAHOMA: return "Tahoma";
			case ID_INTERFACE_FONT_VERDANA: return "Verdana";
			case ID_INTERFACE_FONT_CONSOLAS: return "Consolas";
			case ID_INTERFACE_FONT_GEORGIA: return "Georgia";
			default: return "MS Shell Dlg";
		}
	}

	int FontPoint()
	{
		switch (s_size)
		{
			case ID_INTERFACE_SIZE_8: return 8;
			case ID_INTERFACE_SIZE_9: return 9;
			case ID_INTERFACE_SIZE_10: return 10;
			case ID_INTERFACE_SIZE_11: return 11;
			case ID_INTERFACE_SIZE_12: return 12;
			default: return 9;
		}
	}

	UINT ColorFromName (const std::string & name)
	{
		if (name == "dark") return ID_INTERFACE_COLOR_DARK;
		if (name == "blue") return ID_INTERFACE_COLOR_BLUE;
		if (name == "warm") return ID_INTERFACE_COLOR_WARM;
		if (name == "forest") return ID_INTERFACE_COLOR_FOREST;
		if (name == "slate") return ID_INTERFACE_COLOR_SLATE;
		if (name == "custom") return ID_INTERFACE_COLOR_CUSTOM;
		return ID_INTERFACE_COLOR_LIGHT;
	}

	UINT FontFromName (const std::string & name)
	{
		if (name == "segoe") return ID_INTERFACE_FONT_SEGOE;
		if (name == "tahoma") return ID_INTERFACE_FONT_TAHOMA;
		if (name == "verdana") return ID_INTERFACE_FONT_VERDANA;
		if (name == "consolas") return ID_INTERFACE_FONT_CONSOLAS;
		if (name == "georgia") return ID_INTERFACE_FONT_GEORGIA;
		return ID_INTERFACE_FONT_DEFAULT;
	}

	UINT SizeFromNumber (int size)
	{
		switch (size)
		{
			case 8: return ID_INTERFACE_SIZE_8;
			case 9: return ID_INTERFACE_SIZE_9;
			case 10: return ID_INTERFACE_SIZE_10;
			case 11: return ID_INTERFACE_SIZE_11;
			case 12: return ID_INTERFACE_SIZE_12;
			default: return ID_INTERFACE_SIZE_DEFAULT;
		}
	}

	std::string ExeIniPath()
	{
		char szExe[MAX_PATH];
		if (!GetModuleFileNameA (NULL, szExe, MAX_PATH))
			return "WorldEditorRemix.ini";
		std::string path = szExe;
		const size_t slash = path.find_last_of ("\\/");
		if (slash != std::string::npos)
			path.erase (slash);
		return path + "\\WorldEditorRemix.ini";
	}

	std::string Trim (const std::string & text)
	{
		size_t begin = 0;
		while (begin < text.size() && (text[begin] == ' ' || text[begin] == '\t'))
			++begin;
		size_t end = text.size();
		while (end > begin && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\r' || text[end - 1] == '\n'))
			--end;
		return text.substr (begin, end - begin);
	}

	void LoadSettings()
	{
		if (s_loaded)
			return;
		s_loaded = true;
		FILE * file = fopen (ExeIniPath().c_str(), "rb");
		if (!file)
			return;
		char line[1024];
		while (fgets (line, sizeof (line), file))
		{
			std::string text = line;
			const size_t comment = text.find_first_of (";#");
			if (comment != std::string::npos)
				text.erase (comment);
			const size_t equal = text.find ('=');
			if (equal == std::string::npos)
				continue;
			std::string key = Trim (text.substr (0, equal));
			std::string value = Trim (text.substr (equal + 1));
			for (size_t i = 0; i < key.size(); ++i)
			{
				if (key[i] >= 'a' && key[i] <= 'z')
					key[i] = (char) (key[i] - 'a' + 'A');
			}
			for (size_t i = 0; i < value.size(); ++i)
			{
				if (value[i] >= 'A' && value[i] <= 'Z')
					value[i] = (char) (value[i] - 'A' + 'a');
			}
			if (key == "INTERFACE_COLOR")
				s_color = ColorFromName (value);
			else if (key == "INTERFACE_CUSTOM_BG" || key == "INTERFACE_CUSTOM_TEXT")
			{
				int red = 0, green = 0, blue = 0;
				if (sscanf (value.c_str(), "%d,%d,%d", &red, &green, &blue) == 3)
				{
					const COLORREF color = RGB (
						red < 0 ? 0 : (red > 255 ? 255 : red),
						green < 0 ? 0 : (green > 255 ? 255 : green),
						blue < 0 ? 0 : (blue > 255 ? 255 : blue));
					if (key == "INTERFACE_CUSTOM_BG")
						s_customBg = color;
					else
						s_customText = color;
				}
			}
			else if (key == "INTERFACE_FONT")
				s_font = FontFromName (value);
			else if (key == "INTERFACE_FONT_SIZE")
				s_size = SizeFromNumber (atoi (value.c_str()));
		}
		fclose (file);
	}

	void ReplaceOrAppend (std::string & body, const char * key, const std::string & value)
	{
		size_t pos = 0;
		bool found = false;
		while (pos < body.size())
		{
			size_t end = body.find ('\n', pos);
			if (end == std::string::npos)
				end = body.size();
			std::string line = body.substr (pos, end - pos);
			std::string probe = Trim (line);
			for (size_t i = 0; i < probe.size(); ++i)
			{
				if (probe[i] >= 'a' && probe[i] <= 'z')
					probe[i] = (char) (probe[i] - 'a' + 'A');
			}
			const size_t equal = probe.find ('=');
			if (equal != std::string::npos && Trim (probe.substr (0, equal)) == key)
			{
				const std::string written = std::string (key) + " = " + value;
				body.replace (pos, end - pos, written);
				found = true;
				break;
			}
			pos = end < body.size() ? end + 1 : body.size();
		}
		if (!found)
		{
			if (!body.empty() && body[body.size() - 1] != '\n')
				body += "\r\n";
			body += std::string (key) + " = " + value + "\r\n";
		}
	}

	void SaveSettings()
	{
		const std::string path = ExeIniPath();
		std::string body;
		FILE * file = fopen (path.c_str(), "rb");
		if (file)
		{
			char chunk[4096];
			size_t count = 0;
			while ((count = fread (chunk, 1, sizeof (chunk), file)) > 0)
				body.append (chunk, count);
			fclose (file);
		}
		char sizeText[16];
		sprintf (sizeText, "%d", s_size == ID_INTERFACE_SIZE_DEFAULT ? 0 : FontPoint());
		char bgText[32];
		char textColor[32];
		sprintf (bgText, "%u,%u,%u", GetRValue (s_customBg), GetGValue (s_customBg), GetBValue (s_customBg));
		sprintf (textColor, "%u,%u,%u", GetRValue (s_customText), GetGValue (s_customText), GetBValue (s_customText));
		ReplaceOrAppend (body, "INTERFACE_COLOR", ColorName (s_color));
		ReplaceOrAppend (body, "INTERFACE_CUSTOM_BG", bgText);
		ReplaceOrAppend (body, "INTERFACE_CUSTOM_TEXT", textColor);
		ReplaceOrAppend (body, "INTERFACE_FONT", FontName (s_font));
		ReplaceOrAppend (body, "INTERFACE_FONT_SIZE", sizeText);
		file = fopen (path.c_str(), "wb");
		if (!file)
			return;
		fwrite (body.data(), 1, body.size(), file);
		fclose (file);
	}

	LRESULT CALLBACK ThemeProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	HWND ViewWindow()
	{
		CWnd * main = AfxGetMainWnd();
		if (!main)
			return NULL;
		CFrameWnd * frame = dynamic_cast<CFrameWnd *> (main);
		if (!frame)
			return NULL;
		CWnd * view = frame->GetActiveView();
		return view ? view->GetSafeHwnd() : NULL;
	}

	bool IsStatus (HWND hwnd)
	{
		char name[64];
		GetClassNameA (hwnd, name, sizeof (name));
		return strcmp (name, "msctls_statusbar32") == 0;
	}

	bool IsColorPicker (HWND hwnd)
	{
		if (!hwnd)
			return false;
		if (s_picker && (hwnd == s_picker || IsChild (s_picker, hwnd)))
			return true;
		if (GetDlgItem (hwnd, IDC_COLORPICKER))
			return true;
		HWND parent = GetParent (hwnd);
		return parent && GetDlgItem (parent, IDC_COLORPICKER);
	}

	bool IsCommonFileDialog (HWND hwnd)
	{
		if (!hwnd || (GetWindowLong (hwnd, GWL_STYLE) & WS_CHILD))
			return false;
		char name[32];
		if (!GetClassNameA (hwnd, name, sizeof (name)) || strcmp (name, "#32770") != 0)
			return false;
		if (FindWindowExA (hwnd, NULL, "DUIViewWndClassName", NULL) || FindWindowExA (hwnd, NULL, "DirectUIHWND", NULL))
			return true;
		return GetDlgItem (hwnd, 0x0480) && GetDlgItem (hwnd, 0x0470);
	}

	bool IsShellUi (HWND hwnd)
	{
		for (HWND window = hwnd; window; window = GetParent (window))
		{
			if (IsCommonFileDialog (window))
				return true;
			if ((GetWindowLong (window, GWL_STYLE) & WS_CHILD) == 0)
				break;
		}
		return false;
	}

	void EnsureSubclass (HWND hwnd)
	{
		if (!hwnd || hwnd == ViewWindow() || IsColorPicker (hwnd) || IsShellUi (hwnd))
			return;
		if (GetPropA (hwnd, "WEOldProc"))
			return;
		WNDPROC oldProc = (WNDPROC) GetWindowLongPtr (hwnd, GWLP_WNDPROC);
		if (!oldProc || oldProc == ThemeProc)
			return;
		SetPropA (hwnd, "WEOldProc", (HANDLE) oldProc);
		SetWindowLongPtr (hwnd, GWLP_WNDPROC, (LONG_PTR) ThemeProc);
	}

	void ApplyFont (HWND hwnd)
	{
		if (!hwnd || hwnd == ViewWindow() || IsColorPicker (hwnd) || IsShellUi (hwnd))
			return;
		HFONT current = (HFONT) SendMessage (hwnd, WM_GETFONT, 0, 0);
		if (!GetPropA (hwnd, "WEOrigFont"))
			SetPropA (hwnd, "WEOrigFont", (HANDLE) current);
		if (!CustomFont())
		{
			HFONT original = (HFONT) GetPropA (hwnd, "WEOrigFont");
			SendMessage (hwnd, WM_SETFONT, (WPARAM) original, TRUE);
			return;
		}
		if (s_fontHandle)
			SendMessage (hwnd, WM_SETFONT, (WPARAM) s_fontHandle, TRUE);
	}

	void ApplyControl (HWND hwnd)
	{
		if (!hwnd || hwnd == ViewWindow() || IsColorPicker (hwnd) || IsShellUi (hwnd))
			return;
		char name[128];
		GetClassNameA (hwnd, name, sizeof (name));
		const bool classic = Colored();
		if (strcmp (name, "Button") == 0 || strcmp (name, "Edit") == 0 || strcmp (name, "ComboBox") == 0 ||
			strcmp (name, "ComboLBox") == 0 || strcmp (name, "ListBox") == 0 || strcmp (name, "Static") == 0)
		{
			SetWindowTheme (hwnd, classic ? L"" : NULL, classic ? L"" : NULL);
			SetWindowPos (hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
		}
		else if (strcmp (name, "ToolbarWindow32") == 0)
		{
			SetWindowTheme (hwnd, classic ? L"" : NULL, classic ? L"" : NULL);
			if (classic)
			{
				COLORSCHEME scheme;
				scheme.dwSize = sizeof (scheme);
				scheme.clrBtnHighlight = s_bg;
				scheme.clrBtnShadow = Mix (s_bg, s_text, 35);
				SendMessage (hwnd, TB_SETCOLORSCHEME, 0, (LPARAM) &scheme);
			}
		}
		else if (strcmp (name, "SysTreeView32") == 0)
		{
			TreeView_SetBkColor (hwnd, classic ? s_edit : GetSysColor (COLOR_WINDOW));
			TreeView_SetTextColor (hwnd, classic ? s_text : GetSysColor (COLOR_WINDOWTEXT));
		}
		else if (strcmp (name, "SysListView32") == 0)
		{
			ListView_SetBkColor (hwnd, classic ? s_edit : GetSysColor (COLOR_WINDOW));
			ListView_SetTextColor (hwnd, classic ? s_text : GetSysColor (COLOR_WINDOWTEXT));
			ListView_SetTextBkColor (hwnd, classic ? s_edit : GetSysColor (COLOR_WINDOW));
		}
	}

	BOOL CALLBACK ApplyChild (HWND hwnd, LPARAM)
	{
		if (IsColorPicker (hwnd) || IsShellUi (hwnd))
			return TRUE;
		EnsureSubclass (hwnd);
		ApplyFont (hwnd);
		ApplyControl (hwnd);
		InvalidateRect (hwnd, NULL, TRUE);
		return TRUE;
	}

	void ApplyWindowTree (HWND root)
	{
		if (!root || IsColorPicker (root) || IsShellUi (root))
			return;
		EnsureSubclass (root);
		ApplyFont (root);
		ApplyControl (root);
		EnumChildWindows (root, ApplyChild, 0);
		InvalidateRect (root, NULL, TRUE);
	}

	void ApplyTitleBar (HWND hwnd)
	{
		if (!hwnd)
			return;
		BOOL dark = FALSE;
		DwmSetWindowAttribute (hwnd, 19, &dark, sizeof (dark));
		DwmSetWindowAttribute (hwnd, 20, &dark, sizeof (dark));
		COLORREF caption = Colored() ? s_caption : GetSysColor (COLOR_ACTIVECAPTION);
		COLORREF captionText = Colored() ? s_text : GetSysColor (COLOR_CAPTIONTEXT);
		DwmSetWindowAttribute (hwnd, 35, &caption, sizeof (caption));
		DwmSetWindowAttribute (hwnd, 36, &captionText, sizeof (captionText));
		SetWindowPos (hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
		HMODULE themeLib = GetModuleHandleA ("uxtheme.dll");
		if (!themeLib)
			themeLib = LoadLibraryA ("uxtheme.dll");
		if (themeLib)
		{
			typedef BOOL (WINAPI * AllowFn) (HWND, BOOL);
			AllowFn allow = (AllowFn) GetProcAddress (themeLib, MAKEINTRESOURCEA (133));
			if (allow)
				allow (hwnd, FALSE);
		}
	}

	BOOL CALLBACK ApplyTopLevel (HWND hwnd, LPARAM)
	{
		DWORD processId = 0;
		GetWindowThreadProcessId (hwnd, &processId);
		if (processId != GetCurrentProcessId() || IsShellUi (hwnd))
			return TRUE;
		ApplyTitleBar (hwnd);
		ApplyWindowTree (hwnd);
		return TRUE;
	}

	void RebuildFont()
	{
		HFONT created = NULL;
		if (CustomFont())
		{
			HDC screen = GetDC (NULL);
			const int height = -MulDiv (FontPoint(), GetDeviceCaps (screen, LOGPIXELSY), 72);
			ReleaseDC (NULL, screen);
			created = CreateFontA (height, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, FontFace (s_font));
		}
		HFONT previous = s_fontHandle;
		s_fontHandle = created;
		if (previous)
			DeleteObject (previous);
	}

	void RebuildBrushes()
	{
		HBRUSH oldBg = s_bgBrush;
		HBRUSH oldEdit = s_editBrush;
		const ThemePalette * palette = Palette();
		if (!palette)
		{
			s_bgBrush = NULL;
			s_editBrush = NULL;
		}
		else
		{
			s_bg = palette->bg;
			s_edit = palette->edit;
			s_text = palette->text;
			s_caption = palette->caption;
			s_bgBrush = CreateSolidBrush (s_bg);
			s_editBrush = CreateSolidBrush (s_edit);
		}
		if (oldBg)
			DeleteObject (oldBg);
		if (oldEdit)
			DeleteObject (oldEdit);
	}

	void SetImmersiveMode()
	{
		HMODULE theme = GetModuleHandleA ("uxtheme.dll");
		if (!theme)
			theme = LoadLibraryA ("uxtheme.dll");
		if (!theme)
			return;
		typedef PreferredAppMode (WINAPI * SetModeFn) (PreferredAppMode);
		typedef void (WINAPI * VoidFn)();
		SetModeFn setMode = (SetModeFn) GetProcAddress (theme, MAKEINTRESOURCEA (135));
		VoidFn refresh = (VoidFn) GetProcAddress (theme, MAKEINTRESOURCEA (104));
		VoidFn flush = (VoidFn) GetProcAddress (theme, MAKEINTRESOURCEA (136));
		if (setMode)
			setMode (APPMODE_FORCE_LIGHT);
		if (refresh)
			refresh();
		if (flush)
			flush();
	}

	void ApplyMenus();

	void ApplyNow()
	{
		if (s_applying)
			return;
		++s_applying;
		RebuildBrushes();
		RebuildFont();
		SetImmersiveMode();
		EnumThreadWindows (GetCurrentThreadId(), ApplyTopLevel, 0);
		ApplyMenus();
		--s_applying;
	}

	const DWORD kMenuMagic = 0x314D4557;

	struct MenuLabel
	{
		DWORD magic;
		int popup;
		int bar;
		char text[384];
	};

	HFONT MenuFont()
	{
		if (CustomFont() && s_fontHandle)
			return s_fontHandle;
		static HFONT systemFont = NULL;
		if (!systemFont)
		{
			NONCLIENTMETRICSA metrics;
			memset (&metrics, 0, sizeof (metrics));
			metrics.cbSize = sizeof (metrics);
			if (SystemParametersInfoA (SPI_GETNONCLIENTMETRICS, sizeof (metrics), &metrics, 0))
				systemFont = CreateFontIndirectA (&metrics.lfMenuFont);
		}
		return systemFont ? systemFont : (HFONT) GetStockObject (DEFAULT_GUI_FONT);
	}

	void RestoreMenu (HMENU menu)
	{
		if (!menu)
			return;
		const int count = GetMenuItemCount (menu);
		for (int i = 0; i < count; ++i)
		{
			MENUITEMINFOA info;
			memset (&info, 0, sizeof (info));
			info.cbSize = sizeof (info);
			info.fMask = MIIM_FTYPE | MIIM_SUBMENU | MIIM_DATA;
			if (!GetMenuItemInfoA (menu, i, TRUE, &info))
				continue;
			if (info.hSubMenu)
				RestoreMenu (info.hSubMenu);
			MenuLabel * label = (MenuLabel *) info.dwItemData;
			const bool ours = label && label->magic == kMenuMagic;
			if ((info.fType & MFT_OWNERDRAW) || ours)
			{
				char text[384];
				text[0] = 0;
				if (ours)
					lstrcpynA (text, label->text, sizeof (text));
				info.fMask = MIIM_FTYPE | MIIM_DATA | MIIM_STRING;
				info.fType &= ~MFT_OWNERDRAW;
				info.fType |= MFT_STRING;
				info.dwTypeData = text;
				info.cch = (UINT) lstrlenA (text);
				info.dwItemData = 0;
				SetMenuItemInfoA (menu, i, TRUE, &info);
			}
			if (ours)
				delete label;
		}
		MENUINFO menuInfo;
		memset (&menuInfo, 0, sizeof (menuInfo));
		menuInfo.cbSize = sizeof (menuInfo);
		menuInfo.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
		menuInfo.hbrBack = NULL;
		SetMenuInfo (menu, &menuInfo);
	}

	void PrepareMenu (HMENU menu, bool bar)
	{
		if (!menu || !Colored() || !s_bgBrush)
			return;
		const int count = GetMenuItemCount (menu);
		for (int i = 0; i < count; ++i)
		{
			MENUITEMINFOA info;
			memset (&info, 0, sizeof (info));
			info.cbSize = sizeof (info);
			info.fMask = MIIM_FTYPE | MIIM_SUBMENU | MIIM_DATA;
			if (!GetMenuItemInfoA (menu, i, TRUE, &info))
				continue;
			if (info.hSubMenu)
				PrepareMenu (info.hSubMenu, false);
			if (bar || (info.fType & (MFT_SEPARATOR | MFT_BITMAP)))
				continue;
			char text[384];
			text[0] = 0;
			GetMenuStringA (menu, (UINT) i, text, sizeof (text), MF_BYPOSITION);
			MenuLabel * label = (MenuLabel *) info.dwItemData;
			if (!(info.fType & MFT_OWNERDRAW) || !label || label->magic != kMenuMagic)
			{
				label = new MenuLabel;
				label->magic = kMenuMagic;
				label->text[0] = 0;
			}
			label->bar = bar ? 1 : 0;
			label->popup = (!bar && info.hSubMenu) ? 1 : 0;
			if (text[0])
				lstrcpynA (label->text, text, sizeof (label->text));
			info.fMask = MIIM_FTYPE | MIIM_DATA;
			info.fType = (info.fType & ~(MFT_STRING | MFT_BITMAP)) | MFT_OWNERDRAW;
			info.dwItemData = (ULONG_PTR) label;
			SetMenuItemInfoA (menu, i, TRUE, &info);
		}
		MENUINFO menuInfo;
		memset (&menuInfo, 0, sizeof (menuInfo));
		menuInfo.cbSize = sizeof (menuInfo);
		menuInfo.fMask = MIM_BACKGROUND | MIM_APPLYTOSUBMENUS;
		menuInfo.hbrBack = s_bgBrush;
		SetMenuInfo (menu, &menuInfo);
	}

	void ApplyMenus()
	{
		CWnd * main = AfxGetMainWnd();
		if (!main)
			return;
		HMENU menu = GetMenu (main->GetSafeHwnd());
		if (!menu)
			return;
		if (Colored())
			PrepareMenu (menu, true);
		else
			RestoreMenu (menu);
		DrawMenuBar (main->GetSafeHwnd());
	}

	bool IsButtonClass (HWND hwnd)
	{
		char name[32];
		return GetClassNameA (hwnd, name, sizeof (name)) && strcmp (name, "Button") == 0;
	}

	bool IsPushButton (HWND hwnd)
	{
		if (!IsButtonClass (hwnd))
			return false;
		const LONG style = GetWindowLong (hwnd, GWL_STYLE);
		const UINT type = (UINT) style & BS_TYPEMASK;
		if (type == BS_OWNERDRAW || (style & (BS_BITMAP | BS_ICON)))
			return false;
		return type == BS_PUSHBUTTON || type == BS_DEFPUSHBUTTON;
	}

	bool IsImageButton (HWND hwnd)
	{
		if (!IsButtonClass (hwnd))
			return false;
		const LONG style = GetWindowLong (hwnd, GWL_STYLE);
		const UINT type = (UINT) style & BS_TYPEMASK;
		if (type == BS_OWNERDRAW)
			return false;
		return (style & (BS_BITMAP | BS_ICON)) != 0;
	}

	bool IsDisabledEdit (HWND hwnd)
	{
		char name[32];
		if (!GetClassNameA (hwnd, name, sizeof (name)) || strcmp (name, "Edit") != 0)
			return false;
		return IsWindowEnabled (hwnd) == FALSE;
	}

	LRESULT PaintPushButton (HWND hwnd)
	{
		PAINTSTRUCT paint;
		HDC dc = BeginPaint (hwnd, &paint);
		RECT rc;
		GetClientRect (hwnd, &rc);
		const int width = rc.right - rc.left;
		const int height = rc.bottom - rc.top;
		HDC memory = CreateCompatibleDC (dc);
		HBITMAP bitmap = width > 0 && height > 0 ? CreateCompatibleBitmap (dc, width, height) : NULL;
		HGDIOBJ oldBitmap = bitmap ? SelectObject (memory, bitmap) : NULL;
		HDC target = bitmap ? memory : dc;
		const bool enabled = IsWindowEnabled (hwnd) != FALSE;
		const LRESULT state = SendMessage (hwnd, BM_GETSTATE, 0, 0);
		const bool pressed = (state & BST_PUSHED) != 0;
		const bool hot = GetPropA (hwnd, "WEHot") != NULL;
		const bool focused = GetFocus() == hwnd || (state & BST_FOCUS) != 0;
		const LONG style = GetWindowLong (hwnd, GWL_STYLE);
		const int lift = pressed ? 8 : (hot ? 24 : 16);
		const COLORREF face = Mix (s_bg, s_text, enabled ? lift : 8);
		const COLORREF border = Mix (s_bg, s_text, (style & BS_DEFPUSHBUTTON) || focused ? 62 : 42);
		HBRUSH brush = CreateSolidBrush (face);
		FillRect (target, &rc, brush);
		DeleteObject (brush);
		HPEN pen = CreatePen (PS_SOLID, 1, border);
		HGDIOBJ oldPen = SelectObject (target, pen);
		HGDIOBJ oldBrush = SelectObject (target, GetStockObject (NULL_BRUSH));
		Rectangle (target, rc.left, rc.top, rc.right, rc.bottom);
		SelectObject (target, oldPen);
		SelectObject (target, oldBrush);
		DeleteObject (pen);
		char text[512];
		text[0] = 0;
		GetWindowTextA (hwnd, text, sizeof (text));
		SetBkMode (target, TRANSPARENT);
		SetTextColor (target, enabled ? s_text : Mix (s_text, s_bg, 50));
		HFONT font = (HFONT) SendMessage (hwnd, WM_GETFONT, 0, 0);
		HGDIOBJ oldFont = font ? SelectObject (target, font) : NULL;
		RECT textRc = rc;
		if (pressed)
		{
			textRc.left += 1;
			textRc.top += 1;
		}
		DrawTextA (target, text, -1, &textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
		if (oldFont)
			SelectObject (target, oldFont);
		if (bitmap)
		{
			BitBlt (dc, 0, 0, width, height, memory, 0, 0, SRCCOPY);
			SelectObject (memory, oldBitmap);
			DeleteObject (bitmap);
		}
		DeleteDC (memory);
		EndPaint (hwnd, &paint);
		return 0;
	}

	void DrawKeyedBitmap (HDC dc, HBITMAP bitmap, int x, int y, COLORREF face)
	{
		BITMAP info;
		if (!bitmap || !GetObject (bitmap, sizeof (info), &info) || info.bmWidth <= 0 || info.bmHeight <= 0)
			return;
		HDC source = CreateCompatibleDC (dc);
		HGDIOBJ oldSource = SelectObject (source, bitmap);
		BITMAPINFO header;
		memset (&header, 0, sizeof (header));
		header.bmiHeader.biSize = sizeof (BITMAPINFOHEADER);
		header.bmiHeader.biWidth = info.bmWidth;
		header.bmiHeader.biHeight = -info.bmHeight;
		header.bmiHeader.biPlanes = 1;
		header.bmiHeader.biBitCount = 32;
		header.bmiHeader.biCompression = BI_RGB;
		void * bits = NULL;
		HBITMAP dib = CreateDIBSection (dc, &header, DIB_RGB_COLORS, &bits, NULL, 0);
		HDC memory = CreateCompatibleDC (dc);
		HGDIOBJ oldMemory = dib ? SelectObject (memory, dib) : NULL;
		if (dib && bits)
		{
			BitBlt (memory, 0, 0, info.bmWidth, info.bmHeight, source, 0, 0, SRCCOPY);
			DWORD * pixels = (DWORD *) bits;
			const int count = info.bmWidth * info.bmHeight;
			for (int i = 0; i < count; ++i)
			{
				const int blue = pixels[i] & 255;
				const int green = (pixels[i] >> 8) & 255;
				const int red = (pixels[i] >> 16) & 255;
				if (abs (red - 192) < 16 && abs (green - 192) < 16 && abs (blue - 192) < 16)
					pixels[i] = face;
			}
			BitBlt (dc, x, y, info.bmWidth, info.bmHeight, memory, 0, 0, SRCCOPY);
		}
		if (oldMemory)
			SelectObject (memory, oldMemory);
		if (dib)
			DeleteObject (dib);
		DeleteDC (memory);
		SelectObject (source, oldSource);
		DeleteDC (source);
	}

	LRESULT PaintImageButton (HWND hwnd)
	{
		PAINTSTRUCT paint;
		HDC dc = BeginPaint (hwnd, &paint);
		RECT rc;
		GetClientRect (hwnd, &rc);
		const int width = rc.right - rc.left;
		const int height = rc.bottom - rc.top;
		HDC memory = CreateCompatibleDC (dc);
		HBITMAP buffer = width > 0 && height > 0 ? CreateCompatibleBitmap (dc, width, height) : NULL;
		HGDIOBJ oldBuffer = buffer ? SelectObject (memory, buffer) : NULL;
		HDC target = buffer ? memory : dc;
		const bool enabled = IsWindowEnabled (hwnd) != FALSE;
		const LRESULT state = SendMessage (hwnd, BM_GETSTATE, 0, 0);
		const bool pressed = (state & BST_PUSHED) != 0;
		const bool checked = (state & BST_CHECKED) != 0 || SendMessage (hwnd, BM_GETCHECK, 0, 0) == BST_CHECKED;
		const bool hot = GetPropA (hwnd, "WEHot") != NULL;
		const int lift = pressed || checked ? 34 : (hot ? 24 : 16);
		const COLORREF face = Mix (s_bg, s_text, enabled ? lift : 8);
		const COLORREF border = Mix (s_bg, s_text, checked ? 70 : 42);
		HBRUSH brush = CreateSolidBrush (face);
		FillRect (target, &rc, brush);
		DeleteObject (brush);
		HPEN pen = CreatePen (PS_SOLID, 1, border);
		HGDIOBJ oldPen = SelectObject (target, pen);
		HGDIOBJ oldBrush = SelectObject (target, GetStockObject (NULL_BRUSH));
		Rectangle (target, rc.left, rc.top, rc.right, rc.bottom);
		SelectObject (target, oldPen);
		SelectObject (target, oldBrush);
		DeleteObject (pen);
		const LONG style = GetWindowLong (hwnd, GWL_STYLE);
		if (style & BS_ICON)
		{
			HICON icon = (HICON) SendMessage (hwnd, BM_GETIMAGE, IMAGE_ICON, 0);
			if (icon)
				DrawIconEx (target, (width - 16) / 2, (height - 16) / 2, icon, 16, 16, 0, NULL, DI_NORMAL);
		}
		else
		{
			HBITMAP image = (HBITMAP) SendMessage (hwnd, BM_GETIMAGE, IMAGE_BITMAP, 0);
			BITMAP info;
			if (image && GetObject (image, sizeof (info), &info))
			{
				const int x = (width - info.bmWidth) / 2 + (pressed ? 1 : 0);
				const int y = (height - info.bmHeight) / 2 + (pressed ? 1 : 0);
				DrawKeyedBitmap (target, image, x, y, face);
			}
		}
		if (buffer)
		{
			BitBlt (dc, 0, 0, width, height, memory, 0, 0, SRCCOPY);
			SelectObject (memory, oldBuffer);
			DeleteObject (buffer);
		}
		DeleteDC (memory);
		EndPaint (hwnd, &paint);
		return 0;
	}

	LRESULT PaintDisabledEdit (HWND hwnd)
	{
		PAINTSTRUCT paint;
		HDC dc = BeginPaint (hwnd, &paint);
		RECT rc;
		GetClientRect (hwnd, &rc);
		FillRect (dc, &rc, s_editBrush ? s_editBrush : s_bgBrush);
		HPEN pen = CreatePen (PS_SOLID, 1, Mix (s_bg, s_text, 40));
		HGDIOBJ oldPen = SelectObject (dc, pen);
		HGDIOBJ oldBrush = SelectObject (dc, GetStockObject (NULL_BRUSH));
		Rectangle (dc, rc.left, rc.top, rc.right, rc.bottom);
		SelectObject (dc, oldPen);
		SelectObject (dc, oldBrush);
		DeleteObject (pen);
		char text[512];
		text[0] = 0;
		GetWindowTextA (hwnd, text, sizeof (text));
		SetBkMode (dc, TRANSPARENT);
		SetTextColor (dc, Mix (s_text, s_bg, 35));
		HFONT font = (HFONT) SendMessage (hwnd, WM_GETFONT, 0, 0);
		HGDIOBJ oldFont = font ? SelectObject (dc, font) : NULL;
		RECT textRc = rc;
		textRc.left += 4;
		textRc.right -= 2;
		DrawTextA (dc, text, -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
		if (oldFont)
			SelectObject (dc, oldFont);
		EndPaint (hwnd, &paint);
		return 0;
	}

	void TrackHot (HWND hwnd, UINT message)
	{
		if (message == WM_MOUSEMOVE && !GetPropA (hwnd, "WEHot"))
		{
			SetPropA (hwnd, "WEHot", (HANDLE) 1);
			TRACKMOUSEEVENT track;
			track.cbSize = sizeof (track);
			track.dwFlags = TME_LEAVE;
			track.hwndTrack = hwnd;
			track.dwHoverTime = 0;
			TrackMouseEvent (&track);
			InvalidateRect (hwnd, NULL, FALSE);
		}
		else if (message == WM_MOUSELEAVE)
		{
			RemovePropA (hwnd, "WEHot");
			InvalidateRect (hwnd, NULL, FALSE);
		}
	}

	void DrawMenuMark (HDC dc, const RECT & rc, COLORREF color)
	{
		HPEN pen = CreatePen (PS_SOLID, 2, color);
		HGDIOBJ old = SelectObject (dc, pen);
		const int x = rc.left + 7;
		const int y = (rc.top + rc.bottom) / 2;
		MoveToEx (dc, x, y, NULL);
		LineTo (dc, x + 3, y + 4);
		LineTo (dc, x + 9, y - 4);
		SelectObject (dc, old);
		DeleteObject (pen);
	}

	void DrawMenuArrow (HDC dc, const RECT & rc, COLORREF color)
	{
		const int x = rc.right - 12;
		const int y = (rc.top + rc.bottom) / 2;
		POINT points[3];
		points[0].x = x;
		points[0].y = y - 4;
		points[1].x = x + 5;
		points[1].y = y;
		points[2].x = x;
		points[2].y = y + 4;
		HPEN pen = CreatePen (PS_SOLID, 1, color);
		HBRUSH brush = CreateSolidBrush (color);
		HGDIOBJ oldPen = SelectObject (dc, pen);
		HGDIOBJ oldBrush = SelectObject (dc, brush);
		Polygon (dc, points, 3);
		SelectObject (dc, oldPen);
		SelectObject (dc, oldBrush);
		DeleteObject (pen);
		DeleteObject (brush);
	}

	bool MeasureMenuItem (MEASUREITEMSTRUCT * measure)
	{
		if (!measure || measure->CtlType != ODT_MENU)
			return false;
		MenuLabel * label = (MenuLabel *) measure->itemData;
		if (!label || label->magic != kMenuMagic)
			return false;
		HDC dc = GetDC (NULL);
		HFONT font = MenuFont();
		HGDIOBJ old = SelectObject (dc, font);
		RECT rc;
		memset (&rc, 0, sizeof (rc));
		DrawTextA (dc, label->text, -1, &rc, DT_CALCRECT | DT_SINGLELINE | DT_NOPREFIX);
		SelectObject (dc, old);
		ReleaseDC (NULL, dc);
		const int textWidth = rc.right - rc.left;
		const int textHeight = rc.bottom - rc.top;
		measure->itemHeight = textHeight + 8;
		if (measure->itemHeight < 22)
			measure->itemHeight = 22;
		measure->itemWidth = textWidth + (label->bar ? 18 : (label->popup ? 40 : 28));
		if (strchr (label->text, '\t'))
			measure->itemWidth += 72;
		return true;
	}

	bool DrawMenuItem (DRAWITEMSTRUCT * draw)
	{
		if (!draw || draw->CtlType != ODT_MENU)
			return false;
		MenuLabel * label = (MenuLabel *) draw->itemData;
		if (!label || label->magic != kMenuMagic)
			return false;
		const bool selected = (draw->itemState & (ODS_SELECTED | ODS_HOTLIGHT)) != 0;
		const bool gray = (draw->itemState & ODS_GRAYED) != 0;
		const COLORREF face = selected ? Mix (s_bg, s_text, 28) : s_bg;
		const COLORREF text = gray ? Mix (s_text, s_bg, 48) : s_text;
		HBRUSH brush = CreateSolidBrush (face);
		FillRect (draw->hDC, &draw->rcItem, brush);
		DeleteObject (brush);
		HFONT font = MenuFont();
		HGDIOBJ oldFont = SelectObject (draw->hDC, font);
		SetBkMode (draw->hDC, TRANSPARENT);
		SetTextColor (draw->hDC, text);
		RECT textRc = draw->rcItem;
		textRc.left += label->bar ? 8 : 22;
		textRc.right -= label->popup ? 18 : 8;
		char copy[384];
		lstrcpynA (copy, label->text, sizeof (copy));
		char * tab = strchr (copy, '\t');
		UINT format = DT_SINGLELINE | DT_VCENTER | DT_LEFT;
		if (draw->itemState & ODS_NOACCEL)
			format |= DT_HIDEPREFIX;
		if (tab)
		{
			*tab = 0;
			DrawTextA (draw->hDC, copy, -1, &textRc, format);
			DrawTextA (draw->hDC, tab + 1, -1, &textRc, DT_SINGLELINE | DT_VCENTER | DT_RIGHT | DT_NOPREFIX);
		}
		else
			DrawTextA (draw->hDC, copy, -1, &textRc, format);
		if (draw->itemState & ODS_CHECKED)
			DrawMenuMark (draw->hDC, draw->rcItem, text);
		if (label->popup)
			DrawMenuArrow (draw->hDC, draw->rcItem, text);
		SelectObject (draw->hDC, oldFont);
		return true;
	}

	void DrawToolbarButton (HWND toolbar, HDC dc, int index, const TBBUTTON & button, const RECT & bounds)
	{
		if (button.fsStyle & TBSTYLE_SEP)
		{
			const int x = (bounds.left + bounds.right) / 2;
			HPEN pen = CreatePen (PS_SOLID, 1, Mix (s_bg, s_text, 35));
			HGDIOBJ old = SelectObject (dc, pen);
			MoveToEx (dc, x, bounds.top + 4, NULL);
			LineTo (dc, x, bounds.bottom - 4);
			SelectObject (dc, old);
			DeleteObject (pen);
			return;
		}
		const DWORD hotIndex = (DWORD) SendMessage (toolbar, TB_GETHOTITEM, 0, 0);
		const bool hot = hotIndex == (DWORD) index;
		const bool pressed = (button.fsState & TBSTATE_PRESSED) != 0;
		const bool checked = (button.fsState & TBSTATE_CHECKED) != 0;
		if (hot || pressed || checked)
		{
			HBRUSH brush = CreateSolidBrush (Mix (s_bg, s_text, pressed ? 12 : 24));
			FillRect (dc, &bounds, brush);
			DeleteObject (brush);
		}
		const bool enabled = (button.fsState & TBSTATE_ENABLED) != 0;
		HIMAGELIST images = NULL;
		if (!enabled)
			images = (HIMAGELIST) SendMessage (toolbar, TB_GETDISABLEDIMAGELIST, 0, 0);
		else if (hot)
			images = (HIMAGELIST) SendMessage (toolbar, TB_GETHOTIMAGELIST, 0, 0);
		if (!images)
			images = (HIMAGELIST) SendMessage (toolbar, TB_GETIMAGELIST, 0, 0);
		if (!images || button.iBitmap < 0)
			return;
		int cx = 16;
		int cy = 16;
		ImageList_GetIconSize (images, &cx, &cy);
		if (cx < 1 || cy < 1)
			return;
		const int x = bounds.left + ((bounds.right - bounds.left) - cx) / 2;
		const int y = bounds.top + ((bounds.bottom - bounds.top) - cy) / 2;
		HDC memory = CreateCompatibleDC (dc);
		HBITMAP bitmap = CreateCompatibleBitmap (dc, cx, cy);
		HGDIOBJ oldBitmap = SelectObject (memory, bitmap);
		RECT box;
		box.left = 0;
		box.top = 0;
		box.right = cx;
		box.bottom = cy;
		HBRUSH key = CreateSolidBrush (RGB (255, 0, 255));
		FillRect (memory, &box, key);
		DeleteObject (key);
		ImageList_Draw (images, button.iBitmap, memory, 0, 0, ILD_TRANSPARENT);
		TransparentBlt (dc, x, y, cx, cy, memory, 0, 0, cx, cy, RGB (255, 0, 255));
		SelectObject (memory, oldBitmap);
		DeleteObject (bitmap);
		DeleteDC (memory);
	}

	LRESULT PaintToolbar (HWND hwnd)
	{
		PAINTSTRUCT paint;
		HDC dc = BeginPaint (hwnd, &paint);
		RECT rc;
		GetClientRect (hwnd, &rc);
		FillRect (dc, &rc, s_bgBrush);
		const int count = (int) SendMessage (hwnd, TB_BUTTONCOUNT, 0, 0);
		for (int i = 0; i < count; ++i)
		{
			TBBUTTON button;
			memset (&button, 0, sizeof (button));
			if (!SendMessage (hwnd, TB_GETBUTTON, i, (LPARAM) &button))
				continue;
			if (button.fsState & TBSTATE_HIDDEN)
				continue;
			RECT bounds;
			memset (&bounds, 0, sizeof (bounds));
			if (!SendMessage (hwnd, TB_GETITEMRECT, i, (LPARAM) &bounds))
				continue;
			DrawToolbarButton (hwnd, dc, i, button, bounds);
		}
		EndPaint (hwnd, &paint);
		return 0;
	}

#ifndef OBJID_MENU
#define OBJID_MENU ((LONG) 0xFFFFFFFD)
#endif
#define WM_UAHDRAWMENU_ 0x0091
#define WM_UAHDRAWMENUITEM_ 0x0092
#define WM_UAHMEASUREMENUITEM_ 0x0094

	typedef union tagUAHMENUITEMMETRICS
	{
		struct { DWORD cx; DWORD cy; } rgsizeBar[2];
		struct { DWORD cx; DWORD cy; } rgsizePopup[4];
	} UAHMENUITEMMETRICS;

	typedef struct tagUAHMENUPOPUPMETRICS
	{
		DWORD rgcx[4];
		DWORD fUpdateMaxWidths : 2;
	} UAHMENUPOPUPMETRICS;

	typedef struct tagUAHMENU
	{
		HMENU hmenu;
		HDC hdc;
		DWORD dwFlags;
	} UAHMENU;

	typedef struct tagUAHMENUITEM
	{
		int iPosition;
		UAHMENUITEMMETRICS umim;
		UAHMENUPOPUPMETRICS umpm;
	} UAHMENUITEM;

	typedef struct tagUAHDRAWMENUITEM
	{
		DRAWITEMSTRUCT dis;
		UAHMENU um;
		UAHMENUITEM umi;
	} UAHDRAWMENUITEM;

	typedef struct tagUAHMEASUREMENUITEM
	{
		MEASUREITEMSTRUCT mis;
		UAHMENU um;
		UAHMENUITEM umi;
	} UAHMEASUREMENUITEM;

	bool DrawUAHMenuBar (HWND hwnd, LPARAM lParam)
	{
		UAHMENU * menu = (UAHMENU *) lParam;
		if (!menu || !menu->hdc || !s_bgBrush)
			return false;
		MENUBARINFO info;
		memset (&info, 0, sizeof (info));
		info.cbSize = sizeof (info);
		if (!GetMenuBarInfo (hwnd, OBJID_MENU, 0, &info))
			return false;
		RECT windowRect;
		GetWindowRect (hwnd, &windowRect);
		RECT bar = info.rcBar;
		OffsetRect (&bar, -windowRect.left, -windowRect.top);
		FillRect (menu->hdc, &bar, s_bgBrush);
		return true;
	}

	bool DrawUAHMenuItem (LPARAM lParam)
	{
		UAHDRAWMENUITEM * item = (UAHDRAWMENUITEM *) lParam;
		if (!item || !item->um.hdc)
			return false;
		char text[256];
		text[0] = 0;
		GetMenuStringA (item->um.hmenu, (UINT) item->umi.iPosition, text, sizeof (text), MF_BYPOSITION);
		const bool hot = (item->dis.itemState & (ODS_HOTLIGHT | ODS_SELECTED)) != 0;
		const bool gray = (item->dis.itemState & (ODS_GRAYED | ODS_DISABLED)) != 0;
		const COLORREF face = hot ? Mix (s_bg, s_text, 28) : s_bg;
		HBRUSH brush = CreateSolidBrush (face);
		FillRect (item->um.hdc, &item->dis.rcItem, brush);
		DeleteObject (brush);
		SetBkMode (item->um.hdc, TRANSPARENT);
		SetTextColor (item->um.hdc, gray ? Mix (s_text, s_bg, 45) : s_text);
		HFONT font = MenuFont();
		HGDIOBJ oldFont = SelectObject (item->um.hdc, font);
		RECT textRc = item->dis.rcItem;
		UINT format = DT_CENTER | DT_VCENTER | DT_SINGLELINE;
		if (item->dis.itemState & ODS_NOACCEL)
			format |= DT_HIDEPREFIX;
		DrawTextA (item->um.hdc, text, -1, &textRc, format);
		SelectObject (item->um.hdc, oldFont);
		return true;
	}

	void PaintMenuBarLine (HWND hwnd)
	{
		if (!GetMenu (hwnd))
			return;
		MENUBARINFO info;
		memset (&info, 0, sizeof (info));
		info.cbSize = sizeof (info);
		if (!GetMenuBarInfo (hwnd, OBJID_MENU, 0, &info))
			return;
		RECT client;
		GetClientRect (hwnd, &client);
		MapWindowPoints (hwnd, NULL, (POINT *) &client, 2);
		RECT windowRect;
		GetWindowRect (hwnd, &windowRect);
		OffsetRect (&client, -windowRect.left, -windowRect.top);
		RECT gap;
		gap.left = 0;
		gap.right = windowRect.right - windowRect.left;
		gap.top = info.rcBar.bottom - windowRect.top;
		gap.bottom = client.top;
		if (gap.bottom <= gap.top)
			gap.bottom = gap.top + 2;
		HDC dc = GetWindowDC (hwnd);
		if (!dc)
			return;
		FillRect (dc, &gap, s_bgBrush);
		ReleaseDC (hwnd, dc);
	}

	void PaintDockGaps (HWND hwnd)
	{
		char name[128];
		GetClassNameA (hwnd, name, sizeof (name));
		if (strncmp (name, "Afx", 3) != 0 || !s_bgBrush)
			return;
		RECT client;
		GetClientRect (hwnd, &client);
		if (client.bottom - client.top > 120)
			return;
		HDC dc = GetDC (hwnd);
		if (!dc)
			return;
		HRGN region = CreateRectRgnIndirect (&client);
		for (HWND child = GetWindow (hwnd, GW_CHILD); child; child = GetWindow (child, GW_HWNDNEXT))
		{
			if (!IsWindowVisible (child))
				continue;
			RECT childRect;
			GetWindowRect (child, &childRect);
			MapWindowPoints (NULL, hwnd, (POINT *) &childRect, 2);
			HRGN childRegion = CreateRectRgnIndirect (&childRect);
			CombineRgn (region, region, childRegion, RGN_DIFF);
			DeleteObject (childRegion);
		}
		FillRgn (dc, region, s_bgBrush);
		DeleteObject (region);
		ReleaseDC (hwnd, dc);
		RECT windowRect;
		GetWindowRect (hwnd, &windowRect);
		RECT edge;
		edge.left = 0;
		edge.top = 0;
		edge.right = windowRect.right - windowRect.left;
		edge.bottom = 3;
		HDC windowDC = GetWindowDC (hwnd);
		if (!windowDC)
			return;
		FillRect (windowDC, &edge, s_bgBrush);
		ReleaseDC (hwnd, windowDC);
	}

	LRESULT PaintStatus (HWND hwnd, WNDPROC oldProc, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (!Colored())
			return CallWindowProc (oldProc, hwnd, message, wParam, lParam);
		if (message != WM_PAINT && message != WM_ERASEBKGND)
			return CallWindowProc (oldProc, hwnd, message, wParam, lParam);
		if (message == WM_ERASEBKGND)
		{
			RECT rect;
			GetClientRect (hwnd, &rect);
			FillRect ((HDC) wParam, &rect, s_bgBrush);
			return 1;
		}
		PAINTSTRUCT paint;
		HDC dc = BeginPaint (hwnd, &paint);
		RECT client;
		GetClientRect (hwnd, &client);
		FillRect (dc, &client, s_bgBrush);
		SetTextColor (dc, s_text);
		SetBkMode (dc, TRANSPARENT);
		HFONT font = (HFONT) SendMessage (hwnd, WM_GETFONT, 0, 0);
		HGDIOBJ oldFont = font ? SelectObject (dc, font) : NULL;
		int parts = (int) SendMessage (hwnd, SB_GETPARTS, 0, 0);
		if (parts < 1)
			parts = 1;
		for (int i = 0; i < parts; ++i)
		{
			RECT part;
			if (!SendMessage (hwnd, SB_GETRECT, i, (LPARAM) &part))
				continue;
			char text[512];
			text[0] = 0;
			SendMessageA (hwnd, SB_GETTEXTA, i, (LPARAM) text);
			part.left += 4;
			DrawTextA (dc, text, -1, &part, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
		}
		if (oldFont)
			SelectObject (dc, oldFont);
		EndPaint (hwnd, &paint);
		return 0;
	}

	LRESULT CALLBACK ThemeProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		WNDPROC oldProc = (WNDPROC) GetPropA (hwnd, "WEOldProc");
		if (!oldProc)
			return DefWindowProc (hwnd, message, wParam, lParam);

		if (message == WM_NCDESTROY)
		{
			SetWindowLongPtr (hwnd, GWLP_WNDPROC, (LONG_PTR) oldProc);
			RemovePropA (hwnd, "WEOldProc");
			RemovePropA (hwnd, "WEOrigFont");
			RemovePropA (hwnd, "WEHot");
			return CallWindowProc (oldProc, hwnd, message, wParam, lParam);
		}

		if (IsStatus (hwnd))
			return PaintStatus (hwnd, oldProc, message, wParam, lParam);

		if (Colored() && s_bgBrush && (message == WM_NCPAINT || message == WM_NCACTIVATE))
		{
			const LRESULT painted = CallWindowProc (oldProc, hwnd, message, wParam, lParam);
			PaintMenuBarLine (hwnd);
			return painted;
		}

		if (Colored() && s_bgBrush)
		{
			if (message == WM_UAHDRAWMENU_ && DrawUAHMenuBar (hwnd, lParam))
				return 0;
			if (message == WM_UAHDRAWMENUITEM_ && DrawUAHMenuItem (lParam))
				return 0;
			if (message == WM_UAHMEASUREMENUITEM_)
				return CallWindowProc (oldProc, hwnd, message, wParam, lParam);
			if (message == WM_PAINT)
			{
				char name[128];
				GetClassNameA (hwnd, name, sizeof (name));
				if (strcmp (name, "ToolbarWindow32") == 0)
					return PaintToolbar (hwnd);
				if (strncmp (name, "Afx", 3) == 0)
				{
					RECT client;
					GetClientRect (hwnd, &client);
					if (client.bottom - client.top <= 120)
					{
						const LRESULT painted = CallWindowProc (oldProc, hwnd, message, wParam, lParam);
						PaintDockGaps (hwnd);
						return painted;
					}
				}
			}
			if (message == WM_MEASUREITEM && MeasureMenuItem ((MEASUREITEMSTRUCT *) lParam))
				return TRUE;
			if (message == WM_DRAWITEM && DrawMenuItem ((DRAWITEMSTRUCT *) lParam))
				return TRUE;
			if (IsPushButton (hwnd) || IsImageButton (hwnd))
			{
				if (message == WM_PAINT)
					return IsImageButton (hwnd) ? PaintImageButton (hwnd) : PaintPushButton (hwnd);
				if (message == WM_ERASEBKGND)
					return 1;
				TrackHot (hwnd, message);
			}
			if (IsDisabledEdit (hwnd))
			{
				if (message == WM_PAINT)
					return PaintDisabledEdit (hwnd);
				if (message == WM_ERASEBKGND)
					return 1;
			}
			if (message == WM_INITMENU || message == WM_INITMENUPOPUP)
			{
				const LRESULT handled = CallWindowProc (oldProc, hwnd, message, wParam, lParam);
				PrepareMenu ((HMENU) wParam, message == WM_INITMENU);
				return handled;
			}
			if (message == WM_CTLCOLOREDIT || message == WM_CTLCOLORLISTBOX)
			{
				SetTextColor ((HDC) wParam, s_text);
				SetBkColor ((HDC) wParam, s_edit);
				return (LRESULT) s_editBrush;
			}
			if (message == WM_CTLCOLORDLG || message == WM_CTLCOLORSTATIC || message == WM_CTLCOLORBTN || message == WM_CTLCOLORSCROLLBAR)
			{
				SetTextColor ((HDC) wParam, s_text);
				SetBkColor ((HDC) wParam, s_bg);
				return (LRESULT) s_bgBrush;
			}
			if (message == WM_ERASEBKGND && hwnd != ViewWindow())
			{
				char name[128];
				GetClassNameA (hwnd, name, sizeof (name));
				RECT client;
				GetClientRect (hwnd, &client);
				const bool shortBar = strncmp (name, "Afx", 3) == 0 && client.bottom - client.top <= 120;
				if (shortBar || strcmp (name, "#32770") == 0 || strncmp (name, "Afx:", 4) == 0 || strcmp (name, "ToolbarWindow32") == 0 || strcmp (name, "ReBarWindow32") == 0)
				{
					RECT rect;
					GetClientRect (hwnd, &rect);
					FillRect ((HDC) wParam, &rect, s_bgBrush);
					return 1;
				}
			}
		}
		return CallWindowProc (oldProc, hwnd, message, wParam, lParam);
	}

	LRESULT CALLBACK ThemeHook (int code, WPARAM wParam, LPARAM lParam)
	{
		if (code == HC_ACTION && !s_applying)
		{
			CWPRETSTRUCT * message = (CWPRETSTRUCT *) lParam;
			if (message->message == WM_INITDIALOG && !IsShellUi (message->hwnd))
				ApplyWindowTree (message->hwnd);
		}
		return CallNextHookEx (s_hook, code, wParam, lParam);
	}

	void EnsureHook()
	{
		if (s_hook)
			return;
		s_hook = SetWindowsHookEx (WH_CALLWNDPROCRET, ThemeHook, NULL, GetCurrentThreadId());
	}

	bool s_ignorePick = false;

	class ThemeColorObserver : public CGTColorDialogDlg::IObserver
	{
		public:
			virtual void SendColor (COLORREF & color)
			{
			if (s_ignorePick)
				return;
			if (s_pickText)
					s_customText = color;
				else
					s_customBg = color;
				s_color = ID_INTERFACE_COLOR_CUSTOM;
				ApplyNow();
				SaveSettings();
			}
	};

	ThemeColorObserver s_observer;

	void ShowCustomPicker (bool text)
	{
		LoadSettings();
		CWnd * main = AfxGetMainWnd();
		if (!main)
			return;
		if (s_color != ID_INTERFACE_COLOR_CUSTOM)
		{
			const ThemePalette * current = Palette();
			if (current)
			{
				s_customBg = current->bg;
				s_customText = current->text;
			}
		}
		s_pickText = text;
		s_color = ID_INTERFACE_COLOR_CUSTOM;
		if (!s_pickerDlg || !::IsWindow (s_pickerDlg->GetSafeHwnd()))
		{
			if (s_pickerDlg)
			{
				delete s_pickerDlg;
				s_pickerDlg = NULL;
				s_picker = NULL;
			}
			s_pickerDlg = new CGTColorDialogDlg (main);
			s_pickerDlg->SetObserver (&s_observer);
			s_ignorePick = true;
			s_pickerDlg->Create (IDD_GTCOLORDIALOG_DIALOG, main);
			s_ignorePick = false;
			s_picker = s_pickerDlg->GetSafeHwnd();
		}
		s_pickerDlg->SetWindowText (text ? "Text" : "Interface");
		s_ignorePick = true;
		s_pickerDlg->SetColor (text ? s_customText : s_customBg);
		CRect frame;
		main->GetWindowRect (&frame);
		s_pickerDlg->SetWindowPos (&CWnd::wndTop, frame.left + 80, frame.top + 80, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
		s_pickerDlg->ShowWindow (SW_SHOW);
		s_pickerDlg->SetColor (text ? s_customText : s_customBg);
		s_ignorePick = false;
		ApplyNow();
		SaveSettings();
	}
}

void InterfaceThemeApplyStartup()
{
	LoadSettings();
	EnsureHook();
	ApplyNow();
}

void InterfaceThemeSetColor (UINT nId)
{
	LoadSettings();
	s_color = nId;
	if (nId == ID_INTERFACE_COLOR_LIGHT)
	{
		s_font = ID_INTERFACE_FONT_DEFAULT;
		s_size = ID_INTERFACE_SIZE_DEFAULT;
	}
	SaveSettings();
	EnsureHook();
	ApplyNow();
}

void InterfaceThemePickCustom (bool text)
{
	LoadSettings();
	EnsureHook();
	ShowCustomPicker (text);
}

void InterfaceThemeSetFont (UINT nId)
{
	LoadSettings();
	s_font = nId;
	SaveSettings();
	EnsureHook();
	ApplyNow();
}

void InterfaceThemeSetSize (UINT nId)
{
	LoadSettings();
	s_size = nId;
	SaveSettings();
	EnsureHook();
	ApplyNow();
}

UINT InterfaceThemeColor()
{
	LoadSettings();
	return s_color;
}

UINT InterfaceThemeFont()
{
	LoadSettings();
	return s_font;
}

UINT InterfaceThemeSize()
{
	LoadSettings();
	return s_size;
}
