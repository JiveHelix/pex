/**
  * @file model_value.h
  *
  * @brief Implements model Value nodes.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 22 Jul 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <string>
#include <stdexcept>
#include <jive/optional.h>
#include "pex/detail/log.h"
#include "pex/no_filter.h"
#include "pex/detail/notify_one.h"
#include "pex/detail/notify_many.h"
#include "pex/detail/value_connection.h"
#include "pex/detail/filters.h"
#include "pex/detail/value.h"
#include "pex/access_tag.h"
#include "pex/transaction.h"
#include "pex/detail/require_has_value.h"


namespace pex
{


template<typename>
class Reference;

template<typename>
class ConstReference;


// TODO: This has the same name as ::pex::detail::ValueConnection, which can
// require explicit namespaces to find it.
template<typename Observer, typename T, typename Filter>
using ValueConnection =
    detail::ValueConnection
    <
        Observer,
        detail::FilteredType<T, Filter>
    >;

namespace model
{

// Model must use unbound callbacks so it can send notifications to
// different observer types.
// All observers are stored as void *.
template<typename T, typename Filter_, typename Access_ = GetAndSetTag>
class Value_
    :
    //Callback values will be the type returned by the Filter, or T if
    // the filter is void.
    public detail::NotifyMany<ValueConnection<void, T, Filter_>, Access_>
{
    static_assert(!std::is_void_v<T>);
    static_assert(detail::FilterIsNoneOrValid<T, Filter_, SetTag>);

public:
    using Type = T;
    using Plain = Type;
    using Filter = Filter_;
    using Callable = typename ValueConnection<void, T, Filter>::Callable;

    // All model nodes have writable access.
    using Access = Access_;

    template<typename>
    friend class ::pex::Transaction;

    template<typename>
    friend class ::pex::Reference;

    template<typename>
    friend class ::pex::ConstReference;

    template<typename>
    friend class Direct;

    Value_()
        :
        filter_{},
        value_{this->FilterOnSet_(Type{})}
    {
        PEX_LOG(this);
    }

    explicit Value_(Type value)
        :
        filter_{},
        value_{this->FilterOnSet_(value)}
    {
        PEX_LOG(this);
    }

    Value_(Type value, Filter filter)
        :
        filter_{filter},
        value_{this->FilterOnSet_(value)}
    {
        PEX_LOG(this);
    }

    Value_(Filter filter)
        :
        filter_{filter},
        value_{this->FilterOnSet_(Type{})}
    {
        PEX_LOG(this);
    }

    Value_(const Value_<Type, Filter> &) = delete;
    Value_(Value_<Type, Filter> &&) = delete;

    ~Value_()
    {
#ifdef ENABLE_PEX_LOG
        if (!this->connections_.empty())
        {
            for (auto &connection: this->connections_)
            {
                PEX_LOG(
                    "Warning: ",
                    LookupPexName(connection.GetObserver()),
                    " is still connected to Model ",
                    LookupPexName(this));
            }
        }
#endif
    }

    /** Set the value and notify interfaces **/
    void Set(Argument<Type> value) requires (HasAccess<SetTag, Access>)
    {
        this->SetWithoutNotify_(value);
        this->DoNotify_();
    }

    Type Get() const
    {
        return this->value_;
    }

    explicit operator Type () const
    {
        return this->value_;
    }

    Value_ & operator=(Argument<Type> value)
        requires (HasAccess<SetTag, Access>)
    {
        this->Set(value);
        return *this;
    }

    void SetFilter(Filter filter)
    {
        this->filter_ = filter;
    }

    Filter & GetFilter()
    {
        return this->filter_;
    }

    const Filter & GetFilter() const
    {
        return this->filter_;
    }

    // This function is used in debug assertions to check that other entities
    // hold a reference to a model value.
    bool HasModel() const { return true; }

protected:
    void SetWithoutNotify_(Argument<Type> value)
    {
        if constexpr (std::is_same_v<NoFilter, Filter>)
        {
            this->value_ = value;
        }
        else
        {
            this->value_ = this->FilterOnSet_(value);
        }
    }

    void DoNotify_()
    {
        this->Notify_(this->value_);
    }

    Type FilterOnSet_(Argument<Type> value) const
    {
        if constexpr (std::is_same_v<NoFilter, Filter>)
        {
            return value;
        }
        else if constexpr (detail::SetterIsMember<Type, Filter>)
        {
            if constexpr (jive::IsOptional<Type>)
            {
                if (!value)
                {
                    return {};
                }

                return this->filter_.Set(*value);
            }
            else
            {
                return this->filter_.Set(value);
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

    Type FilterOnGet_(Argument<Type> value) const
    {
        if constexpr (std::is_same_v<NoFilter, Filter>)
        {
            return value;
        }
        else if constexpr (detail::GetterIsMember<Type, Filter>)
        {
            if constexpr (jive::IsOptional<Type>)
            {
                if (!value)
                {
                    return {};
                }

                return this->filter_.Get(*value);
            }
            else
            {
                return this->filter_.Get(value);
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

                return Filter::Get(*value);
            }
            else
            {
                return Filter::Get(value);
            }
        }
    }

    Filter filter_;
    Type value_;
};


template<typename T>
using Value = Value_<T, NoFilter>;

template<typename T, typename Filter>
using FilteredValue = Value_<T, Filter>;


template<typename T, typename Filter_>
class LockedValue: public Value_<T, Filter_>
{
public:
    template<typename>
    friend class ::pex::Transaction;

    template<typename>
    friend class ::pex::Reference;

    template<typename>
    friend class ::pex::ConstReference;

    template<typename>
    friend class Direct;

    using Base = Value_<T, Filter_>;
    using Base::Base;

    using Type = typename Base::Type;
    using Filter = typename Base::Filter;

    LockedValue()
        :
        Base(),
        mutex_()
    {

    }

    explicit LockedValue(Type value)
        :
        Base(value),
        mutex_()
    {

    }

    LockedValue(Type value, Filter filter)
        :
        Base(value, filter),
        mutex_()
    {

    }

    LockedValue(Filter_ filter)
        :
        Base(filter),
        mutex_()
    {

    }

    Type Get() const
    {
        std::lock_guard lock(this->mutex_);
        return this->value_;
    }

    explicit operator Type () const
    {
        std::lock_guard lock(this->mutex_);
        return this->value_;
    }

protected:
    void SetWithoutNotify_(Argument<Type> value)
    {
        if constexpr (std::is_same_v<NoFilter, Filter_>)
        {
            std::lock_guard lock(this->mutex_);
            this->value_ = value;
        }
        else
        {
            auto filteredValue = this->FilterOnSet_(value);

            std::lock_guard lock(this->mutex_);
            this->value_ = filteredValue;
        }
    }

private:
    mutable std::mutex mutex_;
};


} // namespace model


namespace control
{


template<typename, typename, typename> class Value_;


} // end namespace control


namespace model
{


template<typename Model>
class Direct
{
public:
    using Type = typename Model::Type;
    using Callable = typename Model::Callable;
    static constexpr bool isPexCopyable = true;

    Direct()
        :
        model_(nullptr)
    {

    }

    Direct(Model &model)
        :
        model_(&model)
    {

    }

    Direct(const Direct &other)
        :
        model_(other.model_)
    {

    }

    Direct & operator=(const Direct &other)
    {
        this->model_ = other.model_;

        return *this;
    }

    Type Get() const
    {
        REQUIRE_HAS_VALUE(this->model_);
        return this->model_->Get();
    }

    void Set(Argument<Type> value)
    {
        static_assert(HasAccess<SetTag, typename Model::Access>);

        REQUIRE_HAS_VALUE(this->model_);
        this->model_->Set(value);
    }

    void Connect(void *observer, Callable callable)
    {
        if (this->model_)
        {
            PEX_LOG(
                "Connect ",
                LookupPexName(observer),
                " to ",
                LookupPexName(this->model_));

            this->model_->Connect(observer, callable);
        }
    }

    void ConnectOnce(void *observer, Callable callable)
    {
        if (this->model_)
        {
            PEX_LOG(
                "Connect ",
                LookupPexName(observer),
                " to ",
                LookupPexName(this->model_));

            this->model_->ConnectOnce(observer, callable);
        }
    }

    void Disconnect(void *observer)
    {
        if (this->model_)
        {
            PEX_LOG("Disconnect observer: ", LookupPexName(observer));
            this->model_->Disconnect(observer);
        }
    }

    bool HasModel() const
    {
        return (this->model_ != nullptr);
    }

    template<typename, typename, typename>
    friend class ::pex::control::Value_;

    template<typename>
    friend class ::pex::Reference;

private:
    void SetWithoutNotify_(Argument<Type> value)
    {
        this->model_->SetWithoutNotify_(value);
    }

    void DoNotify_()
    {
        this->model_->DoNotify_();
    }

    using Model_ = Model;

    const Model_ & GetModel_() const
    {
        if (!this->HasModel())
        {
            throw std::logic_error("Model is not set");
        }

        return *this->model_;
    }

private:
    Model *model_;
};


extern template class Value_<bool, NoFilter>;

extern template class Value_<int8_t, NoFilter>;
extern template class Value_<int16_t, NoFilter>;
extern template class Value_<int32_t, NoFilter>;
extern template class Value_<int64_t, NoFilter>;

extern template class Value_<uint8_t, NoFilter>;
extern template class Value_<uint16_t, NoFilter>;
extern template class Value_<uint32_t, NoFilter>;
extern template class Value_<uint64_t, NoFilter>;

extern template class Value_<float, NoFilter>;
extern template class Value_<double, NoFilter>;

extern template class Value_<std::string, NoFilter>;


} // end namespace model


} // namespace pex
