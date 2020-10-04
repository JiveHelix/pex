from typing import Generic, Any, List, Callable
import wx
from ..types import ValueType
from .. import pex
from .window import Window
from .wx_chooser_adapter import WxChooserAdapter


def DefaultToString(value: ValueType) -> str:
    return str(value)


class ComboBox(wx.ComboBox, Window, Generic[ValueType]):
    """
    A read-only wx.ComboBox backed by a pex.InterfaceValue.
    """

    chooser_: pex.ChooserInterface[ValueType]
    chooserAdapter_: WxChooserAdapter[ValueType]

    def __init__(
            self,
            parent: wx.Window,
            chooser: pex.ChooserInterface[ValueType],
            toString: Callable[[ValueType], str] = DefaultToString,
            style: Any = 0) -> None:

        self.chooser_ = chooser
        self.chooserAdapter_ = WxChooserAdapter(toString)

        value = self.chooserAdapter_.ToString(self.chooser_.Get())

        choices = self.chooserAdapter_.GetChoicesAsStrings(
            self.chooser_.choices.Get())

        wx.ComboBox.__init__(
            self,
            parent,
            value=value,
            choices=choices,
            style=style | wx.CB_READONLY)

        Window.__init__(
            self,
            [self.chooser_.selection, self.chooser_.choices])

        self.chooser_.selection.Connect(self.OnSelectionChanged_)
        self.chooser_.choices.Connect(self.OnChoices_)
        self.Bind(wx.EVT_COMBOBOX, self.OnComboBox_)

    def OnSelectionChanged_(self, index: int) -> None:
        self.SetValue(
            self.chooserAdapter_.GetSelectionAsString(
                index,
                self.chooser_.choices.Get()))

    def OnChoices_(self, choices: List[ValueType]) -> None:
        # The available choices have changed.
        # Update what is displayed
        self.Set(self.chooserAdapter_.GetChoicesAsStrings(choices))

        self.SetValue(
            self.chooserAdapter_.GetSelectionAsString(
                self.chooser_.selection.Get(),
                choices))

    def OnComboBox_(self, wxEvent: wx.CommandEvent) -> None:
        index = wxEvent.GetSelection()

        if index < 0:
            return

        self.chooser_.selection.Set(index)
