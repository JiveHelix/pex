##
# @file pex_numeric_field.py
#
# @brief A numeric field connected to a pex.Value interface node.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from typing import Generic, Any
import wx
from .. import pex
from .pex_window import PexWindow

from .utility import (
    NumberValidator,
    Converter,
    IntegerConverter,
    FloatConverter,
    NumberType)


class PexNumericField(wx.Control, PexWindow, Generic[NumberType]):
    """
    A numerical entry field is plumbed to the model through the pex interface.
    """
    value_: pex.Value[NumberType]
    converter_: Converter[NumberType]

    def __init__(
            self,
            parent: wx.Window,
            value: pex.Value[NumberType],
            converter: Converter[NumberType]) -> None:

        self.value_ = value.GetInterfaceNode()
        wx.Control.__init__(self, parent)
        PexWindow.__init__(self, [self.value_,])
        self.converter_ = converter
        valueAsString = self.converter_.Format(self.value_.Get())

        self.field_ = wx.TextCtrl(
            self,
            style=wx.TE_PROCESS_ENTER,
            value=valueAsString,
            validator=NumberValidator())

        self.field_.Bind(wx.EVT_TEXT_ENTER, self.OnEnter_)
        self.field_.Bind(wx.EVT_KILL_FOCUS, self.OnEnter_)
        self.value_.Connect(self.OnValueChanged_)

    def OnEnter_(self, ignored: wx.CommandEvent) -> None:
        try:
            value, valueAsString = \
                self.converter_.Check(self.field_.GetValue())
        except ValueError:
            self.field_.ChangeValue("Error")
            return

        if self.converter_.ComparesEqual(value, self.value_.Get()):
            # value has not changed.
            # Ignore this input.
            return

        self.field_.ChangeValue(valueAsString)
        self.value_.Set(value)

    def OnValueChanged_(self, value: NumberType) -> None:
        self.field_.ChangeValue(self.converter_.Format(value))


class LabeledPexNumericField(Generic[NumberType], wx.Control):
    """
    Combines a label and the TextCtrl entry field.

    """
    numericField_: PexNumericField[NumberType]

    def __init__(
            self,
            parent: wx.Window,
            value: pex.Value[NumberType],
            converter: Converter[NumberType],
            label: str,
            style: Any = wx.HORIZONTAL) -> None:

        super(LabeledPexNumericField, self).__init__(parent)
        self.numericField_ = PexNumericField(self, value, converter)

        label = wx.StaticText(self, label=label)
        sizer = wx.BoxSizer(style)

        if style == wx.HORIZONTAL:
            flag = wx.RIGHT
        else:
            flag = wx.BOTTOM | wx.EXPAND

        sizer.AddMany((
            (label, 0, flag, 5),
            (self.numericField_, 1, flag)))

        self.SetSizerAndFit(sizer)
