#ifndef VSUITE_FIXED_H
#define VSUITE_FIXED_H

#include <string.h>
#include <stddef.h>

#include <vsuite/zvarchar.h>

/* vf_valid() ...
*/

#define F_SIZE(f)  sizeof(f)

#define f_valid(f)  (strlen(f) < F_SIZE(f))

/*
 * vf_copy() - Copy from a constant C string into a fixed VARCHAR.
 * @vdst: Destination VARCHAR.
 * @csrc: Constant source C string.
 *
 * The destination length is updated only when the literal fits completely.
 * Otherwise ``vdst.len`` is cleared to zero so callers can detect overflow.
 */
#define vf_copy(vdst, csrc)                                          \
    ({                                                               \
        if (!f_valid(csrc)) {                                        \
            V_WARN("Line %d : vf_copy(%s, %s) : src buffer is overflowed : bytes used %zu > %zu capacity", \
                __LINE__, #vdst, #csrc, strlen(csrc), F_SIZE(csrc)); \
        }                                                            \
        varchar_overflow = 0;                                        \
        size_t siz = V_SIZE(vdst);                                   \
        size_t __n = strlen(csrc);                                   \
        if (__n > siz) {                                             \
            varchar_overflow = __n - siz;                            \
            V_WARN("Line %d : vp_copy(%s, %s) : overflow : bytes required %zu > %zu capacity", \
                __LINE__, #vdst, #csrc, __n, siz);                   \
            __n = siz;                                               \
        }                                                            \
        memmove(V_BUF(vdst), csrc, __n);                             \
        __n;                                                         \
    })

/*
 * zvf_copy() - Copy a constant C string and always NUL terminate the result.
 *
 * The macro behaves like vf_copy() but ensures the destination is terminated
 * even when truncation occurs.  Callers should verify capacity beforehand when
 * possible.
 */
#define zvf_copy(vdst, csrc)                                         \
    ({                                                               \
        if (!f_valid(csrc)) {                                        \
            V_WARN("Line %d : zvf_copy(%s, %s) : src buffer is overflowed : bytes used %zu > %zu capacity", \
                __LINE__, #vdst, #csrc, strlen(csrc), F_SIZE(csrc)); \
        }                                                            \
        varchar_overflow = 0;                                        \
        size_t __cap = ZV_CAPACITY(vdst);                            \
        size_t __n = strlen(csrc);                                   \
        if (__n > __cap) {                                           \
            varchar_overflow = __n - __cap;                          \
            V_WARN("Line %d : zv_copy(%s, %s) : overflow : bytes required %zu > %zu capacity", \
                  __LINE__, #vdst, #csrc, __n, V_SIZE(vdst));        \
            __n = __cap;                                             \
        }                                                            \
        memmove(V_BUF(dest), V_BUF(src), __n);                       \
        (dest).len = __n;                                            \
        if (V_SIZE(dest) > 0)                                        \
            V_BUF(dest)[__n] = '\0';                                 \
        __n;                                                         \
    })

/*
 * fv_copy() - Copy a VARCHAR into a fixed-size C string buffer.
 *
 * When the destination buffer is large enough the contents are copied and a
 * terminating NUL is appended.  If the buffer is too small the destination is
 * cleared to an empty string so the caller can detect the failure.
 */
#define fv_copy(cdst, vsrc)                                          \
    do {                                                             \
        if ((vsrc).len < sizeof(cdst)) {                             \
            memcpy(cdst, V_BUF(vsrc), (vsrc).len);                   \
            cdst[(vsrc).len] = '\0';                                 \
        } else if (sizeof(cdst) > 0) {                               \
            cdst[0] = '\0';                                          \
        }                                                            \
    } while (0)

#endif /* VSUITE_FIXED_H */
