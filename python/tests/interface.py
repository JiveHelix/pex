##
# @file interface.py
#
# @brief Classes used in test_interface.py
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from __future__ import annotations
from typing import List, Any, Callable
from functools import partial

import attr
import pex


@attr.s(slots=True, init=False, eq=False)
class Interface:
    brightnessDecay: pex.Value[float] = attr.ib(metadata=pex.ModelValue(0.0))

    def GetValues(self) -> List[pex.Value[Any]]:
        """Return a list of all of the model Values"""
        result: List[pex.Value[Any]] = []

        for name in attr.fields_dict(Interface):
            value = getattr(self, name)
            result.append(value)

        return result


class InterfaceNotification:
    """Stores functools.partial instances to add 'name' to callback values.

    By requiring this class to be destroyed before the notifyCallback, we can
    avoid an extra check for None when dereferencing the callback.

    The simplest way to ensure that notifyCallback will be destroyed after the
    notifyCallback is to use a bound method of the class that owns this
    instance.
    """

    # Store reference-counted partial instances
    namedCalbacks_: List[Callable[[Any], None]]

    # Weakly reference the notify callback to avoid a cyclic reference.
    notifyCallback_: pex.Reference[Callable[[str, Any], None]]

    def __init__(
            self,
            interface: Interface,
            notifyCallback: Callable[[str, Any], None]) -> None:

        self.namedCallbacks_ = []

        for value in interface.GetValues():
            callback = partial(self.Notify_, value.name)

            # pex.Value stores all callbacks as weakrefs, so we must store
            # the partial instances here to avoid premature garbage collection.
            self.namedCallbacks_.append(callback)

            value.Connect(callback)

        self.notifyCallback_ = pex.GetReference(notifyCallback)

    def Notify_(self, name: str, value: Any) -> None:
        notifyCallback = self.notifyCallback_()
        assert notifyCallback is not None
        notifyCallback(name, value)
