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


#include <jive/optional.h>
#include "pex/model_value.h"
#include "pex/traits.h"


namespace pex
{


template<typename>
class ConstControlReference;

template<typename, typename>
class Terminus;


namespace control
{


template
<
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
    >,
    Separator
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
    static constexpr bool upstreamIsCopyable = IsCopyable<Upstream>;

    // If Upstream_ is a already a control::Value_, then UpstreamHolder is that
    // control::Value_, else it is Direct<Upstream>
    using UpstreamHolder = UpstreamHolderT<Upstream_>;

    using Filter = Filter_;
    using Access = Access_;
    using This = Value_<Upstream_, Filter, Access>;

    using UpstreamType = typename UpstreamHolder::Type; using Type = detail::FilteredType<UpstreamType, Filter>;
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

    template<typename, typename>
    friend class ::pex::Terminus;

    static_assert(!std::is_same_v<Filter, void>, "Filter must not be void.");

    static_assert(
        detail::FilterIsNoneOrValid<UpstreamType, Filter, Access>);

    using Model = typename UpstreamHolder::Model;

    class UpstreamConnection
    {
        UpstreamHolder &upstream_;
        Value_ *observer_;

    public:
        using FunctionPointer = void (*)(void *, Argument<UpstreamType>);

        UpstreamConnection(
            UpstreamHolder &upstream,
            Value_ *observer,
            FunctionPointer callable)
            :
            upstream_(upstream),
            observer_(observer)
        {
            this->upstream_.ConnectOnce(observer, callable);
        }

        ~UpstreamConnection()
        {
            PEX_LOG(
                "control::Value_ Disconnect: ",
                LookupPexName(this->observer_),
                " from ",
                LookupPexName(&this->upstream_));

            this->upstream_.Disconnect(this->observer_);
        }
    };

    Value_(): Base(), upstream_(), filter_()
    {
        PEX_NAME_UNIQUE("pex::control::Value");
    }

    explicit Value_(PexArgument<Upstream> pex)
        :
        Base(),
        upstream_(pex),
        filter_()
    {
        PEX_NAME_UNIQUE("pex::control::Value");

        if constexpr (upstreamIsCopyable)
        {
            this->upstream_.ClearConnections();
        }

        if constexpr (detail::FilterIsMember<UpstreamType, Filter>)
        {
            this->filter_ = Filter{};
        }
    }

    explicit Value_(PexArgument<Upstream> pex, const Filter &filter)
        :
        Base(),
        upstream_(pex),
        filter_(filter)
    {
        PEX_NAME_UNIQUE("pex::control::Value");

        if constexpr (upstreamIsCopyable)
        {
            this->upstream_.ClearConnections();
        }
    }

    Value_(void *observer, PexArgument<Upstream> pex, Callable callable)
        :
        Base(),
        upstream_(pex),
        filter_()
    {
        PEX_NAME_UNIQUE("pex::control::Value");

        if constexpr (upstreamIsCopyable)
        {
            this->upstream_.ClearConnections();
        }

        this->Connect(observer, callable);

        if constexpr (detail::FilterIsMember<UpstreamType, Filter>)
        {
            this->filter_ = Filter{};
        }
    }

    Value_(void *observer, const Value_ &pex, Callable callable)
        :
        Value_(pex)
    {
        if constexpr (upstreamIsCopyable)
        {
            this->upstream_.ClearConnections();
        }

        this->Connect(observer, callable);
    }

    ~Value_()
    {
        PEX_CLEAR_NAME(this);
    }

    /**
     ** Allow copy and assignment from another Value that has a different
     ** filter, but tracks the same model.
     **/
    template<typename OtherFilter, typename OtherAccess>
    Value_(
        const Value_<Upstream, OtherFilter, OtherAccess> &other)
        :
        Base(),
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

        PEX_NAME_UNIQUE("pex::control::Value");

        if constexpr (upstreamIsCopyable)
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

                this->upstreamConnection_.emplace(
                    this->upstream_,
                    this,
                    &Value_::OnUpstreamChanged_);
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
        this->Base::operator=(other);

        this->upstreamConnection_.reset();

        this->upstream_ = other.upstream_;

        if constexpr (upstreamIsCopyable)
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
                this->upstreamConnection_.emplace(
                    this->upstream_,
                    this,
                    &Value_::OnUpstreamChanged_);
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

        PEX_NAME_UNIQUE("pex::control::Value");

        if constexpr (upstreamIsCopyable)
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
                this->upstreamConnection_.emplace(
                    this->upstream_,
                    this,
                    &Value_::OnUpstreamChanged_);
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
        this->Base::operator=(std::move(other));
        this->upstreamConnection_.reset();

        this->upstream_ = std::move(other.upstream_);

        using OtherType = detail::FilteredType<UpstreamType, OtherFilter>;

        if constexpr (upstreamIsCopyable)
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
                this->upstreamConnection_.emplace(
                    this->upstream_,
                    this,
                    &Value_::OnUpstreamChanged_);
            }
        }

        return *this;
    }


    Value_(const Value_ &other)
        :
        Base(other),
        upstream_(other.upstream_),
        filter_(other.filter_)
    {
        static_assert(IsCopyable<Value_>, "This value is not copyable.");

        PEX_NAME_UNIQUE("pex::control::Value");

        if constexpr (upstreamIsCopyable)
        {
            this->upstream_.ClearConnections();
        }

        if constexpr (HasAccess<GetTag, Access>)
        {
            if (this->HasConnections())
            {
                PEX_LOG("Copy from other: ", this, " to ", &this->upstream_);

                this->upstreamConnection_.emplace(
                    this->upstream_,
                    this,
                    &Value_::OnUpstreamChanged_);
            }
        }
    }

    Value_(Value_ &&other) noexcept
        :
        Base(std::move(other)),
        upstream_(std::move(other.upstream_)),
        filter_(std::move(other.filter_))
    {
        // TODO: I don't think moving a member function filter is a problem.
        // A control::Value with member-function filter is neither copyable nor
        // move-able.
        // static_assert(IsCopyable<Value_>, "This value cannot be moved.");

        PEX_NAME_UNIQUE("pex::control::Value");

        if constexpr (upstreamIsCopyable)
        {
            this->upstream_.ClearConnections();
        }

        if constexpr (HasAccess<GetTag, Access>)
        {
            if (this->HasConnections())
            {
                PEX_LOG("Connect ", this);

                this->upstreamConnection_.emplace(
                    this->upstream_,
                    this,
                    &Value_::OnUpstreamChanged_);
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

        this->upstreamConnection_.reset();

        this->upstream_ = other.upstream_;

        if constexpr (upstreamIsCopyable)
        {
            this->upstream_.ClearConnections();
        }

        if constexpr (HasAccess<GetTag, Access>)
        {
            if (this->HasConnections())
            {
                PEX_LOG("Connect ", this);

                this->upstreamConnection_.emplace(
                    this->upstream_,
                    this,
                    &Value_::OnUpstreamChanged_);
            }
        }

        return *this;
    }

    Value_ & operator=(Value_ &&other)
    {
        this->Base::operator=(std::move(other));

        PEX_LOG("control::Value_ move assign: Disconnect ", this);
        this->upstreamConnection_.reset();

        this->upstream_ = std::move(other.upstream_);
        this->filter_ = std::move(other.filter_);

        if constexpr (upstreamIsCopyable)
        {
            this->upstream_.ClearConnections();
        }

        if constexpr (HasAccess<GetTag, Access>)
        {
            if (this->HasConnections())
            {
                PEX_LOG("Connect ", this);

                this->upstreamConnection_.emplace(
                    this->upstream_,
                    this,
                    &Value_::OnUpstreamChanged_);
            }
        }

        return *this;
    }

    void Connect(void *observer, Callable callable)
    {
        static_assert(HasAccess<GetTag, Access>);

        if (!this->upstreamConnection_)
        {
            // This is the first request for a connection.
            // Connect ourselves to the upstream.
            PEX_LOG("Connect ", this);

            this->upstreamConnection_.emplace(
                this->upstream_,
                this,
                &Value_::OnUpstreamChanged_);
        }

        this->Base::Connect(observer, callable);
    }

    void ConnectOnce(void *observer, Callable callable)
    {
        static_assert(HasAccess<GetTag, Access>);

        if (!this->upstreamConnection_)
        {
            // This is the first request for a connection.
            // Connect ourselves to the upstream.

            this->upstreamConnection_.emplace(
                this->upstream_,
                this,
                &Value_::OnUpstreamChanged_);
        }

        this->Base::ConnectOnce(observer, callable);
    }

    void Disconnect(void *observer)
    {
        this->Base::Disconnect(observer);

        if (!this->HasConnections())
        {
            // The last connection has been disconnected.
            // Remove ourselves from the upstream.
            this->upstreamConnection_.reset();
        }
    }

    std::vector<size_t> GetNotificationOrderChain(void *observer)
    {
        auto upstreamChain = this->upstream_.GetNotificationOrderChain(this);
        upstreamChain.push_back(this->GetNotificationOrder(observer));

        return upstreamChain;
    }

    void SetFilter(const Filter &filter)
    {
        this->filter_ = filter;
    }

    const Filter & GetFilter() const
    {
        if (!this->filter_)
        {
            throw std::logic_error("filter_ has not been set.");
        }

        return *this->filter_;
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
        this->upstreamConnection_.reset();
    }

    void Notify()
    {
        this->upstream_.Notify();
    }

protected:
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

    UpstreamType FilterOnSet_(Argument<Type> value) const
    {
        if constexpr (std::is_same_v<NoFilter, Filter>)
        {
            return value;
        }
        else if constexpr (detail::SetterIsMember<UpstreamType, Filter>)
        {
            REQUIRE_HAS_VALUE(this->filter_);

            if constexpr (jive::IsOptional<Type>)
            {
                if (!value)
                {
                    return {};
                }

                return this->filter_->Set(*value);
            }
            else
            {
                return this->filter_->Set(value);
            }
        }
        else
        {
            // The filter is not a member function.
            if constexpr (jive::IsOptional<Type>)
            {
                if (!value)
                {
                    return {};
                }

                return Filter::Set(*value);
            }
            else
            {
                return Filter::Set(value);
            }
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

            if constexpr (jive::IsOptional<Type>)
            {
                if (!value)
                {
                    return {};
                }

                return this->filter_->Get(*value);
            }
            else
            {
                return this->filter_->Get(value);
            }
        }
        else
        {
            // The filter is not a member function.

            if constexpr (jive::IsOptional<Type>)
            {
                if (!value)
                {
                    return value;
                }

                return Filter::Get(*value);
            }
            else
            {
                return Filter::Get(value);
            }
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

    void ChangeUpstream_(PexArgument<Upstream> upstream)
    {
        this->upstreamConnection_.reset();

        this->upstream_ = upstream;

        if (this->HasConnections())
        {
            this->upstreamConnection_.emplace(
                this->upstream_,
                this,
                &Value_::OnUpstreamChanged_);
        }
    }

public:
    const Model & GetModel_() const
    {
        return this->upstream_.GetModel_();
    }

    UpstreamHolder upstream_;
    std::optional<Filter> filter_;
    std::optional<UpstreamConnection> upstreamConnection_;
};


template<typename Upstream_>
class ValueContainer: public Value_<Upstream_>
{
public:
    static constexpr bool isValueContainer = true;

    using Base = Value_<Upstream_>;
    using Upstream = typename Base::Upstream;

    using Base::Base;
    using Base::Set;
    using Base::Get;

    using Type = typename Base::Type;
    using ValueType = typename Type::value_type;
    using Access = typename Base::Access;

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

    template<typename, typename>
    friend class ::pex::Terminus;

    /** Set the value and notify interfaces **/
    void Set(size_t index, Argument<ValueType> value)
        requires (HasAccess<SetTag, Access>)
    {
        this->SetWithoutNotify_(index, value);
        this->Notify();
    }

    Type Get(size_t index) const
    {
        return this->upstream_[index];
    }

    size_t size() const
    {
        return this->upstream_.size();
    }

    size_t empty() const
    {
        return this->upstream_.empty();
    }

    // There is no non-const operator[].
    // We must have a way to publish changed values.
    const ValueType & operator[](size_t index) const
    {
        return this->upstream_[index];
    }

    const ValueType & at(size_t index) const
    {
        return this->upstream_.at(index);
    }

protected:
    using Base::SetWithoutNotify_;

    void SetWithoutNotify_(size_t index, Argument<Type> value)
    {
        this->upstream_.SetWithoutNotify_(index, value);
    }
};


template<typename T>
class KeyValueContainer: public Value_<T, NoFilter>
{
public:
    static constexpr bool isKeyValueContainer = true;

    using Base = Value_<T, NoFilter>;

    using Base::Base;
    using Base::Set;
    using Base::Get;

    using Type = typename Base::Type;
    using MappedType = typename Type::mapped_type;
    using KeyType = typename Type::key_type;
    using Access = typename Base::Access;

    template<typename>
    friend class ::pex::Transaction;

    template<typename>
    friend class ::pex::Reference;

    template<typename>
    friend class ::pex::ConstReference;

    template<typename>
    friend class Direct;

    template<typename, typename>
    friend class Publisher;

    /** Set the value and notify interfaces **/
    void Set(const KeyType &key, Argument<MappedType> value)
        requires (HasAccess<SetTag, Access>)
    {
        this->SetWithoutNotify_(key, value);
        this->Notify();
    }

    Type Get(const KeyType &key) const
    {
        return this->upstream_[key];
    }

    size_t size() const
    {
        return this->upstream_.size();
    }

    size_t empty() const
    {
        return this->upstream_.empty();
    }

    size_t count(const KeyType &key) const
    {
        return this->upstream_.count(key);
    }

    const MappedType & at(const KeyType &key) const
    {
        return this->upstream_.at(key);
    }

protected:
    using Base::SetWithoutNotify_;

    void SetWithoutNotify_(const KeyType &key, Argument<MappedType> value)
    {
        this->upstream_[key] = value;
    }
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


template
<
    typename Upstream,
    typename Filter = NoFilter,
    typename Access = GetAndSetTag
>
class Mux: public Value_<Upstream, Filter, Access>
{
public:
    static constexpr bool isPexCopyable = false;
    using Base = Value_<Upstream, Filter, Access>;

    Mux()
        :
        Base{}
    {

    }

    Mux(const Mux<Upstream> &) = delete;
    Mux(Mux<Upstream> &&) = delete;
    Mux & operator=(const Mux &) = delete;
    Mux & operator=(Mux &&) = delete;

    void ChangeUpstream(PexArgument<Upstream> upstream)
    {
        this->ChangeUpstream_(upstream);
    }
};


template
<
    typename Upstream,
    typename Filter,
    typename Access = GetAndSetTag
>
using FilteredMux = Mux<Upstream, Filter, Access>;


template<typename Upstream>
class ValueContainerMux: public ValueContainer<Upstream>
{
public:
    static constexpr bool isPexCopyable = false;

    ValueContainerMux(const ValueContainerMux<Upstream> &) = delete;
    ValueContainerMux(ValueContainerMux<Upstream> &&) = delete;
    ValueContainerMux & operator=(const ValueContainerMux &) = delete;
    ValueContainerMux & operator=(ValueContainerMux &&) = delete;

    void ChangeUpstream(PexArgument<Upstream> upstream)
    {
        this->ChangeUpstream_(upstream);
    }
};


template<typename Upstream>
class KeyValueContainerMux: public KeyValueContainer<Upstream>
{
public:
    static constexpr bool isPexCopyable = false;

    KeyValueContainerMux(const KeyValueContainerMux<Upstream> &) = delete;
    KeyValueContainerMux(KeyValueContainerMux<Upstream> &&) = delete;
    KeyValueContainerMux & operator=(const KeyValueContainerMux &) = delete;
    KeyValueContainerMux & operator=(KeyValueContainerMux &&) = delete;

    void ChangeUpstream(PexArgument<Upstream> upstream)
    {
        this->ChangeUpstream_(upstream);
    }
};


} // namespace control


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
