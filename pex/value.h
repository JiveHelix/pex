/**
  * @file value.h
  *
  * @brief Implements model and interface Value nodes.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 22 Jul 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <type_traits>
#include "pex/access_tag.h"
#include "pex/reference.h"
#include "pex/transaction.h"
#include "pex/detail/value_detail.h"
#include "pex/detail/not_null.h"


namespace pex
{

template<typename Observer, typename T, typename Filter>
using Notification =
    detail::ValueNotify
    <
        Observer,
        typename detail::FilteredType<T, Filter>::Type
    >;

namespace model
{

// Model must use unbound callbacks so it can send notifications to
// different observer types.
// All observers are stored as void *.
template<typename T, typename Filter_>
class Value_
    :
    // Notification values will be the type returned by the Filter, or T if
    // the filter is void.
    public detail::NotifyMany<Notification<void, T, Filter_>>
{
    static_assert(detail::FilterIsVoidOrValid<T, Filter_, SetTag>::value);

public:
    using Type = T;
    using Filter = Filter_;
    using Notify = Notification<void, T, Filter>;

    // All model nodes have writable access.
    using Access = GetAndSetTag;

    template<typename Pex>
    friend class ::pex::Transaction;

    template<typename Pex>
    friend class ::pex::Reference;

    template<typename Pex>
    friend class ::pex::ConstReference;

    Value_()
        :
        filter_{},
        value_{}
    {

    }

    explicit Value_(T value)
        :
        filter_{},
        value_{this->FilterOnSet_(value)}
    {
        static_assert(
            detail::FilterIsVoidOrStatic<T, Filter, SetTag>::value,
            "A filter with member functions requires a pointer.");
    }

    Value_(T value, Filter *filter)
        :
        filter_{filter},
        value_{this->FilterOnSet_(value)}
    {
        static_assert(
            detail::FilterIsMember<T, Filter>::value,
            "Static or void Filter does not require a filter instance.");
    }

    Value_(Filter *filter)
        :
        filter_{filter},
        value_{}
    {
        static_assert(
            detail::FilterIsMember<T, Filter>::value,
            "Static or void Filter does not require a filter instance.");
    }

    Value_(const Value_<T, Filter> &) = delete;
    Value_(Value_<T, Filter> &&) = delete;

    /** Set the value and notify interfaces **/
    void Set(typename detail::Argument<T>::Type value)
    {
        this->SetWithoutNotify_(value);
        this->DoNotify_();
    }

    T Get() const
    {
        return this->value_;
    }

    void SetFilter(Filter *filter)
    {
        static_assert(
            detail::FilterIsMember<T, Filter>::value,
            "Static or void Filter does not require a filter instance.");

        NOT_NULL(filter);
        this->filter_ = filter;
    }

    /** If Filter requires a Filter instance, and it has been set, then bool
     ** conversion returns true.
     **
     ** If the Filter is void or static, it always returns true.
     **/
    explicit operator bool () const
    {
        if constexpr (detail::FilterIsMember<T, Filter>::value)
        {
            return this->filter_ != nullptr;
        }
        else
        {
            return true;
        }
    }

private:
    void SetWithoutNotify_(typename detail::Argument<T>::Type value)
    {
        if constexpr (std::is_void_v<Filter>)
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

    T FilterOnSet_(typename detail::Argument<T>::Type value) const
    {
        if constexpr (std::is_same_v<void, Filter>)
        {
            return value;
        }
        else if constexpr (detail::SetterIsMember<T, Filter>::value)
        {
            NOT_NULL(this->filter_);
            return this->filter_->Set(value);
        }
        else
        {
            // The filter is not a member method.
            return Filter::Set(value);
        }
    }

    T FilterOnGet_(
        typename detail::Argument<T>::Type value) const
    {
        if constexpr (std::is_same_v<void, Filter>)
        {
            return value;
        }
        else if constexpr (detail::GetterIsMember<T, Filter>::value)
        {
            NOT_NULL(this->filter_);
            return this->filter_->Get(value);
        }
        else
        {
            // The filter doesn't accept a Filter * argument.
            return Filter::Get(value);
        }
    }

    Filter * filter_;
    T value_;
};


template<typename T>
using Value = Value_<T, void>;

template<typename T, typename Filter>
using FilteredValue = Value_<T, Filter>;


} // namespace model


namespace interface
{


template<
    typename Observer_,
    typename Model_,
    typename Filter_ = void,
    typename Access_ = GetAndSetTag
>
class Value_
    :
    // Notification values will be the type returned by the Filter, or
    // Model_::Type if the filter is void.
    public detail::NotifyOne
    <
        Notification<Observer_, typename Model_::Type, Filter_>
    >
{
public:
    using Observer = Observer_;
    using Model = Model_;
    using Filter = Filter_;
    using Access = Access_;
    using ModelType = typename Model::Type;
    using Type = typename detail::FilteredType<ModelType, Filter>::Type;

    template
    <
        typename AnyObserver,
        typename AnyModel,
        typename AnyFilter,
        typename AnyAccess
    >
    friend class Value_;

    static_assert(
        detail::DefinesType<Model>::value,
        "Model must define Model::Type");

    static_assert(
        detail::FilterIsVoidOrValid<ModelType, Filter, Access>::value);

    Value_(): model_(nullptr), filter_(nullptr) {}

    explicit Value_(Model * model)
        :
        model_(model),
        filter_(nullptr)
    {
        static_assert(
            detail::FilterIsVoidOrStatic<ModelType, Filter, Access>::value,
            "A filter with member methods requires a pointer.");

        NOT_NULL(model);

        if constexpr (detail::ImplementsConnect<Model>::value)
        {
            this->model_->Connect(this, &Value_::OnModelChanged_);
        }
    }

    explicit Value_(Model * model, Filter *filter)
        :
        model_(model),
        filter_(filter)
    {
        static_assert(
            detail::FilterIsMember<ModelType, Filter>::value,
            "A void or static filter cannot use a pointer.");

        NOT_NULL(model);

        if constexpr (detail::ImplementsConnect<Model>::value)
        {
            this->model_->Connect(this, &Value_::OnModelChanged_);
        }
    }

    ~Value_()
    {
        if constexpr (detail::ImplementsDisconnect<Model>::value)
        {
            if (this->model_)
            {
                this->model_->Disconnect(this);
            }
        }
    }

    /** Allow copy and assignment from another Value that may have different
     ** observers and filters, but tracks the same model.
     **/
    template<typename OtherObserver, typename OtherFilter>
    explicit Value_(
        const Value_<OtherObserver, Model, OtherFilter, Access> &other)
        :
        model_(other.model_),
        filter_(nullptr)
    {
        if constexpr (detail::ImplementsConnect<Model>::value)
        {
            if (this->model_)
            {
                this->model_->Connect(this, &Value_::OnModelChanged_);
            }
        }
    }

    /** When the filter type matches, the filter can be copied, too. **/
    template<typename OtherObserver>
    explicit Value_(const Value_<OtherObserver, Model, Filter, Access> &other)
        :
        model_(other.model_),
        filter_(other.filter_)
    {
        if constexpr (detail::ImplementsConnect<Model>::value)
        {
            if (this->model_)
            {
                this->model_->Connect(this, &Value_::OnModelChanged_);
            }
        }
    }

    template
    <
        typename OtherObserver,
        typename OtherModel,
        typename OtherFilter,
        typename OtherAccess
    >
    Value_<Observer, Model, Filter, Access> & operator=(
        const Value_<
            OtherObserver,
            OtherModel,
            OtherFilter,
            OtherAccess> &other)
    {
        static_assert(
            std::is_same_v<Model, OtherModel>,
            "Copy may differ in observer and filter types, but must track the "
            "same model value.");

        static_assert(
            std::is_same_v<Access, OtherAccess>,
            "Copy may differ in observer and filter types, but must have the "
            "same access level.");

        if constexpr (detail::ImplementsDisconnect<Model>::value)
        {
            if (this->model_)
            {
                this->model_->Disconnect(this);
            }
        }

        this->model_ = other.model_;

        if constexpr (std::is_same_v<OtherFilter, Filter>)
        {
            this->filter_ = other.filter_;
        }

        if constexpr (detail::ImplementsConnect<Model>::value)
        {
            if (this->model_)
            {
                this->model_->Connect(
                    this,
                    &Value_<Observer, Model, Filter>::OnModelChanged_);
            }
        }

        return *this;
    }

    void SetFilter(Filter *filter)
    {
        static_assert(
            detail::FilterIsMember<ModelType, Filter>::value,
            "Static or void Filter does not require a filter instance.");

        this->filter_ = filter;
    }

    /** Implicit bool conversion returns true if the interface is currently
     ** tracking a Model and a Filter if required.
     **/
    explicit operator bool () const
    {
        if constexpr (
            detail::FilterIsMember<ModelType, Filter>::value)
        {
            // Filter requires a filter instance.
            return (this->model_ != nullptr) && (this->filter_ != nullptr);
        }
        else
        {
            return this->model_ != nullptr;
        }
    }

    Type Get() const
    {
        NOT_NULL(this->model_);

        if constexpr (std::is_void_v<Filter>)
        {
            return this->model_->Get();
        }
        else
        {
            return this->FilterOnGet_(this->model_->Get());
        }
    }

    void Set(typename detail::Argument<Type>::Type value)
    {
        static_assert(
            std::is_same_v<Access, GetAndSetTag>,
            "Cannot Set a read-only value.");

        NOT_NULL(this->model_);

        if constexpr (std::is_void_v<Filter>)
        {
            this->model_->Set(value);
        }
        else
        {
            this->model_->Set(this->FilterOnSet_(value));
        }
    }

private:
    ModelType FilterOnSet_(typename detail::Argument<Type>::Type value) const
    {
        if constexpr (std::is_same_v<void, Filter>)
        {
            return value;
        }
        else if constexpr (detail::SetterIsMember<ModelType, Filter>::value)
        {
            NOT_NULL(this->filter_);
            return this->filter_->Set(value);
        }
        else
        {
            // The filter is not a member method.
            return Filter::Set(value);
        }
    }

    Type FilterOnGet_(
        typename detail::Argument<ModelType>::Type value) const
    {
        if constexpr (std::is_same_v<void, Filter>)
        {
            return value;
        }
        else if constexpr (detail::GetterIsMember<ModelType, Filter>::value)
        {
            NOT_NULL(this->filter_);
            return this->filter_->Get(value);
        }
        else
        {
            // The filter doesn't accept a Filter * argument.
            return Filter::Get(value);
        }
    }

    static void OnModelChanged_(
        void * observer,
        typename detail::Argument<ModelType>::Type value)
    {
        // The model value has changed.
        // Update our observers.
        auto self = static_cast<Value_ *>(observer);

        if constexpr (!std::is_void_v<Filter>)
        {
            self->Notify_(self->FilterOnGet_(value));
        }
        else
        {
            self->Notify_(value);
        }
    }

    Model *model_;
    Filter *filter_;
};


template<typename Observer, typename Model, typename Access = GetAndSetTag>
using Value = Value_<Observer, Model, void, Access>;


template
<
    typename Observer,
    typename Model,
    typename Filter,
    typename Access = GetAndSetTag
>
using FilteredValue = Value_<Observer, Model, Filter, Access>;


template<typename Observer, typename Value>
struct ObservedValue;

template
<
    typename Observer,
    template<typename, typename...> typename Value,
    typename OtherObserver,
    typename... Others
>
struct ObservedValue<Observer, Value<OtherObserver, Others...>>
{
    using Type = Value<Observer, Others...>;
};


template<typename Observer>
struct BoundFilteredValue
{
    template<typename Model, typename Filter>
    using Type = FilteredValue<Observer, Model, Filter>;
};


template<typename Observer>
struct BoundValue
{
    template<typename Model>
    using Type = Value_<Observer, Model>;
};


} // namespace interface

} // namespace pex
