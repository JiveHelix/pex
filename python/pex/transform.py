##
# @file transform.py
#
# @brief A class decorator that creates a new class with all of the
# attributes of a prototype class converted to a new type.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 11 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from __future__ import annotations
from typing import (
    TypeVar,
    Callable,
    Type,
    Any,
    Optional,
    List,
    ClassVar,
    Dict,
    Generic,
    Union)

import types
import inspect
import attr

from .initialize_from_attr import InitializeModelFromAttr


T = TypeVar("T")
ProtoType = TypeVar("ProtoType")
AttributeType = TypeVar("AttributeType")


def GetHasName(attributeMaker: Callable[..., AttributeType]) -> bool:
    """ @return True if 'name' is the first parameter """

    if isinstance(attributeMaker, type):
        # this is a class
        try: # type: ignore
            init = getattr(attributeMaker, '__init__')
        except AttributeError:
            return False

        signature = inspect.signature(init)
    else:
        signature = inspect.signature(attributeMaker)

    return next(iter(signature.parameters.keys())) == 'name'


def GetClassName(class_: Type[Any]) -> str:
    return "{}.{}".format(class_.__module__, class_.__name__)


def GetMemberType(
        protoType: Union[Any, Type[Any]],
        memberName: str) -> Type[Any]:
    return type(getattr(protoType, memberName))


def GetMemberTypeName(
        protoType: Union[Any, Type[Any]],
        memberName: str) -> str:
    return GetClassName(GetMemberType(protoType, memberName))


class Transform(Generic[AttributeType, ProtoType]):
    """
    Create a new class by converting all of protoType members to a new type
    using attributeMaker.

    If any protoType members have already been transformed, that transform will
    be used, allowing nesting of transformed classes.

    """

    # A global record of transformed prototypes
    classByProtoTypeName: ClassVar[Dict[str, Type[Any]]] = {}

    # Instance vars:
    protoType_: Type[ProtoType]
    attributeMaker_: Callable[..., AttributeType]
    init_: bool
    hasName_: bool

    def __init__(
            self,
            protoType: Type[ProtoType],
            attributeMaker: Callable[..., AttributeType],
            init: bool = False) -> None:

        self.protoType_ = protoType
        setattr(self, "attributeMaker_", attributeMaker)
        self.init_ = init
        self.hasName_ = GetHasName(attributeMaker)

    def __call__(self, class_: Type[T]) -> Type[T]:

        if self.hasName_:
            # The attributeMaker accepts a name argument
            def MakeNodeName(
                    name: str,
                    instanceName: Optional[str] = None) -> str:
                if instanceName is not None:
                    return '.'.join((class_.__name__, instanceName, name))
                else:
                    return '.'.join((class_.__name__, name))

            def TransformMember(
                    name: str,
                    protoType: Union[ProtoType, Type[ProtoType]],
                    instanceName: Optional[str] = None) -> AttributeType:

                return self.attributeMaker_(
                    MakeNodeName(name, instanceName),
                    getattr(protoType, name))
        else:
            # attributeMaker has no name argument
            def TransformMember(
                    name: str,
                    protoType: Union[ProtoType, Type[ProtoType]],
                    instanceName: Optional[str] = None) -> AttributeType:

                if instanceName is not None:
                    print(
                        "Warning: instanceName ({}) ignored".format(
                            instanceName))

                return self.attributeMaker_(getattr(protoType, name))


        # Set the class vars from protoType
        classVars: List[str] = []

        # dir, when called on the class itself, only returns ClassVars
        # It will also return member descriptors, which we do not actually
        # want to transform.
        attrsMembers = attr.fields_dict(self.protoType_)

        for name in dir(self.protoType_):

            if name.startswith("_"):
                # Ignoring dunders and private names
                continue

            if name in attrsMembers:
                # Ignoring member descriptor that should not be initialized
                # until the __init__ method.
                continue

            memberType: Type[Any] = GetMemberType(self.protoType_, name)

            if memberType in (types.MethodType, types.FunctionType):
                # Ignoring functions and methods
                continue

            memberClass = Transform.classByProtoTypeName.get(
                GetClassName(memberType),
                None)

            if memberClass is not None:
                # This member has already been transformed
                # Instead of the attributeMaker, use the transformed class.
                setattr(
                    class_,
                    name,
                    memberClass(getattr(self.protoType_, name), name))
            else:
                setattr(
                    class_,
                    name,
                    TransformMember(name, self.protoType_))

            classVars.append(name)


        setattr(class_, "TransformMember", staticmethod(TransformMember))
        setattr(class_, '__transform_all_vars__', classVars)

        instanceVars = list(attr.fields_dict(self.protoType_).keys())
        setattr(class_, '__transform_vars__', instanceVars)

        if self.init_:
            def __init__( # pylint: disable=invalid-name
                    self: Any,
                    protoType: ProtoType,
                    namePrefix: Optional[str] = None) -> None:

                for name in instanceVars:
                    memberClass = Transform.classByProtoTypeName.get(
                        GetMemberTypeName(protoType, name),
                        None)

                    if memberClass is not None:
                        # This member is also a transformed class
                        # Use it directly as a nested transformation
                        if namePrefix is not None:
                            memberName = "{}.{}".format(namePrefix, name)
                        else:
                            memberName = name

                        setattr(
                            self,
                            name,
                            memberClass(getattr(protoType, name), memberName))
                    else:
                        setattr(
                            self,
                            name,
                            TransformMember(name, protoType, namePrefix))

                if attr.has(class_):
                    # Initialize
                    InitializeModelFromAttr(self, namePrefix)

            setattr(class_, "__init__", __init__)

        def GetProtoType(instance: Any) -> ProtoType:
            values: Dict[str, Any] = {
                name: getattr(instance, name).Get()
                for name in instanceVars}

            return self.protoType_(**values) # type: ignore

        setattr(class_, "GetProtoType", GetProtoType)

        # Cache this transformed class for later use.
        Transform.classByProtoTypeName[GetClassName(self.protoType_)] = class_

        return class_


def GetInstanceVars(class_: Type[Any]) -> List[str]:
    return class_.__transform_vars__


def IsTransformed(class_: Type[Any]) -> bool:
    return hasattr(class_, "__transform_vars__")


def GetAllVars(class_: Type[Any]) -> List[str]:
    return class_.__transform_all_vars__
