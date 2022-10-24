/**
  * @file control_value.h
  *
  * @brief Implements control Value nodes.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 22 Jul 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once


#include <iostream>
#include "pex/model_value.h"
#include "pex/traits.h"


namespace pex
{


namespace control
{


template<
    typename Observer_,
    typename Pex_,
    typename Filter_ = NoFilter,
    typename Access_ = GetAndSetTag
>
class Value_
    :
    public detail::NotifyMany
    <
        ValueConnection
        <
            Observer_,
            typename UpstreamT<Pex_>::Type,
            Filter_
        >,
        Access_
    >
{
public:
    using Observer = Observer_;
    using Pex = Pex_;

    // If Pex_ is a already a control::Value_, then Upstream is that
    // control::Value_, else it is Direct<Pex>
    using Upstream = UpstreamT<Pex_>;
    using Filter = Filter_;
    using Access = Access_;
    using This = Value_<Observer, Pex, Filter, Access>;

    using UpstreamType = typename Upstream::Type;
    using Type = detail::FilteredType<UpstreamType, Filter>;

    using Callable =
        typename ValueConnection
        <
            Observer_,
            typename Upstream::Type,
            Filter
        >::Callable;

    // Make any template specialization of Value_ a 'friend' class.
    template <typename, typename, typename, typename>
    friend class ::pex::control::Value_;

    template<typename>
    friend class ::pex::Reference;

    // A control::Value with a stateful filter cannot be copied, so it can be
    // managed by Direct when needed.
    template<typename>
    friend class ::pex::model::Direct;

    static_assert(!std::is_same_v<Filter, void>, "Filter must not be void.");

    static_assert(
        detail::FilterIsNoneOrValid<UpstreamType, Filter, Access>);

    Value_(): upstream_(), filter_() {}

    explicit Value_(PexArgument<Pex> pex)
        :
        upstream_(pex),
        filter_()
    {
        if constexpr (HasAccess<GetTag, Access>)
        {
            PEX_LOG("Connect ", this);
            this->upstream_.Connect(this, &Value_::OnUpstreamChanged_);
        }
    }

    explicit Value_(PexArgument<Pex> pex, Filter filter)
        :
        upstream_(pex),
        filter_(filter)
    {
        static_assert(
            detail::FilterIsMember<UpstreamType, Filter>,
            "A void or static filter cannot be set.");

        if constexpr (HasAccess<GetTag, Access>)
        {
            PEX_LOG("Connect ", this);
            this->upstream_.Connect(this, &Value_::OnUpstreamChanged_);
        }
    }

    ~Value_()
    {
        PEX_LOG(
            "control::Value_::~Value_::Disconnect: ",
            this,
            " from ",
            &this->upstream_);

        this->upstream_.Disconnect(this);
    }

    /** 
     ** Allow copy and assignment from another Value that may have different
     ** observers and filters, but tracks the same model.
     **/
    template<typename OtherObserver, typename OtherFilter, typename OtherAccess>
    Value_(
        const Value_<OtherObserver, Pex, OtherFilter, OtherAccess> &other)
        :
        upstream_(other.upstream_),
        filter_()
    {
        // Allow the copy if the other has access at or above ours.
        static_assert(
            HasAccess<Access, OtherAccess>,
            "Cannot copy from another value without equal or greater access.");

        // You can replace a filter with another filter, but only if it uses
        // static functions?
        static_assert(
            IsCopyable<Value_<OtherObserver, Pex, OtherFilter, OtherAccess>>,
            "Value is not copyable.");

        if constexpr (HasAccess<GetTag, Access>)
        {
            PEX_LOG(
                "Copy from OtherObserver: ",
                this,
                " to ",
                &this->upstream_);

            this->upstream_.Connect(this, &Value_::OnUpstreamChanged_);
        }
    }

    // Enable the assignment operator if the other has access at or above ours.
    template<typename OtherObserver, typename OtherFilter, typename OtherAccess>
    std::enable_if_t
    <
        HasAccess<Access, OtherAccess>
        && IsCopyable<Value_<OtherObserver, Pex, OtherFilter, OtherAccess>>,
        Value_ &
    >
    operator=(
        const Value_<OtherObserver, Pex, OtherFilter, OtherAccess> &other)
    {
        PEX_LOG("Disconnect ", this);
        this->upstream_.Disconnect(this);
        this->upstream_ = other.upstream_;

        if constexpr (HasAccess<GetTag, Access>)
        {
            PEX_LOG("Connect ", this);
            this->upstream_.Connect(this, &Value_::OnUpstreamChanged_);
        }

        return *this;
    }

    Value_(const Value_ &other)
        :
        upstream_(other.upstream_),
        filter_()
    {
        static_assert(
            IsCopyable<Value_>,
            "This value is not copyable.");

        if constexpr (HasAccess<GetTag, Access>)
        {
            PEX_LOG("Copy from other: ", this, " to ", &this->upstream_);
            this->upstream_.Connect(this, &Value_::OnUpstreamChanged_);
        }
    }

    Value_(Value_ &&other)
        :
        upstream_(std::move(other.upstream_)),
        filter_(std::move(other.filter_))
    {
        if constexpr (HasAccess<GetTag, Access>)
        {
            PEX_LOG("Connect ", this);
            this->upstream_.Connect(this, &Value_::OnUpstreamChanged_);
        }
    }

    Value_ & operator=(const Value_ &other)
    {
        static_assert(
            IsCopyable<Value_>,
            "This value is not copyable.");

        static_assert(
            !detail::FilterIsMember<UpstreamType, Filter>,
            "IsCopyable implies that Filter uses static functions.");

        PEX_LOG("Disconnect ", this);
        this->upstream_.Disconnect(this);
        this->upstream_ = other.upstream_;

        if constexpr (HasAccess<GetTag, Access>)
        {
            PEX_LOG("Connect ", this);
            this->upstream_.Connect(
                this,
                &Value_::OnUpstreamChanged_);
        }

        return *this;
    }

    Value_ & operator=(Value_ &&other)
    {
        PEX_LOG("Disconnect ", this);
        this->upstream_.Disconnect(this);
        this->upstream_ = std::move(other.upstream_);
        this->filter_ = std::move(other.filter_);

        if constexpr (HasAccess<GetTag, Access>)
        {
            PEX_LOG("Connect ", this);
            this->upstream_.Connect(
                this,
                &Value_::OnUpstreamChanged_);
        }

        return *this;
    }

    template<typename OtherObserver>
    using Downstream = Value_
        <
            OtherObserver,
            This,
            NoFilter,
            Access
        >;

    template<typename OtherObserver>
    Downstream<OtherObserver> GetDownstream()
    {
        return Downstream<OtherObserver>(*this);
    }

    void SetFilter(Filter filter)
    {
        static_assert(
            detail::FilterIsMember<UpstreamType, Filter>,
            "Static or void Filter does not require a filter instance.");

        this->filter_ = filter;
    }

    Type Get() const
    {
        static_assert(
            HasAccess<GetTag, Access>,
            "Cannot Get a write-only value.");

        if constexpr (std::is_same_v<NoFilter, Filter>)
        {
            return this->upstream_.Get();
        }
        else
        {
            return this->FilterOnGet_(this->upstream_.Get());
        }
    }

    explicit operator Type () const
    {
        return this->Get();
    }

    void Set(Argument<Type> value)
    {
        static_assert(
            HasAccess<SetTag, Access>,
            "Cannot Set a read-only value.");

        if constexpr (std::is_same_v<NoFilter, Filter>)
        {
            this->upstream_.Set(value);
        }
        else
        {
            this->upstream_.Set(this->FilterOnSet_(value));
        }
    }

    Value_ & operator=(Argument<Type> value)
    {
        this->Set(value);
        return *this;
    }

    bool HasModel() const
    {
        return this->upstream_.HasModel();
    }

private:
    void SetWithoutNotify_(Argument<Type> value)
    {
        static_assert(
            HasAccess<SetTag, Access>,
            "Cannot Set a read-only value.");

        if constexpr (std::is_same_v<NoFilter, Filter>)
        {
            this->upstream_.SetWithoutNotify_(value);
        }
        else
        {
            this->upstream_.SetWithoutNotify_(this->FilterOnSet_(value));
        }
    }

    void DoNotify_()
    {
        this->upstream_.DoNotify_();
    }

    UpstreamType FilterOnSet_(Argument<Type> value) const
    {
        if constexpr (std::is_same_v<NoFilter, Filter>)
        {
            return value;
        }
        else if constexpr (detail::SetterIsMember<UpstreamType, Filter>)
        {
            REQUIRE_HAS_VALUE(this->filter_);
            return this->filter_->Set(value);
        }
        else
        {
            // The filter is not a member function.
            return Filter::Set(value);
        }
    }

    Type FilterOnGet_(Argument<UpstreamType> value) const
    {
        if constexpr (std::is_same_v<NoFilter, Filter>)
        {
            return value;
        }
        else if constexpr (detail::GetterIsMember<UpstreamType, Filter>)
        {
            REQUIRE_HAS_VALUE(this->filter_);
            return this->filter_->Get(value);
        }
        else
        {
            // The filter is not a member function.
            return Filter::Get(value);
        }
    }

    static void OnUpstreamChanged_(
        void * observer,
        Argument<UpstreamType> value)
    {
        // The model value has changed.
        // Update our observer.
        auto self = static_cast<Value_ *>(observer);

        if constexpr (!std::is_same_v<NoFilter, Filter>)
        {
            self->Notify_(self->FilterOnGet_(value));
        }
        else
        {
            self->Notify_(value);
        }
    }

    Upstream upstream_;
    std::optional<Filter> filter_;
};


template<typename Observer, typename Pex, typename Access = GetAndSetTag>
using Value = Value_<Observer, Pex, NoFilter, Access>;


template
<
    typename Observer,
    typename Pex,
    typename Filter,
    typename Access = GetAndSetTag
>
using FilteredValue = Value_<Observer, Pex, Filter, Access>;


template<typename Observer, typename Value>
struct ChangeObserver_;

template
<
    typename Observer,
    template<typename, typename...> typename Value,
    typename OtherObserver,
    typename... Others
>
struct ChangeObserver_<Observer, Value<OtherObserver, Others...>>
{
    using Type = Value<Observer, Others...>;
};

template
<
    typename Observer,
    typename Value
>
using ChangeObserver = typename ChangeObserver_<Observer, Value>::Type;


template<typename ControlValue, typename NewAccess>
struct ChangeAccess_
{
    static_assert(
        pex::HasAccess<NewAccess, typename ControlValue::Access>,
        "ChangeAccess can only remove existing access.");

    using Type = Value_
        <
            typename ControlValue::Observer,
            typename ControlValue::Pex,
            typename ControlValue::Filter,
            NewAccess
        >;
};

template<typename ControlValue, typename NewAccess>
using ChangeAccess = typename ChangeAccess_<ControlValue, NewAccess>::Type;



template<typename Observer>
struct BoundFilteredValue
{
    template<typename Pex, typename Filter>
    using Type = FilteredValue<Observer, Pex, Filter>;
};


template<typename Observer>
struct BoundValue
{
    template<typename Pex>
    using Type = Value_<Observer, Pex>;
};


template<typename ControlValue, typename Filter>
struct FilteredLike_
{
    static_assert(
        std::is_same_v<typename ControlValue::Filter, NoFilter>,
        "ControlValue has a filter that must not be replaced.");

    using Type = Value_
        <
            typename ControlValue::Observer,
            typename ControlValue::Pex,
            Filter,
            typename ControlValue::Access
        >;
};

template<typename ControlValue, typename Filter>
using FilteredLike = typename FilteredLike_<ControlValue, Filter>::Type;


} // namespace control


template<typename ...T>
struct IsControlBase_: std::false_type {};

template<typename ...T>
struct IsControlBase_<pex::control::Value_<T...>>: std::true_type {};

template<typename ...T>
inline constexpr bool IsControlBase = IsControlBase_<T...>::value;


template<typename T>
struct IsControl_: IsBaseOf<pex::control::Value_, T> {};

template<typename T>
inline constexpr bool IsControl = IsControl_<T>::value;


} // namespace pex
