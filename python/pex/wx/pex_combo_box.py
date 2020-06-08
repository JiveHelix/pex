from .. import pex
from .pex_window import PexWindow

class PexComboBox(wx.ComboBox, PexWindow):
    def __init__(
            self,
            parent: wx.Window,
            choices: pex.Value[List[str]],
            value: pex.Value[str],
            *args, **kwargs) -> None:

        self.choices_ = choices.GetInterfaceNode()
        self.value_ = value.GetInterfaceNode()

        kwargs['choices'] = choices.Get()
        wx.ComboBox.__init__(self, parent, *args, **kwargs) 
        PexWindow.__init__(self, [self.choices_, self.value_])

        self.SetValue(self.value_.Get())

        self.choices_.Connect(self.OnChoices_)
        self.value_.Connect(self.OnValue_)

        self.Bind(wx.EVT_COMBOBOX, self.OnComboBox_)

    def OnChoices_(self, choices: List[str]) -> None:
        self.Set(choices)

    def OnValue_(self, value: str) -> None:
        self.SetValue(value)

    def OnComboBox_(self, ignored: wx.CommandEvent) -> None:
        self.value_.Set(self.GetString(self.GetSelection())
