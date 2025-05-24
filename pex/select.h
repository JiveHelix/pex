/**
  * @file select.h
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
#include "pex/argument.h"
#include "pex/value.h"
#include "pex/reference.h"
#include "pex/find_index.h"
#include "pex/traits.h"
#include "pex/make_control.h"


namespace pex
{


// Forward declare the control::Select
// Necessary for the friend declaration in model::Select
namespace control
{

template<typename>
class Select;


} // end namespace control


template<typename, typename>
class SelectTerminus;


namespace model
{


/**
 ** Get and Set will pass through the selected index unless it is not a valid
 ** choice. In that case, the index of the last valid choice will be returned.
 **
 **/
template<typename T>
struct SelectFilter
{
    using Choices = std::vector<T>;

    SelectFilter(const Choices &choices)
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


template
<
    typename T,
    typename ChoiceMaker,
    typename ChoicesAccess_ = GetAndSetTag
>
class Select
{
public:
    using Type = T;

    using Value = pex::model::Value<Type>;
    static constexpr auto observerName = "pex::model::Select";

    // Model always has full access.
    using Access = GetAndSetTag;

    using ChoicesAccess = ChoicesAccess_;
    using Selection = ::pex::model::FilteredValue<size_t, SelectFilter<Type>>;
    using Choices = ::pex::model::Value<std::vector<Type>>;

    using Control = pex::control::Value<Selection>;

    template<typename Observer>
    using Terminus = pex::Terminus<Observer, Selection>;

    template<typename>
    friend class ::pex::Reference;

    template <typename>
    friend class ::pex::control::Select;

    template <typename, typename>
    friend class ::pex::SelectTerminus;


private:
    using ConstReference = ::pex::ConstReference<Choices>;
    using Defer = ::pex::Defer<Choices>;

public:
    Select()
        :
        Select(ChoiceMaker::GetChoices())
    {

    }

    Select(pex::Argument<Type> value)
        :
        Select(
            value,
            ChoiceMaker::GetChoices())
    {

    }

    Select(
        pex::Argument<Type> value,
        const std::vector<Type> &choices)
        :
        value_(value),
        choices_(choices),
        selection_(
            RequireIndex(value, choices),
            SelectFilter(this->choices_.Get())),
        terminus_(
            this,
            this->selection_)
    {
        this->terminus_.Connect(&Select::OnSelection_);
    }

    Select(const std::vector<Type> &choices)
        :
        value_(choices.front()),
        choices_(choices),
        selection_(0, SelectFilter(this->choices_.Get())),
        terminus_(
            this,
            this->selection_)
    {
        this->terminus_.Connect(&Select::OnSelection_);
    }

    // Unlike model::Value and control::Value, which Set/Get the same type,
    // this class Gets the actual value, but Sets the index of the selection.
    Type Get() const
    {
        return this->value_.Get();
    }

    explicit operator Type () const
    {
        return this->value_.Get();
    }

    explicit operator Control ()
    {
        return Control(this->selection_);
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
        else
        {
            // OnSelection_ must be called to update value_.
            this->OnSelection_(this->selection_.Get());
        }

        this->selection_.SetFilter(SelectFilter(this->choices_.Get()));
    }

    void SetSelection(size_t index)
    {
        this->selection_.Set(index);
    }

    void SetValue(Argument<Type> value)
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
            LookupPexName(&this->value_),
            " with ",
            LookupPexName(context));

        this->value_.Connect(context, callable);
    }

    void Disconnect(void * context)
    {
        PEX_LOG(
            this,
            " calling Disconnect on ",
            LookupPexName(&this->value_),
            " with ",
            LookupPexName(context));

        this->value_.Disconnect(context);
    }

    // Initialize values without sending notifications.
    void SetInitial(pex::Argument<Type> value)
    {
        auto choices = this->choices_.Get();
        auto index = FindIndex(value, choices);

        if (index < 0)
        {
            // TODO:
            // During initialization, SetInitial may be called with
            // the default constructed T.
            //
            // Leave selection_ unchanged for now.

            return;
        }

        detail::AccessReference(this->selection_)
            .SetWithoutNotify(
                RequireIndex(
                    value,
                    ConstReference(this->choices_).Get()));

        detail::AccessReference(this->value_)
            .SetWithoutNotify(value);
    }


private:
    void OnSelection_(size_t index)
    {
        this->value_.Set(
            ConstReference(this->choices_).Get().at(index));
    }

    // This method is used to set data using a Plain representation, which uses
    // the Value type rather than the index.
    void SetWithoutNotify_(pex::Argument<Type> value)
    {
        auto choices = this->choices_.Get();
        auto index = FindIndex(value, choices);

        if (index < 0)
        {
            // After choices has been set, all values must already be in
            // choices.
            throw std::out_of_range("Value not a valid choice.");
        }

        detail::AccessReference(this->selection_)
            .SetWithoutNotify(
                RequireIndex(
                    value,
                    ConstReference(this->choices_).Get()));

        detail::AccessReference(this->value_)
            .SetWithoutNotify(value);
    }

    void DoNotify_()
    {
        detail::AccessReference(this->selection_).DoNotify();
    }

private:
    Value value_;
    Choices choices_;
    Selection selection_;
    Terminus<Select> terminus_;
};


} // namespace model


template<typename T>
struct IsModelSelect_: std::false_type {};

template<typename T, typename Choices, typename Access>
struct IsModelSelect_<pex::model::Select<T, Choices, Access>>
    : std::true_type {};

template<typename T>
inline constexpr bool IsModelSelect = IsModelSelect_<T>::value;


namespace control
{


template<typename Upstream_>
class Select
{
public:
    using Upstream = Upstream_;

    using Filter = ::pex::NoFilter;

    using Access = GetAndSetTag;
    using ChoicesAccess = typename Upstream::ChoicesAccess;

    static constexpr bool choicesMayChange =
        HasAccess<typename Upstream::ChoicesAccess, SetTag>;

    using Type = typename Upstream::Type;

    static constexpr bool isPexCopyable = true;

    using Selection = ::pex::control::Value
        <
            typename Upstream::Selection,
            ::pex::GetAndSetTag
        >;

    // Choices is read-only to users of this Control.
    using Choices =
        ::pex::control::Value
        <
            typename Upstream::Choices,
            ::pex::GetTag
        >;

    // Value is read-only to users of this Control.
    using Value =
        ::pex::control::Value
        <
            typename Upstream::Value,
            ::pex::GetTag
        >;

    template<typename>
    friend class ::pex::Reference;

    Select()
        :
        choices(),
        selection(),
        value()
    {

    }

    Select(Upstream &upstream)
    {
        if constexpr (IsModelSelect<Upstream>)
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

    Type Get() const
    {
        return this->value.Get();
    }

    explicit operator Type () const
    {
        return this->value.Get();
    }

    bool HasModel() const
    {
        return this->value.HasModel()
            && this->selection.HasModel()
            && this->value.HasModel();
    }

    void Connect(void *observer, typename Value::Callable callable)
    {
        this->value.Connect(observer, callable);
    }

    void Disconnect(void *observer)
    {
        this->value.Disconnect(observer);
    }

private:
    void SetWithoutNotify_(pex::Argument<Type> value_)
    {
        detail::AccessReference(this->selection).
            SetWithoutNotify(
                RequireIndex(
                    value_,
                    ConstControlReference(this->choices).Get()));
    }

    void DoNotify_()
    {
        detail::AccessReference(this->selection).DoNotify();
    }

public:
    Choices choices;
    Selection selection;
    Value value;
};


} // namespace control


template<typename ...T>
struct IsControlSelect_: std::false_type {};

template<typename ...T>
struct IsControlSelect_<control::Select<T...>>: std::true_type {};

template<typename ...T>
inline constexpr bool IsControlSelect = IsControlSelect_<T...>::value;


// Specializations of MakeControl for SelectTerminus.
template<typename P>
struct MakeControl<P, std::enable_if_t<IsModelSelect<P>>>
{
    using Control = control::Select<P>;
    using Upstream = P;
};


template<typename P>
struct MakeControl<P, std::enable_if_t<IsControlSelect<P>>>
{
    using Control = P;
    using Upstream = typename P::Upstream;
};


template<typename P>
inline constexpr bool IsSelect = IsControlSelect<P> || IsModelSelect<P>;


template<typename Observer, typename Upstream_>
class SelectTerminus
{
public:
    using Upstream = Upstream_;

    static constexpr bool choicesMayChange =
        HasAccess<typename Upstream::ChoicesAccess, SetTag>;

    static constexpr bool isPexCopyable = true;

    using UpstreamControl = typename MakeControl<Upstream>::Control;

    using Type = typename UpstreamControl::Type;

    using Selection =
        pex::Terminus<Observer, typename UpstreamControl::Selection>;

    using Choices =
        pex::Terminus<Observer, typename UpstreamControl::Choices>;

    using Value =
        pex::Terminus<Observer, typename UpstreamControl::Value>;

    using Callable = typename Value::Callable;

    SelectTerminus()
        :
        choices{},
        selection{},
        value{}
    {

    }

    SelectTerminus(Observer *observer, const UpstreamControl &pex)
        :
        choices(observer, pex.choices),
        selection(observer, pex.selection),
        value(observer, pex.value)
    {

    }

    SelectTerminus(
        Observer *observer,
        const UpstreamControl &pex,
        Callable callable)
        :
        choices(observer, pex.choices),
        selection(observer, pex.selection),
        value(observer, pex.value, callable)
    {

    }

    SelectTerminus(Observer *observer, UpstreamControl &&pex)
        :
        choices(observer, std::move(pex.choices)),
        selection(observer, std::move(pex.selection)),
        value(observer, std::move(pex.value))
    {

    }

    SelectTerminus(
        Observer *observer,
        typename MakeControl<Upstream>::Upstream &upstream)
        :
        choices(observer, upstream.choices_),
        selection(observer, upstream.selection_),
        value(observer, upstream.value_)
    {

    }

    SelectTerminus(const SelectTerminus &other) = delete;
    SelectTerminus(SelectTerminus &&other) = delete;
    SelectTerminus & operator=(const SelectTerminus &) = delete;
    SelectTerminus & operator=(SelectTerminus &&) = delete;

    // Copy construct
    SelectTerminus(Observer *observer, const SelectTerminus &other)
        :
        choices(observer, other.choices),
        selection(observer, other.selection),
        value(observer, other.value)
    {

    }

    // Copy construct from other observer
    template<typename O>
    SelectTerminus(
        Observer *observer,
        const SelectTerminus<O, Upstream> &other)
        :
        choices(observer, other.choices),
        selection(observer, other.selection),
        value(observer, other.value)
    {

    }

    // Move construct
    SelectTerminus(Observer *observer, SelectTerminus &&other)
        :
        choices(observer, std::move(other.choices)),
        selection(observer, std::move(other.selection)),
        value(observer, std::move(other.value))
    {

    }

    // Move construct from other observer
    template<typename O>
    SelectTerminus(
        Observer *observer,
        SelectTerminus<O, Upstream> &&other)
        :
        choices(observer, std::move(other.choices)),
        selection(observer, std::move(other.selection)),
        value(observer, std::move(other.value))
    {

    }

    void Disconnect(Observer *)
    {
        this->Disconnect();
    }

    void Disconnect()
    {
        this->choices.Disconnect();
        this->selection.Disconnect();
        this->value.Disconnect();
    }

    // Copy assign
    template<typename O>
    SelectTerminus & Assign(
        Observer *observer,
        const SelectTerminus<O, Upstream> &other)
    {
        this->choices.Assign(observer, other.choices);
        this->selection.Assign(observer, other.selection);
        this->value.Assign(observer, other.value);

        return *this;
    }

    // Move assign
    template<typename O>
    SelectTerminus & Assign(
        Observer *observer,
        SelectTerminus<O, Upstream> &&other)
    {
        this->choices.Assign(observer, std::move(other.choices));
        this->selection.Assign(observer, std::move(other.selection));
        this->value.Assign(observer, std::move(other.value));

        return *this;
    }

    void Connect(Callable callable)
    {
        this->value.Connect(callable);
    }

    explicit operator Type () const
    {
        return this->value.Get();
    }

    Type Get() const
    {
        return this->value.Get();
    }

#if 0
    void Set(size_t selectionIndex)
    {
        this->selection.Set(selectionIndex);
    }
#endif

    bool HasModel() const
    {
        return this->choices.HasModel()
            && this->selection.HasModel()
            && this->value.HasModel();
    }

    template<typename>
    friend class ::pex::Reference;

private:
    void SetWithoutNotify_(pex::Argument<Type> value_)
    {
        detail::AccessReference(this->selection).
            SetWithoutNotify(
                RequireIndex(
                    value_,
                    this->choices.Get()));
    }

    void DoNotify_()
    {
        detail::AccessReference(this->selection).DoNotify();
    }

public:
    Choices choices;
    Selection selection;
    Value value;
};


} // namespace pex
