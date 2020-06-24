from __future__ import annotations
from typing import Generic, List, Optional, cast, Callable
from .types import ValueType, Reference
from .value import Value, FilteredValue
from .proxy import ConverterProxy


def DefaultToString(value: ValueType) -> str:
    return str(value)


class Choices(Generic[ValueType]):
    value: FilteredValue[ValueType]
    choices: Value[List[ValueType]]
    valueToString_: ConverterProxy[ValueType, str]

    @classmethod
    def CreateModelNode(
            class_,
            name: str,
            initialValue: ValueType,
            choices: Optional[List[ValueType]] = None,
            valueToString: Callable[[ValueType], str] = DefaultToString) \
                -> Choices[ValueType]:

        instance = class_()

        instance.value = FilteredValue.CreateModelNode(
            "{}.value".format(name),
            initialValue)

        initialChoices: List[ValueType]

        if choices is not None:
            initialChoices = cast(List[ValueType], choices)
        else:
            initialChoices = [initialValue, ]

        instance.choices = Value.CreateModelNode(
            "{}.choices".format(name),
            initialChoices)

        instance.valueToString_ = ConverterProxy.Create(
            valueToString,
            instance.RestoreDefaultToString_)

        return instance

    def RestoreDefaultToString_(
            self,
            ignored: Reference[Callable[[ValueType], str]]) -> None:

        self.valueToString_ = ConverterProxy.Create(DefaultToString, None)

    def GetInterfaceNode(self) -> Choices[ValueType]:
        interfaceNode: Choices[ValueType] = Choices()
        interfaceNode.value = self.value.GetInterfaceNode()
        interfaceNode.choices = self.choices.GetInterfaceNode()
        interfaceNode.valueToString_ = self.valueToString_
        return interfaceNode

    def GetValueAsString(self) -> str:
        return self.valueToString_(self.value.Get())

    def GetChoicesAsStrings(self) -> List[str]:
        return [self.valueToString_(value) for value in self.choices.Get()]

    def GetValueInChoices(self) -> bool:
        return self.value.Get() in self.choices.Get()
