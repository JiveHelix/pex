from __future__ import annotations

from typing import Generic, TypeVar, Callable, Optional, cast, Type

import attr

from .value import (
    ModelValue,
    InterfaceValue,
    FilteredReadOnlyValue,
    FilteredInterfaceValue,
    ValueContext)

from .types import ValueCallback
from .transform import TransformModel, TransformInterface
from .initializers import InitializeInterface
from .compound_creator import CompoundCreator


ModelNumber = TypeVar('ModelNumber', int, float)
InterfaceNumber = TypeVar('InterfaceNumber', int, float)


@attr.s(auto_attribs=True)
class Range(Generic[ModelNumber]):
    value: ModelNumber
    minimum: ModelNumber
    maximum: ModelNumber


@TransformModel(Range, ModelValue.Create, init=True)
class RangeModel(CompoundCreator[ModelNumber]):
    @classmethod
    def Create(
            class_,
            name: str,
            value: ModelNumber) -> RangeModel[ModelNumber]:
        """
        Uses the value as the default minimum and maximum.
        Use SetMinimum and SetMaximum to make this instance useful.

        """
        return RangeModel(Range(value, value, value), name)

    def SetMinimum(self, minimum: ModelNumber) -> None:
        minimum = min(minimum, self.maximum.Get())

        # Delay publishing the new minimum until value has been adjusted.
        with ValueContext(self.minimum) as context:
            context.Set(minimum)

            if self.value.Get() < minimum:
                self.value.Set(minimum)

    def SetMaximum(self, maximum: ModelNumber) -> None:
        maximum = max(maximum, self.minimum.Get())

        # Delay publishing the new maximum until value has been adjusted.
        with ValueContext(self.maximum) as context:
            context.Set(maximum)

            if self.value.Get() > maximum:
                self.value.Set(maximum)

    def Set(self, value: ModelNumber) -> None:
        value = max(
            self.minimum.Get(),
            min(value, self.maximum.Get()))

        self.value.Set(value)

    def SetWithoutNotify_(self, value: ModelNumber) -> None:
        value = max(
            self.minimum.Get(),
            min(value, self.maximum.Get()))

        self.value.SetWithoutNotify_(value)

    def Notify_(self) -> None:
        self.value.Notify_()

    def Connect(self, callback: ValueCallback[ModelNumber]) -> None:
        self.value.Connect(callback)

    def Disconnect(self, callback: ValueCallback) -> None:
        self.value.Disconnect(callback)

    def Get(self) -> ModelNumber:
        return self.value.Get()


@TransformInterface(RangeModel, InterfaceValue.Create, init=False)
@attr.s(auto_attribs=True, init=False)
class RangeInterface(Generic[ModelNumber, InterfaceNumber]):

    value: FilteredInterfaceValue[ModelNumber, InterfaceNumber]
    minimum: FilteredReadOnlyValue[ModelNumber, InterfaceNumber]
    maximum: FilteredReadOnlyValue[ModelNumber, InterfaceNumber]

    def __init__(
            self,
            rangeModel: Optional[RangeModel[ModelNumber]],
            name: Optional[str] = None) -> None:

        if rangeModel is not None:
            InitializeInterface(
                self,
                cast(RangeModel[ModelNumber], rangeModel),
                name)

    @classmethod
    def Create(
            class_: Type[RangeInterface[ModelNumber, InterfaceNumber]],
            model: RangeModel[ModelNumber],
            namePrefix: Optional[str] = None) \
                -> RangeInterface[ModelNumber, InterfaceNumber]:

        return class_(model, namePrefix)

    def AttachFilterOnGet(
            self,
            filterOnGet: Callable[[ModelNumber], InterfaceNumber]) -> None:

        self.value.AttachFilterOnGet(filterOnGet)
        self.minimum.AttachFilterOnGet(filterOnGet)
        self.maximum.AttachFilterOnGet(filterOnGet)


    def AttachFilterOnSet(
            self,
            filterOnSet: Callable[[InterfaceNumber], ModelNumber]) -> None:

        self.value.AttachFilterOnSet(filterOnSet)


class RangeFactory(Generic[ModelNumber]):
    value_: ModelNumber
    minimum_: ModelNumber
    maximum_: ModelNumber

    def __init__(
            self,
            value: ModelNumber,
            minimum: ModelNumber,
            maximum: ModelNumber) -> None:

        self.value_ = value
        self.minimum_ = minimum
        self.maximum_ = maximum

    def __call__(self) -> Range[ModelNumber]:
        return Range(self.value_, self.minimum_, self.maximum_)


