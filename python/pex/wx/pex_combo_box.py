from typing import Generic, Any, List
import wx
from ..types import ValueType
from .. import pex
from .pex_window import PexWindow


class PexComboBox(wx.ComboBox, PexWindow, Generic[ValueType]):
    """
    A read-only wx.ComboBox backed by a pex.Value.
    """
    def __init__(
            self,
            parent: wx.Window,
            choices: pex.Choices[ValueType],
            *args: Any,
            **kwargs: Any) -> None:

        self.choices_ = choices.GetInterfaceNode()
        kwargs['choices'] = self.choices_.GetChoicesAsStrings()
        kwargs['style'] = wx.CB_READONLY

        wx.ComboBox.__init__(self, parent, *args, **kwargs)

        PexWindow.__init__(
            self,
            [self.choices_.value, self.choices_.choices])

        if self.choices_.GetValueInChoices():
            self.SetValue(self.choices_.GetValueAsString())

        self.choices_.value.Connect(self.OnValue_)
        self.choices_.choices.Connect(self.OnChoices_)
        self.Bind(wx.EVT_COMBOBOX, self.OnComboBox_)

    def OnValue_(self, ignored: ValueType) -> None:
        self.SetValue(self.choices_.GetValueAsString())

    def OnChoices_(self, ignored: List[ValueType]) -> None:
        # The available choices have changed.
        # Update what is displayed
        self.Set(self.choices_.GetChoicesAsStrings())

    def OnComboBox_(self, ignored: wx.CommandEvent) -> None:
        self.choices_.value.Set(
            self.choices_.choices.Get()[self.GetSelection()])
