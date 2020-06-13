from typing import TypeVar, Any, Generic, List
import wx
from .. import pex

pexTube = TypeVar("pexTube", pex.Signal, pex.Value[Any])

class PexWindow(Generic[pexTube]):
    """ A mixin that disconnects pex when the window is destroyed. """

    def __init__(self: wx.Window, tubes: List[pexTube]):
        self.tubes_: List[pexTube] = tubes
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

