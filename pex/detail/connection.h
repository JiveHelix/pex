/**
  * @file notify.h
  *
  * @brief Wraps observer and callback for a connection.
  *
  * @author Jive Helix (jivehelix@gmail.com)
  * @date 15 Jul 2020
  * @copyright Jive Helix
  * Licensed under the MIT license. See LICENSE file.
**/

#pragma once

#include <vector>
#include <type_traits>
#include "jive/compare.h"
#include <optional>


namespace pex
{

namespace detail
{

template<typename Observer_, typename Callable_>
class Connection: jive::Compare<Connection<Observer_, Callable_>>
{
public:
    using Observer = Observer_;
    using Callable = Callable_;

    static constexpr auto IsMemberFunction =
        std::is_member_function_pointer_v<Callable>;

    Connection(Observer * const observer, Callable callable)
        :
        observer_(observer),
        callable_(callable)
    {

    }

    /** Conversion from observer pointer for comparisons. **/
    explicit Connection(Observer * const observer)
        :
        observer_(observer),
        callable_{}
    {

    }

    Connection(const Connection &other)
        :
        observer_(other.observer_),
        callable_(other.callable_)
    {

    }

    Connection & operator=(const Connection &other)
    {
        this->observer_ = other.observer_;
        this->callable_ = other.callable_;

        return *this;
    }

    /** Compare using the memory address of observer_ **/
    template<typename Operator>
    bool Compare(const Connection &other) const
    {
        return Operator::Call(this->observer_, other.observer_);
    }

    const Observer * GetObserver() const
    {
        return this->observer_;
    }

protected:
    Observer * observer_;
    Callable callable_;
};


} // namespace detail

} // namespace pex
