#pragma once


namespace pex
{


namespace poly
{


template
<
    ::pex::HasMinimalSupers Templates
>
template<typename GroupBase>
std::unique_ptr<MakeSuperControl<typename Templates::Supers>>
DerivedGroup<Templates>::GroupFinisher_
    ::Model<GroupBase>::CreateControl()
{
    using DerivedModel =
        typename DerivedGroup<Templates>::Model;

    static_assert(
        std::derived_from<DerivedModel, std::remove_cvref_t<decltype(*this)>>);

    using DerivedControl = typename DerivedGroup<Templates>::Control;

    auto derivedModel = dynamic_cast<DerivedModel *>(this);

    if (!derivedModel)
    {
        throw std::logic_error("Expected this class to be a base");
    }

    return std::make_unique<DerivedControl>(*derivedModel);
}

#if defined(__GNUG__) && !defined(__clang__) && !defined(_WIN32)
// Avoid bogus -Wpedantic
#ifndef DO_PRAGMA
#define DO_PRAGMA_(arg) _Pragma (#arg)
#define DO_PRAGMA(arg) DO_PRAGMA_(arg)
#endif

#define GNU_NO_PEDANTIC_PUSH \
    DO_PRAGMA(GCC diagnostic push) \
    DO_PRAGMA(GCC diagnostic ignored "-Wpedantic")

#define GNU_NO_PEDANTIC_POP \
    DO_PRAGMA(GCC diagnostic pop)

// GNU compiler needs the template keyword to parse these definitions
// correctly.
#define TEMPLATE template

#else

#define GNU_NO_PEDANTIC_PUSH
#define GNU_NO_PEDANTIC_POP
#define TEMPLATE

#endif // defined __GNUG__


GNU_NO_PEDANTIC_PUSH

template
<
    ::pex::HasMinimalSupers Templates
>
template<typename GroupBase>
std::unique_ptr<MakeSuperControl<typename Templates::Supers>>
DerivedGroup<Templates>::GroupFinisher_
    ::TEMPLATE Control<GroupBase>::Copy() const
{
    using DerivedControl =
        typename DerivedGroup<Templates>::Control;

    auto derivedControl = dynamic_cast<const DerivedControl *>(this);

    if (!derivedControl)
    {
        throw std::logic_error("Expected this class to be a base");
    }

    return std::make_unique<DerivedControl>(*derivedControl);
}


template<typename Derived, typename Base>
Derived & RequireDerived(Base &base)
{
    static_assert(std::derived_from<Derived, Base>);
    auto derived = dynamic_cast<Derived *>(&base);

    if (!derived)
    {
        throw PolyError("Mismatched polymorphic value");
    }

    return *derived;
}


template
<
    ::pex::HasMinimalSupers Templates
>
template<typename GroupBase>
DerivedGroup<Templates>::GroupFinisher_
    ::TEMPLATE Control<GroupBase>::Control(
        ::pex::poly::MakeSuperModel<typename Templates::Supers> &model)
    :
    GroupBase(RequireDerived<Upstream>(model)),
    ensemble_(),
    baseNotifier_()
{
    PEX_CONCISE_LOG(this, " from ", LookupPexName(&model));

    auto name = LookupPexName(&model);

    if (name.at(0) == 'T')
    {
        throw std::runtime_error("Maybe Terminus_");
    }

    PEX_NAME(
        fmt::format(
            "DerivedGroup<{}>::Control<{}>",
            jive::GetTypeName<Templates>(),
            jive::GetTypeName<GroupBase>()));

    PEX_MEMBER(ensemble_);
    PEX_MEMBER(baseNotifier_);
}


template
<
    ::pex::HasMinimalSupers Templates
>
template<typename GroupBase>
template<typename BaseSignal>
DerivedGroup<Templates>::GroupFinisher_
    ::TEMPLATE Control<GroupBase>::Control(
        const ControlWrapper<BaseSignal> &control)
    :
    GroupBase(),
    ensemble_(),
    baseNotifier_()
{
    PEX_CONCISE_LOG(this);

    PEX_NAME(
        fmt::format(
            "DerivedGroup<{}>::Control<{}>",
            jive::GetTypeName<Templates>(),
            jive::GetTypeName<GroupBase>()));

    PEX_MEMBER(ensemble_);
    PEX_MEMBER(baseNotifier_);

    using DerivedControl =
        typename DerivedGroup<Templates>::Control;

    auto base = control.GetVirtual();
    auto upcast = dynamic_cast<const DerivedControl *>(base);

    if (!upcast)
    {
        throw PolyError("Mismatched polymorphic value");
    }

    *this = *upcast;
}

#if defined(__GNUG__) && !defined(__clang__) && !defined(_WIN32)
GNU_NO_PEDANTIC_POP
#undef TEMPLATE
#endif


} // end namespace poly


} // end namespace pex
