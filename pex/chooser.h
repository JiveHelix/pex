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


template<typename T>
class Chooser
{
public:
    using Type = T;
    using Selection = ::pex::model::Value<size_t>;
    using Choices = ::pex::model::Value<std::vector<Type>>;

private:
    using ConstReference = ::pex::ConstReference<Choices>;
    using Defer = ::pex::Defer<Choices>;

public:
    Chooser(ArgumentT<Type> initialValue)
        :
        selection_(static_cast<size_t>(0)),
        choices_(std::vector<Type>{initialValue})
    {

    }

    Chooser(
        ArgumentT<Type> initialValue,
        const std::vector<Type> &choices)
        :
        selection_(RequireIndex(initialValue, choices)),
        choices_(choices)
    {

    }

    Chooser(const std::vector<Type> &choices)
        :
        selection_(static_cast<size_t>(0)),
        choices_(choices)
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
    }

    void SetSelectedIndex(size_t index)
    {
        if (index >= ConstReference(this->choices_).Get().size())
        {
            throw std::out_of_range("Selection not in choices.");
        }

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
                ConstReference(this->choices).Get()));
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
    Selection selection_;
    Choices choices_;
};


} // namespace model


template<typename T>
struct IsModelChooser: std::false_type {};

template<typename T>
struct IsModelChooser<model::Chooser<T>>: std::true_type {};


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

    Chooser(Upstream &upstream)
    {
        if constexpr (IsModelChooser<Upstream>::value)
        {
            this->selection = Selection(upstream.selection_);
            this->choices = Choices(upstream.choices_);
        }
        else
        {
            this->selection = Selection(upstream.selection);
            this->choices = Choices(upstream.choices);
        }
    }

    Selection selection;
    Choices choices;
};

} // namespace control

} // namespace pex
