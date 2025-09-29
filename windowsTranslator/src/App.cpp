#include "App.h"
#include "SimpleSettingsDialog.h"
#include <commctrl.h>
#include <shlwapi.h>
#include <fstream>
#include <urlmon.h>
#include <algorithm>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "shlwapi.lib")

CWinTranslatorApp* CWinTranslatorApp::s_pInstance = nullptr;

CWinTranslatorApp::CWinTranslatorApp() 
    : m_hInstance(GetModuleHandle(nullptr))
    , m_hwndMain(nullptr)
    , m_hwndOverlay(nullptr)
    , m_overlayVisible(false)
{
    s_pInstance = this;
    ZeroMemory(&m_nid, sizeof(m_nid));
    LoadSettings();
}

CWinTranslatorApp::~CWinTranslatorApp() {
    UnregisterHotkeys();
    if (m_nid.hWnd) {
        Shell_NotifyIcon(NIM_DELETE, &m_nid);
    }
    if (m_hwndOverlay) {
        DestroyWindow(m_hwndOverlay);
    }
}

int CWinTranslatorApp::Run() {
    // Initialize common controls
    InitCommonControls();
    
    if (!InitInstance()) {
        return FALSE;
    }
    
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}

bool CWinTranslatorApp::InitInstance() {
    // Register main window class
    WNDCLASSEX wcex = {0};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = MainWndProc;
    wcex.hInstance = m_hInstance;
    wcex.lpszClassName = L"WinTranslatorMainWnd";
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_MAINICON));
    wcex.hIconSm = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_MAINICON));
    
    if (!RegisterClassEx(&wcex)) {
        return false;
    }
    
    // Register overlay window class
    WNDCLASSEX wcexOverlay = {0};
    wcexOverlay.cbSize = sizeof(WNDCLASSEX);
    wcexOverlay.lpfnWndProc = OverlayWndProc;
    wcexOverlay.hInstance = m_hInstance;
    wcexOverlay.lpszClassName = L"WinTranslatorOverlay";
    wcexOverlay.hbrBackground = nullptr;
    wcexOverlay.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcexOverlay.style = CS_HREDRAW | CS_VREDRAW;
    
    if (!RegisterClassEx(&wcexOverlay)) {
        return false;
    }
    
    // Create main window (hidden but functional)
    m_hwndMain = CreateWindow(
        L"WinTranslatorMainWnd",
        L"Win Translator",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 200, 100,
        nullptr, nullptr, m_hInstance, nullptr);
    
    // Hide it immediately
    ShowWindow(m_hwndMain, SW_HIDE);
    
    if (!m_hwndMain) {
        return false;
    }
    
    InitTrayIcon();
    RegisterHotkeys();
    
    return true;
}

void CWinTranslatorApp::InitTrayIcon() {
    m_nid.cbSize = sizeof(NOTIFYICONDATA);
    m_nid.hWnd = m_hwndMain;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_TRAYICON;
    
    // Use custom icon
    m_nid.hIcon = LoadIcon(m_hInstance, MAKEINTRESOURCE(IDI_MAINICON));
    if (!m_nid.hIcon) {
        // Fallback to default icon if custom icon fails
        m_nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    }
    wcscpy_s(m_nid.szTip, L"Win Translator");
    
    Shell_NotifyIcon(NIM_ADD, &m_nid);
}

LRESULT CALLBACK CWinTranslatorApp::MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP) {
                s_pInstance->ShowContextMenu();
            }
            break;
            
        case WM_HOTKEY:
            switch (wParam) {
                case HOTKEY_SMART_TRANSLATE_ID:
                    s_pInstance->HandleSmartTranslateHotkey();
                    break;
                case HOTKEY_HIDE_ID:
                    s_pInstance->HandleHideHotkey();
                    break;
            }
            break;
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDM_OPTIONS:
                    s_pInstance->ShowOptionsDialog();
                    break;
                case IDM_EXIT:
                    PostQuitMessage(0);
                    break;
            }
            break;
            
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void CWinTranslatorApp::ShowContextMenu() {
    POINT pt;
    GetCursorPos(&pt);
    
    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, IDM_OPTIONS, L"Options");
    AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenu(hMenu, MF_STRING, IDM_EXIT, L"Exit");
    
    SetForegroundWindow(m_hwndMain);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, m_hwndMain, nullptr);
    DestroyMenu(hMenu);
}

void CWinTranslatorApp::RegisterHotkeys() {
    // Register hotkeys using saved settings
    RegisterHotKey(m_hwndMain, HOTKEY_SMART_TRANSLATE_ID, 
                   m_settings.translateModifiers, m_settings.translateKey);
    RegisterHotKey(m_hwndMain, HOTKEY_HIDE_ID, 
                   m_settings.hideModifiers, m_settings.hideKey);
}

void CWinTranslatorApp::UnregisterHotkeys() {
    UnregisterHotKey(m_hwndMain, HOTKEY_SMART_TRANSLATE_ID);
    UnregisterHotKey(m_hwndMain, HOTKEY_HIDE_ID);
}

void CWinTranslatorApp::HandleSmartTranslateHotkey() {
    // Close any existing overlay first
    HideTranslationOverlay();
    
    // Get text from clipboard (manually copied by user)
    std::wstring clipboardText = GetClipboardText();
    if (clipboardText.empty()) {
        return; // Nothing to translate
    }
    
    // Call AI API with JSON request (includes language detection + translation)
    std::wstring jsonResponse;
    int maxRetries = 3;
    
    for (int retry = 0; retry < maxRetries; retry++) {
        if (retry > 0) {
            Sleep(1000); // Wait 1 second between retries
        }
        
        jsonResponse = CallTranslationAPI(clipboardText, L"auto");
        
        // Check if response contains error
        if (jsonResponse.find(L"error code") != std::wstring::npos) {
            continue; // Retry
        }
        
        // Check if response looks like valid JSON
        if (jsonResponse.find(L"{") != std::wstring::npos && 
            jsonResponse.find(L"translation") != std::wstring::npos) {
            break; // Success
        }
    }
    
    if (!jsonResponse.empty() && jsonResponse.find(L"error code") == std::wstring::npos) {
        // Parse JSON response
        bool isSourceLanguage = ExtractJsonBool(jsonResponse, L"is_source_language");
        std::wstring translation = ExtractJsonValue(jsonResponse, L"translation");
        
        if (!translation.empty()) {
            // Always show overlay and save to clipboard regardless of language direction
            ShowTranslationOverlay(translation);
            SaveToClipboard(translation);
        } else {
            ShowTranslationOverlay(L"Translation failed - please try again");
        }
    } else {
        ShowTranslationOverlay(L"API Error - please check internet connection");
    }
}

std::wstring CWinTranslatorApp::GetClipboardText() {
    std::wstring result;
    
    if (OpenClipboard(m_hwndMain)) {
        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (hData) {
            wchar_t* pData = (wchar_t*)GlobalLock(hData);
            if (pData) {
                result = pData;
            }
            GlobalUnlock(hData);
        }
        CloseClipboard();
    }
    
    return result;
}

bool CWinTranslatorApp::IsTextInSourceLanguage(const std::wstring& text) {
    // Simple heuristic: if source language is English, check for English patterns
    // If source language is Turkish, check for Turkish patterns
    
    if (m_settings.sourceLang == L"English" || m_settings.sourceLang == L"english") {
        // Check for English patterns
        std::wstring lowerText = text;
        std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::towlower);
        
        // Common English words
        std::vector<std::wstring> englishWords = {
            L"the", L"and", L"for", L"are", L"but", L"not", L"you", L"all", L"can", L"had", L"her", L"was", L"one", L"our", L"out", L"day", L"get", L"has", L"him", L"his", L"how", L"its", L"may", L"new", L"now", L"old", L"see", L"two", L"who", L"boy", L"did", L"man", L"men", L"put", L"say", L"she", L"too", L"use"
        };
        
        for (const auto& word : englishWords) {
            if (lowerText.find(L" " + word + L" ") != std::wstring::npos || 
                lowerText.find(word + L" ") == 0 ||
                lowerText.find(L" " + word) == lowerText.length() - word.length() - 1) {
                return true;
            }
        }
    } else if (m_settings.sourceLang == L"Turkish" || m_settings.sourceLang == L"turkish") {
        // Check for Turkish patterns - Turkish specific characters
        if (text.find_first_of(L"ğüşıöçĞÜŞIİÖÇ") != std::wstring::npos) {
            return true;
        }
        
        // Common Turkish words
        std::wstring lowerText = text;
        std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::towlower);
        
        std::vector<std::wstring> turkishWords = {
            L"bir", L"bu", L"da", L"de", L"en", L"ve", L"ki", L"mu", L"mı", L"ile", L"için", L"olan", L"her", L"ne", L"ya", L"çok", L"daha", L"sonra", L"kadar", L"böyle", L"şey", L"hem", L"hep"
        };
        
        for (const auto& word : turkishWords) {
            if (lowerText.find(L" " + word + L" ") != std::wstring::npos || 
                lowerText.find(word + L" ") == 0 ||
                lowerText.find(L" " + word) == lowerText.length() - word.length() - 1) {
                return true;
            }
        }
    }
    
    return false; // Default: assume target language
}

void CWinTranslatorApp::SaveToClipboard(const std::wstring& text) {
    if (OpenClipboard(m_hwndMain)) {
        EmptyClipboard();
        size_t size = (text.length() + 1) * sizeof(wchar_t);
        HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE, size);
        if (hClipboardData) {
            wchar_t* pData = (wchar_t*)GlobalLock(hClipboardData);
            wcscpy_s(pData, text.length() + 1, text.c_str());
            GlobalUnlock(hClipboardData);
            SetClipboardData(CF_UNICODETEXT, hClipboardData);
        }
        CloseClipboard();
    }
}

std::wstring CWinTranslatorApp::ExtractJsonValue(const std::wstring& json, const std::wstring& key) {
    // Simple JSON value extraction
    std::wstring searchKey = L"\"" + key + L"\":";    
    size_t keyPos = json.find(searchKey);
    if (keyPos == std::wstring::npos) return L"";
    
    size_t valueStart = json.find(L":", keyPos) + 1;
    while (valueStart < json.length() && (json[valueStart] == L' ' || json[valueStart] == L'\t')) {
        valueStart++;
    }
    
    if (valueStart >= json.length()) return L"";
    
    if (json[valueStart] == L'"') {
        // String value
        valueStart++;
        size_t valueEnd = json.find(L'"', valueStart);
        if (valueEnd == std::wstring::npos) return L"";
        return json.substr(valueStart, valueEnd - valueStart);
    } else {
        // Boolean or other value
        size_t valueEnd = json.find_first_of(L",}", valueStart);
        if (valueEnd == std::wstring::npos) valueEnd = json.length();
        std::wstring value = json.substr(valueStart, valueEnd - valueStart);
        
        // Trim whitespace
        while (!value.empty() && (value.back() == L' ' || value.back() == L'\t' || value.back() == L'\n' || value.back() == L'\r')) {
            value.pop_back();
        }
        return value;
    }
}

bool CWinTranslatorApp::ExtractJsonBool(const std::wstring& json, const std::wstring& key) {
    std::wstring value = ExtractJsonValue(json, key);
    return (value == L"true");
}

void CWinTranslatorApp::HandleHideHotkey() {
    HideTranslationOverlay();
}

std::wstring CWinTranslatorApp::GetSelectedText() {
    std::wstring result;
    std::wstring beforeClipboard;
    
    // Get current clipboard content (before we send Ctrl+C)
    if (OpenClipboard(m_hwndMain)) {
        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (hData) {
            wchar_t* pData = (wchar_t*)GlobalLock(hData);
            if (pData) {
                beforeClipboard = pData;
            }
            GlobalUnlock(hData);
        }
        CloseClipboard();
    }
    
    // Focus the foreground window to ensure Ctrl+C works
    HWND foregroundWnd = GetForegroundWindow();
    if (foregroundWnd) {
        SetForegroundWindow(foregroundWnd);
        Sleep(50);
    }
    
    // Send Ctrl+C with SendInput (more reliable)
    INPUT inputs[4] = {};
    
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;
    inputs[0].ki.dwFlags = 0;
    
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 'C';
    inputs[1].ki.dwFlags = 0;
    
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 'C';
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_CONTROL;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    
    UINT sentInputs = SendInput(4, inputs, sizeof(INPUT));
    
    // If SendInput failed, try alternative method
    if (sentInputs == 0) {
        keybd_event(VK_CONTROL, 0, 0, 0);
        keybd_event('C', 0, 0, 0);
        keybd_event('C', 0, KEYEVENTF_KEYUP, 0);
        keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);
    }
    
    // Wait for clipboard to be updated (longer for slow apps)
    Sleep(500);
    
    // Try multiple times to get clipboard content (some apps are slow)
    for (int attempt = 0; attempt < 3 && result.empty(); attempt++) {
        if (attempt > 0) Sleep(200); // Additional wait for retries
        
        if (OpenClipboard(m_hwndMain)) {
            HANDLE hData = GetClipboardData(CF_UNICODETEXT);
            if (hData) {
                wchar_t* pData = (wchar_t*)GlobalLock(hData);
                if (pData && wcslen(pData) > 0) {
                    std::wstring afterClipboard = pData;
                    
                    // Use new clipboard content if it's different OR if we had empty before
                    if (beforeClipboard != afterClipboard) {
                        result = afterClipboard;
                    }
                }
                GlobalUnlock(hData);
            }
            CloseClipboard();
        }
    }
    
    
    return result;
}

std::wstring CWinTranslatorApp::URLEncode(const std::wstring& text) {
    std::wstring result;
    
    // Convert to UTF-8 first
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    char* utf8Buffer = new char[utf8Len];
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, utf8Buffer, utf8Len, nullptr, nullptr);
    
    // URL encode the UTF-8 bytes
    for (int i = 0; i < utf8Len - 1; i++) {
        unsigned char c = utf8Buffer[i];
        
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || 
            (c >= '0' && c <= '9') || c == '-' || c == '_' || 
            c == '.' || c == '~') {
            result += (wchar_t)c;
        } else if (c == ' ') {
            result += L"%20";
        } else {
            wchar_t buf[4];
            swprintf_s(buf, L"%%%02X", c);
            result += buf;
        }
    }
    
    delete[] utf8Buffer;
    return result;
}

std::wstring CWinTranslatorApp::CallTranslationAPI(const std::wstring& text, const std::wstring& targetLang) {
    std::wstring result;
    
    // Create JSON request prompt for AI
    std::wstring prompt = L"Detect if the following text is in " + m_settings.sourceLang + 
                         L" (return true) or " + m_settings.targetLang + L" (return false). " +
                         L"Then translate it to the opposite language. " +
                         L"Respond ONLY in JSON format: {\"is_source_language\": true/false, \"translation\": \"translated text\"}. " +
                         L"Text: " + text;
    std::wstring encodedPrompt = URLEncode(prompt);
    
    std::wstring url = L"https://text.pollinations.ai/" + encodedPrompt;
    
    // Convert to ANSI for WinINet
    int len = WideCharToMultiByte(CP_ACP, 0, url.c_str(), -1, nullptr, 0, nullptr, nullptr);
    char* urlA = new char[len];
    WideCharToMultiByte(CP_ACP, 0, url.c_str(), -1, urlA, len, nullptr, nullptr);
    
    HINTERNET hInternet = InternetOpenA("WinTranslator/1.0", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (hInternet) {
        HINTERNET hUrl = InternetOpenUrlA(hInternet, urlA, nullptr, 0, INTERNET_FLAG_RELOAD, 0);
        if (hUrl) {
            char buffer[4096];
            DWORD bytesRead;
            std::string response;
            
            while (InternetReadFile(hUrl, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                response += buffer;
            }
            
            InternetCloseHandle(hUrl);
            
            // Convert response to wide string
            if (!response.empty()) {
                int wlen = MultiByteToWideChar(CP_UTF8, 0, response.c_str(), -1, nullptr, 0);
                wchar_t* wbuffer = new wchar_t[wlen];
                MultiByteToWideChar(CP_UTF8, 0, response.c_str(), -1, wbuffer, wlen);
                result = wbuffer;
                delete[] wbuffer;
            }
        }
        InternetCloseHandle(hInternet);
    }
    
    delete[] urlA;
    return result;
}

void CWinTranslatorApp::ShowTranslationOverlay(const std::wstring& text) {
    m_overlayText = text;
    
    // Calculate text size for dynamic sizing using the same font we'll render with
    HDC hdc = GetDC(nullptr);
    
    // Create the same font we'll use for drawing
    HFONT hFont = CreateFont(
        -16,                    // Height (16pt)
        0,                      // Width (auto)
        0,                      // Escapement
        0,                      // Orientation
        FW_NORMAL,              // Weight
        FALSE,                  // Italic
        FALSE,                  // Underline
        FALSE,                  // StrikeOut
        DEFAULT_CHARSET,        // CharSet
        OUT_DEFAULT_PRECIS,     // OutPrecision
        CLIP_DEFAULT_PRECIS,    // ClipPrecision
        CLEARTYPE_QUALITY,      // Quality (ClearType)
        DEFAULT_PITCH | FF_DONTCARE, // PitchAndFamily
        L"Segoe UI");
    
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    
    RECT measureRect = {0, 0, 450, 0}; // Max width 450
    DrawText(hdc, text.c_str(), -1, &measureRect, DT_CALCRECT | DT_WORDBREAK);
    
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
    ReleaseDC(nullptr, hdc);
    
    int textWidth = measureRect.right - measureRect.left + 40; // padding
    int textHeight = measureRect.bottom - measureRect.top + 40; // padding
    
    // Min/Max constraints - allow larger overlays for long text
    int windowWidth = std::max(200, std::min(600, textWidth));
    int windowHeight = std::max(60, std::min(400, textHeight));
    
    if (!m_hwndOverlay) {
        // Create overlay window
        m_hwndOverlay = CreateWindowEx(
            WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            L"WinTranslatorOverlay",
            L"Translation",
            WS_POPUP,
            0, 0, windowWidth, windowHeight,
            nullptr, nullptr, m_hInstance, nullptr);
    }
    
    if (m_hwndOverlay) {
        
        // Position at bottom of screen
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        
        // Resize existing window to fit text
        SetWindowPos(m_hwndOverlay, HWND_TOPMOST,
                     (screenWidth - windowWidth) / 2,
                     screenHeight - windowHeight - 100,
                     windowWidth, windowHeight,
                     SWP_SHOWWINDOW | SWP_NOACTIVATE);
        
        // Make only white background transparent, no opacity
        SetLayeredWindowAttributes(m_hwndOverlay, RGB(255, 255, 255), 0, LWA_COLORKEY);
        
        // Force repaint
        InvalidateRect(m_hwndOverlay, nullptr, TRUE);
        
        m_overlayVisible = true;
    }
}

void CWinTranslatorApp::HideTranslationOverlay() {
    if (m_hwndOverlay && m_overlayVisible) {
        ShowWindow(m_hwndOverlay, SW_HIDE);
        m_overlayVisible = false;
    }
}

LRESULT CALLBACK CWinTranslatorApp::OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // Get text from instance
            const wchar_t* text = s_pInstance ? s_pInstance->m_overlayText.c_str() : nullptr;
            
            if (text && wcslen(text) > 0) {
                RECT rect;
                GetClientRect(hwnd, &rect);
                
                // Clear background to white (for colorkey transparency)
                HBRUSH hWhiteBrush = CreateSolidBrush(RGB(255, 255, 255));
                FillRect(hdc, &rect, hWhiteBrush);
                DeleteObject(hWhiteBrush);
                
                // Create rounded rectangle with solid background
                HBRUSH hBrush = CreateSolidBrush(RGB(245, 245, 245)); // Light gray background
                HPEN hPen = CreatePen(PS_SOLID, 2, RGB(180, 180, 180)); // Gray border
                HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
                HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                
                RoundRect(hdc, 10, 10, rect.right - 10, rect.bottom - 10, 15, 15);
                
                SelectObject(hdc, hOldBrush);
                SelectObject(hdc, hOldPen);
                DeleteObject(hBrush);
                DeleteObject(hPen);
                
                // Set text properties - dark text on light background
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(30, 30, 30)); // Dark text
                
                // Create and select a better font (Segoe UI)
                HFONT hFont = CreateFont(
                    -16,                    // Height (16pt)
                    0,                      // Width (auto)
                    0,                      // Escapement
                    0,                      // Orientation
                    FW_NORMAL,              // Weight
                    FALSE,                  // Italic
                    FALSE,                  // Underline
                    FALSE,                  // StrikeOut
                    DEFAULT_CHARSET,        // CharSet
                    OUT_DEFAULT_PRECIS,     // OutPrecision
                    CLIP_DEFAULT_PRECIS,    // ClipPrecision
                    CLEARTYPE_QUALITY,      // Quality (ClearType)
                    DEFAULT_PITCH | FF_DONTCARE, // PitchAndFamily
                    L"Segoe UI");
                
                HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
                
                // Draw text
                RECT textRect = {20, 20, rect.right - 20, rect.bottom - 20};
                DrawText(hdc, text, -1, &textRect, DT_WORDBREAK | DT_CENTER | DT_VCENTER);
                
                // Restore old font and cleanup
                SelectObject(hdc, hOldFont);
                DeleteObject(hFont);
            }
            
            EndPaint(hwnd, &ps);
            break;
        }
        
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void CWinTranslatorApp::LoadSettings() {
    wchar_t appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, appDataPath))) {
        std::wstring settingsDir = std::wstring(appDataPath) + L"\\WinTranslator";
        CreateDirectory(settingsDir.c_str(), nullptr);
        
        std::wstring settingsFile = settingsDir + L"\\settings.txt";
        std::wifstream file(settingsFile);
        
        if (file.is_open()) {
            std::wstring line;
            while (std::getline(file, line)) {
                size_t pos = line.find(L'=');
                if (pos != std::wstring::npos) {
                    std::wstring key = line.substr(0, pos);
                    std::wstring value = line.substr(pos + 1);
                    
                    if (key == L"SourceLang") {
                        m_settings.sourceLang = value;
                    } else if (key == L"TargetLang") {
                        m_settings.targetLang = value;
                    } else if (key == L"TranslateHotkey") {
                        m_settings.translateHotkey = _wtoi(value.c_str());
                    } else if (key == L"ShowHotkey") {
                        m_settings.showHotkey = _wtoi(value.c_str());
                    } else if (key == L"HideHotkey") {
                        m_settings.hideHotkey = _wtoi(value.c_str());
                    } else if (key == L"TranslateModifiers") {
                        m_settings.translateModifiers = _wtoi(value.c_str());
                    } else if (key == L"TranslateKey") {
                        m_settings.translateKey = _wtoi(value.c_str());
                    } else if (key == L"HideModifiers") {
                        m_settings.hideModifiers = _wtoi(value.c_str());
                    } else if (key == L"HideKey") {
                        m_settings.hideKey = _wtoi(value.c_str());
                    }
                }
            }
            file.close();
        }
    }
}

void CWinTranslatorApp::SaveSettings() {
    wchar_t appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, appDataPath))) {
        std::wstring settingsDir = std::wstring(appDataPath) + L"\\WinTranslator";
        CreateDirectory(settingsDir.c_str(), nullptr);
        
        std::wstring settingsFile = settingsDir + L"\\settings.txt";
        std::wofstream file(settingsFile);
        
        if (file.is_open()) {
            file << L"SourceLang=" << m_settings.sourceLang << std::endl;
            file << L"TargetLang=" << m_settings.targetLang << std::endl;
            file << L"TranslateHotkey=" << m_settings.translateHotkey << std::endl;
            file << L"ShowHotkey=" << m_settings.showHotkey << std::endl;
            file << L"HideHotkey=" << m_settings.hideHotkey << std::endl;
            
            // New hotkey settings
            file << L"TranslateModifiers=" << m_settings.translateModifiers << std::endl;
            file << L"TranslateKey=" << m_settings.translateKey << std::endl;
            file << L"HideModifiers=" << m_settings.hideModifiers << std::endl;
            file << L"HideKey=" << m_settings.hideKey << std::endl;
            file.close();
            
            // Debug: Show saved settings path
            std::wstring debugMsg = L"Settings saved to: " + settingsFile + L"\n\n";
            debugMsg += L"Source: " + m_settings.sourceLang + L"\n";
            debugMsg += L"Target: " + m_settings.targetLang;
            // MessageBox(m_hwndMain, debugMsg.c_str(), L"Debug: Settings Saved", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBox(m_hwndMain, L"Failed to save settings file!", L"Error", MB_OK | MB_ICONERROR);
        }
    } else {
        MessageBox(m_hwndMain, L"Failed to get AppData path!", L"Error", MB_OK | MB_ICONERROR);
    }
}

void CWinTranslatorApp::ShowOptionsDialog() {
    CSimpleSettingsDialog dlg;
    
    // Load current settings into dialog
    dlg.SetSourceLanguage(m_settings.sourceLang);
    dlg.SetTargetLanguage(m_settings.targetLang);
    
    // Set current hotkeys (load from saved settings)
    dlg.SetHotkeys(m_settings.translateModifiers, m_settings.translateKey, 
                   m_settings.hideModifiers, m_settings.hideKey);
    
    // Show dialog
    INT_PTR result = dlg.ShowModal(m_hwndMain);
    
    if (result == IDOK) {
        // User clicked Apply, save the settings
        m_settings.sourceLang = dlg.GetSourceLanguage();
        m_settings.targetLang = dlg.GetTargetLanguage();
        
        // Get hotkey settings and store them
        UINT translateMod = dlg.GetTranslateModifiers();
        UINT translateKey = dlg.GetTranslateKey();
        UINT hideMod = dlg.GetHideModifiers();
        UINT hideKey = dlg.GetHideKey();
        
        // Update internal settings structure with new hotkey values
        m_settings.translateModifiers = translateMod;
        m_settings.translateKey = translateKey;
        m_settings.hideModifiers = hideMod;
        m_settings.hideKey = hideKey;
        
        // Save to file
        SaveSettings();
        
        // Re-register hotkeys with new values
        UnregisterHotkeys();
        RegisterHotkeys();
        
        // Show confirmation with hotkey info
        std::wstring confirmMsg = L"Settings applied successfully!\n\n";
        confirmMsg += L"Source Language: " + m_settings.sourceLang + L"\n";
        confirmMsg += L"Target Language: " + m_settings.targetLang + L"\n\n";
        confirmMsg += L"Hotkeys updated and active:\n";
        
        // Format hotkey display
        std::wstring translateHotkeyStr;
        if (translateMod & MOD_CONTROL) translateHotkeyStr += L"Ctrl+";
        if (translateMod & MOD_SHIFT) translateHotkeyStr += L"Shift+";
        if (translateMod & MOD_ALT) translateHotkeyStr += L"Alt+";
        if (translateKey >= 'A' && translateKey <= 'Z') {
            translateHotkeyStr += (wchar_t)translateKey;
        } else if (translateKey >= '0' && translateKey <= '9') {
            translateHotkeyStr += (wchar_t)translateKey;
        } else {
            translateHotkeyStr += L"Key" + std::to_wstring(translateKey);
        }
        
        std::wstring hideHotkeyStr;
        if (hideMod & MOD_CONTROL) hideHotkeyStr += L"Ctrl+";
        if (hideMod & MOD_SHIFT) hideHotkeyStr += L"Shift+";
        if (hideMod & MOD_ALT) hideHotkeyStr += L"Alt+";
        if (hideKey == VK_ESCAPE) {
            hideHotkeyStr += L"ESC";
        } else if (hideKey >= 'A' && hideKey <= 'Z') {
            hideHotkeyStr += (wchar_t)hideKey;
        } else if (hideKey >= '0' && hideKey <= '9') {
            hideHotkeyStr += (wchar_t)hideKey;
        } else {
            hideHotkeyStr += L"Key" + std::to_wstring(hideKey);
        }
        
        confirmMsg += L"• Translate: " + translateHotkeyStr + L"\n";
        confirmMsg += L"• Hide: " + hideHotkeyStr + L"\n\n";
        confirmMsg += L"Hotkeys are now active - no restart required!";
        
        MessageBox(m_hwndMain, confirmMsg.c_str(), L"Settings Applied", MB_OK | MB_ICONINFORMATION);
    }
}

