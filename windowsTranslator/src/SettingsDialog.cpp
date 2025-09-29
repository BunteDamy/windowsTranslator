#include "SettingsDialog.h"
#include <sstream>

CSettingsDialog::CSettingsDialog(UINT nResID) : CDialog(nResID)
{
    // Set default values
    m_sourceLanguage = L"English";
    m_targetLanguage = L"Turkish";
    m_hotkeySettings.translateModifiers = MOD_CONTROL | MOD_SHIFT;
    m_hotkeySettings.translateKey = '2';
    m_hotkeySettings.hideModifiers = 0;
    m_hotkeySettings.hideKey = VK_ESCAPE;
}

CSettingsDialog::~CSettingsDialog()
{
}

BOOL CSettingsDialog::OnInitDialog()
{
    // Attach controls
    m_editSourceLang.AttachDlgItem(IDC_SOURCE_LANG, *this);
    m_editTargetLang.AttachDlgItem(IDC_TARGET_LANG, *this);
    m_editHotkeyTranslate.AttachDlgItem(IDC_HOTKEY_TRANSLATE, *this);
    m_editHotkeyHide.AttachDlgItem(IDC_HOTKEY_HIDE, *this);
    
    // Load current values into controls
    SaveToControls();
    
    return TRUE;
}

BOOL CSettingsDialog::OnCommand(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    
    UINT nID = LOWORD(wParam);
    
    switch (nID)
    {
        case IDC_OK:
            OnOK();
            return TRUE;
            
        case IDC_CANCEL:
            OnCancel();
            return TRUE;
            
        case IDC_APPLY:
            LoadFromControls();
            return TRUE;
            
        case IDC_RESET:
            ResetToDefaults();
            SaveToControls();
            return TRUE;
            
        case IDC_HOTKEY_TRANSLATE:
            if (HIWORD(wParam) == EN_SETFOCUS)
            {
                OnHotkeyChanged(IDC_HOTKEY_TRANSLATE);
                return TRUE;
            }
            break;
            
        case IDC_HOTKEY_HIDE:
            if (HIWORD(wParam) == EN_SETFOCUS)
            {
                OnHotkeyChanged(IDC_HOTKEY_HIDE);
                return TRUE;
            }
            break;
    }
    
    return FALSE;
}

void CSettingsDialog::OnOK()
{
    LoadFromControls();
    CDialog::OnOK();
}

void CSettingsDialog::OnCancel()
{
    CDialog::OnCancel();
}

void CSettingsDialog::LoadFromControls()
{
    // Get language settings
    m_sourceLanguage = m_editSourceLang.GetWindowText();
    m_targetLanguage = m_editTargetLang.GetWindowText();
}

void CSettingsDialog::SaveToControls()
{
    // Set language values
    m_editSourceLang.SetWindowText(m_sourceLanguage.c_str());
    m_editTargetLang.SetWindowText(m_targetLanguage.c_str());
    
    // Set hotkey values
    m_editHotkeyTranslate.SetWindowText(FormatHotkey(m_hotkeySettings.translateModifiers, m_hotkeySettings.translateKey).c_str());
    m_editHotkeyHide.SetWindowText(FormatHotkey(m_hotkeySettings.hideModifiers, m_hotkeySettings.hideKey).c_str());
}

void CSettingsDialog::ResetToDefaults()
{
    m_sourceLanguage = L"English";
    m_targetLanguage = L"Turkish";
    m_hotkeySettings.translateModifiers = MOD_CONTROL | MOD_SHIFT;
    m_hotkeySettings.translateKey = '2';
    m_hotkeySettings.hideModifiers = 0;
    m_hotkeySettings.hideKey = VK_ESCAPE;
}

std::wstring CSettingsDialog::FormatHotkey(UINT modifiers, UINT key)
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
            case VK_BACK: result += L"BACKSPACE"; break;
            case VK_DELETE: result += L"DELETE"; break;
            case VK_INSERT: result += L"INSERT"; break;
            case VK_HOME: result += L"HOME"; break;
            case VK_END: result += L"END"; break;
            case VK_PRIOR: result += L"PAGE UP"; break;
            case VK_NEXT: result += L"PAGE DOWN"; break;
            case VK_UP: result += L"UP"; break;
            case VK_DOWN: result += L"DOWN"; break;
            case VK_LEFT: result += L"LEFT"; break;
            case VK_RIGHT: result += L"RIGHT"; break;
            default:
                {
                    std::wstringstream ss;
                    ss << L"0x" << std::hex << key;
                    result += ss.str();
                }
                break;
        }
    }
    
    return result;
}

void CSettingsDialog::OnHotkeyChanged(int controlId)
{
    // Simple hotkey capture - for now just show a message
    // In a full implementation, you'd capture key combinations here
    std::wstring message;
    
    if (controlId == IDC_HOTKEY_TRANSLATE)
    {
        message = L"Click OK to set new translate hotkey.\\nCurrently: ";
        message += FormatHotkey(m_hotkeySettings.translateModifiers, m_hotkeySettings.translateKey);
        message += L"\\n\\nNote: Hotkey changes require application restart.";
    }
    else if (controlId == IDC_HOTKEY_HIDE)
    {
        message = L"Click OK to set new hide hotkey.\\nCurrently: ";
        message += FormatHotkey(m_hotkeySettings.hideModifiers, m_hotkeySettings.hideKey);
        message += L"\\n\\nNote: Hotkey changes require application restart.";
    }
    
    MessageBox(message.c_str(), L"Hotkey Settings", MB_OK | MB_ICONINFORMATION);
}
