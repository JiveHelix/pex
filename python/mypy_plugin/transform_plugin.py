##
# @file transform_plugin.py
#
# @brief A custom plugin to type check transformed classes.
#
# @author Jive Helix (jivehelix@gmail.com)
# @date 11 Jun 2020
# @copyright Jive Helix
# Licensed under the MIT license. See LICENSE file.

from __future__ import annotations

import copy
from typing import DefaultDict
from collections import defaultdict

import mypy.plugin
import mypy.types

from mypy.nodes import (
    Argument,
    Var,
    ARG_POS,
    ARG_OPT,
    MemberExpr,
    TypeInfo,
    FuncDef,
    Decorator,
    CallExpr)

from mypy.plugins.common import add_method_to_class


transform_makers = {"Transform", "TransformModel", "TransformInterface"}


typeInfoByPrototypeNameBySeries: DefaultDict[str, Dict[str, TypeInfo]] = \
    defaultdict(dict)


class LookupMemberError(RuntimeError):
    pass


def lookup_member_expr(
        context: mypy.plugin.ClassDefContext,
        node: MemberExpr) -> TypeInfo:

    node_name = node.name
    nodeType = context.api.lookup_fully_qualified_or_none(node.expr.fullname)

    if nodeType.node.name == node_name:
        # This is the type we were looking for
        return nodeType.node

    if node_name not in nodeType.node.names:
        raise LookupMemberError(
            "Unable to find node name: {}".format(node_name))

    node_member = nodeType.node.names[node_name].node

    if isinstance(node_member, TypeInfo):
        return node_member

    if not isinstance(node_member, Decorator):
        # We are looking for a decorated classmethod
        raise LookupMemberError("Expected decorator")

    if not node_member.var.is_classmethod:
        raise LookupMemberError("Expected classmethod")

    # Return the type of the class
    return node_member.var.info


def transform_class_maker_callback(
        context: mypy.plugin.ClassDefContext) -> None:
    proto_type = context.reason.args[0]
    attribute_type = context.reason.args[1]

    if context.reason.callee.name == "TransformModel":
        series = "model"
    elif context.reason.callee.name == "TransformInterface":
        series = "interface"
    else:
        try:
            seriesIndex = context.reason.arg_names.index("series")
        except ValueError:
            # No series specified.
            # Default to model.
            series = "model"
        else:
            series = context.reason.args[seriesIndex].value

    try:
        initIndex = context.reason.arg_names.index("init")
    except ValueError:
        # init defaults to False
        init = False
    else:
        init = ('True' == context.reason.args[initIndex].name)

    proto_type_node = proto_type.node

    if isinstance(attribute_type, CallExpr):
        functor = attribute_type.callee.node
        functorCall = functor.names['__call__']
        attribute_type_node = functorCall.node.type.ret_type.type
    else:
        attribute_type_node = attribute_type.node

    # Multiple passes may be required to resolve all type information.
    # We must def until all nodes are defined.
    if not context.api.final_iteration:
        if proto_type_node is None or attribute_type_node is None:
            context.api.defer()
            return

    if proto_type_node is None:
        if not isinstance(proto_type, MemberExpr):
            print("No way to lookup prototype node.")
            return

        # proto_type may be named in another module
        proto_type_node = lookup_member_expr(context, proto_type)

    if attribute_type_node is None:
        if not isinstance(attribute_type, MemberExpr):
            print("No way to lookup attribute_type node.")
            return

        try:
            # attribute_type may be named in another module
            attribute_type_node = lookup_member_expr(context, attribute_type)
        except LookupMemberError as error:
            print(
                'Unable to find member "{}" in "{}"'.format(
                    attribute_type.name,
                    attribute_type.expr.fullname))
            print(
                'Transform is currently unable to look up inherited members')

            return

    if isinstance(attribute_type_node, FuncDef):
        # This is a free standing function
        # Get the attribute type from the return value
        ret_type = attribute_type_node.type.ret_type

        if isinstance(ret_type, mypy.types.Instance):
            attribute_type_node = attribute_type_node.type.ret_type.type
        elif isinstance(ret_type, mypy.types.UnboundType):
            if not context.api.final_iteration:
                # wait until the type has been fully analyzed.
                context.api.defer()
                return

            # How is the ret_type still UnboundType?
            # Is this a bug?
            print(
                "Unable to find return type for {}. "
                "Prefer a class or a classmethod".format(
                    attribute_type_node.name))
            return

    if not isinstance(attribute_type_node, TypeInfo):
        # Unable to determine the attribute type.
        print("Unable to determine the attribute type")
        return

    context.cls.info = transform_type_info(
        context,
        proto_type_node,
        attribute_type_node,
        init,
        series)

    typeInfoByPrototypeNameBySeries[series][proto_type_node.fullname] = \
        context.cls.info


def module_name_matches(searchParts: List[str], moduleParts: List[str]) -> bool:

    if len(moduleParts) <= len(searchParts):
        return False

    return moduleParts[-len(searchParts):] == searchParts


def find_module(
        api: mypy.semanal.SemanticAnalyzer,
        moduleName: str) -> Optional[mypy.nodes.MypyFile]:

    splitSearchName = moduleName.split('.')

    if moduleName in api.modules:
        return api.modules[moduleName]

    # Find the pex.value module

    for name in api.modules:
        splitName = name.split('.')
        if module_name_matches(splitSearchName, name.split('.')):
            return api.modules[name]

    return None


def transform_type_info(
        context: mypy.plugin.ClassDefContext,
        proto_type_node: TypeInfo,
        attribute_type_node: TypeInfo,
        init: bool,
        series: str) -> TypeInfo:

    transformed_info: TypeInfo = context.cls.info

    # Get the list of proto_type class members that are not dunders or private
    # Also, any names that are already defined in the transformed class are
    # allowed to override anything in the prototype class.
    names = [
        name for name in proto_type_node.names
        if not name.startswith('_') and name not in context.cls.info.names]

    transformedNames = []

    tubeModule = find_module(context.api, "pex.tube")

    if tubeModule is None:
        raise RuntimeError("Unable to locate pex.tube.")

    tubeFullname = tubeModule.names['Tube'].fullname

    for name in names:

        proto_node = proto_type_node.names[name] # SymbolTableNode

        if isinstance(proto_node.node, (FuncDef, Decorator)):
            # Ignore methods
            continue

        transformedNames.append(name)
        copied_node = proto_node.copy() # SymbolTableNode
        copied_node.node = copy.copy(proto_node.node) # Var

        copied_node.node._fullname = "{}.{}".format(
            transformed_info.fullname,
            copied_node.node.name)

        copied_node.plugin_generated = True

        try:
            nestedTypeInfo = \
                typeInfoByPrototypeNameBySeries[series].get(
                    proto_node.node.type.type.fullname,
                    None)
        except AttributeError:
            # proto_node.node may not have type and proto_node.node.type may
            # not have type.
            # Without these, this member is not transformable anyway.
            nestedTypeInfo = None

        if nestedTypeInfo is not None:
            # This member's type has been transformed.
            typeArgs = []

            if nestedTypeInfo.is_generic():
                # The nested type has type args
                if (hasattr(proto_node.node.type, "type")
                        and proto_node.node.type.type.is_generic()):
                    # The proto_node is a generic with type args
                    typeArgs = proto_node.node.type.args

            copied_node.node.type = \
                mypy.types.Instance(nestedTypeInfo, typeArgs)

        elif (hasattr(proto_node.type, "type")
                and proto_node.type.type.name == "ModelSignal"):

            # ModelSignal is always transformed to InterfaceSignal
            pexSignalModule = find_module(context.api, "pex.signal")
            interfaceSignal = pexSignalModule.names["InterfaceSignal"]

            copied_node.node.type = \
                mypy.types.Instance(interfaceSignal.node, [])

        else:
            typeArgs = []

            if attribute_type_node.is_generic():
                if not hasattr(proto_node.node.type, "type"):
                    # Check below would have failed: type.type.has_base(tube)
                    # This proto_node is not a pex.tube.Tube
                    typeArgs = [proto_node.node.type]
                elif (attribute_type_node.has_base(tubeFullname)
                        and proto_node.node.type.type.has_base(tubeFullname)):
                    # When transforming pex types, use the type args to the pex
                    # type rather than the pex type itself.
                    typeArgs = proto_node.node.type.args
                else:
                    typeArgs = [proto_node.node.type]
            else:
                typeArgs = []

            copied_node.node.type = \
                mypy.types.Instance(attribute_type_node, typeArgs)

        transformed_info.names[name] = copied_node

    prototypeInstance = mypy.types.Instance(proto_type_node, [])

    prototypeArgument = Argument(
        Var(
            proto_type_node.name.lower(),
            prototypeInstance),
        prototypeInstance,
        None,
        ARG_POS)

    if init:
        nameType = context.api.named_type('__builtins__.str')

        optionalNamedArgument = Argument(
            Var("name", nameType),
            nameType,
            None,
            ARG_OPT)

        add_method_to_class(
            context.api,
            context.cls,
            "__init__",
            [prototypeArgument, optionalNamedArgument],
            mypy.types.NoneType())

    if series == "model":
        # Only model nodes support GetPrototype
        add_method_to_class(
            context.api,
            context.cls,
            "GetPrototype",
            [],
            prototypeInstance)

        # Find the pex.value module
        pexValueModule = find_module(context.api, "pex.value")

        if pexValueModule is not None:

            multipleValueContext = \
                pexValueModule.names['MultipleValueContext'].node

            multipleValueContextInstance = \
                mypy.types.Instance(multipleValueContext, [])

            optionalContext = Argument(
                Var(
                    'context',
                    multipleValueContextInstance),
                multipleValueContextInstance,
                None,
                ARG_OPT)

            add_method_to_class(
                context.api,
                context.cls,
                "LoadPrototype",
                [prototypeArgument, optionalContext],
                mypy.types.NoneType())

        else:
            print("Warning: Transform plugin unable to locate module pex.value")

    # Now that the class is built, update the info
    for name in transformedNames:
        transformed_info.names[name].node.info = transformed_info

    return transformed_info
