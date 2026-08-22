@echo off
echo ==============================================
echo Token-Generator 一键编译脚本 (MinGW-64 Windows)
echo ==============================================
echo.

echo [1/2] 正在编译资源文件 resources.rc...
windres resources.rc -o resources.o
if %errorlevel% neq 0 (
    echo 错误：资源编译失败，请检查是否安装了 MinGW-w64！
    pause
    exit /b 1
)

echo [2/2] 正在联立静态编译多文件 C++ 代码...
g++ -Ofast -Wall -Wextra -static ^
    Main.cpp Globals.cpp Utils.cpp TokenEngine.cpp ThemeEngine.cpp SimConsole.cpp UIEditor.cpp MainWindow.cpp ^
    resources.o -o TokenGenerator.exe ^
    -lwtsapi32 -luserenv -lntdll -ladvapi32 -lgdi32 -lcomctl32 -lcomdlg32 -luuid -lole32

if %errorlevel% neq 0 (
    echo 错误：C++ 联立编译失败，请检查编译日志！
    pause
    exit /b 1
)

echo.
echo ==============================================
echo 恭喜！TokenGenerator.exe 静态编译打包生成成功！
echo ==============================================
pause
