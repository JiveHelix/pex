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

    input_: SignalManifold
    output_: SignalManifold

    def __init__(self, name: str, nodeType: NodeType):
        super(Signal, self).__init__(name, nodeType)

        if nodeType == NodeType.model:
            # Model nodes connect to the model manifold and publish to the
            # interface manifold
            self.input_ = Signal.interfaceManifold_
            self.output_ = Signal.modelManifold_
        else:
            assert nodeType == NodeType.interface

            # Interface nodes connect to the interface manifold and publish to
            # the model manifold
            self.input_ = Signal.modelManifold_
            self.output_ = Signal.interfaceManifold_

        self.output_.Connect(self.name_, self.OnSignal_)

    @classmethod
    def CreateInterfaceNode(class_, name: str) -> Signal:
        return class_(name, NodeType.interface)

    @classmethod
    def CreateModelNode(class_, name: str) -> Signal:
        return class_(name, NodeType.model)

    def GetInterfaceNode(self) -> Signal:
        return self.__class__(self.name_, NodeType.interface)

    def Signal(self) -> None:
        self.input_.Publish(self.name_)

    def Connect(self, callback: SignalCallback) -> None:
        self.output_.Connect(self.name_, callback)

    def Disconnect(self, callback: SignalCallback) -> None:
        self.output_.Disconnect(callback)

    def DisconnectAll(self) -> None:
        self.output_.DisconnectName(self.name_)

    def OnSignal_(self) -> None:
        if self.nodeType_ == NodeType.model:
            # Echo the signal back to any interface nodes that might be
            # listening.
            self.Signal()
