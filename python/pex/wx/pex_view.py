##
# @file pex_view.py
#
# @brief A read-only view of a pex.Value interface node.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from typing import Generic, TypeVar

import wx
from .. import pex


T = TypeVar('T')


class PexView(wx.StaticText, Generic[T]):
    def __init__(
            self,
            parent: wx.Window,
            value: pex.Value[T],
            formatString: str = "{:.3f}"):

        super(PexView, self).__init__(
            parent,
            label=formatString.format(value.Get()))

        self.value_ = value
        self.value_.RegisterCallback(self.OnValueChanged_)
        self.formatString_ = formatString

    def OnValueChanged_(self, value: T) -> None:
        self.SetLabel(self.formatString_.format(value))
        self.GetParent().Layout()
