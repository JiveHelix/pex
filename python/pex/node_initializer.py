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
    cast)

import typing

from functools import lru_cache

import attr
from .signal import Signal
from .value import Value, ValueBase


@lru_cache(32)
def GetClassNamespace_(class_: Type[Any]) -> Dict[str, Any]:
    return vars(sys.modules[class_.__module__])


@lru_cache(32)
def GetPexType(
        type_: Union[Signal, ValueBase[Any], str],
        class_: Type[Any]) -> Union[Signal, ValueBase[Any]]:

    """Returns the actual type if type_ is a string."""

    if isinstance(type_, str):
        forwardRef = ForwardRef(type_, is_argument=False)

        result: Union[Signal, ValueBase[Any]]

        # pylint: disable=protected-access
        evaluated = forwardRef._evaluate(GetClassNamespace_(class_), None)

        if evaluated is None:
            raise RuntimeError("Unable to resolve type {}".format(type_))

        if issubclass(type(evaluated), typing._GenericAlias): # type: ignore
            result = evaluated.__origin__
        else:
            result = evaluated

        if not (
                issubclass(cast(type, result), Signal)
                or issubclass(cast(type, result), ValueBase)):

            raise RuntimeError(
                "model node must be a Signal or ValueBase[Any]. "
                "Found {}".format(result))

        return result

    else:
        return type_


argKey_ = 'arg'

T = TypeVar('T')


def ModelValue(arg: Optional[T] = None) -> Dict[str, Optional[T]]:
    return {argKey_: arg}


def GetModelValue(attribute: attr.Attribute) -> Optional[Any]:
    return attribute.metadata.get(argKey_, None)


def ModelNodeInitializer(instance: object, interfaceClass: Type[Any]) -> None:
    for name, attribute in attr.fields_dict(interfaceClass).items():
        # The type of the member may be stored as a string.
        # GetPexType will resolve to the actual class.
        memberClass = GetPexType(attribute.type, interfaceClass)
        modelValue = GetModelValue(attribute)
        modelNode: Union[Signal, Value]

        if modelValue is not None:
            assert issubclass(cast(type, memberClass), ValueBase)

            if callable(modelValue):
                initialValue = modelValue()
            else:
                initialValue = modelValue

            modelNode = \
                cast(Value[Any], memberClass).CreateModelNode(
                    name,
                    initialValue)
        else:
            assert issubclass(cast(type, memberClass), Signal)
            modelNode = cast(Signal, memberClass).CreateModelNode(name)

        setattr(instance, name, modelNode)


def InterfaceNodeInitializer(
        instance: object,
        interfaceClass: Type[Any],
        model: Any) -> None:

    for name in attr.fields_dict(interfaceClass):
        setattr(instance, name, getattr(model, name).GetInterfaceNode())
