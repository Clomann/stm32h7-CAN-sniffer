"""
Generate a Python layout module from app/services/logging/CanLogBuffer.h.

The generated module (`generated_canlog_layout.py` in the same directory)
provides struct formats, sizes, and constants so log_parser.py stays in sync
with the C definitions.
"""

from __future__ import annotations

import ast
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Tuple


REPO_ROOT = Path(__file__).resolve().parents[2]
HEADER_PATH = REPO_ROOT / "app/services/logging/CanLogBuffer.h"
OUTPUT_PATH = Path(__file__).with_name("generated_canlog_layout.py")

# Minimal type map for the header
BASE_TYPE_FORMAT = {
    "uint8_t": "B",
    "int8_t": "b",
    "uint16_t": "H",
    "int16_t": "h",
    "uint32_t": "I",
    "int32_t": "i",
}


@dataclass
class Field:
    name: str
    type_name: str
    array_len: int | None = None  # None for scalars, 0 for flexible arrays


def _strip_suffixes(value: str) -> str:
    """Remove C integer suffixes (u/U/l/L after a numeric literal) to make expressions Python-evaluable."""
    return re.sub(r"((?:0[xX][0-9a-fA-F]+|\d+))[uUlL]+", r"\1", value)


def _eval_int_expr(expr: str, names: Dict[str, int] | None = None) -> int:
    cleaned = _strip_suffixes(expr)
    tree = ast.parse(cleaned, mode="eval")
    allowed = (ast.Expression, ast.BinOp, ast.UnaryOp, ast.Constant, ast.Add, ast.Sub, ast.Mult, ast.Div, ast.FloorDiv, ast.Mod, ast.Pow, ast.LShift, ast.RShift, ast.BitOr, ast.BitAnd, ast.BitXor, ast.USub, ast.UAdd, ast.Call, ast.Name, ast.Load)
    for node in ast.walk(tree):
        if not isinstance(node, allowed):
            raise ValueError(f"Disallowed expression in macro: {expr}")
        if isinstance(node, ast.Call):
            raise ValueError(f"Function calls not allowed in macro: {expr}")
    names = names or {}
    return int(eval(compile(tree, filename="", mode="eval"), {"__builtins__": {}}, names))


def parse_macros(text: str) -> Dict[str, int]:
    macros: Dict[str, int] = {}
    raw_exprs: Dict[str, str] = {}
    for line in text.splitlines():
        if not line.lstrip().startswith("#define"):
            continue
        if re.match(r"^\s*#define\s+\w+\(", line):
            continue  # Skip function-like macros (no space between name and '(' in C).
        match = re.match(r"^\s*#define\s+(\w+)\s+(.+)$", line)
        if not match:
            continue
        name, raw = match.groups()
        raw = raw.split("//", 1)[0].split("/*", 1)[0].strip()
        if not raw:
            continue
        raw_exprs[name] = raw

    unresolved = dict(raw_exprs)
    progress = True
    while unresolved and progress:
        progress = False
        for name, raw in list(unresolved.items()):
            try:
                macros[name] = _eval_int_expr(raw, macros)
            except Exception:
                continue
            del unresolved[name]
            progress = True
    return macros


def parse_typedef_aliases(text: str) -> Dict[str, str]:
    aliases: Dict[str, str] = {}
    for base, alias in re.findall(r"typedef\s+([A-Za-z_][\w\s\*]+?)\s+(\w+);\s*", text):
        base = base.strip()
        if base.endswith("*"):
            continue
        aliases[alias] = base
    return aliases


def parse_struct(text: str, name: str) -> List[Field]:
    struct_pattern = re.compile(
        r"typedef\s+struct\s*(?:__attribute__\(\(packed\)\))?\s*\{(.*?)\}\s*(?:__attribute__\(\(packed\)\))?\s*(\w+)\s*;",
        flags=re.S,
    )
    matches = {m.group(2): m.group(1) for m in struct_pattern.finditer(text)}
    if name not in matches:
        raise ValueError(f"Struct {name} not found")

    body = matches[name]
    body = re.sub(r"/\*.*?\*/", "", body, flags=re.S)
    body = re.sub(r"//.*", "", body)
    fields: List[Field] = []
    for raw_line in body.split(";"):
        line = raw_line.strip()
        if not line:
            continue
        line = line.split("//", 1)[0].split("/*", 1)[0].strip()
        if not line:
            continue
        # Remove attributes like _Alignas(...)
        line = re.sub(r"_Alignas\([^)]*\)\s*", "", line)
        array_match = re.match(r"(.+?)\s+(\w+)\s*\[(.*?)\]$", line)
        if array_match:
            type_part, name_part, len_part = array_match.groups()
            len_part = len_part.strip()
            if len_part == "":
                array_len = 0  # flexible
            else:
                try:
                    array_len = _eval_int_expr(len_part)
                except Exception:
                    continue
            fields.append(Field(name=name_part, type_name=type_part.strip(), array_len=array_len))
            continue

        simple_match = re.match(r"(.+?)\s+(\w+)$", line)
        if simple_match:
            type_part, name_part = simple_match.groups()
            fields.append(Field(name=name_part, type_name=type_part.strip(), array_len=None))
            continue
    return fields


def resolve_type(type_name: str, aliases: Dict[str, str]) -> str:
    # Follow aliases until we hit a base type or unknown
    seen = set()
    t = type_name
    while t in aliases and t not in seen:
        seen.add(t)
        t = aliases[t]
    return t


def build_struct_format(struct_name: str, structs: Dict[str, List[Field]], aliases: Dict[str, str], cache: Dict[str, str]) -> str:
    if struct_name in cache:
        return cache[struct_name]

    fmt_parts: List[str] = []
    for field in structs[struct_name]:
        if field.array_len == 0:
            break  # flexible array; stop here

        resolved = resolve_type(field.type_name, aliases)

        if resolved in BASE_TYPE_FORMAT:
            part = BASE_TYPE_FORMAT[resolved]
        elif resolved in structs:
            part = build_struct_format(resolved, structs, aliases, cache)
        else:
            raise ValueError(f"Unknown type {resolved} in struct {struct_name}")

        if field.array_len and field.array_len > 1:
            part = f"{field.array_len}{part}"

        fmt_parts.append(part)

    fmt = "<" + "".join(fmt_parts)
    cache[struct_name] = fmt
    return fmt


def generate_layout(header_path: Path = HEADER_PATH, output_path: Path = OUTPUT_PATH) -> None:
    text = header_path.read_text()
    macros = parse_macros(text)
    aliases = parse_typedef_aliases(text)

    target_structs = [
        "CanLogEntryHeaderType",
        "CanLogBlockHeaderType",
        "CanLogEntryType",
        "CanLogSyncType",
    ]

    structs: Dict[str, List[Field]] = {name: parse_struct(text, name) for name in target_structs}
    fmt_cache: Dict[str, str] = {}
    formats = {name: build_struct_format(name, structs, aliases, fmt_cache) for name in target_structs}

    entry_fixed_fields = [f for f in structs["CanLogEntryType"] if f.name not in {"header", "data"} and f.array_len != 0]
    sync_fixed_fields = [f for f in structs["CanLogSyncType"] if f.name != "header"]

    output = []
    output.append('"""Auto-generated from app/services/logging/CanLogBuffer.h. Do not edit manually."""')
    output.append("import struct")
    output.append("")

    def emit_constants():
        keys = [
            "CANLOG_VERSION",
            "CANLOG_ENTRY_MAX_DATA_LENGTH",
            "BLOCK_SIZE",
            "LOG_BUFFER_SIZE",
            "CAN_DLC_MASK",
            "CAN_FLAG_IDE",
            "CAN_FLAG_RTR_FDF",
            "CAN_FLAG_BRS",
            "CAN_FLAG_ESI",
            "CLB_ENTRY_TYPE_NONE",
            "CLB_ENTRY_TYPE_FRAME",
            "CLB_ENTRY_TYPE_SYNC",
            "CLB_ENTRY_TYPE_MARKER",
            "CANLOG_UNDEFINED_TYPE",
            "CANLOG_CLASSIC_TYPE",
            "CANLOG_FD_TYPE",
            "CANLOG_MARKER_TYPE",
            "CANLOG_CUSTOM_TYPE",
        ]
        for key in keys:
            if key in macros:
                output.append(f"{key} = {macros[key]}")
        output.append("")

    def emit_struct(name: str, label: str) -> None:
        fmt = formats[name]
        output.append(f"{label}_FORMAT = \"{fmt}\"")
        output.append(f"{label}_STRUCT = struct.Struct({label}_FORMAT)")
        output.append(f"{label}_SIZE = {label}_STRUCT.size")
        output.append("")

    emit_constants()
    emit_struct("CanLogBlockHeaderType", "BLOCK_HEADER")
    emit_struct("CanLogEntryHeaderType", "ENTRY_HEADER")

    def emit_field_list(label: str, specs: List[Tuple[str, str, int | None]]):
        output.append(f"{label} = [")
        for name, fmt_char, array_len in specs:
            array_len_repr = "None" if array_len is None else array_len
            output.append(f'    ("{name}", "{fmt_char}", {array_len_repr}),')
        output.append("]")
        output.append("")

    def emit_struct_format(label: str, specs: List[Tuple[str, str, int | None]]) -> None:
        fmt_literal = "<" + "".join((fmt if array_len in (None, 1) else f"{array_len}{fmt}") for _, fmt, array_len in specs)
        output.append(f'{label}_FORMAT = "{fmt_literal}"')
        output.append(f"{label}_STRUCT = struct.Struct({label}_FORMAT)")
        output.append(f"{label}_SIZE = {label}_STRUCT.size")
        output.append("")

    def build_specs(fields: List[Field]) -> List[Tuple[str, str, int | None]]:
        specs: List[Tuple[str, str, int | None]] = []
        for f in fields:
            resolved = resolve_type(f.type_name, aliases)
            if resolved in BASE_TYPE_FORMAT:
                fmt_char = BASE_TYPE_FORMAT[resolved]
            elif resolved in formats:
                fmt_char = formats[resolved][1:]  # drop endianness
            else:
                raise ValueError(f"Unknown type {resolved}")
            specs.append((f.name, fmt_char, f.array_len))
        return specs

    entry_specs = build_specs(entry_fixed_fields)
    sync_specs = build_specs(sync_fixed_fields)

    emit_field_list("ENTRY_FIXED_FIELDS", entry_specs)
    emit_struct_format("ENTRY_FIXED", entry_specs)

    emit_field_list("SYNC_FIXED_FIELDS", sync_specs)
    emit_struct_format("SYNC_FIXED", sync_specs)
    output.append("")

    output_path.write_text("\n".join(output))


def needs_regeneration(
    header_path: Path = HEADER_PATH,
    output_path: Path = OUTPUT_PATH,
    generator_path: Path | None = None,
) -> bool:
    if not output_path.exists():
        return True
    if generator_path is None:
        generator_path = Path(__file__)
    output_mtime = output_path.stat().st_mtime
    return output_mtime < header_path.stat().st_mtime or output_mtime < generator_path.stat().st_mtime


if __name__ == "__main__":
    generate_layout()
