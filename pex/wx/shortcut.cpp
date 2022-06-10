#include "pex/wx/shortcut.h"


namespace pex
{


namespace wx
{


std::string Key::GetString() const
{
    if (this->keyStringByKeyCode_.count(this->code_))
    {
        return this->keyStringByKeyCode_.at(this->code_);
    }
    
    if (this->code_ > 127)
    {
        throw std::runtime_error("Unsupported key code.");
    }

    return std::string(1, static_cast<char>(this->code_));
}

int Key::GetInt() const { return this->code_; }

const std::map<int, std::string> Key::keyStringByKeyCode_
{
    {WXK_DELETE, "DELETE"},
    {WXK_BACK, "BACK"},
    {WXK_INSERT, "INSERT"},
    {WXK_RETURN, "RETURN"},
    {WXK_PAGEUP, "PGUP"},
    {WXK_PAGEDOWN, "PGDN"},
    {WXK_LEFT, "LEFT"},
    {WXK_RIGHT, "RIGHT"},
    {WXK_UP, "UP"},
    {WXK_DOWN, "DOWN"},
    {WXK_HOME, "HOME"},
    {WXK_END, "END"},
    {WXK_SPACE, "SPACE"},
    {WXK_TAB, "TAB"},
    {WXK_ESCAPE, "ESCAPE"}
};


std::string GetModifierString(int modifierBitfield)
{
    static const std::map<wxAcceleratorEntryFlags, std::string>
    modifierStringByWxAccel
    {
        {wxACCEL_NORMAL, ""},
        {wxACCEL_SHIFT, "SHIFT"},
        {wxACCEL_CTRL, "CTRL"},
        {wxACCEL_ALT, "ALT"},
        {wxACCEL_RAW_CTRL, "RAWCTRL"}
    };

    static const std::vector<wxAcceleratorEntryFlags>

    modifierOrder
    {
        wxACCEL_CTRL,
        wxACCEL_SHIFT,
        wxACCEL_ALT
#ifdef __APPLE__
        , wxACCEL_RAW_CTRL
#endif 
    };

    std::vector<std::string> modifiers;

    for (auto modifier: modifierOrder)
    {
        if (modifierBitfield & modifier)
        {
            modifiers.push_back(modifierStringByWxAccel.at(modifier));
        }
    }

    return jive::strings::Join(modifiers.begin(), modifiers.end(), '+');
}


void Shortcut::AddToMenu(wxMenu *menu)
{
    menu->Append(
        this->menuItem_ = new wxMenuItem(
            menu,
            this->id_,
            this->GetMenuItemLabel_(),
            this->longDescription_));
}

wxAcceleratorEntry Shortcut::GetAcceleratorEntry() const
{
    return wxAcceleratorEntry(
        this->modifier_,
        this->key_.GetInt(),
        this->id_,
        this->menuItem_);
}

int Shortcut::GetModifier() const { return this->modifier_; }

int Shortcut::GetKeyAsInt() const { return this->key_.GetInt(); }

int Shortcut::GetId() const { return this->id_; }

void Shortcut::OnEventMenu()
{
    this->signal_.Trigger();
}

wxString Shortcut::GetMenuItemLabel_() const
{
    auto modifier = GetModifierString(this->modifier_);
    
    std::string result(this->description_);

    if (!modifier.empty())
    {
        jive::strings::Concatenate(
            &result,
            '\t',
            modifier,
            '+',
            this->key_.GetString());
    }
    else
    {
        jive::strings::Concatenate(
            &result,
            '\t',
            this->key_.GetString());
    }
    
    return wxString(result);
}


ShortcutsBase::ShortcutsBase(
    wxWindow *window,
    const ShortcutsByMenu &shortcutsByMenu)
    :
    window_(window),
    shortcutsByMenu_(shortcutsByMenu)
{

}

ShortcutsBase::~ShortcutsBase()
{
    PEX_LOG("Unbind shortcuts");

    for (auto & [menuName, shortcuts]: this->shortcutsByMenu_)
    {
        this->UnbindShortcuts(shortcuts);
    }
}

void ShortcutsBase::BindShortcuts(Shortcuts &shortcuts)
{
    for (auto &shortcut: shortcuts)
    {
        this->window_->Bind(
            wxEVT_MENU,
            ShortcutFunctor(shortcut),
            shortcut.GetId());
    }
}

void ShortcutsBase::UnbindShortcuts(Shortcuts &shortcuts)
{
    for (auto &shortcut: shortcuts)
    {
        this->window_->Unbind(
            wxEVT_MENU,
            ShortcutFunctor(shortcut),
            shortcut.GetId());
    }
}


MenuShortcuts::MenuShortcuts(
    wxWindow *window,
    ShortcutsByMenu &shortcutsByMenu)
    :
    ShortcutsBase(window, shortcutsByMenu),
    menuBar_(std::make_unique<wxMenuBar>())
{
    for (auto & [menuName, shortcuts]: this->shortcutsByMenu_)
    {
        auto menu = std::make_unique<wxMenu>();
        this->AddToMenu(menu.get(), shortcuts);

        this->menuBar_->Append(
            menu.release(),
            wxString(std::forward<decltype(menuName)>(menuName)));

        this->BindShortcuts(shortcuts);
    }
}

wxMenuBar *MenuShortcuts::GetMenuBar()
{
    return this->menuBar_.release();
}

void MenuShortcuts::AddToMenu(
    wxMenu *menu,
    Shortcuts &shortcuts)
{
    for (auto &shortcut: shortcuts)
    {
        shortcut.AddToMenu(menu);
    }
}


AcceleratorShortcuts::AcceleratorShortcuts(
    wxWindow *window,
    const ShortcutsByMenu &shortcutsByMenu)
    :
    ShortcutsBase(window, shortcutsByMenu),
    acceleratorTable_()
{
    std::vector<std::vector<wxAcceleratorEntry>> entries;
    size_t entryCount = 0;

    for (auto & [menuName, shortcuts]: this->shortcutsByMenu_)
    {
        auto shortcutEntries = CreateAcceleratorEntries(shortcuts); 
        entryCount += shortcutEntries.size();
        entries.push_back(shortcutEntries);
        this->BindShortcuts(shortcuts);
    }

    std::vector<wxAcceleratorEntry> allEntries;
    allEntries.reserve(entryCount);

    for (auto &it: entries)
    {
        allEntries.insert(allEntries.end(), it.begin(), it.end());
    }

    assert(allEntries.size() == entryCount);

    this->acceleratorTable_ = wxAcceleratorTable(
        static_cast<int>(entryCount),
        allEntries.data());
}

const wxAcceleratorTable & AcceleratorShortcuts::GetAcceleratorTable() const
{
    return this->acceleratorTable_;
}

std::vector<wxAcceleratorEntry>
AcceleratorShortcuts::CreateAcceleratorEntries(const Shortcuts &shortcuts)
{
    std::vector<wxAcceleratorEntry> entries;
    entries.reserve(shortcuts.size());

    for (auto &shortcut: shortcuts)
    {
        entries.push_back(shortcut.GetAcceleratorEntry());
    }

    return entries;
}


} // end namespace wx


} // end namespace pex
