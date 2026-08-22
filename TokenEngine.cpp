#include "TokenEngine.h"
#include "Globals.h"
#include "Utils.h"

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
    DWORD dwCreationFlags = CREATE_UNICODE_ENVIRONMENT;
    if (windowMode != -1) dwCreationFlags |= CREATE_NEW_CONSOLE;
    LPCSTR lpCurrentDir = workingDir.empty() ? NULL : workingDir.c_str();

    if (windowMode == -1) {
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    }
    BOOL success = CreateProcessAsUserA(
        hToken, NULL, cmdLine, NULL, NULL, (windowMode == -1) ? TRUE : FALSE,
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

