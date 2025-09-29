@echo off
echo Building Win Translator with Dialog...

REM Compile resource file first
rc /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um" resources\WinTranslator.rc

REM Check if resource compilation succeeded
if not exist resources\WinTranslator.res (
    echo Resource compilation failed!
    pause
    exit /b 1
)

REM Compile C++ files
cl /EHsc /O2 /DUNICODE /D_UNICODE ^
   /I"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\include" ^
   /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\ucrt" ^
   /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\um" ^
   /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22621.0\shared" ^
   src/main.cpp src/App.cpp src/SimpleSettingsDialog.cpp ^
   /link ^
   /LIBPATH:"C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207\lib\x64" ^
   /LIBPATH:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x64" ^
   /LIBPATH:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\ucrt\x64" ^
   comctl32.lib shell32.lib wininet.lib urlmon.lib shlwapi.lib gdi32.lib user32.lib kernel32.lib ^
   resources\WinTranslator.res ^
   /SUBSYSTEM:WINDOWS /OUT:win-translator.exe

echo.
if exist win-translator.exe (
    echo ✅ Build successful! win-translator.exe created.
    echo To run: .\win-translator.exe
) else (
    echo ❌ Build failed!
)
echo.
pause
