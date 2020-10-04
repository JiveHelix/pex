##
# @file labeled_widget.py
#
# @brief Attaches a StaticText label to any control.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.


from typing import Any
import wx


def AddLabel(
        parent: wx.Window,
        label:str,
        widget: Any,
        style: Any) -> wx.Sizer:

    label = wx.StaticText(parent, label=label)
    sizer = wx.BoxSizer(style)

    if style == wx.HORIZONTAL:
        flag = wx.RIGHT
    else:
        flag = wx.BOTTOM | wx.EXPAND

    sizer.AddMany((
        (label, 0, flag, 5),
        (widget, 1, flag)))

    return sizer
