#ifndef ZSUITE_ZV_H
#define ZSUITE_ZV_H

#include <string.h>
#include <stddef.h>
#include <ctype.h>

#include <vsuite/varchar.h>

/* zv_valid: Check if a fixed VARCHAR is zero-terminated */
#define zv_valid(v) \
    (((v).len < V_SIZE(v)) && (V_BUF(v)[(v).len] == '\0'))

/* zv_zero_term: force zero-termination (truncate if necessary) */
#define zv_zero_term(v)                          \
    do {                                         \
        if ((v).len >= V_SIZE(v))                \
            (v).len = V_SIZE(v) - 1;             \
        V_BUF(v)[(v).len] = '\0';               \
    } while (0)

/* zv_init: zero-length and terminate */
#define zv_init(v)                    \
    do {                              \
        (v).len = 0;                  \
        if (V_SIZE(v) > 0)            \
            V_BUF(v)[0] = '\0';      \
    } while (0)

/* zv_clear: like zv_init, always leave terminator */
#define zv_clear(v) zv_init(v)

/* zv_copy: like v_copy but guarantees \0 termination */
#define zv_copy(dest, src)                                                      \
    ((V_SIZE(dest) > (src).len)                                                 \
        ? (memmove(V_BUF(dest), V_BUF(src), (src).len),                          \
           (dest).len = (src).len,                                              \
           V_BUF(dest)[(dest).len] = '\0',                                     \
           (src).len)                                                           \
        : ((dest).len = (V_SIZE(dest) > 0 ? V_SIZE(dest) - 1 : 0),              \
           V_SIZE(dest) > 0 ? (V_BUF(dest)[(dest).len] = '\0') : (void)0,      \
           0))

/* zv_ltrim: call v_ltrim then re-zero-terminate */
#define zv_ltrim(v) do { v_ltrim(v); zv_zero_term(v); } while (0)

/* zv_rtrim: call v_rtrim then re-zero-terminate */
#define zv_rtrim(v) do { v_rtrim(v); zv_zero_term(v); } while (0)

/* zv_trim: call v_trim then re-zero-terminate */
#define zv_trim(v)  do { v_trim(v); zv_zero_term(v); } while (0)

/* zv_upper: identical to v_upper */
#define zv_upper(v) v_upper(v)

/* zv_lower: identical to v_lower */
#define zv_lower(v) v_lower(v)

#endif /* ZSUITE_ZV_H */
