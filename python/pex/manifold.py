##
# @file manifold.py
#
# @brief A hub connecting messages between the interface and the model.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from __future__ import annotations
from typing import Set, DefaultDict, Any, Generic
from collections import defaultdict

from .types import SignalCallback, ValueCallback, ValueType
from .reference import GetReference, Reference
from .proxy import SignalProxy, ValueProxy

# Each manifold uses two dictionaries to allow the same callback to be
# used for multiple names, and multiple callbacks for each name.
#
# On Disconnect/ForgetCallback_, The namesByCallback_ dictionary is used to
# retreive the list of all the names that hold a reference to the callback.

class SignalManifold:

    callbacksByName_: DefaultDict[str, Set[SignalProxy]]
    namesByCallback_: DefaultDict[SignalProxy, Set[str]]

    def __init__(self) -> None:
        self.callbacksByName_ = defaultdict(set)
        self.namesByCallback_ = defaultdict(set)

    def Connect(self, name: str, callback: SignalCallback) -> None:
        callbackProxy = SignalProxy.Create(callback, self.ForgetCallback_)
        self.namesByCallback_[callbackProxy].add(name)
        self.callbacksByName_[name].add(callbackProxy)

    def Disconnect(self, callback: SignalCallback) -> None:
        self.ForgetCallback_(GetReference(callback))

    def DisconnectName(self, name: str) -> None:
        callbacks: Set[SignalProxy] = self.callbacksByName_.get(name, set())

        for callback in callbacks:
            self.ForgetProxy_(callback)

    def Publish(self, name: str) -> None:
        """Calls each of the callbacks registered to this name.

        Note: The callbackRef is not checked for None. The weakref callback
        allows us to disconnect before the callback reference is finalized.
        """
        for callbackProxy in self.callbacksByName_.get(name, set()):
            callbackProxy()

    def ForgetCallback_(
            self,
            reference: Reference[SignalCallback]) -> None:
        """
        The referent is about to be finalized. Remove our references to it.
        """
        # We have stored SignalProxy(s).
        self.ForgetProxy_(SignalProxy(reference))

    def ForgetProxy_(self, signalProxy: SignalProxy) -> None:
        connectedNames = \
            self.namesByCallback_.pop(signalProxy, set())

        for name in connectedNames:
            self.callbacksByName_[name].discard(signalProxy)

            if len(self.callbacksByName_[name]) == 0:
                # All callbacks for this name have been disconnected
                # Remove the name.
                self.callbacksByName_.pop(name)


class ValueManifold:
    callbacksByName_: DefaultDict[str, Set[ValueProxy[Any]]]
    namesByCallback_: DefaultDict[ValueProxy[Any], Set[str]]

    def __init__(self) -> None:
        self.callbacksByName_ = defaultdict(set)
        self.namesByCallback_ = defaultdict(set)

    def Connect(self, name: str, callback: ValueCallback[Any]) -> None:
        callbackProxy = ValueProxy.Create(callback, self.ForgetCallback_)
        self.namesByCallback_[callbackProxy].add(name)
        self.callbacksByName_[name].add(callbackProxy)

    def Disconnect(self, callback: ValueCallback[Any]) -> None:
        self.ForgetCallback_(GetReference(callback))

    def DisconnectName(self, name: str) -> None:
        callbacks: Set[ValueProxy[Any]] = self.callbacksByName_.get(name, set())

        for callback in callbacks:
            self.ForgetProxy_(callback)

    def Publish(self, name: str, value: Any) -> None:
        """Calls each of the callbacks registered to this name.

        Note: The callbackRef is not checked for None. The weakref callback
        allows us to disconnect before the callback reference is finalized.
        """
        for callbackProxy in self.callbacksByName_.get(name, []):
            callbackProxy(value)

    def ForgetCallback_(
            self,
            reference: Reference[ValueCallback[Any]]) -> None:
        """
        The referent is about to be finalized. Remove our references to it.
        """

        # We have stored ValueProxy(s).
        self.ForgetProxy_(ValueProxy(reference))

    def ForgetProxy_(self, valueProxy: ValueProxy[Any]) -> None:
        connectedNames = \
            self.namesByCallback_.pop(valueProxy, set())

        for name in connectedNames:
            self.callbacksByName_[name].discard(valueProxy)

            if len(self.callbacksByName_[name]) == 0:
                # All callbacks for this name have been disconnected
                # Remove the name.
                self.callbacksByName_.pop(name)



class ValueCallbackManifold(Generic[ValueType]):
    """
    The set of registered callbacks for a value.

    Automatically removes callbacks when they are finalized.
    """

    callbacks_: Set[ValueProxy[ValueType]]

    def __init__(self) -> None:
        self.callbacks_ = set()

    def ForgetCallback_(
            self,
            reference: Reference[ValueCallback[ValueType]]) -> None:
        """
        The referent has been deleted.

        Remove it from the set of callbackReferences so that it is not called
        in the future.
        """
        valueProxy = ValueProxy(reference)
        self.callbacks_.discard(valueProxy)

    def Add(self, callback: ValueCallback[ValueType]) -> None:
        valueProxy = ValueProxy.Create(callback, self.ForgetCallback_)
        self.callbacks_.add(valueProxy)

    def Clear(self) -> None:
        self.callbacks_.clear()

    def Disconnect(self, callback: ValueCallback[ValueType]) -> None:
        self.ForgetCallback_(GetReference(callback))

    def __call__(self, value: ValueType) -> None:
        for callbackProxy in self.callbacks_:
            callbackProxy(value)
