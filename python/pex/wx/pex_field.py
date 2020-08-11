##
# @file pex_field.py
#
# @brief A text field connected to a pex.Value interface node.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from typing import Any, Optional
import wx
from .. import pex
from .pex_window import PexWindow


class PexField(wx.Control, PexWindow):
    """
    A text entry field is plumbed to the model through the pex interface.
    """
    value_: pex.Value[str]

    def __init__(
            self,
            parent: wx.Window,
            value: pex.Value[str],
            fieldStyle: Optional[Any] = None) -> None:

        self.value_ = value.GetInterfaceNode()
        wx.Control.__init__(self, parent)
        PexWindow.__init__(self, [self.value_,])
        valueAsString = self.value_.Get()

        style = wx.TE_PROCESS_ENTER

        if fieldStyle is not None:
            style |= fieldStyle

        self.field_ = wx.TextCtrl(
            self,
            style=style,
            value=value)

        self.field_.Bind(wx.EVT_TEXT_ENTER, self.OnEnter_)
        self.field_.Bind(wx.EVT_KILL_FOCUS, self.OnEnter_)
        self.value_.Connect(self.OnValueChanged_)

    def OnEnter_(self, ignored: wx.CommandEvent) -> None:
        value = self.field_.GetValue()
        if self.value_.Get() == value:
            # value has not changed.
            # Ignore this input.
            return

        self.field_.ChangeValue(value)
        self.value_.Set(value)

    def OnValueChanged_(self, value: str) -> None:
        self.field_.ChangeValue(value)


class LabeledPexField(wx.Control):
    """
    Combines a label and the TextCtrl entry field.

    """
    field_: PexField

    def __init__(
            self,
            parent: wx.Window,
            value: pex.Value[str],
            label: str,
            fieldStyle: Optional[Any] = None,
            layoutStyle: Any = wx.HORIZONTAL) -> None:

        super(LabeledPexField, self).__init__(parent)
        self.field_ = PexField(self, value, fieldStyle)

        label = wx.StaticText(self, label=label)
        sizer = wx.BoxSizer(layoutStyle)

        if layoutStyle == wx.HORIZONTAL:
            flag = wx.RIGHT
        else:
            flag = wx.BOTTOM | wx.EXPAND

        sizer.AddMany((
            (label, 0, flag, 5),
            (self.field_, 1, flag)))

        self.SetSizerAndFit(sizer)
