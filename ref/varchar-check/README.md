# varchar/check

This directory contains a small collection of helper macros used to
validate and manipulate Oracle style `VARCHAR` structures.  The macros
are intended for use in Pro*C or C programs that work with the classic
`VARCHAR` layout (`unsigned short len; char arr[N];`).  They perform
runtime checks and convenience operations to help catch programming
errors when dealing with these buffers.

## Building the test suite

A simple test program is provided to exercise all macros.  To build and
run it:

```bash
cd varchar/check
make
./test-suite.exe
```

Use `--verbose` or `-v` to display each individual test result.

## Files

- `varchar-check.h` – macro definitions with extensive inline
  documentation.
- `test-suite.c` – comprehensive unit tests for all macros.
- `Makefile` – minimal build script for the test suite.

The test suite does not depend on an Oracle client installation and can
be built with any standard C compiler.
