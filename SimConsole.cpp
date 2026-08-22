#include "SimConsole.h"
#include "Globals.h"
#include "Utils.h"
#include "TokenEngine.h"
#include <string>

int GetDisplayWidth(const std::string& str) {
    int w = 0;
    for (size_t i = 0; i < str.length(); ) {
        unsigned char c = (unsigned char)str[i];
        if (c < 128) {
            w += 1;
            i += 1;
        } else {
            w += 2;
            i += 2;
        }
    }
    return w;
}

std::string PadRight(const std::string& str, int width) {
    int curW = GetDisplayWidth(str);
    int pad = width - curW;
    if (pad <= 0) return str;
    return str + std::string(pad, ' ');
}

void GetILInfo(DWORD il, std::string& name, std::string& sid) {
    switch (il) {
        case SECURITY_MANDATORY_UNTRUSTED_RID:
            name = "Mandatory Label\\Untrusted Mandatory Level";
            sid = "S-1-16-0";
            break;
        case SECURITY_MANDATORY_LOW_RID:
            name = "Mandatory Label\\Low Mandatory Level";
            sid = "S-1-16-4096";
            break;
        case SECURITY_MANDATORY_MEDIUM_RID:
            name = "Mandatory Label\\Medium Mandatory Level";
            sid = "S-1-16-8192";
            break;
        case SECURITY_MANDATORY_MEDIUM_PLUS_RID:
            name = "Mandatory Label\\Medium Plus Mandatory Level";
            sid = "S-1-16-8448";
            break;
        case SECURITY_MANDATORY_HIGH_RID:
            name = "Mandatory Label\\High Mandatory Level";
            sid = "S-1-16-12288";
            break;
        case SECURITY_MANDATORY_SYSTEM_RID:
        default:
            name = "Mandatory Label\\System Mandatory Level";
            sid = "S-1-16-16384";
            break;
    }
}

void SetRichEditDefaultColor(HWND hEdit, COLORREF color) {
    CHARFORMAT2 cf;
    ZeroMemory(&cf, sizeof(cf));
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.crTextColor = color;
    cf.dwEffects = 0;
    SendMessage(hEdit, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&cf);
}

void UpdateTokenPreview(HWND hwnd) {
    if (!g_hPreviewEdit || !IsWindow(g_hPreviewEdit)) return;

    char bufUser[256] = { 0 };
    GetWindowTextA(GetDlgItem(hwnd, ID_COMBO_USER), bufUser, 255);
    std::string userStr = bufUser;
    if (userStr.empty()) userStr = "System";

    PSID pUserSid = ResolveIdentity(userStr.c_str());
    std::string userSidStr = "S-1-5-18";
    if (pUserSid) {
        LPSTR sidStr = NULL;
        if (ConvertSidToStringSidA(pUserSid, &sidStr)) {
            userSidStr = sidStr;
            LocalFree(sidStr);
        }
        HeapFree(GetProcessHeap(), 0, pUserSid);
    }

    int ilIdx = SendMessageA(GetDlgItem(hwnd, ID_COMBO_IL), CB_GETCURSEL, 0, 0);
    DWORD dwIL = SECURITY_MANDATORY_SYSTEM_RID;
    switch (ilIdx) {
        case 1: dwIL = SECURITY_MANDATORY_HIGH_RID; break;
        case 2: dwIL = SECURITY_MANDATORY_MEDIUM_PLUS_RID; break;
        case 3: dwIL = SECURITY_MANDATORY_MEDIUM_RID; break;
        case 4: dwIL = SECURITY_MANDATORY_LOW_RID; break;
        case 5: dwIL = SECURITY_MANDATORY_UNTRUSTED_RID; break;
    }
    std::string ilName, ilSid;
    GetILInfo(dwIL, ilName, ilSid);

    std::stringstream ss;

    // ---      ---
    ss << "\xd3\xc3\xbb\xa7\xd0\xc5\xcf\xa2\x0d\x0a";
    ss << "----------------\x0d\x0a\x0d\x0a";
    ss << PadRight("\xd3\xc3\xbb\xa7\xc3\xfb", 25) << PadRight("SID", 20) << "\x0d\x0a";
    ss << PadRight("=========================", 25) << PadRight("====================", 20) << "\x0d\x0a";
    ss << PadRight(userStr, 25) << PadRight(userSidStr, 20) << "\x0d\x0a\x0d\x0a";

    // ---     ---
    ss << "\xd7\xe9\xd0\xc5\xcf\xa2\x0d\x0a";
    ss << "-----------------\x0d\x0a\x0d\x0a";
    ss << PadRight("\xd7\xe9\xc3\xfb", 40) << PadRight("\xc0\xe0\xd0\xcd", 10) << PadRight("SID", 15) << PadRight("\xca\xf4\xd0\xd4", 30) << "\x0d\x0a";
    ss << PadRight("========================================", 40)
       << PadRight("==========", 10)
       << PadRight("===============", 15)
       << PadRight("==============================", 30) << "\x0d\x0a";

    ss << PadRight(ilName, 40)
       << PadRight("\xb1\xea\xc7\xa9", 10)
       << PadRight(ilSid, 15)
       << PadRight("\xbd\xf6\xcf\xde\xc6\xf4\xd3\xc3\xb5\xc4\xd7\xe9, \xd7\xe9\xd3\xc3\xd3\xda\xcd\xea\xd5\xfb\xd0\xd4", 30) << "\x0d\x0a";

    ss << PadRight(userStr, 40)
       << PadRight("\xb1\xf0\xc3\xfb", 10)
       << PadRight(userSidStr, 15)
       << PadRight("\xb1\xd8\xd0\xe8\xb5\xc4\xd7\xe9, \xc6\xf4\xd3\xc3\xd3\xda\xc4\xac\xc8\xcf, \xc6\xf4\xd3\xc3\xb5\xc4\xd7\xe9, \xcb\xf9\xd3\xd0\xd5\xdf", 30) << "\x0d\x0a";

    ss << PadRight("BUILTIN\\Administrators", 40)
       << PadRight("\xb1\xf0\xc3\xfb", 10)
       << PadRight("S-1-5-32-544", 15)
       << PadRight("\xb1\xd8\xd0\xe8\xb5\xc4\xd7\xe9, \xc6\xf4\xd3\xc3\xd3\xda\xc4\xac\xc8\xcf, \xc6\xf4\xd3\xc3\xb5\xc4\xd7\xe9", 30) << "\x0d\x0a";

    ss << PadRight("NT AUTHORITY\\Authenticated Users", 40)
       << PadRight("\xd2\xd1\xd6\xaa\xd7\xe9", 10)
       << PadRight("S-1-5-11", 15)
       << PadRight("\xb1\xd8\xd0\xe8\xb5\xc4\xd7\xe9, \xc6\xf4\xd3\xc3\xd3\xda\xc4\xac\xc8\xcf, \xc6\xf4\xd3\xc3\xb5\xc4\xd7\xe9", 30) << "\x0d\x0a";

    ss << PadRight("Everyone", 40)
       << PadRight("\xd2\xd1\xd6\xaa\xd7\xe9", 10)
       << PadRight("S-1-1-0", 15)
       << PadRight("\xb1\xd8\xd0\xe8\xb5\xc4\xd7\xe9, \xc6\xf4\xd3\xc3\xd3\xda\xc4\xac\xc8\xcf, \xc6\xf4\xd3\xc3\xb5\xc4\xd7\xe9", 30) << "\x0d\x0a";

    for (const auto& grp : extraGroups) {
        PSID pSid = ResolveIdentity(grp.c_str());
        std::string grpSid = "S-1-5-Unknown";
        if (pSid) {
            LPSTR sidStr = NULL;
            if (ConvertSidToStringSidA(pSid, &sidStr)) {
                grpSid = sidStr;
                LocalFree(sidStr);
            }
            HeapFree(GetProcessHeap(), 0, pSid);
        }
        ss << PadRight(grp, 40)
           << PadRight("\xd2\xd1\xd6\xaa\xd7\xe9", 10)
           << PadRight(grpSid, 15)
           << PadRight("\xb1\xd8\xd0\xe8\xb5\xc4\xd7\xe9, \xc6\xf4\xd3\xc3\xd3\xda\xc4\xac\xc8\xcf, \xc6\xf4\xd3\xc3\xb5\xc4\xd7\xe9", 30) << "\x0d\x0a";
    }
    ss << "\x0d\x0a";

    // ---      ---
    ss << "\xcc\xd8\xc8\xa8\xd0\xc5\xcf\xa2\x0d\x0a";
    ss << "----------------------\x0d\x0a\x0d\x0a";
    ss << PadRight("\xcc\xd8\xc8\xa8\xc3\xfb", 42) << PadRight("\xc3\xe8\xca\xf6", 36) << PadRight("\xd7\xb4\xcc\xac", 8) << "\x0d\x0a";
    ss << PadRight("=========================================", 42)
       << PadRight("===================================", 36)
       << PadRight("======", 8) << "\x0d\x0a";

    int PrivilegesStatus[35] = {}; // 0: Enabled, 1: Disabled, 2: Removed
    for(int id : DisabledPrivilege) {
        if (id >= 1 && id <= 35) PrivilegesStatus[id - 1] = 1;
    }
    for(int id : RemovePrivilege) {
        if (id >= 1 && id <= 35) PrivilegesStatus[id - 1] = 2;
    }

    for (int i = 0; i < 35; i++) {
        if (PrivilegesStatus[i] == 2) {
            ss << PadRight(AllPrivileges[i], 42)
               << PadRight(g_PrivInfos[i].desc, 36)
               << PadRight("\xd2\xd1\xd2\xc6\xb3\xfd", 8) << "\x0d\x0a";
            continue;
        }

        const char* statusStr = "\xd2\xd1\xc6\xf4\xd3\xc3";
        if (PrivilegesStatus[i] == 1) statusStr = "\xd2\xd1\xbd\xfb\xd3\xc3";

        ss << PadRight(AllPrivileges[i], 42)
           << PadRight(g_PrivInfos[i].desc, 36)
           << PadRight(statusStr, 8) << "\x0d\x0a";
    }

    std::string previewText = ss.str();
    SetRichEditDefaultColor(g_hPreviewEdit, g_ColorText);
    SetWindowTextA(g_hPreviewEdit, previewText.c_str());
}

