#pragma once


#include <iostream>
#include <vector>
#include <fields/describe.h>
#include <pex/detail/log.h>


namespace pex
{


namespace detail
{


class LogsObservers
{
public:
    using Bullets = std::array<std::string_view, 4>;
    static constexpr Bullets bullets{"∆", "•", "§", "◊"};

    static std::string_view GetBullet(int indent)
    {
        assert(indent >= 0);
        static constexpr size_t bulletCount = bullets.size();

        return bullets[static_cast<size_t>(indent) % bulletCount];
    }

    void PrintObservers(int indent = 0) const
    {
        std::cout << LookupPexName(this, indent);
        std::cout << fields::MakeIndent(indent)
            << GetBullet(indent) << " observed by:";

        if (this->entries_.empty())
        {
            std::cout << " None" << std::endl;

            return;
        }

        for (auto it: this->entries_)
        {
            it.second(indent + 1);
        }

        std::cout << std::endl;
    }

private:
    using Entry = std::pair<void *, std::function<void(int)>>;

    std::vector<Entry> entries_;

    static void PrintObserver(void *observer, int indent = 0)
    {
        std::cout << LookupPexName(observer, indent);

        const void * linkedObserver = GetLinkedObserver(observer);

        while (linkedObserver)
        {
            std::cout << fields::MakeIndent(indent)
                << GetBullet(indent) << " linked observer: "
                << LookupPexName(linkedObserver, indent + 1) << std::endl;

            linkedObserver = GetLinkedObserver(linkedObserver);
            ++indent;
        }
    }

protected:
    template<typename T>
    void RegisterObserver(T *observer)
    {
        if constexpr (std::derived_from<T, LogsObservers>)
        {
            this->entries_.emplace_back(
                observer,
                [observer](int indent)
                {
                    observer->PrintObservers(indent);
                });
        }
        else
        {
            this->entries_.emplace_back(
                observer,
                [observer](int indent)
                {
                    PrintObserver(observer, indent);
                });
        }
    }

    void RemoveObserver(void *observer)
    {
        auto found = std::find_if(
            std::begin(this->entries_),
            std::end(this->entries_),
            [observer](const auto &entry) -> bool
            {
                return observer == entry.first;
            });

        if (found == std::end(this->entries_))
        {
            throw std::logic_error("Observer not found");
        }

        this->entries_.erase(found);
    }
};


} // end namespace detail


} // end namespace pex
