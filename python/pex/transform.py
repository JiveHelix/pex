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

from .transform_common import *
from .initializers import Initialize
from .value import MultipleValueContext

class Transform(Generic[Attribute, Prototype]):
    """
    Create a new class by converting all of prototype members to a new type
    using attributeMaker.

    If any prototype members have already been transformed, that transform will
    be used, allowing nesting of transformed classes.

    """

    # Instance vars:
    prototype_: Type[Prototype]
    attributeMaker_: Callable[..., Attribute]
    init_: bool
    series_: str
    hasName_: bool

    def __init__(
            self,
            prototype: Type[Prototype],
            attributeMaker: Callable[..., Attribute],
            init: bool = False,
            series: str = "model") -> None:

        self.prototype_ = prototype

        # This hack gets around mypy's failure to allow assigning a callable as
        # a member.
        setattr(self, "attributeMaker_", attributeMaker)

        self.init_ = init
        self.series_ = series
        self.hasName_ = GetHasName(attributeMaker)

    def CreateInitMethod_(
            self,
            class_: Type[T],
            instanceVars: List[str]) -> None:

        series = self.series_

        def __init__( # pylint: disable=invalid-name
                self: Any,
                prototype: Prototype,
                namePrefix: Optional[str] = None) -> None:

            if prototype is not None:
                Initialize(
                    series,
                    self,
                    prototype,
                    namePrefix)

        setattr(class_, "__init__", __init__)


    def __call__(self, class_: Type[T]) -> Type[T]:
        """
        Creates a method in the transformed class called TransformMember.

        Any class members (not instance members) will be instantiated here.

        """
        # TODO attributeMaker can be overridden by targetTypeByClassName
        if self.hasName_:
            # The attributeMaker accepts a name argument
            def MakeNodeName(
                    name: str,
                    instanceName: Optional[str] = None) -> str:
                if instanceName is not None:
                    return '.'.join((instanceName, name))
                else:
                    return '.'.join((class_.__name__, name))

            def TransformMember(
                    name: str,
                    prototype: Union[Prototype, Type[Prototype]],
                    instanceName: Optional[str] = None) -> Attribute:

                return self.attributeMaker_(
                    MakeNodeName(name, instanceName),
                    getattr(prototype, name))
        else:
            # attributeMaker has no name argument
            def TransformMember(
                    name: str,
                    prototype: Union[Prototype, Type[Prototype]],
                    instanceName: Optional[str] = None) -> Attribute:

                return self.attributeMaker_(getattr(prototype, name))

        setattr(class_, "TransformMember", staticmethod(TransformMember))

        # Get the class vars from prototype
        # Ignore dunders, private names, attrs-defined members, methods, and
        # functions.
        classVars: List[str] = []

        prototypeMembers: Set[str] = set()

        classVarCandidates: Set[str] = set(dir(self.prototype_))

        if IsTransformed(self.prototype_):
            classVarCandidates.update(GetClassVars(self.prototype_))
            prototypeMembers.update(
                GetTransformedInstanceVars(self.prototype_))

        if attr.has(self.prototype_):
            prototypeMembers.update(attr.fields_dict(self.prototype_).keys())

        if attr.has(class_):
            attrsOverrides = attr.fields_dict(class_)
        else:
            attrsOverrides = {}

        for name in classVarCandidates:

            if name.startswith("_"):
                # Ignoring dunders and private names
                continue

            if name in prototypeMembers:
                # Ignoring member descriptor that should not be initialized
                # until the __init__ method.
                continue

            if name in attrsOverrides:
                # The decorated class overrides a member of the prototype
                # class.
                continue

            memberType: Type[Any] = GetMemberType(self.prototype_, name)

            if memberType in (types.MethodType, types.FunctionType):
                # Ignoring functions and methods
                continue

            # Check the cache for an existing transformation.
            try:
                decoratedClass = GetDecoratedClass(memberType, self.series_)
            except KeyError:
                # This memberType does not have a transformation.
                setattr(
                    class_,
                    name,
                    TransformMember(name, self.prototype_))
            else:
                # This member has already been transformed
                # Instead of the attributeMaker, use the transformed class.
                setattr(
                    class_,
                    name,
                    decoratedClass(getattr(self.prototype_, name), name))

            classVars.append(name)

        setattr(class_, '__transform_class_vars__', classVars)

        transformedInstanceVars = [
            name for name in prototypeMembers
            if name not in attrsOverrides]

        setattr(class_, '__transform_vars__', transformedInstanceVars)

        if self.init_:
            self.CreateInitMethod_(
                class_,
                transformedInstanceVars)

        if self.series_ == "model":
            # Only Model nodes support GetPrototype
            def GetPrototype(instance: Any) -> Prototype:
                """
                Create an instance of the prototype class populated with values
                from the pex nodes.
                """
                values: Dict[str, Any] = {}

                for name in prototypeMembers:
                    node = getattr(instance, name)

                    if hasattr(node, "Get"):
                        value = node.Get()
                    elif hasattr(node, "GetPrototype"):
                        value = node.GetPrototype()
                    else:
                        print(
                            "Warning: {} of {} ignored".format(
                                node, instance))

                    values[name] = value

                return self.prototype_(**values) # type: ignore

            setattr(class_, "GetPrototype", GetPrototype)

            def LoadPrototype(
                    instance: Any,
                    proto: Prototype,
                    context: Optional[MultipleValueContext] = None) -> None:
                """
                Set the values of all nodes using values from the Prototype.
                """
                def AssignMembers(
                        instance: Any,
                        proto: Prototype,
                        context: MultipleValueContext) -> None:

                    for name in prototypeMembers:
                        node = getattr(instance, name)
                        value = getattr(proto, name)

                        if hasattr(node, "Set"):
                            context.Set(node, value)
                        elif hasattr(node, "LoadPrototype"):
                            node.LoadPrototype(value, context)
                        else:
                            print(
                                "Warning: Ignoring {}. "
                                "Missing Set/LoadPrototype.")

                if context is None:
                    with MultipleValueContext() as context:
                        AssignMembers(instance, proto, context)
                else:
                    AssignMembers(instance, proto, context)

            setattr(class_, "LoadPrototype", LoadPrototype)


        # Cache this transformed class for later use.
        classByPrototypeNameBySeries[self.series_][
            GetClassName(self.prototype_)] = class_

        return class_


class TransformModel(
        Generic[Attribute, Prototype],
        Transform[Attribute, Prototype]):

    def __init__(
            self,
            prototype: Type[Prototype],
            attributeMaker: Callable[..., Attribute],
            init: bool = False):
        super(TransformModel, self).__init__(
            prototype,
            attributeMaker,
            init=init,
            series='model')


class TransformInterface(
        Generic[Attribute, Prototype],
        Transform[Attribute, Prototype]):

    def __init__(
            self,
            prototype: Type[Prototype],
            attributeMaker: Callable[..., Attribute],
            init: bool = False):
        super(TransformInterface, self).__init__(
            prototype,
            attributeMaker,
            init=init,
            series='interface')

