from typing import Generic, TypeVar, cast
import wx
from .. import pex
from .pex_window import PexWindow

Number = TypeVar("Number", int, float)

class PexSpinCtrlDouble(wx.SpinCtrlDouble, PexWindow, Generic[Number]):
    """ wx.SpinCtrlDouble backed by a pex.Value. """

    value_: pex.Value[Number]

    def __init__(
            self,
            parent: wx.Window,
            value: pex.Value[Number],
            minimum: Number,
            maximum: Number,
            increment: Number) -> None:

        self.value_ = value.GetInterfaceNode()

        wx.SpinCtrlDouble.__init__(
            self,
            parent,
            min=minimum,
            max=maximum,
            initial=self.value_.Get(),
            inc=increment)

        PexWindow.__init__(self, [self.value_, ])

        self.Bind(
            wx.EVT_SPINCTRLDOUBLE,
            self.OnSpinCtrlDouble_)

        self.value_.Connect(self.OnValue_)

    def OnSpinCtrlDouble_(self, wxEvent: wx.CommandEvent) -> None:
        self.value_.Set(cast(Number, wxEvent.GetValue()))

    def OnValue_(self, value: Number) -> None:
        self.SetValue(value)
