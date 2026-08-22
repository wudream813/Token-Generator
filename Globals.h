#pragma once
#include "Common.h"

// Constants
#define ID_LISTVIEW_GROUPS    2001
#define ID_BTN_ADD_GRP        2002
#define ID_BTN_DEL_GRP        2003
#define ID_BTN_EDIT_GRP       2004

#define ID_LISTVIEW_PRIVS     3001
#define ID_BTN_RESET          3002
#define ID_BTN_DISABLE        3003
#define ID_BTN_REMOVE         3004

#define ID_BTN_RUN            1001
#define ID_BTN_CANCEL         1002
#define ID_EDIT_CMD           1003
#define ID_COMBO_USER         1004
#define ID_COMBO_IL           1005
#define ID_CHECK_UIACCESS     1007
#define ID_CHECK_DEBUG        1009
#define ID_EDIT_DESKTOP       1010
#define ID_EDIT_DIR           1011
#define ID_COMBO_MODE         1012
#define ID_EDIT_LOG           1013
#define ID_EDIT_GROUPS        1014
#define ID_EDIT_PRIVS         1015
#define ID_BTN_CMD_BROWSE     1016
#define ID_BTN_DIR_BROWSE     1017
#define ID_COMBO_PRESET       1018
#define ID_BTN_EDIT_GRP_BTN   1019
#define ID_BTN_EDIT_PRIV_BTN  1020
#define ID_PREVIEW_LOG        1021

// Color macros
#define COLOR_BG_DARK            RGB(24, 24, 24)
#define COLOR_CARD_DARK          RGB(32, 32, 32)
#define COLOR_CTRL_BG_DARK       RGB(45, 45, 45)
#define COLOR_TEXT_DARK          RGB(240, 240, 240)
#define COLOR_TEXT_MUTED_DARK    RGB(160, 160, 160)
#define COLOR_ACCENT_DARK        RGB(0, 120, 215)
#define COLOR_ACCENT_HOVER_DARK  RGB(20, 140, 235)
#define COLOR_ACCENT_PUSH_DARK   RGB(0, 100, 180)
#define COLOR_SECONDARY_DARK     RGB(60, 60, 60)
#define COLOR_SEC_HOVER_DARK     RGB(80, 80, 80)
#define COLOR_SEC_PUSH_DARK      RGB(45, 45, 45)
#define COLOR_BORDER_DARK        RGB(70, 70, 70)

#define COLOR_BG_LIGHT           RGB(243, 243, 243)
#define COLOR_CARD_LIGHT         RGB(255, 255, 255)
#define COLOR_CTRL_BG_LIGHT      RGB(255, 255, 255)
#define COLOR_TEXT_LIGHT         RGB(32, 32, 32)
#define COLOR_TEXT_MUTED_LIGHT   RGB(110, 110, 110)
#define COLOR_ACCENT_LIGHT       RGB(0, 90, 158)
#define COLOR_ACCENT_HOVER_LIGHT RGB(16, 110, 190)
#define COLOR_ACCENT_PUSH_LIGHT  RGB(0, 74, 127)
#define COLOR_SECONDARY_LIGHT    RGB(225, 229, 235)
#define COLOR_SEC_HOVER_LIGHT    RGB(210, 215, 222)
#define COLOR_SEC_PUSH_LIGHT     RGB(190, 195, 202)
#define COLOR_BORDER_LIGHT       RGB(210, 210, 210)

// Globals declarations
extern COLORREF g_ColorBg;
extern COLORREF g_ColorCard;
extern COLORREF g_ColorCtrlBg;
extern COLORREF g_ColorText;
extern COLORREF g_ColorTextMuted;
extern COLORREF g_ColorAccent;
extern COLORREF g_ColorAccentHover;
extern COLORREF g_ColorAccentPush;
extern COLORREF g_ColorSecondary;
extern COLORREF g_ColorSecHover;
extern COLORREF g_ColorSecPush;
extern COLORREF g_ColorBorder;

extern BOOL g_bDarkMode;

extern bool g_bDebug;
extern bool g_bUIAccess;
extern std::vector<std::string> extraGroups;
extern std::vector<int> RemovePrivilege;
extern std::vector<int> DisabledPrivilege;
extern int g_WindowCreateMode;
extern DWORD Integrity_Level;
extern HWND g_hLogEdit;
extern HWND g_hGroupEditor;
extern HWND g_hPrivEditor;
extern HWND g_hPreviewEdit;
extern LPSTR g_desktop;
extern LPSTR g_runCommand;
extern LPSTR g_identityStr;

// Theme resources
extern HBRUSH hBrushBg;
extern HBRUSH hBrushCard;
extern HBRUSH hBrushCtrlBg;
extern HFONT hFontTitle;
extern HFONT hFontSubtitle;
extern HFONT hFontNormal;
extern HFONT hFontLog;

extern const char* AllPrivileges[];
extern PrivInfo g_PrivInfos[35];
