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


namespace pex
{


namespace control
{


/** 
 ** Copyable Upstream may be stored directly, else use Direct.
 **/
template<typename Pex, typename Enable = void>
struct Upstream_
{
    using Type = Pex;
};


template<typename Pex>
struct Upstream_
<
    Pex,
    std::enable_if_t<!IsCopyable<Pex>>
>
{
    using Type = ::pex::model::Direct<Pex>;
};


template<typename Pex>
using UpstreamT = typename Upstream_<Pex>::Type;


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

    static_assert(!std::is_same_v<Filter, void>, "Filter must not be void.");

    static_assert(
        detail::FilterIsNoneOrValid<UpstreamType, Filter, Access>);

    Value_(): upstream_(), filter_() {}

    explicit Value_(Pex &pex)
        :
        upstream_(pex),
        filter_()
    {
        if constexpr (HasAccess<GetTag, Access>)
        {
            PEX_LOG("Connect");
            this->upstream_.Connect(this, &Value_::OnUpstreamChanged_);
        }
    }

    explicit Value_(Pex &pex, Filter filter)
        :
        upstream_(pex),
        filter_(filter)
    {
        static_assert(
            detail::FilterIsMember<UpstreamType, Filter>,
            "A void or static filter cannot be set.");

        if constexpr (HasAccess<GetTag, Access>)
        {
            PEX_LOG("Connect");
            this->upstream_.Connect(this, &Value_::OnUpstreamChanged_);
        }
    }

    ~Value_()
    {
        PEX_LOG("control::Value_::~Value_::Disconnect: ", this);
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

        static_assert(
            IsCopyable<Value_<OtherObserver, Pex, OtherFilter, OtherAccess>>,
            "Value is not copyable.");

        if constexpr (HasAccess<GetTag, Access>)
        {
            PEX_LOG("Connect");
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
        PEX_LOG("Disconnect");
        this->upstream_.Disconnect(this);
        this->upstream_ = other.upstream_;

        if constexpr (HasAccess<GetTag, Access>)
        {
            PEX_LOG("Connect");
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
            PEX_LOG("Connect");
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
            PEX_LOG("Connect");
            this->upstream_.Connect(this, &Value_::OnUpstreamChanged_);
        }
    }

    Value_ & operator=(const Value_ &other)
    {
        // TODO: Use SFINAE to disable copy constructor
        static_assert(
            IsCopyable<Value_>,
            "This value is not copyable.");

        PEX_LOG("Disconnect");
        this->upstream_.Disconnect(this);
        this->upstream_ = other.upstream_;
        
        if constexpr (HasAccess<GetTag, Access>)
        {
            PEX_LOG("Connect");
            this->upstream_.Connect(
                this,
                &Value_::OnUpstreamChanged_);
        }

        return *this;
    }

    Value_ & operator=(Value_ &&other)
    {
        PEX_LOG("Disconnect");
        this->upstream_.Disconnect(this);
        this->upstream_ = std::move(other.upstream_);
        
        if constexpr (HasAccess<GetTag, Access>)
        {
            PEX_LOG("Connect");
            this->upstream_.Connect(
                this,
                &Value_::OnUpstreamChanged_);
        }

        return *this;
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

    void Set(ArgumentT<Type> value)
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

    Value_ & operator=(ArgumentT<Type> value)
    {
        this->Set(value);
        return *this;
    }

private:
    void SetWithoutNotify_(ArgumentT<Type> value)
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
    
    UpstreamType FilterOnSet_(ArgumentT<Type> value) const
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

    Type FilterOnGet_(ArgumentT<UpstreamType> value) const
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
        ArgumentT<UpstreamType> value)
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
struct ChangeObserver;

template
<
    typename Observer,
    template<typename, typename...> typename Value,
    typename OtherObserver,
    typename... Others
>
struct ChangeObserver<Observer, Value<OtherObserver, Others...>>
{
    using Type = Value<Observer, Others...>;
};


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


} // namespace pex
