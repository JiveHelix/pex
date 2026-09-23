#pragma once


#include <fields/fields.h>
#include <jive/describe_type.h>

#include <pex/identity.h>
#include <pex/tailors.h>
#include <pex/accessors.h>
#include <pex/traits.h>
#include <pex/type_tester.h>
#include <pex/for_each.h>
#include <pex/detail/group_detail.h>


/**


// The Group struct uses a Schema type to define a pattern or blueprint for the
// variants we will need, like Plain, Model, and Control

// The Schema type defines the members with template parameter types. Tailors
// then fashion customized type like pex::Identity, Model, Control, etc.

template<template<typename> typename T>
struct GpsSchema
{
    T<int64_t> time;
    T<double> latitude;
    T<double> longitude;
    T<double> elevation;
};

// Create the Plain-old-data structure, Model, and Control types
using GpsGroup = Group<GpsSchema>;

using Gps = typename GpsGroup::Plain;
using GpsModel = typename GpsGroup::Model;
using GpsControl = typename GpsGroup::Control;


// A Finisher type may be optionally specified to define a final form of any of
// the types that Group defines.

struct Finisher
{
    template<typename Base>
    struct Plain: public Base
    {
        void MyFunction() { doSomething; }
    };
};

using CustomizedGpsGroup = Group<GpsSchema, Finisher>;

using Gps = typename CustomizedGpsGroup::Plain;

Gps gps;
gps.MyFunction();


**/


namespace pex
{


namespace detail
{


template
<
    template<template<typename> typename> typename Schema_,
    typename Finisher = void
>
struct GroupModel_
{
    template<template<typename> typename T>
    using Schema = Schema_<T>;

    using Plain = detail::FinishPlain<Finisher, Schema<pex::Identity>>;

    template<template<typename> typename Tailor, typename Upstream>
    using DeferGroup = DeferGroup<Schema, Tailor, Upstream>;

    template<typename Derived>
    using ModelAccessors = GroupAccessors
        <
            Plain,
            Schema,
            ModelTailor,
            Derived
        >;

    struct Model:
        public detail::MuteOwner,
        public detail::MuteControl,
        public Schema_<ModelTailor>,
        public ModelAccessors<Model>
    {
    public:
        using Plain = typename GroupModel_::Plain;
        using Type = Plain;
        using Defer = DeferGroup<ModelTailor, Model>;

        // TODO: Pick one
        template<typename T>
        using Pex = pex::ModelTailor<T>;

        template<typename T>
        using Tailor = pex::ModelTailor<T>;

        Model()
            :
            detail::MuteOwner(),
            detail::MuteControl(this->GetMuteNode()),
            Schema<ModelTailor>{},
            ModelAccessors<Model>{}
        {
            this->SetInitial(Plain{});

            PEX_NAME(fmt::format("{} Model", jive::GetTypeName<Plain>()));

            PEX_NAMES(this);
        }

        Model(const Plain &plain)
            :
            detail::MuteOwner(),
            detail::MuteControl(this->GetMuteNode()),
            Schema<ModelTailor>{},
            ModelAccessors<Model>{}
        {
            this->SetInitial(plain);

            PEX_NAME(fmt::format("{} Model", jive::GetTypeName<Plain>()));
            PEX_NAMES(this);
        }

        Model(const Model &) = delete;
        Model(Model &&) = delete;
        Model & operator=(const Model &) = delete;
        Model & operator=(Model &&) = delete;

        Model & operator=(const Plain &plain)
        {
            this->Set(plain);

            return *this;
        }

        ~Model()
        {
            CLEAR_PEX_NAMES;
            PEX_CLEAR_NAME(this);
        }

        bool HasModel() const { return true; }
    };
};


template
<
    template<template<typename> typename> typename Schema,
    typename Finisher = void
>
using GroupModel = typename detail::GroupModel_<Schema, Finisher>::Model;


} // end namespace detail


template
<
    template<template<typename> typename> typename Schema_,
    typename Finisher = void
>
struct Group
{
    static constexpr bool isGroup = true;

    template<template<typename> typename T>
    using Schema = Schema_<T>;

    static_assert(
        std::is_void_v<Finisher>
            || detail::CheckFinisher
                <
                    Finisher,
                    Schema<pex::Identity>,
                    Schema<ModelTailor>,
                    Schema<ControlTailor>,
                    Schema<MuxTailor>,
                    Schema<FollowTailor>
                >,
        "Expected at least one customization");

    using Plain = detail::FinishPlain<Finisher, Schema<pex::Identity>>;
    using Type = Plain;

    template<template<typename> typename Tailor, typename Upstream>
    using DeferGroup = DeferGroup<Schema, Tailor, Upstream>;

private:
    using FinishedModel_ =
        typename detail::FinishModel
        <
            Finisher,
            detail::GroupModel<Schema, Finisher>
        >;

public:
    // Inject the types that GroupModel does not know about.
    struct Model
        :
        public FinishedModel_
    {
        using GroupType = Group;
        static constexpr bool isGroupModel = true;

        using FinishedModel_::FinishedModel_;

        using FinishedModel_::operator=;
    };


    template<template<typename> typename Tailor>
    using Ensemble = detail::Ensemble<Plain, Schema_, Tailor>;

    template<typename Derived>
    using ControlAccessors = GroupAccessors
        <
            Plain,
            Schema,
            ControlTailor,
            Derived
        >;

    using ControlMembers = Schema_<ControlTailor>;

    template<typename Upstream_>
    struct Control_:
        public detail::MuteControl,
        public ControlMembers,
        public ControlAccessors<Control_<Upstream_>>
    {
        using GroupType = Group;
        static constexpr bool isGroupControl = true;

        using Ensemble = typename Group::template Ensemble<ControlTailor>;
        using AccessorsBase = ControlAccessors<Control_>;
        using Type = Plain;
        using Upstream = Upstream_;

        using Defer = DeferGroup
            <
                ControlTailor,
                Control_
            >;

        // UpstreamType could be the type returned by a filter.
        // Filters have not been implemented by this class, so it remains the
        // same as the Type.
        using UpstreamType = Plain;
        using Filter = NoFilter;

        static constexpr bool isPexCopyable = true;

        // TODO: Pick one
        template<typename T>
        using Pex = typename pex::ControlTailor<T>;

        template<typename T>
        using Tailor = pex::ControlTailor<T>;

        Control_()
            :
            detail::MuteControl(),
            ControlMembers{},
            AccessorsBase{}
        {
            PEX_NAME(fmt::format("{} Control", jive::GetTypeName<Plain>()));
            PEX_NAMES(this);
        }

        Control_(Upstream &upstream)
            :
            detail::MuteControl(upstream.GetMuteNode()),
            ControlMembers{},
            AccessorsBase{}
        {
            AssignEmplace(*this, upstream);

            PEX_NAME(fmt::format("{} Control", jive::GetTypeName<Plain>()));
            PEX_NAMES(this);
        }

        Control_(const Control_ &other)
            :
            detail::MuteControl(other),
            ControlMembers{},
            AccessorsBase{}
        {
            fields::Assign(*this, other);

            PEX_NAME(fmt::format("{} Control", jive::GetTypeName<Plain>()));
            PEX_NAMES(this);
        }

    private:
        void Emplace(Upstream &upstream) = delete;

        void Emplace(const Control_ &other) = delete;

    protected:
        void StandardEmplace_(Upstream &upstream)
        {
            this->detail::MuteControl::Emplace(upstream.GetMuteNode());
            AssignEmplace(*this, upstream);
        }

        void StandardEmplace_(const Control_ &other)
        {
            this->detail::MuteControl::Emplace(other);
            AssignEmplace(*this, other);
        }

    public:
        Control_ & operator=(const Control_ &other)
        {
            this->detail::MuteControl::operator=(other);
            fields::Assign(*this, other);

            return *this;
        }

        Control_(Control_ &&other)
            :
            detail::MuteControl(other),
            ControlMembers{},
            AccessorsBase{}
        {
            fields::MoveAssign(*this, std::move(other));

            PEX_NAME(fmt::format("{} Control", jive::GetTypeName<Plain>()));
            PEX_NAMES(this);
        }

        ~Control_()
        {
            CLEAR_PEX_NAMES;
            PEX_CLEAR_NAME(this);
        }

        Control_ & operator=(Control_ &&other)
        {
            this->detail::MuteControl::operator=(other);
            fields::MoveAssign(*this, std::move(other));

            return *this;
        }

        bool HasModel() const
        {
            return pex::detail::HasModel(*this);
        }
    };

    template<typename Upstream>
    using Control =
        typename detail::FinishControl<Finisher, Control_<Upstream>>;

    using DefaultControl = Control<Model>;

    template<typename Derived>
    using MuxAccessors = GroupAccessors
        <
            Plain,
            Schema,
            MuxTailor,
            Derived
        >;

    using MuxMembers = Schema_<MuxTailor>;

    struct Mux_:
        public detail::MuteMux,
        public MuxMembers,
        public MuxAccessors<Mux_>
    {
        using GroupType = Group;
        static constexpr bool isGroupMux = true;
        static constexpr bool isPexCopyable = false;

        // We must use FollowTailor to track changes to Mux values.
        using Ensemble = typename Group::template Ensemble<FollowTailor>;
        using AccessorsBase = MuxAccessors<Mux_>;
        using Type = Plain;
        using Upstream = Model;

        using Defer = DeferGroup
            <
                MuxTailor,
                Mux_
            >;

        // UpstreamType could be the type returned by a filter.
        // Filters have not been implemented by this class, so it remains the
        // same as the Type.
        using UpstreamType = Plain;
        using Filter = NoFilter;

        // TODO: Pick one
        template<typename T>
        using Pex = typename pex::MuxTailor<T>;

        template<typename T>
        using Tailor = typename pex::MuxTailor<T>;

        Mux_()
            :
            detail::MuteMux(),
            MuxMembers{},
            AccessorsBase{}
        {
            PEX_NAME(fmt::format("{} Mux", jive::GetTypeName<Plain>()));
            PEX_NAMES(this);
        }

        Mux_(Model &model)
            :
            detail::MuteMux(model.GetMuteNode()),
            MuxMembers{},
            AccessorsBase{}
        {
            PEX_NAME(fmt::format("{} Mux", jive::GetTypeName<Plain>()));
            PEX_NAMES(this);

            this->ChangeUpstream(model);
        }

        Mux_(const Mux_ &) = delete;

        Mux_ & operator=(const Mux_ &other) = delete;

        Mux_(Mux_ &&other) = delete;

        ~Mux_()
        {
            CLEAR_PEX_NAMES;
            PEX_CLEAR_NAME(this);
        }

        Mux_ & operator=(Mux_ &&other) = delete;

    private:
        void Emplace(Upstream &) = delete;
        void Emplace(const Mux_ &) = delete;

    protected:
        void StandardEmplace_(Upstream &upstream)
        {
            this->detail::MuteMux::Emplace(upstream.GetMuteNode());
            AssignEmplace(*this, upstream);
        }

        void StandardEmplace_(const Mux_ &other)
        {
            this->detail::MuteMux::Emplace(other);
            AssignEmplace(*this, other);
        }

    public:
#if 0
        detail::MuteFollow GetMuteNode() const
        {
            return this->CloneMuteNode();
        }
#endif

        bool HasModel() const
        {
            return pex::detail::HasModel(*this);
        }

        void ChangeUpstream(Upstream &upstream)
        {
            this->detail::MuteMux::ChangeUpstream(upstream.GetMuteNode());

            if constexpr (fields::HasFields<Upstream>)
            {
                static_assert(
                    fields::HasFields<Mux_>,
                    "Mux_ must have fields when Upstream has fields");

                auto swapper = [this, &upstream](
                    const auto &thisField,
                    const auto &upstreamField) -> void
                {
                    (this->*(thisField.member)).ChangeUpstream(
                        upstream.*(upstreamField.member));
                };

                jive::ZipApply(swapper, Mux_::fields, Upstream::fields);
            }
            else
            {
                auto swapper = [](
                    auto &muxMember,
                    auto &upstreamMember) -> void
                {
                    muxMember.ChangeUpstream(upstreamMember);
                };

                fields::ForEachZip(*this, upstream, swapper);
            }
        }
    };

    using Mux = typename detail::FinishMux<Finisher, Mux_>;

    template<typename Derived>
    using FollowAccessors = GroupAccessors
        <
            Plain,
            Schema,
            FollowTailor,
            Derived
        >;

    using FollowMembers = Schema_<FollowTailor>;

    struct Follow_:
        public detail::MuteFollow,
        public FollowMembers,
        public FollowAccessors<Follow_>
    {
        using GroupType = Group;

        // This structure behaves like a group control.
        static constexpr bool isGroupFollow = true;

        using Ensemble = typename Group::template Ensemble<FollowTailor>;
        using AccessorsBase = FollowAccessors<Follow_>;
        using Type = Plain;
        using Upstream = Mux;

        using Defer = DeferGroup
            <
                FollowTailor,
                Follow_
            >;

        // UpstreamType could be the type returned by a filter.
        // Filters have not been implemented by this class, so it remains the
        // same as the Type.
        using UpstreamType = Plain;
        using Filter = NoFilter;

        static constexpr bool isPexCopyable = true;

        // TODO: Pick one
        template<typename T>
        using Pex = typename pex::FollowTailor<T>;

        template<typename T>
        using Tailor = typename pex::FollowTailor<T>;

        Follow_()
            :
            detail::MuteFollow(),
            FollowMembers{},
            AccessorsBase{}
        {
            PEX_NAME(fmt::format("{} Follow", jive::GetTypeName<Plain>()));
            PEX_NAMES(this);
        }

        Follow_(Upstream &upstream)
            :
            detail::MuteFollow(upstream.GetMuteNode()),
            FollowMembers{},
            AccessorsBase{}
        {
            AssignEmplace(*this, upstream);

            PEX_NAME(fmt::format("{} Follow", jive::GetTypeName<Plain>()));
            PEX_NAMES(this);
        }

        Follow_(const Follow_ &other)
            :
            detail::MuteFollow(other),
            FollowMembers{},
            AccessorsBase{}
        {
            fields::Assign(*this, other);

            PEX_NAME(fmt::format("{} Follow", jive::GetTypeName<Plain>()));
            PEX_NAMES(this);
        }

        Follow_ & operator=(const Follow_ &other)
        {
            this->detail::MuteFollow::operator=(other);
            fields::Assign(*this, other);

            return *this;
        }

        Follow_(Follow_ &&other)
            :
            detail::MuteFollow(other),
            FollowMembers{},
            AccessorsBase{}
        {
            fields::MoveAssign(*this, std::move(other));

            PEX_NAME(fmt::format("{} Follow", jive::GetTypeName<Plain>()));
            PEX_NAMES(this);
        }

        ~Follow_()
        {
            CLEAR_PEX_NAMES;
            PEX_CLEAR_NAME(this);
        }

        Follow_ & operator=(Follow_ &&other)
        {
            this->detail::MuteFollow::operator=(other);
            fields::MoveAssign(*this, std::move(other));

            return *this;
        }

    private:
        void Emplace(Upstream &upstream) = delete;
        void Emplace(const Follow_ &other) = delete;

    protected:
        void StandardEmplace_(Upstream &upstream)
        {
            this->detail::MuteFollow::Emplace(upstream.GetMuteNode());
            AssignEmplace(*this, upstream);
        }

        void StandardEmplace_(const Follow_ &other)
        {
            this->detail::MuteFollow::Emplace(other);
            AssignEmplace(*this, other);
        }

    public:
        bool HasModel() const
        {
            return pex::detail::HasModel(*this);
        }
    };

    using Follow = typename detail::FinishFollow<Finisher, Follow_>;

    static typename Model::Defer MakeDefer(Model &model)
    {
        return typename Model::Defer(model);
    }

    template<typename Upstream>
    static typename Control<Upstream>::Defer MakeDefer(
        Control<Upstream> &control)
    {
        return typename Control<Upstream>::Defer(control);
    }
};


} // end namespace pex
