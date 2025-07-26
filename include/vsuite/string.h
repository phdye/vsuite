#ifndef VARCHAR_MACROS_S_H
#define VARCHAR_MACROS_S_H

#include <string.h>
#include <ctype.h>
/*
 * S_SIZE() - Return the total capacity of fixed C-String
 * @s: Fixed allocation C-String
 */
#define S_SIZE(s) (sizeof(s))

/*
 * s_has_capacity() - Test if @s can hold @N bytes.
 * @s:  Fixed C-String variable being queried.
 * @N:  Number of characters to check for.
 *
 * Returns true when the destination buffer has at least @N bytes of space.
 */
#define s_has_capacity(s, N) ((s) <= S_SIZE(s))

/*
 * s_unused_capacity() - Return number of unused bytes
 * @s:  VARCHAR variable being queried.
 *
 * Returns the number of remaining bytes available.
 */
#define s_unused_capacity(s) (S_SIZE(v) - strlen(s))

/*
 * s_has_unused_capacity() - Test if @s can hold @N additional bytes.
 * @s:  Fixed C-String variable being queried.
 * @N:  Number of characters to check for.
 *
 * Returns true when the destination buffer has at least ``N`` bytes of unused
 * space.  Overflowed lengths are treated as having zero free space.
 */
#define s_has_unused_capacity(s,N) \
    ((N) <= s_unused_capacity(s))

/*
 * v_init() - Reset a VARCHAR to an empty state.
 * @v: VARCHAR variable to modify.
 *
 * Sets ``len`` to zero and fills the underlying buffer with ``'\0'``
 * bytes to ensure any previous contents are cleared.
 */
#define s_init(v) do { if (S_SIZE(s) > 0) { (s)[0] = '\0'); } } while(0)

/*
 * s_valid() - Validate that @s does not exceed the buffer size.
 */
#define s_valid(s) (strlen(s) <= S_SIZE(s))

/*
 * s_clear() - Alias for s_init(); provided for readability.
 */
#define s_clear(s) \
    ({                                                                     \
        varchar_overflow = 0;                                              \
        size_t siz = S_SIZE(s);                                            \
        if (siz > 0) {                                                     \
            memset((s), '\0', siz);                                        \
        } else {                                                           \
            V_WARN("Line %d : s_clear(%s) : overflow : 0 size string cannot be cleared.", \
                __LINE__, #s);                                             \
        }                                                                  \
        siz;                                                               \
    })

/*
 * v_copy() - Copy one VARCHAR into another.
 * @dest: Destination VARCHAR that receives the data.
 * @src:  Source VARCHAR to copy from.
 *
 * Copies ``src`` into ``dest`` truncating when the destination buffer is
 * smaller than ``src.len``.  The destination ``len`` field is **not** modified.
 * The number of bytes moved (after truncation) is returned and any overflow is
 * recorded in ``varchar_overflow``.
 */
#define v_copy(dest, src)                                                  \
    ({                                                                     \
        varchar_overflow = 0;                                              \
        size_t __n = (src).len;                                            \
        if (__n > V_SIZE(dest)) {                                          \
            varchar_overflow = __n - V_SIZE(dest);                         \
            V_WARN("Line %d : v_copy(%s, %s) : overflow : bytes required %zu > %zu capacity", \
                __LINE__, #dest, #src, __n, V_SIZE(dest));                 \
            __n = V_SIZE(dest);                                            \
        }                                                                  \
        memmove(V_BUF(dest), V_BUF(src), __n);                             \
        __n;                                                               \
    })

/*
 * v_strncpy() - Copy at most n characters from src to dest.
 * @dest: Destination VARCHAR that receives the data.
 * @src:  Source VARCHAR to copy from.
 * @n:    Maximum number of characters to copy.
 * 
 * Copies up to ``n`` characters from ``src`` into ``dest``.  If the requested
 * length exceeds the destination capacity the copy is truncated and the number
 * of bytes moved is returned.  ``dest.len`` remains unchanged and overflow is
 * reported via ``varchar_overflow``.
 */
#define v_strncpy(dest, src, n)                                            \
    ({                                                                     \
        varchar_overflow = 0;                                              \
        size_t __n = (n);                                                  \
        if (__n > (src).len)                                               \
            __n = (src).len;                                               \
        if (__n > V_SIZE(dest)) {                                          \
            varchar_overflow = __n - V_SIZE(dest);                         \
            V_WARN("Line %d : v_strncpy(%s, %s, %u) : overflow : bytes required %zu > %zu capacity", \
                __LINE__, #dest, #src, (n), __n, V_SIZE(dest));            \
            __n = V_SIZE(dest);                                            \
        }                                                                  \
        memmove(V_BUF(dest), V_BUF(src), __n);                             \
        __n;\
    })

/*
 * v_strcat() - Append src to dest.
 * @dest: Destination VARCHAR that receives the data.
 * @src:  Source VARCHAR to append.
 *
 * Appends ``src`` to ``dest`` without NUL termination.  When ``dest`` does not
 * have enough unused space only the portion that fits is appended.  The
 * resulting ``len`` is increased by the number of bytes appended and that value
 * is returned.  Overflow information is stored in ``varchar_overflow``.
 */
#define v_strcat(dest, src)                                        \
    ({                                                             \
        varchar_overflow = 0;                                      \
        size_t __avail = v_unused_capacity(dest);                  \
        size_t __n = (src).len;                                    \
        if (__n > __avail) {                                       \
            varchar_overflow = __n - __avail;                      \
            V_WARN("Line %d : v_strcat(%s, %s) : overflow : bytes required %zu > %zu capacity", \
                    __LINE__, #dest, #src, __n, V_SIZE(dest));     \
            __n = __avail;                                         \
        }                                                          \
        memmove(V_BUF(dest) + (dest).len, V_BUF(src), __n);        \
        (dest).len += __n;                                         \
        (int)__n;                                                  \
    })

/*
 * v_strncat() - Append at most n characters from src to dest.
 *
 * Concatenates up to ``n`` bytes from ``src``.  When space is insufficient the
 * data is truncated and only the part that fits is appended.  The destination
 * length increases by the number of bytes appended which is also returned.  Any
 * truncation is noted in ``varchar_overflow``.
 */
#define v_strncat(dest, src, n)                                    \
    ({                                                             \
        varchar_overflow = 0;                                      \
        size_t __avail = v_unused_capacity(dest);                  \
        size_t __n = (n);                                          \
        if (__n > (src).len)                                       \
            __n = (src).len;                                       \
        if (__n > __avail) {                                       \
            varchar_overflow = __n - __avail;                      \
            V_WARN("Line %d : v_strncat(%s, %s, %u) : overflow : bytes required %zu > %zu capacity", \
                __LINE__, #dest, #src, (n), __n, V_SIZE(dest));    \
            __n = __avail;                                         \
        }                                                          \
        memmove(V_BUF(dest) + (dest).len, V_BUF(src), __n);        \
        (dest).len += __n;                                         \
        (int)__n;                                                  \
    })

/*
 * v_ltrim() - Remove leading ASCII whitespace from a VARCHAR.
 *
 * Characters are shifted left in-place using ``memmove`` when leading
 * whitespace is detected.  ``len`` is adjusted to reflect the new size.
 */
#define v_ltrim(v) do {                                            \
    size_t i = 0;                                                  \
    while (i < (v).len && isspace((unsigned char)V_BUF(v)[i]))     \
        i++;                                                       \
    if (i > 0) {                                                   \
        memmove(V_BUF(v), V_BUF(v) + i, (v).len - i);              \
        (v).len -= i;                                              \
    }                                                              \
} while (0)

/*
 * v_rtrim() - Strip trailing ASCII whitespace from a VARCHAR.
 *
 * The length is decremented while whitespace characters are found at the end
 * of the buffer.  Content is left in place; only ``len`` changes.
 */
#define v_rtrim(v) do {                                            \
    while ((v).len > 0 &&                                          \
           isspace((unsigned char)V_BUF(v)[(v).len - 1])) {        \
        (v).len--;                                                 \
    }                                                              \
} while (0)

/*
 * v_trim() - Convenience wrapper to run both v_rtrim() and v_ltrim().
 *
 * Leading and trailing whitespace are removed as described by the individual
 * helpers.
 */
#define v_trim(v)  \
    do { v_rtrim(v); v_ltrim(v); } while (0)

/*
 * v_upper() - In-place ASCII uppercase conversion.
 *
 * Each byte is converted with ``toupper`` using unsigned char promotion.
 */
#define v_upper(v) do {                                  \
    for (size_t i = 0; i < (v).len; i++)                 \
        V_BUF(v)[i] = toupper((unsigned char)V_BUF(v)[i]); \
} while (0)

/*
 * v_lower() - In-place ASCII lowercase conversion.
 *
 * Like ``v_upper`` but using ``tolower``.
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
 * The macro writes formatted output directly into the destination buffer.  The
 * underlying ``vsnprintf`` call always receives the full capacity of ``v`` so
 * a trailing NUL byte is produced.  The returned value excludes this
 * terminator and represents the number of bytes stored in ``v``.  If the
 * formatted data exceeds the capacity it is truncated, ``varchar_overflow`` is
 * set and ``v.len`` is updated to the truncated size.  A formatting error
 * results in ``v`` becoming an empty string and the negative error code is
 * propagated.
 */
static inline int v_sprintf_fcn(char *dst_buf, size_t capacity,
                                unsigned short *dst_len, const char *fmt, ...) {
    varchar_overflow = 0;

    // Without capacity, there is nothing to do.
    if (capacity <= 0) {
        return -1;
    }

    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(dst_buf, capacity, fmt, ap);
    va_end(ap);

    // fprintf(stderr, "v_sprintf() => %2d : strlen() = %2zu : '%s'\n", n, strlen(dst_buf), dst_buf);

    // On error, do nothing but ensure destination is an empty string
    if (n < 0) { 
        *dst_buf = '\0';
        *dst_len = 0;
        return n;
    }

    if ((unsigned) n > capacity) {
        varchar_overflow = n - capacity;
        V_WARN("Line %d : v_sprintf_fcn(%s, fmt, ...) : overflow : bytes required %d > %zu capacity : fmt = \"%s\"", 
                __LINE__, "dst_buf", n, capacity, fmt);
        n = capacity;
    }

    // vsnprintf does not includes the terminator in the count.

    *dst_len = (unsigned short)n;
    return n;
}

#define v_sprintf(v, fmt, ...) \
    ({ \
        size_t capacity = V_SIZE(v); \
        int n = v_sprintf_fcn(V_BUF(v), capacity, &(v).len, fmt, ##__VA_ARGS__); \
        if (varchar_overflow > 0) { \
            V_WARN("Line %d : v_sprintf(%s, fmt, ...) : overflow : bytes required %zu > %zu capacity : fmt = \"%s\"", \
                __LINE__, #v, varchar_overflow+capacity, capacity, fmt); \
        } \
        n; \
    })

#endif /* VARCHAR_MACROS_V_H */
