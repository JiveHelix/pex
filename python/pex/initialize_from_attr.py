##
# @file initialize_from_attr.py
#
# @brief Initialize any Values/Signals that were declared with attrs.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jul 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from __future__ import annotations

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

import attr

from .signal import Signal
from .value import Value

from .type_inspection import GetPexType, GetValueType

T = TypeVar('T')

argKey_ = 'arg'


def ModelValue(arg: Optional[T] = None) -> Dict[str, Optional[T]]:
    return {argKey_: arg}


def GetModelValue(attribute: attr.Attribute) -> Optional[Any]:
    return attribute.metadata.get(argKey_, None)


def InitializeModelFromAttr(
        instance: Any,
        prefix: Optional[str] = None) -> None:

    for name, attribute in attr.fields_dict(type(instance)).items():
        # The type of the member may be stored as a string.
        # GetPexType will resolve to the actual class.
        if attribute.type is None:
            raise RuntimeError("Cannot resolve attribute.type")

        memberClass = GetPexType(attribute.type, type(instance))
        modelValue = GetModelValue(attribute)
        modelNode: Union[Signal, Value]

        if prefix is not None:
            nodeName = "{}.{}".format(prefix, name)
        else:
            nodeName = name

        # GetPexType has to return a Union
        # Use typing.cast to tell the type system that this is a type.
        if issubclass(cast(type, memberClass), Value):
            if modelValue is None:
                modelValue = GetValueType(attribute.type, type(instance))

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


def InitializeInterfaceFromAttr(instance: object, model: Any) -> None:
    for name in attr.fields_dict(type(instance)):
        setattr(instance, name, getattr(model, name).GetInterfaceNode())
