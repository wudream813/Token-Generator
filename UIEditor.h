#pragma once
#include "Common.h"

void UpdateGuiSummaries(HWND hEditGroups, HWND hEditPrivs);
void ShowGroupEditor(HWND hParent);
void ShowPrivilegeEditor(HWND hParent);
void AddNewGroupItem(HWND hList);
void DeleteSelectedGroup(HWND hList);
void EditSelectedGroup(HWND hList);
void UpdatePrivStatus(HWND hList, int iItem, const char* status);
void SetSelectedPrivs(HWND hList, const char* status);
LRESULT CALLBACK GroupEditorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK PrivilegeEditorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
