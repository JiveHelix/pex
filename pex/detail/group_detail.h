#pragma once


#include <pex/detail/mute.h>
#include <pex/detail/ensemble.h>
#include <pex/detail/has_model.h>
#include <pex/detail/choose_not_void.h>
#include <pex/detail/traits.h>


namespace pex
{


namespace detail
{


template<typename Finisher, typename T, typename = void>
struct FinishPlain_
{
    using Type = T;
};


template<typename Finisher, typename T>
struct FinishPlain_
<
    Finisher,
    T,
    std::enable_if_t<HasPlainTemplate<Finisher, T>>
>
{
    using Type = typename Finisher::template Plain<T>;
};


template<typename Finisher, typename T>
struct FinishPlain_
<
    Finisher,
    T,
    std::enable_if_t
    <
        HasPlain<Finisher> && !HasPlainTemplate<Finisher, T>
    >
>
{
    using Type = typename Finisher::Plain;
};


template<typename Finisher, typename T>
using FinishPlain = typename FinishPlain_<Finisher, T>::Type;


template<typename Finisher, typename T, typename = void>
struct FinishModel_
{
    using Type = T;
};


template<typename Finisher, typename T>
struct FinishModel_
<
    Finisher,
    T,
    std::enable_if_t<HasModelTemplate<Finisher, T>>
>
{
    using Type = typename Finisher::template Model<T>;
};


template<typename Finisher, typename T>
struct FinishModel_
<
    Finisher,
    T,
    std::enable_if_t
    <
        DeclaresModel<Finisher> && !HasModelTemplate<Finisher, T>
    >
>
{
    using Type = typename Finisher::Model;
};


template<typename Finisher, typename T>
using FinishModel = typename FinishModel_<Finisher, T>::Type;


template<typename Target, typename Source>
void DoAssignEmplace(Target &target, Source && source) noexcept
{
    target.Emplace(std::forward<Source>(source));
}

template
<
    typename Target,
    typename Source
>
void AssignEmplace(Target &&target, Source &&source)
{
    using TargetType = std::remove_cvref_t<Target>;
    using SourceType  = std::remove_cvref_t<Source>;

    if constexpr (fields::HasFields<TargetType>
            && fields::HasFields<SourceType>)
    {
        auto initializer = [&target, &source](
            const auto &targetField,
            const auto &sourceField) -> void
        {
            DoAssignEmplace(
                target.*(targetField.member),
                source.*(sourceField.member));
        };

        jive::ZipApply(
            initializer,
            target.fields,
            source.fields);
    }
    else
    {
        auto initializer = [](
            auto &targetMember,
            auto &&sourceMember) -> void
        {
            targetMember.Emplace(
                std::forward<decltype(sourceMember)>(sourceMember));
        };

        fields::ForEachZip(target, source, initializer);
    }
}


template<typename Base>
struct StandardEmplace: public Base
{
    using Base::Base;
    using Upstream = typename Base::Upstream;

    void Emplace(Upstream &upstream)
    {
        this->StandardEmplace_(upstream);
    }

    void Emplace(const Base &other)
    {
        this->StandardEmplace_(other);
    }
};


template<typename Finisher, typename T, typename = void>
struct FinishControl_
{
    using Type = StandardEmplace<T>;
};


template<typename Finisher, typename T>
concept HasValidControlTemplate =
    HasControlTemplate<Finisher, T>
    && HasEmplaceUpstream<typename Finisher::template Control<T>>
    && HasEmplaceCopy<typename Finisher::template Control<T>>;


template<typename Finisher, typename T>
struct FinishControl_
<
    Finisher,
    T,
    std::enable_if_t<HasControlTemplate<Finisher, T>>
>
{
#if 0
    // This was meant to help track down compile errors.
    // Without Emplace implemented in custom controls, compilation will fail.
    static_assert(
        HasValidControlTemplate<Finisher, T>,
        "Expected customized Control template to override "
        "Emplace(Upstream &) and Emplace(const Control &)");
#endif

    using Type = typename Finisher::template Control<T>;
};

template<typename Finisher, typename T>
using FinishControl = typename FinishControl_<Finisher, T>::Type;


template<typename Finisher, typename T, typename = void>
struct FinishMux_
{
    using Type = StandardEmplace<T>;
};

template<typename Finisher, typename T>
struct FinishMux_
<
    Finisher,
    T,
    std::enable_if_t<HasMuxTemplate<Finisher, T>>
>
{
    using Type = typename Finisher::template Mux<T>;
};

template<typename Finisher, typename T>
using FinishMux = typename FinishMux_<Finisher, T>::Type;


template<typename Finisher, typename T, typename = void>
struct FinishFollow_
{
    using Type = StandardEmplace<T>;
};

template<typename Finisher, typename T>
struct FinishFollow_
<
    Finisher,
    T,
    std::enable_if_t<HasFollowTemplate<Finisher, T>>
>
{
    using Type = typename Finisher::template Follow<T>;
};

template<typename Finisher, typename T>
using FinishFollow = typename FinishFollow_<Finisher, T>::Type;


template
<
    typename Finisher,
    typename PlainBase,
    typename ModelBase,
    typename ControlBase,
    typename MuxBase,
    typename FollowBase,
    typename = void
>
struct CheckFinisher_: std::false_type {};

template
<
    typename Finisher,
    typename PlainBase,
    typename ModelBase,
    typename ControlBase,
    typename MuxBase,
    typename FollowBase
>
struct CheckFinisher_
<
    Finisher,
    PlainBase,
    ModelBase,
    ControlBase,
    MuxBase,
    FollowBase,
    std::enable_if_t
    <
        (
            HasPlainTemplate<Finisher, PlainBase>
            || HasPlain<Finisher>
            || HasModelTemplate<Finisher, ModelBase>
            || DeclaresModel<Finisher>
            || HasControlTemplate<Finisher, ControlBase>
            || HasMuxTemplate<Finisher, MuxBase>
            || HasFollowTemplate<Finisher, FollowBase>)
    >
>: std::true_type {};


template
<
    typename Finisher,
    typename PlainBase,
    typename ModelBase,
    typename ControlBase,
    typename MuxBase,
    typename FollowBase
>
inline constexpr bool CheckFinisher =
    CheckFinisher_
    <
        Finisher,
        PlainBase,
        ModelBase,
        ControlBase,
        MuxBase,
        FollowBase
    >::value;


} // end namespace detail


} // end namespace pex
