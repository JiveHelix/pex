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


struct Signal;

template<typename T, typename = void>
struct ModelSelector_
{
    using Type = pex::model::Value<T>;
};

template<typename T>
struct ModelSelector_<T, std::enable_if_t<std::is_same_v<T, Signal>>>
{
    using Type = pex::model::Signal;
};

template<typename T>
using ModelSelector = typename ModelSelector_<T>::Type;


template<template<typename> typename T>
struct ApplicationTemplate
{
    T<Signal> sayWhatsUp;
    T<Signal> sayHello;
    T<Signal> sayFortyTwo;
    T<Signal> frobnicate;
    T<std::string> message;
};

struct ApplicationModel: ApplicationTemplate<ModelSelector>
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


struct ApplicationControl
{
    pex::control::Signal<void> sayWhatsUp;
    pex::control::Signal<void> sayHello;
    pex::control::Signal<void> sayFortyTwo;
    pex::control::Signal<void> frobnicate;
    pex::control::Value<void, pex::model::Value<std::string>> message;

    ApplicationControl() = default;

    ApplicationControl(ApplicationModel &model)
    {
        fields::AssignConvert<ApplicationFields>(*this, model);
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
    AnotherFrame(const pex::wx::ShortcutsByMenu &shortcuts)
        :
        wxFrame(nullptr, wxID_ANY, "A frame with no menus"),
        acceleratorShortcuts_(this, shortcuts)
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
        pex::wx::ShortcutsByMenu &shortcuts)
        :
        wxFrame(nullptr, wxID_ANY, "pex::wx::Shortcut Demo"),
        menuShortcuts_(this, shortcuts)
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

    auto shortcutsByMenu = pex::wx::ShortcutsByMenu(
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
