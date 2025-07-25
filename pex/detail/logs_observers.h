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
    void PrintObservers(int indent = 0) const
    {
        std::cout << fields::MakeIndent(indent)
            << "Observers of "
            << LookupPexName(this);

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
        std::cout << fields::MakeIndent(indent) << LookupPexName(observer);
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
            throw std::logic_error("Obsever not found");
        }

        this->entries_.erase(found);
    }
};


} // end namespace detail


} // end namespace pex
