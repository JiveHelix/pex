#pragma once

#include <bitset>
#include <array>

#include "jive/formatter.h"
#include "pex/value.h"
#include "pex/wx/check_box.h"


namespace pex
{


namespace wx
{


template<size_t bitCount>
using BitsetModel = pex::model::Value<std::bitset<bitCount>>;


template<size_t bitCount>
struct FlagFilter
{
public:
    using Bitset = std::bitset<bitCount>;

    FlagFilter() = default;

    FlagFilter(const BitsetModel<bitCount> *model, size_t index)
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
    const BitsetModel<bitCount> *model_;
    size_t index_;
};


template<size_t bitCount>
using FlagInterface =
    pex::interface::FilteredValue<
        void,
        BitsetModel<bitCount>,
        FlagFilter<bitCount>>;


template<size_t bitCount>
class BitsetFlagsInterface
{
public:
    using Filter = FlagFilter<bitCount>;
    using Flag = FlagInterface<bitCount>;

    BitsetFlagsInterface(BitsetModel<bitCount> *bitset)
        :
        flags{},
        filters_{}
    {
        for (size_t i = 0; i < bitCount; ++i)
        {
            this->filters_[i] = Filter(bitset, i);
            this->flags[i] = Flag(bitset, &this->filters_[i]);
        }
    }

    BitsetFlagsInterface(const BitsetFlagsInterface &other)
        :
        flags(other.flags),
        filters_(other.filters_)
    {
        // The flags need to point to the filters that are members of the new
        // instance.
        for (size_t i = 0; i < bitCount; ++i)
        {
            flags[i].SetFilter(&this->filters_[i]);
        }
    }

    BitsetFlagsInterface & operator=(const BitsetFlagsInterface &other)
    {
        this->flags = other.flags;
        this->filters_ = other.filters_;

        // The flags need to point to the filters that are members of this
        // instance.
        for (size_t i = 0; i < bitCount; ++i)
        {
            flags[i].SetFilter(&this->filters_[i]);
        }
    }

public:
    std::array<Flag, bitCount> flags;

private:
    std::array<Filter, bitCount> filters_;
};


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
    using Interface = BitsetFlagsInterface<bitCount>;
    using CheckBoxInterface = typename Interface::Flag;

    BitsetCheckBoxes(
        wxWindow *parent,
        Interface interface,
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
        interface_(interface)
    {
        auto flagsSizer = std::make_unique<wxBoxSizer>(orient);

        for (size_t i = 0; i < bitCount; ++i)
        {
            flagsSizer->Add(
                new CheckBox<CheckBoxInterface>(
                    this,
                    flagNames[i],
                    this->interface_.flags[i],
                    checkBoxStyle),
                0,
                (orient == wxHORIZONTAL) ? wxRIGHT : wxBOTTOM,
                5);
        }
        
        this->SetSizerAndFit(flagsSizer.release());
    }

private:
    Interface interface_; 
};


template<size_t bitCount>
struct MakeBitsetCheckBoxes
{
public:
    using Type = BitsetCheckBoxes<bitCount>;
    using Interface = typename Type::Interface;

    Interface interface;
    FlagNames<bitCount> flagNames = FlagNames<bitCount>::MakeDefault();
    long style = 0;
    long checkBoxStyle = 0;
    int orient = wxHORIZONTAL;
    
    Type * operator()(wxWindow *parent) const
    {
        return new Type(
            parent,
            this->interface,
            this->flagNames,
            this->style,
            this->checkBoxStyle,
            this->orient);
    }
};


} // end namespace wx


} // end namespace pex
