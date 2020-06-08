##
# @file pex_radio_box.py
#
# @brief A wx.RadioBox connected to a pex.Value interface node.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from typing import List, Generic, TypeVar, Callable

import wx
from .. import pex
from .pex_window import PexWindow

T = TypeVar('T')

class PexRadioBox(wx.Control, PexWindow, Generic[T]):
    def __init__(
            self,
            parent: wx.Window,
            value: pex.Value[T],
            choices: List[T],
            valueToString: Callable[[T], str] = str) -> None:

        assert value.Get() in choices
        self.value_ = value.GetInterfaceNode()
        wx.Control.__init__(self, parent)
        PexWindow.__init__(self, self.value_)
        self.choices_ = choices
        self.valueToString_ = valueToString

        self.radioBox_ = wx.RadioBox(
            self,
            choices=[valueToString(value) for value in choices])

        self.radioBox_.SetSelection(choices.index(self.value_.Get()))
        self.value_.Connect(self.OnPexValue_)
        self.radioBox_.Bind(wx.EVT_RADIOBOX, self.OnRadioBox_)

    def OnPexValue_(self, value: T) -> None:
        self.radioBox_.SetSelection(self.choices_.index(value))

    def OnRadioBox_(self, wxEvent: wx.CommandEvent) -> None:
        self.value_.Set(self.choices_[wxEvent.GetSelection()])
