##
# @file model.py
#
# @brief Models used in test_interface.py.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

import attr
from tests.interface import Interface
from pex import ModelNodeInitializer, InterfaceNodeInitializer


@attr.s(init=False, eq=False)
class TestModel(Interface):
    """All of the model nodes are instantiated here."""

    def __init__(self) -> None:
        ModelNodeInitializer(self, Interface)


@attr.s(init=False, eq=False)
class TestUser(Interface):
    """Uses model to create all of the interface nodes."""

    def __init__(self, model: TestModel) -> None:
        InterfaceNodeInitializer(self, Interface, model)
