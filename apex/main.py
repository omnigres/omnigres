from io import StringIO
import typing

import os.path
import shutil
import subprocess

import re
import json
import sys

import rich
import click

CLANG_BUILTIN_TYPES = set([
    # Character types
    "char",
    "signed char",
    "unsigned char",

    # Integer types
    "short",
    "unsigned short",
    "int",
    "unsigned int",
    "long",
    "unsigned long",
    "long long",
    "unsigned long long",

    # Extended integer types
    "__int128",
    "unsigned __int128",

    # Floating point types
    "float",
    "double",
    "long double",
    "__float128",

    # Special types
    "void",
    "_Bool",

    # Wide character types
    "wchar_t",
    "char16_t",
    "char32_t",

    # Vector types (compiler builtins)
    "__builtin_va_list",

    # Null pointer type
    "typeof(nullptr)",  # C++11, but Clang extension for C
])

def parse_clang_layout(abi):
    """Parses Clang's -fdump-record-layouts output into a JSON structure."""
    structs = {}
    current_struct = None
    field_pattern = re.compile(r"^\s*(\d+)\s*\|\s\s\s([^\s].+)")
    size_pattern = re.compile(r"^\s*\|?\s*\[\s*sizeof=(\d+),(?:\s*dsize=\d+,)?\s*align=(\d+)\s*")

    previous_line_was_marker = False

    for line in abi.splitlines():
        line = line.strip()
        if line.startswith("*** Dumping AST Record Layout"):
            previous_line_was_marker = True
            continue  # Skip to next line

        # Match struct declaration
        struct_match = re.match(r"^\s*\d+\s*\|\s*struct (.+)", line)
        if struct_match and previous_line_was_marker:
            # Cleanup
            if current_struct is not None and current_struct.startswith("(unnamed"):
                del structs[current_struct]
            current_struct = struct_match.group(1)
            structs[current_struct] = {"fields": [], "size": 0, "align": None}
            previous_line_was_marker = False  # Reset marker flag
            continue

        union_match = re.match(r"^\s*\d+\s*\|\s*union (.+)", line)
        if union_match and previous_line_was_marker:
            # Cleanup
            if current_struct is not None and current_struct.startswith("(unnamed"):
                del structs[current_struct]
            current_struct = None
            continue

        # Match field declaration
        if current_struct:
            field_match = field_pattern.match(line)
            if field_match:
                offset = int(field_match.group(1))
                field_info = field_match.group(2).strip().rpartition(" ")[-1]
                structs[current_struct]["fields"].append({"offset": offset, "type": field_info})

            # Match struct size and alignment
            size_match = size_pattern.match(line)
            if size_match:
                structs[current_struct]["size"] = int(size_match.group(1))
                structs[current_struct]["align"] = int(size_match.group(2))

    return structs

class APIStructMember:

    def __init__(self, name, type):
        self.name = name
        self.type = type

    def __repr__(self):
        return "({!r}, {!r})".format(self.name, self.type)

class APIStruct:
    members : list[APIStructMember] = []
    def __init__(self, name):
        self.name = name

    def __repr__(self):
        return "struct {!r}:\n{!r}".format(self.name, self.members)


class API:
    structs: list[APIStruct] = []

def process_type(ast, type, api: API):
    match type['kind']:
        case "TypedefDecl":
            pass

    rich.print(type)

def process_struct(ast, struct, api: API):
    api_struct = APIStruct(name=struct['name'])
    for field in struct['inner']:
        if field['kind'] == "FieldDecl":
            if 'typeAliasDeclId' in field['type']:
                type = next(
                    iter([type for type in ast['inner'] if type['id'] == field['type'].get('typeAliasDeclId', None)]),
                    None)
                if type is None:
                    raise RuntimeError(f"`{struct['name']}.{field['name']}` has no defined type alias declaration")
                api_struct.members.append(APIStructMember(field['name'], type['name']))
                process_type(ast, type, api)
            elif field['type'].get('qualType', None) in CLANG_BUILTIN_TYPES:
                api_struct.members.append(APIStructMember(field['name'], field['type']['qualType']))
            else:
                ref_struct = find_record_decl(ast, field['type']['qualType'])
                if ref_struct is None:
                    raise RuntimeError(f"Unknown type {field['type']['qualType']}")
                else:
                    process_struct(ast, ref_struct, api)
    api.structs.append(api_struct)

def find_record_decl(ast, name):
    return next(
        iter([item for item in ast["inner"] if item['kind'] == "RecordDecl" and item.get('name', None) == name]), None)


@click.command()
@click.argument("header_file", type=click.Path(exists=True, dir_okay=False))
@click.argument("api_type", type=str, required=True)
@click.option("--pg_config", type=click.Path(exists=True, dir_okay=False), required=False)
def main(header_file, api_type, pg_config=None):
    if pg_config is None:
        pg_config = shutil.which("pg_config")
    pg_include_server = str(
        subprocess.run([pg_config, "--includedir-server"], capture_output=True, text=True).stdout).rstrip('\n')
    clang = shutil.which("clang")
    result = subprocess.run(
        [clang, "-Xclang", "-fdump-record-layouts-complete", "-target", "x86_64-unknown-linux-gnu",
         "-I", os.path.dirname(os.path.abspath(__file__)), "-I",
         pg_include_server, header_file], capture_output=True, text=True)
    abi = str(result.stdout)
    print(str(result.stderr))
    parsed_layout = parse_clang_layout(abi)[api_type]
    rich.print(parsed_layout)
    result = subprocess.run(
        [clang, "-fsyntax-only", "-Xclang", "-ast-dump=json", "-I", os.path.dirname(os.path.abspath(__file__)), "-I",
         pg_include_server, header_file], capture_output=True, text=True)
    ast = json.loads(str(result.stdout))
    # Let's start
    root_struct = find_record_decl(ast, api_type)
    api = API()
    process_struct(ast, root_struct, api)
    rich.inspect(api)


if __name__ == "__main__":
    main()
