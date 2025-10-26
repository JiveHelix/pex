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


// Forward declare the control::OptionalSelect
// Necessary for the friend declaration in model::OptionalSelect
namespace control
{

template<typename>
class OptionalSelect;

template<typename>
class OptionalSelectMux;


} // end namespace control


template<typename, typename>
class OptionalSelectTerminus;


namespace model
{


/**
 ** Get and Set will pass through the selected index unless it is not a valid
 ** choice. In that case, the index of the last valid choice will be returned.
 **
 **/
template<typename T>
struct OptionalSelectFilter
{
    using Choices = std::vector<T>;

    OptionalSelectFilter(const Choices &choices)
        :
        choices_(choices)
    {

    }

    std::optional<size_t> Get(const std::optional<size_t> &selectedIndex) const
    {
        if (!selectedIndex)
        {
            return {};
        }

        if (this->choices_.empty())
        {
            return {};
        }

        if (selectedIndex >= this->choices_.size())
        {
            return this->choices_.size() - 1;
        }

        return selectedIndex;
    }

    std::optional<size_t> Set(const std::optional<size_t> &selectedIndex) const
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
class OptionalSelect: public Separator
{
public:
    static constexpr bool isOptionalSelectModel = true;

    using Type = T;

    using Value = pex::model::Value<std::optional<Type>>;
    static constexpr auto observerName = "pex::model::OptionalSelect";

    // Model always has full access.
    using Access = GetAndSetTag;

    using ChoicesAccess = ChoicesAccess_;

    static_assert(
        std::same_as
        <
            std::optional<size_t>,
            detail::FilteredType<size_t, OptionalSelectFilter<Type>>
        >);

    static_assert(
        detail::MemberSetterTakesOptional<size_t, OptionalSelectFilter<Type>>);

    static_assert(detail::GetterIsMember<size_t, OptionalSelectFilter<Type>>);
    static_assert(detail::SetterIsMember<size_t, OptionalSelectFilter<Type>>);

    using Selection =
        ::pex::model::FilteredValue
        <
            std::optional<size_t>,
            OptionalSelectFilter<Type>
        >;

    using Choices = ::pex::model::Value<std::vector<Type>>;

    using Control = pex::control::Value<Selection>;

    template<typename Observer>
    using Terminus = pex::Terminus<Observer, Control>;

    template<typename>
    friend class ::pex::Reference;

    template <typename>
    friend class ::pex::control::OptionalSelect;

    template <typename>
    friend class ::pex::control::OptionalSelectMux;

    template <typename, typename>
    friend class ::pex::OptionalSelectTerminus;


private:
    using ConstReference = ::pex::ConstReference<Choices>;
    using Defer = ::pex::Defer<Choices>;

public:
    OptionalSelect()
        :
        OptionalSelect(ChoiceMaker::GetChoices())
    {
        this->Initialize_();
    }

    OptionalSelect(pex::Argument<Type> value)
        :
        OptionalSelect(
            value,
            ChoiceMaker::GetChoices())
    {
        this->Initialize_();
    }

    OptionalSelect(
        pex::Argument<Type> value,
        const std::vector<Type> &choices)
        :
        value_(value),
        choices_(choices),

        selection_(
            RequireIndex(value, choices),
            OptionalSelectFilter(this->choices_.Get())),

        terminus_(
            PEX_THIS("OptionalSelectModel"),
            Control(PEX_MEMBER_PASS(selection_)),
            &OptionalSelect::OnSelection_)
    {
        this->Initialize_();
    }

    OptionalSelect(const std::vector<Type> &choices)
        :
        value_{},
        choices_(choices),
        selection_(0, OptionalSelectFilter(this->choices_.Get())),
        terminus_(
            PEX_THIS("OptionalSelectModel"),
            Control(PEX_MEMBER_PASS(selection_)),
            &OptionalSelect::OnSelection_)
    {
        this->Initialize_();

        if (!choices.empty())
        {
            this->value_.Set(choices.front());
        }
    }

    ~OptionalSelect()
    {
        PEX_CLEAR_NAME(this);
        PEX_CLEAR_NAME(&this->value_);
        PEX_CLEAR_NAME(&this->choices_);
        PEX_CLEAR_NAME(&this->selection_);
    }

    OptionalSelect(const OptionalSelect &) = delete;
    OptionalSelect(OptionalSelect &&) = delete;
    OptionalSelect & operator=(const OptionalSelect &) = delete;
    OptionalSelect & operator=(OptionalSelect &&) = delete;

    OptionalSelect & operator=(pex::Argument<Type> value)
    {
        this->SetValue(value);

        return *this;
    }

    void Initialize_()
    {
        PEX_NAME("OptionalSelectModel");
        PEX_MEMBER(value_);
        PEX_MEMBER(choices_);
        PEX_MEMBER(selection_);
    }

    std::optional<Type> Get() const
    {
        return this->value_.Get();
    }

    explicit operator std::optional<Type> () const
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

        auto selection = this->selection_.Get();

        if (selection)
        {
            if (*selection >= choices.size())
            {
                // The previously selected index is no longer available
                // Cancel the selection.
                this->selection_.Set(std::nullopt);
            }
            else
            {
                // OnSelection_ must be called to update value_.
                this->OnSelection_(selection);
            }
        }

        this->selection_.SetFilter(OptionalSelectFilter(this->choices_.Get()));
    }

    void SetSelection(std::optional<size_t> index)
    {
        // The OnSelection_ callback should said value_ to std::nullopt;
        this->selection_.Set(index);
    }

    void SetValue(const std::optional<Type> &value)
    {
        if (value)
        {
            // value exists, so it must be in the choices_ vector.
            this->selection_.Set(
                RequireIndex(
                    *value,
                    ConstReference(this->choices_).Get()));
        }
        else
        {
            this->selection_.Set(value);
        }
    }

    std::optional<size_t> GetSelectedIndex() const
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
    void SetInitial(const std::optional<Type> &value)
    {
        if (!value)
        {

            detail::AccessReference(this->selection_)
                .SetWithoutNotify(std::nullopt);

            detail::AccessReference(this->value_)
                .SetWithoutNotify(std::nullopt);

            return;
        }

        assert(value.has_value());

        auto choices = this->choices_.Get();
        auto index = FindIndex(*value, choices);

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
                    *value,
                    ConstReference(this->choices_).Get()));

        detail::AccessReference(this->value_)
            .SetWithoutNotify(*value);
    }

    void Notify()
    {
        this->selection_.Notify();
    }

private:
    void OnSelection_(const std::optional<size_t> &index)
    {
        if (!index)
        {
            this->value_.Set(std::nullopt);
        }
        else
        {
            this->value_.Set(
                ConstReference(this->choices_).Get().at(*index));
        }
    }

    // This method is used to set data using a Plain representation, which uses
    // the Value type rather than the index.
    void SetWithoutNotify_(const std::optional<Type> &value)
    {
        if (!value)
        {
            this->value_.Set(std::nullopt);
            this->selection_.Set(std::nullopt);

            return;
        }

        detail::AccessReference(this->selection_)
            .SetWithoutNotify(
                RequireIndex(
                    *value,
                    ConstReference(this->choices_).Get()));

        detail::AccessReference(this->value_)
            .SetWithoutNotify(value);
    }

private:
    Value value_;
    Choices choices_;
    Selection selection_;
    Terminus<OptionalSelect> terminus_;
};


} // namespace model


namespace control
{


template<typename Upstream_>
class OptionalSelect
{
public:
    static constexpr bool isOptionalSelectControl = true;

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

    OptionalSelect()
        :
        choices(),
        selection(),
        value()
    {

    }

    OptionalSelect(Upstream &upstream)
    {
        if constexpr (IsOptionalSelectModel<Upstream>)
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

    std::optional<Type> Get() const
    {
        return this->value.Get();
    }

    explicit operator std::optional<Type> () const
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
    void SetWithoutNotify_(const std::optional<Type> &value_)
    {
        if (!value_)
        {
            detail::AccessReference(this->selection).
                SetWithoutNotify(value_);

            return;
        }

        assert(value_.has_value());

        detail::AccessReference(this->selection).
            SetWithoutNotify(
                RequireIndex(
                    *value_,
                    ConstControlReference(this->choices).Get()));
    }

public:
    Choices choices;
    Selection selection;
    Value value;
};


template<typename Upstream_>
class OptionalSelectMux
{
public:
    static_assert(IsOptionalSelectModel<Upstream_>);

    using Upstream = Upstream_;

    using Filter = ::pex::NoFilter;

    using Access = GetAndSetTag;
    using ChoicesAccess = typename Upstream::ChoicesAccess;

    static constexpr bool choicesMayChange =
        HasAccess<typename Upstream::ChoicesAccess, SetTag>;

    using Type = typename Upstream::Type;

    static constexpr bool isOptionalSelectMux = true;
    static constexpr bool isPexCopyable = false;

    using Selection = ::pex::control::Mux<typename Upstream::Selection>;

    // Choices is read-only to users of this Control.
    using Choices = ::pex::control::Mux<typename Upstream::Choices>;

    // Value is read-only to users of this Control.
    using Value = ::pex::control::Mux<typename Upstream::Value>;

    template<typename>
    friend class ::pex::Reference;

    OptionalSelectMux()
        :
        choices(),
        selection(),
        value()
    {

    }

    OptionalSelectMux(Upstream &upstream)
    {
        this->ChangeUpstream(upstream);
    }

    void ChangeUpstream(Upstream &upstream)
    {
        this->choices.ChangeUpstream(upstream.choices_);
        this->selection.ChangeUpstream(upstream.selection_);
        this->value.ChangeUpstream(upstream.value_);
    }

    std::optional<Type> Get() const
    {
        return this->value.Get();
    }

    explicit operator std::optional<Type> () const
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
    void SetWithoutNotify_(const std::optional<Type> &value_)
    {
        if (!value_)
        {
            detail::AccessReference(this->selection)
                .SetWithoutNotify(value_);

            return;
        }

        detail::AccessReference(this->selection).
            SetWithoutNotify(
                RequireIndex(
                    *value_,
                    ConstControlReference(this->choices).Get()));
    }

public:
    Choices choices;
    Selection selection;
    Value value;
};


template<typename Upstream>
struct OptionalSelectFollow: public OptionalSelect<Upstream>
{
public:
    static constexpr auto isOptionalSelectControl = false;
    static constexpr auto isOptionalSelectFollow = true;

    using Base = OptionalSelect<Upstream>;
    using Base::Base;
};


} // namespace control




} // namespace pex
