#pragma once


#include "pex/identity.h"
#include "pex/traits.h"
#include "pex/poly_base.h"
#include "pex/detail/traits.h"
#include "pex/detail/poly_detail.h"


namespace pex
{


namespace poly
{


template
<
    typename Templates
>
requires ::pex::HasMinimalSupers<Templates>
class PolyDerived_
    :
    public Templates::Supers::ValueBase,
    public Templates::template Template<pex::Identity>
{
public:
    using ValueBase = typename Templates::Supers::ValueBase;

    static_assert(
        detail::IsCompatibleBase<ValueBase>,
        "Expected virtual functions to be overloaded in this class");

    using VirtualBase = typename detail::VirtualBase_<ValueBase>::Type;
    using Json = typename ValueBase::Json;
    using TemplateBase = typename Templates::template Template<pex::Identity>;

    PolyDerived_()
        :
        ValueBase(),
        TemplateBase()
    {

    }

    PolyDerived_(const TemplateBase &other)
        :
        ValueBase(),
        TemplateBase(other)
    {

    }

    std::ostream & Describe(
        std::ostream &outputStream,
        const fields::Style &style,
        int indent) const override
    {
        return fields::DescribeFields(
            outputStream,
            *this,
            PolyDerived_::fields,
            style,
            indent);
    }

    Json Unstructure() const override
    {
        return pex::poly::PolyUnstructure<Json>(*this);
    }

    bool operator==(const VirtualBase &other) const override
    {
        auto otherPolyBase = dynamic_cast<const PolyDerived_ *>(&other);

        if (!otherPolyBase)
        {
            return false;
        }

        return (fields::ComparisonTuple(*this)
            == fields::ComparisonTuple(*otherPolyBase));
    }

    std::string_view GetTypeName() const override
    {
        return PolyDerived_::fieldsTypeName;
    }

    std::shared_ptr<ValueBase> Copy() const override
    {
        if constexpr (::pex::detail::HasDerived<Templates>)
        {
            using Type = Templates::template Derived<PolyDerived_>;
            auto self = dynamic_cast<const Type *>(this);

            if (!self)
            {
                throw std::logic_error(
                    "This is not the class you are looking for.");
            }

            return std::make_shared<Type>(*self);
        }
        else
        {
            return std::make_shared<PolyDerived_>(*this);
        }
    }
};


template
<
    typename Templates,
    typename = void
>
struct MakePolyDerived
{
    using Type = PolyDerived_<Templates>;
};


template<typename Templates>
struct MakePolyDerived
<
    Templates,
    std::enable_if_t<::pex::detail::HasDerived<Templates>>
>
{
    using Type =
        Templates::template Derived<PolyDerived_<Templates>>;
};


template<typename Templates>
using PolyDerived = typename MakePolyDerived<Templates>::Type;


} // end namespace poly


} // end namespace pex
