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


#include "pex/model_value.h"


namespace pex
{


namespace control
{


template<typename Pex, typename Enable = void>
struct Upstream_
{
    using Type = Pex;
};


template<typename Pex>
struct Upstream_
<
    Pex,
    std::enable_if_t<IsModel<Pex>::value>
>
{
    using Type = ::pex::model::Direct<Pex>;
};


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
            typename Upstream_<Pex_>::Type::Type,
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
    using Upstream = typename Upstream_<Pex_>::Type;
    using Filter = Filter_;
    using Access = Access_;

    using UpstreamType = typename Upstream::Type;
    using Type = typename detail::FilteredType<UpstreamType, Filter>::Type;

    // Make any template specialization of Value_ a 'friend' class.
    template <typename, typename, typename, typename>
    friend class ::pex::control::Value_;

    template<typename>
    friend class ::pex::Reference;

    static_assert(
        detail::FilterIsNoneOrValid<UpstreamType, Filter, Access>::value);

    Value_(): upstream_(), filter_() {}

    explicit Value_(Pex &pex)
        :
        upstream_(pex),
        filter_()
    {
        if constexpr (HasAccess<GetTag, Access>)
        {
            this->upstream_.Connect(this, &Value_::OnUpstreamChanged_);
        }
    }

    explicit Value_(Pex &pex, Filter filter)
        :
        upstream_(pex),
        filter_(filter)
    {
        static_assert(
            detail::FilterIsMember<UpstreamType, Filter>::value,
            "A void or static filter cannot be set.");

        if constexpr (HasAccess<GetTag, Access>)
        {
            this->upstream_.Connect(this, &Value_::OnUpstreamChanged_);
        }
    }

    ~Value_()
    {
        this->upstream_.Disconnect(this);
    }

    /** Allow copy and assignment from another Value that may have different
     ** observers and filters, but tracks the same model.
     **/
    template<typename OtherObserver, typename OtherFilter>
    explicit Value_(
        const Value_<OtherObserver, Pex, OtherFilter, Access> &other)
        :
        upstream_(other.upstream_),
        filter_()
    {
        if constexpr (HasAccess<GetTag, Access>)
        {
            this->upstream_.Connect(this, &Value_::OnUpstreamChanged_);
        }

        if constexpr (std::is_same_v<OtherFilter, Filter>)
        {
            // The filter type matches.
            // Copy the filter, too.
            this->filter_ = other.filter_;
        }
    }

    template<typename OtherObserver, typename OtherFilter>
    Value_ & operator=(
        const Value_<OtherObserver, Pex, OtherFilter, Access> &other)
    {
        this->upstream_.Disconnect(this);
        this->upstream_ = other.upstream_;

        if constexpr (std::is_same_v<OtherFilter, Filter>)
        {
            // The filter type matches.
            // Copy the filter, too.
            this->filter_ = other.filter_;
        }

        if constexpr (HasAccess<GetTag, Access>)
        {
            this->upstream_.Connect(this, &Value_::OnUpstreamChanged_);
        }

        return *this;
    }

    Value_(const Value_ &other)
        :
        upstream_(other.upstream_),
        filter_(other.filter_)
    {
        if constexpr (HasAccess<GetTag, Access>)
        {
            this->upstream_.Connect(this, &Value_::OnUpstreamChanged_);
        }
    }

    Value_ & operator=(const Value_ &other)
    {
        this->upstream_.Disconnect(this);
        this->upstream_ = other.upstream_;
        this->filter_ = other.filter_;

        if constexpr (HasAccess<GetTag, Access>)
        {
            this->upstream_.Connect(
                this,
                &Value_::OnUpstreamChanged_);
        }

        return *this;
    }

    void SetFilter(Filter filter)
    {
        static_assert(
            detail::FilterIsMember<UpstreamType, Filter>::value,
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
        else if constexpr (detail::SetterIsMember<UpstreamType, Filter>::value)
        {
            REQUIRE_HAS_VALUE(this->filter_);
            return this->filter_->Set(value);
        }
        else
        {
            // The filter is not a member method.
            return Filter::Set(value);
        }
    }

    Type FilterOnGet_(ArgumentT<UpstreamType> value) const
    {
        if constexpr (std::is_same_v<NoFilter, Filter>)
        {
            return value;
        }
        else if constexpr (detail::GetterIsMember<UpstreamType, Filter>::value)
        {
            REQUIRE_HAS_VALUE(this->filter_);
            return this->filter_->Get(value);
        }
        else
        {
            // The filter doesn't accept a Filter * argument.
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
using FilteredLike = Value_
<
    typename ControlValue::Observer,
    typename ControlValue::Pex,
    Filter,
    typename ControlValue::Access
>;


} // namespace control


} // namespace pex
