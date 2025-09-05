#include <windows.h>
#include <sstream>
#include <iostream>
#include <vector>
#include <map>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <string>

#include "resource.h"
#include "Settings.h"

using namespace std;

//-----------------------------------------------------------------------------------------------------------

LPCWSTR TEXT_T_D_ARRAY[3] =
{
	L"KATY, BAN OUR NEWBORN RUNES",
	L"RUB AN EASTERN NOUN BY WORK",
	L"BARTON BUYERS ARE UNKNOWN"};
	LPCWSTR textToDisplay;

	LPCWSTR TEXT_FONT_NAME = L"Arial";

	int TEXT_SIZE = 54;

	int LONG_TIMER_FRAME_TIME = 5000;
	int SHORT_TIMER_FRAME_TIME = 33;

	int ELLIPSE_WAIT = 9;
	int MAX_ELLIPSES = 7;

	int ELLIPSE_MIN_THICKNESS = 1;
	int ELLIPSE_MAX_THICKNESS = 100;

	int ELLIPSE_MIN_RADIUS_STEP = 1;
	int ELLIPSE_MAX_RADIUS_STEP = 2;

	int ELLIPSE_MIN_ALPHA_STEP = 1;
	int ELLIPSE_MAX_ALPHA_STEP = 10;

	//-----------------------------------------------------------------------------------------------------------

	const wchar_t LP_CLASS_NAME[] = L"DropsClass";
	const wchar_t LP_WINDOW_NAME[] = L"Drops";

	POINT g_initMousePos = {0};
	bool g_hasInitMouse = false;

	struct MonitorInfoStruct
	{
		RECT monitorRect, workRect;
		bool isPrimary;
	};
	vector<MonitorInfoStruct> monitors;
	vector<HWND> windows;

	struct WindowStruct
	{
		int textX, textY;
		int textMaxX, textMaxY;
		COLORREF mainColor;
		COLORREF invertedColor;
	};
	map<HWND, WindowStruct> windowMap;

	HFONT textFont = NULL;
	HCURSOR blankCursor = NULL;

	struct Drop
	{
		int x, y, radius, alpha, thickness, radiusStep, alphaStep;
		bool active;
	};
	map<HWND, vector<Drop>> drops;

	enum
	{
		TID_LONG = 1,
		TID_SHORT = 2
	};

	HDC g_memDC = NULL;
	HBITMAP g_backBmp = NULL;
	HBITMAP g_oldBmp = NULL;
	int g_bbW = 0, g_bbH = 0;

	RECT windowRect;
	RECT textRect = {0, 0, 0, 0};
	int textW = 0, textH = 0;

	//-----------------------------------------------------------------------------------------------------------

	// MONITORS
	//-----------------------------------------------------------------------------------------------------------

	BOOL CALLBACK MonitorEnumProc(HMONITOR hMon, HDC, LPRECT, LPARAM)
	{
		MONITORINFO mi;
		mi.cbSize = sizeof(mi);

		if (GetMonitorInfo(hMon, &mi))
		{
			MonitorInfoStruct s;
			s.monitorRect = mi.rcMonitor;
			s.workRect = mi.rcWork;
			s.isPrimary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
			monitors.push_back(s);
		}

		return TRUE;
	}

	void SetMonitors()
	{
		monitors.clear();
		EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
	}

	//-----------------------------------------------------------------------------------------------------------

	// HELPERS
	//-----------------------------------------------------------------------------------------------------------
void EnableDpiAware()
{
    // Only available Vista+
    HMODULE hUser32 = LoadLibraryW(L"user32.dll");
    if (hUser32)
    {
        typedef BOOL (WINAPI *SetProcessDPIAwareFunc)();
        SetProcessDPIAwareFunc pSetProcessDPIAware =
            (SetProcessDPIAwareFunc)GetProcAddress(hUser32, "SetProcessDPIAware");

        if (pSetProcessDPIAware)
        {
            pSetProcessDPIAware(); // make process system-DPI aware
        }

        FreeLibrary(hUser32);
    }
}


	void DeleteObjects()
	{
		if (g_memDC)
		{
			SelectObject(g_memDC, g_oldBmp);
			DeleteObject(g_backBmp);
			DeleteDC(g_memDC);
		}
		DeleteObject(textFont);
		DeleteObject(blankCursor);
	}

	void HideCursor(WNDCLASS wc)
	{
		blankCursor = CreateCursor(NULL, 0, 0, 0, 0, NULL, NULL);
		wc.hCursor = blankCursor;
		SetCursor(blankCursor);
		ShowCursor(false);
	}

	static WNDCLASS GetWindowClass(WNDCLASS wc, WNDPROC wp, HINSTANCE hi)
	{
		ZeroMemory(&wc, sizeof(wc));
		wc.lpfnWndProc = wp;
		wc.hInstance = hi;
		wc.hIcon = LoadIcon(hi, MAKEINTRESOURCE(IDI_DROPS));
		wc.hbrBackground = NULL;
		wc.lpszClassName = LP_CLASS_NAME;

		return wc;
	}
	static HFONT GetTextFont()
	{
		return CreateFontW(
			TEXT_SIZE,
			0, // width (auto)
			0,
			0,
			FW_BOLD,
			FALSE, // italic
			FALSE, // underline
			FALSE, // strike
			DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS,
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE,
			TEXT_FONT_NAME);
	}

	static int GetRandom(int min, int max)
	{
		return min + (rand() % (max - min + 1));
	}

	static COLORREF GetInvertedColor(COLORREF color)
	{
		return RGB(255 - GetRValue(color), 255 - GetGValue(color), 255 - GetBValue(color));
	}

	static COLORREF GetLerpColor(COLORREF colorA, COLORREF colorB, int t)
	{
		if (t < 0)
		{
			t = 0;
		}
		if (t > 255)
		{
			t = 255;
		}

		int colorARed = GetRValue(colorA), colorAGreen = GetGValue(colorA), colorABlue = GetBValue(colorA);
		int colorBRed = GetRValue(colorB), colorBGreen = GetGValue(colorB), colorBBlue = GetBValue(colorB);

		return RGB(
			(colorARed * (255 - t) + colorBRed * t) / 255,
			(colorAGreen * (255 - t) + colorBGreen * t) / 255,
			(colorABlue * (255 - t) + colorBBlue * t) / 255);
	}

	static vector<Drop> &DropsFor(HWND hwnd)
	{
		map<HWND, vector<Drop>>::iterator it = drops.find(hwnd);

		if (it == drops.end())
		{
			vector<Drop> v(MAX_ELLIPSES);

			for (int i = 0; i < MAX_ELLIPSES; ++i)
			{
				v[i].active = false;
				v[i].x = 0;
				v[i].y = 0;
				v[i].radius = 0;
				v[i].alpha = 0;
				v[i].thickness = GetRandom(ELLIPSE_MIN_THICKNESS, ELLIPSE_MAX_THICKNESS);
				v[i].radiusStep = GetRandom(ELLIPSE_MIN_RADIUS_STEP, ELLIPSE_MAX_RADIUS_STEP);
				v[i].alphaStep = GetRandom(ELLIPSE_MIN_ALPHA_STEP, ELLIPSE_MAX_ALPHA_STEP);
			}

			drops[hwnd] = v;
			it = drops.find(hwnd);
		}

		return it->second;
	}

	static void SpawnDrop(HWND hwnd)
	{
		RECT rc;
		GetClientRect(hwnd, &rc);

		int w = rc.right - rc.left;
		int h = rc.bottom - rc.top;

		if (w <= 0 || h <= 0)
		{
			return;
		}

		vector<Drop> &v = DropsFor(hwnd);

		for (size_t i = 0; i < v.size(); ++i)
		{
			if (!v[i].active)
			{
				v[i].active = true;
				v[i].radius = 0;
				v[i].alpha = 255;
				v[i].x = (rand() % (w - 20)) + 10;
				v[i].y = (rand() % (h - 20)) + 10;
				v[i].thickness = GetRandom(ELLIPSE_MIN_THICKNESS, ELLIPSE_MAX_THICKNESS);
				v[i].radiusStep = GetRandom(ELLIPSE_MIN_RADIUS_STEP, ELLIPSE_MAX_RADIUS_STEP);
				v[i].alphaStep = GetRandom(ELLIPSE_MIN_ALPHA_STEP, ELLIPSE_MAX_ALPHA_STEP);
				return;
			}
		}
	}

	static void TickDrops(HWND hwnd)
	{

		vector<Drop> &v = DropsFor(hwnd);

		for (size_t i = 0; i < v.size(); ++i)
		{

			if (!v[i].active)
			{
				continue;
			}

			v[i].radius += v[i].radiusStep;

			if (v[i].alpha > v[i].alphaStep)
			{
				v[i].alpha -= v[i].alphaStep;
			}
			else
			{
				v[i].alpha = 0;
			}

			if (v[i].alpha <= 0)
			{
				v[i].active = false;
			}
		}
	}

	static void EnsureBackbuffer(HWND hwnd, int w, int h)
	{
		if (w == g_bbW && h == g_bbH && g_memDC)
		{
			return;
		}

		if (g_memDC)
		{
			SelectObject(g_memDC, g_oldBmp);

			DeleteObject(g_backBmp);
			DeleteDC(g_memDC);

			g_memDC = NULL;
			g_backBmp = NULL;
			g_oldBmp = NULL;
		}

		HDC wndDC = GetDC(hwnd);

		g_memDC = CreateCompatibleDC(wndDC);
		g_backBmp = CreateCompatibleBitmap(wndDC, w, h);
		g_oldBmp = (HBITMAP)SelectObject(g_memDC, g_backBmp);

		ReleaseDC(hwnd, wndDC);

		g_bbW = w;
		g_bbH = h;
	}

	//-----------------------------------------------------------------------------------------------------------

	// WINDOW PROC
	//-----------------------------------------------------------------------------------------------------------

	LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_CREATE:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hwnd, &ps);

				SelectObject(hdc, textFont);
				SetBkMode(hdc, TRANSPARENT);
				SetTextColor(hdc, windowMap[hwnd].mainColor);

				GetClientRect(hwnd, &windowRect);

				RECT textFontRect = {0, 0, 0, 0};

				DrawTextW(hdc, textToDisplay, -1, &textFontRect, DT_SINGLELINE | DT_CALCRECT);

				textW = textFontRect.right - textFontRect.left;
				textH = textFontRect.bottom - textFontRect.top;

				windowMap[hwnd].textMaxX = max(0, (windowRect.right - windowRect.left) - textW);
				windowMap[hwnd].textMaxY = max(0, (windowRect.bottom - windowRect.top) - textH);

				windowMap[hwnd].textX = rand() % (windowMap[hwnd].textMaxX + 1);
				windowMap[hwnd].textY = rand() % (windowMap[hwnd].textMaxY + 1);

				return 0;
			}

		case WM_SIZE:
			{
				GetClientRect(hwnd, &windowRect);

				int w = windowRect.right - windowRect.left;
				int h = windowRect.bottom - windowRect.top;

				if (w < 1)
				{w = 1;}
				if (h < 1)
				{h = 1;}

				EnsureBackbuffer(hwnd, w, h);

				return 0;
			}

		case WM_ERASEBKGND:
			{
				return 1; // Fully repaint
			}

		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hwnd, &ps);

				int w = windowRect.right - windowRect.left;
				int h = windowRect.bottom - windowRect.top;
				EnsureBackbuffer(hwnd, max(1, w), max(1, h));

				COLORREF mainColor = windowMap[hwnd].mainColor;
				COLORREF invertedColor = windowMap[hwnd].invertedColor;

				// ----- DRAW EVERYTHING INTO g_memDC -----

				// BACKGROUND
				//-----------------------------------------------------------------------------------------------
				SelectObject(g_memDC, GetStockObject(DC_BRUSH));
				SetDCBrushColor(g_memDC, windowMap[hwnd].mainColor);
				PatBlt(g_memDC, 0, 0, w, h, PATCOPY);

				// ELLIPSES
				//-----------------------------------------------------------------------------------------------
				SelectObject(g_memDC, GetStockObject(NULL_BRUSH));

				vector<Drop> &v = DropsFor(hwnd);
				for (size_t i = 0; i < v.size(); ++i)
				{
					if (!v[i].active)
					{
						continue;
					}

					COLORREF ellipseBorderColor = GetLerpColor(mainColor, invertedColor, v[i].alpha);

					HPEN newPen = CreatePen(PS_SOLID, v[i].thickness, ellipseBorderColor);
					HPEN oldPen = (HPEN)SelectObject(g_memDC, newPen);

					Ellipse(g_memDC,
						v[i].x - v[i].radius,
						v[i].y - v[i].radius,
						v[i].x + v[i].radius,
						v[i].y + v[i].radius);

					SelectObject(g_memDC, oldPen);
					DeleteObject(newPen);
				}

				// TEXT
				//-----------------------------------------------------------------------------------------------
				SetBkMode(g_memDC, TRANSPARENT);
				SetTextColor(g_memDC, invertedColor);
				SelectObject(g_memDC, textFont);
				RECT textRect;
				SetRect(&textRect,
					windowMap[hwnd].textX,
					windowMap[hwnd].textY,
					windowMap[hwnd].textX + textW,
					windowMap[hwnd].textY + textH);
				DrawTextW(g_memDC, textToDisplay, -1, &textRect, DT_SINGLELINE);

				// ----- BLIT BACKBUFFER TO WINDOW -----
				BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top,
					ps.rcPaint.right - ps.rcPaint.left,
					ps.rcPaint.bottom - ps.rcPaint.top,
					g_memDC, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);

				EndPaint(hwnd, &ps);
				return 0;
			}

		case WM_MOUSEMOVE:
			{
				POINT pt;
				GetCursorPos(&pt);

				if (!g_hasInitMouse)
				{
					g_initMousePos = pt;
					g_hasInitMouse = true;
				}
				else
				{
					if (pt.x != g_initMousePos.x || pt.y != g_initMousePos.y)
					{
						PostQuitMessage(0);
					}
				}
				return 0;
			}

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_MOUSEWHEEL:
		case WM_DESTROY:
			DeleteObjects();
			PostQuitMessage(0);
			return 0;
		}

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	LRESULT CALLBACK PreviewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_CREATE:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hwnd, &ps);
				SelectObject(hdc, textFont);
				SetBkMode(hdc, TRANSPARENT);
				SetTextColor(hdc, windowMap[hwnd].mainColor);

				GetClientRect(hwnd, &windowRect);

				RECT textFontRect = {0, 0, 0, 0};

				DrawTextW(hdc, textToDisplay, -1, &textFontRect, DT_SINGLELINE | DT_CALCRECT);

				textW = textFontRect.right - textFontRect.left;
				textH = textFontRect.bottom - textFontRect.top;

				windowMap[hwnd].textMaxX = max(0, (windowRect.right - windowRect.left) - textW);
				windowMap[hwnd].textMaxY = max(0, (windowRect.bottom - windowRect.top) - textH);

				windowMap[hwnd].textX = rand() % (windowMap[hwnd].textMaxX + 1);
				windowMap[hwnd].textY = rand() % (windowMap[hwnd].textMaxY + 1);

				return 0;
			}

		case WM_SIZE:
			{
				GetClientRect(hwnd, &windowRect);

				int w = windowRect.right - windowRect.left;
				int h = windowRect.bottom - windowRect.top;

				if (w < 1){w = 1;}
				if (h < 1){h = 1;}

				EnsureBackbuffer(hwnd, w, h);

				return 0;
			}

		case WM_ERASEBKGND:
			{return 1;}

		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hwnd, &ps);

				int w = windowRect.right - windowRect.left;
				int h = windowRect.bottom - windowRect.top;
				EnsureBackbuffer(hwnd, max(1, w), max(1, h));

				COLORREF mainColor = windowMap[hwnd].mainColor;
				COLORREF invertedColor = windowMap[hwnd].invertedColor;

				// ----- DRAW EVERYTHING INTO g_memDC -----

				// BACKGROUND
				//-----------------------------------------------------------------------------------------------
				SelectObject(g_memDC, GetStockObject(DC_BRUSH));
				SetDCBrushColor(g_memDC, windowMap[hwnd].mainColor);
				PatBlt(g_memDC, 0, 0, w, h, PATCOPY);

				// ELLIPSES
				//-----------------------------------------------------------------------------------------------
				SelectObject(g_memDC, GetStockObject(NULL_BRUSH));

				vector<Drop> &v = DropsFor(hwnd);
				for (size_t i = 0; i < v.size(); ++i)
				{
					if (!v[i].active)
					{
						continue;
					}

					COLORREF ellipseBorderColor = GetLerpColor(mainColor, invertedColor, v[i].alpha);

					HPEN newPen = CreatePen(PS_SOLID, v[i].thickness, ellipseBorderColor);
					HPEN oldPen = (HPEN)SelectObject(g_memDC, newPen);

					Ellipse(g_memDC,
						v[i].x - v[i].radius,
						v[i].y - v[i].radius,
						v[i].x + v[i].radius,
						v[i].y + v[i].radius);

					SelectObject(g_memDC, oldPen);
					DeleteObject(newPen);
				}

				// TEXT
				//-----------------------------------------------------------------------------------------------
				SetBkMode(g_memDC, TRANSPARENT);
				SetTextColor(g_memDC, invertedColor);
				SelectObject(g_memDC, textFont);
				RECT textRect;
				SetRect(&textRect,
					windowMap[hwnd].textX,
					windowMap[hwnd].textY,
					windowMap[hwnd].textX + textW,
					windowMap[hwnd].textY + textH);
				DrawTextW(g_memDC, textToDisplay, -1, &textRect, DT_SINGLELINE);

				// ----- BLIT BACKBUFFER TO WINDOW -----
				BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top,
					ps.rcPaint.right - ps.rcPaint.left,
					ps.rcPaint.bottom - ps.rcPaint.top,
					g_memDC, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);

				EndPaint(hwnd, &ps);
				return 0;
			}

		case WM_DESTROY:
			DeleteObjects();
			PostQuitMessage(0);
			return 0;
		}

		return DefWindowProc(hwnd, msg, wParam, lParam);
	}

	//-----------------------------------------------------------------------------------------------------------

	// TIMERS
	//-----------------------------------------------------------------------------------------------------------

	VOID CALLBACK longTimerProc(HWND, UINT, UINT_PTR, DWORD)
	{
		for (size_t i = 0; i < windows.size(); ++i)
		{
			HWND hwnd = windows[i];
			GetClientRect(hwnd, &windowRect);

			windowMap[hwnd].mainColor = RGB(rand() % 256, rand() % 256, rand() % 256);
			windowMap[hwnd].invertedColor = GetInvertedColor(windowMap[hwnd].mainColor);

			windowMap[hwnd].textX = rand() % (windowMap[hwnd].textMaxX + 1);
			windowMap[hwnd].textY = rand() % (windowMap[hwnd].textMaxY + 1);

			InvalidateRect(windows[i], NULL, FALSE);
		}
	}

	VOID CALLBACK shortTimerProc(HWND, UINT, UINT_PTR, DWORD)
	{
		for (size_t i = 0; i < windows.size(); ++i)
		{
			HWND hwnd = windows[i];

			vector<Drop> &v = DropsFor(hwnd);

			bool hasFree = false;

			for (size_t k = 0; k < v.size(); ++k)
			{
				if (!v[k].active)
				{
					hasFree = true;
					break;
				}
			}

			if (hasFree && (rand() % ELLIPSE_WAIT) == 0)
			{
				SpawnDrop(hwnd);
			}

			TickDrops(hwnd);
			InvalidateRect(hwnd, NULL, FALSE);
		}
	}

	//-----------------------------------------------------------------------------------------------------------

	int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow)
	{
		EnableDpiAware();

		wstring cmd(lpCmdLine);
		transform(cmd.begin(), cmd.end(), cmd.begin(), towlower);

		if (cmd.find(L"/p") != wstring::npos)
		{
			size_t pos = cmd.find(L"/p");
			if (pos != wstring::npos)
			{
				wstring hwndStr = cmd.substr(pos + 3);
				//HWND previewHWND = (HWND)_wtoi(hwndStr.c_str());
				HWND previewHWND = (HWND)(ULONG_PTR)wcstoul(hwndStr.c_str(), NULL, 10);

				if (previewHWND)
				{
					srand((unsigned)time(0));

					// SET SETTINGS
					textToDisplay = TEXT_T_D_ARRAY[GetRandom(0, (sizeof(TEXT_T_D_ARRAY) / sizeof(TEXT_T_D_ARRAY[0])) - 1)];

					TEXT_FONT_NAME = L"Arial";
					TEXT_SIZE = 10;

					ELLIPSE_WAIT = 9;
					MAX_ELLIPSES = 5;

					ELLIPSE_MIN_THICKNESS = 1;
					ELLIPSE_MAX_THICKNESS = 1;

					ELLIPSE_MIN_RADIUS_STEP = 1;
					ELLIPSE_MAX_RADIUS_STEP = 1;

					ELLIPSE_MIN_ALPHA_STEP = 4;
					ELLIPSE_MAX_ALPHA_STEP = 8;


					// FONT
					textFont = GetTextFont();

					// WINDOW
					WNDCLASS wc;
					wc = GetWindowClass(wc, PreviewWndProc, hInstance);
					RegisterClass(&wc);

					// X, Y, W, D
					RECT rc;
					GetClientRect(previewHWND, &rc);
					
					int windowWidth, windowHeight, windowPositionX, windowPositionY;

					windowWidth = rc.right - rc.left;
					windowHeight = rc.bottom - rc.top;
					//windowPositionX = rc.left;
					//windowPositionY = rc.top;

					// HWND
					HWND hwnd = CreateWindowExW(
						0,
						LP_CLASS_NAME, LP_WINDOW_NAME,
						WS_CHILD | WS_VISIBLE,
						0,0,//windowPositionX, windowPositionY,
						windowWidth, windowHeight,
						previewHWND,
						NULL,
						hInstance,
						NULL);

					if (!hwnd)
					{
						return 0;
					}

					windows.push_back(hwnd);

					longTimerProc(hwnd, WM_TIMER, LONG_TIMER_FRAME_TIME, GetTickCount());

					ShowWindow(hwnd, SW_SHOW);

					SetTimer(NULL, TID_LONG, LONG_TIMER_FRAME_TIME, longTimerProc);
					SetTimer(NULL, TID_SHORT, SHORT_TIMER_FRAME_TIME, shortTimerProc);

					MSG msg;
					while (GetMessage(&msg, NULL, 0, 0))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}

					DeleteObjects();

					return (int)msg.wParam;
				}
			}
		}
		else if (cmd.find(L"/s") != wstring::npos)
		{

			srand((unsigned)time(0));


			// SETTINGS
			SetFontNames();
			LoadSettings();
			SaveSettings();
			
			// SET SETTINGS
			if(g_TextCheck == BST_CHECKED)
			{
				textToDisplay = TEXT_T_D_ARRAY[GetRandom(0, (sizeof(TEXT_T_D_ARRAY) / sizeof(TEXT_T_D_ARRAY[0])) - 1)];
			}
			else
			{
				textToDisplay = g_TextToDisplay;
			}

			TEXT_FONT_NAME = g_FontName;
			TEXT_SIZE = g_FontSize;

			ELLIPSE_WAIT = g_ELLIPSE_WAIT;
			MAX_ELLIPSES = g_MAX_ELLIPSES;

			ELLIPSE_MIN_THICKNESS = g_ELLIPSE_MIN_THICKNESS;
			ELLIPSE_MAX_THICKNESS = g_ELLIPSE_MAX_THICKNESS;

			ELLIPSE_MIN_RADIUS_STEP = g_ELLIPSE_MIN_RADIUS_STEP;
			ELLIPSE_MAX_RADIUS_STEP = g_ELLIPSE_MAX_RADIUS_STEP;

			ELLIPSE_MIN_ALPHA_STEP = g_ELLIPSE_MIN_ALPHA_STEP;
			ELLIPSE_MAX_ALPHA_STEP = g_ELLIPSE_MAX_ALPHA_STEP;

			// TEXT
			textFont = GetTextFont();

			// WINDOW
			WNDCLASS wc;
			wc = GetWindowClass(wc, WndProc, hInstance);
			RegisterClass(&wc);

			// CURSOR
			HideCursor(wc);

			int windowWidth, windowHeight, windowPositionX, windowPositionY;
			SetMonitors();
			for (size_t i = 0; i < monitors.size(); ++i)
			{
				const MonitorInfoStruct &m = monitors[i];

				// X, Y, W, H
				windowWidth = m.monitorRect.right - m.monitorRect.left;
				windowHeight = m.monitorRect.bottom - m.monitorRect.top;
				windowPositionX = m.monitorRect.left;
				windowPositionY = m.monitorRect.top;

				// HWND
				HWND hwnd = CreateWindowExW(
					WS_EX_TOPMOST,
					LP_CLASS_NAME, LP_WINDOW_NAME,
					WS_POPUP,
					windowPositionX, windowPositionY,
					windowWidth, windowHeight,
					NULL,
					NULL,
					hInstance,
					NULL);

				if (!hwnd)
				{
					return 0;
				}

				windows.push_back(hwnd);

				longTimerProc(hwnd, WM_TIMER, LONG_TIMER_FRAME_TIME, GetTickCount());

				ShowWindow(hwnd, nCmdShow);
			}

			SetTimer(NULL, TID_LONG, LONG_TIMER_FRAME_TIME, longTimerProc);
			SetTimer(NULL, TID_SHORT, SHORT_TIMER_FRAME_TIME, shortTimerProc);

			MSG msg;
			while (GetMessage(&msg, NULL, 0, 0))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			DeleteObjects();

			return (int)msg.wParam;
		}
		else
		{
			ShowConfigureDialog(hInstance);
			return 0;
		}
	}
