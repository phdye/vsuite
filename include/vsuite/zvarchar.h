#ifndef ZSUITE_ZV_H
#define ZSUITE_ZV_H

#include <string.h>
#include <stddef.h>
#include <ctype.h>

#include <vsuite/varchar.h>

/*
 * ZV_CAPACITY() - Number of bytes usable for data in a zero-terminated VARCHAR.
 * @v: VARCHAR variable.
 */
#define ZV_CAPACITY(v) (V_SIZE(v) - 1)

/*
 * zv_has_capacity() - Test if a zvarchar can hold @N characters plus the
 * terminating NUL byte.
 */
#define zv_has_capacity(v, N) \
    ((N) <= ZV_CAPACITY(v))

/*
 * zv_valid() - Verify that ``v`` is correctly NUL terminated.
 */
#define zv_valid(v) \
    (((v).len < V_SIZE(v)) && (V_BUF(v)[(v).len] == '\0'))

/*
 * zv_zero_terminate() - Ensure a VARCHAR is properly NUL terminated.  If the length
 * exceeds or equals the buffer size, the string is truncated, then terminated.
 */
#define zv_zero_terminate(v)                     \
    do {                                         \
        if ((v).len >= V_SIZE(v))                \
            (v).len = V_SIZE(v) - 1;             \
        V_BUF(v)[(v).len] = '\0';                \
    } while (0)

/*
 * zv_init() - Reset a zvarchar to an empty terminated string.
 */
#define zv_init(v)                    \
    do {                              \
        (v).len = 0;                  \
        if (V_SIZE(v) > 0)            \
            V_BUF(v)[0] = '\0';      \
    } while (0)

/*
 * zv_clear() - Alias of zv_init(); provided for readability.
 */
#define zv_clear(v) zv_init(v)

/*
 * zv_copy() - Copy one VARCHAR into another ensuring NUL termination.
 *
 * The destination is only updated when it has sufficient capacity.  On success
 * the source bytes are copied and a terminator written.  If the destination is
 * too small it is truncated to leave space for the terminator and zero is
 * returned.
 */
#define zv_copy(dest, src) \
    ((V_SIZE(dest) > (src).len) \
        ? (memmove(V_BUF(dest), V_BUF(src), (src).len), \
           (dest).len = (src).len, \
           V_BUF(dest)[(dest).len] = '\0', \
           (src).len) \
        : ((dest).len = (V_SIZE(dest) > 0 ? V_SIZE(dest) - 1 : 0), \
           V_SIZE(dest) > 0 ? (V_BUF(dest)[(dest).len] = '\0') : (void)0, \
           0))

/*
 * zv_ltrim() - Call v_ltrim() and then re-apply zero termination.
 */
#define zv_ltrim(v) do { v_ltrim(v); zv_zero_terminate(v); } while (0)

/*
 * zv_rtrim() - Call v_rtrim() and then re-apply zero termination.
 */
#define zv_rtrim(v) do { v_rtrim(v); zv_zero_terminate(v); } while (0)

/*
 * zv_trim() - Trim both ends and keep the terminator intact.
 */
#define zv_trim(v)  do { v_trim(v); zv_zero_terminate(v); } while (0)

/*
 * zv_upper() - Uppercase conversion preserving the terminator.
 */
#define zv_upper(v) v_upper(v)

/*
 * zv_lower() - Lowercase conversion preserving the terminator.
 */
#define zv_lower(v) v_lower(v)

#endif /* ZSUITE_ZV_H */
