#include "ThemeEngine.h"
#include "Globals.h"
#include "Utils.h"

BOOL IsSystemInDarkMode() {
    HKEY hKey;
    DWORD value = 0;
    DWORD size = sizeof(value);
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExA(hKey, "AppsUseLightTheme", NULL, NULL, (LPBYTE)&value, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return (value == 0); // 0       1      
        }
        RegCloseKey(hKey);
    }
    return FALSE;
}

void EnableImmersiveDarkMode(HWND hwnd, BOOL bEnable) {
    HMODULE hDwm = LoadLibraryA("dwmapi.dll");
    if (hDwm) {
        PDwmSetWindowAttribute pDwmSetWindowAttribute = (PDwmSetWindowAttribute)(void*)GetProcAddress(hDwm, "DwmSetWindowAttribute");
        if (pDwmSetWindowAttribute) {
            BOOL value = bEnable;
            pDwmSetWindowAttribute(hwnd, 20, &value, sizeof(value));
            pDwmSetWindowAttribute(hwnd, 19, &value, sizeof(value));
        }
        FreeLibrary(hDwm);
    }
}

void ApplyThemeToControl(HWND hwnd) {
    HMODULE hUxTheme = LoadLibraryA("uxtheme.dll");
    if (hUxTheme) {
        PSetWindowTheme pSetWindowTheme = (PSetWindowTheme)(void*)GetProcAddress(hUxTheme, "SetWindowTheme");
        if (pSetWindowTheme) {
            if (g_bDarkMode) {
                pSetWindowTheme(hwnd, L"DarkMode_Explorer", NULL);
            } else {
                pSetWindowTheme(hwnd, L"Explorer", NULL);
            }
        }
        FreeLibrary(hUxTheme);
    }
}

void UpdateThemeColors() {
    g_bDarkMode = IsSystemInDarkMode();
    
    if (g_bDarkMode) {
        g_ColorBg = COLOR_BG_DARK;
        g_ColorCard = COLOR_CARD_DARK;
        g_ColorCtrlBg = COLOR_CTRL_BG_DARK;
        g_ColorText = COLOR_TEXT_DARK;
        g_ColorTextMuted = COLOR_TEXT_MUTED_DARK;
        g_ColorAccent = COLOR_ACCENT_DARK;
        g_ColorAccentHover = COLOR_ACCENT_HOVER_DARK;
        g_ColorAccentPush = COLOR_ACCENT_PUSH_DARK;
        g_ColorSecondary = COLOR_SECONDARY_DARK;
        g_ColorSecHover = COLOR_SEC_HOVER_DARK;
        g_ColorSecPush = COLOR_SEC_PUSH_DARK;
        g_ColorBorder = COLOR_BORDER_DARK;
    } else {
        g_ColorBg = COLOR_BG_LIGHT;
        g_ColorCard = COLOR_CARD_LIGHT;
        g_ColorCtrlBg = COLOR_CTRL_BG_LIGHT;
        g_ColorText = COLOR_TEXT_LIGHT;
        g_ColorTextMuted = COLOR_TEXT_MUTED_LIGHT;
        g_ColorAccent = COLOR_ACCENT_LIGHT;
        g_ColorAccentHover = COLOR_ACCENT_HOVER_LIGHT;
        g_ColorAccentPush = COLOR_ACCENT_PUSH_LIGHT;
        g_ColorSecondary = COLOR_SECONDARY_LIGHT;
        g_ColorSecHover = COLOR_SEC_HOVER_LIGHT;
        g_ColorSecPush = COLOR_SEC_PUSH_LIGHT;
        g_ColorBorder = COLOR_BORDER_LIGHT;
    }
    
    if (hBrushBg) DeleteObject(hBrushBg);
    if (hBrushCard) DeleteObject(hBrushCard);
    if (hBrushCtrlBg) DeleteObject(hBrushCtrlBg);
    
    hBrushBg = CreateSolidBrush(g_ColorBg);
    hBrushCard = CreateSolidBrush(g_ColorCard);
    hBrushCtrlBg = CreateSolidBrush(g_ColorCtrlBg);
}

void InitializeThemeResources() {
    hBrushBg = CreateSolidBrush(g_ColorBg);
    hBrushCard = CreateSolidBrush(g_ColorCard);
    hBrushCtrlBg = CreateSolidBrush(g_ColorCtrlBg);
    
    hFontTitle = CreateFontA(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, GB2312_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Microsoft YaHei");
    hFontSubtitle = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GB2312_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Microsoft YaHei");
    hFontNormal = CreateFontA(15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GB2312_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Microsoft YaHei");
    hFontLog = CreateFontA(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GB2312_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Courier New");
}

void CleanThemeResources() {
    if (hBrushBg) DeleteObject(hBrushBg);
    if (hBrushCard) DeleteObject(hBrushCard);
    if (hBrushCtrlBg) DeleteObject(hBrushCtrlBg);
    if (hFontTitle) DeleteObject(hFontTitle);
    if (hFontSubtitle) DeleteObject(hFontSubtitle);
    if (hFontNormal) DeleteObject(hFontNormal);
    if (hFontLog) DeleteObject(hFontLog);
}

LRESULT CALLBACK ModernButtonSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    (void)uIdSubclass;
    (void)dwRefData;
    switch (msg) {
        case WM_MOUSEMOVE: {
            TRACKMOUSEEVENT tme = {};
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
            
            if (!GetPropA(hwnd, "hover")) {
                SetPropA(hwnd, "hover", (HANDLE)1);
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        }
        case WM_MOUSELEAVE: {
            if (GetPropA(hwnd, "hover")) {
                RemovePropA(hwnd, "hover");
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        }
        case WM_DESTROY: {
            RemovePropA(hwnd, "hover");
            break;
        }
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void MakeButtonModern(HWND hBtn) {
    LONG_PTR style = GetWindowLongPtr(hBtn, GWL_STYLE);
    SetWindowLongPtr(hBtn, GWL_STYLE, style | BS_OWNERDRAW);
    SetWindowSubclass(hBtn, ModernButtonSubclassProc, (UINT_PTR)hBtn, 0);
}

void DrawModernButton(LPDRAWITEMSTRUCT pdis) {
    HWND hBtn = pdis->hwndItem;
    UINT id = pdis->CtlID;
    HDC hdc = pdis->hDC;
    RECT rect = pdis->rcItem;
    
    BOOL isPressed = (pdis->itemState & ODS_SELECTED);
    BOOL isDisabled = (pdis->itemState & ODS_DISABLED);
    BOOL isHovered = (GetPropA(hBtn, "hover") != NULL);
    
    BOOL isPrimary = (id == ID_BTN_RUN || id == ID_BTN_ADD_GRP || id == ID_BTN_RESET || id == (ID_BTN_REMOVE + 10));
    
    COLORREF bgCol, textCol = g_ColorText;
    if (isDisabled) {
        bgCol = g_bDarkMode ? RGB(45, 45, 45) : RGB(220, 220, 220);
        textCol = g_ColorTextMuted;
    } else if (isPrimary) {
        if (isPressed) bgCol = g_ColorAccentPush;
        else if (isHovered) bgCol = g_ColorAccentHover;
        else bgCol = g_ColorAccent;
        textCol = RGB(255, 255, 255); 
    } else {
        if (isPressed) bgCol = g_ColorSecPush;
        else if (isHovered) bgCol = g_ColorSecHover;
        else bgCol = g_ColorSecondary;
        textCol = g_ColorText;
    }
    
    HBRUSH hBrush = CreateSolidBrush(bgCol);
    HPEN hPen = CreatePen(PS_SOLID, 1, isHovered && !isPrimary && !isDisabled ? g_ColorBorder : bgCol);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
    
    RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 6, 6);
    
    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hPen);
    DeleteObject(hBrush);
    
    char text[256];
    GetWindowTextA(hBtn, text, sizeof(text));
    
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textCol);
    
    HFONT hFont = (HFONT)SendMessageA(hBtn, WM_GETFONT, 0, 0);
    HFONT hOldFont = NULL;
    if (hFont) hOldFont = (HFONT)SelectObject(hdc, hFont);
    
    DrawTextA(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    if (hOldFont) SelectObject(hdc, hOldFont);
}

void DrawCardFrame(HDC hdc, int x, int y, int w, int h, const char* title) {
    HBRUSH hCardBrush = CreateSolidBrush(g_ColorCard);
    HPEN hBorderPen = CreatePen(PS_SOLID, 1, g_ColorBorder);
    
    HPEN hOldPen = (HPEN)SelectObject(hdc, hBorderPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hCardBrush);
    
    RoundRect(hdc, x, y, x + w, y + h, 8, 8);
    
    SelectObject(hdc, hOldPen);
    SelectObject(hdc, hOldBrush);
    DeleteObject(hBorderPen);
    DeleteObject(hCardBrush);
    
    if (title && strlen(title) > 0) {
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, g_ColorAccent);
        HFONT hFont = CreateFontA(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, GB2312_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Microsoft YaHei");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        
        RECT textRect = { x + 12, y + 8, x + w - 12, y + 25 };
        DrawTextA(hdc, title, -1, &textRect, DT_LEFT | DT_SINGLELINE);
        
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
    }
}

