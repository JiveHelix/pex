
#include <iostream>
#include <string>
#include <bitset>
#include <array>
#include "wxshim.h"
#include "pex/value.h"
#include "pex/wx/view.h"
#include "pex/wx/field.h"

#include "pex/wx/labeled_widget.h"
#include "pex/wx/check_box.h"

constexpr auto bitCount = 8;
using Bitset = std::bitset<bitCount>;

using BitsetModel = pex::model::Value<Bitset>;
using BitsetInterface = pex::interface::Value<void, BitsetModel>;

template<typename Observer, typename Bitset>
class FlagInterface
    :
    public pex::interface::Value<Observer, pex::model::Value<bool>>
{
public:
    template<typename AnyObserver, typename OtherBitset>
    friend class FlagInterface;

    using Model = pex::model::Value<Bitset>;

    FlagInterface() = default;

    FlagInterface(Model *model, size_t index)
        :
        bitsetModel_(model),
        index_(index)
    {
        if (this->bitsetModel_)
        {
            this->bitsetModel_->Connect(this, &FlagInterface::OnModelChanged_);
        }
    }

    ~FlagInterface()
    {
        if (this->bitsetModel_)
        {
            this->bitsetModel_->Disconnect(this);
        }
    }

    template<typename OtherObserver>
    FlagInterface(const FlagInterface<OtherObserver, Bitset> &other)
        :
        bitsetModel_(other.bitsetModel_),
        index_(other.index_)
    {
        if (this->bitsetModel_)
        {
            this->bitsetModel_->Connect(this, &FlagInterface::OnModelChanged_);
        }
    }

    template<typename OtherObserver>
    FlagInterface & operator=(const FlagInterface<OtherObserver, Bitset> &other)
    {
        if (this->bitsetModel_)
        {
            this->bitsetModel_->Disconnect(this);
        }

        this->bitsetModel_ = other.bitsetModel_;
        this->index_ = other.index_;

        if (this->bitsetModel_)
        {
            this->bitsetModel_->Connect(this, &FlagInterface::OnModelChanged_);
        }
    }

    void Set(bool value)
    {
        (*pex::Reference(*this->bitsetModel_))[this->index_] = value;
    }

    bool Get()
    {
        return this->bitsetModel_->Get()[this->index_];
    }

private:
    static void OnModelChanged_(
        void * observer,
        const Bitset &value)
    {
        // The model value has changed.
        // Update our observers.
        auto self = static_cast<FlagInterface<Observer, Bitset> *>(observer);
        self->Notify_(value[self->index_]);
    }

    Model *bitsetModel_;
    size_t index_;
};


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
    std::array<FlagInterface<void, std::bitset<bitCount>>, bitCount> flags;

    Interface(Model &model)
        :
        bitset(&model.bitset)
    {
        for (size_t i = 0; i < bitCount; ++i)
        {
            flags[i] =
                FlagInterface<void, std::bitset<bitCount>>(&model.bitset, i);
        }
    }
};


class ExampleApp: public wxApp
{
public:
    ExampleApp(): model_{}
    {

    }

    bool OnInit() override;

private:
    Model model_;
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
    ExampleFrame *exampleFrame =
        new ExampleFrame(Interface(this->model_));

    exampleFrame->Show();
    return true;
}

using CheckBox = pex::wx::CheckBox<FlagInterface<void, std::bitset<bitCount>>>;

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

    wxBoxSizer *flagsSizer = new wxBoxSizer(wxHORIZONTAL);

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

    wxBoxSizer *topSizer = new wxBoxSizer(wxVERTICAL);

    auto flags = wxLEFT | wxBOTTOM | wxRIGHT | wxEXPAND;

    topSizer->Add(bitsetView, 0, wxALL, 10);
    topSizer->Add(bitsetField, 0, flags, 10);
    topSizer->Add(flagsSizer, 0, flags, 10);

    this->SetSizerAndFit(topSizer);
}
