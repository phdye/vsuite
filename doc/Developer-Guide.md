# Developer Guide

This guide summarizes the design of VSuite and explains the naming conventions for the macros.  It also shows basic usage patterns and how overflow conditions are handled.

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

## Further Reading

- [Concept overview](Concept.md)
- [Implementation roadmap](Implementation-Roadmap.md)
