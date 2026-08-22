#pragma once
#include "Common.h"

HANDLE GetLsassToken();
HANDLE CreateCustomToken(DWORD targetSessionId, PSID pUserSid, const std::vector<std::string>& extraGroups);
BOOL ExecuteSudoOperation(LPSTR cmdLine, LPSTR identityStr, LPSTR desktop, const std::string& workingDir, int windowMode, DWORD integrityLevel, BOOL bUIAccess, const std::vector<std::string>& extraGroups, const std::vector<int>& DisabledPrivilege, const std::vector<int>& RemovePrivilege, PROCESS_INFORMATION* pOutPI);
PSID ResolveIdentity(const char* identityStr);
void DeleteDisabledPrivileges(HANDLE& hToken);
