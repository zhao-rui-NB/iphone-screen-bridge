#!/usr/bin/env python3
"""Convert a .bin file to a C array for MCU firmware use."""

from __future__ import annotations

import argparse
import os
from pathlib import Path


def format_c_array(data: bytes, name: str, columns: int, use_const: bool, use_static: bool) -> str:
    qualifier = []
    if use_static:
        qualifier.append("static")
    if use_const:
        qualifier.append("const")

    qualifier_str = (" ".join(qualifier) + " ") if qualifier else ""
    header = f"{qualifier_str}uint8_t {name}[{len(data)}] = {{\n"
    lines = []
    for i in range(0, len(data), columns):
        chunk = data[i : i + columns]
        hex_items = ", ".join(f"0x{b:02X}" for b in chunk)
        lines.append(f"    {hex_items}")
    body = ",\n".join(lines)
    footer = "\n};\n"
    return header + body + footer


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Convert a binary file to a C byte array."
    )
    parser.add_argument("input", help="Path to .bin file")
    parser.add_argument(
        "-o",
        "--output",
        help="Output .c file path. Defaults to <input_stem>.c in the same folder.",
    )
    parser.add_argument(
        "-n",
        "--name",
        help="C array name. Defaults to the input file stem.",
    )
    parser.add_argument(
        "-c",
        "--columns",
        type=int,
        default=16,
        help="Number of bytes per line. Default: 16.",
    )
    parser.add_argument(
        "--no-const",
        action="store_true",
        help="Do not add const qualifier.",
    )
    parser.add_argument(
        "--static",
        action="store_true",
        help="Add static qualifier.",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    input_path = Path(args.input)
    if not input_path.is_file():
        raise SystemExit(f"Input file not found: {input_path}")

    name = args.name or input_path.stem
    if not name.isidentifier():
        raise SystemExit("Array name must be a valid C identifier.")

    output_path = Path(args.output) if args.output else input_path.with_suffix(".c")
    data = input_path.read_bytes()

    content = format_c_array(
        data=data,
        name=name,
        columns=max(1, args.columns),
        use_const=not args.no_const,
        use_static=args.static,
    )

    output_path.write_text(content, encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
