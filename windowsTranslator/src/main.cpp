#include <windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <commctrl.h>
#include <shlobj.h>
#include <wininet.h>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>

// Win32++ includes (we'll include core functionality inline for simplicity)
#include "App.h"

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
                     LPSTR lpCmdLine, int nCmdShow)
{
    try
    {
        // Initialize application
        CWinTranslatorApp theApp;
        
        // Run the application
        return theApp.Run();
    }
    catch (...)
    {
        MessageBox(nullptr, L"An unhandled exception occurred", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }
}
