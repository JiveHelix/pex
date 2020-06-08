##
# @file tube.py
#
# @brief The base class for Signal and Value. Makes connections between the
# model and the interface.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from __future__ import annotations
import warnings
from typing import ClassVar, Dict, NoReturn, Any
from .types import NodeType


class Tube:
    """
    Links one model node to one or more interface nodes.

    Each model node must be a singleton. There is no limit to the number of
    interface nodes that can be created.
    """

    modelNodeNames: ClassVar = set()

    name_: str
    nodeType_: NodeType

    def __init__(self, name: str, nodeType: NodeType):
        """ 'Private' constructor. Use one of the classmethods below. """
        if nodeType == NodeType.model:
            if name in Tube.modelNodeNames:
                raise RuntimeError("Model node ({}) exists.".format(name))

            # Store this node name in the singleton list
            Tube.modelNodeNames.add(name)

        self.name_ = name
        self.nodeType_ = nodeType

    def __del__(self) -> None:
        if hasattr(self, "nodeType_"):
            if self.nodeType_ == NodeType.model:
                if self.name_ not in Tube.modelNodeNames:
                    warnings.warn("Unknown model node: {}".format(self.name_))
                else:
                    Tube.modelNodeNames.remove(self.name_)

    def __copy__(self) -> NoReturn:
        raise TypeError(
            "Nodes must not be copied. "
            "Create an interface node instead.")

    def __deepcopy__(self, ignored: Dict[Any, Any]) -> NoReturn:
        raise TypeError(
            "Nodes must not be copied. "
            "Create an interface node instead.")

    def __repr__(self) -> str:
        return self.name_

    @property
    def name(self) -> str:
        return self.name_

    @property
    def nodeType(self) -> NodeType:
        return self.nodeType_
