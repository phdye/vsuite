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
#define s_has_capacity(s, N) ((N) <= S_SIZE(s))

/*
 * s_unused_capacity() - Return number of unused bytes
 * @s:  VARCHAR variable being queried.
 *
 * Returns the number of remaining bytes available.
 */
#define s_unused_capacity(s) \
    ({ \
        size_t __len = strnlen((s), S_SIZE(s)); \
        __len >= S_SIZE(s) ? 0 : S_SIZE(s) - __len - 1; \
    })

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
 * s_init() - Reset a C string buffer to an empty state.
 * @s: Fixed C string to modify.
 *
 * Fills the buffer with ``'\0'`` bytes to ensure any previous contents are
 * cleared.
 */
#define s_init(s)                                                          \
    ({                                                                     \
        size_t siz = S_SIZE(s);                                            \
        if (siz > 0) {                                                     \
            (s)[0] = '\0';                                                 \
            1;                                                             \
        } else {                                                           \
            V_WARN("Line %d : s_init(%s) : overflow : size 0 string cannot be initialized.", \
                __LINE__, #s);                                             \
            0;                                                             \
        }                                                                  \
    })

/*
 * s_valid() - Validate that @s does not exceed the buffer size.
 */
#define s_valid(s) (strnlen((s), S_SIZE(s)) < S_SIZE(s))

/*
 * s_clear() - Clear a C string buffer.
 */
#define s_clear(s) \
    ({                                                                     \
        size_t siz = S_SIZE(s);                                            \
        if (siz > 0) {                                                     \
            memset((s), '\0', siz);                                        \
        } else {                                                           \
            V_WARN("Line %d : s_clear(%s) : overflow : size 0 string cannot be cleared.", \
                __LINE__, #s);                                             \
        }                                                                  \
        siz;                                                               \
    })

/*
 * s_copy() - Copy one C string into another.
 * @dest: Destination C string that receives the data.
 * @src:  Source C string to copy from.
 *
 * Copies ``src`` into ``dest`` truncating when the destination buffer is
 * smaller than ``dest``.  The number of bytes moved (after truncation) is
 * returned and any overflow is recorded in ``varchar_overflow``.
 */
#define s_copy(dest, src)                                                   \
    ({                                                                      \
        varchar_overflow = 0;                                               \
        size_t __cap = S_SIZE(dest);                                        \
        size_t __n = strlen(src);                                           \
        if (__n >= __cap) {                                                 \
            varchar_overflow = __n - (__cap - 1);                           \
            V_WARN("Line %d : s_copy(%s, %s) : overflow : bytes required %zu > %zu capacity", \
                  __LINE__, #dest, #src, __n, __cap);                       \
            __n = __cap - 1;                                                \
        }                                                                   \
        memmove((dest), (src), __n);                                        \
        (dest)[__n] = '\0';                                                \
        __n;                                                                \
    })

/*
 * s_strncpy() - Copy at most n characters from src to dest.
 * @dest: Destination C string that receives the data.
 * @src:  Source C string to copy from.
 * @n:    Maximum number of characters to copy.
 * 
 * Copies up to ``n`` characters from ``src`` into ``dest``.  If the requested
 * length exceeds the destination capacity the copy is truncated and the number
 * of bytes moved is returned.  Overflow is reported via ``varchar_overflow``.
 */
#define s_strncpy(dest, src, n)                                             \
    ({                                                                      \
        varchar_overflow = 0;                                               \
        size_t __cap = S_SIZE(dest);                                        \
        size_t __n = (n);                                                   \
        size_t __src_len = strlen(src);                                     \
        if (__n > __src_len)                                                \
            __n = __src_len;                                                \
        if (__n >= __cap) {                                                 \
            varchar_overflow = __n - (__cap - 1);                           \
            V_WARN("Line %d : s_strncpy(%s, %s, %u) : overflow : bytes required %zu > %zu capacity", \
                  __LINE__, #dest, #src, (unsigned)(n), __n, __cap);        \
            __n = __cap - 1;                                                \
        }                                                                   \
        memmove((dest), (src), __n);                                        \
        (dest)[__n] = '\0';                                                \
        (int)__n;                                                           \
    })

/*
 * s_strcat() - Append src to dest.
 * @dest: Destination C string that receives the data.
 * @src:  Source C string to append.
 *
 * Appends ``src`` to ``dest`` without NUL termination.  When ``dest`` does not
 * have enough unused space only the portion that fits is appended.  The
 * resulting string length increases by the number of bytes appended and that
 * value is returned.  Overflow information is stored in ``varchar_overflow``.
 */
#define s_strcat(dest, src)                                         \
    ({                                                              \
        varchar_overflow = 0;                                       \
        size_t __dlen = strnlen((dest), S_SIZE(dest));              \
        size_t __avail = (__dlen < S_SIZE(dest))                    \
                            ? S_SIZE(dest) - 1 - __dlen             \
                            : 0;                                    \
        size_t __n = strlen(src);                                   \
        if (__n > __avail) {                                        \
            varchar_overflow = __n - __avail;                       \
            V_WARN("Line %d : s_strcat(%s, %s) : overflow : bytes required %zu > %zu capacity", \
                  __LINE__, #dest, #src, __n, S_SIZE(dest));        \
            __n = __avail;                                          \
        }                                                           \
        memmove((dest) + __dlen, (src), __n);                       \
        (dest)[__dlen + __n] = '\0';                                \
        (int)__n;                                                   \
    })

/*
 * s_strncat() - Append at most n characters from src to dest.
 *
 * Concatenates up to ``n`` bytes from ``src``.  When space is insufficient the
 * data is truncated and only the part that fits is appended.  The resulting
 * string length increases by the number of bytes appended which is also returned.
 * Any truncation is noted in ``varchar_overflow``.
 */
#define s_strncat(dest, src, n)                                     \
    ({                                                              \
        varchar_overflow = 0;                                       \
        size_t __dlen = strnlen((dest), S_SIZE(dest));              \
        size_t __avail = (__dlen < S_SIZE(dest))                    \
                            ? S_SIZE(dest) - 1 - __dlen             \
                            : 0;                                    \
        size_t __n = (n);                                           \
        size_t __src_len = strlen(src);                             \
        if (__n > __src_len)                                        \
            __n = __src_len;                                        \
        if (__n > __avail) {                                        \
            varchar_overflow = __n - __avail;                       \
            V_WARN("Line %d : s_strncat(%s, %s, %u) : overflow : bytes required %zu > %zu capacity", \
                  __LINE__, #dest, #src, (unsigned)(n), __n, S_SIZE(dest)); \
            __n = __avail;                                          \
        }                                                           \
        memmove((dest) + __dlen, (src), __n);                       \
        (dest)[__dlen + __n] = '\0';                                \
        (int)__n;                                                   \
    })

/*
 * s_ltrim() - Remove leading ASCII whitespace from a C string.
 *
 * Characters are shifted left in-place using ``memmove`` when leading
 * whitespace is detected.
 */
#define s_ltrim(s) do {                                            \
    size_t __len = strnlen((s), S_SIZE(s));                        \
    size_t i = 0;                                                  \
    while (i < __len && isspace((unsigned char)(s)[i]))            \
        i++;                                                       \
    if (i > 0) {                                                   \
        memmove((s), (s) + i, __len - i + 1);                      \
    }                                                              \
} while (0)

/*
 * s_rtrim() - Strip trailing ASCII whitespace from a C string.
 *
 * Characters are removed from the end while whitespace is encountered.
 */
#define s_rtrim(s) do {                                            \
    size_t __len = strnlen((s), S_SIZE(s));                        \
    while (__len > 0 &&                                           \
           isspace((unsigned char)(s)[__len - 1])) {               \
        __len--;                                                  \
    }                                                             \
    (s)[__len] = '\0';                                            \
} while (0)

/*
 * s_trim() - Convenience wrapper to run both s_rtrim() and s_ltrim().
 *
 * Leading and trailing whitespace are removed as described by the individual
 * helpers.
 */
#define s_trim(s)  \
    do { s_rtrim(s); s_ltrim(s); } while (0)

/*
 * s_upper() - In-place ASCII uppercase conversion.
 *
 * Each byte is converted with ``toupper`` using unsigned char promotion.
 */
#define s_upper(s) do {                                   \
    for (size_t i = 0; (s)[i] != '\0'; i++)              \
        (s)[i] = toupper((unsigned char)(s)[i]);          \
} while (0)

/*
 * s_lower() - In-place ASCII lowercase conversion.
 *
 * Like ``s_upper`` but using ``tolower``.
 */

#define s_lower(s) do {                                   \
    for (size_t i = 0; (s)[i] != '\0'; i++)              \
        (s)[i] = tolower((unsigned char)(s)[i]);          \
} while (0)

#endif /* VARCHAR_MACROS_S_H */
