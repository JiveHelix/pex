#pragma once


#include <fields/fields.h>
#include "pex/group.h"
#include "pex/reference.h"
#include "pex/endpoint.h"
#include "pex/traits.h"
#include "pex/indexed_map.h"


#if defined(__GNUC__) && ((__GNUC__ == 12) || (__GNUC__ == 13))
    #define SUPPRESS_GCC_NULL_DEREF_WARNING_BEGIN() \
        _Pragma("GCC diagnostic push")              \
        _Pragma("GCC diagnostic ignored \"-Wnull-dereference\"")

    #define SUPPRESS_GCC_NULL_DEREF_WARNING_END() \
        _Pragma("GCC diagnostic pop")
#else
    #define SUPPRESS_GCC_NULL_DEREF_WARNING_BEGIN()
    #define SUPPRESS_GCC_NULL_DEREF_WARNING_END()
#endif


namespace pex
{


template<typename T>
struct OrderFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::moveToTop, "moveToTop"),
        fields::Field(&T::moveUp, "moveUp"),
        fields::Field(&T::moveDown, "moveDown"),
        fields::Field(&T::moveToBottom, "moveToBottom"));
};


template<template<typename> typename T>
class OrderTemplate
{
public:
    T<MakeSignal> moveToTop;
    T<MakeSignal> moveUp;
    T<MakeSignal> moveDown;
    T<MakeSignal> moveToBottom;

    static constexpr auto fields =
        OrderFields<OrderTemplate>::fields;

    static constexpr auto fieldsTypeName = "Order";
};


using OrderGroup = Group<OrderFields, OrderTemplate>;
using OrderModel = typename OrderGroup::Model;
using OrderControl = typename OrderGroup::Control;
using Order = typename OrderGroup::Plain;


template<typename T>
struct OrderedListFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::indices, "indices"),
        fields::Field(&T::reorder, "reorder"),
        fields::Field(&T::list, "list"));
};


using IndicesList = std::vector<size_t>;


template<typename ListMaker>
struct OrderedListTemplate
{
    template<template<typename> typename T>
    struct Template
    {
        T<IndicesList> indices;
        T<pex::MakeSignal> reorder;
        T<ListMaker> list;

        static constexpr auto fields = OrderedListFields<Template>::fields;
        static constexpr auto fieldsTypeName = "OrderedList";
    };
};


namespace detail
{


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
concept IsOrder =
    std::is_same_v<std::remove_reference_t<T>, OrderControl>
    || std::is_same_v<std::remove_reference_t<T>, OrderModel>;


template<typename T>
concept HasVirtualGetOrder = requires(T t)
{
    { t.GetVirtual()->GetOrder() } -> IsOrder;
};


template<typename T>
concept HasOrderMember = requires (T t)
{
    { t.order } -> IsOrder;
};


template<typename T>
concept HasOrder =
    HasOrderMember<T> || HasVirtualGetOrder<T>;


template<typename ListMaker>
concept ListHasOrderMember =
    HasOrderMember<ListControlItem<ListMaker>>
    && HasOrderMember<ListModelItem<ListMaker>>;


template<typename ListMaker>
concept ListHasVirtualGetOrder =
    HasVirtualGetOrder<ListControlItem<ListMaker>>
    && HasVirtualGetOrder<ListModelItem<ListMaker>>;


template<typename ListMaker>
concept ListHasOrder =
    ListHasOrderMember<ListMaker> || ListHasVirtualGetOrder<ListMaker>;


template<typename ListMaker>
std::optional<OrderControl> GetOrderControl(
    ModelSelector<ListMaker> &list,
    size_t storageIndex)
{
    if constexpr (ListHasOrderMember<ListMaker>)
    {
        return OrderControl(list.at(storageIndex).order);
    }
    else if constexpr (ListHasVirtualGetOrder<ListMaker>)
    {
        auto virtualItem = list.at(storageIndex).GetVirtual();

        if (!virtualItem)
        {
            return std::nullopt;
        }

        return OrderControl(virtualItem->GetOrder());
    }
    else
    {
        return std::nullopt;
    }
}


template<typename ListMaker>
struct OrderedListCustom
{
    template<typename Base>
    class Model
        :
        public Base
    {
    public:
        static constexpr bool hasOrder = ListHasOrder<ListMaker>;

        using Selected = control::ListOptionalIndex;
        using MemberAdded = control::ListOptionalIndex;
        using MemberRemoved = control::ListOptionalIndex;
        using Count = control::ListCount;

    private:
        using CountEndpoint = Endpoint<Model, Count>;
        using MemberRemovedEndpoint = Endpoint<Model, MemberRemoved>;
        using MemberAddedEndpoint = Endpoint<Model, MemberAdded>;

        MemberAddedEndpoint memberAddedEndpoint_;
        MemberRemovedEndpoint memberWillRemoveEndpoint_;
        MemberRemovedEndpoint memberRemovedEndpoint_;

    public:
        Selected selected;
        Count count;
        MemberAdded memberAdded;
        MemberRemoved memberWillRemove;
        MemberRemoved memberRemoved;

        using Indices = ModelSelector<IndicesList>;

        using List = decltype(Base::list);
        using ListItem = typename List::ListItem;

        Model()
            :
            Base(),

            memberAddedEndpoint_(
                PEX_THIS("OrderedList::Model"),
                this->list.memberAdded,
                &Model::OnListMemberAdded_),

            memberWillRemoveEndpoint_(
                this,
                this->list.memberWillRemove,
                &Model::OnListMemberWillRemove_),

            memberRemovedEndpoint_(
                this,
                this->list.memberRemoved,
                &Model::OnListMemberRemoved_),

            selected(this->list.selected),
            count(this->list.count),
            memberAdded(this->list.memberAdded),
            memberWillRemove(this->list.memberWillRemove),
            memberRemoved(this->list.memberRemoved),
            moveToTopEndpoints_(),
            moveUpEndpoints_(),
            moveDownEndpoints_(),
            moveToBottomEndpoints_(),
            indicesEndpoint_(
                this,
                this->indices,
                &Model::OnIndices_)
        {
            PEX_MEMBER(memberAddedEndpoint_);

            this->RestoreConnections_(0);

            assert(this->list.count.Get() == this->indices.Get().size());
            assert(this->list.Get().size() == this->list.count.Get());
        }

        const ListItem & operator[](size_t index) const
        {
            return this->list[this->indices[index]];
        }

        ListItem & operator[](size_t index)
        {
            return this->list[this->indices[index]];
        }

        const ListItem & at(size_t index) const
        {
            return this->list.at(this->indices.at(index));
        }

        ListItem & at(size_t index)
        {
            return this->list.at(this->indices.at(index));
        }

        const ListItem & GetUnordered(size_t index) const
        {
            return this->list.at(index);
        }

        ListItem & GetUnordered(size_t index)
        {
            return this->list.at(index);
        }

        size_t GetStorageIndex(size_t orderedIndex) const
        {
            return this->indices.at(orderedIndex);
        }

        size_t GetOrderedIndex(size_t storageIndex)
        {
            auto asList = this->indices.Get();
            auto end = std::end(asList);

            auto found = std::find(
                std::begin(asList),
                end,
                storageIndex);

            if (found == end)
            {
                throw std::out_of_range("storage index not found");
            }

            return *found;
        }

        // TODO Prepend, Append, Insert should follow the order found in
        // indices.
        // Currently, they are pass-through plumbing to the underlying list.
        template<typename Derived>
        void Prepend(const Derived &item)
        {
            auto newIndex = this->list.Append(item);
            this->MoveToTop(newIndex);
        }

        template<typename Derived>
        size_t Append(const Derived &item)
        {
            auto newIndex = this->list.Append(item);

            return newIndex;
        }

        template<typename Derived>
        void Insert(size_t index, const Derived &item)
        {
            this->list.Insert(index, item);
        }

        void Set(const typename List::Type &listType)
        {
            this->list.Set(listType);
        }

        void MoveToBottom(size_t storageIndex)
        {
            // Find the requested storageIndex in the indices list.
            auto orderedIndices = this->indices.Get();

            if (orderedIndices.size() < 2)
            {
                return;
            }

            auto found = std::find(
                std::begin(orderedIndices),
                std::end(orderedIndices),
                storageIndex);

            auto end = std::end(orderedIndices);

            if (found == end)
            {
                // Not in the list.
                throw std::out_of_range("Index not in list");
            }

            --end;

            if (found == end)
            {
                // Already at the end of the list.
                return;
            }

            auto index = *found;
            orderedIndices.erase(found);
            orderedIndices.push_back(index);

            this->indices.Set(orderedIndices);
        }

        void MoveToTop(size_t storageIndex)
        {
            // Find the requested storageIndex in the indices list.
            auto orderedIndices = this->indices.Get();

            if (orderedIndices.size() < 2)
            {
                return;
            }

            auto found = std::find(
                std::begin(orderedIndices),
                std::end(orderedIndices),
                storageIndex);

            if (found == std::end(orderedIndices))
            {
                // Not in the list.
                throw std::out_of_range("Index not in list");
            }

            if (found == std::begin(orderedIndices))
            {
                // Already at the beginning of the list.
                return;
            }

            auto index = *found;
            orderedIndices.erase(found);
            SUPPRESS_GCC_NULL_DEREF_WARNING_BEGIN()
            orderedIndices.insert(std::begin(orderedIndices), index);
            SUPPRESS_GCC_NULL_DEREF_WARNING_END()

            this->indices.Set(orderedIndices);
        }

        void MoveDown(size_t storageIndex)
        {
            // Find the requested storageIndex in the indices list.
            auto orderedIndices = this->indices.Get();

            auto found = std::find(
                std::begin(orderedIndices),
                std::end(orderedIndices),
                storageIndex);

            auto end = std::end(orderedIndices);

            if (found == end)
            {
                // Not in the list.
                throw std::out_of_range("Index not in list");
            }

            --end;

            if (found == end)
            {
                // Already at the bottom of the list.
                return;
            }

            auto target = found;
            ++target;
            std::swap(*target, *found);
            this->indices.Set(orderedIndices);
        }

        void MoveUp(size_t storageIndex)
        {
            // Find the requested storageIndex in the indices list.
            auto orderedIndices = this->indices.Get();

            auto found = std::find(
                std::begin(orderedIndices),
                std::end(orderedIndices),
                storageIndex);

            if (found == std::end(orderedIndices))
            {
                // Not in the list.
                throw std::out_of_range("Index not in list");
            }

            if (found == std::begin(orderedIndices))
            {
                // Already at the top of the list.
                return;
            }

            auto target = found;
            --target;
            std::swap(*target, *found);
            this->indices.Set(orderedIndices);
        }

        template<typename T>
        void AssignItem(size_t index, T &&item)
        {
            auto storageIndex = this->indices.at(index).Get();

            // Clear the move endpoints before possibly deleting the list
            // member they are tracking
            this->moveToTopEndpoints_.erase(storageIndex);
            this->moveUpEndpoints_.erase(storageIndex);
            this->moveDownEndpoints_.erase(storageIndex);
            this->moveToBottomEndpoints_.erase(storageIndex);

            this->list.at(storageIndex).Set(std::forward<T>(item));

            if constexpr (hasOrder)
            {
                auto order = GetOrderControl<ListMaker>(this->list, index);
                assert(order);
                this->MakeOrderConnections_(*order, storageIndex);
            }
        }

        void EraseSelected()
        {
            this->list.EraseSelected();
        }

        void Erase(size_t index)
        {
            this->list.Erase(index);
        }

        size_t size() const
        {
            return this->list.size();
        }

        bool empty() const
        {
            return this->list.empty();
        }

    private:
        void OnIndices_(const std::vector<size_t> &)
        {
            this->reorder.Trigger();
        }

        void MakeOrderConnections_(
            OrderControl &order,
            size_t storageIndex)
        {
            auto result = this->moveToTopEndpoints_.try_emplace(
                storageIndex,
                this,
                order.moveToTop,
                &Model::MoveToTop,
                storageIndex);

            assert(result.second);

            result = this->moveUpEndpoints_.try_emplace(
                storageIndex,
                this,
                order.moveUp,
                &Model::MoveUp,
                storageIndex);

            assert(result.second);

            result = this->moveDownEndpoints_.try_emplace(
                storageIndex,
                this,
                order.moveDown,
                &Model::MoveDown,
                storageIndex);

            assert(result.second);

            result = this->moveToBottomEndpoints_.try_emplace(
                storageIndex,
                this,
                order.moveToBottom,
                &Model::MoveToBottom,
                storageIndex);

            assert(result.second);
        }

        void ClearInvalidatedConnections_(size_t firstToClear)
        {
            auto keys =
                GetInvalidatedKeys(
                    firstToClear,
                    this->moveToBottomEndpoints_);

#ifndef NDEBUG
            auto checkMoveToTop =
                GetInvalidatedKeys(firstToClear, this->moveToTopEndpoints_);

            auto checkMoveUp =
                GetInvalidatedKeys(firstToClear, this->moveUpEndpoints_);

            auto checkMoveDown =
                GetInvalidatedKeys(firstToClear, this->moveDownEndpoints_);

            assert(checkMoveToTop.size() == keys.size());
            assert(checkMoveUp.size() == keys.size());
            assert(checkMoveDown.size() == keys.size());

            std::sort(std::begin(keys), std::end(keys));
            std::sort(std::begin(checkMoveToTop), std::end(checkMoveToTop));
            std::sort(std::begin(checkMoveUp), std::end(checkMoveUp));
            std::sort(std::begin(checkMoveDown), std::end(checkMoveDown));

            for (size_t i = 0; i < keys.size(); ++i)
            {
                assert(keys[i] == checkMoveToTop[i]);
                assert(keys[i] == checkMoveUp[i]);
                assert(keys[i] == checkMoveDown[i]);
            }
#endif

            for (auto &key: keys)
            {
                this->moveToBottomEndpoints_.erase(key);
                this->moveToTopEndpoints_.erase(key);
                this->moveUpEndpoints_.erase(key);
                this->moveDownEndpoints_.erase(key);
            }

            ClearInvalidated(
                firstToClear,
                this->itemCreatedEndpoints_);
        }

        void OnItemCreated_(size_t index)
        {
            if constexpr (!hasOrder)
            {
                return;
            }

            auto order = GetOrderControl<ListMaker>(this->list, index);

            assert(order);

            this->MakeOrderConnections_(*order, index);
        }

        void RestoreConnection_(size_t index)
        {
            auto order =
                GetOrderControl<ListMaker>(this->list, index);

            if (order)
            {
                this->MakeOrderConnections_(*order, index);
            }
            else
            {
                if constexpr (ListHasVirtualGetOrder<ListMaker>)
                {
                    auto result = this->itemCreatedEndpoints_.try_emplace(
                        index,
                        this,
                        this->GetUnordered(index).GetBaseCreated(),
                        &Model::OnItemCreated_,
                        index);

                    assert(result.second);
                }
                else
                {
                    // order must exist already because there is no
                    // separate derived creation step.
                    assert(order);
                }
            }
        }

        void RestoreConnections_(size_t firstToRestore)
        {
            if constexpr (!hasOrder)
            {
                return;
            }

            assert(this->indices.size() == this->count.Get());

            size_t listCount = this->list.count.Get();

            for (
                size_t storageIndex = firstToRestore;
                storageIndex < listCount;
                ++storageIndex)
            {
                this->RestoreConnection_(storageIndex);
            }
        }

        void OnListMemberAdded_(const std::optional<size_t> &addedIndex)
        {
            if (!addedIndex)
            {
                return;
            }

            if (this->indices.size() == this->list.count.Get())
            {
                // We already have enough indices.
                // This happens this group is set directly, and indices is Set
                // before each of the items is added.
                // Adding each item generates a memberAdded notification, which
                // we can safely ignore in this instance.
                return;
            }

            auto added = *addedIndex;

            auto previous = this->indices.Get();

            std::transform(
                std::cbegin(previous),
                std::cend(previous),
                std::begin(previous),
                [added](size_t value)
                {
                    if (value >= added)
                    {
                        return value + 1;
                    }

                    return value;
                });

            previous.insert(
                previous.begin() + static_cast<ssize_t>(added),
                added);

            detail::AccessReference(this->indices).SetWithoutNotify(previous);
            assert(this->indices.size() == previous.size());
            this->ClearInvalidatedConnections_(added);
            this->RestoreConnections_(added);
        }

        void OnListMemberWillRemove_(const std::optional<size_t> &removedIndex)
        {
            if constexpr (!hasOrder)
            {
                return;
            }

            if (!removedIndex)
            {
                return;
            }

            this->ClearInvalidatedConnections_(*removedIndex);
        }

        void OnListMemberRemoved_(const std::optional<size_t> &removedIndex)
        {
            if (!removedIndex)
            {
                return;
            }

            auto removed = *removedIndex;

            auto previous = this->indices.Get();

            std::erase_if(
                previous,
                [removed](size_t index)
                {
                    return index == removed;
                });

            std::transform(
                std::cbegin(previous),
                std::cend(previous),
                std::begin(previous),
                [removed](size_t value)
                {
                    if (value > removed)
                    {
                        return value - 1;
                    }

                    return value;
                });

            detail::AccessReference(this->indices).SetWithoutNotify(previous);
            this->RestoreConnections_(removed);
        }

    private:

        using MoveOrderEndpoint =
            pex::BoundEndpoint
            <
                control::Signal<>,
                decltype(&Model::MoveUp)
            >;

        using ItemCreatedEndpoint =
            pex::BoundEndpoint
            <
                pex::control::Signal<>,
                decltype(&Model::OnItemCreated_)
            >;

        std::map<size_t, MoveOrderEndpoint> moveToTopEndpoints_;
        std::map<size_t, MoveOrderEndpoint> moveUpEndpoints_;
        std::map<size_t, MoveOrderEndpoint> moveDownEndpoints_;
        std::map<size_t, MoveOrderEndpoint> moveToBottomEndpoints_;
        std::map<size_t, ItemCreatedEndpoint> itemCreatedEndpoints_;

        using IndicesEndpoint = Endpoint<Model, decltype(Model::indices)>;

        IndicesEndpoint indicesEndpoint_;
    };


    template<typename List, typename Indices>
    class OrderedListIterator
    {
    public:
        using ListItem = detail::ListMember<List>;

        OrderedListIterator(
            List &list,
            const Indices &indices,
            size_t initialIndex)
            :
            list_(list),
            indices_(indices),
            index_(initialIndex)
        {
            assert(
                this->list_.size() == this->indices_.size());
        }

        ListItem & operator*()
        {
            return this->list_.at(
                size_t(this->indices_.at(this->index_)));
        }

        ListItem * operator->()
        {
            return &this->list_.at(
                size_t(this->indices_.at(this->index_)));
        }

        const ListItem & operator*() const
        {
            return this->list_.at(
                size_t(this->indices_.at(this->index_)));
        }

        const ListItem * operator->() const
        {
            return &this->list_.at(
                size_t(this->indices_.at(this->index_)));
        }

        // Prefix Increment
        OrderedListIterator & operator++()
        {
            ++this->index_;

            return *this;
        }

        // Postfix Increment
        OrderedListIterator operator++(int)
        {
            OrderedListIterator old = *this;
            this->operator++();

            return old;
        }

        // Prefix Decrement
        OrderedListIterator & operator--()
        {
            --this->index_;

            return *this;
        }

        // Postfix Decrement
        OrderedListIterator operator--(int)
        {
            OrderedListIterator old = *this;
            this->operator--();

            return old;
        }

        bool operator==(const OrderedListIterator &other) const
        {
            return this->index_ == other.index_;
        }

        bool operator!=(const OrderedListIterator &other) const
        {
            return this->index_ != other.index_;
        }

    private:
        List &list_;
        const Indices &indices_;
        size_t index_;
    };


    template<typename List, typename Indices>
    class ReverseOrderedListIterator
    {
    public:
        using ListItem = typename detail::ListMember<List>;

        ReverseOrderedListIterator(
            List &list,
            const Indices &indices,
            size_t initialIndex)
            :
            list_(list),
            indices_(indices),
            index_(initialIndex),
            count_(indices_.size())
        {
            assert(this->list_.size() == this->indices_.size());
        }

        ListItem & operator*()
        {
            return this->list_.at(
                size_t(this->indices_.at(this->count_ - this->index_ - 1)));
        }

        ListItem * operator->()
        {
            return &this->list_.at(
                size_t(this->indices_.at(this->count_ - this->index_ - 1)));
        }

        const ListItem & operator*() const
        {
            return this->list_.at(
                size_t(this->indices_.at(this->count_ - this->index_ - 1)));
        }

        const ListItem * operator->() const
        {
            return &this->list_.at(
                size_t(this->indices_.at(this->count_ - this->index_ - 1)));
        }

        // Prefix Increment
        ReverseOrderedListIterator & operator++()
        {
            ++this->index_;

            return *this;
        }

        // Postfix Increment
        ReverseOrderedListIterator operator++(int)
        {
            ReverseOrderedListIterator old = *this;
            this->operator++();

            return old;
        }

        // Prefix Decrement
        ReverseOrderedListIterator & operator--()
        {
            --this->index_;

            return *this;
        }

        // Postfix Decrement
        ReverseOrderedListIterator operator--(int)
        {
            ReverseOrderedListIterator old = *this;
            this->operator--();

            return old;
        }

        bool operator==(const ReverseOrderedListIterator &other) const
        {
            return this->index_ == other.index_;
        }

        bool operator!=(const ReverseOrderedListIterator &other) const
        {
            return this->index_ != other.index_;
        }

    private:
        List &list_;
        const Indices &indices_;
        size_t index_;
        size_t count_;
    };


    template<typename Derived, typename Base>
    class Iterable
    {
    public:
        using List = decltype(Base::list);
        using Indices = decltype(Base::indices);
        using ListItem = typename detail::ListMember<List>;

        using Iterator =
            OrderedListIterator<List, Indices>;

        using ReverseIterator =
            ReverseOrderedListIterator<List, Indices>;

        const ListItem & operator[](size_t index) const
        {
            auto self = this->GetDerived();

            // Convert to storageIndex using size_t. This works for both
            // control::Value indices and plain-old size_t indices.
            return self->list[size_t(self->indices[index])];
        }

        ListItem & operator[](size_t index)
        {
            auto self = this->GetDerived();

            return self->list[size_t(self->indices[index])];
        }

        const ListItem & at(size_t index) const
        {
            auto self = this->GetDerived();

            return self->list.at(size_t(self->indices.at(index)));
        }

        ListItem & at(size_t index)
        {
            auto self = this->GetDerived();

            return self->list.at(size_t(self->indices.at(index)));
        }

        const ListItem & GetUnordered(size_t index) const
        {
            return this->GetDerived()->list.at(index);
        }

        ListItem & GetUnordered(size_t index)
        {
            return this->GetDerived()->list.at(index);
        }

        Iterator begin()
        {
            auto self = this->GetDerived();

            return Iterator(self->list, self->indices, 0);
        }

        Iterator end()
        {
            auto self = this->GetDerived();

            return Iterator(
                self->list,
                self->indices,
                self->indices.size());
        }

        const Iterator begin() const
        {
            auto self = this->GetDerived();

            return Iterator(
                const_cast<List &>(self->list), self->indices, 0);
        }

        const Iterator end() const
        {
            auto self = this->GetDerived();

            return Iterator(
                const_cast<List &>(self->list),
                self->indices,
                self->indices.size());
        }

        ReverseIterator rbegin()
        {
            auto self = this->GetDerived();

            return ReverseIterator(self->list, self->indices, 0);
        }

        ReverseIterator rend()
        {
            auto self = this->GetDerived();

            return ReverseIterator(
                self->list,
                self->indices,
                self->indices.size());
        }

        const ReverseIterator rbegin() const
        {
            auto self = this->GetDerived();

            return ReverseIterator(
                const_cast<List &>(self->list), self->indices, 0);
        }

        const ReverseIterator rend() const
        {
            auto self = this->GetDerived();

            return ReverseIterator(
                const_cast<List &>(self->list),
                self->indices,
                self->indices.size());
        }

        size_t size() const
        {
            auto self = this->GetDerived();

            return self->list.size();
        }

        bool empty() const
        {
            auto self = this->GetDerived();

            return self->list.empty();
        }

    private:
        Derived * GetDerived()
        {
            return static_cast<Derived *>(this);
        }

        const Derived * GetDerived() const
        {
            return static_cast<const Derived *>(this);
        }
    };


    template<typename Base>
    class Control: public Base, public Iterable<Control<Base>, Base>
    {
    public:
        using List = decltype(Base::list);
        using Selected = typename List::Selected;
        using MemberAdded = control::ListOptionalIndex;
        using MemberRemoved = control::ListOptionalIndex;
        using Count = typename List::Count;
        using ListItem = typename List::ListItem;
        using Upstream = typename Base::Upstream;

        static_assert(
            std::is_same_v
            <
                typename Iterable<Control<Base>, Base>::ListItem,
                ListItem
            >);

        static_assert(IsValueContainer<decltype(Base::indices)>);

    private:
        // In group customizations, 'Upstream' is always the group's Model.
        Upstream *upstream_;

    public:
        Selected selected;
        Count count;
        MemberAdded memberAdded;
        MemberRemoved memberWillRemove;
        MemberRemoved memberRemoved;

        Control()
            :
            Base(),
            upstream_(nullptr),
            selected(),
            count(),
            memberAdded(),
            memberWillRemove(),
            memberRemoved()
        {
            PEX_NAME("OrderedList::Control");
        }

        Control(typename Base::Upstream &upstream)
            :
            Base(upstream),
            upstream_(&upstream),
            selected(this->list.selected),
            count(this->list.count),
            memberAdded(this->upstream_->list.memberAdded),
            memberWillRemove(this->upstream_->list.memberWillRemove),
            memberRemoved(this->upstream_->list.memberRemoved)
        {
            PEX_NAME("OrderedList::Control");

            assert(this->list.count.Get() == this->indices.size());
            assert(this->list.Get().size() == this->list.count.Get());
        }

        Control(const Control &other)
            :
            Base(*other.upstream_),
            upstream_(other.upstream_),
            selected(this->list.selected),
            count(this->list.count),
            memberAdded(this->upstream_->list.memberAdded),
            memberWillRemove(this->upstream_->list.memberWillRemove),
            memberRemoved(this->upstream_->list.memberRemoved)
        {
            PEX_NAME("OrderedList::Control");
        }

        Control & operator=(const Control &other)
        {
            this->Base::operator=(other);
            this->upstream_ = other.upstream_;
            this->selected = other.selected;
            this->count = other.count;
            this->memberAdded = other.memberAdded;
            this->memberWillRemove = other.memberWillRemove;
            this->memberRemoved = other.memberRemoved;

            return *this;
        }

        size_t GetStorageIndex(size_t orderedIndex) const
        {
            return this->upstream_->GetStorageIndex(orderedIndex);
        }

        template<typename Derived>
        void Prepend(const Derived &item)
        {
            if (!this->upstream_)
            {
                throw PexError("No connection to model");
            }

            this->upstream_->Prepend(item);
        }

        template<typename Derived>
        std::optional<size_t> Append(const Derived &item)
        {
            return this->list.Append(item);
        }

        void Set(const typename List::Type &listType)
        {
            this->list.Set(listType);
        }

        void MoveToTop(size_t storageIndex)
        {
            if (!this->upstream_)
            {
                throw std::logic_error("Unitialized control");
            }

            this->upstream_->MoveToTop(storageIndex);
        }

        void MoveToBottom(size_t storageIndex)
        {
            if (!this->upstream_)
            {
                throw std::logic_error("Unitialized control");
            }

            this->upstream_->MoveToBottom(storageIndex);
        }

        template<typename T>
        void AssignItem(size_t index, T &&item)
        {
            if (!this->upstream_)
            {
                throw std::logic_error("Unitialized control");
            }

            this->upstream_->AssignItem(index, std::forward<T>(item));
        }

        void EraseSelected()
        {
            assert(this->upstream_);
            this->upstream_->EraseSelected();
        }

        size_t size() const
        {
            return this->list.size();
        }

        bool empty() const
        {
            return this->list.empty();
        }
    };

    template<typename Base>
    class Plain: public Base, public Iterable<Plain<Base>, Base>
    {
    public:
        using IterableBase = Iterable<Plain<Base>, Base>;
        using ListItem = typename IterableBase::ListItem;

        Plain()
            :
            Base{},
            IterableBase{}
        {
            if constexpr (ListMaker::initialCount != 0)
            {
                this->resize(ListMaker::initialCount);
            }
        }

        Plain(const std::vector<ListItem> &items)
            :
            Base{},
            IterableBase{}
        {
            this->resize(items.size());
            std::copy(items.begin(), items.end(), this->begin());
        }

        // TODO: Resize eliminates items at the end of the unordered list.
        // Consider removing the last items as sorted.
        void resize(size_t size)
        {
            size_t previousSize = this->list.size();

            assert(this->indices.size() == previousSize);

            this->list.resize(size);

            if (size > previousSize)
            {
                this->IncreaseSize_(previousSize, size);

                return;
            }

            // value < previousSize
            // Remove references to indices that no longer exist
            std::erase_if(
                this->indices,
                [size](size_t index)
                {
                    return index >= size;
                });

            assert(this->indices.size() == size);
        }

        operator std::vector<ListItem> () const
        {
            return {this->begin(), this->end()};
        }

    private:
        void InitializeSize_(size_t newSize)
        {
            // The size of the list grew.
            // Add default indices for the new elements.
            size_t newIndex = 0;

            while (newIndex < newSize)
            {
                // This function is called while ListConnect has been muted.
                // Observers of the full list of indices will not be notified
                // until we are done.
                this->indices[newIndex].Set(newIndex);
                ++newIndex;
            }
        }

        void IncreaseSize_(size_t previousSize, size_t newSize)
        {
            // The size of the list grew.
            // Add default indices for the new elements.
            this->indices.resize(newSize);
            size_t newIndex = previousSize;

            while (newIndex < newSize)
            {
                // This function is called while ListConnect has been muted.
                // Observers of the full list of indices will not be notified
                // until we are done.
                this->indices[newIndex] = newIndex;
                ++newIndex;
            }
        }
    };
};


template<typename ListMaker>
using OrderedListGroup =
    Group
    <
        OrderedListFields,
        OrderedListTemplate<ListMaker>::template Template,
        OrderedListCustom<ListMaker>
    >;

template<typename ListMaker>
using OrderedListControl = typename OrderedListGroup<ListMaker>::Control;


template<typename T>
concept HasGetUnordered = requires(T t, size_t index)
{
    { t.GetUnordered(index) };
};


template<typename T>
concept HasIndices = requires (T t)
{
    { t.indices } -> IsValueContainer;
};


template<typename List>
auto & GetUnordered(List &list, size_t index)
{
    if constexpr (HasGetUnordered<List>)
    {
        return list.GetUnordered(index);
    }
    else
    {
        return list.at(index);
    }
}


template<HasOrder Item>
auto & GetOrder(Item &item)
{
    if constexpr (HasOrderMember<Item>)
    {
        return item.order;
    }
    else
    {
        return item.GetVirtual()->GetOrder();
    }
}


} // end namespace pex
