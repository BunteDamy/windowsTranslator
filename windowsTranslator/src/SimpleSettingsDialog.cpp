#include "SimpleSettingsDialog.h"
#include "../resources/resource.h"

// Hotkey control messages
#ifndef HKM_SETHOTKEY
#define HKM_SETHOTKEY (WM_USER + 1)
#endif

#ifndef HKM_GETHOTKEY
#define HKM_GETHOTKEY (WM_USER + 2)
#endif

CSimpleSettingsDialog::CSimpleSettingsDialog()
    : m_hwndDlg(nullptr)
{
    m_sourceLanguage = L"English";
    m_targetLanguage = L"Turkish";
    
    // Default hotkeys
    m_translateModifiers = MOD_CONTROL | MOD_SHIFT;
    m_translateKey = '2';
    m_hideModifiers = 0;
    m_hideKey = VK_ESCAPE;
}

CSimpleSettingsDialog::~CSimpleSettingsDialog()
{
}

INT_PTR CSimpleSettingsDialog::ShowModal(HWND hwndParent)
{
    return DialogBoxParam(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_SETTINGS), 
                         hwndParent, DialogProc, reinterpret_cast<LPARAM>(this));
}

INT_PTR CALLBACK CSimpleSettingsDialog::DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CSimpleSettingsDialog* pThis = nullptr;
    
    if (uMsg == WM_INITDIALOG) {
        pThis = reinterpret_cast<CSimpleSettingsDialog*>(lParam);
        SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
        pThis->m_hwndDlg = hwndDlg;
    } else {
        pThis = reinterpret_cast<CSimpleSettingsDialog*>(GetWindowLongPtr(hwndDlg, DWLP_USER));
    }
    
    if (!pThis) return FALSE;
    
    switch (uMsg)
    {
        case WM_INITDIALOG:
            return pThis->OnInitDialog(hwndDlg);
            
        case WM_COMMAND:
            return pThis->OnCommand(hwndDlg, wParam, lParam);
            
        case WM_CLOSE:
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;
    }
    
    return FALSE;
}

BOOL CSimpleSettingsDialog::OnInitDialog(HWND hwndDlg)
{
    // Set dialog title
    SetWindowText(hwndDlg, L"Win Translator - Settings");
    
    // Set language values
    SetDlgItemText(hwndDlg, IDC_SOURCE_LANG, m_sourceLanguage.c_str());
    SetDlgItemText(hwndDlg, IDC_TARGET_LANG, m_targetLanguage.c_str());
    
    // Set hotkey controls (Windows hotkey controls)
    HWND hHotkeyTranslate = GetDlgItem(hwndDlg, IDC_HOTKEY_TRANSLATE);
    HWND hHotkeyHide = GetDlgItem(hwndDlg, IDC_HOTKEY_HIDE);
    
    // Convert RegisterHotKey modifiers to hotkey control modifiers
    BYTE translateModForControl = 0;
    if (m_translateModifiers & MOD_SHIFT) translateModForControl |= 0x01;   // HOTKEYF_SHIFT
    if (m_translateModifiers & MOD_CONTROL) translateModForControl |= 0x02; // HOTKEYF_CONTROL
    if (m_translateModifiers & MOD_ALT) translateModForControl |= 0x04;     // HOTKEYF_ALT
    
    BYTE hideModForControl = 0;
    if (m_hideModifiers & MOD_SHIFT) hideModForControl |= 0x01;   // HOTKEYF_SHIFT
    if (m_hideModifiers & MOD_CONTROL) hideModForControl |= 0x02; // HOTKEYF_CONTROL
    if (m_hideModifiers & MOD_ALT) hideModForControl |= 0x04;     // HOTKEYF_ALT
    
    // Set current hotkey combinations
    WORD translateHotkey = MAKEWORD(m_translateKey, translateModForControl);
    WORD hideHotkey = MAKEWORD(m_hideKey, hideModForControl);
    
    SendMessage(hHotkeyTranslate, HKM_SETHOTKEY, translateHotkey, 0);
    SendMessage(hHotkeyHide, HKM_SETHOTKEY, hideHotkey, 0);
    
    // Position dialog on the right side of screen, centered vertically
    RECT rcDlg;
    GetWindowRect(hwndDlg, &rcDlg);
    
    int dialogWidth = rcDlg.right - rcDlg.left;
    int dialogHeight = rcDlg.bottom - rcDlg.top;
    
    // Get screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // Position: Right side with some margin, vertically centered
    int x = screenWidth - dialogWidth - 50;  // 50px margin from right edge
    int y = (screenHeight - dialogHeight) / 2; // Vertically centered
    
    SetWindowPos(hwndDlg, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    
    return TRUE;
}

BOOL CSimpleSettingsDialog::OnCommand(HWND hwndDlg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    
    switch (LOWORD(wParam))
    {
        case IDC_APPLY:
            OnOK(hwndDlg);
            return TRUE;
            
        case IDCANCEL:
        case IDC_CANCEL:
            OnCancel(hwndDlg);
            return TRUE;
    }
    
    return FALSE;
}

void CSimpleSettingsDialog::OnOK(HWND hwndDlg)
{
    // Get language settings from controls
    wchar_t buffer[256];
    
    GetDlgItemText(hwndDlg, IDC_SOURCE_LANG, buffer, 256);
    m_sourceLanguage = buffer;
    
    GetDlgItemText(hwndDlg, IDC_TARGET_LANG, buffer, 256);
    m_targetLanguage = buffer;
    
    // Get hotkey settings from hotkey controls
    HWND hHotkeyTranslate = GetDlgItem(hwndDlg, IDC_HOTKEY_TRANSLATE);
    HWND hHotkeyHide = GetDlgItem(hwndDlg, IDC_HOTKEY_HIDE);
    
    WORD translateHotkey = (WORD)SendMessage(hHotkeyTranslate, HKM_GETHOTKEY, 0, 0);
    WORD hideHotkey = (WORD)SendMessage(hHotkeyHide, HKM_GETHOTKEY, 0, 0);
    
    // Extract key and modifiers
    BYTE translateKeyRaw = LOBYTE(translateHotkey);
    BYTE translateModRaw = HIBYTE(translateHotkey);
    BYTE hideKeyRaw = LOBYTE(hideHotkey);
    BYTE hideModRaw = HIBYTE(hideHotkey);
    
    // Convert hotkey control modifiers to RegisterHotKey modifiers
    m_translateKey = translateKeyRaw;
    m_translateModifiers = 0;
    if (translateModRaw & 0x01) m_translateModifiers |= MOD_SHIFT;   // HOTKEYF_SHIFT
    if (translateModRaw & 0x02) m_translateModifiers |= MOD_CONTROL; // HOTKEYF_CONTROL
    if (translateModRaw & 0x04) m_translateModifiers |= MOD_ALT;     // HOTKEYF_ALT
    
    m_hideKey = hideKeyRaw;
    m_hideModifiers = 0;
    if (hideModRaw & 0x01) m_hideModifiers |= MOD_SHIFT;   // HOTKEYF_SHIFT
    if (hideModRaw & 0x02) m_hideModifiers |= MOD_CONTROL; // HOTKEYF_CONTROL
    if (hideModRaw & 0x04) m_hideModifiers |= MOD_ALT;     // HOTKEYF_ALT
    
    // Hotkey conversion completed
    
    // Validate input
    if (m_sourceLanguage.empty()) {
        MessageBox(hwndDlg, L"Source language cannot be empty!", L"Error", MB_OK | MB_ICONWARNING);
        return;
    }
    
    if (m_targetLanguage.empty()) {
        MessageBox(hwndDlg, L"Target language cannot be empty!", L"Error", MB_OK | MB_ICONWARNING);
        return;
    }
    
    EndDialog(hwndDlg, IDOK);
}

void CSimpleSettingsDialog::OnCancel(HWND hwndDlg)
{
    EndDialog(hwndDlg, IDCANCEL);
}

std::wstring CSimpleSettingsDialog::FormatHotkey(UINT modifiers, UINT key)
{
    std::wstring result;
    
    if (modifiers & MOD_CONTROL)
        result += L"Ctrl+";
    if (modifiers & MOD_SHIFT)
        result += L"Shift+";
    if (modifiers & MOD_ALT)
        result += L"Alt+";
    if (modifiers & MOD_WIN)
        result += L"Win+";
    
    // Format the key
    if (key >= 'A' && key <= 'Z')
    {
        result += static_cast<wchar_t>(key);
    }
    else if (key >= '0' && key <= '9')
    {
        result += static_cast<wchar_t>(key);
    }
    else
    {
        switch (key)
        {
            case VK_ESCAPE: result += L"ESC"; break;
            case VK_F1: result += L"F1"; break;
            case VK_F2: result += L"F2"; break;
            case VK_F3: result += L"F3"; break;
            case VK_F4: result += L"F4"; break;
            case VK_F5: result += L"F5"; break;
            case VK_F6: result += L"F6"; break;
            case VK_F7: result += L"F7"; break;
            case VK_F8: result += L"F8"; break;
            case VK_F9: result += L"F9"; break;
            case VK_F10: result += L"F10"; break;
            case VK_F11: result += L"F11"; break;
            case VK_F12: result += L"F12"; break;
            case VK_SPACE: result += L"SPACE"; break;
            case VK_TAB: result += L"TAB"; break;
            case VK_RETURN: result += L"ENTER"; break;
            default:
                result += L"Key" + std::to_wstring(key);
                break;
        }
    }
    
    return result;
}

