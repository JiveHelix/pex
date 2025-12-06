#pragma once

#ifdef _MSC_VER
    #define TRACE_TYPES() __pragma(message(__FUNCSIG__))
#else
    #define TRACE_TYPES()
#endif
