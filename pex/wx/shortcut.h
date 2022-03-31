#pragma once

#include <map>
#include <functional>
#include "pex/signal.h"
#include "jive/strings.h"
#include "jive/formatter.h"
#include "jive/for_each.h"
#include "pex/wx/wxshim.h"


namespace pex
{

namespace wx
{


class Key
{
public:
    template<typename T>
    Key(T keyCode)
        :
        code_(static_cast<int>(keyCode))
    {
        static_assert(
            std::is_same_v<T, char>
            || std::is_same_v<T, wxKeyCode>,
            "A key code must be a char or a wxKeyCode enum value.");
    }

    std::string GetString() const
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

    int GetInt() const { return this->code_; }

private:
    int code_;

    static inline const std::map<int, std::string>
    keyStringByKeyCode_
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
};


/**
Returns a string describing all modifiers in the bitfield.

modifierBitfield may be bitwise combination of modifiers.

For Mac builds, wxACCEL_CTRL and wxACCEL_CMD map to the command key,
wxACCEL_RAW_CTRL is the modifier for the 'control' key.

For other builds, wxACCEL_RAW_CTRL maps to the 'ctrl' key.
**/
inline std::string GetModifierString(int modifierBitfield)
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
    modifierOrder{wxACCEL_CTRL, wxACCEL_SHIFT, wxACCEL_ALT, wxACCEL_RAW_CTRL};

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


class Shortcut
{
public:
    // We are not observing this signal, so the Observer can be void.
    using SignalType = pex::control::Signal<void>;

    template<typename KeyCode>
    Shortcut(
        SignalType signal,
        int modifier,
        KeyCode keyCode,
        std::string_view description,
        std::string_view longDescription)
        :
        signal_(signal),
        id_(wxWindow::NewControlId()),
        modifier_(modifier),
        key_(keyCode),
        description_(description),
        longDescription_(longDescription)
    {

    }

    void AddToMenu(wxMenu *menu) const
    {
        menu->Append(
            new wxMenuItem(
                menu,
                this->id_,
                this->GetMenuItemLabel_(),
                this->longDescription_));
    }

    int GetModifier() const { return this->modifier_; }

    int GetKeyAsInt() const { return this->key_.GetInt(); }
    
    int GetId() const { return this->id_; }

    void OnEventMenu()
    {
        this->signal_.Trigger();
    }

private: 
    wxString GetMenuItemLabel_() const
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

    SignalType signal_;
    int id_;
    int modifier_;
    Key key_;
    const std::string description_;
    const std::string longDescription_;
};


class ShortcutFunctor
{
public:
    ShortcutFunctor(const Shortcut &shortcut): shortcut_(shortcut) {}

    void operator()(wxCommandEvent &)
    {
        this->shortcut_.OnEventMenu();
    }

private:
    Shortcut shortcut_;
};


template
<
    typename Window,
    typename ShortcutsTuple
>
void BindShortcuts(Window *window, ShortcutsTuple &shortcutsTuple)
{
    jive::ForEach(
        shortcutsTuple,
        [window](auto &shortcut) -> void
        {
            window->Bind(
                wxEVT_MENU,
                ShortcutFunctor(shortcut),
                shortcut.GetId());
        });
}


template<typename ShortcutsTuple>
void AddToMenu(wxMenu *menu, const ShortcutsTuple &shortcutsTuple)
{
    jive::ForEach(
        shortcutsTuple,
        [menu](const auto &shortcut) -> void
        {
            shortcut.AddToMenu(menu);
        });
}


template<typename Window, typename ShortcutsByMenu>
std::unique_ptr<wxMenuBar> InitializeMenus(
    Window *window,
    ShortcutsByMenu &shortcutsByMenu)
{
    auto menuBar = std::make_unique<wxMenuBar>();

    jive::ForEach(
        shortcutsByMenu,
        [window, &menuBar](auto &nameShortcutsPair)
        {
            auto & [menuName, shortcuts] = nameShortcutsPair;
            auto menu = std::make_unique<wxMenu>();
            AddToMenu(menu.get(), shortcuts);

            menuBar->Append(
                menu.release(),
                wxString(std::forward<decltype(menuName)>(menuName)));

            BindShortcuts(window, shortcuts);
        });

    return menuBar;
}
    


} // namespace wx

} // namespace pex
