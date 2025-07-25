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

template<typename T>
T * UseRegisterPexName(T *address, const std::string &name)
{
    RegisterPexName(address, name);

    return address;
}

template<typename T>
T * UseRegisterPexName(T *address, void *parent, const std::string &name)
{
    RegisterPexName(address, parent, name);

    return address;
}

void RegisterPexParent(void *parent, void *child);

void UnregisterPexName(void *address);

std::string LookupPexName(const void *address);

bool HasPexName(const void *address);

bool HasNamedParent(const void *address);

void ResetPexNames();


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

#define UNREGISTER_PEX_NAME(address) pex::UnregisterPexName(address)

#define REGISTER_PEX_NAME_WITH_PARENT(address, parent, name) \
    pex::RegisterPexName(address, parent, name)

#define REGISTER_PEX_PARENT(name) \
    pex::RegisterPexName(&name, this, "name")



#define USE_REGISTER_PEX_NAME(address, name) \
    pex::UseRegisterPexName(address, name)

#define USE_REGISTER_PEX_PARENT(name) \
    pex::UseRegisterPexName(&name, this, "name")

#define RESET_PEX_NAMES pex::ResetPexNames();


#else

struct Separator {};

#define REGISTER_PEX_NAME(address, name)
#define REGISTER_PEX_NAME_WITH_PARENT(address, parent, name)
#define REGISTER_PEX_PARENT(name)
#define UNREGISTER_PEX_NAME(address)

#define USE_REGISTER_PEX_NAME(address, name) address
#define USE_REGISTER_PEX_PARENT(name) &name

#define RESET_PEX_NAMES

#endif // ENABLE_REGISTER_NAME


#define REGISTER_IDENTITY(name) REGISTER_PEX_NAME(&name, "name")

#if 0
#define DO_REGISTER_IDENTITY(name) REGISTER_PEX_NAME(&name, "name")

// Ping‑pong to avoid direct self-recursion
#define REGISTER_IDENTITY_A(x, ...) \
    DO_REGISTER_IDENTITY(x) __VA_OPT__(; REGISTER_IDENTITY_B(__VA_ARGS__))

#define REGISTER_IDENTITY_B(x, ...) \
    DO_REGISTER_IDENTITY(x) __VA_OPT__(; REGISTER_IDENTITY_A(__VA_ARGS__))

// Entry point
#define REGISTER_IDENTITY(...) __VA_OPT__(REGISTER_IDENTITY_A(__VA_ARGS__))

#endif
