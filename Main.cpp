#include "Common.h"
#include "Globals.h"
#include "Utils.h"
#include "TokenEngine.h"
#include "ThemeEngine.h"
#include "SimConsole.h"
#include "UIEditor.h"
#include "MainWindow.h"

void ShowUsage_Detailed(const char* prog);
void ShowUsage_Brief(const char* prog);

// Inline definitions of CLI parsing helpers
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

