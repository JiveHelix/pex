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
#include <pex/terminus.h>
#include <pex/default_value_node.h>


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
    template<typename, typename, typename>
        typename ValueNode_ = ::pex::DefaultValueNode
>
class Range: Separator
{
public:
    static constexpr bool isRangeModel = true;

    using Type = T;

    static_assert(
        std::is_arithmetic_v<jive::RemoveOptional<T>>,
        "Designed only for arithmetic types.");

    // TODO: Make Access a template parameter.
    using Access = GetAndSetTag;

    using ValueNode = ValueNode_<T, RangeFilter<T>, Access>;
    using Value = typename ValueNode::Model;
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

        resetTerminus_(
            PEX_THIS("model::Range"),
            PEX_MEMBER_PASS(this->reset),
            &Range::OnReset_)
    {
        PEX_NAME("model::Range");
        PEX_MEMBER(value);
        PEX_MEMBER(minimum);
        PEX_MEMBER(maximum);
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

        resetTerminus_(
            PEX_THIS("model::Range"),
            PEX_MEMBER_PASS(this->reset),
            &Range::OnReset_)
    {
        PEX_NAME("model::Range");
        PEX_MEMBER(value);
        PEX_MEMBER(minimum);
        PEX_MEMBER(maximum);
    }

    ~Range()
    {
        PEX_CLEAR_NAME(this);
        PEX_CLEAR_NAME(&this->value);
        PEX_CLEAR_NAME(&this->minimum);
        PEX_CLEAR_NAME(&this->maximum);
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

    void Notify()
    {
        this->value.Notify();
        this->minimum.Notify();
        this->maximum.Notify();
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

    using ResetTerminus =
        pex::Terminus
        <
            Range,
            ::pex::control::Signal<::pex::model::Signal>
        >;

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
    using Model = Upstream;
    using Filter = Filter_;
    using Unfiltered = typename Upstream::Value::Type;
    using Access = Access_;

    static constexpr bool isRangeControl = true;

    static constexpr bool isPexCopyable =
        pex::detail::FilterIsNoneOrStatic<Unfiltered, Filter, Access>;

    using ValueNode = typename Upstream::ValueNode;

    using Value =
        typename Upstream::ValueNode::template Control
        <
            typename Upstream::Value,
            Filter,
            Access
        >;

    using Reset = ::pex::control::DefaultSignal;
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

    Range()
        :
        value(),
        minimum(),
        maximum(),
        reset()
    {
        PEX_NAME("control::Range");
        PEX_MEMBER(value);
        PEX_MEMBER(minimum);
        PEX_MEMBER(maximum);
        PEX_MEMBER(reset);
    }

    Range(Upstream &upstream)
        :
        value(upstream.value),
        minimum(upstream.minimum),
        maximum(upstream.maximum),
        reset(upstream.reset)
    {
        PEX_NAME("control::Range");
        PEX_MEMBER(value);
        PEX_MEMBER(minimum);
        PEX_MEMBER(maximum);
        PEX_MEMBER(reset);
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

    template<typename OtherFilter, typename OtherAccess>
    Range(const Range<Upstream, OtherFilter, OtherAccess> &other)
        :
        value(other.value),
        minimum(other.minimum),
        maximum(other.maximum),
        reset(other.reset)
    {
        PEX_NAME("control::Range");
        PEX_MEMBER(value);
        PEX_MEMBER(minimum);
        PEX_MEMBER(maximum);
        PEX_MEMBER(reset);
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
        PEX_NAME("control::Range");
        PEX_MEMBER(value);
        PEX_MEMBER(minimum);
        PEX_MEMBER(maximum);
        PEX_MEMBER(reset);

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
        PEX_NAME("control::Range");
        PEX_MEMBER(value);
        PEX_MEMBER(minimum);
        PEX_MEMBER(maximum);
        PEX_MEMBER(reset);
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
        PEX_NAME("control::Range");
        PEX_MEMBER(value);
        PEX_MEMBER(minimum);
        PEX_MEMBER(maximum);
        PEX_MEMBER(reset);
    }

    ~Range()
    {
        PEX_CLEAR_NAME(this);
        PEX_CLEAR_NAME(&this->value);
        PEX_CLEAR_NAME(&this->minimum);
        PEX_CLEAR_NAME(&this->maximum);
        PEX_CLEAR_NAME(&this->reset);
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

    Value value;
    Limit minimum;
    Limit maximum;

    Reset reset;

    template<typename>
    friend class ::pex::Reference;

    void Notify()
    {
        this->value.Notify();
        this->minimum.Notify();
        this->maximum.Notify();
    }

private:
    void SetWithoutNotify_(Argument<Type> value_)
    {
        detail::AccessReference(this->value).SetWithoutNotify(value_);
    }
};


template<typename Upstream_>
class RangeMux: Separator
{
public:
    using Upstream = Upstream_;
    using Model = Upstream;
    using Unfiltered = typename Upstream::Value::Type;

    static constexpr bool isRangeMux = true;

    using ValueNode = typename Upstream::ValueNode;
    using Value = typename ValueNode::Mux;
    using Reset = ::pex::control::SignalMux;
    using Type = typename Value::Type;

    using Limit = pex::control::Mux<typename Upstream::Limit>;

    using LimitType = typename Limit::Type;
    using Callable = typename Value::Callable;

    RangeMux()
        :
        value(),
        minimum(),
        maximum(),
        reset()
    {
        PEX_NAME("control::RangeMux");
        PEX_MEMBER(value);
        PEX_MEMBER(minimum);
        PEX_MEMBER(maximum);
        PEX_MEMBER(reset);
    }

    RangeMux(Upstream &upstream)
        :
        value(upstream.value),
        minimum(upstream.minimum),
        maximum(upstream.maximum),
        reset(upstream.reset)
    {
        PEX_NAME("control::RangeMux");
        PEX_MEMBER(value);
        PEX_MEMBER(minimum);
        PEX_MEMBER(maximum);
        PEX_MEMBER(reset);
    }

    RangeMux(const RangeMux &other) = delete;
    RangeMux & operator=(const RangeMux &other) = delete;
    RangeMux(RangeMux &&other) = delete;

    ~RangeMux()
    {
        PEX_CLEAR_NAME(this);
        PEX_CLEAR_NAME(&this->value);
        PEX_CLEAR_NAME(&this->minimum);
        PEX_CLEAR_NAME(&this->maximum);
        PEX_CLEAR_NAME(&this->reset);
    }

    RangeMux & operator=(RangeMux &&other)
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

    RangeMux & operator=(Argument<Type> value_)
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

    Value value;
    Limit minimum;
    Limit maximum;

    Reset reset;

    template<typename>
    friend class ::pex::Reference;

    void Notify()
    {
        this->value.Notify();
        this->minimum.Notify();
        this->maximum.Notify();
    }

    void ChangeUpstream(Upstream &upstream)
    {
        this->value.ChangeUpstream(upstream.value);
        this->minimum.ChangeUpstream(upstream.minimum);
        this->maximum.ChangeUpstream(upstream.maximum);
        this->reset.ChangeUpstream(upstream.reset);
    }

private:
    void SetWithoutNotify_(Argument<Type> value_)
    {
        detail::AccessReference(this->value).SetWithoutNotify(value_);
    }
};


template
<
    typename Upstream,
    typename Filter = NoFilter,
    typename Access = pex::GetAndSetTag
>
struct RangeFollow: public Range<Upstream, Filter, Access>
{
public:
    static constexpr bool isRangeControl = false;
    static constexpr bool isRangeFollow = true;

    using Base = Range<Upstream, Filter, Access>;
    using Base::Base;
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



} // namespace pex
