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
#include "pex/log.h"
#include "pex/converter.h"
#include "pex/detail/argument.h"
#include "pex/value.h"
#include "pex/reference.h"
#include "pex/find_index.h"
#include "pex/traits.h"


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


template<typename Upstream, typename ChoicesAccess_ = GetAndSetTag>
class Chooser
{
public:
    using Value = UpstreamHolderT<Upstream>;
    using Type = typename Value::Type;

    using ChoicesAccess = ChoicesAccess_;
    using Selection = ::pex::model::FilteredValue<size_t, ChooserFilter<Type>>;
    using Choices = ::pex::model::Value<std::vector<Type>>;

private:
    using ConstReference = ::pex::ConstReference<Choices>;
    using Defer = ::pex::Defer<Choices>;

public:
    Chooser() = default;

    Chooser(PexArgument<Upstream> upstream)
        :
        value_(upstream),
        choices_(std::vector<Type>{upstream.Get()}),
        selection_(
            static_cast<size_t>(0),
            ChooserFilter(this->choices_.Get()))
    {
        static_assert(
            HasAccess<SetTag, ChoicesAccess>,
            "Choices must be set on construction, they are read-only.");

        PEX_LOG(this, " calling connect on ", &this->selection_);
        this->selection_.Connect(this, &Chooser::OnSelection_);
    }

    Chooser(
        PexArgument<Upstream> upstream,
        const std::vector<Type> &choices)
        :
        value_(upstream),
        choices_(choices),
        selection_(
            RequireIndex(upstream.Get(), choices),
            ChooserFilter(this->choices_.Get()))
    {
        PEX_LOG(this, " calling connect on ", &this->selection_);
        this->selection_.Connect(this, &Chooser::OnSelection_);
    }
    
    ~Chooser()
    {
        PEX_LOG(this, " calling Disconnect on ", &this->selection_);
        this->selection_.Disconnect(this);
    }

    void SetChoices(const std::vector<Type> &choices)
    {
        static_assert(
            HasAccess<SetTag, ChoicesAccess>,
            "Choices cannot be set when they are read-only.");

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

    void SetSelection(Argument<Type> value)
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

    // Receive notifications by type Type when the selection changes.
    void Connect(
        void * context,
        typename Value::Callable callable)
    {
        PEX_LOG(
            this,
            " calling connect on ",
            &this->value_,
            " with ",
            context);

        this->value_.Connect(context, callable);
    }

    void Disconnect(void * context)
    {
        PEX_LOG(
            this,
            " calling Disconnect on ",
            &this->value_,
            " with ",
            context);

        this->value_.Disconnect(context);
    }

    template <typename, typename>
    friend class ::pex::control::Chooser;

private:
    static void OnSelection_(void *context, size_t index)
    {
        auto self = static_cast<Chooser *>(context);

        self->value_.Set(
            ConstReference(self->choices_).Get()[index]);
    }

private:
    Value value_;
    Choices choices_;
    Selection selection_;
};


} // namespace model


template<typename T>
struct IsModelChooser_: std::false_type {};

template<typename T, typename Access>
struct IsModelChooser_<pex::model::Chooser<T, Access>>: std::true_type {};

template<typename T>
inline constexpr bool IsModelChooser = IsModelChooser_<T>::value;


namespace control
{


template<typename Observer, typename Upstream>
class Chooser
{
public:
    static constexpr bool choicesMayChange =
        HasAccess<typename Upstream::ChoicesAccess, SetTag>;

    using Type = typename Upstream::Type;

    using Selection =
        ::pex::control::Value
        <
            Observer,
            typename Upstream::Selection,
            ::pex::GetAndSetTag
        >;

    // Choices is read-only to users of the this Control.
    using Choices =
        ::pex::control::Value
        <
            Observer,
            typename Upstream::Choices,
            ::pex::GetTag
        >;

    // Value is read-only to users of the this Control.
    using Value =
        ::pex::control::Value
        <
            Observer,
            typename Upstream::Value,
            ::pex::GetTag
        >;

    Chooser() = default;

    Chooser(Upstream &upstream)
    {
        if constexpr (IsModelChooser<Upstream>)
        {
            this->choices = Choices(upstream.choices_);
            this->selection = Selection(upstream.selection_);
            this->value = Value(upstream.value_);
        }
        else
        {
            this->choices = Choices(upstream.choices);
            this->selection = Selection(upstream.selection);
            this->value = Value(upstream.value);
        }
    }

    template<typename OtherObserver>
    Chooser(const Chooser<OtherObserver, Upstream> &other)
        :
        choices(other.choices),
        selection(other.selection),
        value(other.value)
    {

    }

    template<typename OtherObserver>
    Chooser & operator=(const Chooser<OtherObserver, Upstream> &other)
    {
        this->choices = other.choices;
        this->selection = other.selection;
        this->value = other.value;

        return *this;
    }

    Choices choices;
    Selection selection;
    Value value;
};


} // namespace control


} // namespace pex
