#pragma once


#include <fields/compare.h>
#include "pex/identity.h"
#include "pex/traits.h"
#include "pex/poly_base.h"
#include "pex/detail/traits.h"
#include "pex/detail/poly_detail.h"
#include "pex/get_type_name.h"


namespace pex
{


namespace poly
{


template<typename Json, typename Templates, typename T>
Json PolyUnstructure(const T &object)
{
    auto jsonValues = fields::Unstructure<Json>(object);
    jsonValues["type"] = std::string(GetTypeName<Templates>());

    return jsonValues;
}

template<typename Json, typename T>
requires ::fields::HasFieldsTypeName<T>
Json PolyUnstructure(const T &object, const std::string &typeName)
{
    auto jsonValues = fields::Unstructure<Json>(object);
    jsonValues["type"] = typeName;

    return jsonValues;
}


template
<
    typename Templates
>
requires ::pex::HasMinimalSupers<Templates>
class DerivedValueTemplate_
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

    DerivedValueTemplate_()
        :
        ValueBase(),
        TemplateBase()
    {

    }

    DerivedValueTemplate_(const TemplateBase &other)
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
            DerivedValueTemplate_::fields,
            style,
            indent);
    }

    Json Unstructure() const override
    {
        return pex::poly::PolyUnstructure<Json, Templates>(*this);
    }

    bool operator==(const VirtualBase &other) const override
    {
        auto otherPolyBase = dynamic_cast<const DerivedValueTemplate_ *>(&other);

        if (!otherPolyBase)
        {
            return false;
        }

        return (fields::ComparisonTuple(*this)
            == fields::ComparisonTuple(*otherPolyBase));
    }

    static std::string_view DoGetTypeName()
    {
        return ::pex::poly::GetTypeName<Templates>();
    }

    std::string_view GetTypeName() const override
    {
        return ::pex::poly::GetTypeName<Templates>();
    }

    // Finds the most "Derived" class provided in Templates to ensure
    // everything is copied.
    std::shared_ptr<ValueBase> Copy() const override
    {
        static_assert(
            !::pex::detail::HasDerived<Templates>,
            "Obsolete customization: Change 'Derived' to 'DerivedValue'");

        if constexpr (::pex::detail::HasDerivedValue<Templates>)
        {
            using Type = typename Templates
                ::template DerivedValue<DerivedValueTemplate_>;

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
            return std::make_shared<DerivedValueTemplate_>(*this);
        }
    }
};


template
<
    typename Templates,
    typename = void
>
struct MakeDerivedValue
{
    static_assert(
        !::pex::detail::HasDerived<Templates>,
        "Obsolete customization: Change 'Derived' to 'DerivedValue'");

    using Type = DerivedValueTemplate_<Templates>;
};


template<typename Templates>
struct MakeDerivedValue
<
    Templates,
    std::enable_if_t<::pex::detail::HasDerivedValue<Templates>>
>
{
    static_assert(
        !::pex::detail::HasDerived<Templates>,
        "Obsolete customization: Change 'Derived' to 'DerivedValue'");

    using Type =
        typename Templates::template DerivedValue<DerivedValueTemplate_<Templates>>;
};


template<typename Templates>
using DerivedValueTemplate = typename MakeDerivedValue<Templates>::Type;


} // end namespace poly


} // end namespace pex
