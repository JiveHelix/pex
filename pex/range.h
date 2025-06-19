/**
  * @file range.h
  *
  * @brief A class encapsulating a bounded value, with pex::Value's for value,
  * minimum, and maximum.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 17 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <iostream>
#include <stdexcept>
#include <limits>
#include <jive/type_traits.h>
#include <jive/platform.h>
#include <jive/optional.h>
#include "pex/value.h"
#include "pex/detail/filters.h"
#include "pex/reference.h"
#include "pex/converting_filter.h"
#include "pex/traits.h"
#include "pex/make_control.h"
#include "pex/terminus.h"


#ifdef USE_OBSERVER_NAME
#include <jive/describe_type.h>
#endif


#undef max
#undef min

namespace pex
{


// TODO: Rename Range to Bounded or Clamped


// Forward declarations
// Necessary for the friend declarations in model::Range

namespace control
{

template<typename, typename, typename>
class Range;


} // end namespace control


template<typename, typename>
class RangeTerminus;


template<
    signed integral,
    unsigned fractional = 0,
    unsigned denominator = 1000000>
struct Limit
{
    template<typename T = double>
    static constexpr T Get()
    {
        if constexpr (std::is_integral_v<T>)
        {
            return static_cast<T>(integral);
        }
        else
        {
            return static_cast<T>(integral)
                + (static_cast<T>(fractional)
                    / static_cast<T>(denominator));
        }
    }
};


namespace model
{


template<typename T>
struct RangeFilter
{
    using Type = jive::RemoveOptional<T>;

    RangeFilter(Type minimum, Type maximum)
        :
        minimum_(minimum),
        maximum_(maximum)
    {

    }

    RangeFilter(const RangeFilter &other)
        :
        minimum_(other.minimum_),
        maximum_(other.maximum_)
    {

    }

    RangeFilter & operator=(const RangeFilter &other)
    {
        this->minimum_ = other.minimum_;
        this->maximum_ = other.maximum_;

        return *this;
    }

    Type Get(Type value) const
    {
        return value;
    }

    Type Set(Type value) const
    {
        return std::max(
            this->minimum_,
            std::min(value, this->maximum_));
    }

    Type GetMinimum() const
    {
        return this->minimum_;
    }

    Type GetMaximum() const
    {
        return this->maximum_;
    }

private:
    Type minimum_;
    Type maximum_;
};


template<typename T, typename Value, typename Enable = void>
struct Minimum
{
    static constexpr auto value = Value::template Get<T>();
};

template<typename T, typename Value>
struct Minimum<T, Value, std::enable_if_t<std::is_void_v<Value>>>
{
    static constexpr auto value = std::numeric_limits<T>::lowest();
};

template<typename T, typename Value, typename Enable = void>
struct Maximum
{
    static constexpr auto value = Value::template Get<T>();
};

template<typename T, typename Value>
struct Maximum<T, Value, std::enable_if_t<std::is_void_v<Value>>>
{
    static constexpr auto value = std::numeric_limits<T>::max();
};


template
<
    typename T,
    // Defaults to numeric_limits<T>::lowest() and ::max()
    typename initialMinimum = void,
    typename initialMaximum = void,
    template<typename, typename, typename> typename ValueTemplate = Value_
>
class Range: Separator
{
public:
    using Type = T;

    static_assert(
        std::is_arithmetic_v<jive::RemoveOptional<T>>,
        "Designed only for arithmetic types.");

    // TODO: Make Access a template parameter.
    using Access = GetAndSetTag;

    using Value = ValueTemplate<T, RangeFilter<T>, Access>;
    using LimitType = jive::RemoveOptional<Type>;

    static constexpr auto defaultMinimum =
        Minimum<LimitType, initialMinimum>::value;

    static constexpr auto defaultMaximum =
        Maximum<LimitType, initialMaximum>::value;

    using Limit = typename ::pex::model::Value<jive::RemoveOptional<Type>>;
    using Callable = typename Value::Callable;

public:
    Range()
        :
        value(RangeFilter<Type>(defaultMinimum, defaultMaximum)),
        minimum(defaultMinimum),
        maximum(defaultMaximum),
        reset(),
        defaultValue_(value.Get()),
        resetTerminus_(this, this->reset, &Range::OnReset_)
    {
        REGISTER_PEX_NAME(this, "model::Range");
        REGISTER_PEX_NAME_WITH_PARENT(&this->value, this, "value");
        REGISTER_PEX_NAME_WITH_PARENT(&this->minimum, this, "minimum");
        REGISTER_PEX_NAME_WITH_PARENT(&this->maximum, this, "maximum");
    }

    explicit Range(Type value_)
        :
        value(
            value_,
            RangeFilter<Type>(
                Minimum<LimitType, initialMinimum>::value,
                Maximum<LimitType, initialMaximum>::value)),
        minimum(Minimum<LimitType, initialMinimum>::value),
        maximum(Maximum<LimitType, initialMaximum>::value),
        reset(),
        defaultValue_(this->value.Get()),
        resetTerminus_(this, this->reset, &Range::OnReset_)
    {

    }

    ~Range()
    {
        UNREGISTER_PEX_NAME(&this->value, "value");
        UNREGISTER_PEX_NAME(&this->minimum, "minimum");
        UNREGISTER_PEX_NAME(&this->maximum, "maximum");
        UNREGISTER_PEX_NAME(this, "model::Range");
    }

    void Connect(void * observer, Callable callable)
    {
        this->value.Connect(observer, callable);
    }

    void SetInitial(pex::Argument<Type> initialValue)
    {
        this->SetWithoutNotify_(initialValue);
        this->defaultValue_ = initialValue;
    }

    void SetDefault(pex::Argument<Type> defaultValue)
    {
        this->defaultValue_ = defaultValue;
    }

    Type GetDefault() const
    {
        return this->defaultValue_;
    }

    void SetLimits(LimitType minimum_, LimitType maximum_)
    {
        if (maximum_ < minimum_)
        {
            // maximum may be equal to minimum, even though it doesn't seem
            // very useful.
            throw std::invalid_argument("requires maximum >= minimum");
        }

        // All model values will be changed, so any request to Get() will
        // return the new value, but notifications will not be sent until we
        // exit this scope.
        auto changeMinimum = ::pex::Defer<Limit>(this->minimum);
        auto changeMaximum = ::pex::Defer<Limit>(this->maximum);

        changeMinimum.Set(minimum_);
        changeMaximum.Set(maximum_);

        this->value.SetFilter(
            RangeFilter<Type>(
                this->minimum.Get(),
                this->maximum.Get()));

        if constexpr (jive::IsOptional<T>)
        {
            auto value_ = this->value.Get();

            if (!value_)
            {
                return;
            }

            auto changeValue = ::pex::Defer<Value>(this->value);

            if (*value_ < minimum_)
            {
                changeValue.Set(minimum_);
            }
            else if (*value_ > maximum_)
            {
                changeValue.Set(maximum_);
            }
            else
            {
                // The value did not change.
                // Do not send notification.
                changeValue.Clear();
            }
        }
        else
        {
            auto value_ = this->value.Get();
            auto changeValue = ::pex::Defer<Value>(this->value);

            if (value_ < minimum_)
            {
                changeValue.Set(minimum_);
            }
            else if (value_ > maximum_)
            {
                changeValue.Set(maximum_);
            }
            else
            {
                // The value did not change.
                // Do not send notification.
                changeValue.Clear();
            }
        }
    }

    void SetMinimum(LimitType minimum_)
    {
        minimum_ = std::min(minimum_, this->maximum.Get());

        // Use a pex::Defer to delay notifying of the bounds change until
        // the value has been (maybe) adjusted.
        auto changeMinimum = ::pex::Defer<Limit>(this->minimum);
        changeMinimum.Set(minimum_);

        this->value.SetFilter(RangeFilter<Type>(
            this->minimum.Get(),
            this->maximum.Get()));

        if constexpr (jive::IsOptional<Type>)
        {
            auto value_ = this->value.Get();

            if (value_)
            {
                if (*value_ < minimum_)
                {
                    // The current value is less than the new minimum.
                    // Adjust the value to the minimum.
                    this->value.Set(minimum_);
                }
            }
        }
        else
        {
            if (this->value.Get() < minimum_)
            {
                // The current value is less than the new minimum.
                // Adjust the value to the minimum.
                this->value.Set(minimum_);
            }
        }
    }

    void SetMaximum(LimitType maximum_)
    {
        maximum_ = std::max(maximum_, this->minimum.Get());
        auto changeMaximum = ::pex::Defer<Limit>(this->maximum);
        changeMaximum.Set(maximum_);

        this->value.SetFilter(RangeFilter<Type>(
            this->minimum.Get(),
            this->maximum.Get()));

        if constexpr (jive::IsOptional<Type>)
        {
            auto value_ = this->value.Get();

            if (value_)
            {
                if (*value_ > maximum_)
                {
                    this->value.Set(maximum_);
                }
            }
        }
        else
        {
            if (this->value.Get() > maximum_)
            {
                this->value.Set(maximum_);
            }
        }
    }

    /*
     * Trim functions adjust the value filter within the limits of minimum and
     * maximum.
     */
    void TrimMinimum(Type minimum_)
    {
        auto filterMaximum = this->value.GetFilter().GetMaximum();
        minimum_ = std::min(minimum_, filterMaximum);
        auto changeMinimum = ::pex::Defer<Limit>(this->minimum);

        if (minimum_ < this->minimum.Get())
        {
            // The new minimum is extending the valid range.
            changeMinimum.Set(minimum_);
        }
        else
        {
            // The minimum is within the allowable range.
            // Only adjust the filter on the value.
            // Do not send a notification for the minimum.
            changeMinimum.Clear();
        }

        this->value.SetFilter(RangeFilter<Type>(minimum_, filterMaximum));

        if constexpr (jive::IsOptional<Type>)
        {
            auto value_ = this->value.Get();

            if (value_)
            {
                if (*value_ < minimum_)
                {
                    // The current value is less than the new minimum.
                    // Adjust the value to the minimum.
                    this->value.Set(minimum_);
                }
            }
        }
        else
        {
            if (this->value.Get() < minimum_)
            {
                // The current value is less than the new minimum.
                // Adjust the value to the minimum.
                this->value.Set(minimum_);
            }
        }
    }

    void TrimMaximum(Type maximum_)
    {
        auto filterMinimum = this->value.GetFilter().GetMinimum();
        maximum_ = std::max(maximum_, filterMinimum);
        auto changeMaximum = ::pex::Defer<Limit>(this->maximum);

        if (maximum_ > this->maximum.Get())
        {
            // The new maximum is extending the valid range.
            changeMaximum.Set(maximum_);
        }
        else
        {
            // The maximum is within the allowable range.
            // Only adjust the filter on the value.
            // Do not send a notification for the maximum.
            changeMaximum.Clear();
        }

        this->value.SetFilter(RangeFilter<Type>(filterMinimum, maximum_));

        if constexpr (jive::IsOptional<Type>)
        {
            auto value_ = this->value.Get();

            if (value_)
            {
                if (*value_ > maximum_)
                {
                    this->value.Set(maximum_);
                }
            }
        }
        else
        {
            if (this->value.Get() > maximum_)
            {
                this->value.Set(maximum_);
            }
        }
    }

    std::enable_if_t<HasAccess<SetTag, Access>>
    Set(Argument<Type> value_)
    {
        this->value.Set(value_);
    }

    Type Get() const
    {
        return this->value.Get();
    }

    explicit operator Type () const
    {
        return this->value.Get();
    }

    Range & operator=(Argument<Type> value_)
    {
        this->Set(value_);
        return *this;
    }

    // This function is used in debug assertions to check that other entities
    // hold a reference to a model value.
    bool HasModel() const { return true; }

    LimitType GetMaximum() const
    {
        return this->maximum.Get();
    }

    LimitType GetMinimum() const
    {
        return this->minimum.Get();
    }

    template<typename, typename, typename>
    friend class ::pex::control::Range;

    template<typename, typename>
    friend class ::pex::RangeTerminus;

    template<typename>
    friend class ::pex::Reference;

private:
    void SetWithoutNotify_(Argument<Type> value_)
    {
        detail::AccessReference(this->value).SetWithoutNotify(value_);
    }

    void DoNotify_()
    {
        detail::AccessReference(this->value).DoNotify();
    }

    void OnReset_()
    {
        this->value.Set(this->defaultValue_);
    }

private:

#ifdef ENABLE_PEX_LOG
    uint8_t separator_;
#endif

public:
    Value value;
    Limit minimum;
    Limit maximum;
    Signal reset;
    Type defaultValue_;

    using ResetTerminus = pex::Terminus<Range, Signal>;
    ResetTerminus resetTerminus_;
};


template<typename Upstream>
class AddRange
{
    static_assert(
        std::is_arithmetic_v<typename Upstream::Type>,
        "Designed only for arithmetic types.");

public:
    // The Range value is a control to a model somewhere else.
    using Value =
        pex::control::FilteredValue
        <
            Upstream,
            RangeFilter<typename Upstream::Type>
        >;

    static_assert(!IsCopyable<Value>);
    static_assert(::pex::IsDirect<::pex::UpstreamHolderT<Value>>);

    using Type = typename Value::Type;

    // The Range limits are stored here in the model.
    using Limit = typename ::pex::model::Value<Type>;

public:
    AddRange()
        :
        value_(),
        minimum_(std::numeric_limits<Type>::lowest()),
        maximum_(std::numeric_limits<Type>::max())
    {

    }

    AddRange(PexArgument<Upstream> upstream)
        :
        value_(upstream),
        minimum_(std::numeric_limits<Type>::lowest()),
        maximum_(std::numeric_limits<Type>::max())
    {
        this->value_.SetFilter(RangeFilter<Type>(
            this->minimum_.Get(),
            this->maximum_.Get()));
    }

    ~AddRange()
    {
        PEX_LOG("Disconnect ", LookupPexName(this));
        this->value_.Disconnect(this);
    }

    void SetUpstream(PexArgument<Upstream> upstream)
    {
        this->value_ = Value(upstream);

        this->value_.SetFilter(RangeFilter<Type>(
            this->minimum_.Get(),
            this->maximum_.Get()));

        PEX_LOG("model::AddRange: ", LookupPexName(this));
        PEX_LOG("model::AddRange.value_: ", &this->value_);
        PEX_LOG("model::AddRange.minimum_: ", &this->minimum_);
        PEX_LOG("model::AddRange.maximum_: ", &this->maximum_);
    }

    void SetLimits(Type minimum, Type maximum)
    {
        if (maximum < minimum)
        {
            // maximum may be equal to minimum, even though it doesn't seem
            // very useful.
            throw std::invalid_argument("requires maximum >= minimum");
        }

        // All model values will be changed, so any request to Get() will
        // return the new value, but notifications will not be sent until we
        // exit this scope.
        auto changeMinimum = ::pex::Defer<Limit>(this->minimum_);
        auto changeMaximum = ::pex::Defer<Limit>(this->maximum_);
        auto changeValue = ::pex::Defer<Value>(this->value_);

        changeMinimum.Set(minimum);
        changeMaximum.Set(maximum);

        this->value_.SetFilter(RangeFilter<Type>(
            this->minimum_.Get(),
            this->maximum_.Get()));

        if (this->value_.Get() < minimum)
        {
            changeValue.Set(minimum);
        }
        else if (this->value_.Get() > maximum)
        {
            changeValue.Set(maximum);
        }
    }

    void SetMinimum(Type minimum)
    {
        minimum = std::min(minimum, this->maximum_.Get());

        // Use a pex::Defer to delay notifying of the bounds change until
        // the value has been (maybe) adjusted.
        auto changeMinimum = ::pex::Defer<Limit>(this->minimum_);
        changeMinimum.Set(minimum);

        this->value_.SetFilter(RangeFilter<Type>(
            this->minimum_.Get(),
            this->maximum_.Get()));

        if (this->value_.Get() < minimum)
        {
            // The current value is less than the new minimum.
            // Adjust the value to the minimum.
            this->value_.Set(minimum);
        }
    }

    void SetMaximum(Type maximum)
    {
        maximum = std::max(maximum, this->minimum_.Get());
        auto changeMaximum = ::pex::Defer<Limit>(this->maximum_);
        changeMaximum.Set(maximum);

        this->value_.SetFilter(RangeFilter<Type>(
            this->minimum_.Get(),
            this->maximum_.Get()));

        if (this->value_.Get() > maximum)
        {
            this->value_.Set(maximum);
        }
    }

    void Set(Argument<Type> value)
    {
        this->value_.Set(value);
    }

    Type GetMaximum() const
    {
        return this->maximum_.Get();
    }

    Type GetMinimum() const
    {
        return this->minimum_.Get();
    }

    bool HasModel() const
    {
        return true;
    }

    template<typename, typename, typename>
    friend class ::pex::control::Range;

private:
    Value value_;
    Limit minimum_;
    Limit maximum_;
};


} // namespace model


template<typename ...T>
struct IsModelRange_: std::false_type {};

template<typename ...T>
struct IsModelRange_<model::Range<T...>>: std::true_type {};

template
<
    typename T,
    typename U,
    typename V,
    template<typename, typename, typename> typename W
>
struct IsModelRange_<model::Range<T, U, V, W>>: std::true_type {};

template<typename ...T>
struct IsModelRange_<model::AddRange<T...>>: std::true_type {};

template<typename ...T>
inline constexpr bool IsModelRange = IsModelRange_<T...>::value;


template<typename T>
struct Bounds
{
    T minimum;
    T maximum;

    T Constrain(T value) const
    {
        return std::min(this->maximum, std::max(value, this->minimum));
    }
};


namespace control
{

template<typename T, typename Enable = std::void_t<>>
struct HasControl_: std::false_type {};

template<typename T>
struct HasControl_
<
    T,
    std::void_t<typename T::Control>
>
: std::true_type {};

template<typename T>
inline constexpr bool HasControl = HasControl_<T>::value;


template
<
    typename Value,
    typename Filter,
    typename Access,
    typename Enable = std::void_t<>
>
struct ValueControl_
{
    using Type = pex::control::Value_<Value, Filter, Access>;
};


// A Value may declare its own FilteredControl
template
<
    typename Value,
    typename Filter,
    typename Access
>
struct ValueControl_
    <
        Value,
        Filter,
        Access,
        std::void_t<typename Value::template FilteredControl<Filter, Access>>
    >
{
    using Type = typename Value::template FilteredControl<Filter, Access>;
};


template<typename ...Args>
using ValueControl = typename ValueControl_<Args...>::Type;


template
<
    typename Upstream_,
    typename Filter_ = NoFilter,
    typename Access_ = pex::GetAndSetTag
>
class Range: Separator
{
public:
    using Upstream = Upstream_;
    using Filter = Filter_;
    using Unfiltered = typename Upstream::Value::Type;
    using Access = Access_;

    static constexpr bool isPexCopyable =
        pex::detail::FilterIsNoneOrStatic<Unfiltered, Filter, Access>;

    using Value = ValueControl<typename Upstream::Value, Filter, Access>;

    using Type = typename Value::Type;

    // Read-only Limit type for accessing range bounds.
    using Limit =
        pex::control::FilteredValue
        <
            typename Upstream::Limit,
            Filter,
            pex::GetTag
        >;

    using LimitType = typename Limit::Type;
    using Callable = typename Value::Callable;

    Range() = default;

    Range(Upstream &upstream)
        :
        value(upstream.value),
        minimum(upstream.minimum),
        maximum(upstream.maximum),
        reset(upstream.reset)
    {
        REGISTER_PEX_NAME(this, "control::Range");
        REGISTER_PEX_NAME_WITH_PARENT(&this->value, this, "value");
        REGISTER_PEX_NAME_WITH_PARENT(&this->minimum, this, "minimum");
        REGISTER_PEX_NAME_WITH_PARENT(&this->maximum, this, "maximum");
    }

    Range(Upstream &upstream, const Filter &filter)
        :
        Range(upstream)
    {
        this->SetFilter(filter);
    }

    void SetFilter(const Filter &filter)
    {
        this->value.SetFilter(filter);
        this->minimum.SetFilter(filter);
        this->maximum.SetFilter(filter);
    }

    const Filter & GetFilter() const
    {
        return this->value.GetFilter();
    }

    ~Range()
    {
        UNREGISTER_PEX_NAME(this, "control::Range");
        UNREGISTER_PEX_NAME(&this->value, "value");
        UNREGISTER_PEX_NAME(&this->minimum, "minimum");
        UNREGISTER_PEX_NAME(&this->maximum, "maximum");
    }

    template<typename OtherFilter, typename OtherAccess>
    Range(const Range<Upstream, OtherFilter, OtherAccess> &other)
        :
        value(other.value),
        minimum(other.minimum),
        maximum(other.maximum),
        reset(other.reset)
    {
        REGISTER_PEX_NAME(this, "control::Range");
        REGISTER_PEX_NAME_WITH_PARENT(&this->value, this, "value");
        REGISTER_PEX_NAME_WITH_PARENT(&this->minimum, this, "minimum");
        REGISTER_PEX_NAME_WITH_PARENT(&this->maximum, this, "maximum");
    }

    template<typename OtherFilter, typename OtherAccess>
    Range(
        const Range<Upstream, OtherFilter, OtherAccess> &other,
        const Filter &filter)
        :
        value(other.value),
        minimum(other.minimum),
        maximum(other.maximum),
        reset(other.reset)
    {
        REGISTER_PEX_NAME(this, "control::Range");
        REGISTER_PEX_NAME_WITH_PARENT(&this->value, this, "value");
        REGISTER_PEX_NAME_WITH_PARENT(&this->minimum, this, "minimum");
        REGISTER_PEX_NAME_WITH_PARENT(&this->maximum, this, "maximum");

        this->SetFilter(filter);
    }

    template<typename OtherFilter, typename OtherAccess>
    Range & operator=(
        const Range<Upstream, OtherFilter, OtherAccess> &other)
    {
        this->value = other.value;
        this->minimum = other.minimum;
        this->maximum = other.maximum;
        this->reset = other.reset;

        return *this;
    }

    Range(const Range &other)
        :
        value(other.value),
        minimum(other.minimum),
        maximum(other.maximum),
        reset(other.reset)
    {
        REGISTER_PEX_NAME(this, "control::Range");
        REGISTER_PEX_NAME_WITH_PARENT(&this->value, this, "value");
        REGISTER_PEX_NAME_WITH_PARENT(&this->minimum, this, "minimum");
        REGISTER_PEX_NAME_WITH_PARENT(&this->maximum, this, "maximum");
    }

    Range & operator=(const Range &other)
    {
        this->value = other.value;
        this->minimum = other.minimum;
        this->maximum = other.maximum;
        this->reset = other.reset;

        return *this;
    }

    Range(Range &&other)
        :
        value(std::move(other.value)),
        minimum(std::move(other.minimum)),
        maximum(std::move(other.maximum)),
        reset(std::move(other.reset))
    {
        REGISTER_PEX_NAME(this, "control::Range");
        REGISTER_PEX_NAME_WITH_PARENT(&this->value, this, "value");
        REGISTER_PEX_NAME_WITH_PARENT(&this->minimum, this, "minimum");
        REGISTER_PEX_NAME_WITH_PARENT(&this->maximum, this, "maximum");
    }

    Range & operator=(Range &&other)
    {
        this->value = std::move(other.value);
        this->minimum = std::move(other.minimum);
        this->maximum = std::move(other.maximum);
        this->reset = std::move(other.reset);

        return *this;
    }

    explicit operator Type () const
    {
        return this->value.Get();
    }

    bool HasObserver(void *observer)
    {
        return this->value.HasObserver(observer);
    }

    void Connect(void *observer, Callable callable)
    {
        this->value.Connect(observer, callable);
    }

    void Disconnect(void *observer)
    {
        this->value.Disconnect(observer);
    }

    Type Get() const
    {
        return this->value.Get();
    }

    void Set(Argument<Type> value_)
    {
        this->value.Set(value_);
    }

    Range & operator=(Argument<Type> value_)
    {
        this->value.Set(value_);
        return *this;
    }

    bool HasModel() const
    {
        return this->value.HasModel()
            && this->minimum.HasModel()
            && this->maximum.HasModel();
    }

    Bounds<LimitType> GetBounds()
    {
        return {
            this->minimum.Get(),
            this->maximum.Get()};
    }

    void ClearConnections()
    {
        this->value.ClearConnections();
        this->minimum.ClearConnections();
        this->maximum.ClearConnections();
    }

#ifdef ENABLE_PEX_LOG
private:
    uint8_t separator_;

public:
#endif
    Value value;
    Limit minimum;
    Limit maximum;

    Signal<> reset;

    template<typename>
    friend class ::pex::Reference;

private:
    void SetWithoutNotify_(Argument<Type> value_)
    {
        detail::AccessReference(this->value).SetWithoutNotify(value_);
    }

    void DoNotify_()
    {
        detail::AccessReference(this->value).DoNotify();
    }
};


// Converts values directly between model type and control type.
template
<
    typename Upstream,
    typename Converted,
    typename Access = pex::GetAndSetTag
>
using ConvertingRange = Range
    <
        Upstream,
        ConvertingFilter
        <
            typename Upstream::Type,
            jive::MatchOptional<typename Upstream::Type, Converted>
        >,
        Access
    >;


template
<
    typename Converted,
    typename Upstream,
    typename Filter,
    typename Access
>
auto MakeConvertingRange(Range<Upstream, Filter, Access> range)
{
    using Result = ConvertingRange<Upstream, Converted, Access>;
    return Result(range);
}


// Maps control values linearly between minimum and maximum model values.
template
<
    typename Upstream,
    typename Access = pex::GetAndSetTag
>
using LinearRange = Range
    <
        Upstream,
        LinearFilter<typename Upstream::Value::Type>,
        Access
    >;


template
<
    typename Upstream,
    ssize_t slope,
    typename Access = pex::GetAndSetTag
>
using StaticLinearRange = Range
    <
        Upstream,
        StaticLinearFilter<typename Upstream::Value::Type, slope>,
        Access
    >;


template
<
    typename Upstream,
    unsigned base,
    unsigned divisor
>
using LogarithmicRange = Range
    <
        Upstream,
        LogarithmicFilter<typename Upstream::Value::Type, base, divisor>
    >;


} // namespace control


template<typename ...T>
struct IsControlRange_: std::false_type {};

template<typename ...T>
struct IsControlRange_<control::Range<T...>>: std::true_type {};

template<typename ...T>
inline constexpr bool IsControlRange = IsControlRange_<T...>::value;


// Specializations of MakeControl for Range.
template<typename P>
struct MakeControl<P, std::enable_if_t<IsModelRange<P>>>
{
    using Control = control::Range<P>;
    using Upstream = P;
};


template<typename P>
struct MakeControl<P, std::enable_if_t<IsControlRange<P>>>
{
    using Control = P;
    using Upstream = typename P::Upstream;
};


template<typename P>
inline constexpr bool IsRange = IsControlRange<P> || IsModelRange<P>;


template<typename Observer, typename Upstream_>
class RangeTerminus
{
public:
    using UpstreamControl = typename MakeControl<Upstream_>::Control;

    using Upstream = typename MakeControl<Upstream_>::Upstream;
    using Value = pex::Terminus<Observer, typename UpstreamControl::Value>;
    using Limit = pex::Terminus<Observer, typename UpstreamControl::Limit>;
    using Type = typename UpstreamControl::Type;
    using Callable = typename Value::Callable;

    Value value;
    Limit minimum;
    Limit maximum;

    static constexpr bool isPexCopyable = UpstreamControl::isPexCopyable;

    RangeTerminus()
        :
        value{},
        minimum{},
        maximum{}
    {

    }

    RangeTerminus(Observer *observer, const UpstreamControl &pex)
        :
        value(observer, pex.value),
        minimum(observer, pex.minimum),
        maximum(observer, pex.maximum)
    {

    }

    RangeTerminus(
        Observer *observer,
        const UpstreamControl &pex,
        Callable callable)
        :
        value(observer, pex.value, callable),
        minimum(observer, pex.minimum),
        maximum(observer, pex.maximum)
    {

    }

    RangeTerminus(Observer *observer, UpstreamControl &&pex)
        :
        value(observer, std::move(pex.value)),
        minimum(observer, std::move(pex.minimum)),
        maximum(observer, std::move(pex.maximum))
    {

    }

    RangeTerminus(
        Observer *observer,
        typename MakeControl<Upstream_>::Upstream &upstream)
        :
        value(observer, upstream.value_),
        minimum(observer, upstream.minimum_),
        maximum(observer, upstream.maximum_)
    {

    }

    RangeTerminus(const RangeTerminus &other) = delete;
    RangeTerminus(RangeTerminus &&other) = delete;
    RangeTerminus & operator=(const RangeTerminus &) = delete;
    RangeTerminus & operator=(RangeTerminus &&) = delete;

    // Copy construct
    RangeTerminus(Observer *observer, const RangeTerminus &other)
        :
        value(observer, other.value),
        minimum(observer, other.minimum),
        maximum(observer, other.maximum)
    {

    }

    // Copy construct from other observer
    template<typename O>
    RangeTerminus(
        Observer *observer,
        const RangeTerminus<O, Upstream_> &other)
        :
        value(observer, other.value),
        minimum(observer, other.minimum),
        maximum(observer, other.maximum)
    {

    }

    // Move construct
    RangeTerminus(Observer *observer, RangeTerminus &&other)
        :
        value(observer, std::move(other.value)),
        minimum(observer, std::move(other.minimum)),
        maximum(observer, std::move(other.maximum))
    {

    }

    // Move construct from other observer
    template<typename O>
    RangeTerminus(
        Observer *observer,
        RangeTerminus<O, Upstream_> &&other)
        :
        value(observer, std::move(other.value)),
        minimum(observer, std::move(other.minimum)),
        maximum(observer, std::move(other.maximum))
    {

    }

    // Copy assign
    RangeTerminus & Assign(
        Observer *observer,
        const RangeTerminus<Observer, Upstream_> &other)
    {
        this->value.Assign(observer, other.value);
        this->minimum.Assign(observer, other.minimum);
        this->maximum.Assign(observer, other.maximum);

        return *this;
    }

    // Move assign
    RangeTerminus & Assign(
        Observer *observer,
        RangeTerminus<Observer, Upstream_> &&other)
    {
        this->value.Assign(observer, std::move(other.value));
        this->minimum.Assign(observer, std::move(other.minimum));
        this->maximum.Assign(observer, std::move(other.maximum));

        return *this;
    }

    void Connect(Callable callable)
    {
        this->value.Connect(callable);
    }

    void Disconnect(Observer *)
    {
        this->Disconnect();
    }

    void Disconnect()
    {
        this->value.Disconnect();
        this->minimum.Disconnect();
        this->maximum.Disconnect();
    }

    explicit operator Type () const
    {
        return this->value.Get();
    }

    explicit operator UpstreamControl () const
    {
        UpstreamControl result;
        result.value = this->value;
        result.minimum = this->minimum;
        result.maximum = this->maximum;

        return result;
    }

    Type Get() const
    {
        return this->value.Get();
    }

    void Set(Argument<Type> value_)
    {
        this->value.Set(value_);
    }

    bool HasModel() const
    {
        return this->value.HasModel()
            && this->minimum.HasModel()
            && this->maximum.HasModel();
    }

    template<typename>
    friend class ::pex::Reference;

private:
    void SetWithoutNotify_(Argument<Type> value_)
    {
        detail::AccessReference(this->value).SetWithoutNotify(value_);
    }

    void DoNotify_()
    {
        detail::AccessReference(this->value).DoNotify();
    }
};


} // namespace pex
