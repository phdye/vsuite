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
#define vp_copy(vdst, dsrc)                                                   \
    do {                                                                      \
        size_t __n = strlen(dsrc);                                            \
        if (__n < V_SIZE(vdst)) {                                             \
            memcpy(V_BUF(vdst), dsrc, __n);                                   \
            (vdst).len = __n;                                                 \
        } else {                                                              \
            (vdst).len = 0;                                                   \
        }                                                                     \
    } while (0)

/*
 * zvp_copy() - Copy a C string pointer and always NUL terminate the result.
 */
#define zvp_copy(vdst, dsrc)                                                  \
    do {                                                                      \
        vp_copy(vdst, dsrc);                                                  \
        zv_zero_term(vdst);                                                   \
    } while (0)

/*
 * pv_copy() - Copy a VARCHAR into a preallocated C string buffer.
 *
 * ``dstr`` must have capacity ``dcap``. When the source fits the data is copied
 * and a terminator appended.  Otherwise the destination is cleared to an empty
 * string (if ``dcap`` is non-zero).
 */
#define pv_copy(dstr, dcap, vsrc)                                             \
    do {                                                                      \
        if ((vsrc).len < (dcap)) {                                            \
            memcpy(dstr, V_BUF(vsrc), (vsrc).len);                            \
            dstr[(vsrc).len] = '\0';                                          \
        } else if ((dcap) > 0) {                                              \
            dstr[0] = '\0';                                                   \
        }                                                                     \
    } while (0)

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

#endif /* VSUITE_PSTR_H */
