#!/usr/bin/env python3
import argparse
import re
from pathlib import Path

import gen_image


KEYWORDS = {
    "if", "for", "while", "switch", "return", "sizeof",
}

FUNC_DEF = re.compile(
    r"(?m)^([A-Za-z_][A-Za-z0-9_\s\*\(\),]*?\s+)([A-Za-z_][A-Za-z0-9_]*)\s*\(([^;{}]*)\)\s*\{"
)


def find_defined_functions(source):
    names = []
    for match in FUNC_DEF.finditer(source):
        name = match.group(2)
        if name not in KEYWORDS:
            names.append(name)
    return sorted(set(names), key=names.index)


def replace_identifiers(source, mapping):
    out = []
    i = 0
    n = len(source)
    while i < n:
        c = source[i]
        if c == '"':
            j = i + 1
            while j < n:
                if source[j] == "\\":
                    j += 2
                elif source[j] == '"':
                    j += 1
                    break
                else:
                    j += 1
            out.append(source[i:j])
            i = j
        elif c == "'":
            j = i + 1
            while j < n:
                if source[j] == "\\":
                    j += 2
                elif source[j] == "'":
                    j += 1
                    break
                else:
                    j += 1
            out.append(source[i:j])
            i = j
        elif source.startswith("//", i):
            j = source.find("\n", i)
            if j < 0:
                j = n
            out.append(source[i:j])
            i = j
        elif source.startswith("/*", i):
            j = source.find("*/", i + 2)
            if j < 0:
                j = n - 2
            j += 2
            out.append(source[i:j])
            i = j
        elif c.isalpha() or c == "_":
            j = i + 1
            while j < n and (source[j].isalnum() or source[j] == "_"):
                j += 1
            ident = source[i:j]
            out.append(mapping.get(ident, ident))
            i = j
        else:
            out.append(c)
            i += 1
    return "".join(out)


def wrapped_source(src, app_name):
    source = Path(src).read_text()
    entry = gen_image.symbol_for(app_name)
    mapping = {}
    for name in find_defined_functions(source):
        if name == "xiao_app_entry":
            mapping[name] = entry
        else:
            mapping[name] = f"{entry}__{name}"
    return replace_identifiers(source, mapping)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--src", required=True)
    parser.add_argument("--app-name", required=True)
    parser.add_argument("--out", required=True)
    args = parser.parse_args()

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(wrapped_source(args.src, args.app_name))


if __name__ == "__main__":
    main()
