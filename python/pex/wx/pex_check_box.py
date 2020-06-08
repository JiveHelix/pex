##
# @file pex_check_box.py
#
# @brief A CheckBox backed by a pex.Value interface node.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

import wx
from .. import pex
from .pex_window import PexWindow


class PexCheckBox(wx.Control, PexWindow):
    def __init__(
            self,
            parent: wx.Window,
            label: str,
            value: pex.Value[bool]) -> None:

        self.value_ = value.GetInterfaceNode()
        wx.Control.__init__(self, parent)
        PexWindow.__init__(self, self.value_)
        self.value_.Connect(self.OnValueChanged_)
        self.checkBox_ = wx.CheckBox(self, label=label)
        self.checkBox_.SetValue(self.value_.Get())
        self.checkBox_.Bind(wx.EVT_CHECKBOX, self.OnCheckBox_)

    def OnValueChanged_(self, value: bool) -> None:
        self.checkBox_.SetValue(value)

    def OnCheckBox_(self, wxEvent: wx.CommandEvent) -> None:
        self.value_.Set(wxEvent.IsChecked())
