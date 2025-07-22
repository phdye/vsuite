#ifndef VSUITE_FIXED_H
#define VSUITE_FIXED_H

#include <string.h>
#include <stddef.h>

#include <vsuite/zvarchar.h>

/*
 * vf_copy() - Copy from a constant C string into a fixed VARCHAR.
 * @vdst: Destination VARCHAR.
 * @csrc: Constant source C string.
 *
 * The destination length is updated only when the literal fits completely.
 * Otherwise ``vdst.len`` is cleared to zero so callers can detect overflow.
 */
#define vf_copy(vdst, csrc)                                                   \
    do {                                                                      \
        size_t __n = strlen(csrc);                                            \
        if (__n <= V_SIZE(vdst)) {                                            \
            memcpy(V_BUF(vdst), csrc, __n);                                   \
            (vdst).len = __n;                                                 \
        } else {                                                              \
            (vdst).len = 0;                                                   \
        }                                                                     \
    } while (0)

/*
 * zvf_copy() - Copy a constant C string and always NUL terminate the result.
 *
 * The macro behaves like vf_copy() but ensures the destination is terminated
 * even when truncation occurs.  Callers should verify capacity beforehand when
 * possible.
 */
#define zvf_copy(vdst, dsrc)                                                  \
    do {                                                                      \
        vf_copy(vdst, dsrc);                                                  \
        zv_zero_terminate(vdst);                                                   \
    } while (0)

/*
 * fv_copy() - Copy a VARCHAR into a fixed-size C string buffer.
 *
 * When the destination buffer is large enough the contents are copied and a
 * terminating NUL is appended.  If the buffer is too small the destination is
 * cleared to an empty string so the caller can detect the failure.
 */
#define fv_copy(cdst, vsrc)                                                   \
    do {                                                                      \
        if ((vsrc).len < sizeof(cdst)) {                                      \
            memcpy(cdst, V_BUF(vsrc), (vsrc).len);                            \
            cdst[(vsrc).len] = '\0';                                          \
        } else if (sizeof(cdst) > 0) {                                        \
            cdst[0] = '\0';                                                   \
        }                                                                     \
    } while (0)

#endif /* VSUITE_FIXED_H */
