/**
  * @file initialize.h
  *
  * @brief Provides automated interface initialization using fields tuples.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 11 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include "jive/zip_apply.h"

namespace pex
{

template<
    template<typename> typename Fields,
    typename Model,
    typename Interface>
void Initialize(Model &model, Interface &interface)
{
    auto initializer = [&model, &interface](
        auto &&modelField,
        auto &&interfaceField) -> void
    {
        using type = std::remove_reference_t<
            decltype(interface.*(interfaceField.member))>;

        // Create a new interface field from the model.
        interface.*(interfaceField.member) =
            type(&(model.*(modelField.member)));
    };

    jive::ZipApply(
        initializer,
        Fields<Model>::fields,
        Fields<Interface>::fields);
}

} // namespace pex
