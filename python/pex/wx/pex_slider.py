##
# @file pex_slider.py
#
# @brief A wx.Slider connected to a pex.Value interface node.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from __future__ import annotations
from typing import List, Generic, TypeVar, Callable, Any

import wx
from .. import pex
from .pex_window import PexWindow
from ..types import Reference
from ..proxy import ConverterProxy


T = TypeVar('T', int, float)

# toValue: Callable[[int], T] = PassThrough[T].FromInt,
# fromValue: Callable[[T], int] = PassThrough[T].ToInt) \


class SliderInterface(Generic[T]):
    value: pex.Value[T]
    minimum: pex.Value[T]
    maximum: pex.Value[T]
    toValue: ConverterProxy[int, T]
    fromValue: ConverterProxy[T, int]

    @staticmethod
    def FromInt(value: int) -> T:
        return value

    @staticmethod
    def ToInt(value: T) -> int:
        return int(value)

    @classmethod
    def CreateModelNode(
            class_,
            name: str,
            initialValue: T,
            minimum: T,
            maximum: T,
            toValue: Callable[[int], T] = SliderInterface.FromInt,
            fromValue: Callable[[T], int] = SliderInterface.ToInt) \
                -> SliderInterface[T]:
        
        instance = class_()

        instance.value = pex.Value.CreateModelNode(
            "{}.value".format(name),
            initialValue)

        instance.minimum = pex.Value.CreateModelNode(
            "{}.minimum".format(name),
            minimum)

        instance.maximum = pex.Value.CreateModelNode(
            "{}.maximum".format(name),
            maximum)

        instance.toValue = ConverterProxy.Create(
            toValue,
            instance.RestoreDefaultToValue_)

        instance.fromValue = ConverterProxy.Create(
            fromValue,
            instance.RestoreDefaultFromValue_)

        return instance
    
    def RestoreDefaultToValue_(
            self,
            ignored: Reference[Callable[[int], T]]) -> None:
        self.toValue = ConverterProxy.Create(self.FromInt, None)

    def RestoreDefaultFromValue_(
            self,
            ignored: Reference[Callable[[T], int]]) -> None:
        self.fromValue = ConverterProxy.Create(self.ToInt, None)

    def GetInterfaceNode(self) -> SliderInterface:
        interfaceNode = type(self)()
        interfaceNode.value = self.value.GetInterfaceNode()
        interfaceNode.minimum = self.minimum.GetInterfaceNode()
        interfaceNode.maximum = self.maximum.GetInterfaceNode()
        interfaceNode.toValue = self.toValue
        interfaceNode.fromValue = self.fromValue
        return interfaceNode


class PexSlider(wx.Control, PexWindow, Generic[T]):
    defaultValue_: T
    sliderInterface_: SliderInterface

    def __init__(
            self,
            parent: wx.Window,
            name: str,
            sliderInterface: SliderInterface,
            **kwargs: Any) -> None:

        self.defaultValue_ = sliderInterface.value.Get()
        self.sliderInterface_ = sliderInterface.GetInterfaceNode()

        wx.Control.__init__(self, parent, **kwargs)

        PexWindow.__init__(
            self,
            [
                self.sliderInterface_.value,
                self.sliderInterface_.minimum,
                self.sliderInterface_.maximum])

        self.slider_ = wx.Slider(
            self,
            value=self.sliderInterface_.value.Get(),
            minValue=self.sliderInterface_.minimum.Get(),
            maxValue=self.sliderInterface_.maximum.Get(),
            name=name)

        self.sliderInterface_.value.Connect(self.OnPexValue_)
        self.sliderInterface_.minimum.Connect(self.OnPexMinimum_)
        self.sliderInterface_.maximum.Connect(self.OnPexMaximum_)

        self.slider_.Bind(wx.EVT_SLIDER, self.OnSlider_)
        self.slider_.Bind(wx.EVT_LEFT_DOWN, self.OnSliderLeftDown_)
    
    def OnPexValue_(self, value: T) -> None:
        self.slider_.SetValue(self.sliderInterface_.fromValue(value))

    def OnPexMinimum_(self, minimum: T) -> None:
        self.slider_.SetMin(self.sliderInterface_.fromValue(minimum))

    def OnPexMaximum_(self, maximum: T) -> None:
        self.slider_.SetMax(self.sliderInterface_.fromValue(maximum))

    def OnSlider_(self, wxEvent: wx.CommandEvent) -> None:
        value = self.sliderInterface_.toValue(wxEvent.GetInt())
        self.sliderInterface_.value.Set(value)

    def OnSliderLeftDown_(self, wxEvent: wx.CommandEvent) -> None:
        """ Set the slider back to the default value. """
        self.slider_.SetValue(
            self.sliderInterface_.fromValue(self.defaultValue_))


class PexSliderAndValue(wx.Control, PexWindow, Generic[T]):
    sliderInterface_: SliderInterface
    pexSlider_: PexSlider
    formatString_: str
    valueDisplay_: wx.StaticText

    def __init__(
            self,
            parent: wx.Window,
            name: str,
            sliderInterface: SliderInterface,
            formatString: str = "{}",
            **sliderKwargs: Any) -> None:
        
        self.sliderInterface_ = sliderInterface.GetInterfaceNode()
        
        wx.Control.__init__(self, parent)
        PexWindow.__init__(self, [self.sliderInterface_.value, ])
        self.pexSlider_ = PexSlider(parent, name, sliderInterface, **sliderKwargs)
        self.formatString_ = formatString

        self.valueDisplay_ = wx.StaticText(
            self,
            label=self.formatString_.format(self.sliderInterface_.value.Get()))

        self.sliderInterface_.value.Connect(self.UpdateValueDisplay)

        self.sizer_ = wx.BoxSizer(wx.HORIZONTAL)
        self.sizer_.AddMany(
            (self.pexSlider_, 1, wx.RIGHT | wx.ALIGN_CENTER_VERTICAL, 5),
            (self.valueDisplay_, 0))

        self.SetSizerAndFit(self.sizer_)

    def UpdateValueDisplay_(self, value: T) -> None:
        self.valueDisplay_.SetLabel(self.formatString_.format(value))
        self.sizer_.Layout()
