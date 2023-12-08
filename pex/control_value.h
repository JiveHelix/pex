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


template<typename>
class ConstControlReference;


namespace control
{


template<
    typename Upstream_,
    typename Filter_ = NoFilter,
    typename Access_ = GetAndSetTag
>
class Value_
    :
    public detail::NotifyMany
    <
        ValueConnection
        <
            void,
            typename UpstreamHolderT<Upstream_>::Type,
            Filter_
        >,
        Access_
    >
{
public:
    using Upstream = Upstream_;

    // This may not ultimately be true depending on the type of filter used.
    // See pex/traits.h for details.
    static constexpr bool isPexCopyable = true;

    // When Upstream_ is copyable, usually (always?) a control::Value_,
    // copy/move construct and copy/move assign must call ClearConnections
    // on this->upstream_ after the copy.
    // We may have copied connections made to upstream_ by other, and we no
    // longer need them.
    static constexpr bool mustClearConnections = IsCopyable<Upstream>;

    // If Upstream_ is a already a control::Value_, then UpstreamHolder is that
    // control::Value_, else it is Direct<Upstream>
    using UpstreamHolder = UpstreamHolderT<Upstream_>;

    using Filter = Filter_;
    using Access = Access_;
    using This = Value_<Upstream_, Filter, Access>;

    using UpstreamType = typename UpstreamHolder::Type;
    using Type = detail::FilteredType<UpstreamType, Filter>;
    using Plain = Type;

    using Connection = ValueConnection<void, UpstreamType, Filter>;
    using Base = detail::NotifyMany<Connection, Access>;

    using Callable = typename Connection::Callable;

    // Make any template specialization of Value_ a 'friend' class.
    template <typename, typename, typename>
    friend class ::pex::control::Value_;

    template<typename>
    friend class ::pex::Reference;

    template<typename>
    friend class ::pex::ConstControlReference;

    // A control::Value with a stateful filter cannot be copied, so it can be
    // managed by Direct when needed.
    template<typename>
    friend class ::pex::model::Direct;

    static_assert(!std::is_same_v<Filter, void>, "Filter must not be void.");

    static_assert(
        detail::FilterIsNoneOrValid<UpstreamType, Filter, Access>);

    Value_(): upstream_(), filter_()
    {

    }

    explicit Value_(PexArgument<Upstream> pex)
        :
        upstream_(pex),
        filter_()
    {
        if constexpr (mustClearConnections)
        {
            this->upstream_.ClearConnections();
        }
    }

    explicit Value_(PexArgument<Upstream> pex, Filter filter)
        :
        upstream_(pex),
        filter_(filter)
    {
        if constexpr (mustClearConnections)
        {
            this->upstream_.ClearConnections();
        }
    }

    Value_(void *observer, PexArgument<Upstream> pex, Callable callable)
        :
        upstream_(pex),
        filter_()
    {
        if constexpr (mustClearConnections)
        {
            this->upstream_.ClearConnections();
        }

        this->Connect(observer, callable);
    }

    Value_(void *observer, const Value_ &pex, Callable callable)
        :
        Value_(pex)
    {
        if constexpr (mustClearConnections)
        {
            this->upstream_.ClearConnections();
        }

        this->Connect(observer, callable);
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
     ** Allow copy and assignment from another Value that has a different
     ** filter, but tracks the same model.
     **/
    template<typename OtherFilter, typename OtherAccess>
    Value_(
        const Value_<Upstream, OtherFilter, OtherAccess> &other)
        :
        upstream_(other.upstream_),
        filter_()
    {
        // Allow the copy if the other has access at or above ours.
        static_assert(
            HasAccess<Access, OtherAccess>,
            "Cannot copy from another value without equal or greater access.");

        // You can replace a filter with another filter, but only if it uses
        // static functions.
        static_assert(
            IsCopyable
            <
                Value_<Upstream, OtherFilter, OtherAccess>
            >,
            "Value is not copyable.");

        if constexpr (mustClearConnections)
        {
            this->upstream_.ClearConnections();
        }

        using OtherType = detail::FilteredType<UpstreamType, OtherFilter>;

        if constexpr (
            HasAccess<GetTag, Access>
            && std::is_same_v<OtherType, Type>)
        {
            this->connections_ = other.connections_;

            if (this->HasConnections())
            {
                PEX_LOG(
                    "Copy from OtherFilter: ",
                    this,
                    " to ",
                    &this->upstream_);

                this->upstream_.ConnectOnce(this, &Value_::OnUpstreamChanged_);
            }
        }
    }

    // Enable the assignment operator if the other has access at or above ours.
    template<typename OtherFilter, typename OtherAccess>
    std::enable_if_t
    <
        HasAccess<Access, OtherAccess>
        && IsCopyable
        <
            Value_<Upstream, OtherFilter, OtherAccess>
        >,
        Value_ &
    >
    operator=(
        const Value_<Upstream, OtherFilter, OtherAccess> &other)
    {
        PEX_LOG("Disconnect ", this);
        this->upstream_.Disconnect(this);

        this->upstream_ = other.upstream_;

        if constexpr (mustClearConnections)
        {
            this->upstream_.ClearConnections();
        }

        using OtherType = detail::FilteredType<UpstreamType, OtherFilter>;

        if constexpr (
            HasAccess<GetTag, Access>
            && std::is_same_v<OtherType, Type>)
        {
            this->connections_ = other.connections_;

            if (this->HasConnections())
            {
                this->upstream_.ConnectOnce(this, &Value_::OnUpstreamChanged_);
            }
        }

        return *this;
    }


    /**
     ** Allow copy and assignment from another Value that has a different
     ** filter, but tracks the same model.
     **/
    template<typename OtherFilter, typename OtherAccess>
    Value_(Value_<Upstream, OtherFilter, OtherAccess> &&other)
        :
        upstream_(std::move(other.upstream_)),
        filter_()
    {
        // Allow the copy if the other has access at or above ours.
        static_assert(
            HasAccess<Access, OtherAccess>,
            "Cannot copy from another value without equal or greater access.");

        // You can replace a filter with another filter, but only if it uses
        // static functions.
        static_assert(
            IsCopyable
            <
                Value_<Upstream, OtherFilter, OtherAccess>
            >,
            "Value cannot be moved.");

        if constexpr (mustClearConnections)
        {
            this->upstream_.ClearConnections();
        }

        using OtherType = detail::FilteredType<UpstreamType, OtherFilter>;

        if constexpr (
            HasAccess<GetTag, Access>
            && std::is_same_v<OtherType, Type>)
        {
            this->connections_ = other.connections_;

            if (this->HasConnections())
            {
                this->upstream_.ConnectOnce(this, &Value_::OnUpstreamChanged_);
            }
        }
    }

    // Enable the assignment operator if the other has access at or above ours.
    template<typename OtherFilter, typename OtherAccess>
    std::enable_if_t
    <
        HasAccess<Access, OtherAccess>
        && IsCopyable
        <
            Value_<Upstream, OtherFilter, OtherAccess>
        >,
        Value_ &
    >
    operator=(Value_<Upstream, OtherFilter, OtherAccess> &&other)
    {
        PEX_LOG("Disconnect ", this);
        this->upstream_.Disconnect(this);

        this->upstream_ = std::move(other.upstream_);

        using OtherType = detail::FilteredType<UpstreamType, OtherFilter>;

        if constexpr (mustClearConnections)
        {
            this->upstream_.ClearConnections();
        }

        if constexpr (
            HasAccess<GetTag, Access>
            && std::is_same_v<OtherType, Type>)
        {
            this->connections_ = other.connections_;

            if (this->HasConnections())
            {
                this->upstream_.ConnectOnce(this, &Value_::OnUpstreamChanged_);
            }
        }

        return *this;
    }


    Value_(const Value_ &other)
        :
        Base(other),
        upstream_(other.upstream_),
        filter_()
    {
        static_assert(IsCopyable<Value_>, "This value is not copyable.");

        if constexpr (mustClearConnections)
        {
            this->upstream_.ClearConnections();
        }

        if constexpr (HasAccess<GetTag, Access>)
        {
            if (this->HasConnections())
            {
                PEX_LOG("Copy from other: ", this, " to ", &this->upstream_);
                this->upstream_.ConnectOnce(this, &Value_::OnUpstreamChanged_);
            }
        }
    }

    Value_(Value_ &&other)
        :
        Base(std::move(other)),
        upstream_(std::move(other.upstream_)),
        filter_(std::move(other.filter_))
    {
        // Yes, this is a move constructor.
        // A control::Value with member-function filter is neither copyable nor
        // move-able.
        static_assert(IsCopyable<Value_>, "This value cannot be moved.");

        if constexpr (mustClearConnections)
        {
            this->upstream_.ClearConnections();
        }

        if constexpr (HasAccess<GetTag, Access>)
        {
            if (this->HasConnections())
            {
                PEX_LOG("Connect ", this);
                this->upstream_.ConnectOnce(this, &Value_::OnUpstreamChanged_);
            }
        }
    }

    Value_ & operator=(const Value_ &other)
    {
        static_assert(IsCopyable<Value_>, "This value is not copyable.");

        // Sanity check
        static_assert(
            !detail::FilterIsMember<UpstreamType, Filter>,
            "IsCopyable implies that Filter uses static functions.");

        this->Base::operator=(other);

        PEX_LOG("Disconnect ", this);
        this->upstream_.Disconnect(this);

        this->upstream_ = other.upstream_;

        if constexpr (mustClearConnections)
        {
            this->upstream_.ClearConnections();
        }

        if constexpr (HasAccess<GetTag, Access>)
        {
            if (this->HasConnections())
            {
                PEX_LOG("Connect ", this);

                this->upstream_.ConnectOnce(
                    this,
                    &Value_::OnUpstreamChanged_);
            }
        }

        return *this;
    }

    Value_ & operator=(Value_ &&other)
    {
        static_assert(IsCopyable<Value_>, "This value cannot be moved.");

        this->Base::operator=(std::move(other));

        PEX_LOG("control::Value_ move assign: Disconnect ", this);
        this->upstream_.Disconnect(this);

        this->upstream_ = std::move(other.upstream_);
        this->filter_ = std::move(other.filter_);

        if constexpr (mustClearConnections)
        {
            this->upstream_.ClearConnections();
        }

        if constexpr (HasAccess<GetTag, Access>)
        {
            if (this->HasConnections())
            {
                PEX_LOG("Connect ", this);

                this->upstream_.ConnectOnce(
                    this,
                    &Value_::OnUpstreamChanged_);
            }
        }

        return *this;
    }

    void Connect(void *observer, Callable callable)
    {
        static_assert(HasAccess<GetTag, Access>);

        if (!this->HasConnections())
        {
            // This is the first request for a connection.
            // Connect ourselves to the upstream.
            PEX_LOG("Connect ", this);

            this->upstream_.ConnectOnce(
                this,
                &Value_::OnUpstreamChanged_);
        }

        this->Base::Connect(observer, callable);
    }

    void Disconnect(void *observer)
    {
        this->Base::Disconnect(observer);

        if (!this->HasConnections())
        {
            // The last connection has been disconnected.
            // Remove ourselves from the upstream.
            this->upstream_.Disconnect(this);
        }
    }

    using Downstream = Value_<This, NoFilter, Access>;

    Downstream GetDownstream()
    {
        return Downstream(*this);
    }

    void SetFilter(Filter filter)
    {
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

    void ClearConnections()
    {
        this->ClearConnections_();
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

    using Model_ = typename UpstreamHolder::Model_;

    const Model_ & GetModel_() const
    {
        return this->upstream_.GetModel_();
    }

    UpstreamHolder upstream_;
    std::optional<Filter> filter_;
};


template<typename Upstream, typename Access = GetAndSetTag>
using Value = Value_<Upstream, NoFilter, Access>;


template
<
    typename Upstream,
    typename Filter,
    typename Access = GetAndSetTag
>
using FilteredValue = Value_<Upstream, Filter, Access>;


template<typename ControlValue, typename NewAccess>
struct ChangeAccess_
{
    static_assert(
        pex::HasAccess<NewAccess, typename ControlValue::Access>,
        "ChangeAccess can only remove existing access.");

    using Type = Value_
        <
            typename ControlValue::Upstream,
            typename ControlValue::Filter,
            NewAccess
        >;
};

template<typename ControlValue, typename NewAccess>
using ChangeAccess = typename ChangeAccess_<ControlValue, NewAccess>::Type;


template<typename ControlValue, typename Filter>
struct FilteredLike_
{
    static_assert(
        std::is_same_v<typename ControlValue::Filter, NoFilter>,
        "ControlValue has a filter that must not be replaced.");

    using Type = Value_
        <
            typename ControlValue::Upstream,
            Filter,
            typename ControlValue::Access
        >;
};

template<typename ControlValue, typename Filter>
using FilteredLike = typename FilteredLike_<ControlValue, Filter>::Type;


extern template class Value_<model::Value_<bool, NoFilter>>;

extern template class Value_<model::Value_<int8_t, NoFilter>>;
extern template class Value_<model::Value_<int16_t, NoFilter>>;
extern template class Value_<model::Value_<int32_t, NoFilter>>;
extern template class Value_<model::Value_<int64_t, NoFilter>>;

extern template class Value_<model::Value_<uint8_t, NoFilter>>;
extern template class Value_<model::Value_<uint16_t, NoFilter>>;
extern template class Value_<model::Value_<uint32_t, NoFilter>>;
extern template class Value_<model::Value_<uint64_t, NoFilter>>;

extern template class Value_<model::Value_<float, NoFilter>>;
extern template class Value_<model::Value_<double, NoFilter>>;

extern template class Value_<model::Value_<std::string, NoFilter>>;


} // namespace control


template<typename ...T>
struct IsControlBase_: std::false_type {};

template<typename ...T>
struct IsControlBase_<pex::control::Value_<T...>>: std::true_type {};

template<typename ...T>
inline constexpr bool IsControlBase = IsControlBase_<T...>::value;


template<typename T, typename = void>
struct IsControl_: std::false_type {};

template<typename T>
struct IsControl_
<
    T,
    std::enable_if_t<IsBaseOf<pex::control::Value_, T>::value>
>: std::true_type {};

template<typename T>
inline constexpr bool IsControl = IsControl_<T>::value;


} // namespace pex
