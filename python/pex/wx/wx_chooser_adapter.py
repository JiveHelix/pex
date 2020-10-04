from __future__ import annotations

from typing import Generic, Any, List, Callable
from ..proxy import FilterProxy
from ..reference import Reference
from ..types import ValueType


def DefaultToString(value: ValueType) -> str:
    return str(value)


class WxChooserAdapter(Generic[ValueType]):

    toString_: FilterProxy[ValueType, str]

    def __init__(
            self,
            toString: Callable[[ValueType], str] = DefaultToString) -> None:

        self.toString_ = FilterProxy.Create(
            toString,
            self.RestoreDefaultToString_)

    def ToString(self, value: ValueType) -> str:
        return self.toString_(value)

    def GetSelectionAsString(self, index: int, choices: List[ValueType]) -> str:
        return self.toString_(choices[index])

    def GetChoicesAsStrings(self, choices: List[ValueType]) -> List[str]:
        return [self.toString_(choice) for choice in choices]

    def RestoreDefaultToString_(
            self,
            ignored: Reference[Callable[[ValueType], str]]) -> None:

        self.toString_ = FilterProxy.Create(DefaultToString, None)

