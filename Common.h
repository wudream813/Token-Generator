#pragma once

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
