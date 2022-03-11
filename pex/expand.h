#pragma once

#include <jive/zip_apply.h>
#include <fields/core.h>


namespace pex
{


template<typename Control, typename Member>
struct ExpandFilter
{
    using Type = typename Control::Type;

    /*
     * @param control A pex::control::Value for the aggregate type.
     * @param member The pointer to member of the aggregate type.
     */
    ExpandFilter(Control control, Member Type::*member)
        :
        control_{control},
        member_{member}
    {

    }

    Member Get(pex::ArgumentT<Type> value) const
    {
        return value.*(this->member_);
    }

    Type Set(Member value) const
    {
        // Get the aggregate instance.
        Type result = this->control_.Get();

        // Set the member controlled by this filter.
        result.*(this->member_) = value;
        return result;
    }

private:
    Control control_;
    Member Type::* member_;
};


template<typename Control, typename T>
using Expand = 
    pex::control::FilteredLike
    <
        Control,
        ExpandFilter<Control, T>
    >;


template
<
    template<typename> typename Fields,
    typename Expanded,
    typename Source
>
void InitializeExpanded(Expanded &expanded, Source source)
{
    auto initializer = [&expanded, &source](
        const auto &expandedField,
        const auto &subField)
    {
        using ExpandedControl = std::remove_reference_t<
            decltype(expanded.*(expandedField.member))>;

        // Copy the source control.
        ExpandedControl expandedControl(source);

        using Filter = typename ExpandedControl::Filter;

        // Set the filter on the expanded control.
        expandedControl.SetFilter(Filter(source, subField.member));

        expanded.*(expandedField.member) = expandedControl;
    };

    jive::ZipApply(
        initializer,
        Fields<Expanded>::fields,
        Fields<typename Source::Type>::fields);
}


} // end namespace pex
