#pragma once

#ifdef ENABLE_PEX_LOG

#include <iostream>
#include <jive/stream_args.h>
#include <jive/path.h>


#define PEX_LOG(...) \
    jive::StreamArgs( \
        std::cout, \
        "[pex:", \
        jive::path::Base(__FILE__), \
        ":", \
        __FUNCTION__, \
        ":", \
        __LINE__, \
        "] ", \
        __VA_ARGS__); assert(std::cout.good())


#else


#define PEX_LOG(...)


#endif // ENABLE_PEX_LOG
