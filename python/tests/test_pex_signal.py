##
# @file test_pex_signal.py
#
# @brief Tests for pex.Signal.
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
class ColorFlags:
    redIsSet: bool = False
    greenIsSet: bool = False
    blueIsSet: bool = False

    def SetRed(self) -> None:
        self.redIsSet = True

    def SetGreen(self) -> None:
        self.greenIsSet = True

    def SetBlue(self) -> None:
        self.blueIsSet = True


# pylint: disable=invalid-name

class TestSignal(unittest.TestCase):
    def test_FunctionCallback(self) -> None:
        isSet = False

        def Set() -> None:
            nonlocal isSet
            isSet = True

        model = pex.Signal.CreateModelNode("value")
        interface = model.GetInterfaceNode()
        interface.Connect(Set)
        model.Signal()

        self.assertTrue(isSet)

    def test_MethodCallback(self) -> None:
        colorFlags = ColorFlags()
        self.assertFalse(colorFlags.greenIsSet)

        model = pex.Signal.CreateModelNode("green")
        interface = model.GetInterfaceNode()
        interface.Connect(colorFlags.SetGreen)
        model.Signal()

        self.assertTrue(colorFlags.greenIsSet)

    def test_MixedFunctionAndMethod(self) -> None:
        model = pex.Signal.CreateModelNode("green")

        colorFlags = ColorFlags()
        otherColorFlags = ColorFlags()

        self.assertFalse(colorFlags.greenIsSet)
        self.assertFalse(otherColorFlags.greenIsSet)

        def SetGreen() -> None:
            otherColorFlags.SetGreen()

        interface = model.GetInterfaceNode()
        interface.Connect(colorFlags.SetGreen)
        interface.Connect(SetGreen)

        model.Signal()

        self.assertTrue(colorFlags.greenIsSet)
        self.assertTrue(otherColorFlags.greenIsSet)

    def test_MultipleMethods(self) -> None:
        model = pex.Signal.CreateModelNode("green")

        colorFlags = ColorFlags()
        otherColorFlags = ColorFlags()

        self.assertFalse(colorFlags.greenIsSet)
        self.assertFalse(otherColorFlags.greenIsSet)

        interface = model.GetInterfaceNode()
        interface.Connect(colorFlags.SetGreen)
        interface.Connect(otherColorFlags.SetGreen)

        model.Signal()

        self.assertTrue(colorFlags.greenIsSet)
        self.assertTrue(otherColorFlags.greenIsSet)

    def test_MultipleCallbacksMultipleTopics(self) -> None:
        greenModel = pex.Signal.CreateModelNode("green")
        redModel = pex.Signal.CreateModelNode("red")

        colorFlags = ColorFlags()
        self.assertFalse(colorFlags.redIsSet)
        self.assertFalse(colorFlags.greenIsSet)

        redInterface = redModel.GetInterfaceNode()
        greenInterface = greenModel.GetInterfaceNode()
        redInterface.Connect(colorFlags.SetRed)
        greenInterface.Connect(colorFlags.SetGreen)

        redModel.Signal()

        self.assertTrue(colorFlags.redIsSet)
        self.assertFalse(colorFlags.greenIsSet)

        greenModel.Signal()

        self.assertTrue(colorFlags.redIsSet)
        self.assertTrue(colorFlags.greenIsSet)

    def test_CallbackDeletion(self) -> None:
        greenModel = pex.Signal.CreateModelNode("green")

        colorFlags = ColorFlags()
        otherColorFlags = ColorFlags()

        self.assertFalse(colorFlags.greenIsSet)
        self.assertFalse(otherColorFlags.greenIsSet)

        interface = greenModel.GetInterfaceNode()
        interface.Connect(colorFlags.SetGreen)
        interface.Connect(otherColorFlags.SetGreen)

        greenModel.Signal()

        self.assertTrue(colorFlags.greenIsSet)
        self.assertTrue(otherColorFlags.greenIsSet)

        otherColorFlags.greenIsSet = False

        del colorFlags

        # The model should have dropped the reference to colorFlags.
        # Calling Set will only call otherColorFlags.SetGreen
        greenModel.Signal()
        self.assertTrue(otherColorFlags.greenIsSet)

    def test_RoundTrip(self) -> None:
        model = pex.Signal.CreateModelNode("green")
        interface = model.GetInterfaceNode()
        anotherInterface = model.GetInterfaceNode()

        self.assertFalse(anotherInterface is interface)

        isSet = False

        def Set() -> None:
            nonlocal isSet
            isSet = True

        anotherInterface.Connect(Set)
        interface.Signal()

        # Check that anotherInterface got the signal
        self.assertTrue(isSet)

    def test_DuplicateModelNode(self) -> None:
        model = pex.Signal.CreateModelNode("green")
        with self.assertRaises(RuntimeError):
            pex.Signal.CreateModelNode("green")

    def test_CopyRaises(self) -> None:
        model = pex.Signal.CreateModelNode("green")

        with self.assertRaises(TypeError):
            copy.copy(model)

        interface = model.GetInterfaceNode()

        with self.assertRaises(TypeError):
            copy.copy(interface)

    def test_DeepCopyRaises(self) -> None:
        model = pex.Signal.CreateModelNode("green")

        with self.assertRaises(TypeError):
            copy.deepcopy(model)

        interface = model.GetInterfaceNode()

        with self.assertRaises(TypeError):
            copy.deepcopy(interface)
