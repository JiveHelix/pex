#pragma once

#include <bitset>
#include <array>

#include "jive/formatter.h"
#include "pex/bitset.h"
#include "pex/wx/check_box.h"


namespace pex
{


namespace wx
{


template<size_t bitCount>
struct FlagNames
{
    std::array<std::string, bitCount> names;

    static FlagNames MakeDefault()
    {
        FlagNames<bitCount> result;

        for (size_t i = 0; i < bitCount; ++i)
        {
            result.names[i] = jive::FastFormatter<16>("bit %zu", i);    
        }

        return result;
    }

    const std::string & operator[](size_t index) const
    {
        return this->names[index];
    }

    std::string & operator[](size_t index)
    {
        return this->names[index];
    }
};


template<size_t bitCount>
class BitsetCheckBoxes: public wxControl
{
public:
    using Control = BitsetFlagsControl<bitCount>;
    using CheckBoxControl = typename Control::Flag;

    BitsetCheckBoxes(
        wxWindow *parent,
        Control control,
        const FlagNames<bitCount> &flagNames =
            FlagNames<bitCount>::MakeDefault(),
        long style = 0,
        long checkBoxStyle = 0,
        int orient = wxHORIZONTAL)
        :
        wxControl(
            parent,
            wxID_ANY,
            wxDefaultPosition,
            wxDefaultSize,
            style),
        control_(control)
    {
        auto flagsSizer = std::make_unique<wxBoxSizer>(orient);

        for (size_t i = 0; i < bitCount; ++i)
        {
            flagsSizer->Add(
                new CheckBox<CheckBoxControl>(
                    this,
                    flagNames[i],
                    this->control_.flags[i],
                    checkBoxStyle),
                0,
                (orient == wxHORIZONTAL) ? wxRIGHT : wxBOTTOM,
                5);
        }
        
        this->SetSizerAndFit(flagsSizer.release());
    }

    ~BitsetCheckBoxes()
    {
        this->DestroyChildren();
    }

private:
    Control control_; 
};


template<size_t bitCount>
struct MakeBitsetCheckBoxes
{
public:
    using Type = BitsetCheckBoxes<bitCount>;
    using Control = typename Type::Control;

    Control control;
    FlagNames<bitCount> flagNames = FlagNames<bitCount>::MakeDefault();
    long style = 0;
    long checkBoxStyle = 0;
    int orient = wxHORIZONTAL;
    
    Type * operator()(wxWindow *parent) const
    {
        return new Type(
            parent,
            this->control,
            this->flagNames,
            this->style,
            this->checkBoxStyle,
            this->orient);
    }
};


} // end namespace wx


} // end namespace pex
