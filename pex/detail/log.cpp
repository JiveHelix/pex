#include "pex/detail/log.h"

#include <map>
#include <fmt/core.h>
#include <vector>


namespace pex
{


std::unique_ptr<std::mutex> logMutex(std::make_unique<std::mutex>());


struct Name
{
    const void *parent;
    std::string name;
};


std::map<const void *, Name> namesByAddress;


std::string FormatName(const void *address, const Name &name)
{
    if (name.parent != nullptr)
    {
        assert(name.parent != address);

        return fmt::format(
            "({} @ {}) childof {}",
            name.name,
            address,
            LookupPexName(name.parent));
    }

    return fmt::format(
        "{} @ {}",
        name.name,
        address);
}


void RegisterPexName(void *address, const std::string &name)
{
#if 0
    std::cout << "RegisterPexName: " << name << " @ " << address
        << " namesByAddress.size(): " << namesByAddress.size()
        << std::endl;
#endif

    if (namesByAddress.count(address))
    {
        auto &entry = namesByAddress[address];
        entry.name = name;
    }
    else
    {
        namesByAddress[address] = Name{nullptr, name};
    }
}


void UnregisterPexName(void *address)
{
    namesByAddress.erase(address);
}


void RegisterPexName(void *address, void *parent, const std::string &name)
{
    if (address == parent)
    {
        throw std::runtime_error("parent must have unique address");
    }

    if (!namesByAddress.count(parent))
    {
        throw std::runtime_error("disallowed anonymous parent");
    }

    if (namesByAddress.count(address))
    {
        auto &entry = namesByAddress[address];

        entry.parent = parent;
        entry.name = name;
    }
    else
    {
        namesByAddress[address] = Name{parent, name};
    }
}


void RegisterPexParent(void *parent, void *child)
{
    if (child == parent)
    {
        throw std::runtime_error("parent must have unique address");
    }

    if (!namesByAddress.count(parent))
    {
        throw std::runtime_error("disallowed anonymous parent");
    }

    if (namesByAddress.count(child))
    {
        auto &name = namesByAddress[child];
        name.parent = parent;
    }
    else
    {
        namesByAddress[child] = Name{parent, ""};
    }
}


bool HasPexName(const void *address)
{
    if (!address)
    {
        return false;
    }

    if (!namesByAddress.count(address))
    {
        return false;
    }

    const auto &entry = namesByAddress[address];

    if (entry.name.empty())
    {
        return false;
    }

    return true;
}


bool HasNamedParent(const void *address)
{
    if (!address)
    {
        return false;
    }

    if (!namesByAddress.count(address))
    {
        return false;
    }

    const auto &entry = namesByAddress[address];

    return HasPexName(entry.parent);
}


std::string LookupPexName(const void *address)
{
    if (address == nullptr)
    {
        return "NULL";
    }

    if (namesByAddress.count(address))
    {
        auto &name = namesByAddress[address];

        return FormatName(address, name);
    }

    return fmt::format("{}", address);
}


void ResetPexNames()
{
    namesByAddress.clear();
}


}
