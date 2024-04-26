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
#include "pex/interface.h"


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

    void Clear()
    {
        this->pex_ = nullptr;
    }

protected:
    void SetWithoutNotify_(Argument<Type> value)
    {
        this->pex_->SetWithoutNotify_(value);
    }

    void SetWithoutFilter_(Argument<Type> value)
    {
        this->pex_->SetWithoutFilter_(value);
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


namespace detail
{


template<typename Pex>
class AccessReference: public Reference<Pex>
{
public:
    using Base = Reference<Pex>;
    using Type = typename Base::Type;

    using Base::Base;

    void Set(Argument<Type> value)
    {
        this->SetWithoutNotify_(value);
        this->DoNotify_();
    }

    void SetWithoutNotify(Argument<Type> value)
    {
        this->SetWithoutNotify_(value);
    }

    void SetWithoutFilter(Argument<Type> value)
    {
        this->SetWithoutFilter_(value);
    }

    void DoNotify()
    {
        this->DoNotify_();
    }
};


}


template<typename Pex, typename Value>
void SetOverride(Pex &pex, Value &&value) requires (IsModel<Pex>)
{
    static_assert(
        !HasAccess<SetTag, typename Pex::Access>,
        "This function is intended to override a read-only value");

    detail::AccessReference<Pex>(pex).Set(std::forward<Value>(value));
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
    using Access = typename Pex::Access;

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

    Defer & operator=(Argument<Type> value)
    {
        this->SetWithoutNotify_(value);
        return *this;
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
        // Choose the single-valued Defer for this member
        using Type = Defer<Selector<T>>;
    };

    template<typename T>
    struct DeferHelper_
    <
        T,
        std::enable_if_t<IsGroup<T>>
    >
    {
        // This member expands to a group.
        // Choose the appropriate DeferredGroup
        using Type = typename T::template DeferredGroup<Selector>;
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


namespace detail
{

template<typename Target, typename = void>
struct CanBeSet_: std::true_type {};

template<typename Target>
struct CanBeSet_
<
    Target,
    std::enable_if_t
    <
        IsSignal<Target>
    >
>: std::false_type {};


template<typename Target>
struct CanBeSet_
<
    Target,
    std::enable_if_t
    <
        !HasAccess<SetTag, typename Target::Access>
    >
>: std::false_type {};


template<typename Target>
inline constexpr bool CanBeSet = CanBeSet_<Target>::value;


template<typename Target, typename Source>
std::enable_if_t<CanBeSet<Target>>
SetByAccess(Target &target, const Source &source)
{
    target.Set(source);
}


template<typename Target, typename Source>
std::enable_if_t<!CanBeSet<Target>>
SetByAccess(Target &, const Source &)
{
    // Do not set members that are read-only.
}


template
<
    template<typename> typename Fields,
    template<template<typename> typename> typename Template,
    template<typename> typename Selector
>
class DeferredGroup:
    public Template<DeferSelector<Selector>::template Type>
{
public:
    using Upstream = Template<Selector>;
    using This = DeferredGroup<Fields, Template, Selector>;

    DeferredGroup() = default;

    DeferredGroup(Upstream &upstream)
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

    template<typename Plain>
    void Set(const Plain &plain)
    {
        auto assign = [this, &plain](auto deferField, auto plainField)
        {
            using MemberType = typename std::remove_reference_t<
                decltype(this->*(deferField.member))>;

            if constexpr (!std::is_same_v<DescribeSignal, MemberType>)
            {
                SetByAccess(
                    this->*(deferField.member),
                    plain.*(plainField.member));
            }
        };

        jive::ZipApply(
            assign,
            Fields<This>::fields,
            Fields<Plain>::fields);
    }
};


template<typename Upstream>
class MuteDeferred
{
public:
    MuteDeferred()
        :
        upstream_(nullptr),
        isMuted_(false)
    {

    }

    MuteDeferred(Upstream &upstream)
        :
        upstream_(&upstream),
        isMuted_(false)
    {
        this->Mute();
    }

    ~MuteDeferred()
    {
        if (this->isMuted_)
        {
            assert(this->upstream_);
            this->upstream_->GetMuteControlReference().Set(false);
        }
    }

    void Mute()
    {
        if (!this->upstream_)
        {
            throw std::logic_error("MuteDeferred is uninitialized");
        }

        this->upstream_->GetMuteControlReference().Set(true);
        this->isMuted_ = true;
    }

private:
    Upstream *upstream_;
    bool isMuted_;
};


} // end namespace detail


template
<
    template<typename> typename Fields,
    template<template<typename> typename> typename Template,
    template<typename> typename Selector,
    typename Upstream
>
class DeferGroup
{
public:
    DeferGroup() = default;

    DeferGroup(Upstream &upstream)
        :
        muteDeferred_(upstream),
        members(upstream)
    {
        // this->muteDeferred_.Mute();
    }

    template<typename Plain>
    void Set(const Plain &plain)
    {
        this->members.Set(plain);
    }

private:
    // Destruction order guarantees that the group will be unmuted only after
    // the deferred members have finished notifying.
    detail::MuteDeferred<Upstream> muteDeferred_;

public:
    detail::DeferredGroup<Fields, Template, Selector> members;
};


/**
 ** Allows direct access to the model value as a const reference, so there is
 ** no need to publish any new values; they cannot be changed.
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


template<typename Control>
class ConstControlReference
{

public:
    using Type = typename Control::Type;

    ConstControlReference(const Control &control)
        :
        modelReference_(control.GetModel_())
    {

    }

    ConstControlReference(const ConstControlReference &) = delete;
    ConstControlReference & operator=(const ConstControlReference &) = delete;

    const Type & Get() const
    {
        return this->modelReference_.Get();
    }

    const Type & operator * () const
    {
        return this->modelReference_.Get();
    }

private:
    const ConstReference<typename Control::Model_> modelReference_;
};


template<typename Pex>
auto MakeDefer(Pex &pex)
{
    if constexpr (DefinesDefer<Pex>::value)
    {
        // Group types define a Defer type.
        return typename Pex::Defer(pex);
    }
    else
    {
        return Defer<Pex>(pex);
    }
}


} // namespace pex
