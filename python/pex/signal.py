##
# @file signal.py
#
# @brief Signal allows either end of the tube to notify the other, but without
# passing any value.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from __future__ import annotations

from typing import ClassVar

from .manifold import SignalManifold
from .types import SignalCallback, NodeType
from .tube import Tube


class Signal(Tube):

    """
    A signal is published without arguments, and is useful for notifying
    a controller that a user has clicked a button or selected a menu item.
    """

    # Private singleton manifolds to connect the interface to the model.
    interfaceManifold_: ClassVar[SignalManifold] = SignalManifold()
    modelManifold_: ClassVar[SignalManifold] = SignalManifold()

    signalPublisher_: SignalManifold
    signalSubscriber_: SignalManifold

    def __init__(self, name: str, nodeType: NodeType):
        super(Signal, self).__init__(name, nodeType)

        if nodeType == NodeType.model:
            # Model nodes subscribe to the model manifold and publish to the
            # interface manifold
            self.signalPublisher_ = Signal.interfaceManifold_
            self.signalSubscriber_ = Signal.modelManifold_
        else:
            assert nodeType == NodeType.interface

            # Interface nodes subscribe to the interface manifold and publish to
            # the model manifold
            self.signalPublisher_ = Signal.modelManifold_
            self.signalSubscriber_ = Signal.interfaceManifold_

        self.signalSubscriber_.Subscribe(self.name_, self.OnSignal_)

    @classmethod
    def CreateInterfaceNode(class_, name: str) -> Signal:
        return class_(name, NodeType.interface)

    @classmethod
    def CreateModelNode(class_, name: str) -> Signal:
        return class_(name, NodeType.model)

    def GetInterfaceNode(self) -> Signal:
        return self.__class__(self.name_, NodeType.interface)

    def Signal(self) -> None:
        self.signalPublisher_.Publish(self.name_)

    def RegisterCallback(self, callback: SignalCallback) -> None:
        self.signalSubscriber_.Subscribe(self.name_, callback)

    def OnSignal_(self) -> None:
        if self.nodeType_ == NodeType.model:
            # Echo the signal back to any interface nodes that might be
            # listening.
            self.Signal()
