/**
  * @file bounded.h
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

#include <stdexcept>
#include <limits>
#include <jive/type_traits.h>
#include <jive/optional.h>
#include <pex/value.h>
#include <pex/reference.h>
#include <pex/converting_filter.h>
#include <pex/traits.h>
#include <pex/terminus.h>
#include <pex/bounded_options.h>
#include <pex/default_value_node.h>
#include <pex/detail/filters.h>


#ifdef USE_OBSERVER_NAME
#include <jive/describe_type.h>
#endif


#undef max
#undef min

namespace pex
{



// Forward declarations
// Necessary for the friend declarations in model::Bounded

namespace control
{

template<typename, typename, typename>
class Bounded;


} // end namespace control


template<typename, typename>
class BoundedTerminus;


namespace model
{


template<typename T, typename Filter_ = NoFilter>
struct BoundedFilter: public FilterAdapter<T, Filter_>
{
    static_assert(
        std::same_as<T, detail::FilteredType<T, Filter_>>,
        "Bounded filter expects the return type to be unchanged");

    using Filter = Filter_;
    using FilterBase = FilterAdapter<T, Filter>;
    using PlainType = jive::RemoveOptional<T>;
    using Type = T;

    BoundedFilter(PlainType minimum, PlainType maximum)
        :
        FilterBase(),
        minimum_(minimum),
        maximum_(maximum)
    {

    }

    BoundedFilter(Filter filter, PlainType minimum, PlainType maximum)
        :
        FilterBase(filter),
        minimum_(minimum),
        maximum_(maximum)
    {

    }

    BoundedFilter(const BoundedFilter &other)
        :
        FilterBase(other),
        minimum_(other.minimum_),
        maximum_(other.maximum_)
    {

    }

    BoundedFilter & operator=(const BoundedFilter &other)
    {
        this->FilterBase::operator=(other);
        this->minimum_ = other.minimum_;
        this->maximum_ = other.maximum_;

        return *this;
    }

    Type Get(Type value) const
    {
        return this->FilterOnGet_(value);
    }

    Type Set(Type value) const
    {
        if constexpr (jive::IsOptional<Type>)
        {
            if (!value)
            {
                // There is nothing to filter.

                return value;
            }

            return std::max(
                this->minimum_,
                std::min(*this->FilterOnSet_(*value), this->maximum_));
        }
        else
        {
            return std::max(
                this->minimum_,
                std::min(this->FilterOnSet_(value), this->maximum_));
        }
    }

    PlainType GetMinimum() const
    {
        return this->minimum_;
    }

    PlainType GetMaximum() const
    {
        return this->maximum_;
    }

    void SetExtrema(PlainType minimum, PlainType maximum)
    {
        this->minimum_ = minimum;
        this->maximum_ = maximum;
    }

private:
    PlainType minimum_;
    PlainType maximum_;
};


template
<
    typename Options
>
class Bounded: Separator
{
public:
    static constexpr bool isBoundedModel = true;
    static constexpr auto observerName = "pex::model::Bounded";

    using Type = Options::Type;

    static_assert(
        std::is_arithmetic_v<jive::RemoveOptional<Type>>,
        "Designed only for arithmetic types.");

    // TODO: Make Access a template parameter.
    using Access = GetAndSetTag;
    using Filter = typename Options::Filter;
    using ComposedFilter = BoundedFilter<Type, Filter>;

    using ValueNode =
        typename Options::template ValueNode<Type, ComposedFilter, Access>;

    using Value = typename ValueNode::Model;
    using LimitType = jive::RemoveOptional<Type>;

    static constexpr auto defaultMinimum = Options::minimum;
    static constexpr auto defaultMaximum = Options::maximum;

    using Limit = typename ::pex::model::Value<jive::RemoveOptional<Type>>;
    using Callable = typename Value::Callable;

public:
    Bounded()
        :
        value(ComposedFilter(defaultMinimum, defaultMaximum)),
        minimum(defaultMinimum),
        maximum(defaultMaximum),
        reset(),
        defaultValue_(value.Get()),

        resetTerminus_(
            PEX_THIS("model::Bounded"),
            PEX_MEMBER_PASS(this->reset),
            &Bounded::OnReset_)
    {
        PEX_NAME("model::Bounded");
        PEX_MEMBER(value);
        PEX_MEMBER(minimum);
        PEX_MEMBER(maximum);
    }

    explicit Bounded(Type value_)
        :
        value(
            value_,
            ComposedFilter(defaultMinimum, defaultMaximum)),

        minimum(defaultMinimum),
        maximum(defaultMaximum),
        reset(),
        defaultValue_(this->value.Get()),

        resetTerminus_(
            PEX_THIS("model::Bounded"),
            PEX_MEMBER_PASS(this->reset),
            &Bounded::OnReset_)
    {
        PEX_NAME("model::Bounded");
        PEX_MEMBER(value);
        PEX_MEMBER(minimum);
        PEX_MEMBER(maximum);
    }

    Bounded(Type value_, Filter filter)
        :
        value(
            value_,
            ComposedFilter(filter, defaultMinimum, defaultMaximum)),

        minimum(defaultMinimum),
        maximum(defaultMaximum),
        reset(),
        defaultValue_(this->value.Get()),

        resetTerminus_(
            PEX_THIS("model::Bounded"),
            PEX_MEMBER_PASS(this->reset),
            &Bounded::OnReset_)
    {
        PEX_NAME("model::Bounded");
        PEX_MEMBER(value);
        PEX_MEMBER(minimum);
        PEX_MEMBER(maximum);
    }

    ~Bounded()
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

    void SetFilter(Filter filter)
    {
        // value.GetFilter() returns a type that is convertible to
        // ComposedFilter &.
        // We are changing the inner filter type, not the BoundedFilter itself.
        static_cast<ComposedFilter &>(this->value.GetFilter())
            .SetFilter(filter);
    }

private:
    void SetExtrema_(LimitType minimum_, LimitType maximum_)
    {
        static_cast<ComposedFilter &>(this->value.GetFilter())
            .SetExtrema(minimum_, maximum_);
    }

public:
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

        this->SetExtrema_(
            this->minimum.Get(),
            this->maximum.Get());

        if constexpr (jive::IsOptional<Type>)
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

        this->SetExtrema_(
            this->minimum.Get(),
            this->maximum.Get());

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

        this->SetExtrema_(
            this->minimum.Get(),
            this->maximum.Get());

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

        this->SetExtrema_(minimum_, filterMaximum);

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

        this->SetExtrema_(filterMinimum, maximum_);

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

    Bounded & operator=(Argument<Type> value_)
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
    friend class ::pex::control::Bounded;

    template<typename, typename>
    friend class ::pex::BoundedTerminus;

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
            Bounded,
            ::pex::control::Signal<::pex::model::Signal>
        >;

    ResetTerminus resetTerminus_;
};


template<typename T>
using DefaultBounded = Bounded<DefaultBoundedOptions<T>>;


#if 0
template<typename Upstream>
class AddBounded
{
    static_assert(
        std::is_arithmetic_v<typename Upstream::Type>,
        "Designed only for arithmetic types.");

public:
    // The Bounded value is a control to a model somewhere else.
    using Value =
        pex::control::FilteredValue
        <
            Upstream,
            BoundedFilter<typename Upstream::Type>
        >;

    static_assert(!IsCopyable<Value>);
    static_assert(::pex::IsDirect<::pex::UpstreamHolderT<Value>>);

    using Type = typename Value::Type;

    // The Bounded limits are stored here in the model.
    using Limit = typename ::pex::model::Value<Type>;

public:
    AddBounded()
        :
        value_(),
        minimum_(std::numeric_limits<Type>::lowest()),
        maximum_(std::numeric_limits<Type>::max())
    {

    }

    AddBounded(PexArgument<Upstream> upstream)
        :
        value_(upstream),
        minimum_(std::numeric_limits<Type>::lowest()),
        maximum_(std::numeric_limits<Type>::max())
    {
        this->value_.SetFilter(BoundedFilter<Type>(
            this->minimum_.Get(),
            this->maximum_.Get()));
    }

    ~AddBounded()
    {
        PEX_LOG("Disconnect ", LookupPexName(this));
        this->value_.Disconnect(this);
    }

    void SetUpstream(PexArgument<Upstream> upstream)
    {
        this->value_ = Value(upstream);

        this->value_.SetFilter(BoundedFilter<Type>(
            this->minimum_.Get(),
            this->maximum_.Get()));

        PEX_LOG("model::AddBounded: ", LookupPexName(this));
        PEX_LOG("model::AddBounded.value_: ", &this->value_);
        PEX_LOG("model::AddBounded.minimum_: ", &this->minimum_);
        PEX_LOG("model::AddBounded.maximum_: ", &this->maximum_);
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

        this->value_.SetFilter(BoundedFilter<Type>(
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

        this->value_.SetFilter(BoundedFilter<Type>(
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

        this->value_.SetFilter(BoundedFilter<Type>(
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
    friend class ::pex::control::Bounded;

private:
    Value value_;
    Limit minimum_;
    Limit maximum_;
};


#endif


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
class Bounded: Separator
{
public:
    using Upstream = Upstream_;
    using Model = Upstream;
    using Filter = Filter_;
    using Unfiltered = typename Upstream::Value::Type;
    using Access = Access_;

    static constexpr bool isBoundedControl = true;

    static constexpr bool isPexCopyable =
        pex::detail::FilterIsNoneOrStatic<Unfiltered, Filter>;

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
            detail::PlainFilter<Unfiltered, Filter, pex::GetTag>,
            pex::GetTag
        >;

    using LimitType = typename Limit::Type;
    using Callable = typename Value::Callable;

    Bounded()
        :
        value(),
        minimum(),
        maximum(),
        reset()
    {
        PEX_NAME("control::Bounded");
        PEX_MEMBER(value);
        PEX_MEMBER(minimum);
        PEX_MEMBER(maximum);
        PEX_MEMBER(reset);
    }

    Bounded(Upstream &upstream)
        :
        value(upstream.value),
        minimum(upstream.minimum),
        maximum(upstream.maximum),
        reset(upstream.reset)
    {
        PEX_NAME("control::Bounded");
        PEX_MEMBER(value);
        PEX_MEMBER(minimum);
        PEX_MEMBER(maximum);
        PEX_MEMBER(reset);
    }

    Bounded(Upstream &upstream, const Filter &filter)
        :
        Bounded(upstream)
    {
        this->SetFilter(filter);
    }

    void Emplace(Upstream &upstream)
    {
        this->value.Emplace(upstream.value);
        this->minimum.Emplace(upstream.minimum);
        this->maximum.Emplace(upstream.maximum);
        this->reset.Emplace(upstream.reset);
    }

    void Emplace(const Bounded &other)
    {
        this->value.Emplace(other.value);
        this->minimum.Emplace(other.minimum);
        this->maximum.Emplace(other.maximum);
        this->reset.Emplace(other.reset);
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
    Bounded(const Bounded<Upstream, OtherFilter, OtherAccess> &other)
        :
        value(other.value),
        minimum(other.minimum),
        maximum(other.maximum),
        reset(other.reset)
    {
        PEX_NAME("control::Bounded");
        PEX_MEMBER(value);
        PEX_MEMBER(minimum);
        PEX_MEMBER(maximum);
        PEX_MEMBER(reset);
    }

    template<typename OtherFilter, typename OtherAccess>
    Bounded(
        const Bounded<Upstream, OtherFilter, OtherAccess> &other,
        const Filter &filter)
        :
        value(other.value),
        minimum(other.minimum),
        maximum(other.maximum),
        reset(other.reset)
    {
        PEX_NAME("control::Bounded");
        PEX_MEMBER(value);
        PEX_MEMBER(minimum);
        PEX_MEMBER(maximum);
        PEX_MEMBER(reset);

        this->SetFilter(filter);
    }

    template<typename OtherFilter, typename OtherAccess>
    Bounded & operator=(
        const Bounded<Upstream, OtherFilter, OtherAccess> &other)
    {
        this->value = other.value;
        this->minimum = other.minimum;
        this->maximum = other.maximum;
        this->reset = other.reset;

        return *this;
    }

    Bounded(const Bounded &other)
        :
        value(other.value),
        minimum(other.minimum),
        maximum(other.maximum),
        reset(other.reset)
    {
        PEX_NAME("control::Bounded");
        PEX_MEMBER(value);
        PEX_MEMBER(minimum);
        PEX_MEMBER(maximum);
        PEX_MEMBER(reset);
    }

    Bounded & operator=(const Bounded &other)
    {
        this->value = other.value;
        this->minimum = other.minimum;
        this->maximum = other.maximum;
        this->reset = other.reset;

        return *this;
    }

    Bounded(Bounded &&other)
        :
        value(std::move(other.value)),
        minimum(std::move(other.minimum)),
        maximum(std::move(other.maximum)),
        reset(std::move(other.reset))
    {
        PEX_NAME("control::Bounded");
        PEX_MEMBER(value);
        PEX_MEMBER(minimum);
        PEX_MEMBER(maximum);
        PEX_MEMBER(reset);
    }

    ~Bounded()
    {
        PEX_CLEAR_NAME(this);
        PEX_CLEAR_NAME(&this->value);
        PEX_CLEAR_NAME(&this->minimum);
        PEX_CLEAR_NAME(&this->maximum);
        PEX_CLEAR_NAME(&this->reset);
    }

    Bounded & operator=(Bounded &&other)
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

    Bounded & operator=(Argument<Type> value_)
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

    Bounds<LimitType> GetBounds() const
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
class BoundedMux: Separator
{
public:
    using Upstream = Upstream_;
    using Model = Upstream;
    using Unfiltered = typename Upstream::Value::Type;

    static constexpr bool isBoundedMux = true;

    using ValueNode = typename Upstream::ValueNode;
    using Value = typename ValueNode::Mux;
    using Reset = ::pex::control::SignalMux;
    using Type = typename Value::Type;

    using Limit = pex::control::Mux<typename Upstream::Limit>;

    using LimitType = typename Limit::Type;
    using Callable = typename Value::Callable;

    BoundedMux()
        :
        value(),
        minimum(),
        maximum(),
        reset()
    {
        PEX_NAME("control::BoundedMux");
        PEX_MEMBER(value);
        PEX_MEMBER(minimum);
        PEX_MEMBER(maximum);
        PEX_MEMBER(reset);
    }

    BoundedMux(Upstream &upstream)
        :
        value(upstream.value),
        minimum(upstream.minimum),
        maximum(upstream.maximum),
        reset(upstream.reset)
    {
        PEX_NAME("control::BoundedMux");
        PEX_MEMBER(value);
        PEX_MEMBER(minimum);
        PEX_MEMBER(maximum);
        PEX_MEMBER(reset);
    }

    BoundedMux(const BoundedMux &other) = delete;
    BoundedMux & operator=(const BoundedMux &other) = delete;
    BoundedMux(BoundedMux &&other) = delete;

    ~BoundedMux()
    {
        PEX_CLEAR_NAME(this);
        PEX_CLEAR_NAME(&this->value);
        PEX_CLEAR_NAME(&this->minimum);
        PEX_CLEAR_NAME(&this->maximum);
        PEX_CLEAR_NAME(&this->reset);
    }

    BoundedMux & operator=(BoundedMux &&other)
    {
        this->value = std::move(other.value);
        this->minimum = std::move(other.minimum);
        this->maximum = std::move(other.maximum);
        this->reset = std::move(other.reset);

        return *this;
    }

    void Emplace(Upstream &upstream)
    {
        this->value.Emplace(upstream.value);
        this->minimum.Emplace(upstream.minimum);
        this->maximum.Emplace(upstream.maximum);
        this->reset.Emplace(upstream.reset);
    }

    void Emplace(const BoundedMux &other)
    {
        this->value.Emplace(other.value);
        this->minimum.Emplace(other.minimum);
        this->maximum.Emplace(other.maximum);
        this->reset.Emplace(other.reset);
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

    BoundedMux & operator=(Argument<Type> value_)
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
struct BoundedFollow: public Bounded<Upstream, Filter, Access>
{
public:
    static constexpr bool isBoundedControl = false;
    static constexpr bool isBoundedFollow = true;

    using Base = Bounded<Upstream, Filter, Access>;
    using Base::Base;
};


// Converts values directly between model type and control type.
template
<
    typename Upstream,
    typename Converted,
    typename Access = pex::GetAndSetTag
>
using ConvertingBounded = Bounded
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
auto MakeConvertingBounded(Bounded<Upstream, Filter, Access> range)
{
    using Result = ConvertingBounded<Upstream, Converted, Access>;
    return Result(range);
}


// Maps control values linearly between minimum and maximum model values.
template
<
    typename Upstream,
    typename Access = pex::GetAndSetTag
>
using LinearBounded = Bounded
    <
        Upstream,
        LinearFilter<typename Upstream::Value::Type>,
        Access
    >;


template
<
    typename Upstream,
    int slope,
    typename Access = pex::GetAndSetTag
>
using StaticLinearBounded = Bounded
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
using LogarithmicBounded = Bounded
    <
        Upstream,
        LogarithmicFilter<typename Upstream::Value::Type, base, divisor>
    >;


} // namespace control



} // namespace pex
