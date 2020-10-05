##
# @file chooser.py
#
# @brief Connect a list of choices with a selected value to the user interface.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jul 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from __future__ import annotations
from typing import Generic, List, Optional, cast, Callable, TypeVar, Any, Type
import attr

from .types import Reference, ValueCallback
from .value import ModelValue, InterfaceValue, ReadableValue, ValueContext
from .transform import TransformModel, TransformInterface
from .initializers import InitializeInterface
from .compound_creator import CompoundCreator


ValueType = TypeVar('ValueType')


@attr.s(auto_attribs=True, init=False)
class Chooser(Generic[ValueType]):
    selection: int
    choices: List[ValueType]

    def __init__(
            self,
            value: ValueType,
            choices: Optional[List[ValueType]]) -> None:

        if choices is not None:
            self.choices = cast(List[ValueType], choices)
            self.selection = self.choices.index(value)
        else:
            self.choices = [value, ]
            self.selection = 0

        assert self.selection >= 0, "initialValue must be in initialChoices"


@TransformModel(Chooser, ModelValue.Create, init=True)
class ChooserModel(CompoundCreator[ValueType]):
    @classmethod
    def Create(
            class_,
            name: str,
            value: ValueType) -> ChooserModel[ValueType]:
        """
        Uses the value as the default set of choices.

        """
        return ChooserModel(Chooser(value, None), name)

    def Connect(self, callback: ValueCallback[Any]) -> None:
        self.selection.Connect(callback)

    def Disconnect(self, callback: ValueCallback) -> None:
        self.selection.Disconnect(callback)

    def Get(self) -> ValueType:
        return self.choices.Get()[self.selection.Get()]

    def SetWithoutNotify_(self, value: ValueType) -> None:
        if value not in self.choices.Get():
            print(
                "Warning: adding {} to choices ({})".format(
                    value,
                    self.choices.Get()))

            choices = self.choices.Get()
            choices.append(value)
            self.choices.SetWithoutNotify_(choices)
            self.selection.SetWithoutNotify_(choices.index(value))
        else:
            self.selection.SetWithoutNotify_(self.choices.Get().index(value))

    def Set(self, value: ValueType) -> None:
        self.SetWithoutNotify_(value)
        self.Notify_()

    def Notify_(self) -> None:
        self.selection.Notify_()
        self.choices.Notify_()

    def SetChoices(self, choices: List[ValueType]) -> None:
        with ValueContext(self.selection) as context:
            value = self.Get()
            if value not in choices:
                context.Set(0)
            else:
                context.Set(choices.index(value))

            self.choices.Set(choices)


@TransformInterface(ChooserModel, InterfaceValue.Create, init=False)
class ChooserInterface(Generic[ValueType]):
    # Override choices with a read-only list
    choices: ReadableValue[List[ValueType]]

    def __init__(
            self,
            model: Optional[ChooserModel],
            namePrefix: Optional[str] = None) -> None:

        if model is not None:
            InitializeInterface(self, model, namePrefix)

    @classmethod
    def Create(
            class_: Type[ChooserInterface[ValueType]],
            model: ChooserModel,
            namePrefix: Optional[str] = None) -> ChooserInterface[ValueType]:
        return class_(model, namePrefix)

    def Get(self) -> ValueType:
        return self.choices.Get()[self.selection.Get()]

    def Connect(self, callback: ValueCallback[Any]) -> None:
        self.selection.Connect(callback)

    def Disconnect(self, callback: ValueCallback) -> None:
        self.selection.Disconnect(callback)


class ChooserFactory(Generic[ValueType]):
    selectedValue_: ValueType
    choices_: List[ValueType]

    def __init__(
            self,
            selectedValue: ValueType,
            choices: List[ValueType]) -> None:

        self.selectedValue_ = selectedValue
        self.choices_ = choices

    def __call__(self) -> Chooser[ValueType]:
        return Chooser(self.selectedValue_, self.choices_)


