#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <algorithm>
#include <string>
#include <cstdlib>

#include "resource.h"

using namespace std;

// CONTROL IDs
#define ID_LBL_TEXT 1000
#define ID_UI_TEXT 1001

#define ID_LBL_TEXT_CHECK 1002
#define ID_UI_TEXT_CHECK 1003

#define ID_LBL_FONT 1004
#define ID_UI_FONT 1005

#define ID_LBL_FONT_SIZE 1006
#define ID_UI_FONT_SIZE 1007

#define ID_LBL_ELLIPSE_WAIT 1008
#define ID_UI_ELLIPSE_WAIT 1009

#define ID_LBL_MAX_ELLIPSES 1010
#define ID_UI_MAX_ELLIPSES 1011

#define ID_LBL_ELLIPSE_MIN_THICKNESS 1012
#define ID_UI_ELLIPSE_MIN_THICKNESS 1013

#define ID_LBL_ELLIPSE_MAX_THICKNESS 1014
#define ID_UI_ELLIPSE_MAX_THICKNESS 1015

#define ID_LBL_ELLIPSE_MIN_RADIUS_STEP 1016
#define ID_UI_ELLIPSE_MIN_RADIUS_STEP 1017

#define ID_LBL_ELLIPSE_MAX_RADIUS_STEP 1018
#define ID_UI_ELLIPSE_MAX_RADIUS_STEP 1019

#define ID_LBL_ELLIPSE_MIN_ALPHA_STEP 1020
#define ID_UI_ELLIPSE_MIN_ALPHA_STEP 1021

#define ID_LBL_ELLIPSE_MAX_ALPHA_STEP 1022
#define ID_UI_ELLIPSE_MAX_ALPHA_STEP 1023

#define ID_BTN_SAVE 1099
#define ID_BTN_EXIT 1100
#define ID_BTN_RESET 1101

#define ID_TXT_COPYRIGHT 1102
#define ID_TXT_LINK 1103

// DEFAULTS
#define DEFAULT_TEXT_TO_DISPLAY L""
#define DEFAULT_TEXT_CHECK BST_CHECKED
#define DEFAULT_FONT_NAMEW L"Arial"
#define DEFAULT_FONT_SIZE 54
#define DEFAULT_ELLIPSE_WAIT 9
#define DEFAULT_MAX_ELLIPSES 7
#define DEFAULT_MIN_THICKNESS 1
#define DEFAULT_MAX_THICKNESS 100
#define DEFAULT_MIN_RADIUS_STEP 1
#define DEFAULT_MAX_RADIUS_STEP 2
#define DEFAULT_MIN_ALPHA_STEP 1
#define DEFAULT_MAX_ALPHA_STEP 10

// Registry PATHS
static const wchar_t REG_PATH_NEW[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Screensavers\\Drops";
static const wchar_t REG_PATH_OLD[] = L"Software\\Microsoft\\Screensavers\\Drops";

// SCROLL
HWND hwndContainer;
HWND g_hwndMain = NULL;   // main (top-level) config window
static int g_scrollY = 0;
int g_contentHeight;

// Cache each child's original (unscrolled) client rect so we never "stack" offsets
struct ChildPos
{
	HWND h;
	RECT base;
};
std::vector<ChildPos> g_childrenBase;

int g_scrollPos = 0;
int g_scrollMax = 0;
const int g_pageSize = 20;

// Globals
wchar_t g_TextToDisplay[33] = L"";
int g_TextCheck = DEFAULT_TEXT_CHECK;
wchar_t g_FontName[LF_FACESIZE] = DEFAULT_FONT_NAMEW;

int g_FontSize = DEFAULT_FONT_SIZE;

int g_ELLIPSE_WAIT = DEFAULT_ELLIPSE_WAIT;
int g_MAX_ELLIPSES = DEFAULT_MAX_ELLIPSES;

int g_ELLIPSE_MIN_THICKNESS = DEFAULT_MIN_THICKNESS;
int g_ELLIPSE_MAX_THICKNESS = DEFAULT_MAX_THICKNESS;

int g_ELLIPSE_MIN_RADIUS_STEP = DEFAULT_MIN_RADIUS_STEP;
int g_ELLIPSE_MAX_RADIUS_STEP = DEFAULT_MAX_RADIUS_STEP;

int g_ELLIPSE_MIN_ALPHA_STEP = DEFAULT_MIN_ALPHA_STEP;
int g_ELLIPSE_MAX_ALPHA_STEP = DEFAULT_MAX_ALPHA_STEP;

vector<wstring> g_fonts;

struct X_MonitorInfoStruct
{
	RECT monitorRect, workRect;
	bool isPrimary;
};
vector<X_MonitorInfoStruct> X_monitors;

HWND textToDisplay;
HWND textCheckBox;

BOOL CALLBACK X_MonitorEnumProc(HMONITOR hMon, HDC, LPRECT, LPARAM)
{
	MONITORINFO mi;
	mi.cbSize = sizeof(mi);

	if (GetMonitorInfo(hMon, &mi))
	{
		X_MonitorInfoStruct s;
		s.monitorRect = mi.rcMonitor;
		s.workRect = mi.rcWork;
		s.isPrimary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
		X_monitors.push_back(s);
	}

	return TRUE;
}
void X_SetMonitors()
{
	X_monitors.clear();
	EnumDisplayMonitors(NULL, NULL, X_MonitorEnumProc, 0);
}

static int ClampOrDefault(int value, int minV, int maxV, int defV)
{
	if (value < minV)
		return defV;
	if (value > maxV)
		return defV;
	return value;
}

// REGISTRY
static HKEY OpenSettingsKey(BOOL create)
{
	HKEY hKey = NULL;
	LONG res;

	// 1) Check new path
	res = RegOpenKeyExW(HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Windows\\CurrentVersion\\Screensavers",
		0, KEY_READ | KEY_WRITE, &hKey);
	if (res == ERROR_SUCCESS)
	{
		// Append \Drops subkey if requested
		if (create || hKey)
		{
			HKEY hDrops = NULL;
			res = RegCreateKeyExW(hKey, L"Drops", 0, NULL,
				REG_OPTION_NON_VOLATILE,
				KEY_READ | KEY_WRITE, NULL,
				&hDrops, NULL);
			RegCloseKey(hKey);
			if (res == ERROR_SUCCESS)
				return hDrops;
			return NULL;
		}
		return hKey;
	}

	// 2) Check old path
	res = RegOpenKeyExW(HKEY_CURRENT_USER,
		L"Software\\Microsoft\\Screensavers",
		0, KEY_READ | KEY_WRITE, &hKey);
	if (res == ERROR_SUCCESS)
	{
		// Append \Drops subkey if requested
		if (create || hKey)
		{
			HKEY hDrops = NULL;
			res = RegCreateKeyExW(hKey, L"Drops", 0, NULL,
				REG_OPTION_NON_VOLATILE,
				KEY_READ | KEY_WRITE, NULL,
				&hDrops, NULL);
			RegCloseKey(hKey);
			if (res == ERROR_SUCCESS)
				return hDrops;
			return NULL;
		}
		return hKey;
	}

	// 3) If neither exists, create old path + Drops
	if (create)
	{
		HKEY hScreensavers = NULL;
		res = RegCreateKeyExW(HKEY_CURRENT_USER,
			L"Software\\Microsoft\\Screensavers",
			0, NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_READ | KEY_WRITE, NULL,
			&hScreensavers, NULL);
		if (res != ERROR_SUCCESS)
			return NULL;

		HKEY hDrops = NULL;
		res = RegCreateKeyExW(hScreensavers, L"Drops", 0, NULL,
			REG_OPTION_NON_VOLATILE,
			KEY_READ | KEY_WRITE, NULL,
			&hDrops, NULL);
		RegCloseKey(hScreensavers);
		if (res == ERROR_SUCCESS)
			return hDrops;
		return NULL;
	}

	return NULL;
}

// LOAD
void LoadSettings()
{
	HKEY hKey = OpenSettingsKey(false);
	if (!hKey)
	{
		// no registry values -> keep defaults
		g_TextToDisplay[0] = L'\0';
		g_TextCheck = DEFAULT_TEXT_CHECK;
		wcscpy_s(g_FontName, LF_FACESIZE, DEFAULT_FONT_NAMEW);
		g_FontSize = DEFAULT_FONT_SIZE;
		g_ELLIPSE_WAIT = DEFAULT_ELLIPSE_WAIT;
		g_MAX_ELLIPSES = DEFAULT_MAX_ELLIPSES;
		g_ELLIPSE_MIN_THICKNESS = DEFAULT_MIN_THICKNESS;
		g_ELLIPSE_MAX_THICKNESS = DEFAULT_MAX_THICKNESS;
		g_ELLIPSE_MIN_RADIUS_STEP = DEFAULT_MIN_RADIUS_STEP;
		g_ELLIPSE_MAX_RADIUS_STEP = DEFAULT_MAX_RADIUS_STEP;
		g_ELLIPSE_MIN_ALPHA_STEP = DEFAULT_MIN_ALPHA_STEP;
		g_ELLIPSE_MAX_ALPHA_STEP = DEFAULT_MAX_ALPHA_STEP;
		return;
	}

	DWORD type = 0;
	DWORD size = 0;
	wchar_t tmp[260];

	// TextToDisplay
	size = sizeof(tmp);
	if (RegQueryValueExW(hKey, L"TextToDisplay", NULL, &type, (LPBYTE)tmp, &size) == ERROR_SUCCESS && type == REG_SZ)
	{
		// size is bytes including terminator
		tmp[(size / sizeof(wchar_t) - 1) < 0 ? 0 : (size / sizeof(wchar_t) - 1)] = L'\0';
		// copy up to 32 chars
		wcsncpy_s(g_TextToDisplay, tmp, 32);
		g_TextToDisplay[32] = L'\0';
	}
	else
	{
		g_TextToDisplay[0] = L'\0';
	}

	// FontName
	size = sizeof(tmp);
	if (RegQueryValueExW(hKey, L"FontName", NULL, &type, (LPBYTE)tmp, &size) == ERROR_SUCCESS && type == REG_SZ)
	{
		tmp[(size / sizeof(wchar_t) - 1) < 0 ? 0 : (size / sizeof(wchar_t) - 1)] = L'\0';
		wcsncpy_s(g_FontName, tmp, LF_FACESIZE - 1);
		g_FontName[LF_FACESIZE - 1] = L'\0';

		bool found = false;
		for (size_t i = 0; i < g_fonts.size(); ++i)
		{
			if (wcscmp(g_fonts[i].c_str(), g_FontName) == 0)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			wcscpy_s(g_FontName, LF_FACESIZE, DEFAULT_FONT_NAMEW);
		}
	}
	else
	{
		wcscpy_s(g_FontName, LF_FACESIZE, DEFAULT_FONT_NAMEW);
	}

	// DWORDs
	DWORD val = 0;
	size = sizeof(val);

	if (RegQueryValueExW(hKey, L"TextCheck", NULL, &type, (LPBYTE)&val, &size) == ERROR_SUCCESS && type == REG_DWORD)
	{
		if (val == BST_CHECKED || val == BST_UNCHECKED)
		{
			g_TextCheck = val;
		}
		else
		{
			g_TextCheck = DEFAULT_TEXT_CHECK;
		}
	}
	else
		g_TextCheck = DEFAULT_TEXT_CHECK;

	if (RegQueryValueExW(hKey, L"FontSize", NULL, &type, (LPBYTE)&val, &size) == ERROR_SUCCESS && type == REG_DWORD)
	{
		g_FontSize = ClampOrDefault((int)val, 1, 255, DEFAULT_FONT_SIZE);
	}
	else
		g_FontSize = DEFAULT_FONT_SIZE;

	if (RegQueryValueExW(hKey, L"EllipseWait", NULL, &type, (LPBYTE)&val, &size) == ERROR_SUCCESS && type == REG_DWORD)
	{
		g_ELLIPSE_WAIT = ClampOrDefault((int)val, 1, 255, DEFAULT_ELLIPSE_WAIT);
	}
	else
		g_ELLIPSE_WAIT = DEFAULT_ELLIPSE_WAIT;

	if (RegQueryValueExW(hKey, L"MaxEllipses", NULL, &type, (LPBYTE)&val, &size) == ERROR_SUCCESS && type == REG_DWORD)
	{
		g_MAX_ELLIPSES = ClampOrDefault((int)val, 1, 255, DEFAULT_MAX_ELLIPSES);
	}
	else
		g_MAX_ELLIPSES = DEFAULT_MAX_ELLIPSES;

	if (RegQueryValueExW(hKey, L"EllipseMinThickness", NULL, &type, (LPBYTE)&val, &size) == ERROR_SUCCESS && type == REG_DWORD)
	{
		g_ELLIPSE_MIN_THICKNESS = ClampOrDefault((int)val, 1, 255, DEFAULT_MIN_THICKNESS);
	}
	else
		g_ELLIPSE_MIN_THICKNESS = DEFAULT_MIN_THICKNESS;

	if (RegQueryValueExW(hKey, L"EllipseMaxThickness", NULL, &type, (LPBYTE)&val, &size) == ERROR_SUCCESS && type == REG_DWORD)
	{
		g_ELLIPSE_MAX_THICKNESS = ClampOrDefault((int)val, 1, 255, DEFAULT_MAX_THICKNESS);
	}
	else
		g_ELLIPSE_MAX_THICKNESS = DEFAULT_MAX_THICKNESS;

	if (RegQueryValueExW(hKey, L"EllipseMinRadiusStep", NULL, &type, (LPBYTE)&val, &size) == ERROR_SUCCESS && type == REG_DWORD)
	{
		g_ELLIPSE_MIN_RADIUS_STEP = ClampOrDefault((int)val, 1, 255, DEFAULT_MIN_RADIUS_STEP);
	}
	else
		g_ELLIPSE_MIN_RADIUS_STEP = DEFAULT_MIN_RADIUS_STEP;

	if (RegQueryValueExW(hKey, L"EllipseMaxRadiusStep", NULL, &type, (LPBYTE)&val, &size) == ERROR_SUCCESS && type == REG_DWORD)
	{
		g_ELLIPSE_MAX_RADIUS_STEP = ClampOrDefault((int)val, 1, 255, DEFAULT_MAX_RADIUS_STEP);
	}
	else
		g_ELLIPSE_MAX_RADIUS_STEP = DEFAULT_MAX_RADIUS_STEP;

	if (RegQueryValueExW(hKey, L"EllipseMinAlphaStep", NULL, &type, (LPBYTE)&val, &size) == ERROR_SUCCESS && type == REG_DWORD)
	{
		g_ELLIPSE_MIN_ALPHA_STEP = ClampOrDefault((int)val, 1, 255, DEFAULT_MIN_ALPHA_STEP);
	}
	else
		g_ELLIPSE_MIN_ALPHA_STEP = DEFAULT_MIN_ALPHA_STEP;

	if (RegQueryValueExW(hKey, L"EllipseMaxAlphaStep", NULL, &type, (LPBYTE)&val, &size) == ERROR_SUCCESS && type == REG_DWORD)
	{
		g_ELLIPSE_MAX_ALPHA_STEP = ClampOrDefault((int)val, 1, 255, DEFAULT_MAX_ALPHA_STEP);
	}
	else
		g_ELLIPSE_MAX_ALPHA_STEP = DEFAULT_MAX_ALPHA_STEP;

	if (g_ELLIPSE_MIN_THICKNESS > g_ELLIPSE_MAX_THICKNESS)
	{
		g_ELLIPSE_MAX_THICKNESS = g_ELLIPSE_MIN_THICKNESS;
	}
	if (g_ELLIPSE_MIN_RADIUS_STEP > g_ELLIPSE_MAX_RADIUS_STEP)
	{
		g_ELLIPSE_MAX_RADIUS_STEP = g_ELLIPSE_MIN_RADIUS_STEP;
	}
	if (g_ELLIPSE_MIN_ALPHA_STEP > g_ELLIPSE_MAX_ALPHA_STEP)
	{
		g_ELLIPSE_MAX_ALPHA_STEP = g_ELLIPSE_MIN_ALPHA_STEP;
	}

	RegCloseKey(hKey);
}

void SaveSettings()
{
	HKEY hKey = OpenSettingsKey(true);
	if (!hKey)
		return;

	// CLAMP before saving

	if (g_TextCheck != BST_CHECKED && g_TextCheck != BST_UNCHECKED)
	{
		g_TextCheck = DEFAULT_TEXT_CHECK;
	}

	g_FontSize = ClampOrDefault(g_FontSize, 1, 255, DEFAULT_FONT_SIZE);
	g_ELLIPSE_WAIT = ClampOrDefault(g_ELLIPSE_WAIT, 1, 255, DEFAULT_ELLIPSE_WAIT);
	g_MAX_ELLIPSES = ClampOrDefault(g_MAX_ELLIPSES, 1, 255, DEFAULT_MAX_ELLIPSES);
	g_ELLIPSE_MIN_THICKNESS = ClampOrDefault(g_ELLIPSE_MIN_THICKNESS, 1, 255, DEFAULT_MIN_THICKNESS);
	g_ELLIPSE_MAX_THICKNESS = ClampOrDefault(g_ELLIPSE_MAX_THICKNESS, 1, 255, DEFAULT_MAX_THICKNESS);
	g_ELLIPSE_MIN_RADIUS_STEP = ClampOrDefault(g_ELLIPSE_MIN_RADIUS_STEP, 1, 255, DEFAULT_MIN_RADIUS_STEP);
	g_ELLIPSE_MAX_RADIUS_STEP = ClampOrDefault(g_ELLIPSE_MAX_RADIUS_STEP, 1, 255, DEFAULT_MAX_RADIUS_STEP);
	g_ELLIPSE_MIN_ALPHA_STEP = ClampOrDefault(g_ELLIPSE_MIN_ALPHA_STEP, 1, 255, DEFAULT_MIN_ALPHA_STEP);
	g_ELLIPSE_MAX_ALPHA_STEP = ClampOrDefault(g_ELLIPSE_MAX_ALPHA_STEP, 1, 255, DEFAULT_MAX_ALPHA_STEP);

	if (g_ELLIPSE_MIN_THICKNESS > g_ELLIPSE_MAX_THICKNESS)
	{
		g_ELLIPSE_MAX_THICKNESS = g_ELLIPSE_MIN_THICKNESS;
	}
	if (g_ELLIPSE_MIN_RADIUS_STEP > g_ELLIPSE_MAX_RADIUS_STEP)
	{
		g_ELLIPSE_MAX_RADIUS_STEP = g_ELLIPSE_MIN_RADIUS_STEP;
	}
	if (g_ELLIPSE_MIN_ALPHA_STEP > g_ELLIPSE_MAX_ALPHA_STEP)
	{
		g_ELLIPSE_MAX_ALPHA_STEP = g_ELLIPSE_MIN_ALPHA_STEP;
	}

	RegSetValueExW(hKey, L"TextToDisplay", 0, REG_SZ, (const BYTE *)g_TextToDisplay, (DWORD)((wcslen(g_TextToDisplay) + 1) * sizeof(wchar_t)));
	RegSetValueExW(hKey, L"FontName", 0, REG_SZ, (const BYTE *)g_FontName, (DWORD)((wcslen(g_FontName) + 1) * sizeof(wchar_t)));
	{
		DWORD v;
		v = (DWORD)g_TextCheck;
		RegSetValueExW(hKey, L"TextCheck", 0, REG_DWORD, (const BYTE *)&v, sizeof(v));
		v = (DWORD)g_FontSize;
		RegSetValueExW(hKey, L"FontSize", 0, REG_DWORD, (const BYTE *)&v, sizeof(v));
		v = (DWORD)g_ELLIPSE_WAIT;
		RegSetValueExW(hKey, L"EllipseWait", 0, REG_DWORD, (const BYTE *)&v, sizeof(v));
		v = (DWORD)g_MAX_ELLIPSES;
		RegSetValueExW(hKey, L"MaxEllipses", 0, REG_DWORD, (const BYTE *)&v, sizeof(v));
		v = (DWORD)g_ELLIPSE_MIN_THICKNESS;
		RegSetValueExW(hKey, L"EllipseMinThickness", 0, REG_DWORD, (const BYTE *)&v, sizeof(v));
		v = (DWORD)g_ELLIPSE_MAX_THICKNESS;
		RegSetValueExW(hKey, L"EllipseMaxThickness", 0, REG_DWORD, (const BYTE *)&v, sizeof(v));
		v = (DWORD)g_ELLIPSE_MIN_RADIUS_STEP;
		RegSetValueExW(hKey, L"EllipseMinRadiusStep", 0, REG_DWORD, (const BYTE *)&v, sizeof(v));
		v = (DWORD)g_ELLIPSE_MAX_RADIUS_STEP;
		RegSetValueExW(hKey, L"EllipseMaxRadiusStep", 0, REG_DWORD, (const BYTE *)&v, sizeof(v));
		v = (DWORD)g_ELLIPSE_MIN_ALPHA_STEP;
		RegSetValueExW(hKey, L"EllipseMinAlphaStep", 0, REG_DWORD, (const BYTE *)&v, sizeof(v));
		v = (DWORD)g_ELLIPSE_MAX_ALPHA_STEP;
		RegSetValueExW(hKey, L"EllipseMaxAlphaStep", 0, REG_DWORD, (const BYTE *)&v, sizeof(v));
	}

	RegCloseKey(hKey);
}

static int CALLBACK EnumFontFamExProc(const LOGFONT *plf, const TEXTMETRIC * /*ptm*/, DWORD /*fontType*/, LPARAM lParam)
{
	vector<wstring> *fonts = reinterpret_cast<vector<wstring> *>(lParam);

	// Filter duplicates
	wstring name = plf->lfFaceName;

	for (size_t i = 0; i < fonts->size(); ++i)
	{
		if (_wcsicmp((*fonts)[i].c_str(), name.c_str()) == 0)
		{
			return 1; // already exists, skip
		}
	}

	fonts->push_back(name);

	return 1;
}

void SetFontNames()
{
	g_fonts.clear();

	LOGFONTW lf;
	ZeroMemory(&lf, sizeof(lf));
	lf.lfCharSet = DEFAULT_CHARSET;

	HDC hdc = GetDC(NULL);
	EnumFontFamiliesExW(hdc, &lf, EnumFontFamExProc, (LPARAM)&g_fonts, 0);

	std::sort(g_fonts.begin(), g_fonts.end());

	ReleaseDC(NULL, hdc);
}

static void SetEditInt(HWND hwndDlg, int id, int value)
{
	wchar_t buf[32];
	_snwprintf_s(buf, sizeof(buf) / sizeof(wchar_t), L"%d", value);
	buf[(sizeof(buf) / sizeof(wchar_t)) - 1] = L'\0';
	SetWindowTextW(GetDlgItem(hwndDlg, id), buf);
}

static int GetEditIntClamped(HWND hwndDlg, int id, int defVal, int minV, int maxV)
{
	wchar_t buf[64];
	buf[0] = L'\0';
	GetWindowTextW(GetDlgItem(hwndDlg, id), buf, (int)(sizeof(buf) / sizeof(wchar_t)));
	int v = _wtoi(buf);
	if (v < minV || v > maxV)
		return defVal;
	return v;
}

void ToggleTextToDisplay()
{
	BOOL enable = (SendMessageW(textCheckBox, BM_GETCHECK, 0, 0) == BST_UNCHECKED);
	EnableWindow(textToDisplay, enable);

	RedrawWindow(hwndContainer, NULL, NULL, 
		RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_FRAME);
}

// Capture each child's base (unscrolled) client rect once, after creation.
void CacheChildBaseRects()
{
	g_childrenBase.clear();

	HWND child = GetWindow(hwndContainer, GW_CHILD);
	while (child)
	{
		RECT rcWin;
		GetWindowRect(child, &rcWin);

		POINT pt;
		pt.x = rcWin.left;
		pt.y = rcWin.top;
		ScreenToClient(hwndContainer, &pt);


		ChildPos cp;
		cp.h = child;

		cp.base.left = pt.x;
		cp.base.top = pt.y;
		cp.base.right = pt.x + (rcWin.right - rcWin.left);
		cp.base.bottom = pt.y + (rcWin.bottom - rcWin.top);

		g_childrenBase.push_back(cp);

		child = GetWindow(child, GW_HWNDNEXT);
	}
}

// Reposition children from their *base* rect minus the scroll offset.
// IMPORTANT: no compounding because we never read current positions.
void RepositionChildren()
{
	for (size_t i = 0; i < g_childrenBase.size(); ++i)
	{
		const ChildPos &c = g_childrenBase[i];

		int x = c.base.left;
		int y = c.base.top - g_scrollY;
		int w = c.base.right - c.base.left;
		int h = c.base.bottom - c.base.top;

		SetWindowPos(c.h, NULL,
			x, 
			y, 
			w, 
			h,
			SWP_NOZORDER | SWP_NOACTIVATE);
	}
}

void ScrollTo(int newY, HWND /*hwnd*/)
{
	// g_hwndMain is the single authoritative scroll-host
	HWND host = g_hwndMain;
	if (!host)
		return;

	RECT rc;
	GetClientRect(host, &rc);
	int clientHeight = rc.bottom;

	if (newY < 0) newY = 0;
	if (g_contentHeight > clientHeight)
	{
		newY = (newY > g_contentHeight - clientHeight) ? g_contentHeight - clientHeight : newY;
	}
	else
	{
		newY = 0;
	}

	if (newY == g_scrollY) return;
	g_scrollY = newY;

	RepositionChildren();

	InvalidateRect(hwndContainer, NULL, TRUE);

	SCROLLINFO si = {0};
	si.cbSize = sizeof(si);
	si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
	si.nMin = 0;
	si.nMax = g_contentHeight - 1;
	si.nPage = clientHeight;
	si.nPos = g_scrollY;
	SetScrollInfo(host, SB_VERT, &si, TRUE);
}


static LRESULT CALLBACK ConfigWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		{
			int id = LOWORD(wParam);

			if (id == ID_UI_TEXT_CHECK && HIWORD(wParam) == BN_CLICKED)
			{
				ToggleTextToDisplay();
			}
			else if (id == ID_BTN_SAVE)
			{
				// TextToDisplay
				wchar_t tmpText[33];
				tmpText[0] = L'\0';
				GetWindowTextW(GetDlgItem(hwndContainer, ID_UI_TEXT), tmpText, 33);
				tmpText[32] = L'\0';
				// copy into global
				wcsncpy_s(g_TextToDisplay, tmpText, 32);
				g_TextToDisplay[32] = L'\0';

				// Text Checkbox
				g_TextCheck = SendMessageW(textCheckBox, BM_GETCHECK, 0, 0);

				// Font selection
				HWND hFontCombo = GetDlgItem(hwndContainer, ID_UI_FONT);
				if (hFontCombo)
				{
					int sel = (int)SendMessageW(hFontCombo, CB_GETCURSEL, 0, 0);
					if (sel != CB_ERR)
					{
						wchar_t fn[LF_FACESIZE];
						SendMessageW(hFontCombo, CB_GETLBTEXT, sel, (LPARAM)fn);
						wcsncpy_s(g_FontName, fn, LF_FACESIZE - 1);
						g_FontName[LF_FACESIZE - 1] = L'\0';
					}
					else
					{
						// if combo has no selection, try text in the combobox edit portion
						wchar_t fn2[LF_FACESIZE];
						GetWindowTextW(hFontCombo, fn2, LF_FACESIZE);
						fn2[LF_FACESIZE - 1] = L'\0';
						if (wcslen(fn2) > 0)
						{
							wcsncpy_s(g_FontName, fn2, LF_FACESIZE - 1);
							g_FontName[LF_FACESIZE - 1] = L'\0';
						}
					}
				}

				// Fields
				g_FontSize = GetEditIntClamped(hwndContainer, ID_UI_FONT_SIZE, DEFAULT_FONT_SIZE, 1, 255);
				g_ELLIPSE_WAIT = GetEditIntClamped(hwndContainer, ID_UI_ELLIPSE_WAIT, DEFAULT_ELLIPSE_WAIT, 1, 255);
				g_MAX_ELLIPSES = GetEditIntClamped(hwndContainer, ID_UI_MAX_ELLIPSES, DEFAULT_MAX_ELLIPSES, 1, 255);

				g_ELLIPSE_MIN_THICKNESS = GetEditIntClamped(hwndContainer, ID_UI_ELLIPSE_MIN_THICKNESS, DEFAULT_MIN_THICKNESS, 1, 255);
				g_ELLIPSE_MAX_THICKNESS = GetEditIntClamped(hwndContainer, ID_UI_ELLIPSE_MAX_THICKNESS, DEFAULT_MAX_THICKNESS, 1, 255);
				g_ELLIPSE_MIN_RADIUS_STEP = GetEditIntClamped(hwndContainer, ID_UI_ELLIPSE_MIN_RADIUS_STEP, DEFAULT_MIN_RADIUS_STEP, 1, 255);
				g_ELLIPSE_MAX_RADIUS_STEP = GetEditIntClamped(hwndContainer, ID_UI_ELLIPSE_MAX_RADIUS_STEP, DEFAULT_MAX_RADIUS_STEP, 1, 255);
				g_ELLIPSE_MIN_ALPHA_STEP = GetEditIntClamped(hwndContainer, ID_UI_ELLIPSE_MIN_ALPHA_STEP, DEFAULT_MIN_ALPHA_STEP, 1, 255);
				g_ELLIPSE_MAX_ALPHA_STEP = GetEditIntClamped(hwndContainer, ID_UI_ELLIPSE_MAX_ALPHA_STEP, DEFAULT_MAX_ALPHA_STEP, 1, 255);

				if (g_ELLIPSE_MIN_THICKNESS > g_ELLIPSE_MAX_THICKNESS)
				{
					g_ELLIPSE_MAX_THICKNESS = g_ELLIPSE_MIN_THICKNESS;
				}
				if (g_ELLIPSE_MIN_RADIUS_STEP > g_ELLIPSE_MAX_RADIUS_STEP)
				{
					g_ELLIPSE_MAX_RADIUS_STEP = g_ELLIPSE_MIN_RADIUS_STEP;
				}
				if (g_ELLIPSE_MIN_ALPHA_STEP > g_ELLIPSE_MAX_ALPHA_STEP)
				{
					g_ELLIPSE_MAX_ALPHA_STEP = g_ELLIPSE_MIN_ALPHA_STEP;
				}

				// Save to registry
				SaveSettings();

				// close window
				DestroyWindow(hwnd);
				PostQuitMessage(0);
				return 0;
			}
			else if (id == ID_BTN_EXIT)
			{
				DestroyWindow(hwnd);
				PostQuitMessage(0);
				return 0;
			}
			else if (id == ID_BTN_RESET)
			{
				SetWindowTextW(GetDlgItem(hwndContainer, ID_UI_TEXT), DEFAULT_TEXT_TO_DISPLAY);

				HWND hFontCombo = GetDlgItem(hwndContainer, ID_UI_FONT);
				int findIdx = (int)SendMessageW(hFontCombo, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)DEFAULT_FONT_NAMEW);
				if (findIdx != CB_ERR)
					SendMessageW(hFontCombo, CB_SETCURSEL, findIdx, 0);

				SendMessageW(textCheckBox, BM_SETCHECK, DEFAULT_TEXT_CHECK, 0);

				SetDlgItemInt(hwndContainer, ID_UI_FONT_SIZE, DEFAULT_FONT_SIZE, FALSE);
				SetDlgItemInt(hwndContainer, ID_UI_ELLIPSE_WAIT, DEFAULT_ELLIPSE_WAIT, FALSE);
				SetDlgItemInt(hwndContainer, ID_UI_MAX_ELLIPSES, DEFAULT_MAX_ELLIPSES, FALSE);
				SetDlgItemInt(hwndContainer, ID_UI_ELLIPSE_MIN_THICKNESS, DEFAULT_MIN_THICKNESS, FALSE);
				SetDlgItemInt(hwndContainer, ID_UI_ELLIPSE_MAX_THICKNESS, DEFAULT_MAX_THICKNESS, FALSE);
				SetDlgItemInt(hwndContainer, ID_UI_ELLIPSE_MIN_RADIUS_STEP, DEFAULT_MIN_RADIUS_STEP, FALSE);
				SetDlgItemInt(hwndContainer, ID_UI_ELLIPSE_MAX_RADIUS_STEP, DEFAULT_MAX_RADIUS_STEP, FALSE);
				SetDlgItemInt(hwndContainer, ID_UI_ELLIPSE_MIN_ALPHA_STEP, DEFAULT_MIN_ALPHA_STEP, FALSE);
				SetDlgItemInt(hwndContainer, ID_UI_ELLIPSE_MAX_ALPHA_STEP, DEFAULT_MAX_ALPHA_STEP, FALSE);

				ToggleTextToDisplay();

				return 0;
			}
			break;
		}

	case WM_KEYDOWN:
		if (GetKeyState(VK_CONTROL) & 0x8000)
		{
			switch (wParam)
			{
			case 'S':
				SendMessage(hwnd, WM_COMMAND, ID_BTN_SAVE, 0);
				break;
			case 'R':
				SendMessage(hwnd, WM_COMMAND, ID_BTN_RESET, 0);
				break;
			case 'X':
				SendMessage(hwnd, WM_COMMAND, ID_BTN_EXIT, 0);
				break;
			}
		}
		break;

	case WM_SYSKEYDOWN:
		switch (wParam)
		{
		case 'S':
			SendMessage(hwnd, WM_COMMAND, ID_BTN_SAVE, 0);
			break;
		case 'R':
			SendMessage(hwnd, WM_COMMAND, ID_BTN_RESET, 0);
			break;
		case 'X':
			SendMessage(hwnd, WM_COMMAND, ID_BTN_EXIT, 0);
			break;
		}
		break;

	case WM_VSCROLL:
		{

			int pos = g_scrollY;

			switch (LOWORD(wParam))
			{
			case SB_LINEUP: pos -= 20; break;
			case SB_LINEDOWN: pos += 20; break;
			case SB_PAGEUP: pos -= 50; break;
			case SB_PAGEDOWN: pos += 50; break;
			case SB_THUMBPOSITION:
			case SB_THUMBTRACK: 
				{
					SCROLLINFO si = {0};
					si.cbSize = sizeof(si);
					si.fMask = SIF_TRACKPOS;

					// read track pos from the authoritative scroll-host (g_hwndMain) if available
					HWND host = g_hwndMain ? g_hwndMain : hwnd;
					GetScrollInfo(host, SB_VERT, &si);

					pos = si.nTrackPos; 
					break;
				}
			}

			ScrollTo(pos, hwnd);
			return 0;
		}

	case WM_MOUSEWHEEL:
		{
			int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
			int pos = g_scrollY - (zDelta / WHEEL_DELTA) * 60;
			ScrollTo(pos, hwnd);
			return 0;
		}
	case WM_ERASEBKGND:
		{
			HDC hdc = (HDC)wParam;

			RECT rc;

			GetClientRect(hwndContainer, &rc);
			FillRect(hdc, &rc, (HBRUSH)(COLOR_BTNFACE+1));			

			GetClientRect(hwnd, &rc);
			FillRect(hdc, &rc, (HBRUSH)(COLOR_BTNFACE+1));			

			return 1;
		}


	case WM_SIZE:
		{
			// Ensure scrollbar mirrors the current client size & content height
			RECT rc;
			HWND host = g_hwndMain ? g_hwndMain : hwnd;
			GetClientRect(host, &rc);
			SCROLLINFO si = {0};
			si.cbSize = sizeof(si);
			si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
			si.nMin = 0;
			si.nMax = max(0, g_contentHeight - 1);
			si.nPage = rc.bottom;
			si.nPos = min(g_scrollY, max(0, g_contentHeight - rc.bottom));
			SetScrollInfo(host, SB_VERT, &si, TRUE);

			// Reposition children for the (potentially) new page height
			RepositionChildren();
			InvalidateRect(hwndContainer, NULL, TRUE);
			return 0;
		}

	case WM_CLOSE:
		DestroyWindow(hwndContainer);
		DestroyWindow(g_hwndMain);
		DestroyWindow(hwnd);
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void ShowConfigureDialog(HINSTANCE hInstance)
{

	SetFontNames();

	// load current values
	LoadSettings();

	// registry values can be edited by the user and become invalid, LoadSettings() fixes the values
	// Save here to apply those fixes in the registry
	SaveSettings();

	X_SetMonitors();

	// register class
	WNDCLASSW wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.lpfnWndProc = ConfigWndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"DropsSettings_ConfigClass";
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DROPS));
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

	RegisterClassW(&wc);

	// create window
	HWND hwnd = CreateWindowExW(
		WS_EX_DLGMODALFRAME | WS_EX_CONTROLPARENT,
		wc.lpszClassName,
		L"Drops Settings",
		WS_OVERLAPPEDWINDOW |  WS_VISIBLE | WS_VSCROLL | WS_CLIPCHILDREN,
		0, 0, 0, 0,
		NULL, NULL, hInstance, NULL);

	g_hwndMain = hwnd;


	HMONITOR hMon = MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);

	MONITORINFO mi;
	mi.cbSize = sizeof(mi);

	const int leftLabelX = 0;
	const int labelW = 200;

	const int leftControlX = 200;
	const int controlW = 300;

	const int rowH = 50;
	int y = 0;

	int windowWidth;
	int windowHeight;
	int windowPositionX;
	int windowPositionY;
	int scrollBarW;
	if (GetMonitorInfo(hMon, &mi))
	{
		// X, Y, W, H
		windowWidth = mi.rcMonitor.right - mi.rcMonitor.left;
		windowHeight = mi.rcMonitor.bottom - mi.rcMonitor.top;
		windowPositionX = mi.rcMonitor.left;
		windowPositionY = mi.rcMonitor.top;

		scrollBarW = GetSystemMetrics(SM_CXVSCROLL);
		SetWindowPos(hwnd, HWND_TOP,
			windowPositionX, windowPositionY,
			labelW + controlW + scrollBarW, windowHeight / 2,
			SWP_NOZORDER | SWP_SHOWWINDOW);
	}

	if (!hwnd)
		return;

	hwndContainer = CreateWindowExW(
		WS_EX_CONTROLPARENT, wc.lpszClassName, NULL,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
		0, 0,
		labelW + controlW + scrollBarW, windowHeight,
		hwnd, NULL, hInstance, NULL);

	SetWindowLong(hwndContainer, GWL_EXSTYLE, GetWindowLong(hwndContainer, GWL_EXSTYLE) | WS_EX_COMPOSITED);
	CacheChildBaseRects();

	// Text to display
	CreateWindowW(L"STATIC", L"Text to display:", WS_CHILD | WS_VISIBLE | SS_LEFT,
		leftLabelX, y, labelW, rowH, hwndContainer, (HMENU)ID_LBL_TEXT, hInstance, NULL);
	textToDisplay = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", g_TextToDisplay,
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP,
		leftControlX, y, controlW, rowH, hwndContainer, (HMENU)ID_UI_TEXT, hInstance, NULL);
	SendMessageW(textToDisplay, EM_SETLIMITTEXT, (WPARAM)32, 0);
	y += rowH;

	// Text checkbox
	CreateWindowW(L"STATIC", L"Use built-in texts:", WS_CHILD | WS_VISIBLE | SS_LEFT,
		leftLabelX, y, labelW, rowH, hwndContainer, (HMENU)ID_LBL_TEXT_CHECK, hInstance, NULL);
	textCheckBox = CreateWindowExW(0, L"BUTTON", L"",
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
		leftControlX, y, controlW, rowH, hwndContainer, (HMENU)ID_UI_TEXT_CHECK, hInstance, NULL);
	SendMessageW(textCheckBox, BM_SETCHECK, g_TextCheck, 0);
	y += rowH;

	ToggleTextToDisplay();

	// Font combo
	CreateWindowW(L"STATIC", L"Font:", WS_CHILD | WS_VISIBLE | SS_LEFT,
		leftLabelX, y, labelW, rowH, hwndContainer, (HMENU)ID_LBL_FONT, hInstance, NULL);
	HWND hFontCombo = CreateWindowW(L"COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP,
		leftControlX, y, controlW, 400, hwndContainer, (HMENU)ID_UI_FONT, hInstance, NULL);

	// POPULATE
	for (size_t i = 0; i < g_fonts.size(); ++i)
	{
		SendMessageW(hFontCombo, CB_ADDSTRING, 0, (LPARAM)g_fonts[i].c_str());
	}

	// select existing
	int findIdx = (int)SendMessageW(hFontCombo, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)g_FontName);
	if (findIdx != CB_ERR)
		SendMessageW(hFontCombo, CB_SETCURSEL, findIdx, 0);
	y += rowH;

	// Font size
	CreateWindowW(L"STATIC", L"Font size:", WS_CHILD | WS_VISIBLE | SS_LEFT,
		leftLabelX, y, labelW, rowH, hwndContainer, (HMENU)ID_LBL_FONT_SIZE, hInstance, NULL);
	CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL,
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER | WS_TABSTOP,
		leftControlX, y, controlW, rowH, hwndContainer, (HMENU)ID_UI_FONT_SIZE, hInstance, NULL);
	SetEditInt(hwndContainer, ID_UI_FONT_SIZE, g_FontSize);
	y += rowH;

	// Ellipse wait
	CreateWindowW(L"STATIC", L"Ellipse wait:", WS_CHILD | WS_VISIBLE | SS_LEFT,
		leftLabelX, y, labelW, rowH, hwndContainer, (HMENU)ID_LBL_ELLIPSE_WAIT, hInstance, NULL);
	CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL,
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER | WS_TABSTOP,
		leftControlX, y, controlW, rowH, hwndContainer, (HMENU)ID_UI_ELLIPSE_WAIT, hInstance, NULL);
	SetEditInt(hwndContainer, ID_UI_ELLIPSE_WAIT, g_ELLIPSE_WAIT);
	y += rowH;

	// Max ellipses
	CreateWindowW(L"STATIC", L"Max ellipses:", WS_CHILD | WS_VISIBLE | SS_LEFT,
		leftLabelX, y, labelW, rowH, hwndContainer, (HMENU)ID_LBL_MAX_ELLIPSES, hInstance, NULL);
	CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL,
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER | WS_TABSTOP,
		leftControlX, y, controlW, rowH, hwndContainer, (HMENU)ID_UI_MAX_ELLIPSES, hInstance, NULL);
	SetEditInt(hwndContainer, ID_UI_MAX_ELLIPSES, g_MAX_ELLIPSES);
	y += rowH;

	// Ellipse min thickness
	CreateWindowW(L"STATIC", L"Ellipse min thickness:", WS_CHILD | WS_VISIBLE | SS_LEFT,
		leftLabelX, y, labelW, rowH, hwndContainer, (HMENU)ID_LBL_ELLIPSE_MIN_THICKNESS, hInstance, NULL);
	CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL,
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER | WS_TABSTOP,
		leftControlX, y, controlW, rowH, hwndContainer, (HMENU)ID_UI_ELLIPSE_MIN_THICKNESS, hInstance, NULL);
	SetEditInt(hwndContainer, ID_UI_ELLIPSE_MIN_THICKNESS, g_ELLIPSE_MIN_THICKNESS);
	y += rowH;

	// Ellipse max thickness
	CreateWindowW(L"STATIC", L"Ellipse max thickness:", WS_CHILD | WS_VISIBLE | SS_LEFT,
		leftLabelX, y, labelW, rowH, hwndContainer, (HMENU)ID_LBL_ELLIPSE_MAX_THICKNESS, hInstance, NULL);
	CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL,
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER | WS_TABSTOP,
		leftControlX, y, controlW, rowH, hwndContainer, (HMENU)ID_UI_ELLIPSE_MAX_THICKNESS, hInstance, NULL);
	SetEditInt(hwndContainer, ID_UI_ELLIPSE_MAX_THICKNESS, g_ELLIPSE_MAX_THICKNESS);
	y += rowH;

	// Ellipse min radius step
	CreateWindowW(L"STATIC", L"Ellipse min radius step:", WS_CHILD | WS_VISIBLE | SS_LEFT,
		leftLabelX, y, labelW, rowH, hwndContainer, (HMENU)ID_LBL_ELLIPSE_MIN_RADIUS_STEP, hInstance, NULL);
	CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL,
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER | WS_TABSTOP,
		leftControlX, y, controlW, rowH, hwndContainer, (HMENU)ID_UI_ELLIPSE_MIN_RADIUS_STEP, hInstance, NULL);
	SetEditInt(hwndContainer, ID_UI_ELLIPSE_MIN_RADIUS_STEP, g_ELLIPSE_MIN_RADIUS_STEP);
	y += rowH;

	// Ellipse max radius step
	CreateWindowW(L"STATIC", L"Ellipse max radius step:", WS_CHILD | WS_VISIBLE | SS_LEFT,
		leftLabelX, y, labelW, rowH, hwndContainer, (HMENU)ID_LBL_ELLIPSE_MAX_RADIUS_STEP, hInstance, NULL);
	CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL,
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER | WS_TABSTOP,
		leftControlX, y, controlW, rowH, hwndContainer, (HMENU)ID_UI_ELLIPSE_MAX_RADIUS_STEP, hInstance, NULL);
	SetEditInt(hwndContainer, ID_UI_ELLIPSE_MAX_RADIUS_STEP, g_ELLIPSE_MAX_RADIUS_STEP);
	y += rowH;

	// Ellipse min alpha step
	CreateWindowW(L"STATIC", L"Ellipse min alpha step:", WS_CHILD | WS_VISIBLE | SS_LEFT,
		leftLabelX, y, labelW, rowH, hwndContainer, (HMENU)ID_LBL_ELLIPSE_MIN_ALPHA_STEP, hInstance, NULL);
	CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL,
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER | WS_TABSTOP,
		leftControlX, y, controlW, rowH, hwndContainer, (HMENU)ID_UI_ELLIPSE_MIN_ALPHA_STEP, hInstance, NULL);
	SetEditInt(hwndContainer, ID_UI_ELLIPSE_MIN_ALPHA_STEP, g_ELLIPSE_MIN_ALPHA_STEP);
	y += rowH;

	// Ellipse max alpha step
	CreateWindowW(L"STATIC", L"Ellipse max alpha step:", WS_CHILD | WS_VISIBLE | SS_LEFT,
		leftLabelX, y, labelW, rowH, hwndContainer, (HMENU)ID_LBL_ELLIPSE_MAX_ALPHA_STEP, hInstance, NULL);
	CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL,
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_NUMBER | WS_TABSTOP,
		leftControlX, y, controlW, rowH, hwndContainer, (HMENU)ID_UI_ELLIPSE_MAX_ALPHA_STEP, hInstance, NULL);
	SetEditInt(hwndContainer, ID_UI_ELLIPSE_MAX_ALPHA_STEP, g_ELLIPSE_MAX_ALPHA_STEP);
	y += rowH * 2;

	// SAVE / EXIT
	CreateWindowW(L"BUTTON", L"&SAVE && EXIT", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
		0, y, (labelW + controlW) / 2, rowH, hwndContainer, (HMENU)ID_BTN_SAVE, hInstance, NULL);
	CreateWindowW(L"BUTTON", L"E&XIT", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
		(labelW + controlW) / 2, y, (labelW + controlW) / 2, rowH, hwndContainer, (HMENU)ID_BTN_EXIT, hInstance, NULL);
	y += rowH;

	// RESET
	CreateWindowW(L"BUTTON", L"&RESET TO DEFAULT", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
		0, y, labelW + controlW, rowH, hwndContainer, (HMENU)ID_BTN_RESET, hInstance, NULL);
	y += rowH * 2;

	// ABOUT
	CreateWindowW(L"STATIC", L"Copyright (C) 2025 Felipe R.T.", WS_CHILD | WS_VISIBLE | SS_LEFT,
		0, y, labelW + controlW, rowH, hwndContainer, (HMENU)ID_TXT_COPYRIGHT, hInstance, NULL);
	y += rowH;

	wchar_t g_Link[50] = L"https://github.com/FelipeRT98/Drops";
	CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", g_Link,
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY | WS_TABSTOP,
		0, y, labelW + controlW, rowH, hwndContainer, (HMENU)ID_TXT_LINK, hInstance, NULL);
	y += rowH;

	g_contentHeight = y;

	RECT rcClient;
	GetClientRect(g_hwndMain, &rcClient);

	int clientHeight = rcClient.bottom;
	int desiredContainerHeight = max(g_contentHeight, clientHeight);

	SetWindowPos(hwndContainer, NULL, 0, 0, labelW + controlW + scrollBarW, desiredContainerHeight,
		SWP_NOZORDER | SWP_SHOWWINDOW);

	CacheChildBaseRects();
	ScrollTo(0, hwnd);
	InvalidateRect(hwndContainer, NULL, TRUE);
	UpdateWindow(hwndContainer);
	RedrawWindow(hwndContainer, NULL, NULL, RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_FRAME);

	SCROLLINFO si = {0};
	si.cbSize = sizeof(si);
	si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
	si.nMin = 0;
	si.nMax = max(0, g_contentHeight - 1);
	si.nPage = rcClient.bottom;
	si.nPos = 0;
	SetScrollInfo(g_hwndMain, SB_VERT, &si, TRUE);

	// Ensure initial enable/disable state for the text box
	// ToggleTextToDisplay() already called above

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		if (!IsDialogMessageW(hwnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (!IsWindow(hwnd))
			break;
	}
}
