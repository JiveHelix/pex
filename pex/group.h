#pragma once


#include <fields/fields.h>

#include "pex/selectors.h"
#include "pex/accessors.h"
#include "pex/traits.h"
#include "pex/detail/mute.h"
#include "pex/detail/aggregate.h"
#include "pex/detail/initialize_terminus.h"
#include "pex/detail/choose_not_void.h"
#include "pex/detail/traits.h"


/**

// The Group struct uses a Fields type and a Template type to create a POD
// struct and corresponding Model and Control.

// Fields must accept a single template parameter for any type that defines the
// member fields, like this:

template<typename T>
struct GpsFields
{
    static constexpr auto fields = std::make_tuple(
        fields::Field(&T::time, "time"),
        fields::Field(&T::latitude, "latitude"),
        fields::Field(&T::longitude, "longitude"),
        fields::Field(&T::elevation, "elevation"));
};


// The Template type defines the members with template parameter types. This
// allows the definition of the POD struct with pex::Identity, and the Model
// and Control classes.

template<template<typename> typename T>
struct GpsTemplate
{
    T<int64_t> time;
    T<double> latitude;
    T<double> longitude;
    T<double> elevation;

    static constexpr auto fields = GpsFields<MovieTemplate>::fields;
};

// Create the Plain-old-data structure, Model, and Control types
using GpsGroup = Group<GpsFields, GpsTemplate>

using Gps = typename GpsGroup::Plain;
using GpsModel = typename GpsGroup::Model;
using GpsControl = typename GpsGroup::Control;

**/


namespace pex
{


namespace detail
{


template<typename Custom, typename T, typename = void>
struct CustomizePlain_
{
    using Type = T;
};


template<typename Custom, typename T>
struct CustomizePlain_<Custom, T, std::enable_if_t<HasPlainTemplate<Custom, T>>>
{
    using Type = typename Custom::template Plain<T>;
};


template<typename Custom, typename T>
struct CustomizePlain_
<
    Custom,
    T,
    std::enable_if_t
    <
        HasPlain<Custom> && !HasPlainTemplate<Custom, T>
    >
>
{
    using Type = typename Custom::Plain;
};


template<typename Custom, typename T>
using CustomizePlain = typename CustomizePlain_<Custom, T>::Type;


template<typename Custom, typename T, typename = void>
struct CustomizeModel_
{
    using Type = T;
};


template<typename Custom, typename T>
struct CustomizeModel_<Custom, T, std::enable_if_t<HasModelTemplate<Custom, T>>>
{
    using Type = typename Custom::template Model<T>;
};

template<typename Custom, typename T>
using CustomizeModel = typename CustomizeModel_<Custom, T>::Type;


template<typename Custom, typename T, typename = void>
struct CustomizeControl_
{
    using Type = T;
};

template<typename Custom, typename T>
struct CustomizeControl_
<
    Custom,
    T,
    std::enable_if_t<HasControlTemplate<Custom, T>>
>
{
    using Type = typename Custom::template Control<T>;
};

template<typename Custom, typename T>
using CustomizeControl = typename CustomizeControl_<Custom, T>::Type;


template
<
    typename Custom,
    typename PlainBase,
    typename ModelBase,
    typename ControlBase,
    typename = void
>
struct CheckCustom_: std::false_type {};

template
<
    typename Custom,
    typename PlainBase,
    typename ModelBase,
    typename ControlBase
>
struct CheckCustom_
<
    Custom,
    PlainBase,
    ModelBase,
    ControlBase,
    std::enable_if_t
    <
        (
            HasPlainTemplate<Custom, PlainBase>
            || HasPlain<Custom>
            || HasModelTemplate<Custom, ModelBase>
            || HasControlTemplate<Custom, ControlBase>)
    >
>: std::true_type {};


template
<
    typename Custom,
    typename PlainBase,
    typename ModelBase,
    typename ControlBase
>
inline constexpr bool CheckCustom =
    CheckCustom_<Custom, PlainBase, ModelBase, ControlBase>::value;


} // end namespace detail


#if 1
template<template<typename> typename T>
struct PlainU
{
    template<typename U>
    using Plain = T<U>;
};

template<typename T>
struct PlainT
{
    using Plain = T;
};
#endif


template
<
    template<template<typename> typename> typename Template
>
class ControlMembers_
    :
    public Template<ControlSelector>
{
protected:
    ControlMembers_() = default;
};


template
<
    template<typename> typename Fields_,
    template<template<typename> typename> typename Template_,
    typename Custom = void
>
struct Group
{
    static constexpr bool isGroup = true;

    template<typename T>
    using Fields = Fields_<T>;

    template<template<typename> typename T>
    using Template = Template_<T>;

    static_assert(
        std::is_void_v<Custom>
            || detail::CheckCustom
                <
                    Custom,
                    Template<pex::Identity>,
                    Template<ModelSelector>,
                    Template<ControlSelector>
                >,
        "Expected at least one customization");

    using Plain = detail::CustomizePlain<Custom, Template<pex::Identity>>;
    using Type = Plain;

    template<template<typename> typename Selector, typename Upstream>
    using DeferGroup = DeferGroup<Fields, Template, Selector, Upstream>;

    template<template<typename> typename Selector>
    using DeferredGroup = detail::DeferredGroup<Fields, Template, Selector>;

    template<typename Derived>
    using ModelAccessors = GroupAccessors
        <
            Plain,
            Fields,
            Template_,
            ModelSelector,
            Derived
        >;

    struct Control_;

    struct Model_:
        public Template_<ModelSelector>,
        public detail::MuteOwner,
        public detail::Mute,
        public ModelAccessors<Model_>
    {
    public:
        static constexpr bool isGroupModel = true;

        using ControlType = typename detail::CustomizeControl<Custom, Control_>;
        using Plain = typename Group::Plain;
        using Type = Plain;
        using Defer = DeferGroup<ModelSelector, Model_>;

        template<typename T>
        using Pex = pex::ModelSelector<T>;

        Model_()
            :
            Template<ModelSelector>(),
            detail::MuteOwner(),
            detail::Mute(this->GetMuteControl()),
            ModelAccessors<Model_>()
        {
            if constexpr (HasDefault<Plain>)
            {
                this->SetWithoutNotify_(Plain::Default());
            }
        }

        Model_(const Plain &plain)
            :
            Template<ModelSelector>(),
            detail::MuteOwner(),
            detail::Mute(this->GetMuteControl()),
            ModelAccessors<Model_>()
        {
            this->SetWithoutNotify_(plain);
        }

        Model_(const Model_ &) = delete;
        Model_(Model_ &&) = delete;
        Model_ & operator=(const Model_ &) = delete;
        Model_ & operator=(Model_ &&) = delete;

        bool HasModel() const { return true; }
    };

    using Model = typename detail::CustomizeModel<Custom, Model_>;

    template<typename Derived>
    using ControlAccessors = GroupAccessors
        <
            Plain,
            Fields,
            Template,
            ControlSelector,
            Derived
        >;

    using ControlMembers = ControlMembers_<Template_>;

    struct Control_:
        public detail::Mute,
        public ControlMembers,
        public ControlAccessors<Control_>
    {
        static constexpr bool isGroupControl = true;

        using Aggregate = detail::Aggregate<Plain, Fields, Template_>;

        using AccessorsBase = ControlAccessors<Control_>;

        // using Plain = typename Group::Plain;
        using Type = Plain;

        using Upstream = Model;

        using Defer = DeferGroup
            <
                ControlSelector,
                Control_
            >;

        // UpstreamType could be the type returned by a filter.
        // Filters have not been implemented by this class, so it remains the
        // same as the Type.
        using UpstreamType = Plain;
        using Filter = NoFilter;

        static constexpr bool isPexCopyable = true;

        template<typename T>
        using Pex =
            typename pex::ControlSelector<T>;

        Control_()
            :
            detail::Mute(),
            ControlMembers(),
            AccessorsBase()
        {

        }

        Control_(Model &model)
            :
            detail::Mute(model.GetMuteControl()),
            ControlMembers(),
            AccessorsBase()
        {
            fields::AssignConvert<Fields>(*this, model);
        }

        Control_(const Control_ &other)
            :
            detail::Mute(other),
            ControlMembers(),
            AccessorsBase()
        {
            fields::Assign<Fields>(*this, other);
        }

        Control_ & operator=(const Control_ &other)
        {
            this->detail::Mute::operator=(other);
            fields::Assign<Fields>(*this, other);

            return *this;
        }

        bool HasModel() const
        {
            return pex::detail::HasModel<Fields>(*this);
        }
    };

    using Control = typename detail::CustomizeControl<Custom, Control_>;

    using Aggregate = typename Control_::Aggregate;

    static typename Model::Defer MakeDefer(Model &model)
    {
        return typename Model::Defer(model);
    }

    static typename Control::Defer MakeDefer(Control &control)
    {
        return typename Control::Defer(control);
    }
};


} // end namespace pex
