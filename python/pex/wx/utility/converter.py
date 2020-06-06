##
# @file converter.py
#
# @brief Converts between textual and numeric representation using rules set by
# range bounds and format string.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from typing import Generic, TypeVar, Optional, Tuple
import math
import abc


NumberType = TypeVar("NumberType", int, float)

class Converter(Generic[NumberType], abc.ABC):
    rangeLow_: Optional[NumberType]
    rangeHigh_: Optional[NumberType]
    formatString_: str

    def __init__(
            self,
            rangeLow: Optional[NumberType] = None,
            rangeHigh: Optional[NumberType] = None,
            formatString: str = "{}") -> None:

        self.rangeLow_ = rangeLow
        self.rangeHigh_ = rangeHigh
        self.formatString_ = formatString

    def Format(self, value: NumberType) -> str:
        return self.formatString_.format(value)

    def Check(self, value: str) -> Tuple[NumberType, str]:
        valueAsNumber = self.Convert(value)

        if self.rangeLow_ is not None:
            valueAsNumber = max(self.rangeLow_, valueAsNumber)

        if self.rangeHigh_ is not None:
            valueAsNumber = min(valueAsNumber, self.rangeHigh_)

        return (valueAsNumber, self.Format(valueAsNumber))

    @staticmethod
    @abc.abstractmethod
    def ComparesEqual(first: NumberType, second: NumberType) -> bool:
        ...

    @abc.abstractmethod
    def Convert(self, value: str) -> NumberType:
        ...


class IntegerConverter(Converter[int]):
    @staticmethod
    def Convert(value: str) -> int:
        return int(value)

    @staticmethod
    def ComparesEqual(first: int, second: int) -> bool:
        return first == second


class FloatConverter(Converter[float]):
    @staticmethod
    def Convert(value: str) -> float:
        return float(value)

    @staticmethod
    def ComparesEqual(first: float, second: float) -> bool:
        return math.isclose(first, second)
