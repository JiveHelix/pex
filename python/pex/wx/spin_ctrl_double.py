from typing import Generic, TypeVar, cast
import wx
from .. import pex
from .window import Window


ModelNumber = TypeVar("ModelNumber", int, float)
InterfaceNumber = TypeVar("InterfaceNumber", int, float)


class SpinCtrlDouble(
        wx.SpinCtrlDouble,
        Window,
        Generic[ModelNumber, InterfaceNumber]):

    """ wx.SpinCtrlDouble backed by a pex.InterfaceValue. """

    valueRange_: pex.RangeInterface[ModelNumber, InterfaceNumber]

    def __init__(
            self,
            parent: wx.Window,
            valueRange: pex.RangeInterface[ModelNumber, InterfaceNumber],
            increment: InterfaceNumber) -> None:

        self.valueRange_ = valueRange

        wx.SpinCtrlDouble.__init__(
            self,
            parent,
            min=self.valueRange_.minimum.Get(),
            max=self.valueRange_.maximum.Get(),
            initial=self.valueRange_.value.Get(),
            inc=increment)

        Window.__init__(
            self,
            [
                self.valueRange_.value,
                self.valueRange_.minimum,
                self.valueRange_.maximum])

        self.Bind(
            wx.EVT_SPINCTRLDOUBLE,
            self.OnSpinCtrlDouble_)

        self.valueRange_.value.Connect(self.OnValue_)
        self.valueRange_.minimum.Connect(self.OnMinimum_)
        self.valueRange_.maximum.Connect(self.OnMaximum_)

    def OnSpinCtrlDouble_(self, wxEvent: wx.CommandEvent) -> None:
        self.value_.Set(cast(InterfaceNumber, wxEvent.GetValue()))

    def OnValue_(self, value: InterfaceNumber) -> None:
        self.SetValue(value)

    def OnMinimum_(self, minimum: InterfaceNumber) -> None:
        maximum = self.valueRange_.maximum.Get()
        self.SetRange(minimum, maximum)

    def OnMaximum_(self, maximum: InterfaceNumber) -> None:
        minimum = self.valueRange_.minimum.Get()
        self.SetRange(minimum, maximum)
