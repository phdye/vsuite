# Contributing to VSuite

Thank you for considering a contribution to **VSuite**, a small set of C macros
for working with Oracle-style `VARCHAR` strings.  This document outlines how to
report issues, submit pull requests, and follow the project's coding conventions.

## Table of Contents

1. [Reporting Issues](#reporting-issues)
2. [Creating Pull Requests](#creating-pull-requests)
3. [Running the Test Suite](#running-the-test-suite)
4. [Coding Guidelines](#coding-guidelines)
   - [Macro Naming](#macro-naming)
   - [Inline Helpers](#inline-helpers)
   - [Unit Tests](#unit-tests)
5. [Style and Formatting](#style-and-formatting)

## Reporting Issues

If you encounter a bug or have a feature request, please open an issue on the
project's GitHub page. Provide a clear description of the problem and, if
possible, a minimal code sample demonstrating the issue. Discussion in the issue
tracker helps us clarify requirements before code changes begin.

## Creating Pull Requests

1. Fork the repository and create a topic branch for your work.
2. Make your changes on that branch, committing logical steps with descriptive
   messages.
3. Ensure the test suite passes (see below).
4. Open a pull request from your branch and reference any relevant issues.
   Describe the purpose of the change and any additional context.

## Running the Test Suite

The `tests` directory contains a Makefile that builds the unit tests. Use the
following commands from the project root:

```sh
cd tests
make      # builds test-*.exe programs
make test # optional, executes all tests
```

All pull requests should run these tests to ensure nothing regresses.

## Coding Guidelines

### Macro Naming

VSuite uses short prefixes based on argument types and follows names similar to
the C standard library. The design notes in `doc/Concept.md` list the prefixes:

```text
- 'v_'  -- Fixed VARCHAR only
- 'x_'  -- Pointers to VARCHAR
- 'vf_' -- Fixed VARCHAR  ← fixed C string
- 'fv_' -- Fixed C string ← fixed VARCHAR
- 'vd_' -- Fixed VARCHAR  ← dynamic C string
- 'dv_' -- Dynamic string ← fixed VARCHAR
```

When adding new functionality, mimic the corresponding C library name. For
example, a fixed/dynamic comparison routine might be called `dv_strcmp`.

### Inline Helpers

Simple helper functions that need to appear in headers should be declared
`static inline`. For instance, `dv_dup_fcn` is defined this way in
`include/vsuite/cstr.h`:

```c
static inline char *dv_dup_fcn(const char *src_buf, unsigned short src_len)
{
    char *d = malloc(src_len + 1);
    if (!d) return NULL;
    memcpy(d, src_buf, src_len);
    d[src_len] = '\0';
    return d;
}
```

### Unit Tests

Every macro or helper should have corresponding coverage in the `tests`
subdirectory. When adding new macros, include a test file or extend an existing
one to exercise the new behaviour. Running `make test` should succeed before you
submit your pull request.

## Style and Formatting

- Keep macro names in lowercase with underscores (e.g., `v_copy`).
- Follow the existing indentation style (spaces are preferred).
- Document any non-obvious behaviour with comments.

We appreciate your help making VSuite better! If you have questions about these
guidelines, please open an issue and we will be happy to discuss.
