##
# @file proxy.py
#
# @brief Implements a proxy for callable references. While similar in
# functionality to weakref.proxy, there are some fundamental differences.
#
# Wraps either a function (with weakref.ref) or a method, with
# weakref.WeakMethod, and __eq__ and __hash__ are plumbed through to the
# underlying ref so that the proxies can be stored in a set or used as keys in
# a dict.
#
# The user must not call these proxies after the weakref has been finalized.
# Use the onFinalize callback to receive a notification.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from __future__ import annotations

from typing import Optional, Callable, Any, Generic, TypeVar
from .reference import Reference, MakeReference
from .types import (
    SignalCallback,
    ValueCallback,
    ValueType,
    ReferenceType)

class SignalProxy:
    reference_: Reference[SignalCallback]

    def __init__(self, reference: Reference[SignalCallback]):
        self.reference_ = reference

    @classmethod
    def Create(
            class_,
            callback: SignalCallback,
            onFinalize: Optional[Callable[[Reference[SignalCallback]], Any]]) \
                -> SignalProxy:

        return class_(MakeReference(callback, onFinalize))

    def __call__(self) -> None:
        """
        Execute the callback without checking for None (in release mode,
        anyway).

        We have ensured that only alive references are stored in this instance.
        """
        callback = self.reference_()
        assert callback is not None
        callback()

    def __hash__(self) -> int:
        return hash(self.reference_)

    def __eq__(self, other: object) -> bool:
        if isinstance(other, SignalProxy):
            return self.reference_ == other.reference_
        elif isinstance(other, ReferenceType):
            return self.reference_ == other
        else:
            raise NotImplementedError("Cannot compare equal")

    def __repr__(self) -> str:
        return "SignalProxy({})".format(hash(self.reference_))


class ValueProxy(Generic[ValueType]):
    reference_: Reference[ValueCallback[ValueType]]

    def __init__(self, reference: Reference[ValueCallback[ValueType]]) -> None:
        self.reference_ = reference

    @classmethod
    def Create(
            class_,
            callback: ValueCallback[ValueType],
            onFinalize: Callable[[Reference[ValueCallback[ValueType]]], Any]) \
                -> ValueProxy[ValueType]:

        return class_(MakeReference(callback, onFinalize))

    def __call__(self, value: ValueType) -> None:
        """
        Execute the callback without checking for None (in release mode,
        anyway).

        It is the responsibility of the client to ensure that only alive
        references are stored in this instance (using the onFinalize callback.
        """
        callback = self.reference_()
        assert callback is not None
        callback(value)

    def __hash__(self) -> int:
        try:
            return hash(self.reference_)
        except TypeError:
            print(
                "Reference must be hashable. If created with attrs, "
                "be sure to use keyword eq=False")
            raise

    def __eq__(self, other: object) -> bool:
        if isinstance(other, ValueProxy):
            return self.reference_ == other.reference_
        elif isinstance(other, ReferenceType):
            return self.reference_ == other
        else:
            raise NotImplementedError("Cannot compare equal")

    def __repr__(self) -> str:
        return "ValueProxy({})".format(hash(self.reference_))


Source = TypeVar('Source')
Target = TypeVar('Target')

class FilterProxy(Generic[Source, Target]):
    reference_: Reference[Callable[[Source], Target]]

    def __init__(
            self,
            reference: Reference[Callable[[Source], Target]]) -> None:

        self.reference_ = reference

    @classmethod
    def Create(
            class_,
            callback: Callable[[Source], Target],
            onFinalize: Optional[
                Callable[[Reference[Callable[[Source], Target]]], Any]]) \
                    -> FilterProxy[Source, Target]:

        return class_(MakeReference(callback, onFinalize))

    def __call__(self, value: Source) -> Target:
        """
        Execute the callback without checking for None (in release mode,
        anyway).

        It is the responsibility of the client to ensure that only alive
        references are stored in this instance (using the onFinalize callback.
        """
        callback = self.reference_()
        assert callback is not None
        return callback(value)
