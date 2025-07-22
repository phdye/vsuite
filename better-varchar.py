"""Simple source-to-source translator for basic ``VARCHAR`` operations.

This helper scans a C source file and replaces a few verbose patterns with
equivalent VSuite macros.  The intent is to make code that works with
Oracle-style ``VARCHAR`` structures more compact and readable.  The
transformations performed are strictly textual and operate via regular
expression search/replace, so the input is expected to be valid C code.

The following conversions are currently supported:

``VARCHAR_SETLENZ``
    ``FOO.arr[FOO.len] = '\0';`` becomes ``VARCHAR_SETLENZ(FOO);``.

``v_copy``
    ``strcpy(FOO.arr, BAR.arr);`` followed by
    ``FOO.arr[BAR.len] = '\0';`` becomes ``v_copy(FOO, BAR);``.

``vp_copy``
    ``strcpy(FOO.arr, "literal");`` becomes ``vp_copy(FOO, "literal");``.

``VARCHAR_sprintf``
    ``sprintf(FOO.arr, fmt, ...);`` becomes ``VARCHAR_sprintf(FOO, fmt, ...);``.

Usage:
    better-varchar.py [options] <input-pc-file> <output-pc-file>

Options:
    -h --help     Show this help message and exit.
    --version     Show program version and exit.
"""

import argparse
import re
import io

# Application version string.  Bumped whenever the behaviour or command
# line interface changes so users can query the script via ``--version``.
VERSION = "0.1"


def parse_args(argv=None):
    """Parse command line options.

    ``argv`` is passed for unit testing; ``None`` picks up ``sys.argv``
    automatically.  The parser accepts a single input file and output
    file and exposes ``--version`` so users can confirm the script
    version at runtime.
    """

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


def transform(text):
    """Run all source transformations on ``text``.

    Each helper performs a specialised regex substitution.  The
    transformations are deliberately ordered so that more specific
    patterns (such as the ``v_copy`` form) run before the more generic
    ``vp_copy`` or ``v_sprintf`` replacements.
    """

    text = replace_setlenz(text)
    text = replace_v_copy(text)
    text = replace_vp_copy(text)
    text = replace_v_sprintf(text)
    return text


# Regular expression fragment that loosely matches a C identifier possibly
# followed by member or array access.  It intentionally accepts characters
# such as ``->`` and ``[]`` so that structures like ``foo->bar[0]`` are
# recognised when used with the macros.
_VAR = r"[A-Za-z0-9_.->\[\]]+"


def replace_setlenz(text):
    """Convert NUL termination assignments to ``VARCHAR_SETLENZ``.

    Matches lines of the form ``VAR.arr[VAR.len] = '\0';``.  The variable
    name is captured so we can preserve any indentation and reuse the
    same identifier in the replacement macro call.
    """

    pattern = re.compile(
        r"(?P<indent>^\s*)(?P<var>" + _VAR +
        r")\.arr\[(?P=var)\.len\]\s*=\s*'\0';",
        re.MULTILINE,
    )
    return pattern.sub(
        lambda m: "%sVARCHAR_SETLENZ(%s);" % (m.group('indent'), m.group('var')),
        text,
    )


def replace_v_copy(text):
    """Collapse ``strcpy`` + length assignment into ``v_copy``.

    This looks for a call to ``strcpy`` where both arguments are the
    ``.arr`` members of ``VARCHAR`` variables.  The function also
    expects the caller to manually NUL terminate the destination using
    ``dst.arr[src.len] = '\0';``.  When such a pattern is found it is
    replaced with the single ``v_copy(dst, src);`` macro call.
    """

    pattern = re.compile(
        r"(?P<indent>^[ \t]*)strcpy\(\s*(?:\(char\*\))?\s*(?P<foo>" + _VAR +
        r")\.arr,\s*(?:\(char\*\))?\s*(?P<bar>" + _VAR +
        r")\.arr\s*\);\s*(?:\n\s*)?(?P=foo)\.arr\[(?P=bar)\.len\]\s*=\s*'\0';",
        re.MULTILINE,
    )
    return pattern.sub(
        lambda m: "%sv_copy(%s, %s);" % (m.group('indent'), m.group('foo'), m.group('bar')),
        text,
    )


def replace_vp_copy(text):
    """Rewrite ``strcpy`` from a string literal into ``vp_copy``.

    This pattern specifically handles copying a quoted literal into a
    ``VARCHAR`` variable.  The literal is preserved verbatim in the
    replacement macro call.
    """

    pattern = re.compile(
        r"(?P<indent>^[ \t]*)strcpy\(\s*(?:\(char\*\))?\s*(?P<foo>" + _VAR +
        r")\.arr,\s*\"(?P<lit>[^\"]*)\"\s*\);",
        re.MULTILINE,
    )
    return pattern.sub(
        lambda m: "%svp_copy(%s, \"%s\");" % (m.group('indent'), m.group('foo'), m.group('lit')),
        text,
    )


def replace_v_sprintf(text):
    """Turn ``sprintf`` calls operating on ``VARCHAR`` buffers into ``VARCHAR_sprintf``.

    The function captures everything after the destination buffer so that
    arbitrary format strings and argument lists are preserved.
    """

    pattern = re.compile(
        r"(?P<indent>^[ \t]*)sprintf\(\s*(?P<foo>" + _VAR +
        r")\.arr,\s*(?P<args>[^;]*?)\)\s*;",
        re.MULTILINE,
    )
    return pattern.sub(
        lambda m: "%sVARCHAR_sprintf(%s, %s);" % (m.group('indent'), m.group('foo'), m.group('args')),
        text,
    )


def main(argv=None):
    """Entry point used by the ``__main__`` block and tests."""

    args = parse_args(argv)
    with io.open(args.input_pc_file, 'r', encoding='utf-8') as fh:
        data = fh.read()
    result = transform(data)
    with io.open(args.output_pc_file, 'w', encoding='utf-8') as fh:
        fh.write(result)


if __name__ == "__main__":
    main()
