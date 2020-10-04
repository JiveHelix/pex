from typing import (
    Any,
    Callable,
    Iterable,
    Union,
    Tuple,
    List,
    Dict)

import itertools

import wx
import attr
from typing_extensions import Protocol
from .. import pex


keyStringByWxDefine = {
    wx.WXK_DELETE: 'DELETE',
    wx.WXK_BACK: 'BACK',
    wx.WXK_INSERT: 'INSERT',
    wx.WXK_RETURN: 'RETURN',
    wx.WXK_PAGEUP: 'PGUP',
    wx.WXK_PAGEDOWN: 'PGDN',
    wx.WXK_LEFT: 'LEFT',
    wx.WXK_RIGHT: 'RIGHT',
    wx.WXK_UP: 'UP',
    wx.WXK_DOWN: 'DOWN',
    wx.WXK_HOME: 'HOME',
    wx.WXK_END: 'END',
    wx.WXK_SPACE: 'SPACE',
    wx.WXK_TAB: 'TAB',
    wx.WXK_ESCAPE: 'ESCAPE'}


modifierStringByWxAccel = {
    wx.ACCEL_NORMAL: '',
    wx.ACCEL_SHIFT: 'SHIFT',
    wx.ACCEL_CMD: 'CTRL',
    wx.ACCEL_ALT: 'ALT'}


modifierOrder = (wx.ACCEL_CMD, wx.ACCEL_SHIFT, wx.ACCEL_ALT)


class Key:
    asciiCode_: int

    def __init__(self, character: Union[int, str]) -> None:
        if isinstance(character, int):
            self.asciiCode_ = character
        else:
            self.asciiCode_ = ord(character)

    def __repr__(self) -> str:
        return keyStringByWxDefine.get(
            self.asciiCode_,
            chr(self.asciiCode_))

    def __int__(self) -> int:
        return self.asciiCode_


def GetModifierString(modifierBitfield: int) -> str:
    """
    Returns a string describing all modifiers in the bitfield.

    modifierBitfield may be bitwise combination of modifiers. Check the
    modifierBitfield against each accel define to build the description string.
    """
    return '+'.join([
        modifierStringByWxAccel[modifier]
        for modifier in modifierOrder
        if modifierBitfield & modifier])


@attr.s(auto_attribs=True, eq=False, slots=True)
class Shortcut:
    signal: pex.InterfaceSignal
    modifier: int
    ascii: str
    description: str
    longDescription: str

    @property
    def name(self) -> str:
        return self.signal.name


class ShortcutMethods:
    signal_: pex.InterfaceSignal
    id_: int
    modifier_: int
    key_: Key
    description_: str
    longDescription_: str

    def __init__(self, shortcut: Shortcut) -> None:
        self.signal_ = shortcut.signal
        self.id_ = wx.Window.NewControlId()
        self.modifier_ = shortcut.modifier
        self.key_ = Key(shortcut.ascii)
        self.description_ = shortcut.description
        self.longDescription_ = shortcut.longDescription

    def AddToMenu(self, menu: wx.Menu) -> None:
        menu.Append(
            wx.MenuItem(
                menu,
                self.id_,
                self.GetMenuItemLabel(),
                self.longDescription_))

    def GetMenuItemLabel(self) -> str:
        modifier = GetModifierString(self.modifier_)

        if len(modifier) > 0:
            return u'{}\t{}+{}'.format(self.description_, modifier, self.key_)
        else:
            return u'{}\t{}'.format(self.description_, self.key_)

    def GetAcceleratorTableEntry(self) -> Tuple[int, int, int]:
        return (self.modifier_, int(self.key_), self.id_)

    def OnEventMenu(self, ignored: wx.CommandEvent) -> None:
        self.signal_.Trigger()

    def GetId(self) -> int:
        return self.id_


class HasWxShortcutMethods(Protocol):
    acceleratorShortcutMethods_: List[ShortcutMethods]
    menuShortcutMethodsByName_: Dict[str, ShortcutMethods]
    acceleratorTable_: wx.AcceleratorTable

    def SetAcceleratorTable(
            self,
            acceleratorTable_: wx.AcceleratorTable) -> None:
        ...

    def Bind(
            self,
            binder: Any,
            callback: Callable[[wx.CommandEvent], None],
            id: int = wx.ID_ANY) -> None: # pylint: disable=redefined-builtin
        ...


class ShortcutMixin:
    def __init__(
            self: HasWxShortcutMethods,
            acceleratorShortcuts: Iterable[Shortcut],
            menuShortcuts: Iterable[Shortcut]):

        self.acceleratorShortcutMethods_ = [
            ShortcutMethods(shortcut) for shortcut in acceleratorShortcuts]

        self.menuShortcutMethodsByName_ = {
            shortcut.name: ShortcutMethods(shortcut)
            for shortcut in menuShortcuts}

        self.acceleratorTable_ = wx.AcceleratorTable([
            shortcutMethods.GetAcceleratorTableEntry()
            for shortcutMethods in self.acceleratorShortcutMethods_])

        self.SetAcceleratorTable(self.acceleratorTable_)

        for shortcut in itertools.chain(
                self.acceleratorShortcutMethods_,
                self.menuShortcutMethodsByName_.values()):

            self.Bind(
                wx.EVT_MENU,
                shortcut.OnEventMenu,
                id=shortcut.GetId())

    def AddShortcutToMenu(self, shortcutName: str, menu: wx.Menu) -> None:
        self.menuShortcutMethodsByName_[shortcutName].AddToMenu(menu)
