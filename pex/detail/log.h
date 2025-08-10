#pragma once

// #define ENABLE_PEX_LOG
// #define ENABLE_PEX_CONCISE_LOG
// #define USE_OBSERVER_NAME
#define ENABLE_PEX_NAMES

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


void PexLinkObserver(const void *address, const void *observer);

const void * GetLinkedObserver(const void *address);

void PexNameUnique(void *address, const std::string &name);

void PexName(void *address, const std::string &name);

void PexName(void *address, void *parent, const std::string &name);

template<typename T>
T * PexNameAndReturn(T *address, const std::string &name)
{
    PexName(address, name);

    return address;
}

template<typename T>
T * PexNameAndReturn(T *address, void *parent, const std::string &name)
{
    PexName(address, parent, name);

    return address;
}

void RegisterPexParent(void *parent, void *child);

void ClearPexName(void *address);

std::string LookupPexName(const void *address, int indent = -1);

bool HasPexName(const void *address);

bool HasNamedParent(const void *address);

const void * GetParent(const void *address);

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


#ifdef ENABLE_PEX_CONCISE_LOG

#include <iostream>
#include <jive/path.h>


#define PEX_CONCISE_LOG(...) \
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


#define PEX_CONCISE_LOG(...)

#endif // ENABLE_PEX_CONCISE_LOG


#ifdef ENABLE_PEX_NAMES

struct Separator { char garbage; };

#define PEX_NAME(name) pex::PexName(this, name)

#define PEX_NAME_UNIQUE(name) pex::PexNameUnique(this, name)

#define PEX_THIS(name) \
    pex::PexNameAndReturn(this, name)

// Name a pex node that does not have a parent.
#define PEX_ROOT(root) pex::PexName(&root, #root)

#define PEX_CLEAR_NAME(address) pex::ClearPexName(address)

// Name a member of pex node 'this'.
#define PEX_MEMBER(member) \
    pex::PexName(&member, this, #member)

#define PEX_MEMBER_ADDRESS(member, name) \
    pex::PexName(member, this, name)

#define PEX_MEMBER_PASS(member) \
    *pex::PexNameAndReturn(&member, this, #member)

#define RESET_PEX_NAMES pex::ResetPexNames();

#define PEX_LINK_OBSERVER(address, observer) \
    pex::PexLinkObserver(address, observer)


#else

struct Separator {};

#define PEX_NAME(name)
#define PEX_NAME_UNIQUE(name)
#define PEX_THIS(name) this
#define PEX_ROOT(root)
#define PEX_MEMBER(member)
#define PEX_MEMBER_ADDRESS(member, name)
#define PEX_MEMBER_NAME(member, name)
#define PEX_CLEAR_NAME(address)
#define PEX_MEMBER_PASS(member) member

#define RESET_PEX_NAMES

#define PEX_LINK_OBSERVER(address, observer)

#endif // ENABLE_PEX_NAMES
