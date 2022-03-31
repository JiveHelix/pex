#pragma once

#include <bitset>
#include <array>
#include <memory>
#include "pex/value.h"


namespace pex
{


template<size_t bitCount>
using BitsetModel = pex::model::Value<std::bitset<bitCount>>;

template<size_t bitCount, typename Access = GetAndSetTag>
using BitsetControl =
    pex::control::Value_<void, BitsetModel<bitCount>, NoFilter, Access>;


template<size_t bitCount>
struct FlagFilter
{
public:
    using Bitset = std::bitset<bitCount>;
    using Control = BitsetControl<bitCount, GetTag>;

    FlagFilter() = default;

    FlagFilter(Control control, size_t index)
        :
        control_(control),
        index_(index)
    {

    }

    bool Get(const Bitset &bitset) const
    {
        return bitset[this->index_];
    }

    Bitset Set(bool value) const
    {
        Bitset result = this->control_.Get();
        result[this->index_] = value;
        return result;
    }

private:
    Control control_;
    size_t index_;
};


template<size_t bitCount>
using FlagControl =
    pex::control::FilteredValue<
        void,
        BitsetControl<bitCount>,
        FlagFilter<bitCount>>;


template<size_t bitCount>
class BitsetFlagsControl
{
public:
    using Filter = FlagFilter<bitCount>;
    using FilteredFlag = FlagControl<bitCount>;

    using Flag = pex::control::Value<void, FilteredFlag>;

    BitsetFlagsControl(BitsetControl<bitCount> bitset)
        :
        flags{}
    {
        for (size_t i = 0; i < bitCount; ++i)
        {
            this->filteredFlags_[i] = std::make_shared<FilteredFlag>(
                bitset,
                Filter(bitset, i));

            this->flags[i] = Flag(*this->filteredFlags_[i]);
        }
    }

public:
    std::array<Flag, bitCount> flags;

private:
    std::array<std::shared_ptr<FilteredFlag>, bitCount> filteredFlags_;
};


} // end namespace pex
