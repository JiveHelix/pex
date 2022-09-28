#include "pex/detail/log.h"

namespace pex
{
    std::unique_ptr<std::mutex> logMutex(std::make_unique<std::mutex>());
}
