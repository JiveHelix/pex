#pragma once


namespace pex
{


namespace poly
{


template
<
    template<typename> typename Fields,
    ::pex::HasMinimalSupers Templates
>
template<typename GroupBase>
std::shared_ptr<MakeControlSuper<typename Templates::Supers>>
Poly<Fields, Templates>::GroupTemplates_
    ::Model<GroupBase>::CreateControl()
{
    using DerivedModel =
        typename Poly<Fields, Templates>::Model;

    static_assert(
        std::derived_from<DerivedModel, std::remove_cvref_t<decltype(*this)>>);

    using Control =
        typename Poly<Fields, Templates>::Control;

    auto derivedModel = dynamic_cast<DerivedModel *>(this);

    if (!derivedModel)
    {
        throw std::logic_error("Expected this class to be a base");
    }

    return std::make_shared<Control>(*derivedModel);
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
    template<typename> typename Fields,
    ::pex::HasMinimalSupers Templates
>
template<typename GroupBase>
std::shared_ptr<MakeControlSuper<typename Templates::Supers>>
Poly<Fields, Templates>::GroupTemplates_
    ::TEMPLATE Control<GroupBase>::Copy() const
{
    using DerivedControl = typename Poly<Fields, Templates>::Control;

    auto derivedControl = dynamic_cast<const DerivedControl *>(this);

    if (!derivedControl)
    {
        throw std::logic_error("Expected this class to be a base");
    }

    return std::make_shared<DerivedControl>(*derivedControl);
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
    template<typename> typename Fields,
    ::pex::HasMinimalSupers Templates
>
template<typename GroupBase>
Poly<Fields, Templates>::GroupTemplates_
    ::TEMPLATE Control<GroupBase>::Control(
        ::pex::poly::MakeModelSuper<typename Templates::Supers> &model)
    :
    GroupBase(RequireDerived<Upstream>(model)),
    aggregate_(),
    baseNotifier_()
{
    REGISTER_PEX_NAME(
        this,
        fmt::format(
            "Poly<Fields, {}>::Control<{}>",
            jive::GetTypeName<Templates>(),
            jive::GetTypeName<GroupBase>()));

    REGISTER_PEX_PARENT(aggregate_);
    REGISTER_PEX_PARENT(baseNotifier_);
}


template
<
    template<typename> typename Fields,
    ::pex::HasMinimalSupers Templates
>
template<typename GroupBase>
Poly<Fields, Templates>::GroupTemplates_
    ::TEMPLATE Control<GroupBase>::Control(
        const ::pex::poly::Control<typename Templates::Supers> &control)
    :
    GroupBase(),
    aggregate_(),
    baseNotifier_()
{
    REGISTER_PEX_NAME(
        this,
        fmt::format(
            "Poly<Fields, {}>::Control<{}>",
            jive::GetTypeName<Templates>(),
            jive::GetTypeName<GroupBase>()));

    REGISTER_PEX_PARENT(aggregate_);
    REGISTER_PEX_PARENT(baseNotifier_);

    using DerivedControl = typename Poly<Fields, Templates>::Control;

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
