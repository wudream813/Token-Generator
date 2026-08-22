#pragma once
#include "Common.h"
#include <string>

int GetDisplayWidth(const std::string& str);
std::string PadRight(const std::string& str, int width);
void GetILInfo(DWORD il, std::string& name, std::string& sid);
void SetRichEditDefaultColor(HWND hEdit, COLORREF color);
void UpdateTokenPreview(HWND hwnd);
