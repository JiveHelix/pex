##
# @file converter.py
#
# @brief Converts between textual and numeric representation.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from typing import Generic, TypeVar, Optional, Tuple
import abc


ValueType = TypeVar("ValueType")


class Converter(Generic[ValueType], abc.ABC):
    formatString_: str

    def __init__(self, formatString: str = "{}") -> None:
        self.formatString_ = formatString

    def ToString(self, value: ValueType) -> str:
        return self.formatString_.format(value)

    @staticmethod
    @abc.abstractmethod
    def ToValue(value: str) -> ValueType:
        ...


class IntegerConverter(Converter[int]):
    @staticmethod
    def ToValue(value: str) -> int:
        return int(value)


class FloatConverter(Converter[float]):
    @staticmethod
    def ToValue(value: str) -> float:
        return float(value)


class StringConverter(Converter[str]):
    @staticmethod
    def ToValue(value: str) -> str:
        return value
