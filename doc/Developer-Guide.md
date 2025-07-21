# Developer Guide

This guide summarizes the design of VSuite and explains the naming conventions for the macros.  It also shows basic usage patterns and how overflow conditions are handled.

## Table of Contents

- [Design Overview](#design-overview)
- [Prefix Conventions](#prefix-conventions)
- [Usage Examples](#usage-examples)
- [Error Handling and Truncation](#error-handling-and-truncation)
- [Macro Reference](#macro-reference)
  - [Declaration](#declaration)
  - [VARCHAR utilities (`varchar.h`)](#core-utilities-varcharh)
  - [Zero-terminated variant (`zvarchar.h`)](#zero-terminated-variant-zvarcharh)
  - [Interop helpers](#interop-helpers)
    - VARCHAR <-> Fixed:
      - [`fixed.h`](#fixedh)
    - VARCHAR <-> Dynamic:
      - [`cstr.h`](#cstrh)
- [Further Reading](#further-reading)

## Design Overview

VSuite provides small macros to manipulate strings that mimic Oracle's host `VARCHAR` structures.  A fixed string is declared as:

```c
VARCHAR(name, SIZE); /* struct with `len` and `arr[SIZE]` */
```

Operations fall into three broad categories:

- **Fixed `VARCHAR`** – stack or static structures where the size is part of the type.
- **Dynamic C strings** – `malloc`-allocated buffers.
- **Pointer variants** – macros that operate on pointers to the above types.

Using these macros allows code that interacts with Oracle databases to work with the same field layout while still interchanging with ordinary C strings.

## Prefix Conventions

Two-letter prefixes indicate destination and source types.  They are formed as `<dst><src>_` using the following type codes:

| Code | Meaning                        |
|------|--------------------------------|
| `v`  | fixed `VARCHAR`                |
| `x`  | pointer to a `VARCHAR`         |
| `f`  | fixed C string (array)         |
| `d`  | dynamic C string (`malloc`)    |
| `p`  | `char *` of unknown storage    |

Examples of prefixes:

| Prefix | Destination → Source                                    |
|--------|---------------------------------------------------------|
| `v_`   | unary/binary on a fixed `VARCHAR`                        |
| `x_`   | unary/binary on a `VARCHAR*`                             |
| `vf_`  | fixed `VARCHAR`  ← fixed C string                        |
| `fv_`  | fixed C string   ← fixed `VARCHAR`                       |
| `vd_`  | fixed `VARCHAR`  ← dynamic C string                      |
| `dv_`  | dynamic C string ← fixed `VARCHAR`                       |
| `vp_`  | fixed `VARCHAR`  ← `char *`                              |
| `pv_`  | `char *`         ← fixed `VARCHAR`                       |
| `vx_`  | fixed `VARCHAR`  ← pointer to `VARCHAR`                  |
| `xv_`  | pointer to `VARCHAR` ← fixed `VARCHAR`                   |

Zero‑terminated variants prefix the table above with `z` (`zv_`, `zvf_`, …) to guarantee that the destination buffer is NUL terminated.

## Usage Examples

Copy between two fixed structures:

```c
VARCHAR(src, 16);
VARCHAR(dst, 16);
strcpy(src.arr, "hello");
src.len = 5;

v_copy(dst, src);           /* copies when dst is large enough */
```

Copy into a dynamic string:

```c
char buf[8];
VARCHAR(v, 8);
strcpy(v.arr, "hi");
v.len = 2;

/* writes NUL on failure */
dv_copy(buf, sizeof buf, v);
```

## Error Handling and Truncation

Most copy macros check capacity before writing.  If the destination is too small, the operation fails and either resets the `len` field (for `v_` variants) or writes an empty string (`\0`) when working with C strings.  The `zv_` forms truncate to `size-1` and always append a terminator.

## Macro Reference

The following sections list every macro currently implemented in VSuite along
with short examples demonstrating their typical use.

### Declaration

- `VARCHAR(name, SIZE)` – declare a fixed structure holding a length and
  character array.

```c
VARCHAR(buf, 32);   /* buf.len and buf.arr available */
```

`varchar_t` and `varchar_buf_t` provide type aliases for a generic
`VARCHAR` and its buffer field.

### Core utilities (`varchar.h`)

- `V_SIZE(v)` – capacity of `v` in bytes.
- `V_BUF(v)` – pointer to the underlying array.
- `v_init(v)` – set length to zero.

```c
VARCHAR(tmp, 8);
v_init(tmp);                    /* tmp.len == 0 */
```

- `v_valid(v)` – test that `len` is within bounds.

```c
if (!v_valid(tmp)) { /* handle overflow */ }
```

- `v_clear(v)` – clear contents without touching the buffer.
- `v_copy(dest, src)` – copy from `src` into `dest`, returning bytes copied or
  zero on failure.

```c
VARCHAR(a, 8); VARCHAR(b, 8);
/* ... populate a ... */
if (v_copy(b, a) == 0) {
    /* destination too small */
}
```

- `v_ltrim(v)`, `v_rtrim(v)`, `v_trim(v)` – remove leading and/or trailing
  whitespace.

```c
strcpy(tmp.arr, "  hi  ");
tmp.len = 6;
v_trim(tmp);                    /* results in "hi" */
```

- `v_upper(v)` and `v_lower(v)` – convert case in place.

```c
strcpy(tmp.arr, "Abc");
tmp.len = 3;
v_upper(tmp);                   /* "ABC" */
```

### Interop helpers

#### `cstr.h`

- `dv_copy(dst, cap, src)` – copy a fixed `VARCHAR` into a dynamic buffer.

```c
char out[5];
dv_copy(out, sizeof out, tmp);
```

- `dv_dup(src)` – allocate a new C string containing the contents of `src`.

```c
char *dup = dv_dup(tmp);
free(dup);
```

- `vd_copy(vdst, dsrc)` – copy from a dynamic C string into a fixed
  `VARCHAR`.

```c
const char *dyn = getenv("HOME");
vd_copy(tmp, dyn);
```

#### `fixed.h`

- `fv_copy(dst, src)` – copy a fixed `VARCHAR` into a fixed array.

```c
char fixed[8];
fv_copy(fixed, tmp);
```

- `vf_copy(dst, csrc)` – copy from a fixed C string into a `VARCHAR`.

```c
VARCHAR(v, 8);
vf_copy(v, "hello");
```

### Zero-terminated variant (`zvarchar.h`)

These macros mirror the `v_` operations but guarantee that the destination is
NUL terminated.

- `zv_valid(v)` – verify length and terminator.
- `zv_zero_term(v)` – force a terminator, truncating if necessary.
- `zv_init(v)` / `zv_clear(v)` – initialise an empty string.
- `zv_copy(dest, src)` – copy while always leaving space for `\0`.
- `zv_ltrim(v)`, `zv_rtrim(v)`, `zv_trim(v)` – trimming operations.
- `zv_upper(v)`, `zv_lower(v)` – case conversion.

```c
VARCHAR(z, 4);
strcpy(z.arr, "abc");
z.len = 3;
zv_zero_term(z);               /* safe even when full */
```

## Further Reading

- [Concept overview](Concept.md)
- [Implementation roadmap](Implementation-Roadmap.md)
