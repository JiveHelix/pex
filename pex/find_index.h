/**
  * @file find_index.h
  *
  * @brief Finds the index of a value found within a container.
  * Currently requires that the container has contiguous layout. It could be
  * implemented using std::distance, but an index into a non-contiguous
  * container has dubious benefit.
  *
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 14 Aug 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <iterator>
#include <type_traits>


template<typename T>
struct IteratorCategory
{
    using Type =
        typename std::iterator_traits<typename T::iterator>::iterator_category;
};

template<typename T, typename = void>
struct HasRandomAccessIterator: std::false_type {};

template<typename T>
struct HasRandomAccessIterator
<
    T,
    std::enable_if_t
    <
        std::is_same_v
        <
            typename IteratorCategory<T>::Type,
            std::random_access_iterator_tag
        >
    >
>: std::true_type {};


template<typename Value, typename Container>
ssize_t FindIndex(const Value &value, const Container &container)
{
    static_assert(
        HasRandomAccessIterator<Container>::value,
        "Requires a container that provides random access iterators.");

    auto found = std::find(container.begin(), container.end(), value);

    if (found == container.end())
    {
        return -1;
    }

    return found - container.begin();
}


template<typename Value, typename Container>
size_t RequireIndex(const Value &value, const Container &container)
{
    auto index = FindIndex(value, container);
    if (index < 0)
    {
        throw std::out_of_range("Item not found in container.");
    }

    return static_cast<size_t>(index);
}
