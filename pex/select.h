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
#include <pex/terminus.h>


namespace pex
{


// Forward declare the control::Select
// Necessary for the friend declaration in model::Select
namespace control
{

template<typename>
class Select;

template<typename>
class SelectMux;


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
class Select: public Separator
{
public:
    static constexpr bool isSelectModel = true;

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
    using Terminus = pex::Terminus<Observer, Control>;

    template<typename>
    friend class ::pex::Reference;

    template <typename>
    friend class ::pex::control::Select;

    template <typename>
    friend class ::pex::control::SelectMux;

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
        this->Initialize_();
    }

    Select(pex::Argument<Type> value)
        :
        Select(
            value,
            ChoiceMaker::GetChoices())
    {
        this->Initialize_();
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
            PEX_THIS("SelectModel"),
            Control(PEX_MEMBER_PASS(selection_)),
            &Select::OnSelection_)
    {
        this->Initialize_();
    }

    Select(const std::vector<Type> &choices)
        :
        value_(choices.front()),
        choices_(choices),
        selection_(0, SelectFilter(this->choices_.Get())),
        terminus_(
            PEX_THIS("SelectModel"),
            Control(PEX_MEMBER_PASS(selection_)),
            &Select::OnSelection_)
    {
        this->Initialize_();
    }

    ~Select()
    {
        PEX_CLEAR_NAME(this);
        PEX_CLEAR_NAME(&this->value_);
        PEX_CLEAR_NAME(&this->choices_);
        PEX_CLEAR_NAME(&this->selection_);
    }

    Select(const Select &) = delete;
    Select(Select &&) = delete;
    Select & operator=(const Select &) = delete;
    Select & operator=(Select &&) = delete;

    Select & operator=(pex::Argument<Type> value)
    {
        this->SetValue(value);

        return *this;
    }

    void Initialize_()
    {
        PEX_NAME("SelectModel");
        PEX_MEMBER(value_);
        PEX_MEMBER(choices_);
        PEX_MEMBER(selection_);
    }

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

    void Notify()
    {
        this->selection_.Notify();
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

private:
    Value value_;
    Choices choices_;
    Selection selection_;
    Terminus<Select> terminus_;
};


} // namespace model


namespace control
{


template<typename Upstream_>
class Select
{
public:
    static constexpr bool isSelectControl = true;

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
        if constexpr (IsSelectModel<Upstream>)
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

    void Notify()
    {
        this->selection.Notify();
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

public:
    Choices choices;
    Selection selection;
    Value value;
};


template<typename Upstream_>
class SelectMux
{
public:
    static_assert(IsSelectModel<Upstream_>);

    using Upstream = Upstream_;

    using Filter = ::pex::NoFilter;

    using Access = GetAndSetTag;
    using ChoicesAccess = typename Upstream::ChoicesAccess;

    static constexpr bool choicesMayChange =
        HasAccess<typename Upstream::ChoicesAccess, SetTag>;

    using Type = typename Upstream::Type;

    static constexpr bool isSelectMux = true;
    static constexpr bool isPexCopyable = false;

    using Selection = ::pex::control::Mux<typename Upstream::Selection>;

    // Choices is read-only to users of this Control.
    using Choices = ::pex::control::Mux<typename Upstream::Choices>;

    // Value is read-only to users of this Control.
    using Value = ::pex::control::Mux<typename Upstream::Value>;

    template<typename>
    friend class ::pex::Reference;

    SelectMux()
        :
        choices(),
        selection(),
        value()
    {

    }

    SelectMux(Upstream &upstream)
    {
        this->ChangeUpstream(upstream);
    }

    void ChangeUpstream(Upstream &upstream)
    {
        this->choices.ChangeUpstream(upstream.choices_);
        this->selection.ChangeUpstream(upstream.selection_);
        this->value.ChangeUpstream(upstream.value_);
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

    void Notify()
    {
        this->selection.Notify();
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

public:
    Choices choices;
    Selection selection;
    Value value;
};


template<typename Upstream>
struct SelectFollow: public Select<Upstream>
{
public:
    static constexpr auto isSelectControl = false;
    static constexpr auto isSelectFollow = true;

    using Base = Select<Upstream>;
    using Base::Base;
};


} // namespace control




} // namespace pex
