@echo off
echo Building Lightweight Freeze Macro...

:: Check if cl.exe is available (Visual Studio)
where cl.exe >nul 2>&1
if %errorlevel% == 0 (
    echo Using Visual Studio compiler...
    cl.exe /EHsc /O2 freeze_macro_standalone.cpp /Fe:freeze_macro.exe
    goto :done
)

:: Check if g++ is available (MinGW/MSYS2)
where g++ >nul 2>&1
if %errorlevel% == 0 (
    echo Using g++ compiler...
    g++ -std=c++20 -O2 -static -o freeze_macro.exe freeze_macro_standalone.cpp
    goto :done
)

:: Check if clang++ is available
where clang++ >nul 2>&1
if %errorlevel% == 0 (
    echo Using clang++ compiler...
    clang++ -std=c++20 -O2 -static -o freeze_macro.exe freeze_macro_standalone.cpp
    goto :done
)

echo Error: No suitable C++ compiler found!
echo Please install one of the following:
echo - Visual Studio (with C++ tools)
echo - MinGW-w64 / MSYS2
echo - LLVM/Clang
pause
exit /b 1

:done
if exist freeze_macro.exe (
    echo.
    echo Build successful! Created freeze_macro.exe
    echo.
    echo Usage:
    echo   freeze_macro.exe     - Run the lightweight freeze macro
    echo.
    echo The program will create a config file on first run.
    echo Edit freeze_macro_config.txt to customize settings.
) else (
    echo Build failed!
)

pause
