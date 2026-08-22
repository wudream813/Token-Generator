#include "MainWindow.h"
#include "Globals.h"
#include "Utils.h"
#include "ThemeEngine.h"
#include "SimConsole.h"
#include "UIEditor.h"
#include "TokenEngine.h"

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            UpdateThemeColors();
            EnableImmersiveDarkMode(hwnd, g_bDarkMode);
            InitializeThemeResources();
            
            //                  
            HICON hIcon = LoadIconA(GetModuleHandle(NULL), "IDI_ICON1");
            if (hIcon) {
                SendMessageA(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
                SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            }

            INITCOMMONCONTROLSEX icex; 
            icex.dwSize = sizeof(INITCOMMONCONTROLSEX); 
            icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_STANDARD_CLASSES; 
            InitCommonControlsEx(&icex);
            
            // ---    1:            ---
            HWND hLblPreset = CreateWindowA("STATIC", "\xb0\xb2\xc8\xab\xd4\xa4\xc9\xe8:", WS_CHILD | WS_VISIBLE, 30, 105, 215, 16, hwnd, NULL, NULL, NULL);
            SendMessageA(hLblPreset, WM_SETFONT, (WPARAM)hFontSubtitle, TRUE);
            
            HWND hComboPreset = CreateWindowA("COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST, 30, 125, 215, 200, hwnd, (HMENU)ID_COMBO_PRESET, NULL, NULL);
            const char* presets[] = { 
                "System", "System+", "Admin", "Admin+", "TrustedInstaller", "TrustedInstaller+", 
                "LOCAL SERVICE", "LOCAL SERVICE+", "NETWORK SERVICE", "NETWORK SERVICE+", "DWM", "DWM+" 
            };
            for (const char* p : presets) SendMessageA(hComboPreset, CB_ADDSTRING, 0, (LPARAM)p);
            SendMessageA(hComboPreset, CB_SETCURSEL, 0, 0);
            SendMessageA(hComboPreset, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
            ApplyThemeToControl(hComboPreset);
            
            HWND hLblUser = CreateWindowA("STATIC", "\xc4\xbf\xb1\xea\xc9\xed\xb7\xdd (\xd3\xc3\xbb\xa7/SID):", WS_CHILD | WS_VISIBLE, 265, 105, 215, 16, hwnd, NULL, NULL, NULL); 
            SendMessageA(hLblUser, WM_SETFONT, (WPARAM)hFontSubtitle, TRUE);
            
            HWND hComboUser = CreateWindowA("COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWN, 265, 125, 215, 200, hwnd, (HMENU)ID_COMBO_USER, NULL, NULL);
            const char* users[] = { "System", "TrustedInstaller", "Administrator", "LOCAL SERVICE", "NETWORK SERVICE", "DWM" }; 
            for (const char* u : users) SendMessageA(hComboUser, CB_ADDSTRING, 0, (LPARAM)u);
            SendMessageA(hComboUser, WM_SETTEXT, 0, (LPARAM)g_identityStr); 
            SendMessageA(hComboUser, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
            ApplyThemeToControl(hComboUser);
            
            // ---    2:        ---
            HWND hLblCmd = CreateWindowA("STATIC", "\xd6\xb4\xd0\xd0\xc3\xfc\xc1\xee\xd3\xeb\xb2\xce\xca\xfd:", WS_CHILD | WS_VISIBLE, 30, 205, 350, 16, hwnd, NULL, NULL, NULL); 
            SendMessageA(hLblCmd, WM_SETFONT, (WPARAM)hFontSubtitle, TRUE);
            
            HWND hEditCmd = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", g_runCommand, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 30, 225, 330, 25, hwnd, (HMENU)ID_EDIT_CMD, NULL, NULL); 
            SendMessageA(hEditCmd, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
            ApplyThemeToControl(hEditCmd);
            
            HWND hBtnFileBrowse = CreateWindowA("BUTTON", "\xe4\xaf\xc0\xc0...", WS_CHILD | WS_VISIBLE, 370, 225, 110, 25, hwnd, (HMENU)ID_BTN_CMD_BROWSE, NULL, NULL);
            SendMessageA(hBtnFileBrowse, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
            MakeButtonModern(hBtnFileBrowse);
            
            HWND hLblDir = CreateWindowA("STATIC", "\xb9\xa4\xd7\xf7\xc4\xbf\xc2\xbc (CWD):", WS_CHILD | WS_VISIBLE, 30, 260, 350, 16, hwnd, NULL, NULL, NULL);
            SendMessageA(hLblDir, WM_SETFONT, (WPARAM)hFontSubtitle, TRUE);
            
            char currDir[MAX_PATH]; GetCurrentDirectoryA(MAX_PATH, currDir);
            HWND hEditDir = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", currDir, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 30, 280, 330, 25, hwnd, (HMENU)ID_EDIT_DIR, NULL, NULL);
            SendMessageA(hEditDir, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
            ApplyThemeToControl(hEditDir);
            
            HWND hBtnDirBrowse = CreateWindowA("BUTTON", "\xe4\xaf\xc0\xc0...", WS_CHILD | WS_VISIBLE, 370, 280, 110, 25, hwnd, (HMENU)ID_BTN_DIR_BROWSE, NULL, NULL);
            SendMessageA(hBtnDirBrowse, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
            MakeButtonModern(hBtnDirBrowse);
            
            // ---    3:           ---
            HWND hLblDesk = CreateWindowA("STATIC", "\xd7\xc0\xc3\xe6\xb9\xa4\xd7\xf7\xc7\xf8 (Desktop):", WS_CHILD | WS_VISIBLE, 30, 360, 215, 16, hwnd, NULL, NULL, NULL); 
            SendMessageA(hLblDesk, WM_SETFONT, (WPARAM)hFontSubtitle, TRUE);
            
            HWND hEditDesktop = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", g_desktop, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 30, 380, 215, 25, hwnd, (HMENU)ID_EDIT_DESKTOP, NULL, NULL); 
            SendMessageA(hEditDesktop, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
            ApplyThemeToControl(hEditDesktop);
            
            HWND hLblIL = CreateWindowA("STATIC", "\xc7\xbf\xd6\xc6\xcd\xea\xd5\xfb\xd0\xd4\xbc\xb6\xb1\xf0 (IL):", WS_CHILD | WS_VISIBLE, 265, 360, 215, 16, hwnd, NULL, NULL, NULL); 
            SendMessageA(hLblIL, WM_SETFONT, (WPARAM)hFontSubtitle, TRUE);
            
            HWND hComboIL = CreateWindowA("COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 265, 380, 215, 200, hwnd, (HMENU)ID_COMBO_IL, NULL, NULL);
            const char* levels[] = { "System (\xcf\xb5\xcd\xb3\xbc\xb6)", "High (\xb8\xdf\xcd\xea\xd5\xfb\xd0\xd4)", "Medium+ (\xd6\xd0\xd4\xf6\xc7\xbf)", "Medium (\xd6\xd0\xcd\xea\xd5\xfb\xd0\xd4)", "Low (\xb5\xcd\xcd\xea\xd5\xfb\xd0\xd4)", "Untrusted (\xce\xb4\xd0\xc5\xc8\xce)" };
            for (const char* l : levels) SendMessageA(hComboIL, CB_ADDSTRING, 0, (LPARAM)l);
            switch(Integrity_Level) {
                case SECURITY_MANDATORY_HIGH_RID: SendMessageA(hComboIL, CB_SETCURSEL, 1, 0); break;
                case SECURITY_MANDATORY_MEDIUM_PLUS_RID: SendMessageA(hComboIL, CB_SETCURSEL, 2, 0); break;
                case SECURITY_MANDATORY_MEDIUM_RID: SendMessageA(hComboIL, CB_SETCURSEL, 3, 0); break;
                case SECURITY_MANDATORY_LOW_RID: SendMessageA(hComboIL, CB_SETCURSEL, 4, 0); break;
                case SECURITY_MANDATORY_UNTRUSTED_RID: SendMessageA(hComboIL, CB_SETCURSEL, 5, 0); break;
                default: SendMessageA(hComboIL, CB_SETCURSEL, 0, 0); break;
            }
            SendMessageA(hComboIL, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
            ApplyThemeToControl(hComboIL);
            
            HWND hLblGrp = CreateWindowA("STATIC", "\xb8\xbd\xbc\xd3\xd3\xc3\xbb\xa7\xd7\xe9 (Groups):", WS_CHILD | WS_VISIBLE, 30, 405, 215, 16, hwnd, NULL, NULL, NULL); 
            SendMessageA(hLblGrp, WM_SETFONT, (WPARAM)hFontSubtitle, TRUE);
            
            HWND hEditGroups = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY, 30, 425, 155, 25, hwnd, (HMENU)ID_EDIT_GROUPS, NULL, NULL); 
            SendMessageA(hEditGroups, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
            ApplyThemeToControl(hEditGroups);
            
            HWND hBtnGrpEdit = CreateWindowA("BUTTON", "\xd0\xde\xb8\xc4...", WS_CHILD | WS_VISIBLE, 195, 425, 50, 25, hwnd, (HMENU)ID_BTN_EDIT_GRP_BTN, NULL, NULL);
            SendMessageA(hBtnGrpEdit, WM_SETFONT, (WPARAM)hFontSubtitle, TRUE);
            MakeButtonModern(hBtnGrpEdit);
            
            HWND hLblPriv = CreateWindowA("STATIC", "\xc1\xee\xc5\xc6\xcc\xd8\xc8\xa8\xc1\xd0\xb1\xed (Privileges):", WS_CHILD | WS_VISIBLE, 265, 405, 215, 16, hwnd, NULL, NULL, NULL); 
            SendMessageA(hLblPriv, WM_SETFONT, (WPARAM)hFontSubtitle, TRUE);
            
            HWND hEditPrivs = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "\xbb\xd6\xb8\xb4\xc4\xac\xc8\xcf (\xc8\xab\xcc\xd8\xc8\xa8\xbc\xa4\xbb\xee)", WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY, 265, 425, 155, 25, hwnd, (HMENU)ID_EDIT_PRIVS, NULL, NULL); 
            SendMessageA(hEditPrivs, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
            ApplyThemeToControl(hEditPrivs);
            
            HWND hBtnPrivEdit = CreateWindowA("BUTTON", "\xd0\xde\xb8\xc4...", WS_CHILD | WS_VISIBLE, 430, 425, 50, 25, hwnd, (HMENU)ID_BTN_EDIT_PRIV_BTN, NULL, NULL);
            SendMessageA(hBtnPrivEdit, WM_SETFONT, (WPARAM)hFontSubtitle, TRUE);
            MakeButtonModern(hBtnPrivEdit);
            
            // ---    4:        ---
            HWND hComboMode = CreateWindowA("COMBOBOX", NULL, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST, 30, 510, 100, 200, hwnd, (HMENU)ID_COMBO_MODE, NULL, NULL);
            const char* modes[] = { "\xc6\xd5\xcd\xa8\xb4\xb0\xbf\xda", "\xba\xf3\xcc\xa8\xd2\xfe\xb2\xd8", "\xd7\xee\xb4\xf3\xbb\xaf", "\xd7\xee\xd0\xa1\xbb\xaf" }; 
            for (const char* m : modes) SendMessageA(hComboMode, CB_ADDSTRING, 0, (LPARAM)m);
            switch(g_WindowCreateMode) {
                case SW_SHOWMAXIMIZED: SendMessageA(hComboMode, CB_SETCURSEL, 2, 0); break;
                case SW_SHOWMINIMIZED: SendMessageA(hComboMode, CB_SETCURSEL, 3, 0); break;
                case SW_HIDE: SendMessageA(hComboMode, CB_SETCURSEL, 1, 0); break;
                default: SendMessageA(hComboMode, CB_SETCURSEL, 0, 0); break;
            }
            SendMessageA(hComboMode, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
            ApplyThemeToControl(hComboMode);
            
            HWND hCheckUIAccess = CreateWindowA("BUTTON", "UI Access", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 140, 512, 85, 20, hwnd, (HMENU)ID_CHECK_UIACCESS, NULL, NULL); 
            SendMessageA(hCheckUIAccess, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
            SendMessageA(hCheckUIAccess, BM_SETCHECK, (WPARAM)g_bUIAccess, 0);
            
            HWND hCheckDebug = CreateWindowA("BUTTON", "\xb5\xf7\xca\xd4\xc8\xd5\xd6\xbe", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 235, 512, 85, 20, hwnd, (HMENU)ID_CHECK_DEBUG, NULL, NULL); 
            SendMessageA(hCheckDebug, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
            SendMessageA(hCheckDebug, BM_SETCHECK, (WPARAM)g_bDebug, 0);
            
            HWND hBtnRun = CreateWindowA("BUTTON", "\xce\xb1\xd4\xec\xb2\xa2\xd4\xcb\xd0\xd0", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 330, 503, 150, 38, hwnd, (HMENU)ID_BTN_RUN, NULL, NULL); 
            SendMessageA(hBtnRun, WM_SETFONT, (WPARAM)hFontNormal, TRUE);
            MakeButtonModern(hBtnRun);
            
            // ---    5:       ---
            g_hLogEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "RICHEDIT50W", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_READONLY | ES_NOHIDESEL, 25, 605, 460, 120, hwnd, (HMENU)ID_EDIT_LOG, NULL, NULL);
            SendMessageA(g_hLogEdit, WM_SETFONT, (WPARAM)hFontLog, TRUE);
            SendMessageA(g_hLogEdit, EM_SETBKGNDCOLOR, 0, (LPARAM)(g_bDarkMode ? RGB(18, 18, 18) : RGB(249, 249, 249)));
            ApplyThemeToControl(g_hLogEdit);

            // ---     :            (whoami /all) ---
            g_hPreviewEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "RICHEDIT50W", "", WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_READONLY | ES_NOHIDESEL, 525, 110, 450, 615, hwnd, (HMENU)ID_PREVIEW_LOG, NULL, NULL);
            SendMessageA(g_hPreviewEdit, WM_SETFONT, (WPARAM)hFontLog, TRUE);
            SendMessageA(g_hPreviewEdit, EM_SETBKGNDCOLOR, 0, (LPARAM)(g_bDarkMode ? RGB(18, 18, 18) : RGB(249, 249, 249)));
            ApplyThemeToControl(g_hPreviewEdit);
            
            UpdateGuiSummaries(hEditGroups, hEditPrivs);
            UpdateTokenPreview(hwnd);
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rect;
            GetClientRect(hwnd, &rect);
            FillRect(hdc, &rect, hBrushBg);
            
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, g_ColorText);
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFontTitle);
            
            RECT titleRect = { 15, 12, 500, 42 };
            DrawTextA(hdc, "Windows Token Generator", -1, &titleRect, DT_LEFT | DT_SINGLELINE);
            
            SelectObject(hdc, hFontSubtitle);
            SetTextColor(hdc, g_ColorTextMuted);
            RECT subRect = { 15, 42, 900, 62 };
            DrawTextA(hdc, "\xc5\xe4\xd6\xc3\xa1\xa2\xce\xb1\xd4\xec\xb2\xa2\xc0\xfb\xd3\xc3\xd7\xd4\xb6\xa8\xd2\xe5\xb0\xb2\xc8\xab\xc1\xee\xc5\xc6\xd4\xcb\xd0\xd0\xbd\xf8\xb3\xcc", -1, &subRect, DT_LEFT | DT_SINGLELINE);
            
            SelectObject(hdc, hOldFont);
            
            DrawCardFrame(hdc, 15, 75, 480, 90, "\xb0\xb2\xc8\xab\xd4\xa4\xc9\xe8\xd3\xeb\xc9\xed\xb7\xdd\xc9\xcf\xcf\xc2\xce\xc4");
            DrawCardFrame(hdc, 15, 175, 480, 145, "\xc4\xbf\xb1\xea\xbd\xf8\xb3\xcc\xd6\xb4\xd0\xd0");
            DrawCardFrame(hdc, 15, 330, 480, 145, "\xb0\xb2\xc8\xab\xc1\xee\xc5\xc6\xd7\xd4\xb6\xa8\xd2\xe5\xb4\xdb\xb8\xc4");
            DrawCardFrame(hdc, 15, 485, 480, 75, "\xbd\xf8\xb3\xcc\xc6\xf4\xb6\xaf\xbf\xd8\xd6\xc6");
            DrawCardFrame(hdc, 15, 570, 480, 170, "\xbf\xd8\xd6\xc6\xcc\xa8\xd4\xcb\xd0\xd0\xc8\xd5\xd6\xbe");
            
            //        
            DrawCardFrame(hdc, 510, 75, 480, 665, "\xc1\xee\xc5\xc6\xc9\xfa\xb3\xc9\xd0\xa7\xb9\xfb\xca\xb5\xca\xb1\xd4\xa4\xc0\xc0 (whoami /all)");
            
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_DRAWITEM: {
            DrawModernButton((LPDRAWITEMSTRUCT)lParam);
            break;
        }
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            HWND hCtrl = (HWND)lParam;
            char className[256];
            GetClassNameA(hCtrl, className, sizeof(className));
            if (_stricmp(className, "EDIT") == 0) {
                SetTextColor(hdc, g_ColorText);
                SetBkColor(hdc, g_ColorCtrlBg);
                return (LRESULT)hBrushCtrlBg;
            }
            SetTextColor(hdc, g_ColorTextMuted);
            SetBkColor(hdc, g_ColorCard);
            SetBkMode(hdc, TRANSPARENT);
            return (LRESULT)hBrushCard;
        }
        case WM_CTLCOLOREDIT: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, g_ColorText);
            SetBkColor(hdc, g_ColorCtrlBg);
            return (LRESULT)hBrushCtrlBg;
        }
        case WM_CTLCOLORLISTBOX: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, g_ColorText);
            SetBkColor(hdc, g_ColorCtrlBg);
            return (LRESULT)hBrushCtrlBg;
        }
        case WM_SETTINGCHANGE: {
            UpdateThemeColors();
            EnableImmersiveDarkMode(hwnd, g_bDarkMode);
            
            HWND hChild = GetWindow(hwnd, GW_CHILD);
            while (hChild) {
                ApplyThemeToControl(hChild);
                hChild = GetWindow(hChild, GW_HWNDNEXT);
            }
            
            SendMessageA(g_hLogEdit, EM_SETBKGNDCOLOR, 0, (LPARAM)(g_bDarkMode ? RGB(18, 18, 18) : RGB(249, 249, 249)));
            SendMessageA(g_hPreviewEdit, EM_SETBKGNDCOLOR, 0, (LPARAM)(g_bDarkMode ? RGB(18, 18, 18) : RGB(249, 249, 249)));
            
            UpdateTokenPreview(hwnd);
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        }
        case WM_APP + 100: {
            UpdateTokenPreview(hwnd);
            break;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            int code = HIWORD(wParam);
            
            HWND hComboPreset = GetDlgItem(hwnd, ID_COMBO_PRESET);
            HWND hComboUser = GetDlgItem(hwnd, ID_COMBO_USER);
            HWND hEditGroups = GetDlgItem(hwnd, ID_EDIT_GROUPS);
            HWND hEditPrivs = GetDlgItem(hwnd, ID_EDIT_PRIVS);
            
            if (id == ID_COMBO_PRESET && code == CBN_SELCHANGE) {
                int idx = SendMessageA(hComboPreset, CB_GETCURSEL, 0, 0);
                char text[256];
                SendMessageA(hComboPreset, CB_GETLBTEXT, idx, (LPARAM)text);
                Preset p = ResolvePreset(text);
                if (p.identityStr) {
                    SetWindowTextA(hComboUser, p.identityStr);
                    extraGroups = p.extraGroups;
                    DisabledPrivilege.clear(); 
                    RemovePrivilege = p.RemovePrivilege;
                    UpdateGuiSummaries(hEditGroups, hEditPrivs);
                    PostMessage(hwnd, WM_APP + 100, 0, 0);
                }
            }
            
            if (id == ID_COMBO_USER && (code == CBN_SELCHANGE || code == CBN_EDITCHANGE)) {
                PostMessage(hwnd, WM_APP + 100, 0, 0);
            }
            
            if (id == ID_COMBO_IL && code == CBN_SELCHANGE) {
                PostMessage(hwnd, WM_APP + 100, 0, 0);
            }
            
            switch(id) {
                case ID_BTN_EDIT_GRP_BTN: {
                    ShowGroupEditor(hwnd);
                    break;
                }
                case ID_BTN_EDIT_PRIV_BTN: {
                    ShowPrivilegeEditor(hwnd);
                    break;
                }
                case ID_BTN_CANCEL: PostQuitMessage(0); break;
                case ID_BTN_RUN: {
                    char bufCmd[2048] = { 0 }; char bufUser[256] = { 0 }; char bufDesk[256] = { 0 }; char bufDir[MAX_PATH] = { 0 };
                    GetWindowTextA(GetDlgItem(hwnd, ID_EDIT_CMD), bufCmd, 2047); 
                    GetWindowTextA(hComboUser, bufUser, 255); 
                    GetWindowTextA(GetDlgItem(hwnd, ID_EDIT_DESKTOP), bufDesk, 255); 
                    GetWindowTextA(GetDlgItem(hwnd, ID_EDIT_DIR), bufDir, MAX_PATH - 1);
                    
                    if (*bufCmd == 0) { MessageBoxA(hwnd, "\xc7\xeb\xca\xe4\xc8\xeb\xd3\xd0\xd0\xa7\xb5\xc4\xbf\xc9\xd6\xb4\xd0\xd0\xb3\xcc\xd0\xf2\xc3\xfc\xc1\xee\xc0\xb4\xd4\xcb\xd0\xd0\xa1\xa3", "\xb4\xed\xce\xf3", MB_OK | MB_ICONERROR); return 0; }
                    if (*bufUser == 0) strcpy(bufUser, "System");
        
                    int ilIdx = SendMessageA(GetDlgItem(hwnd, ID_COMBO_IL), CB_GETCURSEL, 0, 0);
                    DWORD dwIL = SECURITY_MANDATORY_SYSTEM_RID;
                    switch (ilIdx) {
                        case 1: dwIL = SECURITY_MANDATORY_HIGH_RID; break;
                        case 2: dwIL = SECURITY_MANDATORY_MEDIUM_PLUS_RID; break;
                        case 3: dwIL = SECURITY_MANDATORY_MEDIUM_RID; break;
                        case 4: dwIL = SECURITY_MANDATORY_LOW_RID; break;
                        case 5: dwIL = SECURITY_MANDATORY_UNTRUSTED_RID; break;
                    }
        
                    int modeIdx = SendMessageA(GetDlgItem(hwnd, ID_COMBO_MODE), CB_GETCURSEL, 0, 0);
                    int nShow = SW_SHOWNORMAL;
                    switch (modeIdx) {
                        case 1: nShow = SW_HIDE; break;
                        case 2: nShow = SW_SHOWMAXIMIZED; break;
                        case 3: nShow = SW_SHOWMINIMIZED; break;
                    }
        
                    BOOL bUIAccess = (SendMessageA(GetDlgItem(hwnd, ID_CHECK_UIACCESS), BM_GETCHECK, 0, 0) == BST_CHECKED);
                    g_bDebug = (SendMessageA(GetDlgItem(hwnd, ID_CHECK_DEBUG), BM_GETCHECK, 0, 0) == BST_CHECKED);
                    SendMessageA(g_hLogEdit, WM_SETTEXT, 0, (LPARAM)"");
                    Log(LOG_INFO, "=== \xbf\xaa\xca\xbc\xb9\xb9\xbd\xa8\xb0\xb2\xc8\xab\xc1\xee\xc5\xc6\xbd\xf8\xb3\xcc ===");
        
                    ExecuteSudoOperation(bufCmd, bufUser, bufDesk, bufDir, nShow, dwIL, bUIAccess, extraGroups, DisabledPrivilege, RemovePrivilege, NULL);
        
                    Log(LOG_INFO, "=== \xb0\xb2\xc8\xab\xc1\xee\xc5\xc6\xbd\xf8\xb3\xcc\xb9\xb9\xbd\xa8\xc1\xf7\xb3\xcc\xbd\xe1\xca\xf8 ===");
                    break;
                }
                case ID_BTN_CMD_BROWSE: {
                    OPENFILENAMEA ofn = {};
                    char path[MAX_PATH] = "";
            
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFile = path;
                    ofn.nMaxFile = MAX_PATH;
                    ofn.lpstrFilter = "\xbf\xc9\xd6\xb4\xd0\xd0\xb3\xcc\xd0\xf2 (*.exe)\0*.exe\0\xcb\xf9\xd3\xd0\xb3\xcc\xd0\xf2 (*.*)\0*.*\0";
                    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
            
                    if (GetOpenFileNameA(&ofn)) {
                        SetWindowTextA(GetDlgItem(hwnd, ID_EDIT_CMD), path);
                    }
                    break;
                }
                case ID_BTN_DIR_BROWSE: {
                    CoInitialize(NULL);
                    IFileDialog* dlg = nullptr;
                    if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dlg)))) {
                        DWORD opts;
                        dlg->GetOptions(&opts);
                        dlg->SetOptions(opts | FOS_PICKFOLDERS);
            
                        if (SUCCEEDED(dlg->Show(hwnd))) {
                            IShellItem* item;
                            if (SUCCEEDED(dlg->GetResult(&item))) {
                                PWSTR path;
                                if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                                    SetWindowTextW(GetDlgItem(hwnd, ID_EDIT_DIR), path);
                                    CoTaskMemFree(path);
                                }
                                item->Release();
                            }
                        }
                        dlg->Release();
                    }
                    CoUninitialize();
                    break;
                }
            }
            break;
        }
        case WM_CLOSE: PostQuitMessage(0); break;
        case WM_DESTROY: {
            CleanThemeResources();
            PostQuitMessage(0); 
            break;
        }
        default: return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
}

