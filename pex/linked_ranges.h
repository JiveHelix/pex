#pragma once

#include <fields/fields.h>
#include <pex/interface.h>
#include <pex/group.h>
#include <pex/range.h>
#include <pex/selectors.h>


namespace pex
{


template<typename T>
struct LinkedRangesFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::low, "low"),
        fields::Field(&T::high, "high"));
};


template<typename T>
struct LinkedRangesSettings
{
    T low;
    T high;

    static constexpr auto fields =
        LinkedRangesFields<LinkedRangesSettings>::fields;

    static constexpr auto fieldsTypeName = "LinkedRanges";
};


TEMPLATE_COMPARISON_OPERATORS(LinkedRangesSettings)


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
    using Fields = LinkedRangesFields<T>;

    using RangeMaker = MakeRange<Type, LowLimit, HighLimit>;

    template<template<typename> typename T>
    struct Template
    {
        T<RangeMaker> low;
        T<RangeMaker> high;

        static constexpr auto fields = LinkedRangesFields<Template>::fields;
        static constexpr auto fieldsTypeName = "LinkedRanges";
    };

    struct Custom
    {
        struct Plain: public LinkedRangesSettings<Type>
        {
            Plain()
                :
                LinkedRangesSettings<Type>{
                    LowValue::template Get<Type>(),
                    HighValue::template Get<Type>()}
            {

            }
        };

        template<typename Base>
        struct Model: public Base
        {
        private:
            using Low = ControlSelector<RangeMaker>;

            using LowTerminus = pex::RangeTerminus<Model, Low>;

            using High = ControlSelector<RangeMaker>;

            using HighTerminus = pex::RangeTerminus<Model, High>;

            LowTerminus lowTerminus_;
            HighTerminus highTerminus_;

        public:
            static constexpr auto observerName = "LinkedRanges::Model";

            Model()
                :
                Base(),
                lowTerminus_(),
                highTerminus_()
            {
                this->low.TrimMaximum(this->high.Get());
                this->high.TrimMinimum(this->low.Get());

                PEX_LOG(
                    "Connecting LinkedRanges::Model as observer: ",
                    LookupPexName(this));

                this->lowTerminus_.Assign(
                    this,
                    LowTerminus(this, this->low, &Model::OnLow_));

                this->highTerminus_.Assign(
                    this,
                    HighTerminus(this, this->high, &Model::OnHigh_));
            }

            void SetMaximumValue(Type maximumValue)
            {
                // The maximum allowed value of "low" is high.
                // If the value of high is above the new maximumValue,
                // it will be reduced, trimming low.maximum with it.
                this->high.SetMaximum(maximumValue);
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
    };

    using Group = pex::Group<Fields, Template, Custom>;
    using Settings = typename Group::Plain;
    using Control = typename Group::Control;
};


} // end namespace pex
