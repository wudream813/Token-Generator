#include "Globals.h"

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

bool g_bDebug = false;
bool g_bUIAccess = false;
std::vector<std::string> extraGroups;
std::vector<int> RemovePrivilege;
std::vector<int> DisabledPrivilege;
int g_WindowCreateMode = SW_SHOWNORMAL;
DWORD Integrity_Level = SECURITY_MANDATORY_SYSTEM_RID;
HWND g_hLogEdit = NULL;
HWND g_hGroupEditor = NULL;
HWND g_hPrivEditor = NULL;
HWND g_hPreviewEdit = NULL;
LPSTR g_desktop = NULL;
LPSTR g_runCommand = (LPSTR)"cmd.exe";
LPSTR g_identityStr = (LPSTR)"NT AUTHORITY\SYSTEM";

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
