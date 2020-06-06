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


class SignalManifold:
    callbacksByTopic_: DefaultDict[str, Set[SignalProxy]]
    topicsByCallback_: DefaultDict[SignalProxy, Set[str]]

    def __init__(self) -> None:
        self.callbacksByTopic_ = defaultdict(set)
        self.topicsByCallback_ = defaultdict(set)

    def Subscribe(self, topic: str, callback: SignalCallback) -> None:
        callbackProxy = SignalProxy.Create(callback, self.ForgetCallback_)
        self.topicsByCallback_[callbackProxy].add(topic)
        self.callbacksByTopic_[topic].add(callbackProxy)

    def Unsubscribe(self, callback: SignalCallback) -> None:
        self.ForgetCallback_(GetReference(callback))

    def Publish(self, topic: str) -> None:
        """Calls each of the callbacks registered to this topic.

        Note: The callbackRef is not checked for None. The weakref callback
        allows us to unsubscribe before the callback reference is finalized.
        """
        for callbackProxy in self.callbacksByTopic_.get(topic, []):
            callbackProxy()

    def ForgetCallback_(
            self,
            reference: Reference[SignalCallback]) -> None:
        """
        The referent is about to be finalized. Remove our references to it.
        """

        # We have stored SignalProxy(s).
        signalProxy = SignalProxy(reference)
        subscribedTopics = \
            self.topicsByCallback_.pop(signalProxy, set())

        for topic in subscribedTopics:
            self.callbacksByTopic_[topic].discard(signalProxy)

            if len(self.callbacksByTopic_[topic]) == 0:
                # All callbacks for this topic have been unsubscribed
                # Remove the topic.
                self.callbacksByTopic_.pop(topic)


class ValueManifold:
    callbacksByTopic_: DefaultDict[str, Set[ValueCallback[Any]]]
    topicsByCallback_: DefaultDict[ValueCallback[Any], Set[str]]

    def __init__(self) -> None:
        self.callbacksByTopic_ = defaultdict(set)
        self.topicsByCallback_ = defaultdict(set)

    def Subscribe(self, topic: str, callback: ValueCallback[Any]) -> None:
        callbackProxy = ValueProxy.Create(callback, self.ForgetCallback_)
        self.topicsByCallback_[callbackProxy].add(topic)
        self.callbacksByTopic_[topic].add(callbackProxy)

    def Unsubscribe(self, callback: ValueCallback[Any]) -> None:
        self.ForgetCallback_(GetReference(callback))

    def Publish(self, topic: str, value: Any) -> None:
        """Calls each of the callbacks registered to this topic.

        Note: The callbackRef is not checked for None. The weakref callback
        allows us to unsubscribe before the callback reference is finalized.
        """
        for callbackProxy in self.callbacksByTopic_.get(topic, []):
            callbackProxy(value)

    def ForgetCallback_(
            self,
            reference: Reference[ValueCallback[Any]]) -> None:
        """
        The referent is about to be finalized. Remove our references to it.
        """

        # We have stored ValueProxy(s).
        valueProxy = ValueProxy(reference)

        subscribedTopics = \
            self.topicsByCallback_.pop(valueProxy, set())

        for topic in subscribedTopics:
            self.callbacksByTopic_[topic].discard(valueProxy)

            if len(self.callbacksByTopic_[topic]) == 0:
                # All callbacks for this topic have been unsubscribed
                # Remove the topic.
                self.callbacksByTopic_.pop(topic)


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

    def __call__(self, value: ValueType) -> None:
        for callbackProxy in self.callbacks_:
            callbackProxy(value)
