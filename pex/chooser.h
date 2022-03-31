/**
  * @file chooser.h
  *
  * @brief Combine a vector of choices and a selected value.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 12 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <vector>
#include "pex/converter.h"
#include "pex/detail/argument.h"
#include "pex/value.h"
#include "pex/reference.h"
#include "pex/find_index.h"


namespace pex
{


// Forward declare the control::Chooser
// Necessary for the friend declaration in model::Chooser
namespace control
{

template<typename, typename>
class Chooser;


} // end namespace control


namespace model
{


/** 
 ** Get and Set will pass through the selected index unless it is not a valid
 ** choice. In that case, the index of the last valid choice will be returned.
 **
 **/
template<typename T>
struct ChooserFilter
{
    using Choices = std::vector<T>;

    ChooserFilter(const Choices &choices)
        :
        choices_(choices)
    {
        if (choices.empty())
        {
            throw std::invalid_argument("Choices must not be empty");
        }
    }

    size_t Get(size_t selectedIndex) const
    {
        if (selectedIndex >= this->choices_.size())
        {
            return this->choices_.size() - 1;
        }

        return selectedIndex;
    }

    size_t Set(size_t selectedIndex) const
    {
        return this->Get(selectedIndex);
    }

private:
    Choices choices_;
};


template<typename T>
class Chooser
{
public:
    using Type = T;
    using Selection = ::pex::model::FilteredValue<size_t, ChooserFilter<Type>>;
    using Choices = ::pex::model::Value<std::vector<Type>>;

private:
    using ConstReference = ::pex::ConstReference<Choices>;
    using Defer = ::pex::Defer<Choices>;

public:
    Chooser()
        :
        choices_(std::vector<Type>{Type{}}),
        selection_(
            static_cast<size_t>(0),
            ChooserFilter(this->choices_.Get()))
    {

    }

    Chooser(ArgumentT<Type> initialValue)
        :
        choices_(std::vector<Type>{initialValue}),
        selection_(
            static_cast<size_t>(0),
            ChooserFilter(this->choices_.Get()))
    {

    }

    Chooser(
        ArgumentT<Type> initialValue,
        const std::vector<Type> &choices)
        :
        choices_(choices),
        selection_(
            RequireIndex(initialValue, choices),
            ChooserFilter(this->choices_.Get()))
    {

    }

    Chooser(const std::vector<Type> &choices)
        :
        choices_(choices),
        selection_(
            static_cast<size_t>(0),
            ChooserFilter(this->choices_.Get()))
    {
        if (choices.empty())
        {
            throw std::invalid_argument("choices must not be empty");
        }
    }

    void SetChoices(const std::vector<Type> &choices)
    {
        // Don't immediately publish the change to choices.
        // The change is effective immediately, and will be published when
        // changeChoices goes out of scope.
        auto changeChoices = Defer(this->choices_);
        changeChoices.Set(choices);

        if (this->selection_.Get() >= choices.size())
        {
            // Because this->choices_ has been updated (though not published),
            // any listener for the index will be able to retrieve the new list
            // of choices instead of the old one.
            this->selection_.Set(0);
        }

        this->selection_.SetFilter(ChooserFilter(this->choices_.Get()));
    }

    void SetSelectedIndex(size_t index)
    {
        this->selection_.Set(index);
    }

    Type GetSelection() const
    {
        return ConstReference(this->choices_)
            .Get()[this->selection_.Get()];
    }

    void SetSelection(ArgumentT<Type> value)
    {
        this->selection_.Set(
            RequireIndex(
                value,
                ConstReference(this->choices_).Get()));
    }

    size_t GetSelectedIndex() const
    {
        return this->selection_.Get();
    }

    std::vector<Type> GetChoices() const
    {
        return this->choices_.Get();
    }

    void Connect(
        void * context,
        typename Selection::Callable callable)
    {
        this->selection_.Connect(context, callable);
    }

    template <typename, typename>
    friend class ::pex::control::Chooser;

private:
    Choices choices_;
    Selection selection_;
};


} // namespace model


template<typename T>
struct IsModelChooser_: std::false_type {};

template<typename T>
struct IsModelChooser_<model::Chooser<T>>: std::true_type {};

template<typename T>
inline constexpr bool IsModelChooser = IsModelChooser_<T>::value;


namespace control
{


template<typename Observer, typename Upstream>
class Chooser
{
public:
    using Type = typename Upstream::Type;

    using Selection =
        ::pex::control::Value
        <
            Observer,
            typename Upstream::Selection,
            ::pex::GetAndSetTag
        >;

    using Choices =
        ::pex::control::Value
        <
            Observer,
            typename Upstream::Choices,
            ::pex::GetTag
        >;

    Chooser() = default;

    Chooser(Upstream &upstream)
    {
        if constexpr (IsModelChooser<Upstream>)
        {
            this->choices = Choices(upstream.choices_);
            this->selection = Selection(upstream.selection_);
        }
        else
        {
            this->choices = Choices(upstream.choices);
            this->selection = Selection(upstream.selection);
        }
    }

    Choices choices;
    Selection selection;
};

} // namespace control

} // namespace pex
