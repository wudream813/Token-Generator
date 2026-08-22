#include "Utils.h"
#include "Globals.h"

void WriteToStdOut(const char* str) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE || hOut == NULL) return;
    DWORD written;
    if (!WriteConsoleA(hOut, str, strlen(str), &written, NULL)) {
        WriteFile(hOut, str, strlen(str), &written, NULL);
    }
}

void Log(LogLevel level, const char* format, ...) {
    if (g_hLogEdit && IsWindow(g_hLogEdit)) {
        if (!g_bDebug && level == LOG_DEBUG) return;
        std::string msg;
        COLORREF color;
        if (g_bDarkMode) {
            color = RGB(220, 220, 220);
            switch (level) {
            case LOG_ERROR:   msg = "[!] "; color = RGB(255, 65, 54); break;   //   
            case LOG_WARN:    msg = "[-] "; color = RGB(255, 133, 27); break;  //   
            case LOG_INFO:    msg = "[*] "; color = RGB(0, 116, 217); break;   //   
            case LOG_SUCCESS: msg = "[+] "; color = RGB(46, 204, 64); break;   //   
            case LOG_DEBUG:   msg = "[D] "; color = RGB(170, 170, 170); break; //   
            }
        } else {
            color = RGB(32, 32, 32);
            switch (level) {
            case LOG_ERROR:   msg = "[!] "; color = RGB(192, 0, 0); break;     //   
            case LOG_WARN:    msg = "[-] "; color = RGB(180, 80, 0); break;    //   
            case LOG_INFO:    msg = "[*] "; color = RGB(0, 90, 158); break;    //   
            case LOG_SUCCESS: msg = "[+] "; color = RGB(0, 128, 0); break;     //   
            case LOG_DEBUG:   msg = "[D] "; color = RGB(110, 110, 110); break; //   
            }
        }
        char buffer[2048];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        msg += buffer;
        msg += "\x0d\x0a";
        int len = GetWindowTextLengthA(g_hLogEdit);
        SendMessageA(g_hLogEdit, EM_SETSEL, (WPARAM)len, (LPARAM)len);
        CHARFORMAT2 cf;
        ZeroMemory(&cf, sizeof(cf));
        cf.cbSize = sizeof(cf);
        cf.dwMask = CFM_COLOR;
        cf.crTextColor = color;
        cf.dwEffects = 0;
        SendMessage(g_hLogEdit, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&cf);
        SendMessage(g_hLogEdit, EM_REPLACESEL, 0, (LPARAM)msg.c_str());
        SendMessage(g_hLogEdit, WM_VSCROLL, SB_BOTTOM, 0);
    }
    else {
        if (!g_bDebug && level != LOG_ERROR && level != LOG_WARN) return;
        const char* prefix = "";
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE && hOut != NULL) {
            CONSOLE_SCREEN_BUFFER_INFO csbi;
            WORD wOldColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
            if (GetConsoleScreenBufferInfo(hOut, &csbi)) {
                wOldColor = csbi.wAttributes;
            }
            WORD wColor = wOldColor;
            switch (level) {
            case LOG_ERROR:   prefix = "[!] "; wColor = FOREGROUND_RED | FOREGROUND_INTENSITY; break;
            case LOG_WARN:    prefix = "[-] "; wColor = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
            case LOG_INFO:    prefix = "[*] "; wColor = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
            case LOG_SUCCESS: prefix = "[+] "; wColor = FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
            case LOG_DEBUG:   prefix = "[D] "; wColor = FOREGROUND_INTENSITY; break;
            }
            SetConsoleTextAttribute(hOut, wColor);
            WriteToStdOut(prefix);
            char buffer[4096];
            va_list args;
            va_start(args, format);
            vsnprintf(buffer, sizeof(buffer), format, args);
            va_end(args);
            WriteToStdOut(buffer);
            WriteToStdOut("\x0d\x0a");
            SetConsoleTextAttribute(hOut, wOldColor);
        }
    }
}

void Log_ErrorCode(LogLevel level, DWORD ErrorCode) {
    CHAR MessageBuf[2048] = {};
    if (FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        ErrorCode,
        0,
        MessageBuf,
        2048,
        NULL
    )) {
        Log(level, "\xb4\xed\xce\xf3\xcf\xb5\xcd\xb3\xc3\xe8\xca\xf6\xa3\xba%s", MessageBuf);
    }
}

BOOL IsUserAnAdmin() {
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;
    BOOL b = AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup);
    if (b) {
        if (!CheckTokenMembership(NULL, AdministratorsGroup, &b)) b = FALSE;
        FreeSid(AdministratorsGroup);
    }
    return b;
}

LPSTR GetCurrentLpDesktop() {
    HDESK hDesk = GetThreadDesktop(GetCurrentThreadId());
    if (!hDesk) return NULL;
    HWINSTA hWinsta = GetProcessWindowStation();
    if (!hWinsta) return NULL;
    char desk[256], winsta[256];
    DWORD needed;
    if (!GetUserObjectInformationA(hDesk, UOI_NAME, desk, sizeof(desk), &needed)) return NULL;
    if (!GetUserObjectInformationA(hWinsta, UOI_NAME, winsta, sizeof(winsta), &needed)) return NULL;
    size_t len = strlen(winsta) + strlen(desk) + 2;
    LPSTR p = (LPSTR)LocalAlloc(LMEM_FIXED, len);
    if (!p) return NULL;
    _snprintf(p, len, "%s\\%s", winsta, desk);
    return p;
}

DWORD GetPidByNameA(LPCSTR processName) {
    DWORD pid = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe = {};
        pe.dwSize = sizeof(pe);
        if (Process32First(hSnapshot, &pe)) {
            do {
                if (_stricmp(pe.szExeFile, processName) == 0) {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnapshot, &pe));
        }
        CloseHandle(hSnapshot);
    }
    return pid;
}

PSID GetSidFromString(LPCSTR str) {
    PSID sid = NULL;
    if (ConvertStringSidToSidA(str, &sid)) {
        PSID dup = DupSid(sid);
        LocalFree(sid);
        return dup;
    }
    return NULL;
}

PSID GetSidForAccountName(LPCSTR accountName) {
    DWORD sidLen = 0, domainLen = 0;
    SID_NAME_USE use;
    LookupAccountNameA(NULL, accountName, NULL, &sidLen, NULL, &domainLen, &use);
    if (sidLen > 0) {
        PSID sid = (PSID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sidLen);
        char* domain = (char*)HeapAlloc(GetProcessHeap(), 0, domainLen);
        if (sid && domain && LookupAccountNameA(NULL, accountName, sid, &sidLen, domain, &domainLen, &use)) {
            HeapFree(GetProcessHeap(), 0, domain);
            return sid;
        }
        if (domain) HeapFree(GetProcessHeap(), 0, domain);
        if (sid) HeapFree(GetProcessHeap(), 0, sid);
    }
    return NULL;
}

PSID GetLogonSid() {
    HANDLE h;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &h)) return NULL;
    DWORD l = 0;
    GetTokenInformation(h, TokenGroups, 0, 0, &l);
    std::vector<BYTE> b(l);
    GetTokenInformation(h, TokenGroups, b.data(), l, &l);
    CloseHandle(h);
    PTOKEN_GROUPS g = (PTOKEN_GROUPS)b.data();
    for (DWORD i = 0; i < g->GroupCount; i++) {
        if ((g->Groups[i].Attributes & SE_GROUP_LOGON_ID) == SE_GROUP_LOGON_ID) {
            return DupSid(g->Groups[i].Sid);
        }
    }
    return NULL;
}

PSID DupSid(PSID src) {
    if (!src) return NULL;
    DWORD len = GetLengthSid(src);
    PSID dst = (PSID)HeapAlloc(GetProcessHeap(), 0, len);
    if (dst) CopySid(len, dst, src);
    return dst;
}

BOOL EnablePrivilege(HANDLE hToken, LPCSTR privilegeName) {
    HANDLE hTokenToUse = hToken;
    BOOL bMyToken = FALSE;
    if (hTokenToUse == NULL) {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hTokenToUse))
            return FALSE;
        bMyToken = TRUE;
    }
    LUID luid;
    if (!LookupPrivilegeValueA(NULL, privilegeName, &luid)) {
        if (bMyToken) CloseHandle(hTokenToUse);
        return FALSE;
    }
    TOKEN_PRIVILEGES tp = { 1, {{ luid, SE_PRIVILEGE_ENABLED }} };
    BOOL r = AdjustTokenPrivileges(hTokenToUse, FALSE, &tp, sizeof(tp), NULL, NULL);
    DWORD err = GetLastError();
    if (bMyToken) CloseHandle(hTokenToUse);
    if (r && (err == ERROR_SUCCESS)) return TRUE;
    else {
        Log(LOG_ERROR, "\xc6\xf4\xd3\xc3\xcc\xd8\xc8\xa8 %s \xca\xa7\xb0\xdc\xa3\xac\xb4\xed\xce\xf3\xb4\xfa\xc2\xeb\xa3\xba%lu", privilegeName, err);
        return FALSE;
    }
}

int ResolvePrivilegeId(const char* name) {
    if (_stricmp(name, "SeCreateTokenPrivilege") == 0 || strcmp(name, "1") == 0) return 1;
    if (_stricmp(name, "SeAssignPrimaryTokenPrivilege") == 0 || strcmp(name, "2") == 0) return 2;
    if (_stricmp(name, "SeLockMemoryPrivilege") == 0 || strcmp(name, "3") == 0) return 3;
    if (_stricmp(name, "SeIncreaseQuotaPrivilege") == 0 || strcmp(name, "4") == 0) return 4;
    if (_stricmp(name, "SeMachineAccountPrivilege") == 0 || strcmp(name, "5") == 0) return 5;
    if (_stricmp(name, "SeTcbPrivilege") == 0 || strcmp(name, "6") == 0) return 6;
    if (_stricmp(name, "SeSecurityPrivilege") == 0 || strcmp(name, "7") == 0) return 7;
    if (_stricmp(name, "SeTakeOwnershipPrivilege") == 0 || strcmp(name, "8") == 0) return 8;
    if (_stricmp(name, "SeLoadDriverPrivilege") == 0 || strcmp(name, "9") == 0) return 9;
    if (_stricmp(name, "SeSystemProfilePrivilege") == 0 || strcmp(name, "10") == 0) return 10;
    if (_stricmp(name, "SeSystemtimePrivilege") == 0 || strcmp(name, "11") == 0) return 11;
    if (_stricmp(name, "SeProfileSingleProcessPrivilege") == 0 || strcmp(name, "12") == 0) return 12;
    if (_stricmp(name, "SeIncreaseBasePriorityPrivilege") == 0 || strcmp(name, "13") == 0) return 13;
    if (_stricmp(name, "SeCreatePagefilePrivilege") == 0 || strcmp(name, "14") == 0) return 14;
    if (_stricmp(name, "SeCreatePermanentPrivilege") == 0 || strcmp(name, "15") == 0) return 15;
    if (_stricmp(name, "SeBackupPrivilege") == 0 || strcmp(name, "16") == 0) return 16;
    if (_stricmp(name, "SeRestorePrivilege") == 0 || strcmp(name, "17") == 0) return 17;
    if (_stricmp(name, "SeShutdownPrivilege") == 0 || strcmp(name, "18") == 0) return 18;
    if (_stricmp(name, "SeDebugPrivilege") == 0 || strcmp(name, "19") == 0) return 19;
    if (_stricmp(name, "SeAuditPrivilege") == 0 || strcmp(name, "20") == 0) return 20;
    if (_stricmp(name, "SeSystemEnvironmentPrivilege") == 0 || strcmp(name, "21") == 0) return 21;
    if (_stricmp(name, "SeChangeNotifyPrivilege") == 0 || strcmp(name, "22") == 0) return 22;
    if (_stricmp(name, "SeRemoteShutdownPrivilege") == 0 || strcmp(name, "23") == 0) return 23;
    if (_stricmp(name, "SeUndockPrivilege") == 0 || strcmp(name, "24") == 0) return 24;
    if (_stricmp(name, "SeSyncAgentPrivilege") == 0 || strcmp(name, "25") == 0) return 25;
    if (_stricmp(name, "SeEnableDelegationPrivilege") == 0 || strcmp(name, "26") == 0) return 26;
    if (_stricmp(name, "SeManageVolumePrivilege") == 0 || strcmp(name, "27") == 0) return 27;
    if (_stricmp(name, "SeImpersonatePrivilege") == 0 || strcmp(name, "28") == 0) return 28;
    if (_stricmp(name, "SeCreateGlobalPrivilege") == 0 || strcmp(name, "29") == 0) return 29;
    if (_stricmp(name, "SeTimeZonePrivilege") == 0 || strcmp(name, "30") == 0) return 30;
    if (_stricmp(name, "SeCreateSymbolicLinkPrivilege") == 0 || strcmp(name, "31") == 0) return 31;
    if (_stricmp(name, "SeRelabelPrivilege") == 0 || strcmp(name, "32") == 0) return 32;
    if (_stricmp(name, "SeIncreaseWorkingSetPrivilege") == 0 || strcmp(name, "33") == 0) return 33;
    if (_stricmp(name, "SeTrustedCredManAccessPrivilege") == 0 || strcmp(name, "34") == 0) return 34;
    if (_stricmp(name, "SeDelegateSessionUserImpersonatePrivilege") == 0 || strcmp(name, "35") == 0) return 35;
    return -1;
}

long ResolveILlevel(const char* level) {
    if (_stricmp(level, "untrusted") == 0 || _stricmp(level, "u") == 0) return SECURITY_MANDATORY_UNTRUSTED_RID;
    if (_stricmp(level, "low") == 0 || _stricmp(level, "l") == 0) return SECURITY_MANDATORY_LOW_RID;
    if (_stricmp(level, "medium") == 0 || _stricmp(level, "m") == 0) return SECURITY_MANDATORY_MEDIUM_RID;
    if (_stricmp(level, "medium+") == 0 || _stricmp(level, "m+") == 0) return SECURITY_MANDATORY_MEDIUM_PLUS_RID;
    if (_stricmp(level, "high") == 0 || _stricmp(level, "h") == 0) return SECURITY_MANDATORY_HIGH_RID;
    if (_stricmp(level, "system") == 0 || _stricmp(level, "s") == 0) return SECURITY_MANDATORY_SYSTEM_RID;
    return -1;
}

Preset ResolvePreset(const char* arg) {
    if (_stricmp(arg, "Normal") == 0) {
        return { (LPSTR)"NT AUTHORITY\\SYSTEM", {}, {} }; 
    }
    if (_stricmp(arg, "A") == 0 || _stricmp(arg, "Admin") == 0) {
        return { (LPSTR)"BUILTIN\\Administrators", {}, {} }; 
    }
    if (_stricmp(arg, "A+") == 0 || _stricmp(arg, "Admin+") == 0) {
        return { (LPSTR)"BUILTIN\\Administrators", {}, { 1, 3, 5, 6, 15, 20, 25, 26, 32, 34 } };
    }
    if (_stricmp(arg, "S") == 0 || _stricmp(arg, "System") == 0) {
        return { (LPSTR)"NT AUTHORITY\\SYSTEM", {}, {} };
    }
    if (_stricmp(arg, "S+") == 0 || _stricmp(arg, "System+") == 0) {
        return { (LPSTR)"NT AUTHORITY\\SYSTEM", {}, { 1, 3, 5, 10, 11, 14, 23, 25, 26, 30, 31, 32, 33, 35 } };
    }
    if (_stricmp(arg, "TI") == 0 || _stricmp(arg, "TrustedInstaller") == 0) {
        return { (LPSTR)"NT AUTHORITY\\SYSTEM", { "NT AUTHORITY\\SERVICE", "NT SERVICE\\TrustedInstaller" }, {} };
    }
    if (_stricmp(arg, "TI+") == 0 || _stricmp(arg, "TrustedInstaller+") == 0) {
        return { (LPSTR)"NT AUTHORITY\\SYSTEM", { "NT AUTHORITY\\SERVICE", "NT SERVICE\\TrustedInstaller" }, { 1, 5, 23, 25, 26, 32, 34 } };
    }
    if (_stricmp(arg, "NS") == 0 || _stricmp(arg, "NETWORK SERVICE") == 0) {
        return { (LPSTR)"NT AUTHORITY\\NETWORK SERVICE", { "NT AUTHORITY\\SERVICE" }, {} };
    }
    if (_stricmp(arg, "NS+") == 0 || _stricmp(arg, "NETWORK SERVICE+") == 0) {
        return { (LPSTR)"NT AUTHORITY\\NETWORK SERVICE", { "NT AUTHORITY\\SERVICE" }, 
            { 1, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 19, 21, 23, 25, 26, 27, 31, 32, 34, 35 } };
    }
    if (_stricmp(arg, "LS") == 0 || _stricmp(arg, "LOCAL SERVICE") == 0) {
        return { (LPSTR)"NT AUTHORITY\\LOCAL SERVICE", { "NT AUTHORITY\\SERVICE" }, {} };
    }
    if (_stricmp(arg, "LS+") == 0 || _stricmp(arg, "LOCAL SERVICE+") == 0) {
        return { (LPSTR)"NT AUTHORITY\\LOCAL SERVICE", { "NT AUTHORITY\\SERVICE" }, 
            { 1, 3, 5, 6, 7, 8, 9, 10, 12, 13, 14, 15, 16, 17, 19, 21, 23, 25, 26, 27, 31, 32, 34, 35 } };
    }
    if (_stricmp(arg, "DWM") == 0) {
        return { (LPSTR)"Window Manager\\DWM-1", { "S-1-5-4", "S-1-5-19", "S-1-5-90-0" }, {} };
    }
    if (_stricmp(arg, "DWM+") == 0) {
        return { (LPSTR)"Window Manager\\DWM-1", 
            { "NT AUTHORITY\\INTERACTIVE", "NT AUTHORITY\\LOCAL SERVICE", "Window Manager\\Window Manager Group" }, 
            { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 16, 17, 18, 19, 20, 21, 23, 24, 25, 26, 27, 28, 30, 31, 32, 34, 35 } };
    }
    return {NULL, {}, {}};
}

int ResolveWindowCreateMode(const char* arg) {
    if (_stricmp(arg, "I") == 0 || _stricmp(arg, "Inline") == 0) return -1;
    if (_stricmp(arg, "H") == 0 || _stricmp(arg, "Hide") == 0) return SW_HIDE;
    if (_stricmp(arg, "Max") == 0 || _stricmp(arg, "Maximize") == 0) return SW_SHOWMAXIMIZED;
    if (_stricmp(arg, "Min") == 0 || _stricmp(arg, "Minimize") == 0) return SW_SHOWMINIMIZED;
    return -2; //    
}

void TerminateParent(int parentPid, DWORD exitCode) {
    if (parentPid > 0) {
        HANDLE hParent = OpenProcess(PROCESS_TERMINATE, FALSE, (DWORD)parentPid);
        if (hParent) {
            TerminateProcess(hParent, exitCode);
            CloseHandle(hParent);
        }
    }
}

DWORD GetActiveSessionID() {
    DWORD count = 0;
    PWTS_SESSION_INFOA pSessionInfo = NULL;
    DWORD activeSessionId = (DWORD)-1;
    if (WTSEnumerateSessionsA(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &count)) {
        for (DWORD i = 0; i < count; ++i) {
            if (pSessionInfo[i].State == WTSActive) {
                activeSessionId = pSessionInfo[i].SessionId;
                break;
            }
        }
        WTSFreeMemory(pSessionInfo);
    }
    if (activeSessionId == (DWORD)-1)
        ProcessIdToSessionId(GetCurrentProcessId(), &activeSessionId);
    return activeSessionId;
}

