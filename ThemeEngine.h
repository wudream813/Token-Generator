#pragma once
#include "Common.h"

BOOL IsSystemInDarkMode();
void EnableImmersiveDarkMode(HWND hwnd, BOOL bEnable);
void ApplyThemeToControl(HWND hwnd);
void UpdateThemeColors();
void InitializeThemeResources();
void CleanThemeResources();
LRESULT CALLBACK ModernButtonSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
void MakeButtonModern(HWND hBtn);
void DrawModernButton(LPDRAWITEMSTRUCT pdis);
void DrawCardFrame(HDC hdc, int x, int y, int w, int h, const char* title);
