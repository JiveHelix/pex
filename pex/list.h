#pragma once


#include <vector>

#include "pex/model_value.h"
#include "pex/control_value.h"
#include "pex/terminus.h"
#include "pex/signal.h"


namespace pex
{


namespace control
{
    template<typename, typename>
    class List;
}


namespace model
{


template<typename Model_, size_t initialCount>
class List
{
public:
    using Model = Model_;
    using Member = typename Model::Type;
    using Type = std::vector<Member>;
    using Count = Value<size_t>;

    template<typename>
    friend class ::pex::Reference;

    template<typename, typename>
    friend class ::pex::control::List;

    Count count;

    List()
        :
        count(initialCount),
        countWillChange_(),
        items_(),
        countTerminus_(this, this->count, &List::OnCount_)

    {
        size_t toInitialize = initialCount;

        while (toInitialize--)
        {
            this->items_.push_back(std::make_unique<Model>());
        }
    }

    Model & operator[](size_t index)
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
        if (values.size() != this->items_.size())
        {
            throw std::out_of_range("Incorrect element count");
        }

        for (size_t index = 0; index < values.size(); ++index)
        {
            this->items_[index]->Set(values[index]);
        }
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
        if (values.size() != this->items_.size())
        {
            throw std::out_of_range("Incorrect element count");
        }

        for (size_t index = 0; index < values.size(); ++index)
        {
            detail::AccessReference<Model>(*this->items_[index])
                .SetWithoutNotify(values[index]);
        }
    }

    void OnCount_(size_t count_)
    {
        // Signal all listening controls to disconnect.
        this->countWillChange_.Trigger();

        if (count_ < this->items_.size())
        {
            // This is a reduction in size.
            // No new elements need to be created.
            this->items_.resize(count_);
        }
        else
        {
            size_t toInitialize = count_ - this->items_.size();

            while (toInitialize--)
            {
                this->items_.push_back(std::make_unique<Model>());
            }
        }
    }

private:
    Signal countWillChange_;
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


template<typename Upstream_, typename Control_>
class List
{
    static_assert(
        model::IsList<Upstream_>,
        "Expected a model::List as upstream");

public:
    using Upstream = Upstream_;
    using Type = typename Upstream::Type;
    using Control = Control_;
    using CountControl = Value<typename Upstream::Count>;
    using CountWillChangeTerminus = ::pex::Terminus<List, pex::model::Signal>;
    using CountTerminus = ::pex::Terminus<List, CountControl>;

    template<typename>
    friend class ::pex::Reference;

    CountControl count;

    List()
        :
        count(),
        upstream_(nullptr),
        countTerminus_(),
        items_()
    {

    }

    List(Upstream &upstream)
        :
        count(upstream.count),
        upstream_(&upstream),

        countWillChange_(
            this,
            this->upstream_->countWillChange_,
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
        count(other.count),
        upstream_(other.upstream_),

        countWillChange_(
            this,
            this->upstream_->countWillChange_,
            &List::OnCountWillChange_),

        countTerminus_(this, this->count, &List::OnCount_),
        items_(other.items_)
    {

    }

    List & operator=(const List &other)
    {
        this->count = other.count;
        this->upstream_ = other.upstream_;
        this->countWillChange_.Assign(this, other.countWillChange_);
        this->countTerminus_.Assign(this, other.countTerminus_);
        this->items_ = other.items_;

        return *this;
    }

    Control operator[](size_t index) const
    {
        return this->items_.at(index);
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

        for (auto &item: this->items_)
        {
            if (!item->HasModel())
            {
                return false;
            }
        }

        return true;
    }

private:
    void DoNotify_()
    {
        for (auto &item: this->items_)
        {
            detail::AccessReference<Control>(*item).DoNotify();
        }
    }

    void SetWithoutNotify_(const Type &values)
    {
        if (values.size() != this->items_.size())
        {
            throw std::out_of_range("Incorrect element count");
        }

        for (size_t index = 0; index < values.size(); ++index)
        {
            detail::AccessReference<Control>(*this->items_[index])
                .SetWithoutNotify(values[index]);
        }
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

        std::vector<Control> updatedItems;

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
    std::vector<Control> items_;
};


} // end namespace control


} // end namespace pex
