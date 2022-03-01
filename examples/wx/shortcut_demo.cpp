/**
  * @file shortcut_demo.cpp
  *
  * @brief Demonstrates the usage of pex::Shortcut and wx menus.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 23 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#include <iostream>
#include <string>
#include <string_view>
#include "pex/wx/wxshim.h"
#include "pex/signal.h"
#include "pex/value.h"
#include "pex/initialize.h"
#include "pex/wx/shortcut.h"
#include "pex/wx/view.h"
#include "fields/fields.h"


template<typename T>
struct ApplicationFields
{
    static constexpr auto value = std::make_tuple(
        fields::Field(&T::sayWhatsUp, "sayWhatsUp"),
        fields::Field(&T::sayHello, "sayHello"),
        fields::Field(&T::sayFortyTwo, "sayFortyTwo"),
        fields::Field(&T::frobnicate, "frobnicate"),
        fields::Field(&T::message, "message"));
};


struct ApplicationModel
{
    pex::model::Signal sayWhatsUp;
    pex::model::Signal sayHello;
    pex::model::Signal sayFortyTwo;
    pex::model::Signal frobnicate;
    pex::model::Value<std::string> message;

    ApplicationModel()
    {
        this->sayHello.Connect(
            this,
            &ApplicationModel::OnSayHello_);

        this->sayWhatsUp.Connect(
            this,
            &ApplicationModel::OnSayWhatsUp_);

        this->sayFortyTwo.Connect(
            this,
            &ApplicationModel::OnSayFortyTwo_);

        this->frobnicate.Connect(
            this,
            &ApplicationModel::OnFrobnicate_);
    }

    static void OnSayHello_(void * context)
    {
        auto self = static_cast<ApplicationModel *>(context);
        self->message.Set("Hello");
    }

    static void OnSayWhatsUp_(void * context)
    {
        auto self = static_cast<ApplicationModel *>(context);
        self->message.Set("What's up?");
    }

    static void OnSayFortyTwo_(void * context)
    {
        auto self = static_cast<ApplicationModel *>(context);
        self->message.Set("forty-two");
    }

    static void OnFrobnicate_(void * context)
    {
        auto self = static_cast<ApplicationModel *>(context);
        self->message.Set("Frobnicating...");
    }
};


struct ApplicationInterface
{
    pex::interface::Signal<void> sayWhatsUp;
    pex::interface::Signal<void> sayHello;
    pex::interface::Signal<void> sayFortyTwo;
    pex::interface::Signal<void> frobnicate;
    pex::interface::Value<void, pex::model::Value<std::string>> message;

    ApplicationInterface() = default;

    ApplicationInterface(ApplicationModel &model)
    {
        pex::Initialize<ApplicationFields>(model, *this);
    }
};


class ExampleApp: public wxApp
{
public:
    ExampleApp()
        :
        applicationModel_{}
    {
    }

    bool OnInit() override;

private:

    ApplicationModel applicationModel_;
};


class AnotherFrame: public wxFrame
{
public:
    AnotherFrame()
        :
        wxFrame(nullptr, wxID_ANY, "A frame with no menus")
    {
        new wxTextCtrl(
            this,
            wxID_ANY,
            "This frame has no menus, but shortcuts should still work when "
            "it has focus.",
            wxDefaultPosition,
            wxDefaultSize,
            wxTE_MULTILINE | wxTE_READONLY);
    }
};


template<typename ShortcutsByMenu>
class ExampleFrame: public wxFrame
{
    using This = ExampleFrame<ShortcutsByMenu>;

public:
    ExampleFrame(
        ApplicationInterface applicationInterface,
        const ShortcutsByMenu &shortcuts)
        :
        wxFrame(nullptr, wxID_ANY, "pex::wx::Shortcut Demo"),
        shortcuts_(shortcuts)
    {
        auto menuBar = pex::wx::InitializeMenus(this, this->shortcuts_);
        this->SetMenuBar(menuBar.release());

        auto message = new wxStaticText(
            this,
            wxID_ANY,
            "Use the shortcut keys and the menu items.");

        auto view =
            pex::wx::MakeView(this, applicationInterface.message);

        auto topSizer = std::make_unique<wxBoxSizer>(wxVERTICAL);
        topSizer->Add(message, 0, wxALL, 10);
        topSizer->Add(view, 0, wxALL, 10);
        this->SetSizerAndFit(topSizer.release());
    }

private:
    ShortcutsByMenu shortcuts_;
};


wxshimIMPLEMENT_APP(ExampleApp)


bool ExampleApp::OnInit()
{
    auto applicationInterface = ApplicationInterface(this->applicationModel_);

    using namespace std::literals;

    auto shortcutsByMenu = std::make_tuple(
        std::make_pair(
            "File",        
            std::make_tuple(
                pex::wx::Shortcut(
                    applicationInterface.sayFortyTwo,
                    wxACCEL_CMD,
                    'Z',
                    "42",
                    "Say '42'"),

                pex::wx::Shortcut(
                    applicationInterface.frobnicate,
                    wxACCEL_CMD,
                    'F',
                    "Frobnicate",
                    "Do some frobnicating"))),

        std::make_pair(
            "Other",        
            std::make_tuple(
                pex::wx::Shortcut(
                    applicationInterface.sayWhatsUp,
                    wxACCEL_CMD,
                    'W',
                    "What's up?",
                    "Say 'What's up?'"),

                pex::wx::Shortcut(
                    applicationInterface.sayHello,
                    wxACCEL_ALT | wxACCEL_SHIFT,
                    'H',
                    "Hello",
                    "Say 'Hello'"))));
    
    auto exampleFrame = new ExampleFrame(
        applicationInterface,
        shortcutsByMenu);

    auto anotherFrame = new AnotherFrame();

    exampleFrame->Show();
    anotherFrame->Show();

    anotherFrame->SetPosition(exampleFrame->GetRect().GetTopRight());

    exampleFrame->Raise();

    return true;
}
