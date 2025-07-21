VSuite
======

VSuite is a lightweight collection of C macros for working with Oracle-style
`VARCHAR` data.  It focuses on fixed-size structures that hold a `len` field and
an array of characters, mirroring the common representation used when binding
Oracle strings in host programs.

## Directory structure

- `include/vsuite` – public header files providing the `VARCHAR` declaration and
  utility macros.
- `tests` – unit tests with a `Makefile` used to build sample programs.
- `doc` – design notes and additional documentation.

## Building

The tests assume a standard `make` environment and GCC.  To build everything,
run the following from the `tests` directory:

```sh
make      # builds test-*.exe programs
make test # optional, executes all tests
```

## Minimal example

The headers expose a `VARCHAR(name, size)` macro to declare a fixed-size string.
Most operations are implemented as macros.  The snippet below copies one
`VARCHAR` to another using `v_copy`:

```c
#include <string.h>
#include <vsuite/v.h>

int main(void) {
    VARCHAR(src, 16);
    VARCHAR(dst, 16);

    strcpy(src.arr, "hello");
    src.len = 5;

    v_copy(dst, src); /* dst now contains "hello" */
    return 0;
}
```

For additional macros and guidelines see the Developer Guide in `doc` and the
`CONTRIBUTING` page.
