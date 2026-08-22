#pragma once
#include "Common.h"

void WriteToStdOut(const char* str);
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
DWORD GetActiveSessionID();
