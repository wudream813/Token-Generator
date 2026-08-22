#!/bin/bash
# Token-Generator Linux cross-compilation script (MinGW-w64)
set -e

echo "=============================================="
echo "Token-Generator 交叉编译脚本 (Linux -> Windows)"
echo "=============================================="
echo ""

# Check for cross-compiler
if ! command -v x86_64-w64-mingw32-windres &> /dev/null; then
    echo "Error: x86_64-w64-mingw32-windres not found."
    echo "Please install mingw-w64 using: sudo apt install mingw-w64"
    exit 1
fi

echo "[1/2] Compiling Windows resources..."
x86_64-w64-mingw32-windres resources.rc -o resources.o

echo "[2/2] Linking and compiling modular C++ files (UTF-8 to GBK automatic translation)..."
x86_64-w64-mingw32-g++ -Ofast -Wall -Wextra -static \
    -finput-charset=UTF-8 -fexec-charset=GBK \
    Main.cpp Globals.cpp Utils.cpp TokenEngine.cpp ThemeEngine.cpp SimConsole.cpp UIEditor.cpp MainWindow.cpp \
    resources.o -o TokenGenerator.exe \
    -lwtsapi32 -luserenv -lntdll -ladvapi32 -lgdi32 -lcomctl32 -lcomdlg32 -luuid -lole32

echo ""
echo "=============================================="
echo "SUCCESS! TokenGenerator.exe compiled successfully!"
echo "=============================================="
