#!/usr/bin/env python3

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
    -h --help          Show this help message and exit.
    --version          Show program version and exit.
    --lines START:END  Only transform a specific line range (1-indexed).
    --fraction A:B     Transform only a fraction of the file.  ``A`` and
                       ``B`` are percentages (``50%``) or decimals (``0.5``).
    --function NAME    Restrict transformations to functions with ``NAME``.
                       May be repeated.
    --show[:X]         Display the text that would be transformed. ``X`` may
                       be a transform name and the option may repeat.
    --only:X          Selectively apply the specified transform. May repeat.
"""

import argparse
import re
import io
import sys

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

    if argv is None:
        argv = sys.argv[1:]

    show_opts = []
    only_opts = []
    clean = []
    for arg in argv:
        if arg.startswith("--show"):
            if arg == "--show":
                show_opts.append(None)
            elif arg.startswith("--show:"):
                show_opts.append(arg.split(":", 1)[1])
            else:
                show_opts.append(None)
        elif arg.startswith("--only:"):
            only_opts.append(arg.split(":", 1)[1])
        else:
            clean.append(arg)

    parser = argparse.ArgumentParser(
        prog="better-varchar.py",
        usage="better-varchar.py [options] <input-pc-file> <output-pc-file>",
        description="Replace basic VARCHAR actions with macros.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("input_pc_file")
    parser.add_argument("output_pc_file")
    parser.add_argument("--version", action="version", version=VERSION)
    parser.add_argument(
        "--lines",
        metavar="START:END",
        help="Only transform the given line range (1-indexed)",
    )
    parser.add_argument(
        "--fraction",
        metavar="A:B",
        help=(
            "Only transform a fraction of the file. Arguments are percentages"
            " (50%%) or decimals (0.5)."
        ),
    )
    parser.add_argument(
        "--function",
        action="append",
        metavar="NAME",
        help="Restrict transformations to the given function name. May repeat",
    )
    parser.add_argument(
        "--show",
        metavar="TRANSFORM",
        nargs="?",
        action="append",
        help="Display text that would be transformed. Optionally specify transform name.",
    )
    parser.add_argument(
        "--only",
        action="append",
        metavar="TRANSFORM",
        help="Selectively apply the given transform. May repeat",
    )
    args = parser.parse_args(clean)
    args.show = show_opts
    args.only = only_opts
    return args


def transform(text, base=0, show=None, only=None):
    """Run all source transformations on ``text``.

    Each helper performs a specialised regex substitution.  The
    transformations are deliberately ordered so that more specific
    patterns (such as the ``v_copy`` form) run before the more generic
    ``vp_copy`` or ``v_sprintf`` replacements.
    """

    if only:
        only = set(o.replace('-', '_') for o in only)
    text = replace_setlenz(text, base, show) if not only or 'setlenz' in only else text
    text = (
        replace_v_copy_1(text, base, show)
        if not only or 'v_copy_1' in only else text
    )
    text = (
        replace_v_copy_2(text, base, show)
        if not only or 'v_copy_2' in only else text
    )
    text = replace_vp_copy(text, base, show) if not only or 'vp_copy' in only else text
    text = (
        replace_v_sprintf(text, base, show)
        if not only or 'v_sprintf' in only else text
    )
    text = replace_zsetlen(text, base, show) if not only or 'zsetlen' in only else text
    return text


def _parse_line_range(spec, total):
    """Return (start, end) tuple from a ``START:END`` specification."""

    start_s, end_s = spec.split(":", 1)
    start = int(start_s) - 1 if start_s else 0
    end = int(end_s) if end_s else total
    if start < 0:
        start = 0
    if end > total:
        end = total
    return start, end


def _parse_fraction(spec, total):
    """Translate a fractional range into (start, end) line numbers."""

    def as_float(part):
        if part.endswith("%"):
            return float(part[:-1]) / 100.0
        return float(part)

    start_s, end_s = spec.split(":", 1)
    start = as_float(start_s) if start_s else 0.0
    end = as_float(end_s) if end_s else 1.0
    start = max(0, min(1, start))
    end = max(0, min(1, end))
    return int(total * start), int(total * end)


def _function_ranges(lines, names):
    """Return a list of (start, end) ranges for the given function names."""

    if not names:
        return []
    pattern = re.compile(r"^.*\b(" + "|".join(map(re.escape, names)) + r")\s*\(")
    ranges = []
    i = 0
    while i < len(lines):
        if pattern.search(lines[i]):
            start = i
            brace = 0
            while i < len(lines):
                brace += lines[i].count("{") - lines[i].count("}")
                if brace > 0:
                    break
                i += 1
            i += 1
            while i < len(lines) and brace > 0:
                brace += lines[i].count("{") - lines[i].count("}")
                i += 1
            ranges.append((start, i))
        else:
            i += 1
    return ranges


# Regular expression fragment that loosely matches a C identifier possibly
# followed by member or array access.  It intentionally accepts characters
# such as ``->`` and ``[]`` so that structures like ``foo->bar[0]`` are
# recognised when used with the macros.
_VAR = r"[-A-Za-z0-9_.->\[\]]+"


def replace_setlenz(text, base=0, show=None):
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
    if show:
        for m in pattern.finditer(text):
            line = base + text.count("\n", 0, m.start()) + 1
            show("setlenz", line, m.group(0))
    return pattern.sub(
        lambda m: "%sVARCHAR_SETLENZ(%s);" % (m.group('indent'), m.group('var')),
        text,
    )


def replace_v_copy_1(text, base=0, show=None):
    """Collapse ``strcpy`` + length assignment into ``v_copy``.

    This looks for a call to ``strcpy`` where both arguments are the
    ``.arr`` members of ``VARCHAR`` variables.  The function also
    expects the caller to manually NUL terminate the destination using
    ``dst.arr[src.len] = '\0';``.  When such a pattern is found it is
    replaced with the single ``v_copy(dst, src);`` macro call.
    """

    pattern = re.compile(
        r"(?P<indent>^[ \t]*)strcpy\(\s*(?:\(char\*\))?\s*(?P<dst>" + _VAR +
        r")\.arr,\s*(?:\(char\*\))?\s*(?P<src>" + _VAR +
        r")\.arr\s*\);\s*(?:\n\s*)?(?P=dst)\.arr\[(?P=src)\.len\]\s*=\s*'\0';",
        re.MULTILINE,
    )
    if show:
        for m in pattern.finditer(text):
            line = base + text.count("\n", 0, m.start()) + 1
            show("v_copy_1", line, m.group(0))
    return pattern.sub(
        lambda m: "%sv_copy(%s, %s);" % (m.group('indent'), m.group('dst'), m.group('src')),
        text,
    )


def replace_v_copy_2(text, base=0, show=None):
    """Collapse ``strcpy`` of .arr to .arr into ``v_copy``.

    This looks for a call to ``strcpy`` where both arguments are the
    ``.arr`` members of ``VARCHAR`` variables.  When such a pattern is
    found it is replaced with the single ``v_copy(dst, src);`` macro call.
    """
    pattern = re.compile(
        r'(?P<indent>^[ \t]*)strcpy\(\s*(?:\(char\*\))?\s*(?P<dst>' + _VAR +
        r')\.arr,\s*(?:\(char\*\))?\s*(?P<src>' + _VAR + r')\.arr\s*\);',
        re.MULTILINE,
    )
    if show:
        for m in pattern.finditer(text):
            line = base + text.count("\n", 0, m.start()) + 1
            show("v_copy_2", line, m.group(0))
    return pattern.sub(
        lambda m: "%sv_copy(%s, %s);" % (m.group('indent'), m.group('dst'), m.group('src')),
        text,
    )


def replace_vp_copy(text, base=0, show=None):
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
    if show:
        for m in pattern.finditer(text):
            line = base + text.count("\n", 0, m.start()) + 1
            show("vp_copy", line, m.group(0))
    return pattern.sub(
        lambda m: "%svp_copy(%s, \"%s\");" % (m.group('indent'), m.group('foo'), m.group('lit')),
        text,
    )


def replace_v_sprintf(text, base=0, show=None):
    """Turn ``sprintf`` calls operating on ``VARCHAR`` buffers into ``VARCHAR_sprintf``.

    The function captures everything after the destination buffer so that
    arbitrary format strings and argument lists are preserved.
    """

    pattern = re.compile(
        r"(?P<indent>^[ \t]*)sprintf\(\s*(?P<foo>" + _VAR +
        r")\.arr,\s*(?P<args>[^;]*?)\)\s*;",
        re.MULTILINE,
    )
    if show:
        for m in pattern.finditer(text):
            line = base + text.count("\n", 0, m.start()) + 1
            show("v_sprintf", line, m.group(0))
    return pattern.sub(
        lambda m: "%sVARCHAR_sprintf(%s, %s);" % (m.group('indent'), m.group('foo'), m.group('args')),
        text,
    )


def replace_zsetlen(text, base=0, show=None):
    """Convert strlen-based length assignments to ``VARCHAR_ZSETLEN``.

    This handles statements like ``VAR.len = strlen((char*) VAR.arr);`` and
    emits ``VARCHAR_ZSETLEN(VAR);``.  The source variable used inside
    ``strlen`` is ignored so the pattern applies even when the left-hand side
    and argument differ.
    """

    pattern = re.compile(
        r"(?P<indent>^[ \t]*)(?P<dst>" + _VAR + r")\.len\s*=\s*strlen\(\s*(?:\(char\*\))?\s*" + _VAR + r"\.arr\s*\)\s*;",
        re.MULTILINE,
    )
    if show:
        for m in pattern.finditer(text):
            line = base + text.count("\n", 0, m.start()) + 1
            show("zsetlen", line, m.group(0))
    return pattern.sub(
        lambda m: "%sVARCHAR_ZSETLEN(%s);" % (m.group('indent'), m.group('dst')),
        text,
    )


def main(argv=None):
    """Entry point used by the ``__main__`` block and tests."""

    args = parse_args(argv)

    show_all = False
    show_filters = set()
    if args.show:
        for item in args.show:
            if item is None:
                show_all = True
            else:
                show_filters.add(item.replace('-', '_'))

        def _show(name, line, text):
            if show_all or name in show_filters:
                print(f"{name:<15} {line:5d}: {text.replace('\n', r'\\n')}")
    else:
        _show = None
    only_filters = [o.replace('-', '_') for o in args.only] if args.only else None
    with io.open(args.input_pc_file, "r", encoding="utf-8") as fh:
        data = fh.read()

    lines = data.splitlines(True)
    total = len(lines)

    start, end = 0, total
    if args.lines:
        start, end = _parse_line_range(args.lines, total)
    if args.fraction:
        fs, fe = _parse_fraction(args.fraction, total)
        start = max(start, fs)
        end = min(end, fe)

    segments = [(start, end)]
    func_ranges = _function_ranges(lines, args.function) if args.function else []
    if func_ranges:
        segments = []
        for fr in func_ranges:
            s = max(start, fr[0])
            e = min(end, fr[1])
            if s < e:
                segments.append((s, e))
    result_lines = lines[:]
    for s, e in segments:
        chunk = "".join(lines[s:e])
        chunk = transform(chunk, base=s + 1, show=_show, only=only_filters)
        result_lines[s:e] = chunk.splitlines(True)

    result = "".join(result_lines)

    with io.open(args.output_pc_file, "w", encoding="utf-8") as fh:
        fh.write(result)


if __name__ == "__main__":
    main()
