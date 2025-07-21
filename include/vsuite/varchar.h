#ifndef VARCHAR_MACROS_V_H
#define VARCHAR_MACROS_V_H

#include <string.h>
#include <ctype.h>
#include <stddef.h>

/* Declaration macro for fixed VARCHAR */
#define VARCHAR(name, size) struct { unsigned short len; char arr[size]; } name

// typeof(((struct { unsigned short len; char arr[1]; }){0}).arr)
typedef VARCHAR(varchar_t, 1);
#define varchar_buf_t typeof(((varchar_t *)0)->arr)

/* Internal helper to compute size from VARCHAR */
#define V_SIZE(v) (sizeof((v).arr))
#define V_BUF(v)  ((v).arr)

/* Check if Fixed VARCHAR has at least capacity N */
#define v_has_capacity(v, N) \
    ((v).len < (N) && (N) <= V_SIZE(v))

/* Initialize a fixed VARCHAR to empty string */
#define v_init(v) ((v).len = 0)

/* Check validity of a VARCHAR: len <= size */
#define v_valid(v) ((v).len <= V_SIZE(v))

/* Clear the content of the VARCHAR (reset len only) */
#define v_clear(v) ((v).len = 0)

/* Copy into another fixed VARCHAR; returns number of bytes copied or 0 on failure */
#define v_copy(dest, src)                                                      \
    ((V_SIZE(dest) >= (src).len)                                               \
        ? (memmove(V_BUF(dest), V_BUF(src), (src).len),                        \
           (dest).len = (src).len,                                             \
           (src).len)                                                          \
        : ((dest).len = 0, 0))

/* Trim left whitespace */
#define v_ltrim(v) do {                                            \
    size_t i = 0;                                                  \
    while (i < (v).len && isspace((unsigned char)V_BUF(v)[i])) i++; \
    if (i > 0) {                                                   \
        memmove(V_BUF(v), V_BUF(v) + i, (v).len - i);              \
        (v).len -= i;                                              \
    }                                                             \
} while (0)

/* Trim right whitespace */
#define v_rtrim(v) do {                                            \
    while ((v).len > 0 && isspace((unsigned char)V_BUF(v)[(v).len - 1])) { \
        (v).len--;                                                 \
    }                                                             \
} while (0)

/* Trim both sides */
#define v_trim(v)  \
    do { v_rtrim(v); v_ltrim(v); } while (0)

/* Convert to uppercase */
#define v_upper(v) do {                                  \
    for (size_t i = 0; i < (v).len; i++)                 \
        V_BUF(v)[i] = toupper((unsigned char)V_BUF(v)[i]); \
} while (0)

/* Convert to lowercase */
#define v_lower(v) do {                                  \
    for (size_t i = 0; i < (v).len; i++)                 \
        V_BUF(v)[i] = tolower((unsigned char)V_BUF(v)[i]); \
} while (0)

#endif /* VARCHAR_MACROS_V_H */
