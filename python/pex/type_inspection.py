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
from .value import ModelValue, FilteredModelValue

T = TypeVar('T')

@lru_cache(32)
def GetClassNamespace_(class_: Hashable) -> Dict[str, Any]:
    return vars(sys.modules[class_.__module__])


@lru_cache(32)
def GetUnsubscriptedTypeImpl(
        type_: Hashable,
        parentClass: Hashable) -> Type[Any]:

    if isinstance(type_, type):
        return cast(Type[Any], type_)

    if not isinstance(type_, str):
        # It's not a type and it's not a str.
        # We don't know what to do with it.
        raise ValueError("Bad type argument: {}".format(type_))

    forwardRef = ForwardRef(type_, is_argument=False)

    # pylint: disable=protected-access
    evaluated = forwardRef._evaluate(
        GetClassNamespace_(parentClass),
        None,
        frozenset())

    if evaluated is None:
        raise RuntimeError("Unable to resolve type {}".format(type_))

    if isinstance(evaluated, typing._GenericAlias): # type: ignore
        return evaluated.__origin__
    else:
        return evaluated


@lru_cache(32)
def GetFirstTypeArgImpl_(type_: Hashable, parentClass: Type[Any]) -> Type[Any]:
    """ Returns the actual type, even if type_ is a string. """

    if isinstance(type_, type):
        return type_

    if not isinstance(type_, str):
        # It's not a type and it's not a str.
        # We don't know what to do with it.
        raise ValueError("Bad type argument: {}".format(type_))

    forwardRef = ForwardRef(type_, is_argument=False)

    # pylint: disable=protected-access
    evaluated = forwardRef._evaluate(
        GetClassNamespace_(parentClass),
        None,
        frozenset())

    if evaluated is None:
        raise RuntimeError("Unable to resolve type {}".format(type_))

    if isinstance(evaluated, typing._GenericAlias): # type: ignore
        if isinstance(
                evaluated.__args__[0], typing._GenericAlias): # type: ignore
            # Now use the origin to retrieve the default value type.
            return evaluated.__args__[0].__origin__

        return evaluated.__args__[0]

    return evaluated


def GetUnsubscriptedType(type_: Type[Any], parentClass: Type[Any]) -> Type[Any]:
    """
    Return the unsubscripted type, or if the type_ argument is not
    a GenericAlias, returns the type_.

    GetUnsbuscriptedType(List[int]) -> List
    GetUnsbuscriptedType(ModelValue[str]) -> ModelValue
    GetUnsbuscriptedType(float) -> float

    """

    # I cannot see how mypy/typeshed/python can allow me to declare that I am
    # passing a union of hashable types.
    # Explicitly cast them here.
    return GetUnsubscriptedTypeImpl(
        cast(Hashable, type_),
        cast(Hashable, parentClass))


def GetFirstTypeArg(
        type_: Union[Type[T], str],
        parentClass: Type[Any]) -> Type[T]:

    return GetFirstTypeArgImpl_(
        cast(Hashable, type_),
        cast(Hashable, parentClass))
