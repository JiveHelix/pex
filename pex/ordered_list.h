#pragma once


#include <fields/fields.h>
#include "pex/group.h"
#include "pex/reference.h"
#include "pex/endpoint.h"
#include "pex/traits.h"

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
        fields::Field(&T::moveDown, "moveDown"),
        fields::Field(&T::moveUp, "moveUp"));
};


template<template<typename> typename T>
class OrderTemplate
{
public:
    T<MakeSignal> moveDown;
    T<MakeSignal> moveUp;

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


using IndicesList = List<size_t, 0>;

using OrderedIndicesControl = ControlSelector<IndicesList>;


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
        using MemberRemoved = control::ListOptionalIndex;
        using MemberAdded = model::ListOptionalIndex;
        using CountWillChange = control::ListCountWillChange;
        using Count = control::ListCount;

    private:
        using CountEndpoint = Endpoint<Model, Count>;
        using CountWillChangeEndpoint = Endpoint<Model, CountWillChange>;
        using MemberRemovedEndpoint = Endpoint<Model, MemberRemoved>;

        MemberRemovedEndpoint memberRemovedEndpoint_;
        CountWillChangeEndpoint countWillChangeEndpoint_;
        CountEndpoint countEndpoint_;

    public:
        Selected selected;
        CountWillChange countWillChange;
        Count count;
        MemberAdded memberAdded;

        using Indices = ModelSelector<IndicesList>;
        using IndicesListItem = typename Indices::ListItem;

        using List = decltype(Base::list);
        using ListItem = typename List::ListItem;

        Model()
            :
            Base(),

            memberRemovedEndpoint_(
                this,
                this->list.memberRemoved,
                &Model::OnListMemberRemoved_),

            countWillChangeEndpoint_(
                this,
                this->list.countWillChange,
                &Model::OnListCountWillChange_),

            countEndpoint_(this, this->list.count, &Model::OnListCount_),

            selected(this->list.selected),
            countWillChange(this->list.countWillChange),
            count(this->list.count),
            moveDownEndpoints_(),
            moveUpEndpoints_(),
            reorderEndpoint_(this, this->indices, &Model::OnReorder_)
        {
            REGISTER_PEX_NAME(this, "OrderedList::Model");
            REGISTER_PEX_PARENT(this, &this->list);
            REGISTER_PEX_PARENT(this, &this->indices);
            REGISTER_PEX_PARENT(this, &this->reorder);

            this->OnListCount_(this->list.count.Get());

            assert(this->list.count.Get() == this->indices.count.Get());
            assert(this->indices.Get().size() == this->indices.count.Get());
            assert(this->list.Get().size() == this->list.count.Get());

            PEX_LOG("Created OrderedList::Model: ", pex::LookupPexName(this));
        }

        ListItem & operator[](size_t index)
        {
            return this->list[index];
        }

        template<typename Derived>
        void Prepend(const Derived &item)
        {
            auto newIndex = this->list.Append(item);
            this->MoveToTop(newIndex);
            this->memberAdded.Set(newIndex);
        }

        template<typename Derived>
        size_t Append(const Derived &item)
        {
            return this->list.Append(item);
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
            this->moveDownEndpoints_[storageIndex].Disconnect();
            this->moveUpEndpoints_[storageIndex].Disconnect();

            this->list.at(storageIndex).Set(std::forward<T>(item));

            if constexpr (hasOrder)
            {
                this->MakeOrderConnections_(storageIndex);
            }
        }

        void EraseSelected()
        {
            this->list.EraseSelected();
        }

    private:
        void OnReorder_()
        {
            this->reorder.Trigger();
        }

        void RestoreOrderConnections_()
        {
            if constexpr (hasOrder)
            {
                for (size_t index = 0; index < this->count.Get(); ++index)
                {
                    auto storageIndex = this->indices.at(index).Get();
                    this->MakeOrderConnections_(storageIndex);
                }
            }
        }

        void MakeOrderConnections_([[maybe_unused]] size_t storageIndex)
        {
            if constexpr (!hasOrder)
            {
                return;
            }

            OrderControl order;

            if constexpr (ListHasOrderMember<ListMaker>)
            {
                order = this->list.at(storageIndex).order;
            }
            else
            {
                static_assert(ListHasVirtualGetOrder<ListMaker>);

                auto virtualItem = this->list.at(storageIndex).GetVirtual();

                if (!virtualItem)
                {
                    return;
                }

                order = virtualItem->GetOrder();
            }

            this->moveDownEndpoints_[storageIndex] =
                MoveOrderEndpoint(
                    this,
                    order.moveDown,
                    &Model::MoveDown,
                    storageIndex);

            this->moveUpEndpoints_[storageIndex] =
                MoveOrderEndpoint(
                    this,
                    order.moveUp,
                    &Model::MoveUp,
                    storageIndex);
        }

        void IncreaseSize_(size_t previousSize, size_t newSize)
        {
            // The size of the list grew.
            // Add default indices for the new elements.
            size_t newIndex = previousSize;

            while (newIndex < newSize)
            {
                // This function is called while ListConnect has been muted.
                // Observers of the full list of indices will not be notified
                // until we are done.
                this->indices[newIndex].Set(newIndex);
                ++newIndex;
            }

            if constexpr (hasOrder)
            {
                this->moveDownEndpoints_.resize(newSize);
                this->moveUpEndpoints_.resize(newSize);
            }
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
        }

        void OnListCountWillChange_()
        {
            this->moveDownEndpoints_.clear();
            this->moveUpEndpoints_.clear();
        }

        void OnListCount_(size_t value)
        {
            if (value == this->indices.count.Get())
            {
                // The indices size already matches the new list size.
                if constexpr (hasOrder)
                {
                    // It is still necessary to make the order connections.
                    this->moveDownEndpoints_.resize(value);
                    this->moveUpEndpoints_.resize(value);
                    this->RestoreOrderConnections_();
                }

                return;
            }

            // Mute while adjusting indices.
            // Silence notifications on indices to allow the count observers to
            // be notified first.
            auto scopeMute =
                detail::ScopeMute<Indices>(this->indices, true);

            auto previous = this->indices.Get();
            auto previousSize = previous.size();

            if (value > previousSize)
            {
                this->indices.ResizeWithoutNotify(value);
                this->IncreaseSize_(previousSize, value);
                this->RestoreOrderConnections_();

                return;
            }

            // value < previousSize
            // Remove references to indices that no longer exist
            std::erase_if(
                previous,
                [value](size_t index)
                {
                    return index >= value;
                });

            this->indices.Set(previous);

            if constexpr (hasOrder)
            {
                this->moveDownEndpoints_.resize(value);
                this->moveUpEndpoints_.resize(value);
                this->RestoreOrderConnections_();
            }
        }

    private:

        using MoveOrderEndpoint =
            pex::BoundEndpoint
            <
                control::Signal<>,
                decltype(&Model::MoveUp)
            >;

        std::vector<MoveOrderEndpoint> moveDownEndpoints_;
        std::vector<MoveOrderEndpoint> moveUpEndpoints_;

        // static_assert(
            // pex::IsControl<pex::GetControlType<decltype(Model::indices)>>);

        using ReorderEndpoint =
            detail::ListConnect<Model, decltype(Model::indices)>;

        ReorderEndpoint reorderEndpoint_;
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
            assert(
                this->list_.size() == this->indices_.size());
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
            return self->list.at(size_t(self->indices.at(index)));
        }

        ListItem & operator[](size_t index)
        {
            auto self = this->GetDerived();

            return self->list.at(size_t(self->indices.at(index)));
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
        using CountWillChange = typename List::CountWillChange;
        using Count = typename List::Count;
        using ListItem = typename List::ListItem;
        using Upstream = typename Base::Upstream;

        static_assert(
            std::is_same_v
            <
                typename Iterable<Control<Base>, Base>::ListItem,
                ListItem
            >);

    private:
        // In group customizations, 'Upstream' is always the group's Model.
        Upstream *upstream_;

    public:
        Selected selected;
        CountWillChange countWillChange;
        Count count;
        MemberAdded memberAdded;

        Control()
            :
            Base(),
            upstream_(nullptr),
            selected(),
            countWillChange(),
            count(),
            memberAdded()
        {
            REGISTER_PEX_NAME(this, "OrderedList::Control");
            REGISTER_PEX_PARENT(this, &this->list);
            REGISTER_PEX_PARENT(this, &this->indices);
            REGISTER_PEX_PARENT(this, &this->reorder);
        }

        Control(typename Base::Upstream &upstream)
            :
            Base(upstream),
            upstream_(&upstream),
            selected(this->list.selected),
            countWillChange(this->list.countWillChange),
            count(this->list.count),
            memberAdded(this->upstream_->memberAdded)
        {
            REGISTER_PEX_NAME(this, "OrderedList::Control");
            REGISTER_PEX_PARENT(this, &this->list);
            REGISTER_PEX_PARENT(this, &this->indices);
            REGISTER_PEX_PARENT(this, &this->reorder);

            assert(this->list.count.Get() == this->indices.count.Get());
            assert(this->indices.Get().size() == this->indices.count.Get());
            assert(this->list.Get().size() == this->list.count.Get());
        }

        Control(const Control &other)
            :
            Base(*other.upstream_),
            upstream_(other.upstream_),
            selected(this->list.selected),
            countWillChange(this->list.countWillChange),
            count(this->list.count),
            memberAdded(this->upstream_->memberAdded)
        {
            REGISTER_PEX_NAME(this, "OrderedList::Control");
            REGISTER_PEX_PARENT(this, &this->list);
            REGISTER_PEX_PARENT(this, &this->indices);
            REGISTER_PEX_PARENT(this, &this->reorder);
        }

        Control & operator=(const Control &other)
        {
            this->Base::operator=(other);
            this->upstream_ = other.upstream_;
            this->selected = other.selected;
            this->countWillChange = other.countWillChange;
            this->count = other.count;
            this->memberAdded = other.memberAdded;

            return *this;
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
    };

    template<typename Base>
    class Plain: public Base, public Iterable<Plain<Base>, Base>
    {
    public:
        Plain()
            :
            Base{},
            Iterable<Plain<Base>, Base>{}
        {
            if constexpr (ListMaker::initialCount != 0)
            {
                this->resize(ListMaker::initialCount);
            }
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

    private:
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
    { t.indices } -> IsListControl;
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
