##
# @file slider.py
#
# @brief A wx.Slider connected to a pex.RangeInterface node.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from __future__ import annotations
from typing import Generic, Any, TypeVar

import wx
from .. import pex
from ..value import InterfaceValue
from ..range import RangeInterface, ModelNumber, InterfaceNumber

from .window import Window
from .view import View


class Slider(wx.Slider, Window, Generic[ModelNumber]):
    defaultValue_: int
    rangeInterface_: RangeInterface[ModelNumber, int]

    def __init__(
            self,
            parent: wx.Window,
            rangeInterface: RangeInterface[ModelNumber, int],
            **kwargs: Any) -> None:

        self.defaultValue_ = rangeInterface.value.Get()
        self.rangeInterface_ = rangeInterface

        wx.Slider.__init__(
            self,
            parent,
            value=self.rangeInterface_.value.Get(),
            minValue=self.rangeInterface_.minimum.Get(),
            maxValue=self.rangeInterface_.maximum.Get(),
            **kwargs)

        Window.__init__(
            self,
            [
                self.rangeInterface_.value,
                self.rangeInterface_.minimum,
                self.rangeInterface_.maximum])

        self.rangeInterface_.value.Connect(self.OnValue_)
        self.rangeInterface_.minimum.Connect(self.OnMinimum_)
        self.rangeInterface_.maximum.Connect(self.OnMaximum_)

        self.Bind(wx.EVT_SLIDER, self.OnSlider_)
        self.Bind(wx.EVT_LEFT_DOWN, self.OnSliderLeftDown_)

    def OnValue_(self, value: int) -> None:
        self.SetValue(value)

    def OnMinimum_(self, minimum: int) -> None:
        self.SetMin(minimum)

    def OnMaximum_(self, maximum: int) -> None:
        self.SetMax(maximum)

    def OnSlider_(self, wxEvent: wx.CommandEvent) -> None:
        self.rangeInterface_.value.Set(wxEvent.GetInt())

    def OnSliderLeftDown_(self, wxEvent: wx.CommandEvent) -> None:
        """
        Set the slider back to the default value when alt/option key is also
        active.

        """
        if wxEvent.AltDown():
            self.SetValue(self.defaultValue_)
        else:
            wxEvent.Skip()


class SliderAndValue(Generic[ModelNumber], wx.Control):
    pexSlider_: Slider[ModelNumber]
    view_: View[ModelNumber]

    def __init__(
            self,
            parent: wx.Window,
            rangeInterface: RangeInterface[ModelNumber, int],
            value: InterfaceValue[ModelNumber],
            formatString: str = "{}",
            **sliderKwargs: Any) -> None:
        """
        range is filtered to an int for direct use in the wx.Slider.
        value is the unfiltered value from the model for display in the view.
        """
        wx.Control.__init__(self, parent)

        self.pexSlider_ = \
            Slider(parent, rangeInterface, **sliderKwargs)

        self.view_ = View(parent, value)

        self.sizer_ = wx.BoxSizer(wx.HORIZONTAL)
        self.sizer_.AddMany(
            (self.pexSlider_, 1, wx.RIGHT | wx.ALIGN_CENTER_VERTICAL, 5),
            (self.view_, 0))

        self.SetSizerAndFit(self.sizer_)
