#include "UIEditor.h"
#include "Globals.h"
#include "Utils.h"
#include "ThemeEngine.h"

void UpdateGuiSummaries(HWND hEditGroups, HWND hEditPrivs) {
    if (!hEditGroups || !hEditPrivs) return;
    if (extraGroups.empty()) {
        SetWindowTextA(hEditGroups, "\xce\xde");
    } else if (extraGroups.size() == 1) {
        SetWindowTextA(hEditGroups, extraGroups[0].c_str());
    } else {
        std::string txt = extraGroups[0] + " (\xb5\xc8 +" + std::to_string(extraGroups.size() - 1) + " \xb8\xf6\xb8\xbd\xbc\xd3\xd7\xe9)";
        SetWindowTextA(hEditGroups, txt.c_str());
    }
    
    char summary[128];
    if (RemovePrivilege.empty() && DisabledPrivilege.empty()) {
        strcpy(summary, "\xbb\xd6\xb8\xb4\xc4\xac\xc8\xcf (\xc8\xab\xcc\xd8\xc8\xa8\xbc\xa4\xbb\xee)");
    } else {
        sprintf(summary, "%d \xd2\xc6\xb3\xfd, %d \xbd\xfb\xd3\xc3", (int)RemovePrivilege.size(), (int)DisabledPrivilege.size());
    }
    SetWindowTextA(hEditPrivs, summary);
}

void ShowGroupEditor(HWND hParent) {
    if (g_hGroupEditor && IsWindow(g_hGroupEditor)) { 
        SetForegroundWindow(g_hGroupEditor); 
        return; 
    }
    WNDCLASSEXA wc = {}; 
    wc.cbSize = sizeof(wc); 
    wc.lpfnWndProc = GroupEditorWndProc; 
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); 
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW); 
    wc.lpszClassName = "WinSudoGroupEdit";
    RegisterClassExA(&wc);
    
    EnableWindow(hParent, FALSE);
    g_hGroupEditor = CreateWindowExA(WS_EX_DLGMODALFRAME, "WinSudoGroupEdit", "\xb8\xbd\xbc\xd3\xd7\xe9\xb1\xe0\xbc\xad\xc6\xf7", WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 350, 480, hParent, NULL, GetModuleHandle(NULL), NULL);
    SetFocus(g_hGroupEditor);
}

void ShowPrivilegeEditor(HWND hParent) {
    if (g_hPrivEditor && IsWindow(g_hPrivEditor)) { 
        SetForegroundWindow(g_hPrivEditor); 
        return; 
    }
    WNDCLASSEXA wc = {}; 
    wc.cbSize = sizeof(wc); 
    wc.lpfnWndProc = PrivilegeEditorWndProc; 
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); 
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW); 
    wc.lpszClassName = "WinSudoPrivEdit";
    RegisterClassExA(&wc);
    
    EnableWindow(hParent, FALSE);
    g_hPrivEditor = CreateWindowExA(WS_EX_DLGMODALFRAME, "WinSudoPrivEdit", "\xcc\xd8\xc8\xa8\xc5\xe4\xd6\xc3\xb1\xe0\xbc\xad\xc6\xf7", WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 530, 480, hParent, NULL, GetModuleHandle(NULL), NULL);
    SetFocus(g_hPrivEditor);
}

void AddNewGroupItem(HWND hList) {
    int count = ListView_GetItemCount(hList);
    LVITEMA lvi = {};
    lvi.mask = LVIF_TEXT;
    lvi.iItem = count;
    lvi.pszText = (LPSTR)""; 
    int idx = ListView_InsertItem(hList, &lvi);
    ListView_EnsureVisible(hList, idx, FALSE);
    SetFocus(hList);
    ListView_EditLabel(hList, idx);
}

void DeleteSelectedGroup(HWND hList) {
    int iItem = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
    if (iItem != -1) ListView_DeleteItem(hList, iItem);
}

void EditSelectedGroup(HWND hList) {
    int iItem = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
    if (iItem != -1) { SetFocus(hList); ListView_EditLabel(hList, iItem); }
}

void UpdatePrivStatus(HWND hList, int iItem, const char* status) {
    ListView_SetItemText(hList, iItem, 1, (LPSTR)status);
}

void SetSelectedPrivs(HWND hList, const char* status) {
    int iPos = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
    while (iPos != -1) {
        UpdatePrivStatus(hList, iPos, status);
        iPos = ListView_GetNextItem(hList, iPos, LVNI_SELECTED);
    }
    SetFocus(hList);
}

LRESULT CALLBACK GroupEditorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hList;
    switch (msg) {
        case WM_CREATE: {
            HICON hIcon = LoadIconA(GetModuleHandle(NULL), "IDI_ICON1");
            if (hIcon) {
                SendMessageA(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
                SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            }
            EnableImmersiveDarkMode(hwnd, g_bDarkMode);
            HFONT hFont = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GB2312_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Microsoft YaHei");
            
            hList = CreateWindowExA(0, WC_LISTVIEW, "", WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_NOCOLUMNHEADER | LVS_EDITLABELS | LVS_SHOWSELALWAYS, 20, 70, 190, 360, hwnd, (HMENU)ID_LISTVIEW_GROUPS, GetModuleHandle(NULL), NULL);
            ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
            SendMessageA(hList, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            ApplyThemeToControl(hList);
            ListView_SetBkColor(hList, g_ColorBg);
            ListView_SetTextBkColor(hList, g_ColorBg);
            ListView_SetTextColor(hList, g_ColorText);
            
            LVCOLUMNA lvc = {}; lvc.mask = LVCF_WIDTH; lvc.cx = 180;
            ListView_InsertColumn(hList, 0, &lvc);
            
            for (size_t i = 0; i < extraGroups.size(); i++) {
                LVITEMA lvi = {}; lvi.mask = LVIF_TEXT; lvi.iItem = (int)i; lvi.pszText = (LPSTR)extraGroups[i].c_str();
                ListView_InsertItem(hList, &lvi);
            }
            
            HWND hBtnAdd = CreateWindow("BUTTON", "\xd0\xc2\xbd\xa8\xd7\xe9", WS_CHILD | WS_VISIBLE, 225, 70, 95, 30, hwnd, (HMENU)ID_BTN_ADD_GRP, NULL, NULL);
            SendMessageA(hBtnAdd, WM_SETFONT, (WPARAM)hFont, TRUE);
            MakeButtonModern(hBtnAdd);
            
            HWND hBtnEdit = CreateWindow("BUTTON", "\xb1\xe0\xbc\xad", WS_CHILD | WS_VISIBLE, 225, 110, 95, 30, hwnd, (HMENU)ID_BTN_EDIT_GRP, NULL, NULL);
            SendMessageA(hBtnEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
            MakeButtonModern(hBtnEdit);
            
            HWND hBtnDel = CreateWindow("BUTTON", "\xc9\xbe\xb3\xfd", WS_CHILD | WS_VISIBLE, 225, 150, 95, 30, hwnd, (HMENU)ID_BTN_DEL_GRP, NULL, NULL);
            SendMessageA(hBtnDel, WM_SETFONT, (WPARAM)hFont, TRUE);
            MakeButtonModern(hBtnDel);
            
            HWND hBtnSave = CreateWindow("BUTTON", "\xb1\xa3\xb4\xe6\xb2\xa2\xb9\xd8\xb1\xd5", WS_CHILD | WS_VISIBLE, 225, 400, 95, 30, hwnd, (HMENU)ID_BTN_RESET, NULL, NULL); 
            SendMessageA(hBtnSave, WM_SETFONT, (WPARAM)hFont, TRUE);
            MakeButtonModern(hBtnSave);
            
            SetFocus(hList);
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
            HFONT hFont = CreateFontA(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, GB2312_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Microsoft YaHei");
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
            
            RECT titleRect = { 15, 15, 300, 45 };
            DrawTextA(hdc, "\xb8\xbd\xbc\xd3\xd7\xe9\xb1\xe0\xbc\xad\xc6\xf7", -1, &titleRect, DT_LEFT | DT_SINGLELINE);
            
            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);
            
            DrawCardFrame(hdc, 15, 60, 200, 380, "");
            
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_DRAWITEM: {
            DrawModernButton((LPDRAWITEMSTRUCT)lParam);
            break;
        }
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, g_ColorTextMuted);
            SetBkColor(hdc, g_ColorBg);
            return (LRESULT)hBrushBg;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            switch (id) {
                case ID_BTN_ADD_GRP: AddNewGroupItem(hList); break;
                case ID_BTN_DEL_GRP: DeleteSelectedGroup(hList); break;
                case ID_BTN_EDIT_GRP: EditSelectedGroup(hList); break;
                case ID_BTN_RESET: {
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    break;
                }
            }
            break;
        }
        case WM_NOTIFY: {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            if (pnmh->hwndFrom == hList) {
                switch (pnmh->code) {
                case NM_DBLCLK: { LPNMITEMACTIVATE pia = (LPNMITEMACTIVATE)lParam; if (pia->iItem != -1) ListView_EditLabel(hList, pia->iItem); else AddNewGroupItem(hList); break; }
                case LVN_KEYDOWN: { LPNMLVKEYDOWN pnkd = (LPNMLVKEYDOWN)lParam; if (pnkd->wVKey == VK_DELETE) DeleteSelectedGroup(hList); break; }
                case LVN_ENDLABELEDIT: {
                    NMLVDISPINFOA* pdi = (NMLVDISPINFOA*)lParam;
                    if (pdi->item.pszText != NULL) {
                        if (strlen(pdi->item.pszText) == 0) { PostMessage(hList, LVM_DELETEITEM, pdi->item.iItem, 0); return FALSE; }
                        ListView_SetItemText(hList, pdi->item.iItem, 0, pdi->item.pszText); return TRUE;
                    }
                    else {
                        char buf[256] = { 0 }; ListView_GetItemText(hList, pdi->item.iItem, 0, buf, 255);
                        if (strlen(buf) == 0) PostMessage(hList, LVM_DELETEITEM, pdi->item.iItem, 0);
                    }
                    return FALSE;
                }
                }
            }
            break;
        }
        case WM_CLOSE: {
            extraGroups.clear();
            int count = ListView_GetItemCount(hList);
            char buf[256];
            for (int i = 0; i < count; i++) {
                ListView_GetItemText(hList, i, 0, buf, 255);
                if (strlen(buf) > 0) extraGroups.push_back(buf);
            }
            
            HWND hParent = GetWindow(hwnd, GW_OWNER);
            if (hParent && IsWindow(hParent)) {
                HWND hEditGrp = GetDlgItem(hParent, ID_EDIT_GROUPS);
                HWND hEditPriv = GetDlgItem(hParent, ID_EDIT_PRIVS);
                UpdateGuiSummaries(hEditGrp, hEditPriv);
                PostMessage(hParent, WM_APP + 100, 0, 0); 
            }
            DestroyWindow(hwnd);
            break;
        }
        case WM_DESTROY: {
            HWND hParent = GetWindow(hwnd, GW_OWNER);
            if (hParent && IsWindow(hParent)) {
                EnableWindow(hParent, TRUE);
                SetForegroundWindow(hParent);
            }
            g_hGroupEditor = NULL;
            break;
        }
        default: return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK PrivilegeEditorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hList;
    switch (msg) {
        case WM_CREATE: {
            HICON hIcon = LoadIconA(GetModuleHandle(NULL), "IDI_ICON1");
            if (hIcon) {
                SendMessageA(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
                SendMessageA(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            }
            EnableImmersiveDarkMode(hwnd, g_bDarkMode);
            HFONT hFont = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, GB2312_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Microsoft YaHei");
            
            hList = CreateWindowExA(0, WC_LISTVIEW, "", WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SHOWSELALWAYS, 20, 70, 360, 360, hwnd, (HMENU)ID_LISTVIEW_PRIVS, GetModuleHandle(NULL), NULL);
            ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
            SendMessageA(hList, WM_SETFONT, (WPARAM)hFont, TRUE);
            
            ApplyThemeToControl(hList);
            ListView_SetBkColor(hList, g_ColorBg);
            ListView_SetTextBkColor(hList, g_ColorBg);
            ListView_SetTextColor(hList, g_ColorText);
            
            LVCOLUMNA lvc = {};
            lvc.mask = LVCF_WIDTH | LVCF_TEXT;
            lvc.cx = 240; lvc.pszText = (LPSTR)"\xb0\xb2\xc8\xab\xcc\xd8\xc8\xa8\xb1\xea\xca\xb6\xc3\xfb\xb3\xc6";
            ListView_InsertColumn(hList, 0, &lvc);
            lvc.cx = 100; lvc.pszText = (LPSTR)"\xd7\xb4\xcc\xac";
            ListView_InsertColumn(hList, 1, &lvc);
            
            int PrivilegesStatus[35] = {}; // 0: Default, 1: Disabled, 2:Removed
            for(int id : DisabledPrivilege) PrivilegesStatus[id - 1] = 1;
            for(int id : RemovePrivilege) PrivilegesStatus[id - 1] = 2;
            
            for (int i = 0; i < 35; i++) {
                LVITEMA lvi = {};
                lvi.mask = LVIF_TEXT | LVIF_PARAM;
                lvi.iItem = i;
                lvi.pszText = (LPSTR)AllPrivileges[i];
                lvi.lParam = i + 1;
                ListView_InsertItem(hList, &lvi);
                
                const char* status = "\xd2\xd1\xc6\xf4\xd3\xc3 (\xc4\xac\xc8\xcf)";
                if(PrivilegesStatus[i] == 1) status = "\xd2\xd1\xbd\xfb\xd3\xc3";
                else if(PrivilegesStatus[i] == 2) status = "\xcd\xea\xc8\xab\xd2\xc6\xb3\xfd";
                ListView_SetItemText(hList, i, 1, (LPSTR)status);
            }
            
            int btnX = 395;
            HWND hBtnReset = CreateWindow("BUTTON", "\xbb\xd6\xb8\xb4\xc4\xac\xc8\xcf", WS_CHILD | WS_VISIBLE, btnX, 70, 100, 30, hwnd, (HMENU)ID_BTN_RESET, NULL, NULL);
            SendMessageA(hBtnReset, WM_SETFONT, (WPARAM)hFont, TRUE);
            MakeButtonModern(hBtnReset);
            
            HWND hBtnDisable = CreateWindow("BUTTON", "\xbd\xfb\xd3\xc3\xcc\xd8\xc8\xa8", WS_CHILD | WS_VISIBLE, btnX, 110, 100, 30, hwnd, (HMENU)ID_BTN_DISABLE, NULL, NULL);
            SendMessageA(hBtnDisable, WM_SETFONT, (WPARAM)hFont, TRUE);
            MakeButtonModern(hBtnDisable);
            
            HWND hBtnRemove = CreateWindow("BUTTON", "\xd2\xc6\xb3\xfd\xcc\xd8\xc8\xa8", WS_CHILD | WS_VISIBLE, btnX, 150, 100, 30, hwnd, (HMENU)ID_BTN_REMOVE, NULL, NULL);
            SendMessageA(hBtnRemove, WM_SETFONT, (WPARAM)hFont, TRUE);
            MakeButtonModern(hBtnRemove);
            
            HWND hBtnSave = CreateWindow("BUTTON", "\xb1\xa3\xb4\xe6\xb2\xa2\xb9\xd8\xb1\xd5", WS_CHILD | WS_VISIBLE, btnX, 400, 100, 30, hwnd, (HMENU)(ID_BTN_REMOVE + 10), NULL, NULL);
            SendMessageA(hBtnSave, WM_SETFONT, (WPARAM)hFont, TRUE);
            MakeButtonModern(hBtnSave);
            
            SetFocus(hList);
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
            HFONT hFont = CreateFontA(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, GB2312_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Microsoft YaHei");
            HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
            
            RECT titleRect = { 15, 15, 400, 45 };
            DrawTextA(hdc, "\xcc\xd8\xc8\xa8\xc5\xe4\xd6\xc3\xb1\xe0\xbc\xad\xc6\xf7", -1, &titleRect, DT_LEFT | DT_SINGLELINE);
            
            SelectObject(hdc, hOldFont);
            DeleteObject(hFont);
            
            DrawCardFrame(hdc, 15, 60, 370, 380, "");
            
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_DRAWITEM: {
            DrawModernButton((LPDRAWITEMSTRUCT)lParam);
            break;
        }
        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, g_ColorTextMuted);
            SetBkColor(hdc, g_ColorBg);
            return (LRESULT)hBrushBg;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            switch (id) {
                case ID_BTN_RESET: SetSelectedPrivs(hList, "\xd2\xd1\xc6\xf4\xd3\xc3 (\xc4\xac\xc8\xcf)"); break;
                case ID_BTN_DISABLE: SetSelectedPrivs(hList, "\xd2\xd1\xbd\xfb\xd3\xc3"); break;
                case ID_BTN_REMOVE: SetSelectedPrivs(hList, "\xcd\xea\xc8\xab\xd2\xc6\xb3\xfd"); break;
                case ID_BTN_REMOVE + 10: {
                    SendMessage(hwnd, WM_CLOSE, 0, 0);
                    break;
                }
            }
            break;
        }
        case WM_CLOSE: {
            RemovePrivilege.clear();
            DisabledPrivilege.clear();
            
            int count = ListView_GetItemCount(hList);
            char statusBuf[32];
            for (int i = 0; i < count; i++) {
                ListView_GetItemText(hList, i, 1, statusBuf, 31);
                LVITEMA lvi = {};
                lvi.iItem = i;
                lvi.mask = LVIF_PARAM;
                ListView_GetItem(hList, &lvi);
                int privId = (int)lvi.lParam;
                if (strcmp(statusBuf, "\xcd\xea\xc8\xab\xd2\xc6\xb3\xfd") == 0) RemovePrivilege.push_back(privId);
                else if (strcmp(statusBuf, "\xd2\xd1\xbd\xfb\xd3\xc3") == 0) DisabledPrivilege.push_back(privId);
            }
            
            HWND hParent = GetWindow(hwnd, GW_OWNER);
            if (hParent && IsWindow(hParent)) {
                HWND hEditGrp = GetDlgItem(hParent, ID_EDIT_GROUPS);
                HWND hEditPriv = GetDlgItem(hParent, ID_EDIT_PRIVS);
                UpdateGuiSummaries(hEditGrp, hEditPriv);
                PostMessage(hParent, WM_APP + 100, 0, 0); 
            }
            DestroyWindow(hwnd);
            break;
        }
        case WM_DESTROY: {
            HWND hParent = GetWindow(hwnd, GW_OWNER);
            if (hParent && IsWindow(hParent)) {
                EnableWindow(hParent, TRUE);
                SetForegroundWindow(hParent);
            }
            g_hPrivEditor = NULL;
            break;
        }
        default: return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

