#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <shlobj.h>
#include <wininet.h>
#include <string>
#include <map>
#include <vector>

// Resource IDs
#define IDI_MAINICON        101
#define IDI_TRAYICON       102
#define IDM_OPTIONS        201
#define IDM_EXIT           202
#define IDD_OPTIONS        301
#define IDC_TAB_CONTROL    302
#define IDC_LANG_FROM      303
#define IDC_LANG_TO        304
#define IDC_HOTKEY1        305
#define IDC_HOTKEY2        306
#define IDC_HOTKEY3        307

// Messages
#define WM_TRAYICON        (WM_USER + 1)
#define WM_HOTKEY_TRANSLATE    (WM_USER + 2)
#define WM_HOTKEY_SHOW         (WM_USER + 3)
#define WM_HOTKEY_HIDE         (WM_USER + 4)

// Hotkey IDs
#define HOTKEY_SMART_TRANSLATE_ID   1
#define HOTKEY_HIDE_ID             2

struct AppSettings {
    std::wstring sourceLang = L"English";
    std::wstring targetLang = L"Turkish";
    UINT translateHotkey = '1';  // Will be used with MOD_CONTROL | MOD_SHIFT
    UINT showHotkey = '2';       // Will be used with MOD_CONTROL | MOD_SHIFT  
    UINT hideHotkey = VK_ESCAPE; // Will be used with no modifiers
    
    // New hotkey settings for dialog
    UINT translateModifiers = MOD_CONTROL | MOD_SHIFT;
    UINT translateKey = '2';
    UINT hideModifiers = 0;
    UINT hideKey = VK_ESCAPE;
};

class CWinTranslatorApp {
public:
    CWinTranslatorApp();
    ~CWinTranslatorApp();
    
    int Run();
    
private:
    static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    
    bool InitInstance();
    void InitTrayIcon();
    void ShowContextMenu();
    void ShowOptionsDialog();
    void SaveSettings();
    void LoadSettings();
    void RegisterHotkeys();
    void UnregisterHotkeys();
    void HandleSmartTranslateHotkey();
    void HandleHideHotkey();
    std::wstring GetSelectedText();
    std::wstring GetClipboardText();
    bool IsTextInSourceLanguage(const std::wstring& text);
    void SaveToClipboard(const std::wstring& text);
    std::wstring ExtractJsonValue(const std::wstring& json, const std::wstring& key);
    bool ExtractJsonBool(const std::wstring& json, const std::wstring& key);
    std::wstring CallTranslationAPI(const std::wstring& text, const std::wstring& targetLang);
    void ShowTranslationOverlay(const std::wstring& text);
    void HideTranslationOverlay();
    std::wstring URLEncode(const std::wstring& text);
    
private:
    HINSTANCE m_hInstance;
    HWND m_hwndMain;
    HWND m_hwndOverlay;
    NOTIFYICONDATA m_nid;
    AppSettings m_settings;
    bool m_overlayVisible;
    std::wstring m_overlayText;
    
    static CWinTranslatorApp* s_pInstance;
};
