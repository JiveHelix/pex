#pragma once

#ifndef NDEBUG

#include <stdexcept>

#define REQUIRE_HAS_VALUE(required)                        \
    if (!required)                                         \
    {                                                      \
        throw std::logic_error(#required " must be set");  \
    }
#else

#define REQUIRE_HAS_VALUE(required)

#endif
