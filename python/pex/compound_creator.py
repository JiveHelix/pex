from __future__ import annotations
from typing import Generic, TypeVar

T = TypeVar('T')

class CompoundCreator(Generic[T]):

    @classmethod
    def Create(
            class_,
            name: str,
            value: T) -> CompoundCreator[T]:

        raise NotImplemented("Create must be overridden")
