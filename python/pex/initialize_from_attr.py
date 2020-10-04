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

from .signal import ModelSignal, InterfaceSignal
from .value import ModelValueBase
from .compound_creator import CompoundCreator


from .type_inspection import GetUnsubscriptedType, GetFirstTypeArg

T = TypeVar('T')

argKey_ = 'arg'


def MakeDefault(arg: Optional[T] = None) -> Dict[str, Optional[T]]:
    return {argKey_: arg}


def GetDefault(attribute: attr.Attribute) -> Optional[Any]:
    result = attribute.metadata.get(argKey_, None)
    if result is not None:
        return result

    if isinstance(attribute.default, type(attr.NOTHING)):
        return None

    try:
        return attribute.default.factory # type: ignore
    except AttributeError:
        return attribute.default


def InitializeFromAttr(
        instance: Any,
        prototype: Any,
        prefix: Optional[str] = None) -> None:

    for name, attribute in attr.fields_dict(type(instance)).items():
        if attribute.type is None:
            raise RuntimeError("Cannot resolve attribute.type")

        memberClass = GetUnsubscriptedType(attribute.type, type(instance))

        if prefix is not None:
            nodeName = "{}.{}".format(prefix, name)
        else:
            nodeName = name

        if issubclass(memberClass, ModelSignal):
            setattr(
                instance,
                name,
                cast(ModelSignal, memberClass).Create(nodeName))

            continue

        if issubclass(memberClass, InterfaceSignal):
            setattr(
                instance,
                name,
                cast(
                    InterfaceSignal,
                    memberClass).Create(getattr(prototype, name)))

            continue

        # Get the default value set with the metadata=MakeDefault(...) argument
        # to attr.ib, if any.
        default = GetDefault(attribute)

        if hasattr(prototype, name):
            initialValue = getattr(prototype, name)

        else:
            if default is None:
                default = GetFirstTypeArg(
                    attribute.type,
                    type(instance))

            if callable(default):
                initialValue = default()
            else:
                initialValue = default

        if issubclass(memberClass, ModelValueBase):
            # This is a model node.
            # The Create method takes a name an an initial value.
            setattr(
                instance,
                name,
                cast(ModelValueBase[Any], memberClass).Create(
                    nodeName,
                    initialValue))

        elif issubclass(memberClass, CompoundCreator):
            setattr(
                instance,
                name,
                cast(CompoundCreator[Any], memberClass).Create(
                    nodeName,
                    initialValue))

        elif hasattr(memberClass, "Create"):
            # This memberClass behaves like an interface node.
            setattr(
                instance,
                name,
                memberClass.Create(initialValue))
        else:
            setattr(instance, name, initialValue)
