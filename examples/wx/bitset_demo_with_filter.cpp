/**
  * @file bitset_demo_with_filter.cpp
  * 
  * @brief A simpler demonstration of a bitset Field, using FlagFilter to
  * convert between bitset and boolean flags.
  * 
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 17 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#include <iostream>
#include <string>
#include <bitset>
#include <array>
#include "wxshim.h"
#include "pex/wx/view.h"
#include "pex/wx/field.h"

#include "pex/wx/labeled_widget.h"
#include "pex/wx/check_box.h"

#include "pex/detail/filters.h"

constexpr auto bitCount = 8;
using Bitset = std::bitset<bitCount>;

using BitsetModel = pex::model::Value<Bitset>;
using BitsetInterface = pex::interface::Value<void, BitsetModel>;

struct FlagFilter
{
public:
    FlagFilter() = default;

    FlagFilter(BitsetModel *model, size_t index)
        :
        model_(model),
        index_(index)
    {

    }

    bool Get(const Bitset &bitset)
    {
        return bitset[this->index_];
    }

    Bitset Set(bool value)
    {
        Bitset result = this->model_->Get();
        result[this->index_] = value;
        return result;
    }

private:
    BitsetModel *model_;
    size_t index_;
};


using FlagInterface =
    pex::interface::FilteredValue<void, BitsetModel, FlagFilter>;


struct Model
{
    BitsetModel bitset;

    Model()
        :
        bitset{}
    {

    }
};


struct Interface
{
    BitsetInterface bitset;
    std::array<FlagInterface, bitCount> flags;

    Interface(Model &model)
        :
        bitset(&model.bitset),
        flags{},
        filters{}
    {
        for (size_t i = 0; i < bitCount; ++i)
        {
            filters[i] = FlagFilter(&model.bitset, i);
            flags[i] = FlagInterface(&model.bitset, &filters[i]);
        }
    }

private:
    std::array<FlagFilter, bitCount> filters;
};


class ExampleApp: public wxApp
{
public:
    ExampleApp()
        :
        model_{},
        interface_(this->model_)
    {

    }

    bool OnInit() override;

private:
    Model model_;
    Interface interface_;
};


class ExampleFrame: public wxFrame
{
public:
    ExampleFrame(Interface interface);
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"

// Creates the main function for us, and initializes the app's run loop.
wxIMPLEMENT_APP(ExampleApp);

#pragma GCC diagnostic pop


bool ExampleApp::OnInit()
{
    ExampleFrame *exampleFrame = new ExampleFrame(this->interface_);

    exampleFrame->Show();
    return true;
}

using CheckBox = pex::wx::CheckBox<FlagInterface>;

ExampleFrame::ExampleFrame(Interface interface)
    :
    wxFrame(nullptr, wxID_ANY, "Bitset Demo")
{
    auto bitsetView =
        new pex::wx::LabeledWidget<pex::wx::View<BitsetInterface>>(
            this,
            interface.bitset,
            "Bitset (view):");

    auto bitsetField =
        new pex::wx::LabeledWidget<pex::wx::Field<BitsetInterface>>(
            this,
            interface.bitset,
            "Bitset (field):");

    auto flagsSizer = std::make_unique<wxBoxSizer>(wxHORIZONTAL);

    for (size_t i = 0; i < bitCount; ++i)
    {
        flagsSizer->Add(
            new CheckBox(
                this,
                jive::FastFormatter<16>("bit %zu", i),
                interface.flags[i]),
            0,
            wxRIGHT,
            5);
    }

    auto topSizer = std::make_unique<wxBoxSizer>(wxVERTICAL);
    auto flags = wxLEFT | wxBOTTOM | wxRIGHT | wxEXPAND;

    topSizer->Add(bitsetView, 0, wxALL, 10);
    topSizer->Add(bitsetField, 0, flags, 10);
    topSizer->Add(flagsSizer.release(), 0, flags, 10);

    this->SetSizerAndFit(topSizer.release());
}
