#pragma once


#include <fields/assign.h>


namespace pex
{


template
<
    typename Plain,
    typename Derived,
    template<typename> typename Fields
>
class Accessors
{
public:
    Plain Get() const
    {
        Plain result;
        fields::AssignConvert<Fields>(
            result,
            static_cast<const Derived &>(*this));

        return result;
    }

    void Set(const Plain &plain)
    {
        fields::Assign<Fields>(static_cast<Derived &>(*this), plain);
    }

    explicit operator Plain () const
    {
        return this->Get();
    }
};


template
<
    template<typename> typename Fields,
    typename Target,
    typename Source
>
void Assign(Target &target, Source &source)
{
    auto initializer = [&target, &source](
        const auto &targetField,
        const auto &sourceField) -> void
    {
        // Target member must have a Set function.
        (target.*(targetField.member)).Set(source.*(sourceField.member));
    };

    jive::ZipApply(
        initializer,
        Fields<Target>::fields,
        Fields<Source>::fields);
}


} // end namespace pex
