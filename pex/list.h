#pragma once


#include <vector>

#include <jive/scope_flag.h>
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


} // end namespace detail


namespace model
{


using ListCount = Value<size_t>;
using ListSelected = Value<std::optional<size_t>>;


} // namespace model


namespace control
{


using ListCount = Value<::pex::model::ListCount>;
using ListSelected = Value<::pex::model::ListSelected>;
using ListCountWillChange = Signal<GetTag>;


} // end namespace control



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
        using Selected = ::pex::model::ListSelected;
        using Access = GetAndSetTag;

        using Defer = DeferList<Member, ModelSelector, Model>;
        // using DeferredList = detail::DeferredList<Member, ModelSelector, Model>;

        template<typename>
        friend class ::pex::Reference;

        friend class Control;

        template<typename, typename>
        friend class ::pex::detail::ListConnect;

    private:
        model::Signal internalCountWillChange_;
        Count internalCount_;
        bool ignoreCount_;

    public:
        model::Signal countWillChange;
        Count count;
        Selected selected;

        Model()
            :
            detail::MuteOwner(),
            detail::Mute(this->GetMuteControl()),
            internalCountWillChange_(),
            internalCount_(initialCount),
            ignoreCount_(false),
            countWillChange(),
            count(initialCount),
            selected(),
            items_(),
            countTerminus_(this, this->count, &Model::OnCount_)
        {
            size_t toInitialize = initialCount;

            while (toInitialize--)
            {
                this->items_.push_back(std::make_unique<ListItem>());
            }
        }

        Model(const Type &items)
            :
            Model()
        {
            this->Set(items);
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

            if (*selected_ != this->items_.size() - 1)
            {
                // Move the selected element to the back before erasing it.
                // This is less efficient because it involves one extra copy
                // than erasing it in place, but it allows the existing
                // size-changing machinery in this class to be used without
                // modification.

                auto element = std::begin(this->items_)
                    + static_cast<ssize_t>(*selected_);

                auto next = element + 1;

                std::rotate(element, next, std::end(this->items_));
            }

            detail::AccessReference<Selected>(this->selected)
                .SetWithoutNotify({});

            this->count.Set(this->count.Get() - 1);
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

            // count observers will be notified at the end of this function.
            auto deferCount = pex::MakeDefer(this->count);
            deferCount.Set(newIndex + 1);

            // ChangeCount_ will adjust the size of items_.
            this->ChangeCount_(newIndex + 1);

            // Add the new item at the back of the list.
            this->items_.back()->Set(item);

            this->internalCount_.Set(newIndex + 1);

            return newIndex;
        }

        void ResizeWithoutNotify(size_t newSize)
        {
            if (newSize == this->items_.size())
            {
                assert(this->count.Get() == newSize);
                return;
            }

            detail::AccessReference<Count>(this->count)
                .SetWithoutNotify(newSize);

            this->ChangeCount_(newSize);
            this->internalCount_.Set(newSize);
        }

    private:
        void DoNotify_()
        {
            // Ignore all notifications from list members.
            // At the end of this scope, a signal notification may be sent by
            // ListConnect.
            auto scopeMute = detail::ScopeMute<Model>(*this, false);

            for (auto &item: this->items_)
            {
                detail::AccessReference<ListItem>(*item).DoNotify();
            }

            jive::ScopeFlag ignoreCount(this->ignoreCount_);
            detail::AccessReference<Count>(this->count).DoNotify();
        }

        void SetWithoutNotify_(const Type &values)
        {
            // Mute while setting item values.
            // Set isSilenced option to 'true' so nothing is notified when
            // ScopeMute is destroyed.
            auto scopeMute = detail::ScopeMute<Model>(*this, true);

            bool countChanged = false;

            if (values.size() != this->items_.size())
            {
                detail::AccessReference<Count>(this->count)
                    .SetWithoutNotify(values.size());

                this->ChangeCount_(values.size());
                countChanged = true;
            }

            assert(this->items_.size() == values.size());

            for (size_t index = 0; index < values.size(); ++index)
            {
                detail::AccessReference<ListItem>(*this->items_[index])
                    .SetWithoutNotify(values[index]);
            }

            if (countChanged)
            {
                this->internalCount_.Set(values.size());
            }
        }

        void ChangeCount_(size_t count_)
        {
            // Signal all listening controls to disconnect.
            this->countWillChange.Trigger();

            // Now signal control::Lists that the count will change.
            this->internalCountWillChange_.Trigger();

            auto wasSelected = this->selected.Get();

            detail::AccessReference<Selected>(this->selected).SetWithoutNotify({});

            if (count_ < this->items_.size())
            {
                // This is a reduction in size.
                // No new elements need to be created.
                this->items_.resize(count_);
            }
            else
            {
                assert(count_ > this->items_.size());

                size_t toInitialize = count_ - this->items_.size();

                while (toInitialize--)
                {
                    this->items_.push_back(std::make_unique<ListItem>());
                }
            }

            if (wasSelected && *wasSelected < count_)
            {
                detail::AccessReference<Selected>(this->selected)
                    .SetWithoutNotify(wasSelected);
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

            this->ChangeCount_(count_);
            this->internalCount_.Set(count_);
        }

        ::pex::control::ListCount GetInternalCount_()
        {
            return {this->internalCount_};
        }

    private:
        std::vector<std::unique_ptr<ListItem>> items_;
        ::pex::Terminus<Model, Count> countTerminus_;
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
        using Selected = ::pex::control::ListSelected;
        using CountWillChange = ::pex::control::ListCountWillChange;

        using CountWillChangeTerminus =
            ::pex::Terminus<Control, CountWillChange>;

        using CountTerminus = ::pex::Terminus<Control, Count>;
        using Vector = std::vector<ListItem>;
        using Iterator = typename Vector::iterator;
        using ConstIterator = typename Vector::const_iterator;
        using ReverseIterator = typename Vector::reverse_iterator;
        using ConstReverseIterator = typename Vector::const_reverse_iterator;

        using Defer = DeferList<Member, ControlSelector, Control>;

        // using DeferredList =
        //     detail::DeferredList<Member, ControlSelector, Control>;

        static_assert(IsControl<ListItem> || IsGroupControl<ListItem>);

        template<typename>
        friend class ::pex::Reference;

        template<typename, typename>
        friend class ::pex::detail::ListConnect;

        CountWillChange countWillChange;
        Count count;
        Selected selected;

        Control()
            :
            detail::Mute(),
            countWillChange(),
            count(),
            selected(),
            upstream_(nullptr),
            countTerminus_(),
            items_()
        {

        }

        Control(Upstream &upstream)
            :
            detail::Mute(upstream.CloneMuteControl()),
            countWillChange(upstream.countWillChange),
            count(upstream.count),
            selected(upstream.selected),
            upstream_(&upstream),

            countWillChange_(
                this,
                this->upstream_->internalCountWillChange_,
                &Control::OnCountWillChange_),

            countTerminus_(
                this,
                this->upstream_->internalCount_,
                &Control::OnCount_),

            items_()
        {
            for (size_t index = 0; index < this->count.Get(); ++index)
            {
                this->items_.emplace_back((*this->upstream_)[index]);
            }
        }

        Control(const Control &other)
            :
            detail::Mute(other),
            countWillChange(other.countWillChange),
            count(other.count),
            selected(other.selected),
            upstream_(other.upstream_),

            countWillChange_(
                this,
                this->upstream_->internalCountWillChange_,
                &Control::OnCountWillChange_),

            countTerminus_(
                this,
                this->upstream_->internalCount_,
                &Control::OnCount_),

            items_(other.items_)
        {

        }

        Control & operator=(const Control &other)
        {
            this->detail::Mute::operator=(other);
            this->countWillChange = other.countWillChange;
            this->count = other.count;
            this->selected = other.selected;
            this->upstream_ = other.upstream_;
            this->countWillChange_.Assign(this, other.countWillChange_);
            this->countTerminus_.Assign(this, other.countTerminus_);
            this->items_ = other.items_;

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

            if (!this->countWillChange.HasModel())
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

        void OnCountWillChange_()
        {
            this->items_.clear();
        }

        void OnCount_(size_t count_)
        {
            if (!this->items_.empty())
            {
                // items_ should have been cleared by an earlier call to
                // OnCountWillChange_.
                throw std::logic_error("items must be empty");
            }

            Vector updatedItems;

            for (size_t index = 0; index < count_; ++index)
            {
                updatedItems.emplace_back((*this->upstream_)[index]);
            }

            this->items_.swap(updatedItems);
        }

        Count GetInternalCount_()
        {
            assert(this->upstream_);

            return Count(this->upstream_->internalCount_);
        }

    private:
        Upstream *upstream_;
        CountWillChangeTerminus countWillChange_;
        CountTerminus countTerminus_;
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
