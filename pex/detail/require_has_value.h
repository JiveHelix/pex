#pragma once

#ifndef NDEBUG

#include <stdexcept>
#include <iostream>

#define REQUIRE_HAS_VALUE(required)                        \
    if (!required)                                         \
    {                                                      \
        std::cerr << __FILE__ << ":" << __LINE__ << " "    \
            << #required << " is not set" << std::endl;    \
        throw std::logic_error(#required " must be set");  \
    }
#else

#define REQUIRE_HAS_VALUE(required)

#endif
