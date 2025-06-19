#pragma once

// #define ENABLE_PEX_LOG
// #define USE_OBSERVER_NAME
// #define ENABLE_REGISTER_NAME

#include <mutex>
#include <memory>
#include <jive/path.h>
#include <jive/to_stream.h>


namespace pex
{


extern std::unique_ptr<std::mutex> logMutex;


template <typename ...T>
void ToStream(T && ... args)
{
    if (logMutex)
    {
        // The mutex exists.
        std::lock_guard<std::mutex> lock(*logMutex);
        jive::ToStreamFlush(std::forward<T>(args)...);
    }
    else
    {
        // Either the mutex has not been created, or it has already been
        // destroyed.
        jive::ToStreamFlush(std::forward<T>(args)...);
    }
}


void RegisterPexName(void *address, const std::string &name);

void RegisterPexName(void *address, void *parent, const std::string &name);

void RegisterPexParent(void *parent, void *child);

void UnregisterPexName(void *address, const std::string &name);

std::string LookupPexName(const void *address);

bool HasPexName(void *address);


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


#ifdef ENABLE_REGISTER_NAME

struct Separator { char garbage; };

#define REGISTER_PEX_NAME(address, name) pex::RegisterPexName(address, name)

#define UNREGISTER_PEX_NAME(address, name) pex::UnregisterPexName(address, name)

#define REGISTER_PEX_NAME_WITH_PARENT(address, parent, name) \
    pex::RegisterPexName(address, parent, name)

#define REGISTER_PEX_PARENT(parent, child) \
    pex::RegisterPexParent(parent, child)

#else

struct Separator {};

#define REGISTER_PEX_NAME(address, name)
#define REGISTER_PEX_NAME_WITH_PARENT(address, parent, name)
#define REGISTER_PEX_PARENT(parent, child)
#define UNREGISTER_PEX_NAME(address, name)

#endif // ENABLE_REGISTER_NAME
