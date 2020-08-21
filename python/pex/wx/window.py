from typing import List
import wx
from .. import pex


class Window:
    """ A mixin that disconnects pex when the window is destroyed. """
    tubes_: List[pex.Tube]
    wxId_: int

    def __init__(self: wx.Window, tubes: List[pex.Tube]) -> None:
        self.tubes_ = tubes
        self.wxId_ = self.GetId()

        self.Bind(
            wx.EVT_WINDOW_DESTROY,
            self.OnWindowDestroy_,
            id=self.wxId_)

    def OnWindowDestroy_(self, wxEvent: wx.CommandEvent) -> None:
        if wxEvent.GetId() != self.wxId_:
            print("WARNING: Received EVT_WINDOW_DESTROY for another window!")
            return

        wxEvent.Skip()

        for tube in self.tubes_:
            tube.DisconnectAll()

