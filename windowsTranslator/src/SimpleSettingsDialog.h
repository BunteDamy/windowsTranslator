#pragma once

#include <windows.h>
#include <string>

class CSimpleSettingsDialog
{
public:
    CSimpleSettingsDialog();
    ~CSimpleSettingsDialog();
    
    INT_PTR ShowModal(HWND hwndParent);
    
    // Data access
    std::wstring GetSourceLanguage() const { return m_sourceLanguage; }
    std::wstring GetTargetLanguage() const { return m_targetLanguage; }
    
    void SetSourceLanguage(const std::wstring& lang) { m_sourceLanguage = lang; }
    void SetTargetLanguage(const std::wstring& lang) { m_targetLanguage = lang; }
    
    // Hotkey access
    UINT GetTranslateModifiers() const { return m_translateModifiers; }
    UINT GetTranslateKey() const { return m_translateKey; }
    UINT GetHideModifiers() const { return m_hideModifiers; }
    UINT GetHideKey() const { return m_hideKey; }
    
    void SetHotkeys(UINT translateMod, UINT translateKey, UINT hideMod, UINT hideKey) {
        m_translateModifiers = translateMod;
        m_translateKey = translateKey;
        m_hideModifiers = hideMod;
        m_hideKey = hideKey;
    }

private:
    static INT_PTR CALLBACK DialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    BOOL OnInitDialog(HWND hwndDlg);
    BOOL OnCommand(HWND hwndDlg, WPARAM wParam, LPARAM lParam);
    void OnOK(HWND hwndDlg);
    void OnCancel(HWND hwndDlg);
    std::wstring FormatHotkey(UINT modifiers, UINT key);
    
    std::wstring m_sourceLanguage;
    std::wstring m_targetLanguage;
    HWND m_hwndDlg;
    
    // Hotkey settings
    UINT m_translateModifiers;
    UINT m_translateKey;
    UINT m_hideModifiers;
    UINT m_hideKey;
};
