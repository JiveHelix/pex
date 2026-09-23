#pragma once

#include <fields/fields.h>
#include <pex/interface.h>
#include <pex/group.h>
#include <pex/range_terminus.h>
#include <pex/tailors.h>


namespace pex
{

// TODO: Rename LinkedRange to Range


template<typename T>
struct LinkedRangesSettings
{
    T low;
    T high;

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
    using RangeMaker = MakeRange<Type, LowLimit, HighLimit>;

    template<template<typename> typename T>
    struct Schema
    {
        T<RangeMaker> low;
        T<RangeMaker> high;

        static constexpr auto fieldsTypeName = "LinkedRanges";
    };

    struct Finisher
    {
        struct Plain: public LinkedRangesSettings<Type>
        {
            using Reflector = LinkedRangesSettings<Type>;

            Plain(Type low_, Type high_)
                :
                LinkedRangesSettings<Type>{low_, high_}
            {

            }

            Plain()
                :
                LinkedRangesSettings<Type>{
                    LowValue::template Get<Type>(),
                    HighValue::template Get<Type>()}
            {

            }

            Type GetRange() const
            {
                return this->high - this->low;
            }
        };

        template<typename Base>
        struct Model: public Base
        {
        private:
            using Low = ControlTailor<RangeMaker>;

            using LowTerminus = pex::RangeTerminus<Model, Low>;

            using High = ControlTailor<RangeMaker>;

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

                this->lowTerminus_.Emplace(this, this->low, &Model::OnLow_);
                this->highTerminus_.Emplace(this, this->high, &Model::OnHigh_);
            }

            void SetInitial(const Plain &plain)
            {
                if (plain.low > plain.high)
                {
                    throw std::logic_error("low > high");
                }

                if (plain.high > this->high.GetMaximum())
                {
                    detail::AccessReference(this->high.maximum)
                        .SetWithoutNotify(plain.high);
                }

                if (plain.low < this->low.GetMinimum())
                {
                    detail::AccessReference(this->low.minimum)
                        .SetWithoutNotify(plain.low);
                }

                detail::AccessReference(this->high.minimum)
                    .SetWithoutNotify(plain.low);

                detail::AccessReference(this->low.maximum)
                    .SetWithoutNotify(plain.high);

                this->high.value.SetFilter(
                    pex::model::RangeFilter<Type>(
                        this->high.minimum.Get(),
                        this->high.maximum.Get()));

                this->low.value.SetFilter(
                    pex::model::RangeFilter<Type>(
                        this->low.minimum.Get(),
                        this->low.maximum.Get()));

                this->high.SetInitial(plain.high);
                this->low.SetInitial(plain.low);
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

    using Group = pex::Group<Schema, Finisher>;
    using Settings = typename Group::Plain;

    template<typename Upstream>
    using Control = typename Group::template Control<Upstream>;
};


template<typename Range>
struct MakeLinkedRanges_
{
    using Type = LinkedRanges
    <
        typename Range::Type,
        typename Range::Minimum,
        typename Range::Minimum,
        typename Range::Maximum,
        typename Range::Maximum
    >;
};


template<typename Range>
using MakeLinkedRanges = typename MakeLinkedRanges_<Range>::Type;


} // end namespace pex
