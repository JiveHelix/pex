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
#include "pex/interface.h"
#include "pex/group.h"
#include "pex/wx/wxshim.h"
#include "pex/wx/window.h"
#include "pex/wx/shortcut.h"
#include "pex/wx/view.h"
#include "fields/fields.h"
#include "fields/assign.h"


template<typename T>
struct ApplicationFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::sayWhatsUp, "sayWhatsUp"),
        fields::Field(&T::sayHello, "sayHello"),
        fields::Field(&T::sayFortyTwo, "sayFortyTwo"),
        fields::Field(&T::frobnicate, "frobnicate"),
        fields::Field(&T::message, "message"));
};

template<template<typename> typename T>
struct ApplicationTemplate
{
    T<pex::MakeSignal> sayWhatsUp;
    T<pex::MakeSignal> sayHello;
    T<pex::MakeSignal> sayFortyTwo;
    T<pex::MakeSignal> frobnicate;
    T<std::string> message;
};


using ApplicationGroup = pex::Group<ApplicationFields, ApplicationTemplate>;


struct ApplicationModel: public ApplicationGroup::Model
{
    ApplicationModel()
    {
        PEX_LOG("Connect");
        this->sayHello.Connect(
            this,
            &ApplicationModel::OnSayHello_);

        PEX_LOG("Connect");
        this->sayWhatsUp.Connect(
            this,
            &ApplicationModel::OnSayWhatsUp_);

        PEX_LOG("Connect");
        this->sayFortyTwo.Connect(
            this,
            &ApplicationModel::OnSayFortyTwo_);

        PEX_LOG("Connect");
        this->frobnicate.Connect(
            this,
            &ApplicationModel::OnFrobnicate_);
    }

    ~ApplicationModel()
    {
        PEX_LOG("Disconnect");
        this->sayHello.Disconnect(this);

        PEX_LOG("Disconnect");
        this->sayWhatsUp.Disconnect(this);

        PEX_LOG("Disconnect");
        this->sayFortyTwo.Disconnect(this);

        PEX_LOG("Disconnect");
        this->frobnicate.Disconnect(this);
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


using ApplicationControl = typename ApplicationGroup::Control<void>;


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
    AnotherFrame(const pex::wx::ShortcutGroups & shortcuts)
        :
        wxFrame(nullptr, wxID_ANY, "A frame with no menus"),
        acceleratorShortcuts_(pex::wx::Window(this), shortcuts)
    {
        this->SetAcceleratorTable(
            this->acceleratorShortcuts_.GetAcceleratorTable());

        new wxTextCtrl(
            this,
            wxID_ANY,
            "This frame has no menus, but shortcuts should still work when "
            "it has focus.",
            wxDefaultPosition,
            wxDefaultSize,
            wxTE_MULTILINE | wxTE_READONLY);
    }

private:
    pex::wx::AcceleratorShortcuts acceleratorShortcuts_;
};


class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(
        ApplicationControl applicationControl,
        pex::wx::ShortcutGroups & shortcuts
        )
        :
        wxFrame(nullptr, wxID_ANY, "pex::wx::Shortcut Demo"),
        menuShortcuts_(pex::wx::Window(this), shortcuts)
    {
        this->SetMenuBar(this->menuShortcuts_.GetMenuBar());

        auto message = new wxStaticText(
            this,
            wxID_ANY,
            "Use the shortcut keys and the menu items.");

        auto view =
            new pex::wx::View(this, applicationControl.message);

        auto topSizer = std::make_unique<wxBoxSizer>(wxVERTICAL);
        topSizer->Add(message, 0, wxALL, 10);
        topSizer->Add(view, 0, wxALL, 10);
        this->SetSizerAndFit(topSizer.release());
    }

private:
    pex::wx::MenuShortcuts menuShortcuts_;
};


wxshimIMPLEMENT_APP_CONSOLE(ExampleApp)


bool ExampleApp::OnInit()
{
    auto applicationControl = ApplicationControl(this->applicationModel_);

    using namespace std::literals;

    auto shortcutsByMenu = pex::wx::ShortcutGroups(
        {{
            "File", pex::wx::Shortcuts(
                {
                    pex::wx::Shortcut(
                        applicationControl.sayFortyTwo,
                        wxACCEL_CMD,
                        'Z',
                        "42",
                        "Say '42'"),

                    pex::wx::Shortcut(
                        applicationControl.frobnicate,
                        wxACCEL_CMD,
                        'F',
                        "Frobnicate",
                        "Do some frobnicating")})},
        {
            "Other", pex::wx::Shortcuts(       
                {
                    pex::wx::Shortcut(
                        applicationControl.sayWhatsUp,
                        wxACCEL_CMD,
                        'W',
                        "What's up?",
                        "Say 'What's up?'"),

                    pex::wx::Shortcut(
                        applicationControl.sayHello,
                        wxACCEL_ALT | wxACCEL_SHIFT,
                        'H',
                        "Hello",
                        "Say 'Hello'")})}});
    
    auto exampleFrame = new ExampleFrame(
        applicationControl,
        shortcutsByMenu);

    auto anotherFrame = new AnotherFrame(shortcutsByMenu);

    exampleFrame->Show();
    anotherFrame->Show();

    anotherFrame->SetPosition(exampleFrame->GetRect().GetTopRight());

    exampleFrame->Raise();

    return true;
}
