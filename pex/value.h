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
#include "pex/detail/value_detail.h"


namespace pex
{

namespace model
{


// Model must use unbound callbacks so it can notify disparate types.
// All observers are stored as void *.
template<typename T, typename Filter_>
class Value_ : public detail::NotifyMany<detail::ValueNotify<void, T>>
{
    static_assert(detail::ModelFilterIsVoidOrValid<T, Filter_>::value);

public:
    using Notify = detail::ValueNotify<void, T>;
    using Type = T;
    using Filter = Filter_;

    // All model nodes have writable access.
    using Access = GetAndSetTag;

    template<typename Pex>
    friend class Transaction;

    template<typename Pex>
    friend class ::pex::Reference;

    template<typename Pex>
    friend class ::pex::ConstReference;

    Value_()
        :
        value_{},
        filter_{}
    {

    }

    explicit Value_(T value)
        :
        value_{value},
        filter_{}
    {

    }

    Value_(T value, Filter *filter)
        :
        value_{value},
        filter_{filter}
    {
        static_assert(
            detail::FilterIsMember<T, Filter>::value,
            "Static or void Filter does not require a filter instance.");
    }

    Value_(Filter *filter)
        :
        value_{},
        filter_{filter}
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
        if constexpr (std::is_void_v<Filter>)
        {
            this->value_ = value;
        }
        else if constexpr (
            std::is_invocable_r_v<T, decltype(&Filter::Set), Filter, T>)
        {
            NOT_NULL(this->filter_);
            this->value_ = this->filter_->Set(value);
        }
        else
        {
            // The filter is not void
            // and the filter doesn't accept a Filter * argument.
            this->value_ = Filter::Set(value);
        }

        this->Notify_(this->value_);
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
    T value_;
    Filter * filter_;
};


/**
 ** While Transaction exists, the model's value has been changed, but not
 ** published.
 **
 ** When you are ready to publish, call Commit.
 **
 ** If the Transaction goes out of scope without a call to Commit, the model
 ** value is reverted, and nothing is published.
 **
 **/
template<typename Pex>
class Transaction
{
    static_assert(
        std::is_same_v<void, typename Pex::Filter>,
        "Direct access to underlying value is incompatible with filters.");

public:
    using Type = typename Pex::Type;

    Transaction(Pex &model)
        :
        model_(&model),
        oldValue_(model.Get())
    {

    }

    Transaction(Pex &model, Type value)
        :
        model_(&model),
        oldValue_(model.Get())
    {
        this->model_->SetWithoutNotify_(value);
    }

    Transaction(const Transaction &) = delete;
    Transaction & operator=(const Transaction &) = delete;

    Type & Get()
    {
        NOT_NULL(this->model_);
        return this->model_->value_;
    }

    void Commit()
    {
        if (nullptr != this->model_)
        {
            this->model_->Notify_(this->model_->value_);
            this->model_ = nullptr;
        }
    }

    ~Transaction()
    {
        // Revert on destruction
        if (nullptr != this->model_)
        {
            this->model_->SetWithoutNotify_(this->oldValue_);
        }
    }

    Pex *model_;
    Type oldValue_;
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
    public detail::NotifyOne
    <
        detail::ValueNotify
        <
            Observer_,
            typename detail::FilterType<typename Model_::Type, Filter_>::Type
        >
    >
{
public:
    using Observer = Observer_;
    using Model = Model_;
    using Filter = Filter_;
    using Access = Access_;
    using ModelType = typename Model::Type;
    using Type = typename detail::FilterType<ModelType, Filter>::Type;

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
        detail::InterfaceFilterIsVoidOrValid
        <
            ModelType,
            Filter,
            Access
        >::value);

    Value_(): model_(nullptr), filter_(nullptr) {}

    explicit Value_(Model * model)
        :
        model_(model),
        filter_(nullptr)
    {
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
            return this->FilterGet_(this->model_->Get());
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
            this->model_->Set(this->FilterSet_(value));
        }
    }

private:
    ModelType FilterSet_(typename detail::Argument<Type>::Type value) const
    {
        static_assert(
            std::is_same_v<Access, GetAndSetTag>,
            "Cannot Set a read-only value.");

        if constexpr (detail::SetterIsMember<ModelType, Filter>::value)
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

    Type FilterGet_(
        typename detail::Argument<ModelType>::Type value) const
    {
        if constexpr (detail::GetterIsMember<ModelType, Filter>::value)
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
            self->Notify_(self->FilterGet_(value));
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
