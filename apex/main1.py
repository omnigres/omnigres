import dataclasses
import textwrap
import typing
from struct import Struct

from clang import cindex
from clang.cindex import Index, CursorKind, TypeKind
import clang.cindex

import shutil
import subprocess
import os

import click
import rich

from typing import Protocol


def resolve_underlying_type(clang_type):
    """Follow typedef chains and get the canonical underlying type"""
    current = clang_type

    # Follow typedef chain
    while current.kind == TypeKind.TYPEDEF:
        current = current.get_canonical()

    return current


def extract_type_info(type):
    """Extract linearizable info from field types"""

    match type.kind:
        case TypeKind.TYPEDEF:
            return TypeAlias(name=type.spelling, type=extract_type_info(resolve_underlying_type(type)))

        case TypeKind.POINTER:
            pointee = type.get_pointee()

            # Check if it's a function pointer
            if pointee.kind == TypeKind.FUNCTIONPROTO:
                return analyze_function_pointer(pointee)
            else:
                resolved_pointee = extract_type_info(pointee)
                return Pointer(cursor=type, type=type, pointee=resolved_pointee)

        case TypeKind.RECORD:
            return Struct(cursor=None, name=type.spelling,
                          type=type,
                          size=type.get_size(),
                          alignment=type.get_align())
        case TypeKind.ELABORATED:
            return extract_type_info(type.get_named_type())
        case _:
            return PrimitiveType(type=type)


def analyze_function_pointer(func_type):
    """Extract linearizable info from function pointer"""

    # Get return type
    return_type = func_type.get_result()
    resolved_return = resolve_underlying_type(return_type)

    # Get parameter types
    args = func_type.argument_types()
    params = []
    for param_type in args:
        resolved_param = extract_type_info(param_type)
        params.append(resolved_param)

    return FunctionPointer(type=func_type, return_type=extract_type_info(resolved_return), param_types=params)


class DeterministicSignature(Protocol):
    def signature(self) -> str:
        raise NotImplementedError


class Type(Protocol):
    def type_name(self) -> str:
        raise NotImplementedError

    def related_types(self) -> typing.List[typing.Self]:
        return []

    def all_related_types(self) -> typing.List[typing.Self]:
        types = []
        for related_type in self.related_types():
            types.append(related_type)
            types.extend(related_type.all_related_types())
        return types


class DeterministicallySignedType(Type, DeterministicSignature):
    pass


@dataclasses.dataclass
class TypeAlias(Type, DeterministicSignature):
    name: str
    type: DeterministicallySignedType

    def type_name(self) -> str:
        return self.name

    def related_types(self) -> typing.List[Type]:
        return self.type.related_types()

    def signature(self) -> str:
        return f"{self.name}: {self.type.signature()}"


@dataclasses.dataclass
class PrimitiveType(Type, DeterministicSignature):
    type: clang.cindex.Type

    def signature(self) -> str:
        return type.spelling

    def type_name(self) -> str:
        return type.spelling


@dataclasses.dataclass
class Pointer(Type, DeterministicSignature):
    cursor: cindex.Cursor
    type: clang.cindex.Type
    pointee: Type

    def type_name(self) -> str:
        return f"{self.pointee.type_name()} *"

    def signature(self) -> str:
        return f"{self.pointee.type_name()} *"

    def related_types(self) -> typing.List[Type]:
        return [self.pointee]


@dataclasses.dataclass
class FunctionPointer(Type, DeterministicSignature):
    type: clang.cindex.Type
    return_type: Type
    param_types: list[Type]

    def type_name(self) -> str:
        return f"{self.type.spelling} *"

    def signature(self) -> str:
        return f"{{{self.return_type.type_name()}}}"

    def related_types(self) -> typing.List[Type]:
        types = self.param_types.copy()
        types.append(self.return_type)
        return types

@dataclasses.dataclass
class StructMember(DeterministicSignature):
    cursor: cindex.Cursor
    type: clang.cindex.Type
    name: str
    offset: int
    size: int

    def signature(self) -> str:
        return f"""- {self.name}:
    type: {self.type.spelling}
    size: {self.size}
    offset: {self.offset}
"""


@dataclasses.dataclass
class Struct(Type, DeterministicSignature):
    cursor: cindex.Cursor | None
    type: clang.cindex.Type
    name: str
    size: int
    alignment: int

    def type_name(self) -> str:
        return self.type.spelling

    def related_types(self) -> typing.List[Type]:
        rich.print([extract_type_info(member.type) for member in self.members()])
        return [extract_type_info(member.type) for member in self.members()]

    def members(self):
        fields: list[StructMember] = []
        rich.print(self)
        for field in self.cursor.get_children():
            if field.kind == CursorKind.FIELD_DECL:
                field_type = field.type
                offset = field.get_field_offsetof() // 8
                fields.append(StructMember(cursor=field, type=field_type, name=field.spelling, offset=offset,
                                           size=field_type.get_size()))
        return fields

    def signature(self) -> str:
        members = self.members()
        empty = members == []
        trailing = " []" if empty else ":"
        return textwrap.dedent(f"""
{self.name}{trailing}
{''.join([str(member.signature()) for member in self.members()])}""".strip())

def analyze_file(filename, api_type, args, target_triple="x86_64-unknown-linux-gnu"):
    # Create index and parse with target
    index = Index.create()

    # Parse with target specification
    fargs = [f'--target={target_triple}']
    fargs.extend(args or [])
    tu = index.parse(filename, args=fargs)

    if not tu:
        print("Failed to parse file")
        return

    resolved_types: dict[str, DeterministicallySignedType] = {}


    def collect_complete_types(cursor):
        if cursor.kind in [CursorKind.STRUCT_DECL, CursorKind.CLASS_DECL, CursorKind.TYPEDEF_DECL]:
            # cursor.type is the semantically resolved type
            resolved_type = cursor.type

            # Check if type is complete (not just forward declared)
            if resolved_type.get_size() > 0:
                resolved_types[cursor.spelling] = Struct(cursor=cursor, name=cursor.spelling,
                                                         type=resolved_type,
                                                         size=resolved_type.get_size(),
                                                         alignment=resolved_type.get_align())

        for child in cursor.get_children():
            collect_complete_types(child)

    # First pass: collect all complete types
    collect_complete_types(tu.cursor)

    type = resolved_types.get(api_type, None)
    types: list[DeterministicallySignedType] = [type]
    types.extend(type.all_related_types())
    rich.print(types)
    for type in sorted(types, key=lambda t: t.type_name()):
        print(type.signature())

@click.argument("api_type", type=str, required=True)
@click.argument("header_file", type=click.Path(exists=True, dir_okay=False))
@click.option("--pg_config", type=click.Path(exists=True, dir_okay=False), required=False)
@click.command()
def main(header_file, api_type, pg_config):
    if pg_config is None:
        pg_config = shutil.which("pg_config")
    pg_include_server = str(
        subprocess.run([pg_config, "--includedir-server"], capture_output=True, text=True).stdout).rstrip('\n')
    analyze_file(header_file, api_type,
                 args=[os.path.dirname(os.path.abspath(__file__)), "-I", pg_include_server, "-I", header_file])


if __name__ == "__main__":
    main()
