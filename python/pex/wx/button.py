import wx
from .. import pex
from .window import Window

class Button(wx.Button, Window):
    signal_: pex.Signal

    def __init__(
            self,
            parent: wx.Window,
            label: str,
            signal: pex.Signal) -> None:

        self.signal_ = signal
        wx.Button.__init__(self, parent, label=label)
        Window.__init__(self, [self.signal_])
        self.Bind(wx.EVT_BUTTON, self.OnButton_)

    def OnButton_(self, ignored: wx.CommandEvent) -> None:
        self.signal_.Signal()

