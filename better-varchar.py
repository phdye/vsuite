"""Usage: better-varchar.py [options] <input-pc-file> <output-pc-file>

Replace basic VARCHAR actions with their macro equivalents.

Options:
  -h --help     Show this help message and exit.
  --version     Show program version and exit.
"""

from __future__ import annotations

import argparse
import re
from pathlib import Path

VERSION = "0.1"


def parse_args(argv=None):
    parser = argparse.ArgumentParser(
        prog="better-varchar.py",
        usage="better-varchar.py [options] <input-pc-file> <output-pc-file>",
        description="Replace basic VARCHAR actions with macros.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("input_pc_file")
    parser.add_argument("output_pc_file")
    parser.add_argument("--version", action="version", version=VERSION)
    return parser.parse_args(argv)


def transform(text: str) -> str:
    text = replace_setlenz(text)
    text = replace_v_copy(text)
    text = replace_vp_copy(text)
    text = replace_v_sprintf(text)
    return text


_VAR = r"[A-Za-z0-9_.->\[\]]+"


def replace_setlenz(text: str) -> str:
    pattern = re.compile(
        rf"(?P<indent>^\s*)(?P<var>{_VAR})\.arr\[(?P=var)\.len\]\s*=\s*'\\0';",
        re.MULTILINE,
    )
    return pattern.sub(lambda m: f"{m.group('indent')}VARCHAR_SETLENZ({m.group('var')});", text)


def replace_v_copy(text: str) -> str:
    pattern = re.compile(
        rf"(?P<indent>^[ \t]*)strcpy\(\s*(?:\(char\*\))?\s*(?P<foo>{_VAR})\.arr,\s*(?:\(char\*\))?\s*(?P<bar>{_VAR})\.arr\s*\);\s*(?:\n\s*)?(?P=foo)\.arr\[(?P=bar)\.len\]\s*=\s*'\\0';",
        re.MULTILINE,
    )
    return pattern.sub(lambda m: f"{m.group('indent')}v_copy({m.group('foo')}, {m.group('bar')});", text)


def replace_vp_copy(text: str) -> str:
    pattern = re.compile(
        rf"(?P<indent>^[ \t]*)strcpy\(\s*(?:\(char\*\))?\s*(?P<foo>{_VAR})\.arr,\s*\"(?P<lit>[^\"]*)\"\s*\);",
        re.MULTILINE,
    )
    return pattern.sub(lambda m: f"{m.group('indent')}vp_copy({m.group('foo')}, \"{m.group('lit')}\");", text)


def replace_v_sprintf(text: str) -> str:
    pattern = re.compile(
        rf"(?P<indent>^[ \t]*)sprintf\(\s*(?P<foo>{_VAR})\.arr,\s*(?P<args>[^;]*?)\)\s*;",
        re.MULTILINE,
    )
    return pattern.sub(lambda m: f"{m.group('indent')}v_sprintf({m.group('foo')}, {m.group('args')});", text)


def main(argv=None):
    args = parse_args(argv)
    data = Path(args.input_pc_file).read_text()
    result = transform(data)
    Path(args.output_pc_file).write_text(result)


if __name__ == "__main__":
    main()
