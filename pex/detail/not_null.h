#pragma once

#ifndef NDEBUG

#include <stdexcept>

#define NOT_NULL(pointer)                                     \
    if (pointer == nullptr)                                   \
    {                                                         \
        throw std::logic_error(#pointer " must not be NULL"); \
    }

#else

#define NOT_NULL(pointer)

#endif
