##
# @file register_model.py
#
# @brief Register a Model class that has inherited from a transformed
# interface.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jul 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from __future__ import annotations
from typing import Optional, Any, Type, TypeVar, Dict

from .transform import (
    Transform,
    GetClassName,
    GetMemberTypeName)

T = TypeVar("T")

# A global record of model classes for a transformed interface
modelByInterfaceName: Dict[str, Type[Any]] = {}


def RegisterModel(class_: Type[T]) -> Type[T]:
    """
    This class decorator is optional, allowing an interface with transformed
    members to be initialized using the InitializeModel function.
    """

    if class_.__base__ is object:
        raise ValueError(
            "The model must be a child class of the interface.")

    name = GetClassName(class_.__base__)
    modelByInterfaceName[name] = class_
    return class_


def GetMemberModel(protoType: Any, name: str) -> Optional[Type[Any]]:

    memberInterface = Transform.classByProtoTypeName.get(
        GetMemberTypeName(protoType, name),
        None)

    if memberInterface is None:
        return None

    memberModel = modelByInterfaceName.get(
        GetClassName(memberInterface),
        None)

    if memberModel is None:
        raise RuntimeError(
            "Member interface is transformed without a registered model.")

    return memberModel


def GetIsModelClass(class_: Type[Any]) -> bool:
    """
    Determines whether class_ is a registered model.

    """
    # Get the name of the model's parent class (the interface class), and check
    # for it in the registered models.
    return GetClassName(class_.__base__) in modelByInterfaceName

