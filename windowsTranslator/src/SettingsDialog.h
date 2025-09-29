#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include "wxx_wincore.h"
#include "wxx_dialog.h"
#include "wxx_controls.h"
#include "../resources/resource.h"
#include <string>

using namespace Win32xx;

struct HotkeySettings {
    UINT translateModifiers = MOD_CONTROL | MOD_SHIFT;
    UINT translateKey = '2';
    UINT hideModifiers = 0;
    UINT hideKey = VK_ESCAPE;
};

class CSettingsDialog : public CDialog
{
public:
    CSettingsDialog(UINT nResID);
    virtual ~CSettingsDialog();

    // Data access
    std::wstring GetSourceLanguage() const { return m_sourceLanguage; }
    std::wstring GetTargetLanguage() const { return m_targetLanguage; }
    HotkeySettings GetHotkeySettings() const { return m_hotkeySettings; }
    
    void SetSourceLanguage(const std::wstring& lang) { m_sourceLanguage = lang; }
    void SetTargetLanguage(const std::wstring& lang) { m_targetLanguage = lang; }
    void SetHotkeySettings(const HotkeySettings& settings) { m_hotkeySettings = settings; }

protected:
    // Message handlers
    virtual BOOL OnInitDialog() override;
    virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam) override;
    virtual void OnOK() override;
    virtual void OnCancel() override;

private:
    // Control references
    CEdit m_editSourceLang;
    CEdit m_editTargetLang;
    CEdit m_editHotkeyTranslate;
    CEdit m_editHotkeyHide;
    
    // Data members
    std::wstring m_sourceLanguage;
    std::wstring m_targetLanguage;
    HotkeySettings m_hotkeySettings;
    
    // Helper methods
    void LoadFromControls();
    void SaveToControls();
    void ResetToDefaults();
    std::wstring FormatHotkey(UINT modifiers, UINT key);
    void OnHotkeyChanged(int controlId);
};
