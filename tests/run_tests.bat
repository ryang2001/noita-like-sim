@echo off
echo ================================
echo  Compiling Test Program...
echo ================================
echo.

g++ -o test_core.exe test_core.cpp -I.. -I"C:/mingw64/include/SDL2" -L"C:/mingw64/lib" -m64 -lmingw32 -lSDL2main -lSDL2 -std=c++20 -DSDL_MAIN_HANDLED=1 2>&1

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Compilation failed!
    echo.
    pause
    exit /b 1
)

echo.
echo [SUCCESS] Test program compiled successfully!
echo.

echo ================================
echo Running Tests...
echo ================================
echo.

test_core.exe

echo.
echo Test completed. Press any key to exit...
echo.
pause >nul
