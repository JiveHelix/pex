#pragma once


#include <vector>

#include "pex/model_value.h"
#include "pex/control_value.h"
#include "pex/terminus.h"
#include "pex/signal.h"
#include "pex/detail/mute.h"
#include "pex/reference.h"


namespace pex
{


namespace control
{
    template<typename, typename, typename>
    class List;
}


namespace model
{


using ListCount = Value<size_t>;
using ListSelected = Value<std::optional<size_t>>;


template<typename Model_, size_t initialCount>
class List
    :
    public detail::MuteOwner,
    public detail::Mute
{
public:
    static constexpr auto observerName = "pex::model::List";

    using Model = Model_;
    using Item = typename Model::Type;
    using Type = std::vector<Item>;
    using Count = ListCount;
    using Selected = ListSelected;
    using Access = GetAndSetTag;

    template<typename>
    friend class ::pex::Reference;

    template<typename, typename, typename>
    friend class ::pex::control::List;

    Signal countWillChange;
    Count count;
    Selected selected;

    List()
        :
        detail::MuteOwner(),
        detail::Mute(this->GetMuteControl()),
        countWillChange(),
        count(initialCount),
        selected(),
        items_(),
        countTerminus_(this, this->count, &List::OnCount_)
    {
        size_t toInitialize = initialCount;

        while (toInitialize--)
        {
            this->items_.push_back(std::make_unique<Model>());
        }
    }

    List(const Type &items)
        :
        List()
    {
        this->Set(items);
    }

    Model & operator[](size_t index)
    {
        auto &pointer = this->items_[index];

#ifndef NDEBUG
        if (!pointer)
        {
            throw std::logic_error("item unitialized");
        }
#endif

        return *pointer;
    }

    Model & at(size_t index)
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
        // Mute while setting item values.
        auto muteDeferred = detail::MuteDeferred<List>(*this);
        muteDeferred.Mute();

        if (values.size() != this->items_.size())
        {
            this->count.Set(values.size());
        }

        assert(this->items_.size() == values.size());

        for (size_t index = 0; index < values.size(); ++index)
        {
            this->items_[index]->Set(values[index]);
        }
    }

    template<typename Derived>
    size_t Append(const Derived &item)
    {
        // Mute while setting item values.
        // TODO: Rename MuteDeferred. It unmutes at end of scope.
        auto muteDeferred = detail::MuteDeferred<List>(*this);
        muteDeferred.Mute();

        size_t newIndex = this->count.Get();
        auto deferCount = pex::MakeDefer(this->count);
        deferCount.Set(newIndex + 1);
        this->OnCount_(newIndex + 1);
        this->items_.back()->Set(item);

        return newIndex;
    }

private:
    void DoNotify_()
    {
        for (auto &item: this->items_)
        {
            detail::AccessReference<Model>(*item).DoNotify();
        }
    }

    void SetWithoutNotify_(const Type &values)
    {
        // Mute while setting item values.
        auto muteDeferred = detail::MuteDeferred<List>(*this);
        muteDeferred.Mute();

        if (values.size() != this->items_.size())
        {
            this->count.Set(values.size());
        }

        assert(this->items_.size() == values.size());

        for (size_t index = 0; index < values.size(); ++index)
        {
            detail::AccessReference<Model>(*this->items_[index])
                .SetWithoutNotify(values[index]);
        }
    }

    void OnCount_(size_t count_)
    {
        // Signal all listening controls to disconnect.
        this->countWillChange.Trigger();

        this->selected.Set({});

        if (count_ < this->items_.size())
        {
            // This is a reduction in size.
            // No new elements need to be created.
            this->items_.resize(count_);
        }
        else if (count_ != this->items_.size())
        {
            size_t toInitialize = count_ - this->items_.size();

            while (toInitialize--)
            {
                this->items_.push_back(std::make_unique<Model>());
            }
        }
    }

private:
    ::pex::detail::MuteOwner mute_;
    std::vector<std::unique_ptr<Model>> items_;
    ::pex::Terminus<List, Count> countTerminus_;
};


template<typename ...T>
struct IsList_: std::false_type {};

template<typename T, size_t S>
struct IsList_<List<T, S>>: std::true_type {};

template<typename ...T>
inline constexpr bool IsList = IsList_<T...>::value;


} // end namespace model


namespace control
{


using ListCount = Value<::pex::model::ListCount>;
using ListSelected = Value<::pex::model::ListSelected>;


template
<
    typename Upstream_,
    typename ItemControl_,
    typename Access_ = GetAndSetTag
>
class List: public detail::Mute
{
    static_assert(
        model::IsList<Upstream_>,
        "Expected a model::List as upstream");

public:
    static constexpr auto observerName = "pex::control::List";

    using Access = Access_;

    using Upstream = Upstream_;
    using Type = typename Upstream::Type;
    using Item = typename Upstream::Item;
    using ItemControl = ItemControl_;
    using Count = ListCount;
    using Selected = ListSelected;
    using CountWillChange = Signal<GetTag>;
    using CountWillChangeTerminus = ::pex::Terminus<List, CountWillChange>;
    using CountTerminus = ::pex::Terminus<List, Count>;
    using Vector = std::vector<ItemControl>;
    using Iterator = typename Vector::iterator;
    using ConstIterator = typename Vector::const_iterator;

    template<typename>
    friend class ::pex::Reference;

    CountWillChange countWillChange;
    Count count;
    Selected selected;

    List()
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

    List(Upstream &upstream)
        :
        detail::Mute(upstream.CloneMuteControl()),
        countWillChange(upstream.countWillChange),
        count(upstream.count),
        selected(upstream.selected),
        upstream_(&upstream),

        countWillChange_(
            this,
            this->countWillChange,
            &List::OnCountWillChange_),

        countTerminus_(this, this->count, &List::OnCount_),
        items_()
    {
        for (size_t index = 0; index < this->count.Get(); ++index)
        {
            this->items_.emplace_back((*this->upstream_)[index]);
        }
    }

    List(const List &other)
        :
        detail::Mute(other),
        countWillChange(other.countWillChange),
        count(other.count),
        selected(other.selected),
        upstream_(other.upstream_),

        countWillChange_(
            this,
            this->countWillChange,
            &List::OnCountWillChange_),

        countTerminus_(this, this->count, &List::OnCount_),
        items_(other.items_)
    {

    }

    List & operator=(const List &other)
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

    const ItemControl & operator[](size_t index) const
    {
        return this->items_[index];
    }

    ItemControl & operator[](size_t index)
    {
        return this->items_[index];
    }

    const ItemControl & at(size_t index) const
    {
        return this->items_.at(index);
    }

    ItemControl & at(size_t index)
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
        for (auto &item: this->items_)
        {
            detail::AccessReference<ItemControl>(item).DoNotify();
        }
    }

    void SetWithoutNotify_(const Type &values)
    {
#ifndef NDEBUG
        if (!this->upstream_)
        {
            throw std::logic_error("control::List is uninitialized");
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
        if (this->items_.size())
        {
            throw std::logic_error("items must be empty");
        }

        Vector updatedItems;

        for (size_t index = 0; index < count_; ++index)
        {
            updatedItems.emplace_back((*this->upstream_)[index]);
        }

        this->items_.swap(updatedItems);
    }

private:
    Upstream *upstream_;
    CountWillChangeTerminus countWillChange_;
    CountTerminus countTerminus_;
    Vector items_;
};


template<typename ...T>
struct IsList_: std::false_type {};

template<typename ...T>
struct IsList_<List<T...>>: std::true_type {};

template<typename ...T>
inline constexpr bool IsList = IsList_<T...>::value;


} // end namespace control

template<typename T>
inline constexpr bool IsListModel = model::IsList<T>;

template<typename T>
inline constexpr bool IsListControl = control::IsList<T>;

template<typename T>
inline constexpr bool IsList = control::IsList<T> || model::IsList<T>;


} // end namespace pex


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
