from __future__ import annotations

from typing import (
    TypeVar,
    Callable,
    Type,
    Any,
    Optional,
    List,
    Set,
    ClassVar,
    Dict,
    Generic,
    Union,
    DefaultDict)

import types
import inspect
from collections import defaultdict

import attr

T = TypeVar("T")
Prototype = TypeVar("Prototype")
Attribute = TypeVar("Attribute")


# A global record of transformed prototypes
classByPrototypeNameBySeries: DefaultDict[str, Dict[str, Type[Any]]] = \
    defaultdict(dict)


def GetHasName(attributeMaker: Callable[..., Attribute]) -> bool:
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
        prototype: Union[Any, Type[Any]],
        memberName: str) -> Type[Any]:
    return type(getattr(prototype, memberName))


def GetMemberTypeName(
        prototype: Union[Any, Type[Any]],
        memberName: str) -> str:
    return GetClassName(GetMemberType(prototype, memberName))


def GetDecoratedClass(class_: Type[Any], series: str = 'model') -> Type[Any]:
    typeName = GetClassName(class_)
    classByPrototypeName = classByPrototypeNameBySeries[series]
    return classByPrototypeName[typeName]


def IsPrototype(class_: Type[Any], series: str = 'model') -> bool:
    if series not in classByPrototypeNameBySeries:
        return False

    typeName = GetClassName(class_)
    return typeName in classByPrototypeNameBySeries[series]


def GetTransformedInstanceVars(class_: Type[Any]) -> List[str]:
    return class_.__transform_vars__


def IsTransformed(class_: Type[Any]) -> bool:
    return hasattr(class_, "__transform_vars__")


def GetClassVars(class_: Type[Any]) -> List[str]:
    return class_.__transform_class_vars__
