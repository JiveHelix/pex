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
#include <type_traits>
#include <stdexcept>
#include <limits>
#include <jive/platform.h>
#include "pex/value.h"
#include "pex/detail/filters.h"
#include "pex/reference.h"
#include "pex/converting_filter.h"
#include "pex/traits.h"
#include "pex/terminus.h"


namespace pex
{


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
    RangeFilter(T minimum, T maximum)
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

    T Get(T value) const
    {
        return value;
    }

    T Set(T value) const
    {
        return std::max(
            this->minimum_,
            std::min(value, this->maximum_));
    }

    T GetMinimum() const
    {
        return this->minimum_;
    }

    T GetMaximum() const
    {
        return this->maximum_;
    }

private:
    T minimum_;
    T maximum_;
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
    template<typename, typename> typename ValueTemplate = Value_
>
class Range
{
public:
    using Type = T;

    static_assert(
        std::is_arithmetic_v<Type>,
        "Designed only for arithmetic types.");

    using Value = ValueTemplate<T, RangeFilter<T>>;
    using Limit = typename ::pex::model::Value<Type>;
    using Callable = typename Value::Callable;

public:
    Range()
        :
        value_(
            RangeFilter<Type>(
                Minimum<T, initialMinimum>::value,
                Maximum<T, initialMaximum>::value)),
        minimum_(Minimum<T, initialMinimum>::value),
        maximum_(Maximum<T, initialMaximum>::value)
    {

    }

    explicit Range(Type value)
        :
        value_(
            value,
            RangeFilter<Type>(
                Minimum<T, initialMinimum>::value,
                Maximum<T, initialMaximum>::value)),
        minimum_(Minimum<T, initialMinimum>::value),
        maximum_(Maximum<T, initialMaximum>::value)
    {

    }

    ~Range()
    {

    }

    void Connect(void * observer, Callable callable)
    {
        this->value_.Connect(observer, callable);
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

        this->value_.SetFilter(
            RangeFilter<Type>(
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
        else
        {
            // The value did not change.
            // Do not send notification.
            changeValue.Clear();
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

    /*
     * Trim functions adjust the value filter within the limits of minimum and
     * maximum.
     */
    void TrimMinimum(Type minimum)
    {
        auto filterMaximum = this->value_.GetFilter().GetMaximum();
        minimum = std::min(minimum, filterMaximum);
        auto changeMinimum = ::pex::Defer<Limit>(this->minimum_);

        if (minimum < this->minimum_.Get())
        {
            // The new minimum is extending the valid range.
            changeMinimum.Set(minimum);
        }
        else
        {
            // The minimum is within the allowable range.
            // Only adjust the filter on the value.
            // Do not send a notification for the minimum.
            changeMinimum.Clear();
        }

        this->value_.SetFilter(RangeFilter<Type>(minimum, filterMaximum));

        if (this->value_.Get() < minimum)
        {
            this->value_.Set(minimum);
        }
    }

    void TrimMaximum(Type maximum)
    {
        auto filterMinimum = this->value_.GetFilter().GetMinimum();
        maximum = std::max(maximum, filterMinimum);
        auto changeMaximum = ::pex::Defer<Limit>(this->maximum_);

        if (maximum > this->maximum_.Get())
        {
            // The new maximum is extending the valid range.
            changeMaximum.Set(maximum);
        }
        else
        {
            // The maximum is within the allowable range.
            // Only adjust the filter on the value.
            // Do not send a notification for the maximum.
            changeMaximum.Clear();
        }

        this->value_.SetFilter(RangeFilter<Type>(filterMinimum, maximum));

        if (this->value_.Get() > maximum)
        {
            this->value_.Set(maximum);
        }
    }

    void Set(Argument<Type> value)
    {
        this->value_.Set(value);
    }

    Type Get() const
    {
        return this->value_.Get();
    }

    explicit operator Type () const
    {
        return this->value_.Get();
    }

    Range & operator=(Argument<Type> value)
    {
        this->Set(value);
        return *this;
    }

    // This function is used in debug assertions to check that other entities
    // hold a reference to a model value.
    bool HasModel() const { return true; }

    Type GetMaximum() const
    {
        return this->maximum_.Get();
    }

    Type GetMinimum() const
    {
        return this->minimum_.Get();
    }

    template<typename, typename, typename>
    friend class ::pex::control::Range;

    template<typename, typename>
    friend class ::pex::RangeTerminus;

    template<typename>
    friend class ::pex::Reference;

private:
    void SetWithoutNotify_(Argument<Type> value)
    {
        detail::AccessReference<Value>(this->value_).SetWithoutNotify(value);
    }

    void DoNotify_()
    {
        detail::AccessReference<Value>(this->value_).DoNotify();
    }

private:
    Value value_;
    Limit minimum_;
    Limit maximum_;
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
            void,
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
        PEX_LOG("Disconnect");
        this->value_.Disconnect(this);
    }

    void SetUpstream(PexArgument<Upstream> upstream)
    {
        this->value_ = Value(upstream);

        this->value_.SetFilter(RangeFilter<Type>(
            this->minimum_.Get(),
            this->maximum_.Get()));

        PEX_LOG("model::AddRange: ", this);
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

    template
    <
        typename,
        typename,
        typename,
        typename
    >
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
    template<typename, typename> typename W
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
    using Type = pex::control::FilteredValue<Value, Filter, Access>;
};


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
class Range
{
public:
    static constexpr bool isPexCopyable = true;

    using Upstream = Upstream_;
    using Filter = Filter_;
    using Type = typename Upstream::Value::Type;
    using Access = Access_;

    static_assert(
        pex::detail::FilterIsNoneOrStatic<Type, Filter, Access>,
        "This class is designed for free function filters.");

    using Value = ValueControl<typename Upstream::Value, Filter, Access>;

    // Read-only Limit type for accessing range bounds.
    using Limit =
        pex::control::FilteredValue
        <
            typename Upstream::Limit,
            Filter,
            pex::GetTag
        >;

    using Callable = typename Value::Callable;

    Range() = default;

    Range(Upstream &upstream)
    {
        if constexpr (IsModelRange<Upstream>)
        {
            this->value = Value(upstream.value_);
            this->minimum = Limit(upstream.minimum_);
            this->maximum = Limit(upstream.maximum_);
        }
        else
        {
            this->value = Value(upstream.value);
            this->minimum = Limit(upstream.minimum);
            this->maximum = Limit(upstream.maximum);
        }
    }

    template<typename OtherFilter, typename OtherAccess>
    Range(const Range<Upstream, OtherFilter, OtherAccess> &other)
        :
        value(other.value),
        minimum(other.minimum),
        maximum(other.maximum)
    {

    }

    template<typename OtherFilter, typename OtherAccess>
    Range & operator=(
        const Range<Upstream, OtherFilter, OtherAccess> &other)
    {
        this->value = other.value;
        this->minimum = other.minimum;
        this->maximum = other.maximum;

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

    Bounds<Type> GetBounds()
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

    Value value;
    Limit minimum;
    Limit maximum;

    template<typename>
    friend class ::pex::Reference;

private:
    void SetWithoutNotify_(Argument<Type> value_)
    {
        detail::AccessReference<Value>(this->value).SetWithoutNotify(value_);
    }

    void DoNotify_()
    {
        detail::AccessReference<Value>(this->value).DoNotify();
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
        ConvertingFilter<typename Upstream::Value::Type, Converted>,
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
    ssize_t slope,
    typename Access = pex::GetAndSetTag
>
using LinearRange = Range
    <
        Upstream,
        LinearFilter<typename Upstream::Value::Type, slope>,
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
    using ControlTemplate = typename MakeControl<Upstream_>::Control;

    using Upstream = typename MakeControl<Upstream_>::Upstream;
    using Value = pex::Terminus<Observer, typename ControlTemplate::Value>;
    using Limit = pex::Terminus<Observer, typename ControlTemplate::Limit>;
    using Type = typename ControlTemplate::Type;
    using Callable = typename Value::Callable;

    Value value;
    Limit minimum;
    Limit maximum;

    static constexpr bool isPexCopyable = true;

    RangeTerminus()
        :
        value{},
        minimum{},
        maximum{}
    {

    }

    RangeTerminus(Observer *observer, const ControlTemplate &pex)
        :
        value(observer, pex.value),
        minimum(observer, pex.minimum),
        maximum(observer, pex.maximum)
    {

    }

    RangeTerminus(
        Observer *observer,
        const ControlTemplate &pex,
        Callable callable)
        :
        value(observer, pex.value, callable),
        minimum(observer, pex.minimum),
        maximum(observer, pex.maximum)
    {

    }

    RangeTerminus(Observer *observer, ControlTemplate &&pex)
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

    explicit operator Type () const
    {
        return this->value.Get();
    }

    explicit operator ControlTemplate () const
    {
        ControlTemplate result;
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
        detail::AccessReference<Value>(this->value).SetWithoutNotify(value_);
    }

    void DoNotify_()
    {
        detail::AccessReference<Value>(this->value).DoNotify();
    }
};


} // namespace pex
