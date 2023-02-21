#pragma once

#include <fields/fields.h>
#include <pex/interface.h>
#include <pex/group.h>
#include <pex/range.h>


namespace pex
{


template
<
    typename Type,
    typename LowLimit,
    typename LowValue,
    typename HighLimit,
    typename HighValue
>
struct LinkedRanges
{
    template<typename T>
    struct Fields
    {
        static constexpr auto fields = std::make_tuple(
            fields::Field(&T::low, "low"),
            fields::Field(&T::high, "high"));
    };

    template<template<typename> typename T>
    struct Template
    {
        T<pex::MakeRange<Type, LowLimit, HighLimit>> low;
        T<pex::MakeRange<Type, LowLimit, HighLimit>> high;

        static constexpr auto fields = Fields<Template>::fields;
        static constexpr auto fieldsTypeName = "LinkedRanges";
    };

    struct Settings: public Template<pex::Identity>
    {
        static Settings Default()
        {
            return {{
                LowValue::template Get<Type>(),
                HighValue::template Get<Type>()}};
        }
    };

    using Group = pex::Group<Fields, Template, Settings>;

    struct Model: public Group::Model
    {
    private:
        using Low = decltype(Model::low);

        using LowTerminus = pex::RangeTerminus<Model, Low>;

        using High = decltype(Model::high);

        using HighTerminus = pex::RangeTerminus<Model, High>;

        LowTerminus lowTerminus_;
        HighTerminus highTerminus_;

    public:
        static constexpr auto observerName = "LinkedRanges::Model";

        Model()
            :
            Group::Model(Settings::Default()),
            lowTerminus_(this, this->low),
            highTerminus_(this, this->high)
        {
            this->low.TrimMaximum(this->high.Get());
            this->high.TrimMinimum(this->low.Get());

            PEX_LOG("Connecting LinkedRanges::Model as observer: ", this);

            this->lowTerminus_.Connect(&Model::OnLow_);
            this->highTerminus_.Connect(&Model::OnHigh_);
        }

    private:
        void OnLow_(Type value)
        {
            this->high.TrimMinimum(value);
        }

        void OnHigh_(Type value)
        {
            this->low.TrimMaximum(value);
        }
    };

    using GroupMaker = pex::MakeGroup<Group, Model>;

    template<typename Observer>
    using Control = typename Group::template Control<Observer>;

    template<typename Observer>
    using Terminus = typename Group::template Terminus<Observer>;
};


} // end namespace pex


