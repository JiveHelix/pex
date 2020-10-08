from typing import (
    Any,
    Callable,
    Optional,
    TYPE_CHECKING)


from .value import ModelValueBase
from .reference import Reference, GetReference


class Group:
    """
    Create a signal callback whenever any of the specified model values change.

    """

    # Weakly reference the notify callback to avoid a cyclic reference.
    if TYPE_CHECKING:
        notifyCallback_: Reference[Callable[[], None]]
    else:
        notifyCallback_: Reference

    def __init__(
            self,
            notifyCallback: Callable[[], None],
            *modelNodes: ModelValueBase[Any]) -> None:

        self.notifyCallback_ = GetReference(notifyCallback)

        for node in modelNodes:
            node.Connect(self.Notify_)


    def Notify_(self, ignored: Optional[Any] = None) -> None:
        notifyCallback = self.notifyCallback_()
        if notifyCallback is not None:
            notifyCallback()
