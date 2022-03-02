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


namespace pex
{


namespace model
{


template<typename T>
class Chooser
{
public:
    using Type = T;
    using Selection = ::pex::model::Value<size_t>;
    using Choices = ::pex::model::Value<std::vector<T>>;

    using SelectionControl =
        ::pex::control::Value
        <
            void,
            Selection,
            ::pex::GetAndSetTag
        >;

    using ChoicesControl =
        ::pex::control::Value
        <
            void,
            Choices,
            ::pex::GetTag
        >;

private:
    using ConstReference = ::pex::ConstReference<Choices>;
    using Reference = ::pex::Reference<Choices>;

public:
    Chooser(ArgumentT<T> initialValue)
        :
        selectedIndex_(static_cast<size_t>(0)),
        choices_(std::vector<T>{initialValue})
    {

    }

    Chooser(
        ArgumentT<T> initialValue,
        const std::vector<T> &choices)
        :
        selectedIndex_(RequireIndex(initialValue, choices)),
        choices_(choices)
    {

    }

    Chooser(const std::vector<T> &choices)
        :
        selectedIndex_(static_cast<size_t>(0)),
        choices_(choices)
    {
        if (choices.empty())
        {
            throw std::invalid_argument("choices must not be empty");
        }
    }

    void SetChoices(const std::vector<T> &choices)
    {
        // Don't immediately publish the change to choices.
        // The change is effective immediately, and will be published when
        // changeChoices goes out of scope.
        auto changeChoices = Reference(this->choices_);
        *changeChoices = choices;

        if (this->selectedIndex_.Get() >= choices.size())
        {
            // Because this->choices_ has been updated (though not published),
            // any listener for the index will be able to retrieve the new list
            // of choices instead of the old one.
            this->selectedIndex_.Set(0);
        }
    }

    void SetSelectedIndex(size_t index)
    {
        if (index >= ConstReference(this->choices_).Get().size())
        {
            throw std::out_of_range("Selection not in choices.");
        }

        this->selectedIndex_.Set(index);
    }

    T GetSelection() const
    {
        return ConstReference(this->choices_)
            .Get()[this->selectedIndex_.Get()];
    }

    void SetSelection(ArgumentT<T> value)
    {
        this->selectedIndex_.Set(
            RequireIndex(
                value,
                ConstReference(this->choices).Get()));
    }

    size_t GetSelectedIndex() const
    {
        return this->selectedIndex_.Get();
    }

    std::vector<T> GetChoices() const
    {
        return this->choices_.Get();
    }

    SelectionControl GetSelectionControl()
    {
        return SelectionControl(this->selectedIndex_);
    }

    ChoicesControl GetChoicesControl()
    {
        return ChoicesControl(this->choices_);
    }

    void Connect(
        void * context,
        typename Selection::Callable callable)
    {
        this->selectedIndex_.Connect(context, callable);
    }

private:
    Selection selectedIndex_;
    Choices choices_;
};


} // namespace model


namespace control
{

template<typename Observer, typename T>
struct Chooser
{
    using Selection =
        typename ObservedValue
        <
            Observer,
            typename ::pex::model::Chooser<T>::SelectionControl
        >::Type;

    using Choices =
        typename ObservedValue
        <
            Observer,
            typename ::pex::model::Chooser<T>::ChoicesControl
        >::Type;

    Chooser(::pex::model::Chooser<T> & model)
        :
        selection(model.GetSelectionControl()),
        choices(model.GetChoicesControl())
    {

    }

    Selection selection;
    Choices choices;
};

} // namespace control

} // namespace pex
