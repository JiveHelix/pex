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
#include "pex/detail/notify.h"
#include "pex/detail/value_detail.h"

namespace pex
{

namespace model
{

// Model must use unbound callbacks so it can notify disparate types.
// All observers are stored as void *.
template<typename T, typename Filter>
class Value_ : public detail::NotifyMany<detail::ValueNotify<void, T>>
{
    static_assert(detail::ModelFilterIsVoidOrValid<T, Filter>::value);

public:
    using Type = T;

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

    Value_(const Value_<T, Filter> &) = delete;
    Value_(Value_<T, Filter> &&) = delete;

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
        if constexpr (detail::FilterIsMember<T, Filter>::value)
        {
            this->filter_ = filter;    
        }
        else
        {
            throw std::logic_error("Filter is void or static");
        }
    }

    /** If Filter requires a Filter instance, and it has been set, then bool
     ** conversion returns true.
     ** 
     ** If the Filter is void or static, it always returns true.
     **/
    operator bool () const
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

template<typename T>
using Value = Value_<T, void>;

template<typename T, typename Filter>
using FilteredValue = Value_<T, Filter>;


} // namespace model


namespace interface
{


template<
    typename Observer,
    typename Model,
    typename Filter = void>
class Value_
#ifdef ALLOW_MULTIPLE_CALLBACKS
    : public detail::NotifyMany<
#else
    : public detail::NotifyOne<
#endif
        detail::ValueNotify<Observer, typename Model::Type>>
{
public:

    static_assert(
        detail::DefinesType<Model>::value,
        "Model must define Model::Type");

    using T = typename Model::Type;

    static_assert(detail::InterfaceFilterIsVoidOrValid<T, Filter>::value);

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
    explicit Value_(const Value_<OtherObserver, Model, OtherFilter> &other)
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
    explicit Value_(const Value_<OtherObserver, Model, Filter> &other)
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

    template<typename OtherObserver, typename OtherModel, typename OtherFilter>
    Value_<Observer, Model, Filter> & operator=(
        const Value_<OtherObserver, OtherModel, OtherFilter> &other)
    {
        static_assert(
            std::is_same_v<Model, OtherModel>,
            "Copy may differ in observer and filter types, but must track the "
            "same model value.");

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
        if constexpr (
            detail::FilterIsMember<typename Model::Type, Filter>::value)
        {
            this->filter_ = filter;    
        }
        else
        {
            throw std::logic_error("Filter is void or static");
        }
    }

    /** Implicit bool conversion returns true if the interface is currently
     ** tracking a Model and a Filter if required.
     **/
    operator bool () const
    {
        if constexpr (
            detail::FilterIsMember<typename Model::Type, Filter>::value)
        {
            // Filter requires a filter instance.
            return (this->model_ != nullptr) && (this->filter_ != nullptr);
        }
        else
        {
            return this->model_ != nullptr;
        }
    }

    T Get() const
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

    void Set(typename detail::Argument<T>::Type value)
    {
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
    T FilterSet_(typename detail::Argument<T>::Type value) const
    {
        if constexpr (
            std::is_invocable_r_v<T, decltype(&Filter::Set), Filter, T>)
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

    T FilterGet_(typename detail::Argument<T>::Type value) const
    {
        if constexpr (
            std::is_invocable_r_v<T, decltype(&Filter::Get), Filter, T>)
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
        typename detail::Argument<T>::Type value)
    {
        // The model value has changed.
        // Update our observers.
        auto self = static_cast<Value_ *>(observer);

        if constexpr (!std::is_void_v<Filter>)
        {
            value = self->FilterGet_(value);
        }

        self->Notify_(value);
    }

    Model *model_;
    Filter *filter_;
};



template<typename Observer, typename Model>
using Value = Value_<Observer, Model, void>;


template<typename Observer, typename Model, typename Filter>
using FilteredValue = Value_<Observer, Model, Filter>;


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
    using Type = Value<Observer, Model>;
};


} // namespace interface

} // namespace pex
