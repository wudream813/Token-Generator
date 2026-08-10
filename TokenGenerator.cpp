/*
compileoptions : -Ofast -Wall -Wextra -fexec-charset=GBK -static -lwtsapi32 -luserenv -lntdll -ladvapi32 -lgdi32 -lcomctl32 -lcomdlg32 -luuid -lole32
*/

#undef UNICODE

#include <windows.h>
#include <wtsapi32.h>
#include <userenv.h>
#include <tlhelp32.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <vector>
#include <sstream>
#include <sddl.h>
#include <richedit.h>
#include <ntdef.h>
#include <shobjidl.h>
#include <winternl.h>

// ==========================================
// 1.      ID     
// ==========================================
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

// ==========================================
// 2.               
// ==========================================
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

typedef NTSTATUS(NTAPI* PNtCreateToken)(
    PHANDLE, ACCESS_MASK, PVOID, TOKEN_TYPE, PLUID, PLARGE_INTEGER,
    PTOKEN_USER, PTOKEN_GROUPS, PTOKEN_PRIVILEGES, PTOKEN_OWNER,
    PTOKEN_PRIMARY_GROUP, PTOKEN_DEFAULT_DACL, PVOID);

typedef HRESULT(WINAPI* PDwmSetWindowAttribute)(HWND, DWORD, LPCVOID, DWORD);
typedef HRESULT(WINAPI* PSetWindowTheme)(HWND, LPCWSTR, LPCWSTR);

enum LogLevel { LOG_ERROR, LOG_WARN, LOG_INFO, LOG_SUCCESS, LOG_DEBUG };

struct Preset {
    LPSTR identityStr;
    std::vector<std::string> extraGroups;
    std::vector<int> RemovePrivilege;
};

struct PrivInfo {
    const char* name;
    const char* desc;
};

// ==========================================
// 3.              
// ==========================================
#define COLOR_BG_DARK            RGB(24, 24, 24)       //     
#define COLOR_CARD_DARK          RGB(32, 32, 32)       //     
#define COLOR_CTRL_BG_DARK       RGB(45, 45, 45)       //        
#define COLOR_TEXT_DARK          RGB(240, 240, 240)    //       
#define COLOR_TEXT_MUTED_DARK    RGB(160, 160, 160)    //     
#define COLOR_ACCENT_DARK        RGB(0, 120, 215)      //      (WinUI 3   )
#define COLOR_ACCENT_HOVER_DARK  RGB(20, 140, 235)     //    
#define COLOR_ACCENT_PUSH_DARK   RGB(0, 100, 180)      //    
#define COLOR_SECONDARY_DARK     RGB(60, 60, 60)       //     
#define COLOR_SEC_HOVER_DARK     RGB(80, 80, 80)       //     
#define COLOR_SEC_PUSH_DARK      RGB(45, 45, 45)       //     
#define COLOR_BORDER_DARK        RGB(70, 70, 70)       //     

#define COLOR_BG_LIGHT           RGB(243, 243, 243)    //     
#define COLOR_CARD_LIGHT         RGB(255, 255, 255)    //     
#define COLOR_CTRL_BG_LIGHT      RGB(255, 255, 255)    //        
#define COLOR_TEXT_LIGHT         RGB(32, 32, 32)       //      
#define COLOR_TEXT_MUTED_LIGHT   RGB(110, 110, 110)    //     
#define COLOR_ACCENT_LIGHT       RGB(0, 90, 158)       //     
#define COLOR_ACCENT_HOVER_LIGHT RGB(16, 110, 190)     //    
#define COLOR_ACCENT_PUSH_LIGHT  RGB(0, 74, 127)       //    
#define COLOR_SECONDARY_LIGHT    RGB(225, 229, 235)    //      
#define COLOR_SEC_HOVER_LIGHT    RGB(210, 215, 222)    //     
#define COLOR_SEC_PUSH_LIGHT     RGB(190, 195, 202)    //     
#define COLOR_BORDER_LIGHT       RGB(210, 210, 210)    //     

COLORREF g_ColorBg = 0;
COLORREF g_ColorCard = 0;
COLORREF g_ColorCtrlBg = 0;
COLORREF g_ColorText = 0;
COLORREF g_ColorTextMuted = 0;
COLORREF g_ColorAccent = 0;
COLORREF g_ColorAccentHover = 0;
COLORREF g_ColorAccentPush = 0;
COLORREF g_ColorSecondary = 0;
COLORREF g_ColorSecHover = 0;
COLORREF g_ColorSecPush = 0;
COLORREF g_ColorBorder = 0;

BOOL g_bDarkMode = FALSE;

// ==========================================
// 4.       
// ==========================================
bool g_bDebug = false, g_bUIAccess = false;
std::vector<std::string> extraGroups;
std::vector<int> RemovePrivilege, DisabledPrivilege;
int g_WindowCreateMode = SW_SHOWNORMAL;
DWORD Integrity_Level = SECURITY_MANDATORY_SYSTEM_RID;
HWND g_hLogEdit = NULL, g_hGroupEditor = NULL, g_hPrivEditor = NULL, g_hPreviewEdit = NULL; //        
LPSTR g_desktop = NULL, g_runCommand = (LPSTR)"cmd.exe", g_identityStr = (LPSTR)"NT AUTHORITY\\SYSTEM";

//          
HBRUSH hBrushBg = NULL;
HBRUSH hBrushCard = NULL;
HBRUSH hBrushCtrlBg = NULL;
HFONT hFontTitle = NULL;
HFONT hFontSubtitle = NULL;
HFONT hFontNormal = NULL;
HFONT hFontLog = NULL;

const char* AllPrivileges[] = {
    "SeCreateTokenPrivilege", "SeAssignPrimaryTokenPrivilege", "SeLockMemoryPrivilege",
    "SeIncreaseQuotaPrivilege", "SeMachineAccountPrivilege", "SeTcbPrivilege",
    "SeSecurityPrivilege", "SeTakeOwnershipPrivilege", "SeLoadDriverPrivilege",
    "SeSystemProfilePrivilege", "SeSystemtimePrivilege", "SeProfileSingleProcessPrivilege",
    "SeIncreaseBasePriorityPrivilege", "SeCreatePagefilePrivilege", "SeCreatePermanentPrivilege",
    "SeBackupPrivilege", "SeRestorePrivilege", "SeShutdownPrivilege", "SeDebugPrivilege",
    "SeAuditPrivilege", "SeSystemEnvironmentPrivilege", "SeChangeNotifyPrivilege",
    "SeRemoteShutdownPrivilege", "SeUndockPrivilege", "SeSyncAgentPrivilege",
    "SeEnableDelegationPrivilege", "SeManageVolumePrivilege", "SeImpersonatePrivilege",
    "SeCreateGlobalPrivilege", "SeTimeZonePrivilege", "SeCreateSymbolicLinkPrivilege",
    "SeRelabelPrivilege", "SeIncreaseWorkingSetPrivilege", "SeTrustedCredManAccessPrivilege",
    "SeDelegateSessionUserImpersonatePrivilege"
};

PrivInfo g_PrivInfos[35] = {
    { "SeCreateTokenPrivilege", "\xb4\xb4\xbd\xa8\xd2\xbb\xb8\xf6\xc1\xee\xc5\xc6\xb6\xd4\xcf\xf3" },
    { "SeAssignPrimaryTokenPrivilege", "\xcc\xe6\xbb\xbb\xd2\xbb\xb8\xf6\xbd\xf8\xb3\xcc\xbc\xb6\xc1\xee\xc5\xc6" },
    { "SeLockMemoryPrivilege", "\xcb\xf8\xb6\xa8\xc4\xda\xb4\xe6\xd2\xb3" },
    { "SeIncreaseQuotaPrivilege", "\xce\xaa\xbd\xf8\xb3\xcc\xb5\xf7\xd5\xfb\xc4\xda\xb4\xe6\xc5\xe4\xb6\xee" },
    { "SeMachineAccountPrivilege", "\xbd\xab\xb9\xa4\xd7\xf7\xd5\xbe\xcc\xed\xbc\xd3\xb5\xbd\xd3\xf2" },
    { "SeTcbPrivilege", "\xd2\xd4\xb2\xd9\xd7\xf7\xcf\xb5\xcd\xb3\xb7\xbd\xca\xbd\xd6\xb4\xd0\xd0" },
    { "SeSecurityPrivilege", "\xb9\xdc\xc0\xed\xc9\xf3\xba\xcb\xba\xcd\xb0\xb2\xc8\xab\xc8\xd5\xd6\xbe" },
    { "SeTakeOwnershipPrivilege", "\xc8\xa1\xb5\xc3\xce\xc4\xbc\xfe\xbb\xf2\xc6\xe4\xcb\xfb\xb6\xd4\xcf\xf3\xb5\xc4\xcb\xf9\xd3\xd0\xc8\xa8" },
    { "SeLoadDriverPrivilege", "\xbc\xd3\xd4\xd8\xba\xcd\xd0\xb6\xd4\xd8\xc9\xe8\xb1\xb8\xc7\xfd\xb6\xaf\xb3\xcc\xd0\xf2" },
    { "SeSystemProfilePrivilege", "\xc5\xe4\xd6\xc3\xce\xc4\xbc\xfe\xcf\xb5\xcd\xb3\xd0\xd4\xc4\xdc" },
    { "SeSystemtimePrivilege", "\xb8\xfc\xb8\xc4\xcf\xb5\xcd\xb3\xca\xb1\xbc\xe4" },
    { "SeProfileSingleProcessPrivilege", "\xc5\xe4\xd6\xc3\xce\xc4\xbc\xfe\xb5\xa5\xd2\xbb\xbd\xf8\xb3\xcc" },
    { "SeIncreaseBasePriorityPrivilege", "\xcc\xe1\xb8\xdf\xbc\xc6\xbb\xae\xd3\xc5\xcf\xc8\xbc\xb6" },
    { "SeCreatePagefilePrivilege", "\xb4\xb4\xbd\xa8\xd2\xbb\xb8\xf6\xd2\xb3\xc3\xe6\xce\xc4\xbc\xfe" },
    { "SeCreatePermanentPrivilege", "\xb4\xb4\xbd\xa8\xd3\xc0\xbe\xc3\xb9\xb2\xcf\xed\xb6\xd4\xcf\xf3" },
    { "SeBackupPrivilege", "\xb1\xb8\xb7\xdd\xce\xc4\xbc\xfe\xba\xcd\xc4\xbf\xc2\xbc" },
    { "SeRestorePrivilege", "\xbb\xb9\xd4\xad\xce\xc4\xbc\xfe\xba\xcd\xc4\xbf\xc2\xbc" },
    { "SeShutdownPrivilege", "\xb9\xd8\xb1\xd5\xcf\xb5\xcd\xb3" },
    { "SeDebugPrivilege", "\xb5\xf7\xca\xd4\xb3\xcc\xd0\xf2" },
    { "SeAuditPrivilege", "\xc9\xfa\xb3\xc9\xb0\xb2\xc8\xab\xc9\xf3\xba\xcb" },
    { "SeSystemEnvironmentPrivilege", "\xd0\xde\xb8\xc4\xb9\xcc\xbc\xfe\xbb\xb7\xbe\xb3\xd6\xb5" },
    { "SeChangeNotifyPrivilege", "\xc8\xc6\xb9\xfd\xb1\xe9\xc0\xfa\xbc\xec\xb2\xe9" },
    { "SeRemoteShutdownPrivilege", "\xb4\xd3\xd4\xb6\xb3\xcc\xcf\xb5\xcd\xb3\xc7\xbf\xd6\xc6\xb9\xd8\xbb\xfa" },
    { "SeUndockPrivilege", "\xb4\xd3\xc0\xa9\xd5\xb9\xce\xeb\xc9\xcf\xc8\xa1\xcf\xc2\xbc\xc6\xcb\xe3\xbb\xfa" },
    { "SeSyncAgentPrivilege", "\xcd\xac\xb2\xbd\xc4\xbf\xc2\xbc\xb7\xfe\xce\xf1\xca\xfd\xbe\xdd" },
    { "SeEnableDelegationPrivilege", "\xd0\xc5\xc8\xce\xbc\xc6\xcb\xe3\xbb\xfa\xba\xcd\xd3\xc3\xbb\xa7\xd5\xcb\xbb\xa7\xbf\xc9\xd2\xd4\xd6\xb4\xd0\xd0\xce\xaf\xc5\xc9" },
    { "SeManageVolumePrivilege", "\xd6\xb4\xd0\xd0\xbe\xed\xce\xac\xbb\xa4\xc8\xce\xce\xf1" },
    { "SeImpersonatePrivilege", "\xc9\xed\xb7\xdd\xd1\xe9\xd6\xa4\xba\xf3\xc4\xa3\xc4\xe2\xbf\xcd\xbb\xa7\xb6\xcb" },
    { "SeCreateGlobalPrivilege", "\xb4\xb4\xbd\xa8\xc8\xab\xbe\xd6\xb6\xd4\xcf\xf3" },
    { "SeTimeZonePrivilege", "\xb8\xfc\xb8\xc4\xca\xb1\xc7\xf8" },
    { "SeCreateSymbolicLinkPrivilege", "\xb4\xb4\xbd\xa8\xb7\xfb\xba\xc5\xc1\xb4\xbd\xd3" },
    { "SeRelabelPrivilege", "\xd0\xde\xb8\xc4\xd2\xbb\xb8\xf6\xb6\xd4\xcf\xf3\xb1\xea\xc7\xa9" },
    { "SeIncreaseWorkingSetPrivilege", "\xd4\xf6\xbc\xd3\xbd\xf8\xb3\xcc\xb9\xa4\xd7\xf7\xbc\xaf" },
    { "SeTrustedCredManAccessPrivilege", "\xd7\xf7\xce\xaa\xca\xdc\xd0\xc5\xc8\xce\xb5\xc4\xba\xf4\xbd\xd0\xb7\xbd\xb7\xc3\xce\xca\xc6\xbe\xbe\xdd\xb9\xdc\xc0\xed\xc6\xf7" },
    { "SeDelegateSessionUserImpersonatePrivilege", "\xbb\xf1\xc8\xa1\xcd\xac\xd2\xbb\xbb\xe1\xbb\xb0\xd6\xd0\xc1\xed\xd2\xbb\xb8\xf6\xd3\xc3\xbb\xa7\xb5\xc4\xc4\xa3\xc4\xe2\xc1\xee\xc5\xc6" }
};

// ==========================================
// 5.        (Forward Declarations)
// ==========================================
void WriteToStdOut(const char* str);
void ShowUsage_Detailed(const char* prog);
void ShowUsage_Brief(const char* prog);
void UpdateThemeColors();
void InitializeThemeResources();
void CleanThemeResources();
void EnableImmersiveDarkMode(HWND hwnd, BOOL bEnable);
void ApplyThemeToControl(HWND hwnd);
void MakeButtonModern(HWND hBtn);
void DrawModernButton(LPDRAWITEMSTRUCT pdis);
void DrawCardFrame(HDC hdc, int x, int y, int w, int h, const char* title);
void UpdateGuiSummaries(HWND hEditGroups, HWND hEditPrivs);
void UpdateTokenPreview(HWND hwnd);
void ShowGroupEditor(HWND hParent);
void ShowPrivilegeEditor(HWND hParent);
void AddNewGroupItem(HWND hList);
void DeleteSelectedGroup(HWND hList);
void EditSelectedGroup(HWND hList);
BOOL IsSystemInDarkMode();
PSID ResolveIdentity(const char* identityStr);
void Log(LogLevel level, const char* format, ...);
void Log_ErrorCode(LogLevel level, DWORD ErrorCode);
BOOL IsUserAnAdmin();
LPSTR GetCurrentLpDesktop();
DWORD GetPidByNameA(LPCSTR processName);
PSID GetSidFromString(LPCSTR str);
PSID GetSidForAccountName(LPCSTR accountName);
PSID GetLogonSid();
PSID DupSid(PSID src);
BOOL EnablePrivilege(HANDLE hToken, LPCSTR privilegeName);
int ResolvePrivilegeId(const char* name);
long ResolveILlevel(const char* level);
Preset ResolvePreset(const char* arg);
int ResolveWindowCreateMode(const char* arg);
void TerminateParent(int parentPid, DWORD exitCode);
HANDLE GetLsassToken();
HANDLE CreateCustomToken(DWORD targetSessionId, PSID pUserSid, const std::vector<std::string>& extraGroups);
BOOL ExecuteSudoOperation(LPSTR cmdLine, LPSTR identityStr, LPSTR desktop, const std::string& workingDir, int windowMode, DWORD integrityLevel, BOOL bUIAccess, const std::vector<std::string>& extraGroups, const std::vector<int>& DisabledPrivilege, const std::vector<int>& RemovePrivilege, PROCESS_INFORMATION* pOutPI);
int GetDisplayWidth(const std::string& str);
std::string PadRight(const std::string& str, int width);
void GetILInfo(DWORD il, std::string& name, std::string& sid);
void SetRichEditDefaultColor(HWND hEdit, COLORREF color);
void UpdatePrivStatus(HWND hList, int iItem, const char* status);
void SetSelectedPrivs(HWND hList, const char* status);
void DeleteDisabledPrivileges(HANDLE& hToken);
DWORD GetActiveSessionID();
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK GroupEditorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK PrivilegeEditorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ==========================================
// 5.5.         
// ==========================================
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

void DeleteDisabledPrivileges(HANDLE& hToken) {
    DWORD len = 0;
    GetTokenInformation(hToken, TokenPrivileges, nullptr, 0, &len);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return;
    PTOKEN_PRIVILEGES tp = (PTOKEN_PRIVILEGES)malloc(len);
    if (!tp) return;
    if (!GetTokenInformation(hToken, TokenPrivileges, tp, len, &len)) {
        free(tp);
        return;
    }
    std::vector<LUID_AND_ATTRIBUTES> toRemove;
    toRemove.reserve(tp->PrivilegeCount);
    for (DWORD i = 0; i < tp->PrivilegeCount; ++i) {
        const auto& p = tp->Privileges[i];
        if ((p.Attributes & SE_PRIVILEGE_ENABLED) == 0) {
            toRemove.push_back({ p.Luid, 0 });
        }
    }
    free(tp);
    if (toRemove.empty()) return;
    HANDLE hNewToken = nullptr;
    if (!CreateRestrictedToken(hToken, 0, 0, nullptr, (DWORD)toRemove.size(), toRemove.data(), 0, nullptr, &hNewToken)) return;
    CloseHandle(hToken);
    hToken = hNewToken;
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

void ShowUsage_Brief(const char* prog) {
    printf("\x0a");
    printf("Token-Generator v1.0.0 (\xb8\xdf\xbc\xb6\xb0\xb2\xc8\xab\xc1\xee\xc5\xc6\xb4\xdb\xb8\xc4\xd3\xeb\xbd\xf8\xb3\xcc\xc6\xf4\xb6\xaf\xc6\xf7)\x0a");
    printf("====================================================\x0a\x0a");
    printf("\xd3\xc3\xb7\xa8: %s [\xd1\xa1\xcf\xee] [\xd6\xb4\xd0\xd0\xc3\xfc\xc1\xee (\xc4\xac\xc8\xcf: cmd.exe)]\x0a\x0a", prog);
    printf("\xd4\xa4\xc9\xe8\xd1\xa1\xcf\xee:\x0a");
    printf("  -Use:<\xc3\xfb\xb3\xc6>      \xd1\xa1\xd4\xf1\xb0\xb2\xc8\xab\xd6\xf7\xcc\xe5\xd4\xa4\xc9\xe8 (\xd6\xa7\xb3\xd6 S/S+, A/A+, TI/TI+, LS/LS+, NS/NS+, DWM/DWM+)\x0a");
    printf("\xc9\xed\xb7\xdd\xd1\xa1\xcf\xee:\x0a");
    printf("  -U:<\xd3\xc3\xbb\xa7\xc3\xfb|SID>   \xca\xd6\xb6\xaf\xd6\xb8\xb6\xa8\xd4\xcb\xd0\xd0\xc4\xbf\xb1\xea\xd3\xc3\xbb\xa7\xc9\xcf\xcf\xc2\xce\xc4\xc9\xed\xb7\xdd (\xc4\xac\xc8\xcf: System)\x0a");
    printf("\xc1\xee\xc5\xc6\xb4\xdb\xb8\xc4\xd1\xa1\xcf\xee:\x0a");
    printf("  -G:<\xb8\xbd\xbc\xd3\xd3\xc3\xbb\xa7\xd7\xe9>   \xd7\xa2\xc8\xeb\xb6\xee\xcd\xe2\xb5\xc4\xb0\xb2\xc8\xab\xd7\xe9\xbb\xf2 SID \xb5\xbd\xc9\xfa\xb3\xc9\xb5\xc4\xc1\xee\xc5\xc6\xd6\xd0\x0a");
    printf("  --UIAccess        \xce\xaa\xc9\xfa\xb3\xc9\xb5\xc4\xc1\xee\xc5\xc6\xbf\xaa\xc6\xf4 UI Access \xcc\xd8\xc8\xa8\xb1\xea\xd6\xbe\x0a");
    printf("  -IL:<\xcd\xea\xd5\xfb\xd0\xd4\xbc\xb6\xb1\xf0>  \xc9\xe8\xd6\xc3\xc1\xee\xc5\xc6\xc7\xbf\xd6\xc6\xcd\xea\xd5\xfb\xd0\xd4 (Untrusted, Low, Medium, Medium+, High, System)\x0a");
    printf("\xcc\xd8\xc8\xa8\xd1\xa1\xcf\xee:\x0a");
    printf("  -Remove:<\xcc\xd8\xc8\xa8>    \xb4\xd3\xc9\xfa\xb3\xc9\xb5\xc4\xc1\xee\xc5\xc6\xd6\xd0\xb3\xb9\xb5\xd7\xc7\xbf\xd6\xc6\xd2\xc6\xb3\xfd\xd6\xb8\xb6\xa8\xcc\xd8\xc8\xa8\x0a");
    printf("  -Disabled:<\xcc\xd8\xc8\xa8>   \xc7\xbf\xd6\xc6\xbd\xab\xd6\xb8\xb6\xa8\xcc\xd8\xc8\xa8\xd6\xc3\xce\xaa\xbd\xfb\xd3\xc3\xd7\xb4\xcc\xac\x0a");
    printf("\xd4\xcb\xd0\xd0\xbf\xd8\xd6\xc6\xd1\xa1\xcf\xee:\x0a");
    printf("  --Debug           \xbf\xaa\xc6\xf4\xb5\xf7\xca\xd4\xc8\xd5\xd6\xbe\xa3\xac\xca\xe4\xb3\xf6\xcf\xea\xcf\xb8\xb5\xc4\xc1\xee\xc5\xc6\xce\xb1\xd4\xec\xd0\xd0\xce\xaa\x0a");
    printf("  -GUI              \xc6\xf4\xb6\xaf\xbe\xab\xd0\xc4\xc9\xe8\xbc\xc6\xb5\xc4 Windows 11 Fluent \xb7\xe7\xb8\xf1\xb8\xdf\xbc\xb6\xbd\xe7\xc3\xe6\x0a");
    printf("  -d:<\xd7\xc0\xc3\xe6>         \xd6\xb8\xb6\xa8\xc6\xf4\xb6\xaf\xb5\xc4 Desktop \xb0\xb2\xc8\xab\xd7\xc0\xc3\xe6 (\xc4\xac\xc8\xcf: \xb5\xb1\xc7\xb0\xbb\xee\xb6\xaf\xd7\xc0\xc3\xe6)\x0a");
    printf("  -C:<\xc2\xb7\xbe\xb6>         \xd6\xb8\xb6\xa8\xbd\xf8\xb3\xcc\xc6\xf4\xb6\xaf\xb5\xc4\xb3\xf5\xca\xbc\xb9\xa4\xd7\xf7\xc4\xbf\xc2\xbc\x0a");
    printf("  -M:<\xcf\xd4\xca\xbe\xc4\xa3\xca\xbd>     \xc9\xe8\xd6\xc3\xc6\xf4\xb6\xaf\xb4\xb0\xbf\xda\xd7\xb4\xcc\xac: Inline (\xc4\xda\xc1\xaa\xb5\xb1\xc7\xb0\xbf\xd8\xd6\xc6\xcc\xa8), Hide (\xd2\xfe\xb2\xd8), Max (\xd7\xee\xb4\xf3\xbb\xaf), Min (\xd7\xee\xd0\xa1\xbb\xaf)\x0a");
}

void ShowUsage_Detailed(const char* prog) {
    printf("\x0a");
    printf("Token-Generator v1.0.0 (\xb8\xdf\xbc\xb6\xb0\xb2\xc8\xab\xc1\xee\xc5\xc6\xb4\xdb\xb8\xc4\xd3\xeb\xbd\xf8\xb3\xcc\xc6\xf4\xb6\xaf\xc6\xf7)\x0a");
    printf("====================================================\x0a\x0a");
    printf("\xd3\xc3\xb7\xa8: %s [\xd1\xa1\xcf\xee] [\xd6\xb4\xd0\xd0\xc3\xfc\xc1\xee (\xc4\xac\xc8\xcf: cmd.exe)]\x0a\x0a", prog);
    printf("\xd4\xa4\xc9\xe8\xb2\xce\xca\xfd\xd1\xa1\xd4\xf1:\x0a");
    printf("  -Use:<\xc3\xfb\xb3\xc6>      \xca\xb9\xd3\xc3\xd2\xd4\xcf\xc2\xbe\xad\xb5\xe4\xb5\xc4\xd4\xa4\xc9\xe8\xc6\xbe\xd6\xa4\xd6\xf7\xcc\xe5\xbb\xb7\xbe\xb3\xd6\xae\xd2\xbb\xa3\xba\x0a");
    printf("      S, System, S+, System+  (NT AUTHORITY\\SYSTEM)\x0a");
    printf("      A, Admin, A+, Admin+  (BUILTIN\\Administrators)\x0a");
    printf("      TI, TrustedInstaller, TI+, TrustedInstaller+  (NT SERVICE\\TrustedInstaller)\x0a");
    printf("      LS, LOCAL SERVICE, LS+, LOCAL SERVICE+  (NT AUTHORITY\\LOCAL SERVICE)\x0a");
    printf("      NS, NETWORK SERVICE, NS+, NETWORK SERVICE+  (NT AUTHORITY\\NETWORK SERVICE)\x0a");
    printf("      DWM, DWM+  (Window Manager\\DWM-1)\x0a");
    printf("\xc9\xed\xb7\xdd\xd1\xa1\xcf\xee:\x0a");
    printf("  -U:<\xd3\xc3\xbb\xa7\xc3\xfb|SID>   \xca\xd6\xb6\xaf\xd6\xb8\xb6\xa8\xd4\xcb\xd0\xd0\xc4\xbf\xb1\xea\xd3\xc3\xbb\xa7\xc9\xcf\xcf\xc2\xce\xc4\xc9\xed\xb7\xdd\x0a");
    printf("\xc1\xee\xc5\xc6\xb4\xdb\xb8\xc4\xd1\xa1\xcf\xee:\x0a");
    printf("  -G:<\xb8\xbd\xbc\xd3\xd7\xe9/SID>   \xd7\xa2\xc8\xeb\xb6\xee\xcd\xe2\xb0\xb2\xc8\xab\xd7\xe9 (\xba\xac\xbf\xd5\xb8\xf1\xb5\xc4\xd7\xe9\xc3\xfb\xb1\xd8\xd0\xeb\xbc\xd3\xd2\xfd\xba\xc5)\x0a");
    printf("  --UIAccess        \xce\xaa\xc1\xee\xc5\xc6\xbf\xaa\xc6\xf4 UI Access \xb1\xea\xd6\xbe\x0a");
    printf("  -IL:<\xcd\xea\xd5\xfb\xd0\xd4\xbc\xb6\xb1\xf0>  \xd0\xde\xb8\xc4\xc1\xee\xc5\xc6\xc7\xbf\xd6\xc6\xcd\xea\xd5\xfb\xd0\xd4\xbc\xb6\xb1\xf0:\x0a");
    printf("      U, Untrusted  \xb7\xc7\xd0\xc5\xc8\xce\xbc\xb6 (SECURITY_MANDATORY_UNTRUSTED_RID)\x0a");
    printf("      L, Low        \xb5\xcd\xcd\xea\xd5\xfb\xd0\xd4\xbc\xb6 (SECURITY_MANDATORY_LOW_RID)\x0a");
    printf("      M, Medium     \xd6\xd0\xb5\xc8\xcd\xea\xd5\xfb\xd0\xd4\xbc\xb6 (SECURITY_MANDATORY_MEDIUM_RID)\x0a");
    printf("      M+, Medium+   \xd6\xd0\xb5\xc8\xd4\xf6\xc7\xbf\xbc\xb6 (SECURITY_MANDATORY_MEDIUM_PLUS_RID)\x0a");
    printf("      H, High       \xb8\xdf\xcd\xea\xd5\xfb\xd0\xd4\xbc\xb6 (SECURITY_MANDATORY_HIGH_RID)\x0a");
    printf("      S, System     \xcf\xb5\xcd\xb3\xcd\xea\xd5\xfb\xd0\xd4\xbc\xb6 (SECURITY_MANDATORY_SYSTEM_RID)\x0a");
    printf("\xd4\xcb\xd0\xd0\xbf\xd8\xd6\xc6\xd1\xa1\xcf\xee:\x0a");
    printf("  --Debug           \xbf\xaa\xc6\xf4\xb5\xf7\xca\xd4\xc8\xd5\xd6\xbe\xa3\xac\xca\xe4\xb3\xf6\xb5\xd7\xb2\xe3 NtCreateToken \xd6\xb4\xd0\xd0\xc1\xf7\xb3\xcc\x0a");
    printf("  -GUI              \xc6\xf4\xb6\xaf\xb8\xfa\xcb\xe6\xcf\xb5\xcd\xb3\xd6\xf7\xcc\xe2\xb5\xc4\xcf\xd6\xb4\xfa GUI \xb9\xdc\xc0\xed\xc6\xf7\x0a");
    printf("  -d:<\xd7\xc0\xc3\xe6>         Winsta\\Desktop \xc2\xb7\xbe\xb6\x0a");
    printf("  -C:<\xc2\xb7\xbe\xb6>         \xd0\xde\xb8\xc4\xbd\xf8\xb3\xcc\xb9\xa4\xd7\xf7\xc4\xbf\xc2\xbc\xc2\xb7\xbe\xb6\x0a");
    printf("  -M:<\xcf\xd4\xca\xbe\xc4\xa3\xca\xbd>     \xc6\xf4\xb6\xaf\xb4\xb0\xbf\xda\xd7\xb4\xcc\xac: I (\xc4\xda\xc1\xaa), H (\xd2\xfe\xb2\xd8), Max (\xd7\xee\xb4\xf3\xbb\xaf), Min (\xd7\xee\xd0\xa1\xbb\xaf)\x0a\x0a");
    printf("\xcc\xd8\xc8\xa8\xd1\xa1\xcf\xee:\x0a");
    printf("  -Remove:<\xcc\xd8\xc8\xa8>    \xb3\xb9\xb5\xd7\xc7\xbf\xd6\xc6\xd2\xc6\xb3\xfd\xd6\xb8\xb6\xa8\xcc\xd8\xc8\xa8\x0a");
    printf("  -Disabled:<\xcc\xd8\xc8\xa8>   \xc7\xbf\xd6\xc6\xd6\xc3\xd6\xb8\xb6\xa8\xcc\xd8\xc8\xa8\xd7\xb4\xcc\xac\xce\xaa\xbd\xfb\xd3\xc3\x0a\x0a");
    printf("\xbf\xc9\xd3\xc3\xb5\xc4\xcc\xd8\xc8\xa8\xd0\xf2\xba\xc5\xd3\xeb\xc3\xfb\xb3\xc6\xb6\xd4\xd5\xd5\xb1\xed:\x0a");
    printf("    SeCreateTokenPrivilege:1\x0a    SeAssignPrimaryTokenPrivilege:2\x0a    SeLockMemoryPrivilege:3\x0a");
    printf("    SeIncreaseQuotaPrivilege:4\x0a    SeMachineAccountPrivilege:5\x0a    SeTcbPrivilege:6\x0a");
    printf("    SeSecurityPrivilege:7\x0a    SeTakeOwnershipPrivilege:8\x0a    SeLoadDriverPrivilege:9\x0a");
    printf("    SeSystemProfilePrivilege:10\x0a    SeSystemtimePrivilege:11\x0a    SeProfileSingleProcessPrivilege:12\x0a");
    printf("    SeIncreaseBasePriorityPrivilege:13\x0a    SeCreatePagefilePrivilege:14\x0a    SeCreatePermanentPrivilege:15\x0a");
    printf("    SeBackupPrivilege:16\x0a    SeRestorePrivilege:17\x0a    SeShutdownPrivilege:18\x0a");
    printf("    SeDebugPrivilege:19\x0a    SeAuditPrivilege:20\x0a    SeSystemEnvironmentPrivilege:21\x0a");
    printf("    SeChangeNotifyPrivilege:22\x0a    SeRemoteShutdownPrivilege:23\x0a    SeUndockPrivilege:24\x0a");
    printf("    SeSyncAgentPrivilege:25\x0a    SeEnableDelegationPrivilege:26\x0a    SeManageVolumePrivilege:27\x0a");
    printf("    SeImpersonatePrivilege:28\x0a    SeCreateGlobalPrivilege:29\x0a    SeTimeZonePrivilege:30\x0a");
    printf("    SeCreateSymbolicLinkPrivilege:31\x0a    SeRelabelPrivilege:32\x0a    SeIncreaseWorkingSetPrivilege:33\x0a");
    printf("    SeTrustedCredManAccessPrivilege:34\x0a    SeDelegateSessionUserImpersonatePrivilege:35\x0a\x0a");
    printf("\xca\xb9\xd3\xc3\xca\xbe\xc0\xfd:\x0a");
    printf("  %s -Use:TI cmd.exe\x0a", prog);
    printf("  %s -GUI\x0a", prog);
    printf("  %s -IL:Medium cmd.exe\x0a", prog);
    printf("  %s -G:\"CONSOLE LOGON\" cmd.exe\x0a", prog);
}

// ==========================================
// 6.              
// ==========================================
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

PSID DupSid(PSID src) {
    if (!src) return NULL;
    DWORD len = GetLengthSid(src);
    PSID dst = (PSID)HeapAlloc(GetProcessHeap(), 0, len);
    if (dst) CopySid(len, dst, src);
    return dst;
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

PSID ResolveIdentity(const char* identityStr) {
    if (_stricmp(identityStr, "S") == 0 || _stricmp(identityStr, "System") == 0) {
        Log(LOG_DEBUG, "\xd3\xb3\xc9\xe4\xc9\xed\xb7\xdd\xc9\xcf\xcf\xc2\xce\xc4: %s -> SYSTEM", identityStr);
        return GetSidFromString("S-1-5-18");
    }
    if (_stricmp(identityStr, "A") == 0 || _stricmp(identityStr, "Admin") == 0) {
        Log(LOG_DEBUG, "\xd3\xb3\xc9\xe4\xc9\xed\xb7\xdd\xc9\xcf\xcf\xc2\xce\xc4: %s -> Administrators", identityStr);
        return GetSidFromString("S-1-5-32-544");
    }
    if (_stricmp(identityStr, "TI") == 0 || _stricmp(identityStr, "TrustedInstaller") == 0) {
        Log(LOG_DEBUG, "\xd3\xb3\xc9\xe4\xc9\xed\xb7\xdd\xc9\xcf\xcf\xc2\xce\xc4: %s -> TrustedInstaller", identityStr);
        return GetSidForAccountName("NT SERVICE\\TrustedInstaller");
    }
    if (_stricmp(identityStr, "LS") == 0 || _stricmp(identityStr, "LOCAL SERVICE") == 0) {
        Log(LOG_DEBUG, "\xd3\xb3\xc9\xe4\xc9\xed\xb7\xdd\xc9\xcf\xcf\xc2\xce\xc4: %s -> LOCAL SERVICE", identityStr);
        return GetSidFromString("S-1-5-19");
    }
    if (_stricmp(identityStr, "NS") == 0 || _stricmp(identityStr, "NETWORK SERVICE") == 0) {
        Log(LOG_DEBUG, "\xd3\xb3\xc9\xe4\xc9\xed\xb7\xdd\xc9\xcf\xcf\xc2\xce\xc4: %s -> NETWORK SERVICE", identityStr);
        return GetSidFromString("S-1-5-20");
    }
    if (_stricmp(identityStr, "DWM") == 0) {
        Log(LOG_DEBUG, "\xd3\xb3\xc9\xe4\xc9\xed\xb7\xdd\xc9\xcf\xcf\xc2\xce\xc4: %s -> Window Manager\\DWM-1", identityStr);
        return GetSidForAccountName("Window Manager\\DWM-1");
    }
    PSID sid = GetSidFromString(identityStr);
    if (sid) return sid;
    return GetSidForAccountName(identityStr);
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

int ResolveWindowCreateMode(const char* arg) {
    if (_stricmp(arg, "I") == 0 || _stricmp(arg, "Inline") == 0) return -1;
    if (_stricmp(arg, "H") == 0 || _stricmp(arg, "Hide") == 0) return SW_HIDE;
    if (_stricmp(arg, "Max") == 0 || _stricmp(arg, "Maximize") == 0) return SW_SHOWMAXIMIZED;
    if (_stricmp(arg, "Min") == 0 || _stricmp(arg, "Minimize") == 0) return SW_SHOWMINIMIZED;
    return -2; //    
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

void TerminateParent(int parentPid, DWORD exitCode) {
    if (parentPid > 0) {
        HANDLE hParent = OpenProcess(PROCESS_TERMINATE, FALSE, (DWORD)parentPid);
        if (hParent) {
            TerminateProcess(hParent, exitCode);
            CloseHandle(hParent);
        }
    }
}

// ==========================================
// 7.                  
// ==========================================
HANDLE GetLsassToken() {
    EnablePrivilege(NULL, "SeDebugPrivilege");
    DWORD lsassPid = GetPidByNameA("lsass.exe");
    if (lsassPid == 0) {
        Log(LOG_ERROR, "\xce\xde\xb7\xa8\xb6\xa8\xce\xbb lsass.exe \xcf\xb5\xcd\xb3\xb7\xfe\xce\xf1\xbd\xf8\xb3\xcc");
        return NULL;
    }
    Log(LOG_DEBUG, "\xd5\xfd\xd4\xda\xb4\xf2\xbf\xaa LSASS \xbd\xf8\xb3\xcc (PID: %d)...", lsassPid);
    HANDLE hLsassProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, lsassPid);
    if (!hLsassProc) {
        Log(LOG_ERROR, "\xb4\xf2\xbf\xaa LSASS \xbd\xf8\xb3\xcc\xca\xa7\xb0\xdc\xa3\xac\xbe\xdc\xbe\xf8\xb7\xc3\xce\xca");
        return NULL;
    }
    HANDLE hLsassToken = NULL;
    if (!OpenProcessToken(hLsassProc, TOKEN_DUPLICATE | TOKEN_QUERY | TOKEN_IMPERSONATE, &hLsassToken)) {
        Log(LOG_ERROR, "\xce\xde\xb7\xa8\xbb\xf1\xc8\xa1 LSASS \xbd\xf8\xb3\xcc\xc1\xee\xc5\xc6");
        CloseHandle(hLsassProc);
        return NULL;
    }
    Log(LOG_SUCCESS, "\xd2\xd1\xb3\xc9\xb9\xa6\xd7\xa5\xc8\xa1 LSASS \xcf\xb5\xcd\xb3\xb5\xc4\xb0\xb2\xc8\xab\xc6\xbe\xd6\xa4\xc9\xcf\xcf\xc2\xce\xc4");
    CloseHandle(hLsassProc);
    return hLsassToken;
}

HANDLE CreateCustomToken(DWORD targetSessionId, PSID pUserSid, const std::vector<std::string>& extraGroups) {
    Log(LOG_DEBUG, "CreateCustomToken: \xbb\xee\xb6\xaf Session \xbb\xe1\xbb\xb0 ID = %d", targetSessionId);
    PNtCreateToken NtCreateToken = (PNtCreateToken)(void*)GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtCreateToken");
    if (!NtCreateToken) {
        Log(LOG_ERROR, "\xbc\xd3\xd4\xd8\xc4\xda\xba\xcb\xbd\xd3\xbf\xda NtCreateToken \xca\xa7\xb0\xdc (ntdll.dll)");
        return NULL;
    }
    if (!pUserSid || !IsValidSid(pUserSid)) {
        Log(LOG_ERROR, "\xce\xde\xd0\xa7\xb5\xc4\xd3\xc3\xbb\xa7\xb0\xb2\xc8\xab\xd6\xf7\xcc\xe5 SID");
        return NULL;
    }
    LPSTR sidStr = NULL;
    if (ConvertSidToStringSidA(pUserSid, &sidStr)) Log(LOG_INFO, "\xc4\xbf\xb1\xea\xce\xb1\xd4\xec\xb5\xc4\xb0\xb2\xc8\xab\xc9\xcf\xcf\xc2\xce\xc4\xd3\xc3\xbb\xa7: %s", sidStr ? sidStr : "\xce\xb4\xd6\xaa\xc9\xed\xb7\xdd");
    if (sidStr) LocalFree(sidStr);

    PSID pSidAdmins = GetSidFromString("S-1-5-32-544");
    PSID pSidAuth = GetSidFromString("S-1-5-11");
    PSID pSidEveryone = GetSidFromString("S-1-1-0");
    PSID pSidIntegrity = GetSidFromString("S-1-16-16384");
    PSID pLogonSid = GetLogonSid();

    HANDLE hThreadToken = NULL;
    OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hThreadToken);
    EnablePrivilege(hThreadToken, "SeCreateTokenPrivilege");
    EnablePrivilege(hThreadToken, "SeTcbPrivilege");
    EnablePrivilege(hThreadToken, "SeAssignPrimaryTokenPrivilege");
    CloseHandle(hThreadToken);

    std::vector<SID_AND_ATTRIBUTES> groups;
    groups.push_back({ pUserSid, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_OWNER });
    if (pLogonSid) groups.push_back({ pLogonSid, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_LOGON_ID });
    if (pSidAdmins) groups.push_back({ pSidAdmins, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY });
    if (pSidAuth) groups.push_back({ pSidAuth, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY });
    if (pSidEveryone) groups.push_back({ pSidEveryone, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY });
    if (pSidIntegrity) groups.push_back({ pSidIntegrity, SE_GROUP_INTEGRITY | SE_GROUP_INTEGRITY_ENABLED });

    for (const auto& groupName : extraGroups) {
        PSID sid = GetSidFromString(groupName.c_str());
        if (!sid) sid = GetSidForAccountName(groupName.c_str());
        if (sid) {
            groups.push_back({ sid, SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT });
            Log(LOG_DEBUG, "\xd7\xa2\xc8\xeb\xb8\xbd\xbc\xd3\xb0\xb2\xc8\xab\xd7\xe9: %s", groupName.c_str());
        }
        else {
            Log(LOG_WARN, "\xce\xde\xb7\xa8\xbd\xe2\xce\xf6\xb8\xbd\xbc\xd3\xd7\xe9\xc3\xfb\xb3\xc6: %s", groupName.c_str());
        }
    }

    DWORD groupsSize = sizeof(TOKEN_GROUPS) + groups.size() * sizeof(SID_AND_ATTRIBUTES);
    PTOKEN_GROUPS pGroups = (PTOKEN_GROUPS)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, groupsSize);
    pGroups->GroupCount = (DWORD)groups.size();
    for (size_t i = 0; i < groups.size(); i++) pGroups->Groups[i] = groups[i];

    DWORD privCount = 35;
    DWORD privSize = sizeof(TOKEN_PRIVILEGES) + privCount * sizeof(LUID_AND_ATTRIBUTES);
    PTOKEN_PRIVILEGES pPrivs = (PTOKEN_PRIVILEGES)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, privSize);
    pPrivs->PrivilegeCount = privCount;
    for (DWORD i = 0; i < privCount; i++) {
        pPrivs->Privileges[i].Luid.LowPart = i + 2;
        pPrivs->Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED;
    }

    TOKEN_USER tUser = { { pUserSid, 0 } };
    TOKEN_OWNER tOwner = { pUserSid };
    TOKEN_PRIMARY_GROUP tPrim = { pUserSid };
    TOKEN_SOURCE tSource;
    memcpy(tSource.SourceName, "TOKENGEN", 8);
    AllocateLocallyUniqueId(&tSource.SourceIdentifier);
    LUID authId = { 0x3e7, 0 };
    LARGE_INTEGER exp; exp.QuadPart = -1;
    OBJECT_ATTRIBUTES oa = {};
    oa.Length = sizeof(OBJECT_ATTRIBUTES);

    Log(LOG_DEBUG, "\xb7\xa2\xc6\xf0\xcf\xb5\xcd\xb3\xd3\xb2\xd6\xd0\xb6\xcf\xb5\xf7\xd3\xc3 NtCreateToken API...");
    HANDLE hNewToken = NULL;
    NTSTATUS status = NtCreateToken(
        &hNewToken, TOKEN_ALL_ACCESS, &oa, TokenPrimary, &authId, &exp,
        &tUser, pGroups, pPrivs, &tOwner, &tPrim, NULL, &tSource);

    if (status != STATUS_SUCCESS) {
        Log(LOG_ERROR, "\xce\xb1\xd4\xec NtCreateToken \xca\xa7\xb0\xdc: 0x%08X", status);
        RevertToSelf();
        HeapFree(GetProcessHeap(), 0, pGroups);
        HeapFree(GetProcessHeap(), 0, pPrivs);
        return NULL;
    }
    Log(LOG_SUCCESS, "\xd7\xd4\xb6\xa8\xd2\xe5\xc8\xa8\xcf\xde\xc1\xee\xc5\xc6\xd2\xd1\xb1\xbb\xbe\xab\xcf\xb8\xb5\xd8\xb6\xcd\xd4\xec\xb3\xc9\xd0\xcd");
    if (!SetTokenInformation(hNewToken, TokenSessionId, &targetSessionId, sizeof(DWORD))) {
        Log(LOG_WARN, "\xb0\xf3\xb6\xa8 Session ID \xd6\xc1\xc4\xbf\xb1\xea\xc1\xee\xc5\xc6\xb7\xa2\xc9\xfa\xd2\xec\xb3\xa3");
    }
    HeapFree(GetProcessHeap(), 0, pGroups);
    HeapFree(GetProcessHeap(), 0, pPrivs);
    return hNewToken;
}

BOOL ExecuteSudoOperation(
    LPSTR cmdLine,
    LPSTR identityStr,
    LPSTR desktop,
    const std::string& workingDir,
    int windowMode,
    DWORD integrityLevel,
    BOOL bUIAccess,
    const std::vector<std::string>& extraGroups,
    const std::vector<int>& DisabledPrivilege,
    const std::vector<int>& RemovePrivilege,
    PROCESS_INFORMATION* pOutPI //   
) {
    Log(LOG_INFO, "\xd5\xfd\xd4\xda\xbd\xf8\xc8\xeb\xbd\xf8\xb3\xcc\xb6\xcd\xd4\xec\xc6\xf4\xb6\xaf\xc6\xf7...");
    Log(LOG_DEBUG, "\xd6\xb4\xd0\xd0\xc3\xfc\xc1\xee: %s", cmdLine);
    Log(LOG_DEBUG, "\xb0\xb2\xc8\xab\xd7\xc0\xc3\xe6: %s", desktop ? desktop : "\xb5\xb1\xc7\xb0\xbb\xee\xb6\xaf\xd7\xc0\xc3\xe6");
    Log(LOG_DEBUG, "\xbb\xb7\xbe\xb3\xc9\xed\xb7\xdd: %s", identityStr);
    if (workingDir.empty()) {
        char tmp[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, tmp);
        Log(LOG_DEBUG, "\xd4\xcb\xd0\xd0\xc2\xb7\xbe\xb6: %s", tmp);
    }
    else Log(LOG_DEBUG, "\xd4\xcb\xd0\xd0\xc2\xb7\xbe\xb6: %s", workingDir.c_str());
    if (bUIAccess) Log(LOG_DEBUG, "UI Access \xd4\xf6\xc7\xbf\xb7\xc3\xce\xca\xcc\xd8\xc8\xa8\xd2\xd1\xd7\xb0\xd4\xd8");
    if (integrityLevel != (DWORD)-1) Log(LOG_DEBUG, "\xcd\xea\xd5\xfb\xd0\xd4\xc7\xbf\xd6\xc6\xb1\xea\xc7\xa9\xbc\xb6\xb1\xf0: 0x%06x", integrityLevel);

    DWORD sess = GetActiveSessionID();
    PSID pTargetSid = ResolveIdentity(identityStr);
    if (!pTargetSid) {
        Log(LOG_ERROR, "\xce\xde\xb7\xa8\xbd\xe2\xce\xf6\xb4\xcb\xc9\xed\xb7\xdd\xc9\xcf\xcf\xc2\xce\xc4: %s", identityStr);
        return FALSE;
    }

    HANDLE hLsassToken = GetLsassToken();
    if (hLsassToken == NULL) return FALSE;
    if (!ImpersonateLoggedOnUser(hLsassToken)) {
        DWORD ErrCode = GetLastError();
        Log(LOG_ERROR, "\xc6\xbe\xd6\xa4\xc4\xa3\xc4\xe2 LSASS \xca\xa7\xb0\xdc: %lu", ErrCode);
        Log_ErrorCode(LOG_WARN, ErrCode);
        CloseHandle(hLsassToken);
        return FALSE;
    }

    HANDLE hToken = CreateCustomToken(sess, pTargetSid, extraGroups);
    if (!hToken) {
        RevertToSelf();
        CloseHandle(hLsassToken);
        return FALSE;
    }

    if (bUIAccess) {
        BOOL UIAccess = TRUE;
        if (SetTokenInformation(hToken, TokenUIAccess, &UIAccess, sizeof(BOOL))) {
            Log(LOG_SUCCESS, "\xc1\xee\xc5\xc6\xd2\xd1\xbf\xaa\xc6\xf4 UI Access \xbf\xd8\xd6\xc6\xb1\xea\xd6\xbe");
        } else {
            DWORD ErrCode = GetLastError();
            Log(LOG_WARN, "\xd0\xb4\xc8\xeb UI Access \xb1\xea\xbc\xc7\xca\xa7\xb0\xdc: %lu", ErrCode);
            Log_ErrorCode(LOG_WARN, ErrCode);
        }
    }

    if (integrityLevel != (DWORD)-1) {
        SID sid = {};
        sid.Revision = SID_REVISION;
        sid.SubAuthorityCount = 1;
        sid.IdentifierAuthority = SECURITY_MANDATORY_LABEL_AUTHORITY;
        sid.SubAuthority[0] = integrityLevel;
        TOKEN_MANDATORY_LABEL tml = {};
        tml.Label.Attributes = SE_GROUP_INTEGRITY;
        tml.Label.Sid = &sid;
        if (SetTokenInformation(hToken, TokenIntegrityLevel, &tml, sizeof(TOKEN_MANDATORY_LABEL) + sizeof(DWORD))) {
            Log(LOG_SUCCESS, "\xc1\xee\xc5\xc6\xc7\xbf\xd6\xc6\xb0\xb2\xc8\xab\xcd\xea\xd5\xfb\xd0\xd4\xb1\xea\xc7\xa9 (IL) \xd0\xde\xb8\xc4\xcd\xea\xb3\xc9");
            DeleteDisabledPrivileges(hToken);
        } else {
            DWORD ErrCode = GetLastError();
            Log(LOG_WARN, "\xc9\xe8\xd6\xc3\xcd\xea\xd5\xfb\xd0\xd4\xb5\xc8\xbc\xb6\xca\xa7\xb0\xdc: %lu", ErrCode);
            Log_ErrorCode(LOG_WARN, ErrCode);
        }
    }
    
    if(!RemovePrivilege.empty()) {
        for(int id : RemovePrivilege) {
            LUID_AND_ATTRIBUTES Privilege{{DWORD(id + 1), 0}, SE_PRIVILEGE_REMOVED};
            HANDLE tmpToken = NULL;
            if (CreateRestrictedToken(hToken, 0, 0, NULL, 1, &Privilege, 0, NULL, &tmpToken)) {
                hToken = tmpToken;
                Log(LOG_SUCCESS, "\xc7\xbf\xd6\xc6\xb4\xd3\xc1\xee\xc5\xc6\xd6\xd0\xc7\xd0\xb3\xfd\xcc\xd8\xc8\xa8 ID %d \xb3\xc9\xb9\xa6", id);
            } else {
                DWORD ErrCode = GetLastError();
                Log(LOG_WARN, "\xc7\xbf\xd6\xc6\xc7\xd0\xb3\xfd\xcc\xd8\xc8\xa8 ID %d \xca\xa7\xb0\xdc: %lu", id, ErrCode);
                Log_ErrorCode(LOG_WARN, ErrCode);
            }
        }
    }
    
    if(!DisabledPrivilege.empty()) {
        for(int id : DisabledPrivilege) {
            TOKEN_PRIVILEGES tp{1, {{{DWORD(id + 1), 0}, 0L}}};
            if (AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL)) {
                Log(LOG_SUCCESS, "\xbd\xfb\xd3\xc3\xcc\xd8\xc8\xa8 ID %d \xb3\xc9\xb9\xa6", id);
            } else {
                DWORD ErrCode = GetLastError();
                Log(LOG_WARN, "\xbd\xfb\xd3\xc3\xcc\xd8\xc8\xa8 ID %d \xca\xa7\xb0\xdc: %lu", id, ErrCode);
                Log_ErrorCode(LOG_WARN, ErrCode);
            }
        }
    }

    LPVOID lpEnv = NULL;
    CreateEnvironmentBlock(&lpEnv, hToken, FALSE);
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.lpDesktop = desktop;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = windowMode;
    PROCESS_INFORMATION pi = {};
    DWORD dwCreationFlags = CREATE_UNICODE_ENVIRONMENT | CREATE_NEW_CONSOLE;
    LPCSTR lpCurrentDir = workingDir.empty() ? NULL : workingDir.c_str();

    BOOL success = CreateProcessAsUserA(
        hToken, NULL, cmdLine, NULL, NULL, FALSE,
        dwCreationFlags, lpEnv, lpCurrentDir, &si, &pi
    );

    RevertToSelf();
    CloseHandle(hLsassToken);
    if (lpEnv) DestroyEnvironmentBlock(lpEnv);
    CloseHandle(hToken);

    if (success) {
        Log(LOG_SUCCESS, "\xbd\xf8\xb3\xcc\xd4\xcb\xd0\xd0\xc6\xf4\xb6\xaf\xb3\xc9\xb9\xa6! \xb7\xd6\xc5\xe4\xbd\xf8\xb3\xcc\xb1\xea\xca\xb6 PID: %lu", pi.dwProcessId);
        if (pOutPI) *pOutPI = pi;
        else {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        return TRUE;
    }
    else {
        DWORD err = GetLastError();
        Log(LOG_ERROR, "CreateProcessAsUserA \xb5\xf7\xd3\xc3\xca\xa7\xb0\xdc: %lu", err);
        Log_ErrorCode(LOG_ERROR, err);
        return FALSE;
    }
}

// ==========================================
// 9.               
// ==========================================
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

// ==========================================
// 10. Fluent UI     Card    
// ==========================================
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

// ==========================================
// 11. whoami /all          
// ==========================================
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

// ==========================================
// 12.              
// ==========================================
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

LRESULT CALLBACK GroupEditorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hList;
    switch (msg) {
        case WM_CREATE: {
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

LRESULT CALLBACK PrivilegeEditorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hList;
    switch (msg) {
        case WM_CREATE: {
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

// ==========================================
// 13.     WndProc     
// ==========================================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            UpdateThemeColors();
            EnableImmersiveDarkMode(hwnd, g_bDarkMode);
            InitializeThemeResources();
            
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

// ==========================================
// 14.        main()   WinMain
// ==========================================
int main(int argc, char* argv[]) {
    LoadLibraryA("Msftedit.dll");
    
    //               
    if (!IsUserAnAdmin()) {
        const char* errMsg = "\xb4\xed\xce\xf3\xa3\xbaToken-Generator \xd0\xe8\xd2\xaa\xcd\xea\xc8\xab\xcc\xe1\xc8\xa8\xb5\xc4\xb9\xdc\xc0\xed\xd4\xb1\xc8\xa8\xcf\xde\xb7\xbd\xbf\xc9\xd6\xb4\xd0\xd0\xa1\xa3\x0a"
                             "\xc7\xeb\xd3\xd2\xbc\xfc\xd2\xd4\xa1\xb0\xb9\xdc\xc0\xed\xd4\xb1\xc9\xed\xb7\xdd\xd4\xcb\xd0\xd0\xa1\xb1\xd6\xd8\xd0\xc2\xd7\xb0\xd4\xd8\xb1\xbe\xd3\xa6\xd3\xc3\xb3\xcc\xd0\xf2\xa1\xa3\x0a";
        WriteToStdOut(errMsg);
        
        MessageBoxA(NULL, 
            "Token-Generator \xd0\xe8\xd2\xaa\xd2\xd4\xb8\xdf\xbc\xb6\xb9\xdc\xc0\xed\xd4\xb1\xcc\xd8\xc8\xa8\xd4\xcb\xd0\xd0\xa1\xa3\x0a\x0a"
            "\xb1\xbe\xb9\xa4\xbe\xdf\xbb\xf9\xd3\xda\xb5\xd7\xb2\xe3\xcf\xb5\xcd\xb3\xc4\xda\xba\xcb\xbc\xb6\xbd\xbb\xbb\xa5\xa3\xac\xb0\xfc\xc0\xa8\xa3\xba\x0a"
            " - \xbf\xaa\xc6\xf4 SeDebugPrivilege \xb0\xb2\xc8\xab\xcc\xd8\xc8\xa8\x0a"
            " - \xc7\xbf\xd0\xd0\xb4\xf2\xbf\xaa\xa1\xa2\xb6\xc1\xc8\xa1\xa1\xa2\xb8\xb4\xd6\xc6 LSASS \xb9\xd8\xbc\xfc\xcf\xb5\xcd\xb3\xbd\xf8\xb3\xcc\xc6\xbe\xd6\xa4\x0a"
            " - \xb5\xf7\xd3\xc3\xce\xb4\xb9\xab\xbf\xaa\xc4\xda\xba\xcb\xba\xaf\xca\xfd NtCreateToken \xc8\xc6\xb9\xfd\xcf\xb5\xcd\xb3\xcf\xde\xd6\xc6\xc7\xbf\xd0\xd0\xb6\xcd\xd4\xec\xb0\xb2\xc8\xab\xc1\xee\xc5\xc6\x0a\x0a"
            "\xc7\xeb\xd3\xd2\xbc\xfc\xb5\xe3\xbb\xf7 TokenGenerator.exe\xa3\xac\xb2\xa2\xd1\xa1\xd4\xf1\xa1\xb0\xd2\xd4\xb9\xdc\xc0\xed\xd4\xb1\xc9\xed\xb7\xdd\xd4\xcb\xd0\xd0\xa1\xb1\xa1\xa3", 
            "Token-Generator - \xb0\xb2\xc8\xab\xb7\xc3\xce\xca\xb1\xbb\xbe\xdc\xbe\xf8", 
            MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // CLI        
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0) { ShowUsage_Detailed(argv[0]); return 0; }
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "/?") == 0) { ShowUsage_Brief(argv[0]); return 0; }
    }
    
    bool bShowGUI = false;
    bool bNeedChangeDir = true;
    int argStart = argc;
    int ParentProcessId = 0;
    
    g_desktop = GetCurrentLpDesktop();
    
    //         (CLI     )
    int parseStart = 1;
    if (argc >= 4 && strcmp(argv[1], "-pid") == 0) {
        bNeedChangeDir = false;
        ParentProcessId = atoi(argv[2]);
        parseStart = 4;
        if (argc >= 6 && strcmp(argv[3], "-E") == 0) {
            SetCurrentDirectoryA(argv[4]);
            parseStart = 6;
        }
    }
    
    for (int i = parseStart; i < argc; i++) {
        if (_stricmp(argv[i], "--debug") == 0) {
            g_bDebug = true;
        }
        else if (_stricmp(argv[i], "--UIAccess") == 0) {
            g_bUIAccess = true;
        }
        else if (_stricmp(argv[i], "-GUI") == 0) {
            bShowGUI = true;
        }
        else if (_strnicmp(argv[i], "-U:", 3) == 0) {
            if (strlen(argv[i]) <= 3) { Log(LOG_ERROR, "-U \xd0\xe8\xd2\xaa\xca\xd6\xb6\xaf\xb6\xa8\xd2\xe5\xc4\xbf\xb1\xea\xd3\xc3\xbb\xa7\xc3\xfb\xa1\xa3"); return 1; }
            g_identityStr = argv[i] + 3;
        }
        else if (_strnicmp(argv[i], "-Use:", 5) == 0) {
            if (strlen(argv[i]) <= 5) { Log(LOG_ERROR, "-Use \xd0\xe8\xd2\xaa\xca\xd6\xb6\xaf\xd1\xa1\xd4\xf1\xd2\xbb\xb8\xf6\xd3\xd0\xd0\xa7\xb5\xc4\xd4\xa4\xc9\xe8\xa1\xa3"); return 1; }
            Preset tmp = ResolvePreset(argv[i] + 5);
            if (tmp.identityStr == NULL) { Log(LOG_ERROR, "\xce\xde\xb7\xa8\xd5\xfd\xc8\xb7\xbd\xe2\xce\xf6\xca\xe4\xc8\xeb\xb5\xc4\xd4\xa4\xc9\xe8\xd6\xb5\xa1\xa3"); return 1; }
            g_identityStr = tmp.identityStr;
            extraGroups = tmp.extraGroups;
            RemovePrivilege = tmp.RemovePrivilege;
        }
        else if (_strnicmp(argv[i], "-D:", 3) == 0 || _strnicmp(argv[i], "-d:", 3) == 0) {
            if (strlen(argv[i]) <= 3) { Log(LOG_ERROR, "-d/-D \xd0\xe8\xd2\xaa\xb6\xa8\xd2\xe5\xd7\xc0\xc3\xe6\xc3\xfb\xb3\xc6\xa1\xa3"); return 1; }
            g_desktop = argv[i] + 3;
        }
        else if (_strnicmp(argv[i], "-IL:", 4) == 0) {
            long IL_id = ResolveILlevel(argv[i] + 4);
            if (IL_id == -1L) { Log(LOG_WARN, "\xce\xde\xb7\xa8\xca\xb6\xb1\xf0\xb5\xc4\xc7\xbf\xd6\xc6\xcd\xea\xd5\xfb\xd0\xd4 IL \xd7\xd6\xb7\xfb: %s", argv[i] + 4); }
            Integrity_Level = IL_id;
        }
        else if (_strnicmp(argv[i], "-Disabled:", 10) == 0) {
            int PrivilegeId = ResolvePrivilegeId(argv[i] + 10);
            if (PrivilegeId == -1) { Log(LOG_WARN, "\xce\xde\xb7\xa8\xbd\xe2\xce\xf6\xb5\xc4\xcc\xd8\xc8\xa8\xc3\xfb\xb3\xc6: %s", argv[i] + 10); }
            DisabledPrivilege.push_back(PrivilegeId);
        }
        else if (_strnicmp(argv[i], "-Remove:", 8) == 0) {
            int PrivilegeId = ResolvePrivilegeId(argv[i] + 8);
            if (PrivilegeId == -1) { Log(LOG_WARN, "\xce\xde\xb7\xa8\xbd\xe2\xce\xf6\xb5\xc4\xcc\xd8\xc8\xa8\xc3\xfb\xb3\xc6: %s", argv[i] + 8); }
            RemovePrivilege.push_back(PrivilegeId);
        }
        else if (_strnicmp(argv[i], "-M:", 3) == 0) {
            if (strlen(argv[i]) <= 3) { Log(LOG_ERROR, "-M \xd0\xe8\xd2\xaa\xb4\xb0\xbf\xda\xc6\xf4\xb6\xaf\xd7\xb4\xcc\xac\xb2\xce\xca\xfd\xa1\xa3"); return 1; }
            int CreateMode = ResolveWindowCreateMode(argv[i] + 3);
            if (CreateMode == -2) { Log(LOG_WARN, "\xce\xde\xb7\xa8\xca\xb6\xb1\xf0\xb5\xc4\xb4\xb0\xbf\xda\xcf\xd4\xca\xbe\xc4\xa3\xca\xbd: %s", argv[i] + 3); }
            g_WindowCreateMode = CreateMode;
        }
        else if (_strnicmp(argv[i], "-C:", 3) == 0) {
            if (strlen(argv[i]) <= 3) { Log(LOG_ERROR, "-C \xd0\xe8\xd2\xaa\xb3\xf5\xca\xbc\xd4\xcb\xd0\xd0\xc4\xbf\xc2\xbc\xa1\xa3"); return 1; }
            if (bNeedChangeDir) {
                if (!SetCurrentDirectoryA(argv[i] + 3)) {
                    DWORD errCode = GetLastError();
                    Log(LOG_WARN, "\xd0\xde\xb8\xc4\xb9\xa4\xd7\xf7\xc4\xbf\xc2\xbc\xca\xa7\xb0\xdc: %s, \xb4\xed\xce\xf3\xc3\xe8\xca\xf6\xc2\xeb: %lu", argv[i] + 3, errCode);
                    Log_ErrorCode(LOG_WARN, errCode);
                }
            }
        }
        else if (_strnicmp(argv[i], "-G:", 3) == 0) {
            if (strlen(argv[i]) <= 3) { Log(LOG_ERROR, "-G \xd0\xe8\xd2\xaa\xb6\xa8\xd2\xe5\xd7\xe9\xc3\xfb\xb3\xc6\xbb\xf2 SID\xa1\xa3"); return 1; }
            extraGroups.push_back(std::string(argv[i] + 3));
        }
        else if (argv[i][0] == '-') {
            Log(LOG_ERROR, "\xce\xb4\xd6\xaa\xb5\xc4\xb2\xce\xca\xfd\xb1\xea\xca\xb6: %s", argv[i]);
        }
        else {
            argStart = i;
            break;
        }
    }
    
    //             -GUI        GUI
    if (argc == 1 || bShowGUI) {
        DWORD procList[2];
        DWORD dwProcCount = GetConsoleProcessList(procList, 2);
        
        if (dwProcCount > 1) {
            STARTUPINFOA si = {};
            si.cb = sizeof(si);
            PROCESS_INFORMATION pi = {};
            BOOL bSuccess = CreateProcessA(NULL, GetCommandLineA(), NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi);
            if (bSuccess) {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                return 0;
            }
        } else if (dwProcCount) {
            FreeConsole();
        }
        
        WNDCLASSEXA wc{};
        wc.cbSize = sizeof(WNDCLASSEXA);
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
        wc.lpszClassName = "TokenGeneratorGuiClass";

        RegisterClassExA(&wc);

        //          +       
        RECT rClient = { 0, 0, 1005, 755 };
        AdjustWindowRectEx(&rClient, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, FALSE, 0);
        int w = rClient.right - rClient.left;
        int h = rClient.bottom - rClient.top;

        HWND hwnd = CreateWindowA("TokenGeneratorGuiClass", "Windows Token Generator", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
            CW_USEDEFAULT, CW_USEDEFAULT, w, h, NULL, NULL, GetModuleHandle(NULL), NULL);

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0) > 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return 0;
    }
    
    if (g_WindowCreateMode == -1) {
        SetConsoleCtrlHandler(NULL, TRUE);
    }
    
    if (ParentProcessId > 0) {
        FreeConsole();
        if (!AttachConsole(ParentProcessId)) {
            AllocConsole();
        }
    }
    
    if (argStart < argc) {
        g_runCommand = GetCommandLineA();
        BOOL InQuotes = 0, InEscape = 0;
        for (int i = 0; i < argStart; i++) {
            for (; *g_runCommand != ' ' || InQuotes; g_runCommand++) {
                if (*g_runCommand == '\\') {
                    InEscape ^= 1;
                }
                else if (!InEscape && *g_runCommand == '\"') {
                    InQuotes ^= 1;
                }
                else {
                    InEscape = 0;
                }
            }
            for (; isspace(*g_runCommand); g_runCommand++);
        }
    }
    
    PROCESS_INFORMATION pi = {};
    BOOL success = ExecuteSudoOperation(
        g_runCommand,
        g_identityStr,
        g_desktop,
        "", 
        g_WindowCreateMode,
        Integrity_Level,
        g_bUIAccess,
        extraGroups,
        DisabledPrivilege,
        RemovePrivilege,
        &pi
    );
    
    if (success) {
        if (g_WindowCreateMode == -1) {
            WaitForSingleObject(pi.hProcess, INFINITE);
            DWORD exitCode = 0;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            TerminateParent(ParentProcessId, exitCode);
            ExitProcess(exitCode);
        }
        else {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            TerminateParent(ParentProcessId, 0);
            ExitProcess(0);
        }
    }
    else {
        TerminateParent(ParentProcessId, 1);
        return 1;
    }
    
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    (void)hInstance; (void)hPrevInstance; (void)lpCmdLine; (void)nShowCmd;
    return main(__argc, __argv);
}
