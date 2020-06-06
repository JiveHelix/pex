##
# @file value.py
#
# @brief Value synchronizes a value between the interface and the model, with
# both ends allowed to register callbacks to be notified when the value has
# changed.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from __future__ import annotations

from typing import ClassVar, Generic
import abc

from .manifold import ValueManifold, ValueCallbackManifold
from .types import ValueCallback, ValueType, NodeType
from .tube import Tube


class ValueBase(Tube, Generic[ValueType]):
    value_: ValueType

    @classmethod
    @abc.abstractmethod
    def CreateModelNode(
            class_,
            name: str,
            value: ValueType) -> ValueBase[ValueType]:
        ...

    def __repr__(self) -> str:
        return "{}: {}".format(self.name_, self.value_)


class Value(Generic[ValueType], ValueBase[ValueType]):

    """
    Manages a synchronized value between model and the interface.

    A value callback has a single argument with a type parameterized
    as ValueType.
    """

    # Private singleton manifolds to connect the interface to the model.
    interfaceManifold_: ClassVar[ValueManifold] = ValueManifold()
    modelManifold_: ClassVar[ValueManifold] = ValueManifold()

    valueCallbacks_: ValueCallbackManifold[ValueType]

    def __init__(self, name: str, nodeType: NodeType, initialValue: ValueType):
        super(Value, self).__init__(name, nodeType)

        if nodeType == NodeType.model:
            # Model nodes subscribe to model manifold and publish to the
            # interface manifold
            self.valuePublisher_ = Value.interfaceManifold_
            self.valueSubscriber_ = Value.modelManifold_
        else:
            assert nodeType == NodeType.interface

            # Interface nodes subscribe to the interface manifold and publish
            # to the model manifold
            self.valuePublisher_ = Value.modelManifold_
            self.valueSubscriber_ = Value.interfaceManifold_

        self.value_ = initialValue
        self.valueCallbacks_ = ValueCallbackManifold()
        self.valueSubscriber_.Subscribe(self.name_, self.OnValueChanged_)

    @classmethod
    def CreateInterfaceNode(
            class_,
            modelNode: Value[ValueType]) -> Value[ValueType]:
        return class_(modelNode.name_, NodeType.interface, modelNode.value_)

    @classmethod
    def CreateModelNode(
            class_,
            name: str,
            value: ValueType) -> Value[ValueType]:

        return class_(name, NodeType.model, value)

    def GetInterfaceNode(self) -> Value[ValueType]:
        return self.CreateInterfaceNode(self)

    def OnValueChanged_(self, value: ValueType) -> None:
        self.value_ = value
        self.valueCallbacks_(value)

        if self.nodeType_ == NodeType.model:
            # There may be multiple interface listeners for this model node.
            # When one of them sends a new value, the others should be notified.
            # Interface nodes do not echo, or we would find ourselves in an
            # infinite loop!
            self.valuePublisher_.Publish(self.name_, self.value_)

    def RegisterCallback(self, callback: ValueCallback[ValueType]) -> None:
        self.valueCallbacks_.Add(callback)

    def Set(self, value: ValueType) -> None:
        self.value_ = value
        self.valuePublisher_.Publish(self.name_, value)

    def Get(self) -> ValueType:
        return self.value_
