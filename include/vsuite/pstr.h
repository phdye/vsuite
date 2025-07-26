#ifndef VSUITE_PSTR_H
#define VSUITE_PSTR_H

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include <vsuite/varchar.h>

/*
 * vp_copy() - Copy from a C string pointer into a fixed VARCHAR.
 * @vdst: Destination VARCHAR.
 * @dsrc: Source C string pointer.
 *
 * The copy only succeeds when the source fits entirely within the destination
 * buffer.  Otherwise ``vdst.len`` is cleared to zero so callers can detect the
 * overflow.
 */
#define vp_copy(vdst, dsrc)                                          \
    ({                                                               \
        varchar_overflow = 0;                                        \
        size_t siz = V_SIZE(vdst);                                   \
        size_t __n = strlen(dsrc);                                   \
        if (__n > siz) {                                             \
            varchar_overflow = __n - siz;                            \
            V_WARN("Line %d : vp_copy(%s, %s) : overflow : bytes required %zu > %zu capacity", \
                __LINE__, #vdst, #dsrc, __n, siz);                   \
            __n = siz;                                               \
        }                                                            \
        memmove(V_BUF(vdst), dsrc, __n);                             \
        __n;                                                         \
    })

/*
 * zvp_copy() - Copy a C string pointer and always NUL terminate the result.
 */
#define zvp_copy(vdst, dsrc)                                         \
    ({                                                               \
        varchar_overflow = 0;                                        \
        size_t __cap = ZV_CAPACITY(vdst);                            \
        size_t __n = strlen(dsrc);                                   \
        if (__n > __cap) {                                           \
            varchar_overflow = __n - __cap;                          \
            V_WARN("Line %d : zvp_copy(%s, %s) : overflow : bytes required %zu > %zu capacity", \
                  __LINE__, #vdst, #dsrc, __n, V_SIZE(vdst));        \
            __n = __cap;                                             \
        }                                                            \
        memmove(V_BUF(vdst), dsrc, __n);                             \
        (vdst).len = __n;                                            \
        if (V_SIZE(vdst) > 0)                                        \
            V_BUF(vdst)[__n] = '\0';                                 \
        __n;                                                         \
    })

/*
 * pv_copy() - Copy a VARCHAR into a preallocated C string buffer.
 *
 * ``dstr`` must have capacity ``dcap``. When the source fits the data is copied
 * and a terminator appended.  Otherwise the destination is cleared to an empty
 * string (if ``dcap`` is non-zero).
 */
#define pv_copy(dstr, dcap, vsrc)                                    \
    ({                                                               \
        varchar_overflow = 0;                                        \
        size_t __cap = (dcap);                                       \
        size_t __n = (vsrc).len;                                     \
        if (__n >= __cap) {                                          \
            varchar_overflow = __n - __cap + 1;                      \
            V_WARN("Line %d : pv_copy(%s, %s) : overflow : bytes required %zu > %zu capacity", \
                  __LINE__, #dstr, #vsrc, __n, __cap);               \
            __n = __cap - 1;                                         \
        }                                                            \
        memmove(dstr, V_BUF(vsrc), __n);                             \
        dstr[__n] = '\0';                                            \
        __n + 1;                                                     \
    })

/* Convenience wrapper to duplicate a VARCHAR into a newly allocated C string */
#define dv_dup(v) dv_dup_fcn(V_BUF(v), (v).len)

/*
 * dv_dup_fcn() - Allocate and duplicate a VARCHAR into a dynamic C string.
 * @src_buf: Pointer to source character data.
 * @src_len: Length of the source data.
 *
 * Returns: a pointer to a newly allocated NUL terminated C string or ``NULL``
 * on allocation failure.
 */
static inline char *dv_dup_fcn(const char *src_buf, unsigned short src_len)
{
    char *d = malloc(src_len + 1); /* allocate space for data plus terminator */
    if (!d)
        return NULL;
    memcpy(d, src_buf, src_len);   /* copy bytes */
    d[src_len] = '\0';            /* append NUL terminator */
    return d;
}

/*
 * zvp_copy() - Copy a C string pointer and always NUL terminate the result.
 */
#define zvp_strcmp(vdst, dsrc)                                                \
    ({                                                                      \
        zv_valid(vdst);                                                       \
        strncmpzv_zero_terminate(vdst);                                              \
    })

#endif /* VSUITE_PSTR_H */
