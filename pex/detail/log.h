#pragma once

#include <mutex>
#include <jive/stream_args.h>


namespace pex
{


extern std::mutex logMutex;


template <typename ...T>
void ToStream(T && ... args)
{
    std::lock_guard<std::mutex> lock(logMutex);
    jive::StreamArgsFlush(std::forward<T>(args)...);
}


} // end namespace pex


#ifdef ENABLE_PEX_LOG

#include <iostream>
#include <jive/path.h>


#define PEX_LOG(...) \
\
    pex::ToStream( \
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
