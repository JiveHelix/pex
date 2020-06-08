##
# @file test_pex_value.py
#
# @brief Tests for pex.Value.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

import unittest
import copy
import attr
import pex


@attr.s(auto_attribs=True, eq=False)
class Color:
    red: int
    green: int
    blue: int

    def SetRed(self, red: int) -> None:
        self.red = red

    def SetGreen(self, green: int) -> None:
        self.green = green

    def SetBlue(self, blue: int) -> None:
        self.blue = blue

# pylint: disable=invalid-name

class TestPexValue(unittest.TestCase):
    def test_FunctionCallback(self) -> None:
        value = 42

        def SetValue(newValue: int) -> None:
            nonlocal value
            value = newValue

        testPexModel = pex.Value.CreateModelNode("value", value)
        testPexInterface = testPexModel.GetInterfaceNode()
        testPexInterface.Connect(SetValue)
        testPexModel.Set(-56)

        self.assertEqual(value, -56)

    def test_MethodCallback(self) -> None:
        color = Color(0, 128, 224)
        testPexModel = pex.Value.CreateModelNode("green", color.green)
        testPexInterface = testPexModel.GetInterfaceNode()
        testPexInterface.Connect(color.SetGreen)
        testPexModel.Set(64)

        self.assertEqual(color.green, 64)

    def test_MixedFunctionAndMethod(self) -> None:
        testPexModel = pex.Value.CreateModelNode("green", 95)

        color = Color(0, testPexModel.Get(), 224)
        anotherColor = Color(50, testPexModel.Get(), 255)

        def SetGreen(value: int) -> None:
            anotherColor.green = value

        testPexInterface = testPexModel.GetInterfaceNode()
        testPexInterface.Connect(color.SetGreen)
        testPexInterface.Connect(SetGreen)

        testPexModel.Set(100)

        self.assertEqual(anotherColor.green, 100)
        self.assertEqual(color.green, 100)

    def test_MultipleCallbacksMultipleTopics(self) -> None:
        greenModel = pex.Value.CreateModelNode("green", 95)
        redModel = pex.Value.CreateModelNode("red", 201)

        color = Color(redModel.Get(), greenModel.Get(), 224)

        redInterface = redModel.GetInterfaceNode()
        greenInterface = greenModel.GetInterfaceNode()
        redInterface.Connect(color.SetRed)
        greenInterface.Connect(color.SetGreen)

        redModel.Set(113)
        greenModel.Set(42)

        self.assertEqual(color.red, 113)
        self.assertEqual(color.green, 42)

    def test_CallbackDeletion(self) -> None:
        greenModel = pex.Value.CreateModelNode("green", 95)

        color = Color(0, greenModel.Get(), 224)
        anotherColor = Color(50, greenModel.Get(), 255)

        testPexInterface = greenModel.GetInterfaceNode()
        testPexInterface.Connect(color.SetGreen)
        testPexInterface.Connect(anotherColor.SetGreen)

        greenModel.Set(100)

        self.assertEqual(anotherColor.green, 100)
        self.assertEqual(color.green, 100)

        del color

        # The model should have dropped the reference to color.
        # Calling Set will only call anotherColor.SetGreen
        greenModel.Set(87)
        self.assertEqual(anotherColor.green, 87)

    def test_RoundTrip(self) -> None:
        greenModel = pex.Value.CreateModelNode("green", 95)

        greenInterface = greenModel.GetInterfaceNode()
        anotherGreenInterface = greenModel.GetInterfaceNode()
        self.assertFalse(anotherGreenInterface is greenInterface)

        self.assertEqual(greenInterface.Get(), 95)
        self.assertEqual(anotherGreenInterface.Get(), 95)

        greenInterface.Set(112)

        self.assertEqual(greenModel.Get(), 112)
        self.assertEqual(greenInterface.Get(), 112)
        self.assertEqual(anotherGreenInterface.Get(), 112)

    def test_DuplicateModelNode(self) -> None:
        greenModel = pex.Value.CreateModelNode("green", 95)

        with self.assertRaises(RuntimeError):
            pex.Value.CreateModelNode("green", 5)

    def test_CopyRaises(self) -> None:
        greenModel = pex.Value.CreateModelNode("green", 95)

        with self.assertRaises(TypeError):
            copy.copy(greenModel)

        greenInterface = greenModel.GetInterfaceNode()

        with self.assertRaises(TypeError):
            copy.copy(greenInterface)

    def test_DeepCopyRaises(self) -> None:
        greenModel = pex.Value.CreateModelNode("green", 95)

        with self.assertRaises(TypeError):
            copy.deepcopy(greenModel)

        greenInterface = greenModel.GetInterfaceNode()
        with self.assertRaises(TypeError):
            copy.deepcopy(greenInterface)
