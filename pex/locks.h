#pragma once

#include <optional>
#include <ostream>
#include <thread>
#include <shared_mutex>

#include <jive/path.h>
#include "pex/log.h"


namespace pex
{


inline std::ostream & LogCommon(
    std::ostream &output,
    const char * fileName,
    int line,
    const char * lockName,
    void * lockId)
{
    assert(output.good());

    return output << "[" << fileName << ":" << line
        << " " << lockName
        << ", thread:" << std::this_thread::get_id()
        << ", lock:" << lockId << "] ";
}


struct MemberLogger
{
public:
    MemberLogger(
        const std::string &fileName,
        int line,
        const char *lockName,
        void * lockId)
    {
        std::lock_guard<std::mutex> lock(*pex::logMutex);

        LogCommon(std::cout, fileName.c_str(), line, lockName, lockId)
            << "LogLock() lock" << std::endl;
    }
};


using Mutex = std::shared_mutex;


struct ExclusiveLock
{
    using Lock = std::unique_lock<Mutex>;
    static constexpr auto name = "WriteLock";
};

struct SharedLock
{
    using Lock = std::shared_lock<Mutex>;
    static constexpr auto name = "ReadLock";
};


template<typename LockType>
class LogLock
{
public:
    using Lock = typename LockType::Lock;

    LogLock(const std::string &fileName, int line, Mutex &mutex)
        :
        fileName_(fileName),
        line_(line),
        lockId_(&mutex),
        memberLogger_(this->fileName_, line, LockType::name, &mutex),
        lock_()
    {
        try
        {
            this->lock_ = Lock(mutex);
        }
        catch (std::system_error &error)
        {
            pex::ToStream(
                std::cout,
                "Lock failed: ",
                error.code(),
                ": ",
                error.what());

            throw;
        }
        catch (std::exception &)
        {
            pex::ToStream(std::cout, "Unknown exception!");
            throw;
        }
    }

    LogLock(const LogLock &) = delete;

    std::ostream & LogCommon()
    {
        return pex::LogCommon(
            std::cout,
            this->fileName_.c_str(),
            this->line_,
            LockType::name,
            this->lockId_);
    }

    ~LogLock()
    {
        std::lock_guard<std::mutex> lock(*pex::logMutex);
        this->LogCommon() << "~LogLock() unlock" << std::endl;
    }

    void unlock()
    {
        {
            std::lock_guard<std::mutex> lock(*pex::logMutex);
            this->LogCommon() << "unlock() calling unlock" << std::endl;
        }

        (*this->lock_).unlock();
    }

    void lock()
    {
        {
            std::lock_guard<std::mutex> lock(*pex::logMutex);
            this->LogCommon() << "lock() calling lock" << std::endl;
        }

        try
        {
            (*this->lock_).lock();
        }
        catch (std::system_error &error)
        {
            pex::ToStream(
                std::cout,
                "Lock failed: ",
                error.code(),
                ": ",
                error.what());

            throw;
        }
        catch (std::exception &e)
        {
            pex::ToStream(std::cout, "Unknown exception!");
            throw;
        }
    }

private:
    std::string fileName_;
    int line_;
    void * lockId_;
    MemberLogger memberLogger_;
    std::optional<Lock> lock_;
};


#ifdef ENABLE_LOG_LOCKS

using WriteLock = LogLock<ExclusiveLock>;
using ReadLock = LogLock<SharedLock>;

#else

using WriteLock = typename ExclusiveLock::Lock;
using ReadLock = typename SharedLock::Lock;

#endif


} // end namespace pex


#ifdef ENABLE_LOG_LOCKS

#define WRITE_LOCK(mutex) pex::WriteLock lock( \
    jive::path::Base(__FILE__), \
    __LINE__, \
    mutex)

#define READ_LOCK(mutex) pex::ReadLock lock( \
    jive::path::Base(__FILE__), \
    __LINE__, \
    mutex)

#define MOVE_LOCK(mutex) pex::WriteLock( \
    jive::path::Base(__FILE__), \
    __LINE__, \
    mutex)

#else

#define WRITE_LOCK(mutex) pex::WriteLock lock(mutex)
#define READ_LOCK(mutex) pex::ReadLock lock(mutex)
#define MOVE_LOCK(mutex) pex::WriteLock(mutex)

#endif
