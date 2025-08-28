#pragma once


#include <pex/promote_control.h>
#include <pex/terminus.h>


namespace pex
{


template<typename Observer, typename Upstream_>
class SelectTerminus
{
public:
    using Upstream = Upstream_;

    static constexpr bool choicesMayChange =
        HasAccess<typename Upstream::ChoicesAccess, SetTag>;

    static constexpr bool isPexCopyable = true;

    using UpstreamControl = typename PromoteControl<Upstream>::Type;

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

    explicit SelectTerminus(const UpstreamControl &pex)
        :
        choices(pex.choices),
        selection(pex.selection),
        value(pex.value)
    {

    }

    SelectTerminus(
        Observer *observer,
        const UpstreamControl &pex,
        Callable callable)
        :
        choices(pex.choices),
        selection(pex.selection),
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

    explicit SelectTerminus(
        typename PromoteControl<Upstream>::Upstream &upstream)
        :
        choices(upstream.choices_),
        selection(upstream.selection_),
        value(upstream.value_)
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

    void Connect(Observer *observer, Callable callable)
    {
        this->value.Connect(observer, callable);
    }

    explicit operator Type () const
    {
        return this->value.Get();
    }

    Type Get() const
    {
        return this->value.Get();
    }

    bool HasModel() const
    {
        return this->choices.HasModel()
            && this->selection.HasModel()
            && this->value.HasModel();
    }

    void Notify()
    {
        this->selection.Notify();
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

public:
    Choices choices;
    Selection selection;
    Value value;
};


} // end namespace pex
