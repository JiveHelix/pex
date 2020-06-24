##
# @file node_initializer.py
#
# @brief Convenience functions for initializing nodes from an attrs-enabled
# interface class.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

import sys

from typing import (
    Optional,
    Any,
    Type,
    TypeVar,
    Dict,
    ForwardRef,
    Union,
    cast,
    Hashable)

import typing

from functools import lru_cache

import attr
from .signal import Signal
from .value import Value, FilteredValue


@lru_cache(32)
def GetClassNamespace_(class_: Hashable) -> Dict[str, Any]:
    return vars(sys.modules[class_.__module__])


@lru_cache(32)
def GetPexTypeImpl(type_: Hashable, class_: Hashable) \
        -> Union[Signal, Value[Any]]:

    """Returns the actual type if type_ is a string."""

    if not isinstance(type_, str):

        if issubclass(cast(type, type_), Signal):
            return cast(Signal, type_)

        if issubclass(cast(type, type_), FilteredValue):
            return cast(FilteredValue, type_)

        if issubclass(cast(type, type_), Value):
            return cast(Value, type_)

        raise RuntimeError(
            "model node must be a Signal or Value[Any]. "
            "Found {}".format(type_))

    forwardRef = ForwardRef(type_, is_argument=False)

    result: Union[Signal, Value[Any]]

    # pylint: disable=protected-access
    evaluated = forwardRef._evaluate(GetClassNamespace_(class_), None)

    if evaluated is None:
        raise RuntimeError("Unable to resolve type {}".format(type_))

    if issubclass(type(evaluated), typing._GenericAlias): # type: ignore
        result = evaluated.__origin__
    else:
        result = evaluated

    if issubclass(cast(type, result), Signal):
        return cast(Signal, result)

    if issubclass(cast(type, result), Value):
        return cast(Value, result)

    raise RuntimeError(
        "model node must be a Signal or Value[Any]. "
        "Found {}".format(result))


def GetPexType(type_: Type[Any], class_: Type[Any]) \
        -> Union[Signal, Value[Any]]:

    # I cannot see how mypy/typeshed/python can allow me to declare that I am
    # passing a union of hashable types.
    # Explicitly cast them here.
    return GetPexTypeImpl(cast(Hashable, type_), cast(Hashable, class_))


argKey_ = 'arg'
T = TypeVar('T')


def ModelValue(arg: Optional[T] = None) -> Dict[str, Optional[T]]:
    return {argKey_: arg}


def GetModelValue(attribute: attr.Attribute) -> Optional[Any]:
    return attribute.metadata.get(argKey_, None)


def ModelNodeInitializer(instance: Any, prefix: str = "") -> None:
    for name, attribute in attr.fields_dict(type(instance)).items():
        # The type of the member may be stored as a string.
        # GetPexType will resolve to the actual class.
        if attribute.type is None:
            raise RuntimeError("Cannot resolve attribute.type")

        memberClass = GetPexType(attribute.type, type(instance))
        modelValue = GetModelValue(attribute)
        modelNode: Union[Signal, Value]
        nodeName = "{}.{}".format(prefix, name)
        # GetPexType has to return a Union
        # Use typing.cast to tell the type system that this is a type.
        if issubclass(cast(type, memberClass), Value):
            if modelValue is None:
                raise ValueError("pex.Value must have initial value")

            if callable(modelValue):
                initialValue = modelValue()
            else:
                initialValue = modelValue

            modelNode = \
                cast(Value[Any], memberClass).CreateModelNode(
                    nodeName,
                    initialValue)
        else:
            assert issubclass(cast(type, memberClass), Signal)
            modelNode = cast(Signal, memberClass).CreateModelNode(nodeName)

        setattr(instance, name, modelNode)


def SignalInitializer(instance: Any) -> None:
    for name in attr.fields_dict(type(instance)):
        setattr(instance, name, Signal.CreateModelNode(name))


def InterfaceNodeInitializer(instance: object, model: Any) -> None:
    for name in attr.fields_dict(type(instance)):
        setattr(instance, name, getattr(model, name).GetInterfaceNode())
