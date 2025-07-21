## ✅ Phase 1: Core Fixed-Only Unary Utilities (`v_`)

### Goal: Basic manipulations of single fixed `VARCHAR` instances.

These are self-contained operations (unary, no dependencies beyond the object itself):

| Macro/Function | Description                                                        |
| -------------- | ------------------------------------------------------------------ |
| `v_init()`     | Initialize (clear length, null-terminate) a fixed `VARCHAR`.       |
| `v_valid()`    | Validate structure: length < size, null-terminated, within bounds. |
| `v_clear()`    | Wipe content but retain allocation.                                |
| `v_nulterm()`  | Ensure NUL termination (with optional validation of space).        |
| `v_ltrim()`    | Trim leading whitespace.                                           |
| `v_rtrim()`    | Trim trailing whitespace.                                          |
| `v_trim()`     | Trim both sides.                                                   |
| `v_upper()`    | Convert contents to uppercase.                                     |
| `v_lower()`    | Convert contents to lowercase.                                     |
| `v_dup()`      | Copy content to another fixed `VARCHAR` of equal or larger size.   |

---

## ✅ Phase 2: Character-Level Search and Scan (`v_`)

### Goal: Read-only lookup and search functions.

Safe to implement once validation routines are available.

| Macro/Function   | Description                                        |
| ---------------- | -------------------------------------------------- |
| `v_index(c)`     | Index of first occurrence of char `c`.             |
| `v_rindex(c)`    | Index of last occurrence of char `c`.              |
| `v_strstr(sub)`  | Substring search.                                  |
| `v_strpbrk(set)` | Return first char from `set` present in `VARCHAR`. |
| `v_strspn(set)`  | Length of span consisting only of chars in `set`.  |
| `v_strcspn(set)` | Length of span consisting of chars not in `set`.   |

---

## ✅ Phase 3: Fixed Binary Utilities (`v_`, `vf_`, `fv_`)

### Goal: Comparisons and assignment between fixed types.

Now that you have single-object support, begin pairwise operations between:

* Fixed `VARCHAR` (`v_`)
* Fixed C-string (`f_`)

| Macro/Function        | Description                                                 |
| --------------------- | ----------------------------------------------------------- |
| `v_copy(fixed_src)`   | Copy from one fixed `VARCHAR` to another.                   |
| `vf_copy(fixed_cstr)` | Copy from fixed C-string into fixed `VARCHAR`.              |
| `fv_copy(varchar)`    | Copy from fixed `VARCHAR` to fixed C-string.                |
| `v_cmp(v2)`           | Case-sensitive compare with another fixed `VARCHAR`.        |
| `vf_cmp(cstr)`        | Compare with a fixed C-string.                              |
| `v_casecmp()`         | Case-insensitive variant of compare.                        |
| `v_concat()`          | Append one fixed `VARCHAR` to another, safe capacity aware. |

---

## ✅ Phase 4: Formatting & Construction

### Goal: Build string content from format strings, numerics, etc.

| Macro/Function            | Description                          |
| ------------------------- | ------------------------------------ |
| `v_sprintf(fmt, ...)`     | Format into fixed `VARCHAR`.         |
| `v_snprintf(n, fmt, ...)` | Safe version with cap.               |
| `v_set(char, len)`        | Fill with `len` copies of `char`.    |
| `v_from_int(n)`           | Format integer into fixed `VARCHAR`. |

---

## ✅ Phase 5: Mixed-Type Interop and Fallbacks

### Goal: Enable interop with dynamic C strings, char pointers.

Only proceed once fixed operations are solid.

| Prefix        | Description                                    |
| ------------- | ---------------------------------------------- |
| `dv_`         | dynamic string (`malloc`-backed) ↔ fixed `VARCHAR`. |
| `vp_` / `pv_` | `VARCHAR` ↔ `char *`, possibly unknown origin. |
| `vx_` / `xv_` | `VARCHAR` ↔ `VARCHAR *`.                       |

This layer will require runtime awareness of allocation safety, copying semantics, etc.

---

## ✅ Phase 6: Pointer (`x_`) and Hybrid (`vx_`, `xv_`) Ops

### Goal: Add full pointer and hybrid variants.

Reimplement phases 1–3 with pointer variants.

---

## Suggested Implementation Order Summary

| Phase | Type    | Scope              | Depends On   |
| ----- | ------- | ------------------ | ------------ |
| 1     | Unary   | Fixed `VARCHAR`    | None         |
| 2     | Search  | Fixed `VARCHAR`    | Phase 1      |
| 3     | Binary  | `v_`, `vf_`, `fv_` | Phases 1,2   |
| 4     | Format  | Fixed `VARCHAR`    | Phase 1      |
| 5     | Interop | Mixed types        | Phases 1–3   |
| 6     | Pointer | `x_`, `vx_`, etc.  | All previous |

---
