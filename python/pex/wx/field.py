##
# @file numeric_field.py
#
# @brief A numeric field connected to a pex.InterfaceValue node.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from typing import Generic, Any, Optional, TypeVar, Union
import wx
from .. import pex
from .window import Window

from .utility import Converter


T = TypeVar('T')


class Field(wx.Control, Window, Generic[T]):
    """
    A numerical entry field is plumbed to the model through the pex interface.
    """
    value_: pex.Interface[T]
    converter_: Converter[T]
    displayedString_: str

    def __init__(
            self,
            parent: wx.Window,
            value: pex.Interface[T],
            converter: Converter[T],
            fieldStyle: Optional[Any] = None) -> None:

        self.value_ = value
        wx.Control.__init__(self, parent)
        Window.__init__(self, [self.value_,])
        self.converter_ = converter
        self.displayedString_ = self.converter_.ToString(self.value_.Get())

        style = wx.TE_PROCESS_ENTER

        if fieldStyle is not None:
            style |= fieldStyle

        self.field_ = wx.TextCtrl(
            self,
            style=style,
            value=self.displayedString_)

        self.field_.Bind(wx.EVT_TEXT_ENTER, self.OnEnter_)
        self.field_.Bind(wx.EVT_KILL_FOCUS, self.OnEnter_)
        self.value_.Connect(self.OnValueChanged_)

    def OnEnter_(self, event: wx.CommandEvent) -> None:
        event.Skip()
        valueAsString = self.field_.GetValue()

        if valueAsString == self.displayedString_:
            # value has not changed.
            # Ignore this input.
            return

        try:
            value = self.converter_.ToValue(valueAsString)
        except ValueError:
            self.field_.ChangeValue(self.displayedString_)
            return

        self.value_.Set(value)

    def OnValueChanged_(self, value: T) -> None:
        self.displayedString_ = self.converter_.ToString(value)
        self.field_.ChangeValue(self.displayedString_)
