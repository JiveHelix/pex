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
    Window &&window,
    const ShortcutsByMenu &shortcutsByMenu)
    :
    window_(std::move(window)),
    hasBindings_(false),
    shortcutsByMenu_(shortcutsByMenu)
{
    this->BindAll_();
}


ShortcutsBase::~ShortcutsBase()
{
    this->UnbindAll_();
}


void ShortcutsBase::BindAll_()
{
    auto window = this->window_.Get();

    if (!window)
    {
        std::cout << "Warning: ShortcutBase::BindAll_() without wxWindow"
            << std::endl;

        return;
    }

    window->Bind(
        wxEVT_CLOSE_WINDOW,
        &ShortcutsBase::OnWindowClose_,
        this);

    for (auto & [menuName, shortcuts]: this->shortcutsByMenu_)
    {
        this->BindShortcuts_(window, shortcuts);
    }

    this->hasBindings_ = true;
}


void ShortcutsBase::UnbindAll_()
{
    auto window = this->window_.Get();

    if (!window || !this->hasBindings_)
    {
        return;
    }

    window->Unbind(
        wxEVT_CLOSE_WINDOW,
        &ShortcutsBase::OnWindowClose_,
        this);

    PEX_LOG("Unbind shortcuts");

    for (auto & [menuName, shortcuts]: this->shortcutsByMenu_)
    {
        this->UnbindShortcuts_(window, shortcuts);
    }

    this->hasBindings_ = false;
}


ShortcutsBase::ShortcutsBase(ShortcutsBase &&other)
    :
    window_(std::move(other.window_)),
    hasBindings_(other.hasBindings_),
    shortcutsByMenu_(other.shortcutsByMenu_)
{
    this->UnbindAll_();
    this->BindAll_();
}


ShortcutsBase & ShortcutsBase::operator=(ShortcutsBase &&other)
{
    other.UnbindAll_();
    this->UnbindAll_();
    this->window_ = std::move(other.window_);
    this->shortcutsByMenu_ = std::move(other.shortcutsByMenu_);
    this->BindAll_();
    
    return *this;
}


void ShortcutsBase::BindShortcuts_(wxWindow *window, Shortcuts &shortcuts)
{
    assert(window != nullptr);

    for (auto &shortcut: shortcuts)
    {
        window->Bind(
            wxEVT_MENU,
            ShortcutFunctor(shortcut),
            shortcut.GetId());
    }
}

void ShortcutsBase::UnbindShortcuts_(wxWindow *window, Shortcuts &shortcuts)
{
    assert(window != nullptr);

    for (auto &shortcut: shortcuts)
    {
        window->Unbind(
            wxEVT_MENU,
            ShortcutFunctor(shortcut),
            shortcut.GetId());
    }
}


void ShortcutsBase::OnWindowClose_(wxCloseEvent &event)
{
    this->UnbindAll_();
    this->window_.Clear();
    event.Skip();
}


MenuShortcuts::MenuShortcuts(
    Window &&window,
    const ShortcutsByMenu &shortcutsByMenu)
    :
    ShortcutsBase(std::move(window), shortcutsByMenu),
    menuBar_(std::make_unique<wxMenuBar>())
{
    for (auto & [menuName, shortcuts]: this->shortcutsByMenu_)
    {
        auto menu = std::make_unique<wxMenu>();
        this->AddToMenu(menu.get(), shortcuts);

        this->menuBar_->Append(
            menu.release(),
            wxString(std::forward<decltype(menuName)>(menuName)));
    }
}

wxMenuBar * MenuShortcuts::GetMenuBar()
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
    Window &&window,
    const ShortcutsByMenu &shortcutsByMenu)
    :
    ShortcutsBase(std::move(window), shortcutsByMenu),
    acceleratorTable_()
{
    std::vector<std::vector<wxAcceleratorEntry>> entries;
    size_t entryCount = 0;

    for (auto & [menuName, shortcuts]: this->shortcutsByMenu_)
    {
        auto shortcutEntries = CreateAcceleratorEntries(shortcuts); 
        entryCount += shortcutEntries.size();
        entries.push_back(shortcutEntries);
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


ShortcutWindow::ShortcutWindow(
    wxWindow *window,
    const pex::wx::ShortcutsByMenu &shortcutsByMenu)
    :
    Window(window),
    acceleratorShortcuts_(
        std::make_unique<AcceleratorShortcuts>(
            pex::wx::Window(window),
            shortcutsByMenu))
{
    window->SetAcceleratorTable(
        this->acceleratorShortcuts_->GetAcceleratorTable());
}


} // end namespace wx


} // end namespace pex
