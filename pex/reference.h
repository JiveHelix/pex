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
#include "pex/detail/mute.h"


namespace pex
{


class NestedLog
{
private:
    static size_t depth_;

public:
    NestedLog()
    {
        ++depth_;
    }

    NestedLog(const std::string &message)
        :
        NestedLog()
    {
        this->operator<<(message) << std::endl;
    }

    ~NestedLog()
    {
        --depth_;
    }

    template<typename T>
    std::ostream & operator<<(T &&object)
    {
        return std::cout << std::string(depth_ * 2, ' ') << object;
    }
};


inline size_t NestedLog::depth_ = 0;


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


template <typename Pex>
AccessReference(Pex) -> AccessReference<Pex>;


} // end namespace detail


// For template argument deduction.
template<typename Pex>
detail::AccessReference<Pex> AccessReference(Pex &pex)
{
    return detail::AccessReference<Pex>(pex);
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

    Defer()
        :
        Base(),
        isChanged_(false)
    {

    }

    Defer(Pex &pex)
        :
        Base(pex),
        isChanged_(false)
    {

    }

    Defer(Defer &&other)
        :
        Base(std::move(other)),
        isChanged_(other.isChanged_)
    {
        other.isChanged_ = false;
    }

    Defer & operator=(Defer &&other)
    {
        if (this->pex_ && this->isChanged_)
        {
            this->DoNotify_();
        }

        Base::operator=(std::move(other));
        this->isChanged_ = other.isChanged_;

        return *this;
    }

    void Set(Argument<Type> value)
    {
        this->isChanged_ = true;
        this->SetWithoutNotify_(value);
    }

    Defer & operator=(Argument<Type> value)
    {
        this->isChanged_ = true;
        this->SetWithoutNotify_(value);
        return *this;
    }

    ~Defer()
    {
        // Notify on destruction
        this->DoNotify();
    }

    void DoNotify()
    {
        if (this->pex_ && this->isChanged_)
        {
            this->DoNotify_();
            this->isChanged_ = false;
        }

        this->pex_ = nullptr;
    }

private:
    bool isChanged_;
};


template<typename Pex, typename Super>
class PolyDefer: public Defer<Pex>
{
public:
    using Base = Defer<Pex>;
    using Type = typename Base::Type;
    using Access = typename Pex::Access;

    using Base::Base;

    PolyDefer(PolyDefer &&other)
        :
        Base(std::move(other))
    {

    }

    PolyDefer & operator=(PolyDefer &&other)
    {
        if (this->pex_)
        {
            this->DoNotify_();
        }

        Base::operator=(std::move(other));

        return *this;
    }

    PolyDefer & operator=(Argument<Type> value)
    {
        this->SetWithoutNotify_(value);
        return *this;
    }

    Super * GetVirtual()
    {
        assert(this->pex_);

        return this->pex_->GetVirtual();
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
        // Choose the appropriate DeferGroup
        using Type = typename Selector<T>::Defer;
    };

    template<typename T>
    struct DeferHelper_
    <
        T,
        std::enable_if_t<IsList<T>>
    >
    {
        // This member expands to a list.
        // Choose the appropriate DeferList
        using Type = typename Selector<T>::Defer;
    };

    template<typename T>
    struct DeferHelper_
    <
        T,
        std::enable_if_t<IsPolyModel<Selector<T>>>
    >
    {
        // This member expands to a PolyModel.
        // Choose the appropriate PolyDefer
        using Type = PolyDefer<Selector<T>, typename Selector<T>::SuperModel>;
    };

    template<typename T>
    struct DeferHelper_
    <
        T,
        std::enable_if_t<IsPolyControl<Selector<T>>>
    >
    {
        // This member expands to a PolyControl.
        // Choose the appropriate PolyDefer
        using Type = PolyDefer<Selector<T>, typename Selector<T>::SuperControl>;
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


} // end namespace detail


template
<
    template<typename> typename Fields,
    template<template<typename> typename> typename Template,
    template<typename> typename Selector,
    typename Upstream
>
class DeferGroup
    :
    public Template<DeferSelector<Selector>::template Type>
{
public:
    using This = DeferGroup<Fields, Template, Selector, Upstream>;

    DeferGroup()
        :
        upstream_(nullptr),
        scopeMute_()
    {

    }

    DeferGroup(Upstream &upstream)
        :
        upstream_(&upstream),
        scopeMute_(upstream, false)
    {
        // Every member of this group will also be Defer/DeferGroup/DeferList
        // depending on each type.
        auto initialize = [this, &upstream]
            (auto deferField, [[maybe_unused]] auto upstreamField)
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

    DeferGroup(const DeferGroup &) = delete;
    DeferGroup & operator=(const DeferGroup &) = delete;

    DeferGroup(DeferGroup &&other)
        :
        upstream_(other.upstream_),
        scopeMute_(std::move(other.scopeMute_))
    {
        auto doMove = [this, &other] (auto deferField)
        {
            using MemberType = typename std::remove_reference_t<
                decltype(this->*(deferField.member))>;

            if constexpr (!std::is_same_v<DescribeSignal, MemberType>)
            {
                this->*(deferField.member) =
                    std::move(other.*(deferField.member));
            }
        };

        jive::ForEach(Fields<This>::fields, doMove);
        other.upstream_ = nullptr;
    }

    DeferGroup & operator=(DeferGroup &&other)
    {
        this->upstream_ = other.upstream_;
        this->scopeMute_ = std::move(other.scopeMute_);

        auto doMove = [this, &other] (auto deferField)
        {
            using MemberType = typename std::remove_reference_t<
                decltype(this->*(deferField.member))>;

            if constexpr (!std::is_same_v<DescribeSignal, MemberType>)
            {
                this->*(deferField.member) =
                    std::move(other.*(deferField.member));
            }
        };

        jive::ForEach(Fields<This>::fields, doMove);
        other.upstream_ = nullptr;

        return *this;
    }

    ~DeferGroup()
    {
        // The DoNotify calls will only send notifications if they haven't
        // already.
        this->DoNotify();
    }

    using Plain = typename Upstream::Plain;

    Plain Get() const
    {
        return this->upstream_->Get();
    }

    void DoNotify()
    {
        if (!this->scopeMute_.IsMuted())
        {
            return;
        }

        // Notify all members before unmuting the aggregate observer.
        auto doNotify = [this](auto deferField)
        {
            using MemberType = typename std::remove_reference_t<
                decltype(this->*(deferField.member))>;

            if constexpr (!std::is_same_v<DescribeSignal, MemberType>)
            {
                (this->*(deferField.member)).DoNotify();
            }
        };

        jive::ForEach(Fields<This>::fields, doNotify);

        this->scopeMute_.Unmute();
    }

    template<typename Plain>
    void Set(const Plain &plain)
    {
        auto assign = [this, &plain](
            auto deferField,
            [[maybe_unused]] auto plainField)
        {
            using MemberType = typename std::remove_reference_t<
                decltype(this->*(deferField.member))>;

            if constexpr (!std::is_same_v<DescribeSignal, MemberType>)
            {
                // Does not set fields are read-only.
                detail::SetByAccess(
                    this->*(deferField.member),
                    plain.*(plainField.member));
            }
        };

        jive::ZipApply(
            assign,
            Fields<This>::fields,
            Fields<Plain>::fields);
    }

    void Clear()
    {
        // Recursively call Clear() on all members that implement it.
        auto clear = [this](auto deferField)
        {
            using MemberType = typename std::remove_reference_t<
                decltype(this->*(deferField.member))>;

            if constexpr (!std::is_same_v<DescribeSignal, MemberType>)
            {
                (this->*(deferField.member)).Clear();
            }
        };

        jive::ForEach(Fields<This>::fields, clear);

        this->scopeMute_.Clear();
    }

private:
    Upstream *upstream_;
    detail::ScopeMute<Upstream> scopeMute_;
};


template
<
    typename MemberType,
    template<typename> typename Selector,
    typename Upstream
>
class DeferList
{
public:
    using DeferredMember =
        typename DeferSelector<Selector>::template Type<MemberType>;

    using Items = std::vector<DeferredMember>;

    using DeferredCount =
        typename DeferSelector<Selector>::template Type<size_t>;

    using DeferredSelected =
        typename DeferSelector<Selector>::template Type<std::optional<size_t>>;

    using Iterator = typename Items::iterator;
    using ConstIterator = typename Items::const_iterator;

    using ReverseIterator = typename Items::reverse_iterator;
    using ConstReverseIterator = typename Items::const_reverse_iterator;

    DeferList()
        :
        scopeMute_(),
        upstream_(nullptr),
        items_(),
        count(),
        selected()
    {

    }

    DeferList(Upstream &upstream)
        :
        scopeMute_(upstream, false),
        upstream_(&upstream),
        items_(upstream.count.Get()),
        count(upstream.count),
        selected(upstream.selected)
    {
        size_t itemCount = items_.size();

        for (size_t i = 0; i < itemCount; ++i)
        {
            this->items_[i] = DeferredMember(upstream[i]);
        }
    }

    ~DeferList()
    {
        this->DoNotify();
    }

    DeferList(DeferList &&other)
        :
        scopeMute_(std::move(other.scopeMute_)),
        upstream_(other.upstream_),
        items_(std::move(other.items_)),
        count(std::move(other.count)),
        selected(std::move(other.selected))
    {
        assert(other.items_.size() == 0);
        other.upstream_ = nullptr;
    }

    DeferList & operator=(DeferList &&other)
    {
        if (this->items_.size())
        {
            throw std::logic_error("Assign to armed DeferList");
        }

        this->scopeMute_ = std::move(other.scopeMute_);
        this->upstream_ = other.upstream_;

        this->items_ = std::move(other.items_);
        this->count = std::move(other.count);
        this->selected = std::move(other.selected);

        assert(other.items_.size() == 0);

        other.upstream_ = nullptr;

        return *this;
    }

    using Type = typename Upstream::Type;

    Type Get() const
    {
        return this->upstream_->Get();
    }

    void DoNotify()
    {
        for (auto &item: this->items_)
        {
            item.DoNotify();
        }

        this->count.DoNotify();
        this->selected.DoNotify();

        this->scopeMute_.Unmute();
    }

    template<typename Plain>
    void Set(const Plain &plain)
    {
        assert(this->upstream_);

        auto itemCount = plain.size();
        this->count.Set(itemCount);

        if (itemCount != this->items_.size())
        {
            // Clear the existing deferred members so they do not notify.
            this->ClearItems();

            detail::AccessReference<Upstream>(*this->upstream_)
                .SetWithoutNotify(plain);

            // SetWithoutNotify will call Unmute.
            // TODO: Consider turning mute/unmute into a push/pop LIFO to
            // prevent nested calls modifying the state.
            this->scopeMute_.Mute(false);

            this->items_.resize(itemCount);

            for (size_t i = 0; i < itemCount; ++i)
            {
                this->items_[i] = DeferredMember((*this->upstream_)[i]);
            }
        }

        for (size_t i = 0; i < itemCount; ++i)
        {
            this->items_[i].Set(plain[i]);
        }
    }

    void ClearItems()
    {
        for (auto &item: this->items_)
        {
            item.Clear();
        }

        this->items_.clear();
    }

    void Clear()
    {
        this->ClearItems();
        this->count.Clear();
        this->selected.Clear();
        this->scopeMute_.Clear();
    }

    Iterator begin()
    {
        return std::begin(this->items_);
    }

    Iterator end()
    {
        return std::end(this->items_);
    }

    ConstIterator begin() const
    {
        return std::begin(this->items_);
    }

    ConstIterator end() const
    {
        return std::end(this->items_);
    }

    ReverseIterator rbegin()
    {
        return std::rbegin(this->items_);
    }

    ReverseIterator rend()
    {
        return std::rend(this->items_);
    }

    ConstReverseIterator rbegin() const
    {
        return std::rbegin(this->items_);
    }

    ConstReverseIterator rend() const
    {
        return std::rend(this->items_);
    }

    DeferredMember & operator[](size_t index)
    {
        return this->items_[index];
    }

    DeferredMember & at(size_t index)
    {
        return this->items_.at(index);
    }

    const DeferredMember & operator[](size_t index) const
    {
        return this->items_[index];
    }

    size_t size() const
    {
        return this->items_.size();
    }

    bool empty() const
    {
        return this->items_.empty();
    }

private:
    // Destruction order guarantees that the group will be unmuted only after
    // the deferred members have finished notifying.
    detail::ScopeMute<Upstream> scopeMute_;
    Upstream * upstream_;

    Items items_;

public:
    DeferredCount count;
    DeferredSelected selected;
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
    else if constexpr (IsPolyModel<Pex>)
    {
        return PolyDefer<Pex, typename Pex::SuperModel>(pex);
    }
    else if constexpr (IsPolyControl<Pex>)
    {
        return PolyDefer<Pex, typename Pex::SuperControl>(pex);
    }
    else
    {
        return Defer<Pex>(pex);
    }
}


} // namespace pex
