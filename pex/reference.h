/**
  * @file reference.h
  *
  * @brief Provides access to Model/Control values by reference, delaying the
  * notification (if any) until editing is complete.
  *
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 14 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/
#pragma once


#include <jive/zip_apply.h>
#include "pex/model_value.h"
#include "pex/traits.h"
#include "pex/control_value.h"
#include "pex/signal.h"



namespace pex
{


/**
 ** While the Reference exists, the model's value has been changed, but not
 ** published.
 **
 ** The underlying value can be accessed directly using the dereference
 ** operator, but only for Values that do not use filters.
 **
 ** The new value will be published in the destructor.
 **/
template<typename Pex>
class Reference
{
public:
    Reference(): pex_(nullptr) {}

    Reference(const Reference &) = delete;

    Reference(Reference &&other)
        :
        pex_(other.pex_)
    {
        other.pex_ = nullptr;
    }

    Reference & operator=(const Reference &) = delete;

    Reference & operator=(Reference &&other)
    {
        this->pex_ = other.pex_;
        other.pex_ = nullptr;

        return *this;
    }

    Reference(Pex &pex)
        :
        pex_(&pex)
    {

    }

public:

    using Type = typename Pex::Type;

    const Type & operator * () const
    {
        return GetUpstreamReference(*this->pex_);
    }

    Type Get() const
    {
        return this->pex_->Get();
    }

    void Set(Argument<Type> value)
    {
        this->pex_->Set(value);
    }

protected:
    void SetWithoutNotify_(Argument<Type> value)
    {
        this->pex_->SetWithoutNotify_(value);
    }

    void DoNotify_()
    {
        this->pex_->DoNotify_();
    }

private:
    template<typename U>
    static const auto & GetUpstreamReference(const U &upstream)
    {
        if constexpr (IsModel<U>)
        {
            static_assert(
                std::is_same_v<NoFilter, typename U::Filter>,
                "Direct access to underlying value is incompatible with "
                "filters.");

            return upstream.value_;
        }
        else if constexpr (IsDirect<U>)
        {
            return GetUpstreamReference(*upstream.model_);
        }
        else if constexpr (IsControl<U>)
        {
            static_assert(
                std::is_same_v<NoFilter, typename U::Filter>,
                "Direct access to underlying value is incompatible with "
                "filters.");

            return GetUpstreamReference(upstream.upstream_);
        }
        else
        {
            static_assert(
                std::is_same_v<NoFilter, typename U::Filter>,
                "Direct access to underlying value is incompatible with "
                "filters.");

            return GetUpstreamReference(upstream.pex_);
        }
    }

protected:
    Pex *pex_;
};


namespace internal
{


template<typename Pex>
class AccessReference: public Reference<Pex>
{
public:
    using Base = Reference<Pex>;
    using Type = typename Base::Type;

    using Base::Base;

    void SetWithoutNotify(Argument<Type> value)
    {
        this->SetWithoutNotify_(value);
    }

    void DoNotify()
    {
        this->DoNotify_();
    }
};


}


/**
 ** While the Defer exists, the model's value has been changed, but not
 ** published.
 **
 ** The new value will be published in the destructor.
 **/
template<typename Pex>
class Defer: public Reference<Pex>
{
public:
    using Base = Reference<Pex>;
    using Type = typename Base::Type;

    using Base::Base;

    Defer(Defer &&other)
        :
        Base(std::move(other))
    {

    }

    Defer & operator=(Defer &&other)
    {
        if (this->pex_)
        {
            this->DoNotify_();
        }

        Base::operator=(std::move(other));

        return *this;
    }

    void Set(Argument<Type> value)
    {
        this->SetWithoutNotify_(value);
    }

    ~Defer()
    {
        // Notify on destruction
        if (this->pex_)
        {
            this->DoNotify_();
        }
    }
};


template<template<typename> typename Selector>
struct DeferSelector
{
    template<typename T, typename Enable = void>
    struct DeferHelper_
    {
        using Type = Defer<Selector<T>>;
    };

    template<typename T>
    struct DeferHelper_
    <
        T,
        std::enable_if_t<IsSignal<Selector<T>>>
    >
    {
        using Type = DescribeSignal;
    };

    template<typename T>
    using Type = typename DeferHelper_<T>::Type;
};


template
<
    typename Plain,
    template<typename> typename Fields,
    template<template<typename> typename> typename Template,
    template<typename> typename Selector
>
class DeferGroup:
    public Template<DeferSelector<Selector>::template Type>
{
public:
    using This = DeferGroup<Plain, Fields, Template, Selector>;
    using Upstream = Template<Selector>;

    DeferGroup(Upstream &upstream)
    {
        auto initialize = [this, &upstream]
            (auto deferField, auto upstreamField)
        {
            using MemberType = typename std::remove_reference_t<
                decltype(this->*(deferField.member))>;

            if constexpr (!std::is_same_v<DescribeSignal, MemberType>)
            {
                this->*(deferField.member) = MemberType(
                    (upstream.*(upstreamField.member)));
            }
        };

        jive::ZipApply(
            initialize,
            Fields<This>::fields,
            Fields<Upstream>::fields);
    }

    void Set(const Plain &plain)
    {
        auto assign = [this, &plain](auto deferField, auto plainField)
        {
            using MemberType = typename std::remove_reference_t<
                decltype(this->*(deferField.member))>;

            if constexpr (!std::is_same_v<DescribeSignal, MemberType>)
            {
                (this->*(deferField.member)).Set(plain.*(plainField.member));
            }
        };

        jive::ZipApply(
            assign,
            Fields<This>::fields,
            Fields<Plain>::fields);
    }
};


/**
 ** Allows direct access to the model value as a const reference, so there is
 ** no need to publish any new values.
 **
 ** It is not possible to create a ConstReference to a Value with a filter.
 **/
template<typename Model>
class ConstReference
{
    static_assert(
        IsModel<Model>,
        "Access to the value by reference is only possible for model values.");

    static_assert(
        std::is_same_v<NoFilter, typename Model::Filter>,
        "Direct access to underlying value is incompatible with filters.");

public:
    using Type = typename Model::Type;

    ConstReference(const Model &model)
        :
        model_(model)
    {

    }

    ConstReference(const ConstReference &) = delete;
    ConstReference & operator=(const ConstReference &) = delete;

    const Type & Get() const
    {
        return this->model_.value_;
    }

    const Type & operator * () const
    {
        return this->model_.value_;
    }

private:
    const Model &model_;
};


} // namespace pex
