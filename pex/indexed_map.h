#pragma once


#include <map>



namespace pex
{


template<typename Value>
using IndexedMap = std::map<size_t, Value>;


template<typename Value>
std::vector<size_t> GetInvalidatedKeys(
    size_t firstToClear,
    IndexedMap<Value> &keyValuePairs)
{
    std::vector<size_t> keys;
    keys.reserve(keyValuePairs.size());

    for (auto &it: keyValuePairs)
    {
        if (it.first >= firstToClear)
        {
            keys.push_back(it.first);
        }
    }

    return keys;
}


template<typename Value>
void ClearInvalidated(
    size_t firstToClear,
    IndexedMap<Value> &keyValuePairs)
{
    auto keys = GetInvalidatedKeys(firstToClear, keyValuePairs);

    for (auto &key: keys)
    {
        keyValuePairs.erase(key);
    }
}


} // end namespace pex
