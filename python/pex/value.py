##
# @file value.py
#
# @brief Synchronizes a value between the interface and the model, with
# both ends allowed to register callbacks to be notified when the value has
# changed.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from __future__ import annotations

from typing import (
    ClassVar,
    Generic,
    TypeVar,
    Type,
    Optional,
    cast,
    Callable,
    NamedTuple,
    Any,
    List)

from types import TracebackType

import abc

from .manifold import ValueManifold, ValueCallbackManifold

from .types import (
    ValueCallback,
    ValueType,
    NodeType,
    Reference)

from .tube import Tube, HasDisconnectAll
from .proxy import FilterProxy


ModelType = TypeVar("ModelType")
InterfaceType = TypeVar("InterfaceType")


class ValueBase(Generic[ModelType, InterfaceType], Tube, HasDisconnectAll):

    """
    Manages a synchronized value between model and the interface.

    A value callback has a single argument with a type parameterized
    as ModelType.
    """
    value_: ModelType

    # Private singleton manifolds to connect the interface to the model.
    interfaceManifold_: ClassVar[ValueManifold] = ValueManifold()
    modelManifold_: ClassVar[ValueManifold] = ValueManifold()

    valueCallbacks_: ValueCallbackManifold[InterfaceType]

    def __init__(
            self,
            name: str,
            nodeType: NodeType,
            initialValue: ModelType) -> None:

        Tube.__init__(self, name, nodeType)

        if nodeType == NodeType.model:
            # Model nodes connect to the model manifold and publish to the
            # interface manifold
            self.output_ = ValueBase.interfaceManifold_
            self.input_ = ValueBase.modelManifold_
        else:
            assert nodeType == NodeType.interface

            # Interface nodes connect to the interface manifold and publish
            # to the model manifold
            self.output_ = ValueBase.modelManifold_
            self.input_ = ValueBase.interfaceManifold_

        self.value_ = initialValue
        self.valueCallbacks_ = ValueCallbackManifold()
        self.input_.Connect(self.name_, self.OnValueChanged_)

    def OnValueChanged_(self, value: ModelType) -> None:
        self.value_ = value
        self.valueCallbacks_(cast(InterfaceType, value))

    def Connect(self, callback: ValueCallback[InterfaceType]) -> None:
        self.valueCallbacks_.Add(callback)

    def Disconnect(self, callback: ValueCallback) -> None:
        self.valueCallbacks_.Disconnect(callback)

    def DisconnectAll(self) -> None:
        self.valueCallbacks_.Clear()

    def __repr__(self) -> str:
        return "{}({}: {})".format(
            type(self).__name__,
            self.name_,
            self.value_)


ModelClass = TypeVar('ModelClass', bound='ModelValueBase')


class ModelValueBase(
        Generic[ModelType],
        ValueBase[ModelType, ModelType]):

    @classmethod
    def Create(
            class_: Type[ModelClass],
            name: str,
            value: ModelType) -> ModelClass:

        return class_(name, NodeType.model, value)

    def OnValueChanged_(self, value: ModelType) -> None:
        super(ModelValueBase, self).OnValueChanged_(value)

        # There may be multiple interface listeners for this model node.
        # When one of them sends a new value, the others should be notified.
        # Interface nodes do not echo, or we would find ourselves in an
        # infinite loop!
        self.output_.Publish(self.name_, self.value_)

    def Get(self) -> ModelType:
        return self.value_

    def Set(self, value: ModelType) -> None:
        self.value_ = value
        self.output_.Publish(self.name_, value)

    def SetWithoutNotify_(self, value: ModelType) -> None:
        self.value_ = value

    def Notify_(self) -> None:
        self.output_.Publish(self.name_, self.value_)



T = TypeVar('T')


class Interface(Generic[T], HasDisconnectAll):
    """
    All read-write interface nodes implement these functions.
    """
    @abc.abstractmethod
    def Get(self) -> T:
        ...

    @abc.abstractmethod
    def Set(self, value: T) -> None:
        ...

    @abc.abstractmethod
    def Connect(self, callback: ValueCallback[T]) -> None:
        ...


InterfaceClass = TypeVar('InterfaceClass', bound='ReadableValue')


class ReadableValue(
        Generic[ModelType],
        ValueBase[ModelType, ModelType]):

    model_: ModelValueBase[ModelType]

    def __init__(self, modelValue: ModelValueBase[ModelType]) -> None:
        super(ReadableValue, self).__init__(
            modelValue.name_,
            NodeType.interface,
            modelValue.value_)

        self.model_ = modelValue

    @classmethod
    def Create(
            class_: Type[InterfaceClass],
            modelValue: ModelValueBase[ModelType]) -> InterfaceClass:

        return cast(
            InterfaceClass,
            ReadableValue(modelValue))

    def Get(self) -> ModelType:
        return self.model_.Get()


class InterfaceValue(ReadableValue[ModelType], Interface[ModelType]):

    """ Adds write access through Set method. """
    def Set(self, value: ModelType) -> None:
        self.output_.Publish(self.name_, cast(ModelType, value))

    @classmethod
    def Create(
            class_: Type[InterfaceClass],
            modelValue: ModelValueBase[ModelType]) \
                -> InterfaceClass:

        return cast(
            InterfaceClass,
            InterfaceValue(modelValue))


def DefaultFilterOnSet(value: InterfaceType) -> ModelType:
    """ Default No-op filter. """
    return cast(ModelType, value)


def DefaultFilterOnGet(value: ModelType) -> InterfaceType:
    """ Default No-op filter. """
    return cast(InterfaceType, value)


FilteredInterface = TypeVar('FilteredInterface', bound='FilteredReadOnlyValue')


class FilteredReadOnlyValue(ValueBase[ModelType, InterfaceType]):

    """
    Use AttachFilter to assign a function that will filter any call
    to Set and any value received from the manifold.
    """

    model_: ModelValueBase[ModelType]
    filterOnGet_: FilterProxy[ModelType, InterfaceType]

    def __init__(self, modelValue: ModelValueBase[ModelType]) -> None:
        super(FilteredReadOnlyValue, self).__init__(
            modelValue.name_,
            NodeType.interface,
            modelValue.value_)

        self.model_ = modelValue
        self.filterOnGet_ = FilterProxy.Create(DefaultFilterOnGet, None)

    @classmethod
    def Create(
            class_: Type[FilteredInterface],
            modelValue: ModelValueBase[ModelType]) -> FilteredInterface:

        return cast(
            FilteredInterface,
            FilteredReadOnlyValue(modelValue))

    def AttachFilterOnGet(
            self,
            filterOnGet: Callable[[ModelType], InterfaceType]) -> None:

        self.filterOnGet_ = \
            FilterProxy.Create(filterOnGet, self.RestoreDefaultFilterOnGet_)

    def RestoreDefaultFilterOnGet_(
            self,
            ignored: Reference[Callable[[ModelType], InterfaceType]]) -> None:

        self.filterOnGet_ = FilterProxy.Create(DefaultFilterOnGet, None)

    def OnValueChanged_(self, value: ModelType) -> None:
        """ Overrides the method in ModelValueBase to insert filterOnGet_ """
        self.valueCallbacks_(self.filterOnGet_(value))

    def Get(self) -> InterfaceType:
        return self.filterOnGet_(self.model_.Get())


class FilteredInterfaceValue(
        Generic[ModelType, InterfaceType],
        FilteredReadOnlyValue[ModelType, InterfaceType],
        Interface[InterfaceType]):
    """
    Use AttachFilter to assign a function that will filter any call
    to Set and any value received from the manifold.
    """
    filterOnSet_: FilterProxy[InterfaceType, ModelType]

    def __init__(self, modelValue: ModelValueBase[ModelType]) -> None:
        super(FilteredInterfaceValue, self).__init__(modelValue)
        self.filterOnSet_ = FilterProxy.Create(DefaultFilterOnSet, None)
        self.filterOnGet_ = FilterProxy.Create(DefaultFilterOnGet, None)

    def AttachFilterOnSet(
            self,
            filterOnSet: Callable[[InterfaceType], ModelType]) -> None:

        self.filterOnSet_ = \
            FilterProxy.Create(filterOnSet, self.RestoreDefaultFilterOnSet_)

    def RestoreDefaultFilterOnSet_(
            self,
            ignored: Reference[Callable[[InterfaceType], ModelType]]) -> None:

        self.filterOnSet_ = FilterProxy.Create(DefaultFilterOnSet, None)

    def Set(self, value: InterfaceType) -> None:
        self.output_.Publish(self.name_, self.filterOnSet_(value))

    @classmethod
    def Create(
            class_: Type[FilteredInterface],
            modelValue: ModelValueBase[ModelType]) -> FilteredInterface:

        return cast(
            FilteredInterface,
            FilteredInterfaceValue[ModelType, InterfaceType](modelValue))


class ModelValue(Generic[ModelType], ModelValueBase[ModelType]):
    @classmethod
    def Create(
            class_: Type[ModelClass],
            name: str,
            value: ModelType) -> ModelClass:

        return cast(
            ModelClass,
            ModelValue(name, NodeType.model, value))

    def GetInterfaceNode(self) -> InterfaceValue[ModelType]:
        """ The default interface type is InterfaceValue.

        Read-only or filtered interface values will have to be created manually.
        """
        return InterfaceValue.Create(self)


class FilteredModelValue(Generic[ModelType], ModelValue[ModelType]):
    """
    Use AttachFilter to assign a function that will filter any call
    to Set. Only interface nodes can filter on Get.
    """
    filterOnSet_: FilterProxy[ModelType, ModelType]

    def __init__(self, name: str, initialValue: ModelType) -> None:
        super(FilteredModelValue, self).__init__(
            name,
            NodeType.model,
            initialValue)

        self.filterOnSet_ = FilterProxy.Create(DefaultFilterOnSet, None)

    def OnValueChanged_(self, value: ModelType) -> None:
        super(FilteredModelValue, self).OnValueChanged_(
            self.filterOnSet_(value))

    def AttachFilterOnSet(
            self,
            filterOnSet: Callable[[ModelType], ModelType]) -> None:

        self.filterOnSet_ = \
            FilterProxy.Create(filterOnSet, self.RestoreDefaultFilterOnSet_)

    def RestoreDefaultFilterOnSet_(
            self,
            ignored: Reference[Callable[[ModelType], ModelType]]) -> None:

        self.filterOnSet_ = FilterProxy.Create(DefaultFilterOnGet, None)

    def Set(self, value: ModelType) -> None:
        super(FilteredModelValue, self).Set(self.filterOnSet_(value))

    def SetWithoutNotify_(self, value: ModelType) -> None:
        super(FilteredModelValue, self).SetWithoutNotify_(
            self.filterOnSet_(value))

    def SetUnfiltered(self, value: ModelType) -> None:
        ModelValue.Set(self, value)

    @classmethod
    def Create(
            class_: Type[ModelClass],
            name: str,
            value: ModelType) -> ModelClass:

        return cast(
            ModelClass,
            FilteredModelValue(name, value))


class ValueContext(Generic[ModelClass]):
    modelValue: ModelClass

    def __init__(self, modelValue: ModelClass) -> None:
        self.modelValue_ = modelValue
        self.originalValue_ = modelValue.Get()

    def Set(self, value: ModelType) -> None:
        self.modelValue_.SetWithoutNotify_(value)

    def __enter__(self) -> ValueContext:
        return self

    def __exit__(
            self,
            exceptionType: Optional[Type[BaseException]],
            exceptionValue: Optional[BaseException],
            exceptionTraceback: Optional[TracebackType]) -> None:

        if exceptionType is None:
            self.modelValue_.Notify_()
        else:
            # An exception was raised.
            # Revert to the previous value and do not Notify.
            self.modelValue_.SetWithoutNotify_(self.originalValue_)


class ChangedNode(NamedTuple):
    node: ModelValueBase
    originalValue: Any


class MultipleValueContext:
    changedNodes_: List[ChangedNode]

    def __init__(self) -> None:
        self.changedNodes_ = []

    def Set(self, node: ModelValueBase[ModelType], value: ModelType) -> None:
        self.changedNodes_.append(ChangedNode(node, node.Get()))
        node.SetWithoutNotify_(value)

    def __enter__(self) -> MultipleValueContext:
        return self

    def __exit__(
            self,
            exceptionType: Optional[Type[BaseException]],
            exceptionValue: Optional[BaseException],
            exceptionTraceback: Optional[TracebackType]) -> None:

        if exceptionType is None:
            # Operation was successful.
            # Notify all of the changed nodes.
            for node, _ in self.changedNodes_:
                node.Notify_()
        else:
            # An exception was raised.
            # Revert to the previous value and do not Notify.
            for node, originalValue in self.changedNodes_:
                node.SetWithoutNotify_(originalValue)
