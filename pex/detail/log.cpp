#include "pex/detail/log.h"

#include <map>
#include <fmt/core.h>


namespace pex
{


std::unique_ptr<std::mutex> logMutex(std::make_unique<std::mutex>());


struct Name
{
    void *parent;
    std::string name;
};


std::map<void *, Name> namesByAddress;


void RegisterPexName(void *address, const std::string &name)
{
    namesByAddress[address] = Name{nullptr, name};
}


void UnregisterPexName(
    void *address,
    const std::string &name)
{
    if (namesByAddress.count(address) == 1)
    {
        auto storedName = namesByAddress[address];

        if (storedName.name == name)
        {
            namesByAddress.erase(address);
        }
    }
}


void RegisterPexName(void *address, void *parent, const std::string &name)
{
    if (address == parent)
    {
        throw std::runtime_error("parent must have unique address");
    }

    namesByAddress[address] = Name{parent, name};
}


std::string LookupPexName(void *address)
{
    if (address == nullptr)
    {
        return "NULL";
    }

    if (namesByAddress.count(address))
    {
        auto &name = namesByAddress[address];

        if (name.parent != nullptr)
        {
            assert(name.parent != address);

            return fmt::format(
                "{} => {}",
                name.name,
                LookupPexName(name.parent));
        }

        return fmt::format(
            "{} @ {}",
            name.name,
            address);
    }

    return fmt::format("{}", address);
}


}
