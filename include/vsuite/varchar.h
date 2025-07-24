#ifndef VARCHAR_MACROS_V_H
#define VARCHAR_MACROS_V_H

#include <string.h>
#include <ctype.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

#ifndef V_WARN
#define V_WARN(fmt, ...) /* */
#endif

/**
 * VARCHAR() - Declare a fixed-size Oracle style VARCHAR structure.
 * @name: name of the variable to declare.
 * @size: number of characters the buffer can hold.
 *
 * This macro expands to a structure containing a ``len`` field and a
 * character array of ``size`` bytes.  The string data is **not** automatically
 * zero terminated; ``len`` tracks the number of valid bytes in ``arr``.
 *
 * Example::
 *
 *     VARCHAR(buf, 16);
 *     strcpy(buf.arr, "hi");
 *     buf.len = 2;
 */
#define VARCHAR(name, size) struct { unsigned short len; char arr[size]; } name

/*
 * ``varchar_t`` is a helper type used internally to compute array types.
 * It declares a one character VARCHAR which mirrors the layout of any
 * VARCHAR instance.  ``varchar_buf_t`` resolves to the type of the character
 * array within a VARCHAR.
 */
typedef VARCHAR(varchar_t, 1);
/* Resolve the buffer type of a VARCHAR instance */
#define varchar_buf_t typeof(((varchar_t *)0)->arr)

/*
 * V_SIZE() - Return the total capacity of a VARCHAR buffer.
 * @v: VARCHAR variable.
 */
#define V_SIZE(v) (sizeof((v).arr))

/*
 * V_BUF() - Yield the underlying character array of a VARCHAR.
 * @v: VARCHAR variable.
 */
#define V_BUF(v)  ((v).arr)

/*
 * v_has_capacity() - Test if @v can hold @N bytes.
 * @v:  VARCHAR variable being queried.
 * @N:  Number of characters to check for.
 *
 * Returns true when the destination buffer has at least @N bytes of space.
 */
#define v_has_capacity(v, N) \
    ((N) <= V_SIZE(v))

/*
 * v_unused_capacity() - Return number of unused bytes
 * @v:  VARCHAR variable being queried.
 *
 * Returns the number of remaining bytes available
 */
#define v_unused_capacity(v) \
    ((v.len) > V_SIZE(v) ? 0 : (V_SIZE(v) - (v).len))

/*
 * v_has_unused_capacity() - Test if @v can hold @N additional bytes.
 * @v:  VARCHAR variable being queried.
 * @N:  Number of characters to check for.
 *
 * Returns true when the destination buffer has at least @N bytes of unused space.
 */
#define v_has_unused_capacity(v,N) \
    ((N) <= v_unused_capacity(v))

/*
 * v_init() - Reset a VARCHAR to an empty state.
 * @v: VARCHAR variable to modify.
 */
#define v_init(v) (((v).len = 0) || memset(V_BUF(v), '\0', V_SIZE(v)))

/*
 * v_valid() - Validate that @v.len does not exceed the buffer size.
 */
#define v_valid(v) ((v).len <= V_SIZE(v))

/*
 * v_clear() - Alias for v_init(); provided for readability.
 */
#define v_clear(v) v_init(v)

/*
 * v_copy() - Copy one VARCHAR into another.
 * @dest: Destination VARCHAR that receives the data.
 * @src:  Source VARCHAR to copy from.
 *
 * The copy only succeeds when @dest has enough capacity for ``src.len`` bytes.
 * On success the bytes are copied with ``memmove`` and the destination length
 * is updated.  The number of bytes copied is returned.  If the destination is
 * too small, ``dest.len`` is cleared to zero and ``0`` is returned.
 */
#define v_copy(dest, src)                                                      \
    ((V_SIZE(dest) >= (src).len)                                               \
        ? ( /* sufficient space */                                             \
           memmove(V_BUF(dest), V_BUF(src), (src).len),                        \
           (dest).len = (src).len,                                             \
           (src).len)                                                          \
        : ( /* overflow, clear dest */                                         \
           (dest).len = 0,                                                     \
           0))

/*
 * v_ltrim() - Remove leading ASCII whitespace from a VARCHAR.
 *
 * Characters are shifted left in-place using ``memmove`` when leading
 * whitespace is detected.  ``len`` is adjusted to reflect the new size.
 */
#define v_ltrim(v) do {                                            \
    size_t i = 0;                                                  \
    while (i < (v).len && isspace((unsigned char)V_BUF(v)[i]))      \
        i++;                                                       \
    if (i > 0) {                                                   \
        memmove(V_BUF(v), V_BUF(v) + i, (v).len - i);              \
        (v).len -= i;                                              \
    }                                                             \
} while (0)

/*
 * v_rtrim() - Strip trailing ASCII whitespace from a VARCHAR.
 */
#define v_rtrim(v) do {                                            \
    while ((v).len > 0 &&                                         \
           isspace((unsigned char)V_BUF(v)[(v).len - 1])) {        \
        (v).len--;                                                 \
    }                                                             \
} while (0)

/*
 * v_trim() - Convenience wrapper to run both v_rtrim() and v_ltrim().
 */
#define v_trim(v)  \
    do { v_rtrim(v); v_ltrim(v); } while (0)

/*
 * v_upper() - In-place ASCII uppercase conversion.
 */
#define v_upper(v) do {                                  \
    for (size_t i = 0; i < (v).len; i++)                 \
        V_BUF(v)[i] = toupper((unsigned char)V_BUF(v)[i]); \
} while (0)

/*
 * v_lower() - In-place ASCII lowercase conversion.
 */

#define v_lower(v) do {                                  \
    for (size_t i = 0; i < (v).len; i++)                 \
        V_BUF(v)[i] = tolower((unsigned char)V_BUF(v)[i]); \
} while (0)

/*
 * v_sprintf() - Print formatted data into a VARCHAR.
 * @v:   Destination VARCHAR variable.
 * @fmt: ``printf`` style format string.
 * @...: Arguments consumed according to @fmt.
 *
 * The macro formats into a temporary buffer sized one byte larger than the
 * destination so that ``vsnprintf`` can NUL terminate the result.  When the
 * formatted string fits, the bytes are copied into ``v`` and the number of
 * characters written is returned.  On overflow or formatting error ``v.len`` is
 * cleared to zero and ``0`` is returned.
 */
#define V_SPRINTF_OVERFLOW_EMPTY 0
#define V_SPRINTF_OVERFLOW_TRUNCATE 1
static inline int v_sprintf_fcn(char *dst_buf, unsigned short capacity,
                                unsigned short *dst_len, unsigned *overflow, const char *fmt, ...)
{
    *overflow = 0;

    if (capacity <= 0) { // Without capacity, there is nothing to do.
        return -1;
    }

    va_list ap;
    char tmp[65536]; // 64 KB

    va_start(ap, fmt);
    int n = vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);

    if (n < 0) { // On error, do nothing but ensure destination is an empty string
        if (capacity ) {
            *dst_buf = '\0';
        }
        return n;
    }

    if (n > capacity) {
        *overflow = n - capacity;
        n = capacity;
        tmp[n-1] = '\0';
    }

    memcpy(dst_buf, tmp, (size_t) n);
    *dst_len = (unsigned short) n - 1; // VARCHAR length does not include zero-byte terminator
    return n;
}

#define v_sprintf(v, fmt, ...) \
    ({ \
        unsigned capacity = V_SIZE(v); \
        unsigned overflow = 0; \
        int n = v_sprintf_fcn(V_BUF(v), capacity, &(v).len, &overflow, fmt, ##__VA_ARGS__); \
        if (overflow > 0) { \
            V_WARN("Line %d : v_sprintf(%s, fmt, ...) : overflow : bytes required %u > %u capacity : fmt = \"%s\"", \
                __LINE__, overflow+capacity, capacity, fmt); \
        } \
        n; \
    })

#endif /* VARCHAR_MACROS_V_H */
