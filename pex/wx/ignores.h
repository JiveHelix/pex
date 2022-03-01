#pragma once


#ifndef DO_PRAGMA
#define DO_PRAGMA_(arg) _Pragma (#arg)
#define DO_PRAGMA(arg) DO_PRAGMA_(arg)
#endif


#ifndef _WIN32

#define WXSHIM_PUSH_IGNORES \
    DO_PRAGMA(GCC diagnostic push); \
    DO_PRAGMA(GCC diagnostic ignored "-Wold-style-cast"); \
    DO_PRAGMA(GCC diagnostic ignored "-Wsign-conversion"); \
    DO_PRAGMA(GCC diagnostic ignored "-Wdouble-promotion");

#define WXSHIM_POP_IGNORES \
    DO_PRAGMA(GCC diagnostic pop)

#else

#define WXSHIM_PUSH_IGNORES
#define WXSHIM_POP_IGNORES

#endif
