##
# @file node_initializer.py
#
# @brief Convenience functions for initializing nodes from an attrs-enabled
# interface class.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 06 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from typing import Optional, Any

import attr

from .initialize_from_attr import (
    InitializeModelFromAttr,
    InitializeInterfaceFromAttr)

from .transform import IsTransformed, GetInstanceVars
from .register_model import GetMemberModel, GetIsModelClass


def InitializeModel(
        model: Any,
        protoType: Any,
        namePrefix: Optional[str] = None) -> None:

    if IsTransformed(type(model)):
        for name in GetInstanceVars(type(model)):
            memberModel = GetMemberModel(protoType, name)

            if memberModel is not None:
                # This member is also a transformed class
                # Use it directly as a nested transformation
                if namePrefix is not None:
                    memberName = "{}.{}".format(namePrefix, name)
                else:
                    memberName = name

                setattr(
                    model,
                    name,
                    memberModel(getattr(protoType, name), memberName))
            else:
                setattr(
                    model,
                    name,
                    model.TransformMember(name, protoType, namePrefix))

    if attr.has(type(model)):
        # Initialize additional attrs members.
        InitializeModelFromAttr(model, namePrefix)


# Convenience functions for user-interface classes created by @Transform
def InitializeUserInterface(userInterface: Any, model: Any) -> None:
    if IsTransformed(type(model)):
        for name in GetInstanceVars(type(userInterface)):
            modelNode = getattr(model, name)

            if GetIsModelClass(type(modelNode)):
                # Model classes are initialized with recursive calls
                setattr(userInterface, name, type(modelNode).__base__())
                InitializeUserInterface(getattr(userInterface, name), modelNode)
            else:
                setattr(userInterface, name, modelNode.GetInterfaceNode())

    if attr.has(type(userInterface)):
        InitializeInterfaceFromAttr(userInterface, model)
