##
# @file reference.py
#
# @brief Convenience functions for creating weakref.WeakMethod or weakref.ref
# depending on whether the callable is a function or an instance method.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from __future__ import annotations
from typing import Callable, Any, Optional
from inspect import ismethod
from weakref import WeakMethod, ref
from .types import CallbackType, Reference


def MakeReference(
        callback: CallbackType,
        onFinalize: Optional[Callable[[Reference[CallbackType]], Any]]) \
            -> Reference[CallbackType]:

    if ismethod(callback):
        return WeakMethod(callback, onFinalize)
    else:
        return ref(callback, onFinalize)


def GetReference(callback: CallbackType) -> Reference[CallbackType]:
    return MakeReference(callback, None)
