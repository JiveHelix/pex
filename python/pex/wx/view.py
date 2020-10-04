##
# @file view.py
#
# @brief A read-only view of a pex.InterfaceValue node.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from typing import Generic, TypeVar

import wx
from .. import pex
from .window import Window


ModelType = TypeVar('ModelType')


class View(wx.StaticText, Window, Generic[ModelType]):
    def __init__(
            self,
            parent: wx.Window,
            value: pex.InterfaceValue[ModelType],
            formatString: str = "{}"):

        self.value_ = value

        wx.StaticText.__init__(
            self,
            parent,
            label=formatString.format(value.Get()))

        Window.__init__(self, [self.value_,])
        self.value_.Connect(self.OnValueChanged_)
        self.formatString_ = formatString

    def OnValueChanged_(self, value: ModelType) -> None:
        self.SetLabel(self.formatString_.format(value))
        self.GetParent().Layout()
