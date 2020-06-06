##
# @file types.py
#
# @brief Type definitions used throughout the library.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from __future__ import annotations
from enum import Enum, auto
from typing import TypeVar, Callable, Any, TYPE_CHECKING
from weakref import ReferenceType


CallbackType = TypeVar("CallbackType", Callable[..., Any], Callable[[Any], Any])

if TYPE_CHECKING:
    Reference = ReferenceType[CallbackType] # pylint: disable=invalid-name
else:
    # The ReferenceType that is available at runtime does not inherit from
    # Generic.
    Reference = ReferenceType # pylint: disable=invalid-name

SignalCallback = Callable[[], None]

ValueType = TypeVar("ValueType")
ValueCallback = Callable[[ValueType], None]

class NodeType(Enum):
    model = auto()
    interface = auto()
