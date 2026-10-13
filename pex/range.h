#pragma once

#include <fields/fields.h>
#include <pex/interface.h>
#include <pex/group.h>
#include <pex/bounded_terminus.h>
#include <pex/tailors.h>


namespace pex
{


template
<
    typename Options,
    typename Access = pex::GetAndSetTag
>
struct Range
{
    using BoundedMaker = MakeBoundedOptions<Options, Access>;
    using Type = typename Options::Type;

    template<template<typename> typename T>
    struct Schema
    {
        T<BoundedMaker> low;
        T<BoundedMaker> high;

        static constexpr auto fieldsTypeName = "Range";
    };

    struct Finisher
    {
        struct Plain: public Schema<pex::Identity>
        {
            using Reflector = Schema<pex::Identity>;

            Plain()
                :
                Reflector{Options::minimum, Options::maximum}
            {

            }

            Plain(Type low, Type high)
                :
                Reflector{low, high}
            {

            }

            Type GetSpan() const
            {
                return this->high - this->low;
            }

            bool operator==(const Plain &other) const
            {
                return this->low == other.low && this->high == other.high;
            }
        };

        template<typename Base>
        struct Model: public Base
        {
        private:
            using Low = ControlTailor<BoundedMaker>;

            using LowTerminus = pex::BoundedTerminus<Model, Low>;

            using High = ControlTailor<BoundedMaker>;

            using HighTerminus = pex::BoundedTerminus<Model, High>;

            LowTerminus lowTerminus_;
            HighTerminus highTerminus_;

        public:
            static constexpr auto observerName = "Range::Model";

            Model()
                :
                Base(),
                lowTerminus_(),
                highTerminus_()
            {
                this->low.TrimMaximum(this->high.Get());
                this->high.TrimMinimum(this->low.Get());

                PEX_LOG(
                    "Connecting Range::Model as observer: ",
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
                    pex::model::BoundedFilter<Type>(
                        this->high.minimum.Get(),
                        this->high.maximum.Get()));

                this->low.value.SetFilter(
                    pex::model::BoundedFilter<Type>(
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


template<typename BoundedMaker>
using MakeRange = Range<typename BoundedMaker::Options>;


} // end namespace pex
