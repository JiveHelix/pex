##
# @file radio_box.py
#
# @brief A wx.RadioBox connected to a pex.InterfaceValue node.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from typing import List, Generic, TypeVar, Callable

import wx
from .. import pex
from .window import Window
from .wx_chooser_adapter import WxChooserAdapter


ValueType = TypeVar('ValueType')


class RadioBox(wx.Control, Window, Generic[ValueType]):
    chooser_: pex.ChooserInterface[ValueType]
    chooserAdapter_: WxChooserAdapter

    def __init__(
            self,
            parent: wx.Window,
            chooser: pex.ChooserInterface[ValueType],
            toString: Callable[[ValueType], str] = str) -> None:

        self.chooser_ = chooser
        self.chooserAdapter_ = WxChooserAdapter(toString)
        wx.Control.__init__(self, parent)
        Window.__init__(self, [self.chooser_.selection, self.chooser_.choices])

        choices = self.chooserAdapter_.GetChoicesAsStrings(
            self.chooser_.choices.Get())

        self.radioBox_ = wx.RadioBox(
            self,
            choices=choices)

        self.radioBox_.SetSelection(self.chooser_.selection.Get())

        self.chooser_.selection.Connect(self.OnSelection_)
        self.chooser_.choices.Connect(self.OnChoices_)
        self.radioBox_.Bind(wx.EVT_RADIOBOX, self.OnRadioBox_)

    def OnSelection_(self, selection: int) -> None:
        self.radioBox_.SetSelection(selection)

    def OnChoices_(self, ignored: List[ValueType]) -> None:
        # TODO: Replace the radioBox_ when the choices change.
        print(
            "WARNING: RadioBox ignores changes to the choice list. "
            "Create a new RadioBox instead.")

    def OnRadioBox_(self, wxEvent: wx.CommandEvent) -> None:
        self.chooser_.selection.Set(wxEvent.GetSelection())
