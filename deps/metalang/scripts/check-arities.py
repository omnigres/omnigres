#!/usr/bin/env python3

# Make sure that arity specifiers of public metafunctions are consistent with their signatures.

import os
import re
import xml.etree.ElementTree as ET
import subprocess

subprocess.call("doxygen > /dev/null 2> /dev/null", shell=True)


def check_module(module):
    print(f"Checking {module}.h ...")

    tree = ET.parse(f"xml/{module}_8h.xml")
    root = tree.getroot()

    arity_specifiers = gather_arity_specifiers(root)

    for metafunction_name, arity in gather_metafunctions(root).items():
        expected_arity = int(arity)
        actual_arity = int(arity_specifiers[metafunction_name])
        assert expected_arity == actual_arity


def gather_metafunctions(root):
    metafunctions = {}

    for definition in root.findall("./compounddef/sectiondef/memberdef"):
        macro_name = definition.find("name").text
        is_metalang99_compliant = re.search("ML99_[a-z]", macro_name)

        exceptions = {
            "ML99_call", "ML99_callUneval", "ML99_fatal", "ML99_abort", "ML99_tupleGet", "ML99_variadicsGet", "ML99_seqGet"}
        is_exceptional = macro_name in exceptions

        if (is_metalang99_compliant and not is_exceptional):
            arity = len(list(definition.findall("param")))
            metafunctions[macro_name] = arity

    return metafunctions


def gather_arity_specifiers(root):
    arity_specifiers = {}

    for definition in root.findall("./compounddef/programlisting/codeline/highlight[@class='preprocessor']"):
        m = re.match(r"#define(\w+)_ARITY(\d)", "".join(definition.itertext()))
        if m is not None:
            metafunction_name = m.groups()[0]
            arity = m.groups()[1]
            arity_specifiers[metafunction_name] = arity

    return arity_specifiers


for path in os.listdir("include/metalang99"):
    if path.endswith(".h"):
        module = path.replace(".h", "")
        check_module(module)
