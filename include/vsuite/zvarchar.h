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
 * zv_setlenz() - Ensure a VARCHAR is properly NUL terminated.  If the length
 * exceeds or equals the buffer size, the string is truncated, then terminated.
 * 
 * 'setlenz' - set zero-byte terminator using .len
 */
#define zv_setlenz(v) \
    do { \
        if ( !v_valid(v)) { \
            V_WARN("Line %d : VARCHAR_SETLENZ:  %s : length %u exceeds allocated size %zu : truncating to actual size\n\n", \
                __LINE__, #v, (v).len, sizeof((v).arr)); \
            (v).len = V_SIZE(v); \
        } else { \
            if ( !v_has_unused_capacity(v, 1)) { \
                V_WARN("Line %d : VARCHAR_SETLENZ(%s) : does not have an unused byte for the string terminator\n\n", \
                    __LINE__, #v); \
                V_WARN("Line %d : VARCHAR_SETLENZ(%s) : len %u == %zu size (presumably equal) : will lose trailing '%c'\n\n", \
                    __LINE__, #v, (v).len, V_SIZE(v), V_BUF(v)[V_SIZE(v)-1] ); \
                (v).len = V_SIZE(v) - 1; \
            } \
        } \
        V_BUF(v)[(v).len] = '\0'; \
    } while (0)

#define zv_zero_terminate(v) zv_setlenz(v)

/*
 * FIND_FIRST_NUL_BYTE() - Locate the first NUL in ``arr``.
 * @arr:     Character array to scan.
 * @max_len: Maximum number of bytes to examine.
 *
 * Returns a pointer to the first ``'\0'`` within the given range or ``NULL``
 * when none is present.
 */
#define FIND_FIRST_NUL_BYTE(arr,max_len)       \
    ({                                         \
        char * found = NULL;                   \
        for (size_t i = 0; i < max_len; ++i) { \
            if (arr[i] == '\0') {              \
                found = arr + i;               \
                break;                         \
            }                                  \
        }                                      \
        found;                                 \
    })

/*
 * zv_zsetlen() - Determine the length of a NUL terminated VARCHAR.
 * @v: VARCHAR variable whose ``len`` should reflect the first terminator.
 *
 * Scans the buffer up to its declared size looking for ``'\0'``.  ``v.len`` is
 * updated to the index of that terminator.  When none is found a diagnostic is
 * written and ``v.len`` becomes an undefined large value, but the last array
 * element is forced to ``'\0'`` so subsequent operations remain bounded.
 * 
 *  'zsetlen' - use zero-byte terminator to set .len
 */
#define zv_zsetlen(v) \
    { \
        unsigned siz = sizeof((v).arr); \
        char *nul = FIND_FIRST_NUL_BYTE((v).arr, siz); \
        if (nul == NULL) { \
            V_WARN("Line %d : zv_zsetlen(%s) : No NUL byte found within %u sizeof(.arr) bytes : value '%s'\n", \
                    __LINE__, #v, siz, (v).arr); \
            nul = (v).arr + siz - 1; /* point to the last valid byte */ \
        } \
        (v).len = (unsigned short)((unsigned long)nul - (unsigned long)(v).arr); \
        (v).arr[(v).len] = '\0'; \
    }

/*
 * zv_init() - Reset a zvarchar to an empty terminated string.
 */
#define zv_init(v)                    \
    do {                              \
        (v).len = 0;                  \
        if (V_SIZE(v) > 0)            \
            V_BUF(v)[0] = '\0';       \
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
#define zv_copy(dest, src)                                          \
    ({                                                              \
        varchar_overflow = 0;                                       \
        size_t __cap = ZV_CAPACITY(dest);                           \
        size_t __n = (src).len;                                     \
        if (__n > __cap) {                                          \
            varchar_overflow = __n - __cap;                         \
            V_WARN("Line %d : zv_copy(%s, %s) : overflow : bytes required %zu > %zu capacity", \
                  __LINE__, #dest, #src, __n, V_SIZE(dest));        \
            __n = __cap;                                            \
        }                                                           \
        memmove(V_BUF(dest), V_BUF(src), __n);                      \
        (dest).len = __n;                                           \
        if (V_SIZE(dest) > 0)                                       \
            V_BUF(dest)[__n] = '\0';                                \
        __n;                                                        \
    })

/*
 * zv_strncpy() - Copy at most n characters and ensure termination.
 */
#define zv_strncpy(dest, src, n)                                   \
    ({                                                             \
        varchar_overflow = 0;                                      \
        size_t __cap = ZV_CAPACITY(dest);                          \
        size_t __n = (n);                                          \
        if (__n > (src).len)                                       \
            __n = (src).len;                                       \
        if (__n > __cap) {                                         \
            varchar_overflow = __n - __cap;                        \
            V_WARN("Line %d : zv_strncpy(%s, %s, %u) : overflow : bytes required %zu > %zu capacity", \
                   __LINE__, #dest, #src, (unsigned)(n), __n, V_SIZE(dest)); \
            __n = __cap;                                           \
        }                                                          \
        memmove(V_BUF(dest), V_BUF(src), __n);                     \
        (dest).len = __n;                                          \
        if (V_SIZE(dest) > 0)                                      \
            V_BUF(dest)[__n] = '\0';                               \
        (int)__n;                                                  \
    })

/*
 * zv_strcat() - Append src to dest with preserved terminator.
 */
#define zv_strcat(dest, src)                                      \
    ({                                                            \
        varchar_overflow = 0;                                     \
        size_t __avail = (V_SIZE(dest) > (dest).len)              \
                            ? V_SIZE(dest) - 1 - (dest).len       \
                            : 0;                                  \
        size_t __n = (src).len;                                   \
        if (__n > __avail) {                                      \
            varchar_overflow = __n - __avail;                     \
            V_WARN("Line %d : zv_strcat(%s, %s) : overflow : bytes required %zu > %zu capacity", \
                   __LINE__, #dest, #src, __n, V_SIZE(dest));     \
            __n = __avail;                                        \
        }                                                         \
        memmove(V_BUF(dest) + (dest).len, V_BUF(src), __n);       \
        (dest).len += __n;                                        \
        if (V_SIZE(dest) > 0)                                     \
            V_BUF(dest)[(dest).len] = '\0';                       \
        (int)__n;                                                 \
    })

/*
 * zv_strncat() - Append at most n characters from src to dest and terminate.
 */
#define zv_strncat(dest, src, n)                                   \
    ({                                                             \
        varchar_overflow = 0;                                      \
        size_t __avail = (V_SIZE(dest) > (dest).len)               \
                            ? V_SIZE(dest) - 1 - (dest).len        \
                            : 0;                                   \
        size_t __n = (n);                                          \
        if (__n > (src).len)                                       \
            __n = (src).len;                                       \
        if (__n > __avail) {                                       \
            varchar_overflow = __n - __avail;                      \
            V_WARN("Line %d : zv_strncat(%s, %s, %u) : overflow : bytes required %zu > %zu capacity", \
                   __LINE__, #dest, #src, (unsigned)(n), __n, V_SIZE(dest)); \
            __n = __avail;                                         \
        }                                                          \
        memmove(V_BUF(dest) + (dest).len, V_BUF(src), __n);        \
        (dest).len += __n;                                         \
        if (V_SIZE(dest) > 0)                                      \
            V_BUF(dest)[(dest).len] = '\0';                        \
        (int)__n;                                                  \
    })

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
