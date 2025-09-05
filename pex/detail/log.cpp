#include "pex/detail/log.h"

#include <map>
#include <fmt/core.h>
#include <vector>
#include <fields/describe.h>


namespace pex
{


std::unique_ptr<std::mutex> logMutex(std::make_unique<std::mutex>());


struct Name
{
    const void *parent;
    std::string name;
};


std::map<const void *, Name> namesByAddress;
std::map<const void *, Name> deletedNamesByAddress;
std::map<const void *, const void *> observerByLinkedAddress;


std::string FormatName(
    const void *address,
    const Name &name,
    int indent)
{
    if (name.parent != nullptr)
    {
        assert(name.parent != address);

        return fmt::format(
            "{}({} @ {}) member of {}",
            fields::MakeIndent(indent),
            name.name,
            address,
            LookupPexName(
                name.parent,
                (indent > -1) ? indent + 1 : indent));
    }

    return fmt::format(
        "{}{} @ {}",
        fields::MakeIndent(indent),
        name.name,
        address);
}


std::string FormatDeletedName(
    const void *address,
    const Name &name,
    int indent)
{
    if (name.parent != nullptr)
    {
        assert(name.parent != address);

        return fmt::format(
            "{} (deleted) ({} @ {}) child of {}",
            fields::MakeIndent(indent),
            name.name,
            address,
            LookupPexName(
                name.parent,
                (indent > -1) ? indent + 1 : indent));
    }

    return fmt::format(
        "{}{} @ {}",
        fields::MakeIndent(indent),
        name.name,
        address);
}


void PexLinkObserver(const void *address, const void *observer)
{
    assert(HasPexName(observer));
    assert(HasPexName(address));
    assert(address != observer);

    observerByLinkedAddress[address] = observer;
}


const void * GetLinkedObserver(const void *address)
{
    do
    {
        if (observerByLinkedAddress.count(address))
        {
            return observerByLinkedAddress[address];
        }
    }
    while ((address = GetParent(address)));

    return NULL;
}


void PexNameUnique(void *address, const std::string &name)
{
    if (namesByAddress.count(address))
    {
        throw std::logic_error("Name exists");
    }

    namesByAddress[address] = Name{nullptr, name};
}


void PexName(void *address, const std::string &name)
{
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


void ClearPexName(void * address)
{
    if (namesByAddress.count(address))
    {
        deletedNamesByAddress[address] = namesByAddress[address];
    }

    namesByAddress.erase(address);
    observerByLinkedAddress.erase(address);
}


void PexName(void *address, void *parent, const std::string &name)
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


const void * GetParent(const void *address)
{
    if (!address)
    {
        return NULL;
    }

    if (!namesByAddress.count(address))
    {
        return NULL;
    }

    const auto &entry = namesByAddress[address];

    return entry.parent;
}


std::string LookupPexName(const void *address, int indent)
{
    assert(indent < 64);

    auto indentString = fields::MakeIndent(indent);

    if (address == nullptr)
    {
        return indentString + "NULL";
    }

    if (namesByAddress.count(address))
    {
        auto &name = namesByAddress[address];

        return FormatName(address, name, indent);
    }
    else if (deletedNamesByAddress.count(address))
    {
        auto &name = deletedNamesByAddress[address];

        return FormatDeletedName(address, name, indent);
    }

    return fmt::format("{}{}", indentString, address);
}


void ResetPexNames()
{
    namesByAddress.clear();
    deletedNamesByAddress.clear();
}


}
