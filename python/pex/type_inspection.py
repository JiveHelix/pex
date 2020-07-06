##
# @file type_inspection.py
#
# @brief Retrieves type information from attr.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jul 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from __future__ import annotations
from functools import lru_cache
import sys

from typing import (
    Any,
    Type,
    TypeVar,
    Dict,
    ForwardRef,
    Union,
    cast,
    Hashable)

import typing

from .signal import Signal
from .value import Value, FilteredValue

T = TypeVar('T')

@lru_cache(32)
def GetClassNamespace_(class_: Hashable) -> Dict[str, Any]:
    return vars(sys.modules[class_.__module__])


@lru_cache(32)
def GetPexTypeImpl(type_: Hashable, parentClass: Hashable) \
        -> Union[Signal, Value[Any], FilteredValue[Any]]:

    """Returns the actual type if type_ is a string."""

    if not isinstance(type_, str):

        if issubclass(cast(type, type_), Signal):
            return cast(Signal, type_)

        if issubclass(cast(type, type_), FilteredValue):
            return cast(FilteredValue, type_)

        if issubclass(cast(type, type_), Value):
            return cast(Value, type_)

        raise RuntimeError(
            "model node must be a Signal, Value[Any], or FilteredValue[Any] "
            "Found {}".format(type_))

    forwardRef = ForwardRef(type_, is_argument=False)

    result: Union[Signal, Value[Any], FilteredValue[Any]]

    # pylint: disable=protected-access
    evaluated = forwardRef._evaluate(GetClassNamespace_(parentClass), None)

    if evaluated is None:
        raise RuntimeError("Unable to resolve type {}".format(type_))

    if issubclass(type(evaluated), typing._GenericAlias): # type: ignore
        result = evaluated.__origin__
    else:
        result = evaluated

    if issubclass(cast(type, result), Signal):
        return cast(Signal, result)

    if issubclass(cast(type, result), FilteredValue):
        return cast(FilteredValue, result)

    if issubclass(cast(type, result), Value):
        return cast(Value, result)

    raise RuntimeError(
        "model node must be a Signal or Value[Any]. "
        "Found {}".format(result))


@lru_cache(32)
def GetValueTypeImpl(type_: Hashable, parentClass: Type[Any]) -> Type[Any]:

    """Returns the actual type if type_ is a string."""

    if isinstance(type_, type):
        return type_

    if not isinstance(type_, str):
        raise ValueError("Bad type argument: {}".format(type_))

    forwardRef = ForwardRef(type_, is_argument=False)

    # pylint: disable=protected-access
    evaluated = forwardRef._evaluate(GetClassNamespace_(parentClass), None)

    if evaluated is None:
        raise RuntimeError("Unable to resolve type {}".format(type_))

    if isinstance(evaluated, typing._GenericAlias): # type: ignore
        if isinstance(
                evaluated.__args__[0], typing._GenericAlias): # type: ignore
            # Now use the origin to retrieve the default value type.
            return evaluated.__args__[0].__origin__

        return evaluated.__args__[0]

    return evaluated


def GetPexType(type_: Type[Any], parentClass: Type[Any]) \
        -> Union[Signal, Value[Any], FilteredValue[Any]]:

    # I cannot see how mypy/typeshed/python can allow me to declare that I am
    # passing a union of hashable types.
    # Explicitly cast them here.
    return GetPexTypeImpl(cast(Hashable, type_), cast(Hashable, parentClass))


def GetValueType(type_: Union[Type[T], str], parentClass: Type[Any]) -> Type[T]:
    return GetValueTypeImpl(cast(Hashable, type_), cast(Hashable, parentClass))
