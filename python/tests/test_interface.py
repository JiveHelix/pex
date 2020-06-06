##
# @file test_interface.py
#
# @brief Test callbacks and notifications through an interface class.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from typing import Generic, TypeVar, Any, cast, Optional
import unittest
import attr

from tests.model import TestModel, TestUser
from tests.interface import Interface, InterfaceNotification

T = TypeVar('T')

@attr.s(auto_attribs=True, eq=False, init=False)
class Controller(Generic[T]):
    value: T
    notifiedName: Optional[str]
    notifiedValue: T

    def __init__(self, value: T, interface: Interface) -> None:
        self.value = value
        self.notifiedName = None
        self.notifiedValue = value

        self.interfaceNotification = InterfaceNotification(
            interface,
            self.OnInterfaceNotification_)

    def OnNotifyValue(self, value: T) -> None:
        self.value = value

    def OnInterfaceNotification_(self, name: str, value: Any) -> None:
        self.notifiedName = name
        self.notifiedValue = cast(T, value)


# pylint: disable=invalid-name

class TestInterface(unittest.TestCase):
    model_: TestModel
    user_: TestUser

    def setUp(self) -> None:
        self.model_ = TestModel()
        self.user_ = TestUser(self.model_)

    def tearDown(self) -> None:
        del self.user_
        del self.model_

    def test_DirectCallback(self) -> None:
        controller = Controller(
            self.model_.brightnessDecay.Get(),
            self.model_)

        self.assertAlmostEqual(
            controller.value,
            self.model_.brightnessDecay.Get())

        self.model_.brightnessDecay.RegisterCallback(controller.OnNotifyValue)
        self.user_.brightnessDecay.Set(3.14)

        self.assertAlmostEqual(self.model_.brightnessDecay.Get(), 3.14)

        self.assertAlmostEqual(controller.value, 3.14)

    def test_InterfaceNotifcation(self) -> None:
        self.model_.brightnessDecay.Set(0.0)

        controller = Controller(
            self.model_.brightnessDecay.Get(),
            self.model_)

        self.assertAlmostEqual(
            controller.notifiedValue,
            self.model_.brightnessDecay.Get())

        self.user_.brightnessDecay.Set(3.14)

        self.assertAlmostEqual(self.model_.brightnessDecay.Get(), 3.14)
        self.assertAlmostEqual(controller.notifiedValue, 3.14)
        self.assertEqual(controller.notifiedName, "brightnessDecay")
