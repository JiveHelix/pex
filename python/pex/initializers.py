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

from typing import Optional, Any

import attr

from .initialize_from_attr import InitializeFromAttr

from .transform_common import (
    IsTransformed,
    IsPrototype,
    GetDecoratedClass,
    GetTransformedInstanceVars,
    GetMemberType)

from .signal import ModelSignal, InterfaceSignal


def Initialize(
        series: str,
        instance: Any,
        prototype: Any,
        namePrefix: Optional[str] = None) -> None:

    if IsTransformed(type(instance)):
        for name in GetTransformedInstanceVars(type(instance)):
            memberType = GetMemberType(prototype, name)

            if IsPrototype(memberType, series):
                # This member was used as a prototype to a transformed class.
                # Use it directly as a nested transformation
                decoratedClass = GetDecoratedClass(memberType, series)

                if namePrefix is not None:
                    memberName = "{}.{}".format(namePrefix, name)
                else:
                    memberName = name

                setattr(
                    instance,
                    name,
                    decoratedClass(getattr(prototype, name), memberName))

            elif issubclass(memberType, ModelSignal):
                # ModelSignal is always transformed to InterfaceSignal
                setattr(
                    instance,
                    name,
                    InterfaceSignal.Create(getattr(prototype, name)))

            else:
                setattr(
                    instance,
                    name,
                    instance.TransformMember(name, prototype, namePrefix))

    if attr.has(type(instance)):
        # Initialize additional attrs members.
        InitializeFromAttr(instance, prototype, namePrefix)


def InitializeModel(
        instance: Any,
        prototype: Any,
        namePrefix: Optional[str] = None) -> None:

    Initialize("model", instance, prototype, namePrefix)


def InitializeInterface(
        instance: Any,
        prototype: Any,
        namePrefix: Optional[str] = None) -> None:

    Initialize("interface", instance, prototype, namePrefix)
