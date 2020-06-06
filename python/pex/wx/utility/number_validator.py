##
# @file number_validator.py
#
# @brief A wx.Validator that ensures that the entered digits can be converted
# to a number.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from __future__ import annotations
import string

import wx


class NumberValidator(wx.Validator):
    def __init__(self) -> None:
        super(NumberValidator, self).__init__()
        self.Bind(wx.EVT_CHAR, self.OnChar_)

    def Clone(self) -> NumberValidator:
        return NumberValidator()

    def Validate(self, ignored: wx.Window) -> bool:
        value = self.GetWindow().GetValue()

        try:
            float(value)
        except ValueError:
            return False

        return True

    def OnChar_(self, wxEvent: wx.CommandEvent) -> None:
        keyCode = wxEvent.GetKeyCode()

        if keyCode < wx.WXK_SPACE or keyCode == wx.WXK_DELETE or keyCode > 255:
            wxEvent.Skip()
            return

        if chr(keyCode) in string.digits or chr(keyCode) in ('.', '-'):
            # Allow this character to propagate
            wxEvent.Skip()
            return

        if not wx.Validator.IsSilent():
            wx.Bell()
