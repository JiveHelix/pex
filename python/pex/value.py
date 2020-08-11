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

from typing import ClassVar, Generic, TypeVar, Type

from .manifold import ValueManifold, ValueCallbackManifold
from .proxy import FilterProxy
from .types import ValueCallback, ValueType, NodeType, FilterCallable, Reference
from .tube import Tube


T = TypeVar('T', bound='Value')


class Value(Generic[ValueType], Tube):

    """
    Manages a synchronized value between model and the interface.

    A value callback has a single argument with a type parameterized
    as ValueType.
    """
    value_: ValueType

    # Private singleton manifolds to connect the interface to the model.
    interfaceManifold_: ClassVar[ValueManifold] = ValueManifold()
    modelManifold_: ClassVar[ValueManifold] = ValueManifold()

    valueCallbacks_: ValueCallbackManifold[ValueType]

    def __init__(self, name: str, nodeType: NodeType, initialValue: ValueType):
        Tube.__init__(self, name, nodeType)

        if nodeType == NodeType.model:
            # Model nodes connect to the model manifold and publish to the
            # interface manifold
            self.output_ = Value.interfaceManifold_
            self.input_ = Value.modelManifold_
        else:
            assert nodeType == NodeType.interface

            # Interface nodes connect to the interface manifold and publish
            # to the model manifold
            self.output_ = Value.modelManifold_
            self.input_ = Value.interfaceManifold_

        self.value_ = initialValue
        self.valueCallbacks_ = ValueCallbackManifold()
        self.input_.Connect(self.name_, self.OnValueChanged_)

    def GetInterfaceNode(self) -> Value[ValueType]:
        return Value[ValueType](self.name_, NodeType.interface, self.value_)

    @classmethod
    def CreateModelNode(
            class_: Type[T],
            name: str,
            value: ValueType) -> T:

        return class_(name, NodeType.model, value)

    def OnValueChanged_(self, value: ValueType) -> None:
        self.value_ = value
        self.valueCallbacks_(value)

        if self.nodeType_ == NodeType.model:
            # There may be multiple interface listeners for this model node.
            # When one of them sends a new value, the others should be notified.
            # Interface nodes do not echo, or we would find ourselves in an
            # infinite loop!
            self.output_.Publish(self.name_, self.value_)

    def Connect(self, callback: ValueCallback[ValueType]) -> None:
        self.valueCallbacks_.Add(callback)

    def Disconnect(self, callback: ValueCallback) -> None:
        self.valueCallbacks_.Disconnect(callback)

    def DisconnectAll(self) -> None:
        self.valueCallbacks_.Clear()

    def Set(self, value: ValueType) -> None:
        self.value_ = value
        self.output_.Publish(self.name_, value)

    def Get(self) -> ValueType:
        return self.value_

    def __repr__(self) -> str:
        return "{}: {}".format(self.name_, self.value_)


def DefaultFilter(value: ValueType) -> ValueType:
    """ Default No-op filter. """
    return value


class FilteredValue(Generic[ValueType], Value[ValueType]):
    """
    Use AttachFilter to assign a function that will filter any call
    to Set and any value received from the manifold.
    """
    filter_: FilterProxy[ValueType]

    def __init__(self, name: str, nodeType: NodeType, initialValue: ValueType):
        super(FilteredValue, self).__init__(name, nodeType, initialValue)
        self.filter_ = FilterProxy.Create(DefaultFilter, None)

    def AttachFilter(
            self,
            filterValue: FilterCallable[ValueType]) -> None:

        self.filter_ = \
            FilterProxy.Create(filterValue, self.RestoreDefaultFilter_)

    def RestoreDefaultFilter_(
            self,
            ignored: Reference[FilterCallable[ValueType]]) -> None:

        self.filter_ = FilterProxy.Create(DefaultFilter, None)

    def SetUnfiltered(self, value: ValueType) -> None:
        Value.Set(self, value)

    def Set(self, value: ValueType) -> None:
        self.value_ = self.filter_(value)
        self.output_.Publish(self.name_, value)

    def OnValueChanged_(self, value: ValueType) -> None:
        self.value_ = self.filter_(value)
        self.valueCallbacks_(self.value_)

        if self.nodeType_ == NodeType.model:
            # There may be multiple interface listeners for this model node.
            # When one of them sends a new value, the others should be notified.
            # Interface nodes do not echo, or we would find ourselves in an
            # infinite loop!
            self.output_.Publish(self.name_, self.value_)

    def GetInterfaceNode(self) -> FilteredValue[ValueType]:
        return FilteredValue[ValueType](
            self.name_,
            NodeType.interface,
            self.value_)

    @classmethod
    def CreateModelNode(
            class_: Type[T],
            name: str,
            value: ValueType) -> T:

        return class_(name, NodeType.model, value)
