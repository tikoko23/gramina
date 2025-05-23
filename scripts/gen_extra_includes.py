#!/usr/bin/env python3

import re
import os
from os import path
from typing import Tuple, List

from regexes import *

INCLUDE_DIR: str = "include/"
GENERATED_SUBDIR: str = "gen/"

def make_header_guard(filename: str) -> str:
    return "__" + filename.replace("/", "_").replace(".", "_").upper()

def handle_fn_match(m: re.Match) -> str:
    name: str = m.group(1)
    converted: str = name.removeprefix("gramina_")
    return f"#define {converted} {name}"

def handle_type_match(m: re.Match) -> Tuple[str, str]:
    tag, typ = m.group(1, 2)

    tagless_typ: str = typ.replace("_", " ").title().replace(" ", "")
    nn_typ: str = tagless_typ.removeprefix("Gramina")

    tagless: str = f"typedef {tag} {typ} {tagless_typ};"
    nn: str = f"typedef {tag} {typ} {nn_typ};"

    return nn, tagless

def process_header(filename: str) -> None:
    bare_name: str = filename.removeprefix(INCLUDE_DIR)
    gen_file = path.join(path.join(INCLUDE_DIR, GENERATED_SUBDIR), bare_name)

    no_namespace_lines: List[str] = []
    want_tagless_lines: List[str] = []

    with open(filename, "r+") as header:
        lines = header.readlines()
        includes_gen = False
        for line in lines:
            if "/* ignore */" in line:
                continue

            if f'#include "{path.join(GENERATED_SUBDIR, bare_name)}"' in line:
                includes_gen = True

            if "gen_ignore: true" in line:
                return

            symname_match = re.search(r'symname: "(.*)"', line)
            if symname_match is not None:
                symname = symname_match.group(1)
                to = symname.removeprefix("gramina_")
                fn_def = f"#define {to} {symname}"
                no_namespace_lines.append(fn_def)

                continue

            fn_match = re.match(FUNCTION_REGEX, line)
            type_match = re.match(TYPE_REGEX, line)

            if fn_match is not None:
                fn_def = handle_fn_match(fn_match)
                no_namespace_lines.append(fn_def)

            if type_match is not None:
                nn, tgl = handle_type_match(type_match)
                no_namespace_lines.append(nn)
                want_tagless_lines.append(tgl)

        if not includes_gen:
            header.seek(0, 2)
            header.write(f"#include \"{path.join(GENERATED_SUBDIR, bare_name)}\"")

    gen_dirname: str = path.dirname(gen_file)
    os.makedirs(gen_dirname, exist_ok=True)

    nn_h_guard = make_header_guard(f"{bare_name}_NN")
    tg_h_guard = make_header_guard(f"{bare_name}_TG")

    no_namespace_defs = "\n".join(sorted(no_namespace_lines))
    tagless_defs = "\n".join(sorted(want_tagless_lines))

    with open(gen_file, "w") as generated:
        generated.write(f"""
#if defined(GRAMINA_NO_NAMESPACE) && !defined({nn_h_guard})
#define {nn_h_guard}

{no_namespace_defs}

#endif
#if defined(GRAMINA_WANT_TAGLESS) && !defined({tg_h_guard})
#define {tg_h_guard}

{tagless_defs}

#endif
""".strip() + "\n")

def find_headers(dirname: str) -> List[str]:
    if dirname.removesuffix("/").endswith("gen"):
        return []

    headers: List[str] = []

    for entry in os.listdir(dirname):
        full_path = path.join(dirname, entry)
        if path.isdir(full_path):
            subheaders = find_headers(full_path)
            headers.extend(subheaders)
        elif path.isfile(full_path) and full_path.endswith(".h"):
            headers.append(full_path)

    return headers

def main() -> None:
    os.system(f"rm -rf include_backup")
    os.system(f"rm -rf {path.join(INCLUDE_DIR, GENERATED_SUBDIR)}")
    os.system(f"cp -r include include_backup/")

    headers = find_headers(INCLUDE_DIR)
    for h in headers:
        process_header(h)

if os.getcwd().removesuffix("/").endswith("build"):
    os.chdir("..")

main()
