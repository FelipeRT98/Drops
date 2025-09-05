#ifndef DROPS_SETTINGS_H
#define DROPS_SETTINGS_H

#include <windows.h>
#include <vector>
#include <string>
#include <cstdlib>

using namespace std;

// Registry subkeys (tries W11/10/7 first, falls back to XP path)
#define REG_PATH_W11 L"Software\\Microsoft\\Windows\\CurrentVersion\\Screensavers\\Drops"
#define REG_PATH_XP  L"Software\\Microsoft\\Screensavers\\Drops"

// Defaults
#define DEFAULT_TEXT_TO_DISPLAY L""
#define DEFAULT_TEXT_CHECK      BST_CHECKED
#define DEFAULT_FONT_NAME       L"Arial"
#define DEFAULT_FONT_SIZE       54
#define DEFAULT_ELLIPSE_WAIT    9
#define DEFAULT_MAX_ELLIPSES    7
#define DEFAULT_MIN_THICKNESS   1
#define DEFAULT_MAX_THICKNESS   100
#define DEFAULT_MIN_RADIUS      1
#define DEFAULT_MAX_RADIUS      2
#define DEFAULT_MIN_ALPHA       1
#define DEFAULT_MAX_ALPHA       10

// Globals (defined in Settings.cpp)
extern wchar_t g_TextToDisplay[33];          // up to 32 chars + null
extern int     g_TextCheck;
extern wchar_t g_FontName[LF_FACESIZE];
extern int     g_FontSize;
extern int     g_ELLIPSE_WAIT;
extern int     g_MAX_ELLIPSES;
extern int     g_ELLIPSE_MIN_THICKNESS;
extern int     g_ELLIPSE_MAX_THICKNESS;
extern int     g_ELLIPSE_MIN_RADIUS_STEP;
extern int     g_ELLIPSE_MAX_RADIUS_STEP;
extern int     g_ELLIPSE_MIN_ALPHA_STEP;
extern int     g_ELLIPSE_MAX_ALPHA_STEP;
extern vector<wstring>     g_fonts;

// API
void SetFontNames(void);
void LoadSettings(void);
void SaveSettings(void);
void ShowConfigureDialog(HINSTANCE hInstance);

#endif // DROPS_SETTINGS_H
