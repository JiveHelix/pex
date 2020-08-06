#pragma once

#include "jive/zip_apply.h"

namespace pex
{

template<typename Model, typename Interface>
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

    jive::ZipApply(initializer, Model::fields, Interface::fields);
}

} // namespace pex
