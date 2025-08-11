#pragma once


#include <vector>

#include <jive/scope_flag.h>
#include <jive/vector.h>
#include "pex/model_value.h"
#include "pex/control_value.h"
#include "pex/terminus.h"
#include "pex/signal.h"
#include "pex/detail/mute.h"
#include "pex/detail/log.h"
#include "pex/reference.h"
#include "pex/selectors.h"


namespace pex
{


namespace detail
{


template<typename, typename>
class ListConnect;


template<typename T>
concept HasListItem = requires { typename T::ListItem; };


template<typename T, typename Enable = void>
struct ListMember_
{
    using Type = typename T::value_type;
};


template<typename T>
struct ListMember_<T, std::enable_if_t<HasListItem<T>>>
{
    using Type = typename T::ListItem;
};

template<typename T>
using ListMember = typename ListMember_<T>::Type;


} // end namespace detail


template<typename ListMaker>
using ListModelItem = typename ModelSelector<ListMaker>::ListItem;


template<typename ListMaker>
using ListControlItem = typename ControlSelector<ListMaker>::ListItem;


template<typename T>
concept HasGetBaseWillDelete = requires(T t)
{
    { t.GetBaseWillDelete() } -> IsSignal;
};


template<typename T>
concept HasGetBaseCreated = requires(T t)
{
    { t.GetBaseCreated() } -> IsSignal;
};


template<typename T>
concept HasBaseSignals = HasGetBaseWillDelete<T> && HasGetBaseCreated<T>;


namespace model
{


using ListFlag = Value<bool>;
using ListCount = Value<size_t>;
using ListOptionalIndex = Value<std::optional<size_t>>;


} // namespace model


class ScopedListFlag
{
public:
    ScopedListFlag(::pex::model::ListFlag &listFlag)
        :
        flag_(listFlag)
    {
        this->flag_.Set(true);
    }

    ~ScopedListFlag()
    {
        this->flag_.Set(false);
    }

private:
    ::pex::model::ListFlag &flag_;
};


namespace control
{


using ListFlag = Value<::pex::model::ListFlag>;
using ListCount = Value<::pex::model::ListCount>;
using ListOptionalIndex = Value<::pex::model::ListOptionalIndex>;


} // end namespace control


#ifdef ENABLE_PEX_NAMES


template<typename Parent, typename Items>
void RegisterItemNames(Parent *parent, Items &items)
{
    size_t i = 0;

    for (auto &item: items)
    {
        pex::PexName(
            item.get(),
            parent,
            fmt::format("item {}", i++));
    }
}


template<typename Parent, typename Items>
void RegisterItemrefNames(Parent *parent, Items &items)
{
    size_t i = 0;

    for (auto &item: items)
    {
        if constexpr (HasGetVirtual<std::remove_cvref_t<decltype(item)>>)
        {
            pex::PexName(
                item.GetVirtual(),
                parent,
                fmt::format("item {}", i++));
        }
        else
        {
            pex::PexName(
                &item,
                parent,
                fmt::format("item {}", i++));
        }
    }
}


template<typename Items>
void ClearItemNames(Items &items)
{
    for (auto &item: items)
    {
        pex::ClearPexName(item.get());
    }
}


template<typename Items>
void ClearItemrefNames(Items &items)
{
    for (auto &item: items)
    {
        if constexpr (HasGetVirtual<std::remove_cvref_t<decltype(item)>>)
        {
            pex::ClearPexName(item.GetVirtual());
        }
        else
        {
            pex::ClearPexName(&item);
        }
    }
}


#define REGISTER_ITEM_NAMES(parent, items) RegisterItemNames(parent, items)

#define REGISTER_ITEMREF_NAMES(parent, items) \
    RegisterItemrefNames(parent, items)

#define CLEAR_ITEM_NAMES(items) ClearItemNames(items)

#define CLEAR_ITEMREF_NAMES(items) \
    ClearItemrefNames(items)

#else

#define REGISTER_ITEM_NAMES(parent, items)
#define REGISTER_ITEMREF_NAMES(parent, items)
#define CLEAR_ITEM_NAMES(items)
#define CLEAR_ITEMREF_NAMES(items)

#endif


template<typename Member, size_t initialCount_ = 0>
struct List
{
    static constexpr bool isList = true;
    static constexpr size_t initialCount = initialCount_;

    class Control;

    using Item = typename ModelSelector<Member>::Type;
    using Type = std::vector<Item>;

    class Model
        :
        public detail::MuteOwner,
        public detail::Mute
    {
    public:
        static constexpr bool isListModel = true;
        static constexpr auto observerName = "pex::List::Model";

        using ControlType = Control;
        using ListItem = ModelSelector<Member>;
        using Item = typename ListItem::Type;
        using Type = std::vector<Item>;
        using Count = ::pex::model::ListCount;
        using Selected = ::pex::model::ListOptionalIndex;
        using MemberAdded = ::pex::model::ListOptionalIndex;
        using MemberWillRemove = ::pex::model::ListOptionalIndex;
        using MemberRemoved = ::pex::model::ListOptionalIndex;
        using MemberWillReplace = ::pex::model::ListOptionalIndex;
        using MemberReplaced = ::pex::model::ListOptionalIndex;
        using ListFlag = ::pex::model::ListFlag;
        using Access = GetAndSetTag;

        using Defer = DeferList<Member, ModelSelector, Model>;

        template<typename>
        friend class ::pex::Reference;

        friend class Control;

        template<typename, typename>
        friend class ::pex::detail::ListConnect;

    private:
        bool ignoreCount_;

    public:
        Count count;
        Selected selected;
        MemberAdded memberAdded;
        MemberWillRemove memberWillRemove;
        MemberRemoved memberRemoved;
        MemberWillReplace memberWillReplace;
        MemberReplaced memberReplaced;
        ListFlag isNotifying;

        Model()
            :
            detail::MuteOwner(),
            detail::Mute(this->GetMuteControl()),
            ignoreCount_(false),
            count(initialCount),
            selected(),
            memberAdded(),
            memberWillRemove(),
            memberRemoved(),
            memberWillReplace(),
            memberReplaced(),
            isNotifying(),
            items_(),

            countTerminus_(
                PEX_THIS(
                    fmt::format(
                        "pex::List<{}>::Model",
                        jive::GetTypeName<Member>())),

                PEX_MEMBER_PASS(count),
                &Model::OnCount_),

            selectedTerminus_(
                this,
                PEX_MEMBER_PASS(selected),
                &Model::OnSelected_),

            selectionReceived_(false),
            internalMemberAdded_(),
            internalMemberReplaced_(),
            baseWillDeleteEndpoints_(),
            baseCreatedEndpoints_()
        {
            PEX_NAME(
                fmt::format(
                    "pex::List<{}>::Model",
                    jive::GetTypeName<Member>()));

            PEX_MEMBER(memberAdded);
            PEX_MEMBER(memberWillRemove);
            PEX_MEMBER(memberRemoved);
            PEX_MEMBER(memberWillReplace);
            PEX_MEMBER(memberReplaced);
            PEX_MEMBER(isNotifying);

            size_t toInitialize = initialCount;

            while (toInitialize--)
            {
                this->items_.push_back(std::make_unique<ListItem>());
            }

            REGISTER_ITEM_NAMES(this, this->items_);
            PEX_MEMBER(internalMemberWillRemove_);
            PEX_MEMBER(internalMemberAdded_);
            PEX_MEMBER(internalMemberWillReplace_);
            PEX_MEMBER(internalMemberReplaced_);
            PEX_MEMBER(baseWillDeleteEndpoints_);
            PEX_MEMBER(baseCreatedEndpoints_);

            this->RestoreBaseEndpoints_(0);
        }

        Model(const Type &items)
            :
            Model()
        {
            this->Set(items);
        }

        ~Model()
        {
            PEX_CLEAR_NAME(this);
            PEX_CLEAR_NAME(&memberAdded);
            PEX_CLEAR_NAME(&memberWillRemove);
            PEX_CLEAR_NAME(&memberRemoved);
            PEX_CLEAR_NAME(&memberWillReplace);
            PEX_CLEAR_NAME(&memberReplaced);
            PEX_CLEAR_NAME(&isNotifying);
            CLEAR_ITEM_NAMES(this->items_);
            PEX_CLEAR_NAME(&internalMemberWillRemove_);
            PEX_CLEAR_NAME(&internalMemberAdded_);
            PEX_CLEAR_NAME(&internalMemberWillReplace_);
            PEX_CLEAR_NAME(&internalMemberReplaced_);
            PEX_CLEAR_NAME(&baseWillDeleteEndpoints_);
            PEX_CLEAR_NAME(&baseCreatedEndpoints_);
        }

        ListItem & operator[](size_t index)
        {
#ifndef NDEBUG
            auto &pointer = this->items_.at(index);

            if (!pointer)
            {
                throw std::logic_error("item unitialized");
            }

            return *pointer;
#else
            auto &pointer = this->items_[index];

            return *pointer;
#endif
        }

        void EraseSelected()
        {
            auto selected_ = this->selected.Get();

            if (!selected_)
            {
                return;
            }

            this->Erase(*selected_);
        }

        ListItem & at(size_t index)
        {
            auto &pointer = this->items_.at(index);

            if (!pointer)
            {
                throw std::logic_error("item unitialized");
            }

            return *pointer;
        }

        const ListItem & at(size_t index) const
        {
            const auto &pointer = this->items_.at(index);

            if (!pointer)
            {
                throw std::logic_error("item unitialized");
            }

            return *pointer;
        }

        Type Get() const
        {
            Type result;

            for (auto &item: this->items_)
            {
                if (!item)
                {
                    throw std::logic_error("item unitialized");
                }

                result.push_back(item->Get());
            }

            return result;
        }

        size_t size() const
        {
            return this->items_.size();
        }

        bool empty() const
        {
            return this->items_.empty();
        }

        // Initialize values without sending notifications.
        void SetInitial(const Type &values)
        {
            if (values.empty())
            {
                return;
            }

            this->SetWithoutNotify_(values);
        }

        void Set(const Type &values)
        {
            this->SetWithoutNotify_(values);

            if (!this->IsMuted())
            {
                this->DoNotify_();
            }
        }

        template<typename Derived>
        size_t Append(const Derived &item)
        {
            // Mute while setting item values.
            auto scopeMute = detail::ScopeMute<Model>(*this, false);

            size_t newIndex = this->count.Get();
            auto wasSelected = this->selected.Get();
            auto newCount = newIndex + 1;

            // count observers will be notified at the end of this function.
            // ScopeMute calls Mute on aggregate types (groups and lists) at
            // the start of the scope, and unmutes at the end.
            // For any entity listening for a notification of this->count
            // (which is not an aggregate type), they will be notifified when
            // deferCount is destroyed, even though scopeMute is still in
            // effect.
            {
                jive::ScopeFlag ignoreCount(this->ignoreCount_);

                auto deferCount = pex::MakeDefer(this->count);
                deferCount.Set(newCount);

                this->selected.Set({});

                this->items_.push_back(std::make_unique<ListItem>());

                if constexpr (HasGetVirtual<ListItem>)
                {
                    PEX_MEMBER_ADDRESS(
                        this->items_.back()->GetVirtual(),
                        fmt::format("item {}", newCount - 1));
                }
                else
                {
                    PEX_MEMBER_ADDRESS(
                        this->items_.back().get(),
                        fmt::format("item {}", newCount - 1));
                }

                // Add the new item at the back of the list.
                this->items_.back()->Set(item);

                this->selectionReceived_ = false;

                this->RestoreBaseEndpoints_(newIndex);
                this->internalMemberAdded_.Set(newIndex);
                this->memberAdded.Set(newIndex);
            }

            if (wasSelected && !this->selectionReceived_)
            {
                // Nothing changed the selection in response to memberAdded.
                this->selected.Set(wasSelected);
            }

            return newIndex;
        }

        template<typename Derived>
        void Insert(size_t index, const Derived &item)
        {
            assert(index <= this->items_.size());

            auto scopedFlag = ScopedListFlag(this->isNotifying);

            // Mute while setting item values.
            auto scopeMute = detail::ScopeMute<Model>(*this, false);

            auto newCount = this->items_.size() + 1;

            // Insertion doesn't change whether an item is selected.
            // selected will be adjusted to the new index after the insertion.
            auto wasSelected = this->selected.Get();

            // count observers will be notified at the end of this scope.
            {
                jive::ScopeFlag ignoreCount(this->ignoreCount_);

                auto deferCount = pex::MakeDefer(this->count);
                deferCount.Set(newCount);

                if (wasSelected)
                {
                    this->selected.Set({});
                }

                this->ClearInvalidatedEndpoints_(index);

                this->items_.insert(
                    this->items_.begin() + index,
                    std::make_unique<ListItem>());

                if constexpr (HasGetVirtual<ListItem>)
                {
                    PEX_MEMBER_ADDRESS(
                        this->items_.at(index)->GetVirtual(),
                        fmt::format("item {}", index));
                }
                else
                {
                    PEX_MEMBER_ADDRESS(
                        this->items_.at(index).get(),
                        fmt::format("item {}", index));
                }

                this->items_.at(index)->Set(item);
                this->RestoreBaseEndpoints_(index);
                this->selectionReceived_ = false;
                this->internalMemberAdded_.Set(index);
                this->memberAdded.Set(index);
            }

            if (wasSelected && !this->selectionReceived_)
            {
                // Nothing changed the selection in response to memberAdded.
                if (*wasSelected < index)
                {
                    // The selected item was before the insertion point.
                    this->selected.Set(wasSelected);
                }
                else
                {
                    // The selected item was after the insertion point.
                    this->selected.Set(*wasSelected + 1);
                }
            }
        }

    protected:
        void Remove_(size_t index)
        {
            this->memberWillRemove.Set(index);

            // Allow the internal controls to respond to memberWillRemove after
            // outside observers have disconnected.
            this->internalMemberWillRemove_.Set(index);

            assert(this->baseCreatedEndpoints_.size() <= index);

            jive::SafeErase(this->items_, index);

            detail::AccessReference(this->count)
                .SetWithoutNotify(this->items_.size());

            this->memberRemoved.Set(index);
        }

    public:
        void Erase(size_t index)
        {
            assert(index < this->items_.size());

            auto scopedFlag = ScopedListFlag(this->isNotifying);

            auto selected_ = this->selected.Get();

            if (selected_ && *selected_ == index)
            {
                this->selected.Set({});
            }

            size_t newCount = this->count.Get() - 1;

            jive::ScopeFlag ignoreCount(this->ignoreCount_);
            auto deferCount = MakeDefer(this->count);
            deferCount.Set(newCount);

            this->ClearInvalidatedEndpoints_(index);

            this->Remove_(index);

            this->RestoreBaseEndpoints_(index);
        }

        void ResizeWithoutNotify(size_t newSize)
        {
            if (newSize == this->items_.size())
            {
                assert(this->count.Get() == newSize);
                return;
            }

            auto wasSelected = this->selected.Get();
            this->selected.Set({});

            if (newSize < this->items_.size())
            {
                this->ClearInvalidatedEndpoints_(newSize);
                this->ReduceCount_(newSize);
            }
            else
            {
                size_t toInitialize = newSize - this->items_.size();
                size_t firstToRestore = this->items_.size();
                this->selectionReceived_ = false;

                while (toInitialize--)
                {
                    this->items_.push_back(std::make_unique<ListItem>());

                    size_t currentSize = this->items_.size();

                    detail::AccessReference(this->count)
                        .SetWithoutNotify(currentSize);

                    size_t newIndex = currentSize - 1;

                    if constexpr (HasGetVirtual<ListItem>)
                    {
                        PEX_MEMBER_ADDRESS(
                            this->items_.back()->GetVirtual(),
                            fmt::format("item {}", newIndex));
                    }
                    else
                    {
                        PEX_MEMBER_ADDRESS(
                            this->items_.back().get(),
                            fmt::format("item {}", newIndex));
                    }

                    this->internalMemberAdded_.Set(newIndex);
                    this->memberAdded.Set(newIndex);
                }

                this->RestoreBaseEndpoints_(firstToRestore);
            }

            if (
                !this->selectionReceived_
                && wasSelected
                && *wasSelected < newSize)
            {
                // Nothing changed the selection in response to memberAdded.
                this->selected.Set(wasSelected);
            }
        }

    private:
        void DoNotify_()
        {
            // Ignore all notifications from list members.
            // At the end of this scope, a signal notification may be sent by
            // ListConnect.
            auto scopeMute = detail::ScopeMute<Model>(*this, false);

            auto scopedFlag = ScopedListFlag(this->isNotifying);

            for (auto &item: this->items_)
            {
                detail::AccessReference(*item).DoNotify();
            }

            // We don't want count echoed back to us here.
            jive::ScopeFlag ignoreCount(this->ignoreCount_);
            detail::AccessReference(this->count).DoNotify();
        }

        void SetWithoutNotify_(const Type &values)
        {
            // Mute while setting item values.
            // Set isSilenced option to 'true' so nothing is notified when
            // ScopeMute is destroyed.
            auto scopeMute = detail::ScopeMute<Model>(*this, true);
            bool countChanged = false;
            auto wasSelected = this->selected.Get();
            auto valueCount = values.size();

            if (valueCount != this->items_.size())
            {
                countChanged = true;

                this->selected.Set({});

                this->selectionReceived_ = false;

                if (valueCount < this->items_.size())
                {
                    this->ClearInvalidatedEndpoints_(valueCount);
                    this->ReduceCount_(valueCount);

                    // Set all of the new values.
                    for (size_t index = 0; index < valueCount; ++index)
                    {
                        detail::AccessReference(*this->items_[index])
                            .SetWithoutNotify(values[index]);
                    }
                }
                else
                {
                    // Set the new values we already have items_ for.
                    for (size_t index = 0; index < this->items_.size(); ++index)
                    {
                        detail::AccessReference(*this->items_[index])
                            .SetWithoutNotify(values[index]);
                    }

                    size_t firstToRestore = this->items_.size();

                    // Create and set the new items.
                    size_t toInitialize = valueCount - this->items_.size();

                    while (toInitialize--)
                    {
                        // Create, set, and notify member added for each new
                        // item.
                        this->items_.push_back(std::make_unique<ListItem>());

                        size_t currentSize = this->items_.size();

                        detail::AccessReference(this->count)
                            .SetWithoutNotify(currentSize);

                        size_t newIndex = currentSize - 1;

                        if constexpr (HasGetVirtual<ListItem>)
                        {
                            PEX_MEMBER_ADDRESS(
                                this->items_.back()->GetVirtual(),
                                fmt::format("item {}", newIndex));
                        }
                        else
                        {
                            PEX_MEMBER_ADDRESS(
                                this->items_.back().get(),
                                fmt::format("item {}", newIndex));
                        }

                        detail::AccessReference(*this->items_[newIndex])
                            .SetWithoutNotify(values[newIndex]);

                        // Don't call member added until the new value has been
                        // set.
                        this->internalMemberAdded_.Set(newIndex);
                        this->memberAdded.Set(newIndex);
                    }

                    this->RestoreBaseEndpoints_(firstToRestore);
                }
            }
            else
            {
                // No size changes.
                // Set all values.
                for (size_t index = 0; index < valueCount; ++index)
                {
                    detail::AccessReference(*this->items_[index])
                        .SetWithoutNotify(values[index]);
                }
            }

            assert(this->items_.size() == valueCount);

            // Reselect value if it is still in the list.
            if (countChanged && !this->selectionReceived_)
            {
                // memberAdded listeners did not make a selection.
                // Restore the previous selection.
                if (wasSelected && *wasSelected < valueCount)
                {
                    this->selected.Set(wasSelected);
                }
            }
        }

        void ReduceCount_(size_t count_)
        {
            assert(count_ < this->items_.size());

            // This is a reduction in size.
            // No new elements need to be created.
            for (size_t i = this->items_.size(); i > count_; --i)
            {
                // Call Erase with each removed index.
                // Calls memberWillRemove/memberRemoved for each item.
                this->Remove_(i - 1);
            }
        }

        void OnCount_(size_t count_)
        {
            if (this->ignoreCount_)
            {
                return;
            }

            if (count_ == this->items_.size())
            {
                assert(count_ == this->count.Get());

                return;
            }

            auto scopedFlag = ScopedListFlag(this->isNotifying);

            auto wasSelected = this->selected.Get();
            this->selected.Set({});

            // count must match the current actual size of items_ while we make
            // adjustments.
            detail::AccessReference(this->count)
                .SetWithoutNotify(this->items_.size());

            if (count_ < this->items_.size())
            {
                this->ClearInvalidatedEndpoints_(count_);
                this->ReduceCount_(count_);
            }
            else
            {
                size_t toInitialize = count_ - this->items_.size();
                size_t firstToRestore = this->items_.size();

                this->selectionReceived_ = false;

                while (toInitialize--)
                {
                    this->items_.push_back(std::make_unique<ListItem>());

                    size_t currentSize = this->items_.size();

                    detail::AccessReference(this->count)
                        .SetWithoutNotify(currentSize);

                    size_t newIndex = currentSize - 1;

                    if constexpr (HasGetVirtual<ListItem>)
                    {
                        PEX_MEMBER_ADDRESS(
                            this->items_.back()->GetVirtual(),
                            fmt::format("item {}", newIndex));
                    }
                    else
                    {
                        PEX_MEMBER_ADDRESS(
                            this->items_.back().get(),
                            fmt::format("item {}", newIndex));
                    }

                    this->internalMemberAdded_.Set(newIndex);
                    this->memberAdded.Set(newIndex);
                }

                this->RestoreBaseEndpoints_(firstToRestore);
            }

            if (
                !this->selectionReceived_
                && wasSelected
                && *wasSelected < count_)
            {
                this->selected.Set(wasSelected);
            }
        }

        void OnSelected_([[maybe_unused]] const std::optional<size_t> &index)
        {
#ifndef NDEBUG
            if (index)
            {
                assert(*index < this->items_.size());
            }
#endif
            this->selectionReceived_ = true;
        }

        void OnBaseWillDelete_(size_t index)
        {
            this->memberWillReplace.Set(index);
            this->internalMemberWillReplace_.Set(index);
        }

        void OnBaseCreated_(size_t index)
        {
            this->internalMemberReplaced_.Set(index);
            this->memberReplaced.Set(index);
        }

        void RestoreBaseEndpoint_(size_t index)
        {
            static_assert(HasBaseSignals<ListItem>);
            assert(index == this->baseCreatedEndpoints_.size());
            assert(index == this->baseWillDeleteEndpoints_.size());

            this->baseWillDeleteEndpoints_.emplace_back(
                this,
                this->items_.at(index)->GetBaseWillDelete(),
                [index](void *context)
                {
                    static_cast<Model *>(context)->OnBaseWillDelete_(index);
                });

            this->baseCreatedEndpoints_.emplace_back(
                this,
                this->items_.at(index)->GetBaseCreated(),
                [index](void *context)
                {
                    static_cast<Model *>(context)->OnBaseCreated_(index);
                });
        }

        void RestoreBaseEndpoints_([[maybe_unused]] size_t firstToRestore)
        {
            if constexpr (HasBaseSignals<ListItem>)
            {
                size_t listCount = this->items_.size();

                for (size_t index = firstToRestore; index < listCount; ++index)
                {
                    this->RestoreBaseEndpoint_(index);
                }
            }
        }

        void ClearInvalidatedEndpoints_([[maybe_unused]] size_t firstToClear)
        {
            if constexpr (HasBaseSignals<ListItem>)
            {
                assert(this->baseWillDeleteEndpoints_.size() > firstToClear);
                assert(this->baseCreatedEndpoints_.size() > firstToClear);

                this->baseWillDeleteEndpoints_.resize(firstToClear);
                this->baseCreatedEndpoints_.resize(firstToClear);
            }
        }

    private:

        using BaseActionTerminus =
            ::pex::Terminus<void, pex::control::Signal<>>;

        std::vector<std::unique_ptr<ListItem>> items_;
        ::pex::Terminus<Model, Count> countTerminus_;
        ::pex::Terminus<Model, Selected> selectedTerminus_;
        bool selectionReceived_;

        MemberWillRemove internalMemberWillRemove_;
        MemberAdded internalMemberAdded_;
        MemberWillReplace internalMemberWillReplace_;
        MemberReplaced internalMemberReplaced_;

        EndpointVector<BaseActionTerminus> baseWillDeleteEndpoints_;
        EndpointVector<BaseActionTerminus> baseCreatedEndpoints_;
    };


    class Control: public detail::Mute
    {

    public:
        static constexpr bool isListControl = true;
        static constexpr auto observerName = "pex::List::Control";

        using Access = GetAndSetTag;

        using Upstream = Model;
        using Type = typename Upstream::Type;
        using Item = typename Upstream::Item;
        using ListItem = ControlSelector<Member>;
        using Count = ::pex::control::ListCount;
        using Selected = ::pex::control::ListOptionalIndex;
        using MemberAdded = ::pex::control::ListOptionalIndex;
        using MemberWillRemove = ::pex::control::ListOptionalIndex;
        using MemberRemoved = ::pex::control::ListOptionalIndex;
        using MemberWillReplace = ::pex::control::ListOptionalIndex;
        using MemberReplaced = ::pex::control::ListOptionalIndex;
        using ListFlag = ::pex::control::ListFlag;

        using MemberWillRemoveTerminus =
            ::pex::Terminus<Control, MemberWillRemove>;

        using MemberAddedTerminus = ::pex::Terminus<Control, MemberAdded>;

        using MemberWillReplaceTerminus =
            ::pex::Terminus<Control, MemberWillReplace>;

        using MemberReplacedTerminus = ::pex::Terminus<Control, MemberReplaced>;

        using Vector = std::vector<ListItem>;
        using Iterator = typename Vector::iterator;
        using ConstIterator = typename Vector::const_iterator;
        using ReverseIterator = typename Vector::reverse_iterator;
        using ConstReverseIterator = typename Vector::const_reverse_iterator;

        using Defer = DeferList<Member, ControlSelector, Control>;

        static_assert(IsControl<ListItem> || IsGroupControl<ListItem>);

        template<typename>
        friend class ::pex::Reference;

        template<typename, typename>
        friend class ::pex::detail::ListConnect;

        template<typename, typename>
        friend class ::pex::detail::ListConnect;

        Count count;
        Selected selected;
        MemberAdded memberAdded;
        MemberWillRemove memberWillRemove;
        MemberRemoved memberRemoved;
        MemberWillReplace memberWillReplace;
        MemberReplaced memberReplaced;
        ListFlag isNotifying;

        Control()
            :
            detail::Mute(),
            count(),
            selected(),
            memberAdded(),
            memberWillRemove(),
            memberRemoved(),
            memberWillReplace(),
            memberReplaced(),
            isNotifying(),
            upstream_(nullptr),
            memberWillRemoveTerminus_(),
            memberAddedTerminus_(),
            memberWillReplaceTerminus_(),
            memberReplacedTerminus_(),
            items_()
        {
            PEX_NAME(
                fmt::format(
                    "pex::List<{}>::Control",
                    jive::GetTypeName<Member>()));
        }

        Control(Upstream &upstream)
            :
            detail::Mute(upstream.CloneMuteControl()),
            count(upstream.count),
            selected(upstream.selected),
            memberAdded(upstream.memberAdded),
            memberWillRemove(upstream.memberWillRemove),
            memberRemoved(upstream.memberRemoved),
            memberWillReplace(upstream.memberWillReplace),
            memberReplaced(upstream.memberReplaced),
            isNotifying(upstream.isNotifying),
            upstream_(&upstream),

            memberWillRemoveTerminus_(

                PEX_THIS(
                    fmt::format(
                        "pex::List<{}>::Control",
                        jive::GetTypeName<Member>())),

                this->upstream_->internalMemberWillRemove_,
                &Control::OnMemberWillRemove_),

            memberAddedTerminus_(
                this,
                this->upstream_->internalMemberAdded_,
                &Control::OnMemberAdded_),

            memberWillReplaceTerminus_(
                this,
                this->upstream_->internalMemberWillReplace_,
                &Control::OnMemberWillReplace_),

            memberReplacedTerminus_(
                this,
                this->upstream_->internalMemberReplaced_,
                &Control::OnMemberReplaced_),

            items_()
        {
            assert(this->upstream_ != nullptr);

            for (size_t index = 0; index < this->count.Get(); ++index)
            {
                this->items_.emplace_back((*this->upstream_)[index]);
            }
        }

        Control(const Control &other)
            :
            detail::Mute(other),
            count(other.count),
            selected(other.selected),
            memberAdded(other.memberAdded),
            memberWillRemove(other.memberWillRemove),
            memberRemoved(other.memberRemoved),
            memberWillReplace(other.memberWillReplace),
            memberReplaced(other.memberReplaced),
            isNotifying(other.isNotifying),
            upstream_(other.upstream_),

            memberWillRemoveTerminus_(

                PEX_THIS(
                    fmt::format(
                        "pex::List<{}>::Control",
                        jive::GetTypeName<Member>())),

                this->upstream_->internalMemberWillRemove_,
                &Control::OnMemberWillRemove_),

            memberAddedTerminus_(
                this,
                this->upstream_->internalMemberAdded_,
                &Control::OnMemberAdded_),

            memberWillReplaceTerminus_(
                this,
                this->upstream_->internalMemberWillReplace_,
                &Control::OnMemberWillReplace_),

            memberReplacedTerminus_(
                this,
                this->upstream_->internalMemberReplaced_,
                &Control::OnMemberReplaced_),

            items_(other.items_)
        {
            assert(other.upstream_ != nullptr);
            assert(this->memberAddedTerminus_.HasModel());

            PEX_NAME(
                fmt::format(
                    "pex::List<{}>::Control",
                    jive::GetTypeName<Member>()));

            REGISTER_ITEMREF_NAMES(this, this->items_);
        }

        ~Control()
        {
            CLEAR_ITEMREF_NAMES(this->items_);
            PEX_CLEAR_NAME(this);
        }

        Control & operator=(const Control &other)
        {
            this->detail::Mute::operator=(other);
            this->count = other.count;
            this->selected = other.selected;
            this->memberAdded = other.memberAdded;
            this->memberWillRemove = other.memberWillRemove;
            this->memberRemoved = other.memberRemoved;
            this->memberWillReplace = other.memberWillReplace;
            this->memberReplaced = other.memberReplaced;
            this->isNotifying = other.isNotifying;
            this->upstream_ = other.upstream_;

            if (other.HasModel())
            {
                assert(other.memberWillRemoveTerminus_.HasModel());
                assert(other.memberWillRemoveTerminus_.HasConnection());
                assert(other.memberAddedTerminus_.HasModel());
                assert(other.memberAddedTerminus_.HasConnection());
                assert(other.memberWillReplaceTerminus_.HasModel());
                assert(other.memberWillReplaceTerminus_.HasConnection());
                assert(other.memberReplacedTerminus_.HasModel());
                assert(other.memberReplacedTerminus_.HasConnection());
            }

            this->memberWillRemoveTerminus_.RequireAssign(
                this,
                other.memberWillRemoveTerminus_);

            this->memberAddedTerminus_.RequireAssign(
                this,
                other.memberAddedTerminus_);

            this->memberWillReplaceTerminus_.RequireAssign(
                this,
                other.memberWillReplaceTerminus_);

            this->memberReplacedTerminus_.RequireAssign(
                this,
                other.memberReplacedTerminus_);

            if (this->HasModel())
            {
                assert(this->memberWillRemoveTerminus_.HasModel());
                assert(this->memberWillRemoveTerminus_.HasConnection());
                assert(this->memberAddedTerminus_.HasModel());
                assert(this->memberAddedTerminus_.HasConnection());
                assert(this->memberWillReplaceTerminus_.HasModel());
                assert(this->memberWillReplaceTerminus_.HasConnection());
                assert(this->memberReplacedTerminus_.HasModel());
                assert(this->memberReplacedTerminus_.HasConnection());
            }

            CLEAR_ITEMREF_NAMES(this->items_);

            this->items_ = other.items_;

            REGISTER_ITEMREF_NAMES(this, this->items_);

            return *this;
        }

        void EraseSelected()
        {
            assert(this->upstream_);
            this->upstream_->EraseSelected();
        }

        // Implement a std::vector-like interface.
        const ListItem & operator[](size_t index) const
        {
            return this->items_[index];
        }

        ListItem & operator[](size_t index)
        {
            return this->items_[index];
        }

        const ListItem & at(size_t index) const
        {
            return this->items_.at(index);
        }

        ListItem & at(size_t index)
        {
            return this->items_.at(index);
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

        size_t size() const
        {
            return this->items_.size();
        }

        bool empty() const
        {
            return this->items_.empty();
        }

        Type Get() const
        {
            return this->upstream_->Get();
        }

        void Set(const Type &values)
        {
            this->upstream_->Set(values);
        }

        bool HasModel() const
        {
            if (!this->upstream_)
            {
                return false;
            }

            if (!this->count.HasModel())
            {
                return false;
            }

            if (!this->selected.HasModel())
            {
                return false;
            }

            if (!this->memberAdded.HasModel())
            {
                return false;
            }

            if (!this->memberWillRemove.HasModel())
            {
                return false;
            }

            if (!this->memberWillReplace.HasModel())
            {
                return false;
            }

            if (!this->memberReplaced.HasModel())
            {
                return false;
            }

            if (!this->isNotifying.HasModel())
            {
                return false;
            }

            for (auto &item: this->items_)
            {
                if (!item.HasModel())
                {
                    return false;
                }
            }

            return true;
        }

        template<typename Derived>
        std::optional<size_t> Append(const Derived &item)
        {
            // TODO: The user asked to Append an item, but forgot to connect
            // their control to the model?
            // This feels like it should be a hard error.
            if (!this->upstream_)
            {
                return {};
            }

            return this->upstream_->Append(item);
        }

    private:
        void DoNotify_()
        {
#ifndef NDEBUG
            if (!this->upstream_)
            {
                throw std::logic_error("List::Control is uninitialized");
            }
#endif
            this->upstream_->DoNotify_();
        }

        void SetWithoutNotify_(const Type &values)
        {
#ifndef NDEBUG
            if (!this->upstream_)
            {
                throw std::logic_error("List::Control is uninitialized");
            }
#endif
            this->upstream_->SetWithoutNotify_(values);
        }

        void OnMemberWillRemove_(const std::optional<size_t> &index)
        {
            if (!index)
            {
                return;
            }

            jive::SafeErase(this->items_, *index);
        }

        void OnMemberAdded_(const std::optional<size_t> &index)
        {
            if (!index)
            {
                return;
            }

            this->items_.emplace(
                jive::SafeInsertIterator(this->items_, *index),
                (*this->upstream_)[*index]);

#ifdef ENABLE_PEX_NAMES
            if constexpr (HasGetVirtual<ListItem>)
            {
                pex::PexName(
                    this->items_.at(*index).GetVirtual(),
                    this,
                    fmt::format("item {}", *index));
            }
            else
            {
                pex::PexName(
                    &this->items_.at(*index),
                    this,
                    fmt::format("item {}", *index));
            }
#endif
        }

        void OnMemberWillReplace_(const std::optional<size_t> &index)
        {
            if (!index)
            {
                return;
            }

            this->items_.at(*index) = ListItem();
        }

        void OnMemberReplaced_(const std::optional<size_t> &index)
        {
            if (!index)
            {
                return;
            }

            this->items_.at(*index) = ListItem((*this->upstream_)[*index]);
        }

    private:
        Upstream *upstream_;
        MemberWillRemoveTerminus memberWillRemoveTerminus_;
        MemberAddedTerminus memberAddedTerminus_;
        MemberWillReplaceTerminus memberWillReplaceTerminus_;
        MemberReplacedTerminus memberReplacedTerminus_;
        Vector items_;
    };
};


} // end namespace pex


#if 0
namespace std
{

template<typename Upstream, typename Control>
typename pex::control::List<Upstream, Control>::Iterator
begin(pex::control::List<Upstream, Control> &listControl)
{
    return listControl.begin();
}

template<typename Upstream, typename Control>
typename pex::control::List<Upstream, Control>::Iterator
end(pex::control::List<Upstream, Control> &listControl)
{
    return listControl.end();
}

template<typename Upstream, typename Control>
typename pex::control::List<Upstream, Control>::ConstIterator
begin(const pex::control::List<Upstream, Control> &listControl)
{
    return listControl.begin();
}

template<typename Upstream, typename Control>
typename pex::control::List<Upstream, Control>::ConstIterator
end(const pex::control::List<Upstream, Control> &listControl)
{
    return listControl.end();
}

} // end namespace std

#endif
